// Account value.
//
// Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 Gregory W. Chicares.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
//
// https://savannah.nongnu.org/projects/lmi
// email: <gchicares@sbcglobal.net>
// snail: Chicares, 186 Belle Woods Drive, Glastonbury CT 06033, USA

#include "pchfile.hpp"

#include "account_value.hpp"

#include "alert.hpp"
#include "assert_lmi.hpp"
#include "calendar_date.hpp"
#include "contains.hpp"
#include "database.hpp"
#include "dbnames.hpp"
#include "death_benefits.hpp"
#include "ihs_irc7702.hpp"
#include "ihs_irc7702a.hpp"
#include "input.hpp"                    // consummate()
#include "interest_rates.hpp"
#include "ledger.hpp"
#include "ledger_invariant.hpp"
#include "ledger_variant.hpp"
#include "loads.hpp"
#include "materially_equal.hpp"
#include "miscellany.hpp"
#include "mortality_rates.hpp"
#include "outlay.hpp"
#include "premium_tax.hpp"
#include "ssize_lmi.hpp"
#include "stratified_algorithms.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>                     // back_inserter()
#include <limits>
#include <numeric>
#include <string>
#include <utility>

/*
We ideally want transaction functions to be reorderable.
That means each must be atomic and reentrant, with no dependencies.
To what extent is this feasible?

different sorts of variables:

constant input: never changes (e.g. COI rates)

changed by solves: e.g. withdrawals

changed by rules: e.g. withdrawals limited by minimum

changed and later reused: e.g. specamt (set in curr pass, used in guar pass)

These are not mutually exclusive. A withdrawal can change the specified
amount, but it must not wipe out the input specamt array (which can signal
reductions and increases).

Some rules change variables prospectively. For example, a withdrawal reduces
all future spec amts.

It might be helpful to prepare a table of transaction type cross variables
showing {accesses, modifies current year, modifies future years}

*/

//============================================================================
AccountValue::AccountValue(Input const& input)
    :BasicValues           (Input::consummate(input))
    ,DebugFilename         {"anonymous.monthly_trace"}
    ,Debugging             {false}
    ,Solving               {mce_solve_none != BasicValues::yare_input_.SolveType}
    ,SolvingForGuarPremium {false}
    ,ItLapsed              {false}
    ,ledger_{new Ledger(BasicValues::GetLength(), BasicValues::ledger_type(), BasicValues::nonillustrated(), BasicValues::no_can_issue(), false)}
    ,ledger_invariant_     {new LedgerInvariant(BasicValues::GetLength())}
    ,ledger_variant_       {new LedgerVariant  (BasicValues::GetLength())}
    ,SolveGenBasis_        {mce_gen_curr}
    ,SolveSepBasis_        {mce_sep_full}
    ,RunBasis_             {mce_run_gen_curr_sep_full}
    ,GenBasis_             {mce_gen_curr}
    ,SepBasis_             {mce_sep_full}
    ,OldDBOpt              {mce_option1}
    ,YearsDBOpt            {mce_option1}
{
    SetInitialValues();
    LMI_ASSERT(InforceYear < methuselah);
    PerformSpecAmtStrategy();
    PerformSupplAmtStrategy();
    InvariantValues().Init(this);

    set_partial_mortality();
    InvariantValues().InforceLives = partial_mortality_lx;

    // Explicitly initialize antediluvian members. It's generally
    // better to do this in the initializer-list, but here they can
    // all be kept together.
    LapseMonth               = 0;          // Antediluvian.
    LapseYear                = 0;          // Antediluvian.
    AVUnloaned               = 0.0;        // Antediluvian.
    pmt                      = 0.0;        // Antediluvian.
    pmt_mode                 = mce_annual; // Antediluvian.
    ModeIndex                = 0;          // Antediluvian.
    wd                       = 0.0;        // Antediluvian.
    mlyguarv                 = 0.0;        // Antediluvian.
    deathbft                 = 0.0;        // Antediluvian.
    haswp                    = false;      // Antediluvian.
    hasadb                   = false;      // Antediluvian.
    mlydedtonextmodalpmtdate = 0.0;        // Antediluvian.

    set_list_bill_year_and_month();

    OverridingEePmts    .resize(12 * BasicValues::GetLength());
    OverridingErPmts    .resize(12 * BasicValues::GetLength());

    OverridingLoan      .resize(BasicValues::GetLength());
    OverridingWD        .resize(BasicValues::GetLength());

    SurrChg_            .resize(BasicValues::GetLength());

    YearlyTaxBasis      .reserve(BasicValues::GetLength());
    YearlyNoLapseActive .reserve(BasicValues::GetLength());
    loan_ullage_        .reserve(BasicValues::GetLength());
    withdrawal_ullage_  .reserve(BasicValues::GetLength());
}

/// Specified amount (disregarding any term or "supplemental" amount).

double AccountValue::base_specamt(int year) const
{
    return InvariantValues().SpecAmt[year];
}

/// Specified amount of term rider.

double AccountValue::term_specamt(int year) const
{
    return InvariantValues().TermSpecAmt[year];
}

/// Specified amount for 7702 (not 7702A).

double AccountValue::specamt_for_7702(int year) const
{
    return
                              base_specamt(year)
        + (TermIsDbFor7702  ? term_specamt(year) : 0.0)
        ;
}

/// Specified amount for 7702A (not 7702).

double AccountValue::specamt_for_7702A(int year) const
{
    return
                              base_specamt(year)
        + (TermIsDbFor7702A ? term_specamt(year) : 0.0)
        ;
}

//============================================================================
std::shared_ptr<Ledger const> AccountValue::ledger_from_av() const
{
    LMI_ASSERT(ledger_.get());
    return ledger_;
}

//============================================================================
double AccountValue::RunAV()
{
/*
First run current, for solves and strategies. This determines
    payments
    specamt (increases, decreases, option changes)
    and hence surrchg
Then run other bases.

    guar and midpt values are useful only for illustrations
    for profit testing we want to avoid their overhead
    for solves we want only one run
        if solving on guar basis...does *that* basis determine pmts & specamt?
        it probably should, so that the guar columns will show what's wanted
            otherwise the solve lacks meaning
        although I wonder how other illustration systems handle this
        it should also be possible to solve on a midpt basis as well
*/

    if(contains(yare_input_.Comments, "idiosyncrasyZ"))
        {
        Debugging = true;
        DebugPrintInit();
        }

    double z = RunAllApplicableBases();

    FinalizeLifeAllBases();

    return z;
}

//============================================================================
// TODO ?? Perhaps commutation functions could be used to speed up
// this rather expensive function.
void AccountValue::SetGuarPrem()
{
    GuarPremium = 0.0;
    if(BasicValues::IsSubjectToIllustrationReg())
        {
        GuarPremium = SolveGuarPremium();
        }
    LMI_ASSERT(GuarPremium < 1.0e100);
    ledger_->SetGuarPremium(GuarPremium);
}

//============================================================================
double AccountValue::RunOneBasis(mcenum_run_basis a_Basis)
{
    if
        (  !BasicValues::IsSubjectToIllustrationReg()
        && mce_run_gen_mdpt_sep_full == a_Basis
        )
        {
        alarum()
            << "Midpoint basis defined only for illustration-reg ledger."
            << LMI_FLUSH
            ;
        }

    double z = 0.0;
    if(Solving)
        {
// Apparently this should never be done because Solve() is called in
//   RunAllApplicableBases() .
// TODO ?? Isn't this unreachable?
//      LMI_ASSERT(a_Basis corresponds to yare_input_.SolveExpenseGeneralAccountBasis);
//      z = Solve();
        }
    else
        {
        z = RunOneCell(a_Basis);
        }
    return z;
}

//============================================================================
// If not solving
//   if running one basis
//     just do that basis
//   if running all bases
//     run all bases
//
double AccountValue::RunAllApplicableBases()
{
    double z = 0.0;

    // TODO ?? Normally, running on the current basis determines the
    // overriding values for all components of outlay--e.g., premiums,
    // forceouts, loans, and withdrawals. For a solve on any basis
    // other than current, the overriding values could be determined
    // in two ways:
    //
    // (1) on the current basis--but then the solve won't be right;
    //
    // (2) on the solve basis: but if it lapses on that basis, then
    // overriding values were not determined for later durations,
    // yet such values may be needed e.g. for the current basis.

    if(Solving)
        {
        // TODO ?? This conditional tests the value of SolveGenBasis_,
        // which hasn't yet been assigned any contextual value. The
        // test is invalid anyway: the types don't match. Furthermore,
        // the purpose of the statement it controls is not pellucid;
        // it seems safer to execute it than not.
//        if(mce_run_gen_curr_sep_full != SolveGenBasis_)
            {
            RunOneBasis(mce_run_gen_curr_sep_full);
            }

        z = Solve
            (yare_input_.SolveType
            ,yare_input_.SolveBeginYear
            ,yare_input_.SolveEndYear
            ,yare_input_.SolveTarget
            ,yare_input_.SolveTargetValue
            ,yare_input_.SolveTargetYear
            ,yare_input_.SolveExpenseGeneralAccountBasis
            ,yare_input_.SolveSeparateAccountBasis
            );
        Solving = false;
        // TODO ?? Here we might save overriding parameters determined
        // on the solve basis.
        }
    // Run all bases, current first.
    for(auto const& b : ledger_->GetRunBases())
        {
        RunOneBasis(b);
        }
    return z;
}

//============================================================================
/// This implementation seems slightly unnatural because it strives
/// for similarity with run_census_in_parallel::operator(). For
/// instance, 'Year' and 'Month' aren't used directly as loop
/// counters, and the loop has no early-exit condition like
///   if(ItLapsed) break;
/// which isn't necessary anyway because all the functions it calls
/// contain such a condition.

double AccountValue::RunOneCell(mcenum_run_basis a_Basis)
{
    InitializeLife(a_Basis);

    for(int year = InforceYear; year < BasicValues::GetLength(); ++year)
        {
        Year = year;
        CoordinateCounters();
        InitializeYear();

        int inforce_month = (Year == InforceYear) ? InforceMonth : 0;
        for(int month = inforce_month; month < 12; ++month)
            {
            Month = month;
            CoordinateCounters();
            // Absent a group context, case-level k factor is unity:
            // because partial mortality has no effect, experience
            // rating is impossible. USER !! Explain this in user
            // documentation.
            IncrementBOM(year, month, 1.0);
            // TODO ?? PRESSING Adjusting this by inforce is wrong for
            // individual cells run as such, because they don't
            // reflect partial mortality.
            IncrementEOM
                (year
                ,month
                ,SepAcctValueAfterDeduction * InforceLivesBoy()
                ,CumPmts
                );
            }

        SetClaims();
        SetProjectedCoiCharge();
        IncrementEOY(year);
        }

    FinalizeLife(a_Basis);

    return TotalAccountValue();
}

//============================================================================
void AccountValue::InitializeLife(mcenum_run_basis a_Basis)
{
    RunBasis_ = a_Basis;
    set_cloven_bases_from_run_basis(RunBasis_, GenBasis_, SepBasis_);

    // Many values set by SetInitialValues() are never subsequently
    // changed. SOMEDAY !! Factor those out into a function that can
    // be called OAOO.
    SetInitialValues();

    VariantValues().Init(*this, GenBasis_, SepBasis_);
    InvariantValues().ReInit(this);

    // Default initial values assume that the policy never lapses or
    // becomes a MEC, so that the lapse and MEC durations are the last
    // possible month.
    // TODO ?? TAXATION !! Last possible month? Why? In that month, it
    // doesn't quite lapse, and it's certainly wrong to assume it
    // becomes a MEC then.
    LMI_ASSERT(11                       == VariantValues().LapseMonth);
    LMI_ASSERT(BasicValues::GetLength() == VariantValues().LapseYear );
    LMI_ASSERT(false                    == InvariantValues().IsMec   );
    LMI_ASSERT(11                       == InvariantValues().MecMonth);
    LMI_ASSERT(BasicValues::GetLength() == InvariantValues().MecYear );

    daily_interest_accounting = contains
        (yare_input_.Comments
        ,"idiosyncrasy_daily_interest_accounting"
        );

    OldDBOpt = DeathBfts_->dbopt()[0];
    // TAXATION !! 'OldSA' and 'OldDB' need to be distinguished for 7702 and 7702A,
    // with inclusion of term dependent on 'TermIsDbFor7702' and 'TermIsDbFor7702A'.
    OldSA = base_specamt(0) + term_specamt(0);
    // TODO ?? TAXATION !! Shouldn't we increase initial SA if contract in corridor at issue?
    OldDB = OldSA;

    SurrChg_.assign(BasicValues::GetLength(), 0.0);

    // TAXATION !! Input::InforceAnnualTargetPremium should be used here.
    double annual_target_premium = GetModalTgtPrem
        (0
        ,mce_annual
        ,base_specamt(0)
        );
    double sa = specamt_for_7702(0);

    // It is at best superfluous to do this for every basis.
    // TAXATION !! Don't do that then.
    Irc7702_->Initialize7702
        (sa
        ,sa
        ,effective_dbopt_7702(DeathBfts_->dbopt()[0], Effective7702DboRop)
        ,annual_target_premium
        );

    InvariantValues().InitGLP = Irc7702_->RoundedGLP();
    InvariantValues().InitGSP = Irc7702_->RoundedGSP();

    // This is notionally called once per *current*-basis run
    // and actually called once per run, with calculations suppressed
    // for all other bases. TODO ?? TAXATION !! How should we handle MEC-avoid
    // solves on bases other than current?

    // INPUT !! This should depend only on 'yare_input_.InforceIsMec',
    // which probably should be forced to 'false' for new business.
    bool inforce_is_mec =
           yare_input_.EffectiveDate != yare_input_.InforceAsOfDate
        && yare_input_.InforceIsMec
        ;
    InvariantValues().InforceIsMec = inforce_is_mec;
    bool mec_1035 =
              yare_input_.External1035ExchangeFromMec
           && 0.0 != Outlay_->external_1035_amount()
        ||    yare_input_.Internal1035ExchangeFromMec
           && 0.0 != Outlay_->internal_1035_amount()
        ;
    bool is_already_a_mec = inforce_is_mec || mec_1035;
    if(is_already_a_mec)
        {
        InvariantValues().IsMec    = true;
        InvariantValues().MecYear  = 0;
        InvariantValues().MecMonth = 0;
        }
    std::vector<double> pmts_7702a;
    std::vector<double> bfts_7702a;
    if(yare_input_.EffectiveDate == yare_input_.InforceAsOfDate)
        {
        // No need to initialize 'pmts_7702a' in this case.
        bfts_7702a.push_back(specamt_for_7702A(0));
        }
    else
        {
        int length_7702a = std::min(7, BasicValues::GetLength());
        // Premium history starts at contract year zero.
        std::copy_n
            (yare_input_.Inforce7702AAmountsPaidHistory.begin()
            ,length_7702a
            ,std::back_inserter(pmts_7702a)
            );
        // Specamt history is irrelevant except for LDB.
        bfts_7702a = std::vector<double>(length_7702a, yare_input_.InforceLeastDeathBenefit);
        }
    double lowest_death_benefit = yare_input_.InforceLeastDeathBenefit;
    if(yare_input_.EffectiveDate == yare_input_.InforceAsOfDate)
        {
        lowest_death_benefit = bfts_7702a.front(); // TAXATION !! See above--use input LDB instead.
        }
    Irc7702A_->Initialize7702A
        (mce_run_gen_curr_sep_full != RunBasis_
        ,is_already_a_mec
        ,IssueAge
        ,BasicValues::EndtAge
        ,InforceYear
        ,InforceMonth
        ,yare_input_.InforceContractYear
        ,yare_input_.InforceContractMonth
        ,yare_input_.InforceAvBeforeLastMc
        ,lowest_death_benefit // TAXATION !! See above--use input LDB instead.
        ,pmts_7702a
        ,bfts_7702a
        );
}

//============================================================================
void AccountValue::FinalizeLife(mcenum_run_basis a_Basis)
{
    LMI_ASSERT(RunBasis_ == a_Basis);

    DebugEndBasis();

    if(SolvingForGuarPremium)
        {
        return;
        }

    if(mce_run_gen_curr_sep_full == RunBasis_)
        {
        ledger_->SetLedgerInvariant(InvariantValues());
        }
    ledger_->SetOneLedgerVariant(a_Basis, VariantValues());
}

//============================================================================
void AccountValue::FinalizeLifeAllBases()
{
    ledger_->ZeroInforceAfterLapse();
    SetGuarPrem();
}

//============================================================================
void AccountValue::SetInitialValues()
{
    InforceYear                 = yare_input_.InforceYear                     ;
    InforceMonth                = yare_input_.InforceMonth                    ;
    InforceAVGenAcct            = yare_input_.InforceGeneralAccountValue      ;
    InforceAVSepAcct            = yare_input_.InforceSeparateAccountValue     ;
    InforceAVRegLn              = yare_input_.InforceRegularLoanValue         ;
    InforceAVPrfLn              = yare_input_.InforcePreferredLoanValue       ;
    InforceRegLnBal             = yare_input_.InforceRegularLoanBalance       ;
    InforcePrfLnBal             = yare_input_.InforcePreferredLoanBalance     ;
    InforceCumNoLapsePrem       = yare_input_.InforceCumulativeNoLapsePremium ;
    InforceCumPmts              = yare_input_.InforceCumulativeNoLapsePayments;
    InforceTaxBasis             = yare_input_.InforceTaxBasis                 ;

    Year                        = InforceYear;
    Month                       = InforceMonth;
    CoordinateCounters();

    DB7702A                     = 0.0;  // TODO ?? TAXATION !! This seems silly.

    AVRegLn                     = InforceAVRegLn;
    AVPrfLn                     = InforceAVPrfLn;
    RegLnBal                    = InforceRegLnBal;
    PrfLnBal                    = InforcePrfLnBal;
    AVGenAcct                   = InforceAVGenAcct;
    AVSepAcct                   = InforceAVSepAcct;

    SepAcctPaymentAllocation = premium_allocation_to_sepacct(yare_input_);
    GenAcctPaymentAllocation = 1.0 - SepAcctPaymentAllocation;

    if(!database().query<bool>(DB_AllowGenAcct) && 0.0 != GenAcctPaymentAllocation)
        {
        alarum()
            << "No general account is allowed for this product, but "
            << GenAcctPaymentAllocation
            << " is allocated to the general account."
            << LMI_FLUSH
            ;
        }

    if(!database().query<bool>(DB_AllowSepAcct) && 0.0 != SepAcctPaymentAllocation)
        {
        alarum()
            << "No separate account is allowed for this product, but "
            << SepAcctPaymentAllocation
            << " is allocated to the separate account."
            << LMI_FLUSH
            ;
        }

    MaxLoan                     = 0.0;

    GenAcctIntCred              = 0.0;
    SepAcctIntCred              = 0.0;
    RegLnIntCred                = 0.0;
    PrfLnIntCred                = 0.0;

    MaxWD                       = 0.0;
    GrossWD                     = 0.0;
    NetWD                       = 0.0;

    CumPmts                     = InforceCumPmts;
    TaxBasis                    = InforceTaxBasis;
    YearlyTaxBasis.assign(BasicValues::GetLength(), 0.0);
    MlyNoLapsePrem              = 0.0;
    CumNoLapsePrem              = InforceCumNoLapsePrem;

    // Initialize all elements of this vector to 'false'. Then, when
    // the no-lapse criteria fail to be met, future values are right.
    YearlyNoLapseActive.assign(BasicValues::GetLength(), false);
    loan_ullage_       .assign(BasicValues::GetLength(), 0.0);
    withdrawal_ullage_ .assign(BasicValues::GetLength(), 0.0);
    NoLapseActive               = true;
    if(NoLapseDboLvlOnly && mce_option1 != DeathBfts_->dbopt()[0])
        {
        NoLapseActive           = false;
        }
    if(NoLapseUnratedOnly && is_policy_rated(yare_input_))
        {
        NoLapseActive           = false;
        }

    database().query_into(DB_SplitMinPrem       , SplitMinPrem);
    database().query_into(DB_UnsplitSplitMinPrem, UnsplitSplitMinPrem);

    database().query_into(DB_TermCanLapse       , TermCanLapse);
    TermRiderActive             = true;
    TermDB                      = 0.0;

    ItLapsed                    = false;

    Dcv                         = yare_input_.InforceDcv;
    DcvDeathBft                 = 0.0;
    DcvNaar                     = 0.0;
    DcvCoiCharge                = 0.0;
    DcvTermCharge               = 0.0;
    DcvWpCharge                 = 0.0;

    HoneymoonActive             = false;
    HoneymoonValue              = -std::numeric_limits<double>::max();
    if(mce_gen_curr == GenBasis_)
        {
        HoneymoonActive = yare_input_.HoneymoonEndorsement;
        if(0 != Year || 0 != Month)
            {
            HoneymoonActive =
                   HoneymoonActive
                && 0.0 < yare_input_.InforceHoneymoonValue
                ;
            }
        if(HoneymoonActive)
            {
            HoneymoonValue = yare_input_.InforceHoneymoonValue;
            }
        }

    CoiCharge                   = 0.0;
    RiderCharges                = 0.0;
    NetCoiCharge                = 0.0;
    MlyDed                      = 0.0;
    CumulativeSalesLoad         = yare_input_.InforceCumulativeSalesLoad;

    database().query_into(DB_ExpRatCoiRetention, CoiRetentionRate);
    database().query_into(DB_ExpRatAmortPeriod , ExperienceRatingAmortizationYears);
    database().query_into(DB_ExpRatIbnrMult    , IbnrAsMonthsOfMortalityCharges);

    Dumpin             = Outlay_->dumpin();
    External1035Amount = Outlay_->external_1035_amount();
    Internal1035Amount = Outlay_->internal_1035_amount();

    database().query_into(DB_EePremMethod      , ee_premium_allocation_method  );
    database().query_into(DB_EePremAcct        , ee_premium_preferred_account  );
    database().query_into(DB_ErPremMethod      , er_premium_allocation_method  );
    database().query_into(DB_ErPremAcct        , er_premium_preferred_account  );
    database().query_into(DB_DeductionMethod   , deduction_method              );
    database().query_into(DB_DeductionAcct     , deduction_preferred_account   );
    database().query_into(DB_DistributionMethod, distribution_method           );
    database().query_into(DB_DistributionAcct  , distribution_preferred_account);

    // If any account preference is the separate account, then a
    // separate account must be available.
    if
        (    oe_prefer_separate_account == ee_premium_preferred_account
        ||   oe_prefer_separate_account == er_premium_preferred_account
        ||   oe_prefer_separate_account == deduction_preferred_account
        ||   oe_prefer_separate_account == distribution_preferred_account
        )
        {
        LMI_ASSERT(database().query<bool>(DB_AllowSepAcct));
        }
    // If any account preference for premium is the general account,
    // then payment into the separate account must be permitted; but
    // even a product that doesn't permit that might have a general
    // account, e.g. for loans or deductions.
    if
        (    oe_prefer_separate_account == ee_premium_preferred_account
        ||   oe_prefer_separate_account == er_premium_preferred_account
        )
        {
        LMI_ASSERT(database().query<bool>(DB_AllowSepAcct));
        }
}

//============================================================================
// Process monthly transactions up to but excluding interest credit
double AccountValue::IncrementBOM
    (int    year
    ,int    month
    ,double a_case_k_factor
    )
{
    if(ItLapsed || BasicValues::GetLength() <= year)
        {
        // Return value is total assets. After the policy has lapsed or
        // matured, there are no assets.
        return 0.0;
        }

    // Paranoid check.
    LMI_ASSERT(year == Year);
    LMI_ASSERT(month == Month);
    LMI_ASSERT(MonthsSinceIssue == Month + 12 * Year);
    if(daily_interest_accounting)
        {
        LMI_ASSERT(365 <= days_in_policy_year && days_in_policy_year <= 366);
        LMI_ASSERT(28 <= days_in_policy_month && days_in_policy_month <= 31);
        }
    if
        (   year != Year
        ||  month != Month
        ||  MonthsSinceIssue != Month + 12 * Year
        ||  daily_interest_accounting && !(365 <= days_in_policy_year && days_in_policy_year <= 366)
        ||  daily_interest_accounting && !(28 <= days_in_policy_month && days_in_policy_month <= 31)
        )
        {
        alarum()
            << "Expected year = "  << Year
            << "; actual year is  " << year << ".\n"
            << "Expected month = " << Month
            << "; actual month is " << month << ".\n"
            << "Expected MonthsSinceIssue = " << (Month + 12 * Year)
            << "; actual value is " << MonthsSinceIssue << ".\n"
            << "Days in policy year = " << days_in_policy_year << ".\n"
            << "Days in policy month = " << days_in_policy_month << ".\n"
            << LMI_FLUSH
            ;
        }

    if
        (   yare_input_.UsePartialMortality
        &&  yare_input_.UseExperienceRating
        &&  mce_gen_curr == GenBasis_
        )
        {
        case_k_factor = a_case_k_factor;
        }

    DoMonthDR();
    return TotalAccountValue();
}

//============================================================================
// Credit interest and process all subsequent monthly transactions
void AccountValue::IncrementEOM
    (int year
    ,int month
    ,double assets_post_bom
    ,double cum_pmts_post_bom
    )
{
    if(ItLapsed || BasicValues::GetLength() <= Year)
        {
        return;
        }

    // Paranoid check.
    LMI_ASSERT(year == Year);
    LMI_ASSERT(month == Month);
    LMI_ASSERT(MonthsSinceIssue == Month + 12 * Year);
    if(daily_interest_accounting)
        {
        LMI_ASSERT(365 <= days_in_policy_year && days_in_policy_year <= 366);
        LMI_ASSERT(28 <= days_in_policy_month && days_in_policy_month <= 31);
        }

    // Save arguments, constraining their values to be nonnegative,
    // for calculating banded and tiered quantities.
    AssetsPostBom  = std::max(0.0, assets_post_bom  );
    CumPmtsPostBom = std::max(0.0, cum_pmts_post_bom);

    DoMonthCR();
}

//============================================================================
void AccountValue::IncrementEOY(int year)
{
    if(ItLapsed || BasicValues::GetLength() <= Year)
        {
        return;
        }

    // Paranoid check.
    LMI_ASSERT(year == Year);

    FinalizeYear();
}

//============================================================================
bool AccountValue::PrecedesInforceDuration(int year, int month)
{
    return year < InforceYear || (year == InforceYear && month < InforceMonth);
}

//============================================================================
void AccountValue::InitializeYear()
{
    if(ItLapsed || BasicValues::GetLength() <= Year)
        {
        return;
        }

// TODO ?? Solve...() should reset not inputs but...something else?
    SetAnnualInvariants();

    PremiumTax_->start_new_year();
    // Skip this in an incomplete initial inforce year.
    // TAXATION !! Premium tax should perhaps be handled similarly.
    // TAXATION !! For 7702A, some rates change on policy anniversary
    // because age does, but cum 7pp changes on contract anniversary.
    if(Year != InforceYear || 0 == InforceMonth)
        {
        Irc7702_ ->UpdateBOY7702();
        Irc7702A_->UpdateBOY7702A(Year);
        }

    MonthsPolicyFees            = 0.0;
    SpecAmtLoad                 = 0.0;

    AssetsPostBom               = 0.0;
    CumPmtsPostBom              = 0.0;
    SepAcctLoad                 = 0.0;

    YearsTotalCoiCharge         = 0.0;
    YearsTotalRiderCharges      = 0.0;
    YearsAVRelOnDeath           = 0.0;
    YearsLoanRepaidOnDeath      = 0.0;
    YearsGrossClaims            = 0.0;
    YearsDeathProceeds          = 0.0;
    YearsNetClaims              = 0.0;
    YearsTotalNetIntCredited    = 0.0;
    YearsTotalGrossIntCredited  = 0.0;
    YearsTotalLoanIntAccrued    = 0.0;
    YearsTotalNetCoiCharge      = 0.0;
    YearsTotalPolicyFee         = 0.0;
    YearsTotalDacTaxLoad        = 0.0;
    YearsTotalSpecAmtLoad       = 0.0;
    YearsTotalSepAcctLoad       = 0.0;
    YearsTotalGptForceout       = 0.0;

    NextYearsProjectedCoiCharge = 0.0;

    DacTaxRsv                   = 0.0;

    RequestedLoan               = Outlay_->new_cash_loans()[Year];
    // ActualLoan is set in TxTakeLoan() and TxLoanRepay(). A local
    // variable in each function might have sufficed, except that this
    // quantity is used in the optional monthly detail report. Its
    // value depends on the maximum loan, so it cannot be known here.
    ActualLoan                  = 0.0;

    GrossPmts   .assign(12, 0.0);
    EeGrossPmts .assign(12, 0.0);
    ErGrossPmts .assign(12, 0.0);
    NetPmts     .assign(12, 0.0);

    InitializeSpecAmt();
}

//============================================================================
void AccountValue::InitializeSpecAmt()
{
    // TODO ?? These variables are set in current run and used in guar and midpt.
    ActualSpecAmt = base_specamt(Year);
    TermSpecAmt   = term_specamt(Year);

    set_modal_min_premium();

    // No-lapse premium generally changes whenever specamt changes for
    // any reason (e.g., elective increases or decreases, DBO changes,
    // and withdrawals). Target premium may change similarly, or may
    // be fixed immutably at issue. For illustrations, these premium
    // changes occur only on anniversary, because triggering events
    // are allowed only on anniversary.
    //
    // Target premium is annual by its nature: commission is earned
    // immediately if the full target is paid on anniversary.
    // Conversely, no-lapse premium by its nature is on the most
    // frequent mode (monthly for lmi), because no-lapse guarantees
    // are offered for all modes.
    //
    // Arguably the "supplemental" specamt should be included in the
    // target or even the minimum calculation in the TermIsNotRider
    // case; but that's used only with one family of exotic products
    // for which these quantities don't matter anyway.
    //
    int const target_year = TgtPremFixedAtIssue ? 0 : Year;
    MlyNoLapsePrem = GetModalMinPrem
        (target_year
        ,mce_monthly
        ,base_specamt(target_year)
        );
    UnusedTargetPrem = GetModalTgtPrem
        (target_year
        ,mce_annual
        ,base_specamt(target_year)
        );
    AnnualTargetPrem = UnusedTargetPrem;

    if(0 == Year)
        {
        InvariantValues().InitTgtPrem = AnnualTargetPrem;
        }

    // TODO ?? Perform specamt strategy here?
}

/// Set duration at which list-bill premium is to be determined.
///
/// The year and month of determination correspond to the first
/// monthiversary on or after the list-bill date. If there is no
/// such monthiversary, then harmless default values are used.

void AccountValue::set_list_bill_year_and_month()
{
    auto const& cert_date = yare_input_.EffectiveDate;
    auto const& bill_date = yare_input_.ListBillDate;
    if(bill_date < cert_date) return;

    auto const z = years_and_months_since(cert_date, bill_date, false);
    list_bill_year_   = z.first;
    list_bill_month_  = z.second;
}

void AccountValue::set_list_bill_premium()
{
    bool const the_time_is_now =
           Year  == list_bill_year_
        && Month == list_bill_month_
        && mce_run_gen_curr_sep_full == RunBasis_
        ;
    if(!the_time_is_now) return;

    if(!SplitMinPrem)
        {
        auto const z = GetListBillPremMlyDed
            (Year
            ,Outlay_->er_premium_modes()[Year]
            ,base_specamt(Year)
            );
        InvariantValues().ListBillPremium   = z;
        InvariantValues().ErListBillPremium = z;
        }
    else
        {
        auto const z = GetListBillPremMlyDedEx
            (Year
            ,Outlay_->er_premium_modes()[Year]
            ,base_specamt(Year)
            ,term_specamt(Year)
            );
        InvariantValues().EeListBillPremium = z.first;
        InvariantValues().ErListBillPremium = z.second;
        InvariantValues().ListBillPremium = z.first + z.second;
        }
}

/// Calculate "modal minimum" premiums designed for group plans.
///
/// They are intended roughly to approximate the minimum payment
/// (at a modal frequency chosen by the employer) necessary to
/// prevent lapse if no other premium is paid.
///
/// Some products apportion them explicitly between ee and er. For
/// those that don't, convention deems the er to pay it all.
///
/// These values are calculated for all products, but probably should
/// be restricted to group plans that actually use them on reports.

void AccountValue::set_modal_min_premium()
{
    if(!SplitMinPrem)
        {
        auto const z = GetModalMinPrem
            (Year
            ,Outlay_->er_premium_modes()[Year]
            ,base_specamt(Year)
            );
        InvariantValues().ModalMinimumPremium[Year]   = z;
        InvariantValues().ErModalMinimumPremium[Year] = z;
        }
    else
        {
        auto const z = GetModalPremMlyDedEx
            (Year
            ,Outlay_->er_premium_modes()[Year]
            ,base_specamt(Year)
            ,term_specamt(Year)
            );
        InvariantValues().EeModalMinimumPremium[Year] = z.first;
        InvariantValues().ErModalMinimumPremium[Year] = z.second;
        InvariantValues().ModalMinimumPremium[Year] = z.first + z.second;
        }
}

/// Surrender charge.
///
/// The "cash value enhancement" components should be implemented as
/// such, rather than as negative surrender charges.
///
/// This ought to be called annually, OAOO, at the top level, and
/// stored in the variant or invariant ledger depending on whether
/// surrender charges are to vary by basis. All other direct calls
/// to this function should be replaced by constant values looked up
/// in a ledger, or perhaps in SurrChg_ if that vector is ever given
/// any nonzero value.
///
/// SOMEDAY !! Table support and UL model reg formulas should be added.

double AccountValue::SurrChg() const
{
    LMI_ASSERT(0.0 <= SurrChg_[Year]);
    // For the nonce, CSVBoost() is netted against surrender charge.
    // This class's implementation should be revised to distinguish
    // these additive and subtractive components of CSV.
    return SurrChg_[Year] - CSVBoost();
}

/// Cash value augmentation--like a negative surrender charge.
///
/// Probably the input field should be expunged.

double AccountValue::CSVBoost() const
{
    double const z =
          CashValueEnhMult[Year]
        + yare_input_.CashValueEnhancementRate[Year]
        ;
    LMI_ASSERT(0.0 <= z);
    return z * std::max(0.0, TotalAccountValue());
}

//============================================================================
/// Amounts such as claims and account value released on death are
/// multiplied by the beginning-of-year inforce factor when a
/// composite is produced; it would be incorrect to multiply them by
/// the inforce factor here because individual-cell ledgers do not
/// reflect partial mortality. This calculation assumes that partial
/// mortality is curtate.
///
/// It is assumed that the death benefit is sufficient to repay the
/// loan and loan interest--or, at least, that no company would
/// attempt to recover any excess.
///
/// material_difference() is used to subtract amounts that may be
/// materially equal--and, for consistency, in parallel cases where
/// the difference is unlikely to be nearly zero.
///
/// TODO ?? This function is designed to be called at the end of a
/// year, when 'DBReflectingCorr' probably equals the death benefit as
/// of the beginning of the twelfth month, but its end-of-year value
/// (as of the end of the twelfth month) is needed--so the death
/// benefit is updated explicitly. This seems inelegant, and is an
/// obstacle to showing amounts affected by partial mortality on the
/// monthly-detail report.

void AccountValue::SetClaims()
{
    if(!yare_input_.UsePartialMortality || ItLapsed || BasicValues::GetLength() <= Year)
        {
        return;
        }

    TxSetDeathBft();
    TxSetTermAmt();

    YearsGrossClaims       = partial_mortality_qx[Year] * DBReflectingCorr;
    YearsAVRelOnDeath      = partial_mortality_qx[Year] * TotalAccountValue();
    YearsLoanRepaidOnDeath = partial_mortality_qx[Year] * (RegLnBal + PrfLnBal);
    YearsDeathProceeds = material_difference
        (YearsGrossClaims
        ,YearsLoanRepaidOnDeath
        );
    YearsNetClaims = material_difference
        (YearsGrossClaims
        ,YearsAVRelOnDeath
        );
}

//============================================================================
// Proxy for next year's COI charge, used only for experience rating.
void AccountValue::SetProjectedCoiCharge()
{
    if
        (   ItLapsed
        ||  BasicValues::GetLength() <= Year
        ||  !yare_input_.UsePartialMortality
        ||  !yare_input_.UseExperienceRating
        ||  mce_gen_curr != GenBasis_
        )
        {
        return;
        }

    // Project a charge of zero for the year after maturity.
    //
    // This is written separately to emphasize its meaning, though it
    // obviously could be combined with the above '<=' comparison.
    //
    if(BasicValues::GetLength() == 1 + Year)
        {
        return;
        }

    TxSetDeathBft();
    TxSetTermAmt();
    double this_years_terminal_naar = material_difference
        (DBReflectingCorr + TermDB
        ,TotalAccountValue()
        );
    this_years_terminal_naar = std::max(0.0, this_years_terminal_naar);
    double next_years_coi_rate = GetBandedCoiRates(GenBasis_, ActualSpecAmt)[1 + Year];

    NextYearsProjectedCoiCharge =
            12.0
        *   this_years_terminal_naar
        *   next_years_coi_rate
        ;
}

/// Post year-end results to ledger.
///
/// This function is called only if the contract is in force at the
/// end of the year. One might alternatively post a partial year's
/// results in the year of lapse; perhaps the greatest benefit would
/// be a more comprehensive composite. Apparently, in 2008, the most
/// common industry practice is:
///  - for composites, truncate each cell as at the beginning of its
///    lapse year;
///  - for individual illustrations, either print nothing for the year
///    of lapse, or print the whole annualized planned premium, with
///    an asterisk pointing to a footnote.
/// If composites were changed to bring lapsing contracts into an
/// account-value rollforward, it would be necessary to show only
/// payments made prior to lapse, and to limit each charge to the
/// available account value; but that would introduce an inconsistency
/// with prevailing practice. It is not unreasonable to follow common
/// practice by defining a composite as the weighted sum of individual
/// illustrations, which show completed years only.

void AccountValue::FinalizeYear()
{
    VariantValues().TotalLoanBalance[Year] = RegLnBal + PrfLnBal;

    double total_av = TotalAccountValue();
    double surr_chg = SurrChg();
    double csv_net =
          total_av
        - (RegLnBal + PrfLnBal)
//        + ExpRatReserve // This would be added if it existed.
        ;

    // While performing a solve, ignore any sales-load refund, because
    // it wouldn't prevent the contract from lapsing.
    if(!Solving)
        {
        csv_net += GetRefundableSalesLoad();
        }

    // While performing a solve, ignore any positive surrender charge
    // that cannot cause the contract to lapse.
    if(Solving && 0.0 < surr_chg && LapseIgnoresSurrChg)
        {
        ; // Do nothing.
        }
    else
        {
        csv_net -= surr_chg;
        }

    csv_net = std::max(csv_net, HoneymoonValue);

    if(!Solving)
        {
        csv_net = std::max(csv_net, 0.0);
        }

    if(Solving)
        {
        YearlyTaxBasis[Year] = TaxBasis;
        }

    // 7702(f)(2)(A)
    double cv_7702 =
          total_av
        + GetRefundableSalesLoad()
//        + std::max(0.0, ExpRatReserve) // This would be added if it existed.
        ;
    // Increase by negative surrender charge. If some components of
    // the surrender charge are negative while others are positive,
    // consider only their sum here, instead of considering the sign
    // of each component individually.
    if(surr_chg < 0.0)
        {
        cv_7702 -= surr_chg;
        }
    cv_7702 = std::max(cv_7702, HoneymoonValue);

    VariantValues().AcctVal     [Year] = total_av;
    VariantValues().AVGenAcct   [Year] = AVGenAcct + AVRegLn + AVPrfLn;
    VariantValues().AVSepAcct   [Year] = AVSepAcct;
    VariantValues().DacTaxRsv   [Year] = DacTaxRsv;
    VariantValues().CSVNet      [Year] = csv_net;
    VariantValues().CV7702      [Year] = cv_7702;

    // Update death benefit. 'DBReflectingCorr' currently equals the
    // death benefit as of the beginning of the twelfth month, but its
    // end-of-year value (as of the end of the twelfth month) is
    // needed.

    TxSetDeathBft();
    TxSetTermAmt();
    // post values to LedgerVariant
    InvariantValues().TermSpecAmt   [Year] = TermSpecAmt;
    VariantValues().TermPurchased   [Year] = TermDB;
    // Add term rider DB
    VariantValues().BaseDeathBft    [Year] = DBReflectingCorr;
    VariantValues().EOYDeathBft     [Year] = DBReflectingCorr + TermDB;

/*
    // AV already includes any experience refund credited, but it's
    // forborne among the survivors. That was the right thing to use
    // for calculating the corridor death benefit for each individual.
    // But it's not the right thing to multiply by EOY inforce and add
    // into a composite. The right thing is for the composite to sum
    // the AV plus *cash* refund instead of forborne, multiplied by EOY
    // inforce. Otherwise an account-value rollforward cross-check
    // wouldn't work.
    //
    // TODO ?? Maybe the AV before this adjustment is what we really want
    // to display for an individual illustration. That's what we did
    // originally, and I'm not at all sure it's right to change it
    // now.
    VariantValues().AcctVal         [Year] =
          TotalAccountValue()
        + VariantValues().ExpRatRfdCash     [Year]
        - VariantValues().ExpRatRfdForborne [Year]
        ;
*/

    // Monthly deduction detail

    VariantValues().COICharge         [Year] = YearsTotalCoiCharge        ;
    VariantValues().RiderCharges      [Year] = YearsTotalRiderCharges     ;
    VariantValues().AVRelOnDeath      [Year] = YearsAVRelOnDeath          ;
    VariantValues().ClaimsPaid        [Year] = YearsGrossClaims           ;
    VariantValues().DeathProceedsPaid [Year] = YearsDeathProceeds         ;
    VariantValues().NetClaims         [Year] = YearsNetClaims             ;
    VariantValues().NetIntCredited    [Year] = YearsTotalNetIntCredited   ;
    VariantValues().GrossIntCredited  [Year] = YearsTotalGrossIntCredited ;
    VariantValues().LoanIntAccrued    [Year] = YearsTotalLoanIntAccrued   ;
    VariantValues().NetCOICharge      [Year] = YearsTotalNetCoiCharge     ;
    VariantValues().PolicyFee         [Year] = YearsTotalPolicyFee        ;
    VariantValues().DacTaxLoad        [Year] = YearsTotalDacTaxLoad       ;
    VariantValues().SpecAmtLoad       [Year] = YearsTotalSpecAmtLoad      ;
    VariantValues().PremTaxLoad       [Year] = PremiumTax_->ytd_load();

    double notional_sep_acct_charge =
          YearsTotalSepAcctLoad
        + YearsTotalGrossIntCredited
        - YearsTotalNetIntCredited
        ;
    VariantValues().SepAcctCharges    [Year] = notional_sep_acct_charge   ;

    // Record dynamic interest rate in ledger object.
    //
    // TODO ?? Actually, 'YearsSepAcctIntRate' holds the dynamic rate
    // for the current month--here, the year's last month. It would be
    // more accurate to use an average across twelve months.
    //
    // TODO ?? The annual rate is calculated in the interest-rate
    // class's dynamic callback function; it would be better to pass
    // it here, instead of invoking a costly transcendental function
    // that loses accuracy.
    //
    // TODO ?? The rates this code records are static, not dynamic.
    //
    if(MandEIsDynamic)
        {
        VariantValues().RecordDynamicSepAcctRate
            (InterestRates_->SepAcctNetRate
                (SepBasis_
                ,GenBasis_
                ,mce_annual_rate
                )
                [Year]
            ,InterestRates_->SepAcctNetRate
                (SepBasis_
                ,GenBasis_
                ,mce_monthly_rate
                )
                [Year]
            ,Year
            );
        }

    VariantValues().NetPmt[Year] = std::accumulate
        (NetPmts.begin()
        ,NetPmts.end()
        ,-YearsTotalGptForceout
        );

    if(mce_run_gen_curr_sep_full == RunBasis_)
        {
        InvariantValues().GrossPmt  [Year]  = 0.0;
        InvariantValues().EeGrossPmt[Year]  = 0.0;
        InvariantValues().ErGrossPmt[Year]  = 0.0;

        // TODO ?? TAXATION !! This is a temporary workaround until we do it right.
        // Forceouts should be a distinct component, passed separately
        // to ledger values. Probably we should treat 1035 exchanges
        // and NAAR 'forceouts' the same way.
        InvariantValues().GrossPmt  [Year]  -= YearsTotalGptForceout;
        InvariantValues().EeGrossPmt[Year]  -= YearsTotalGptForceout;

        for(int j = 0; j < 12; ++j)
            {
            LMI_ASSERT(materially_equal(GrossPmts[j], EeGrossPmts[j] + ErGrossPmts[j]));
            InvariantValues().GrossPmt  [Year]  += GrossPmts[j];
            InvariantValues().EeGrossPmt[Year]  += EeGrossPmts[j];
            InvariantValues().ErGrossPmt[Year]  += ErGrossPmts[j];
            }
        if(0 == Year)
            {
            InvariantValues().InitPrem = InvariantValues().GrossPmt[Year];
            }
        LMI_ASSERT
            (materially_equal
                (   InvariantValues().GrossPmt  [Year]
                ,   InvariantValues().EeGrossPmt[Year]
                +   InvariantValues().ErGrossPmt[Year]
                )
            );
        InvariantValues().Outlay[Year] =
                InvariantValues().GrossPmt   [Year]
            -   InvariantValues().NetWD      [Year]
            -   InvariantValues().NewCashLoan[Year]
            ;

        InvariantValues().GptForceout[Year] = YearsTotalGptForceout;

// SOMEDAY !! Not yet implemented.
//        InvariantValues().NaarForceout[Year] = InvariantValues().ErGrossPmt[Year];
        }
}

//============================================================================
void AccountValue::SetAnnualInvariants()
{
    YearsCorridorFactor     = GetCorridorFactor()[Year];
    YearsDBOpt              = DeathBfts_->dbopt()[Year];
    YearsMonthlyPolicyFee   = Loads_->monthly_policy_fee(GenBasis_)[Year];
    YearsAnnualPolicyFee    = Loads_->annual_policy_fee (GenBasis_)[Year];

    YearsGenAcctIntRate     = InterestRates_->GenAcctNetRate
        (GenBasis_
        ,mce_monthly_rate
        )
        [Year]
        ;
    YearsSepAcctIntRate     = InterestRates_->SepAcctNetRate
        (SepBasis_
        ,GenBasis_
        ,mce_monthly_rate
        )
        [Year]
        ;

    YearsDcvIntRate         = GetMly7702iGlp()[Year];
    YearsHoneymoonValueRate = InterestRates_->HoneymoonValueRate
        (GenBasis_
        ,mce_monthly_rate
        )
        [Year]
        ;
    YearsPostHoneymoonGenAcctIntRate = InterestRates_->PostHoneymoonGenAcctRate
        (GenBasis_
        ,mce_monthly_rate
        )
        [Year]
        ;

    YearsRegLnIntCredRate   = InterestRates_ ->RegLnCredRate
        (GenBasis_
        ,mce_monthly_rate
        )
        [Year]
        ;
    YearsPrfLnIntCredRate   = InterestRates_ ->PrfLnCredRate
        (GenBasis_
        ,mce_monthly_rate
        )
        [Year]
        ;
    YearsRegLnIntDueRate    = InterestRates_ ->RegLnDueRate
        (GenBasis_
        ,mce_monthly_rate
        )
        [Year]
        ;
    YearsPrfLnIntDueRate    = InterestRates_ ->PrfLnDueRate
        (GenBasis_
        ,mce_monthly_rate
        )
        [Year]
        ;

    YearsCoiRate0           = MortalityRates_->MonthlyCoiRatesBand0(GenBasis_)[Year];
    YearsCoiRate1           = MortalityRates_->MonthlyCoiRatesBand1(GenBasis_)[Year];
    YearsCoiRate2           = MortalityRates_->MonthlyCoiRatesBand2(GenBasis_)[Year];
    YearsDcvCoiRate         = GetMlyDcvqc                          ()         [Year];
    YearsAdbRate            = MortalityRates_->AdbRates            ()         [Year];
    YearsTermRate           = MortalityRates_->MonthlyTermCoiRates (GenBasis_)[Year];
    YearsWpRate             = MortalityRates_->WpRates             ()         [Year];
    YearsSpouseRiderRate    = MortalityRates_->SpouseRiderRates    (GenBasis_)[Year];
    YearsChildRiderRate     = MortalityRates_->ChildRiderRates     ()         [Year];

    YearsTotLoadTgt         = Loads_->target_total_load     (GenBasis_)[Year];
    YearsTotLoadExc         = Loads_->excess_total_load     (GenBasis_)[Year];
    // TAXATION !! This '_lowest_premium_tax' approach needs to be
    // reworked: there should be an option (at least) to use the
    // current tax rates.
    YearsTotLoadTgtLowestPremtax = Loads_->target_premium_load_minimum_premium_tax()[Year];
    YearsTotLoadExcLowestPremtax = Loads_->excess_premium_load_minimum_premium_tax()[Year];
    YearsPremLoadTgt        = Loads_->target_premium_load   (GenBasis_)[Year];
    YearsPremLoadExc        = Loads_->excess_premium_load   (GenBasis_)[Year];
    YearsSalesLoadTgt       = Loads_->target_sales_load     (GenBasis_)[Year];
    YearsSalesLoadExc       = Loads_->excess_sales_load     (GenBasis_)[Year];
    YearsSpecAmtLoadRate    = Loads_->specified_amount_load (GenBasis_)[Year];
    YearsSepAcctLoadRate    = Loads_->separate_account_load (GenBasis_)[Year];
    YearsSalesLoadRefundRate= Loads_->refundable_sales_load_proportion()[Year];
    YearsDacTaxLoadRate     = Loads_->dac_tax_load                    ()[Year];
}

/// Calculate and store actuarial functions for partial mortality.
///
/// Iff partial mortality is used, save qx, tpx, and lx in vectors
/// for use elsewhere in this class and for compositing ledgers.
/// The radix for lx is the number of identical lives that an input
/// cell represents, and qx is forced to unity at the survivorship
/// limit (if any). If partial mortality is not used, then qx is
/// uniformly zero, tpx is one, and lx is the radix.
///
/// Whether a contract continues after its normal maturity date does
/// not matter. It is treated as not expiring on that date because
/// year-end composite values are multiplied by this lx.
///
/// TODO ?? These actuarial functions may be thought of as
/// counting potential inforce lives: they do not reflect lapses.
/// InforceLivesBoy() and InforceLivesEoy() may be used where
/// lapses should be taken into account.

void AccountValue::set_partial_mortality()
{
    double const inforce_lives = yare_input_.NumberOfIdenticalLives;
    partial_mortality_qx .resize(    BasicValues::GetLength());
    partial_mortality_tpx.resize(1 + BasicValues::GetLength(), 1.0);
    partial_mortality_lx .resize(1 + BasicValues::GetLength(), inforce_lives);
    if(yare_input_.UsePartialMortality)
        {
        // partial_mortality_lx[0] was set above.
        for(int j = 0; j < BasicValues::GetLength(); ++j)
            {
            partial_mortality_qx[j] = GetPartMortQ(j);
            double const px = 1.0 - partial_mortality_qx[j];
            partial_mortality_tpx[1 + j] = px * partial_mortality_tpx[j];
            partial_mortality_lx [1 + j] = px * partial_mortality_lx [j];
            }
        }
}

//============================================================================
double AccountValue::GetPartMortQ(int a_year) const
{
    LMI_ASSERT(a_year <= BasicValues::GetLength());
    if(!yare_input_.UsePartialMortality || ItLapsed)
        {
        return 0.0;
        }
    if
        (   MaxSurvivalDur <= a_year
        ||  a_year == BasicValues::GetLength()
        )
        {
        return 1.0;
        }

    double z =
          MortalityRates_->PartialMortalityQ()[a_year]
        * yare_input_.PartialMortalityMultiplier[a_year]
        ;
    return std::max(0.0, std::min(1.0, z));
}

//============================================================================
double AccountValue::GetSepAcctAssetsInforce() const
{
    if(ItLapsed || BasicValues::GetLength() <= Year)
        {
        return 0.0;
        }

    return SepAcctValueAfterDeduction * partial_mortality_lx[Year];
}

//============================================================================
double AccountValue::GetCurtateNetCoiChargeInforce() const
{
    if
        (   ItLapsed
        ||  BasicValues::GetLength() <= Year
        ||  !yare_input_.UsePartialMortality
        ||  !yare_input_.UseExperienceRating
        ||  mce_gen_curr != GenBasis_
        )
        {
        return 0.0;
        }

    LMI_ASSERT(11 == Month);
    return YearsTotalNetCoiCharge * InforceLivesBoy();
}

//============================================================================
double AccountValue::GetCurtateNetClaimsInforce() const
{
    if
        (   ItLapsed
        ||  BasicValues::GetLength() <= Year
        ||  !yare_input_.UsePartialMortality
        ||  !yare_input_.UseExperienceRating
        ||  mce_gen_curr != GenBasis_
        )
        {
        return 0.0;
        }

    LMI_ASSERT(11 == Month);
    return YearsNetClaims * partial_mortality_lx[Year];
}

//============================================================================
double AccountValue::GetProjectedCoiChargeInforce() const
{
    if
        (   ItLapsed
        ||  BasicValues::GetLength() <= Year
        ||  !yare_input_.UsePartialMortality
        ||  !yare_input_.UseExperienceRating
        ||  mce_gen_curr != GenBasis_
        )
        {
        return 0.0;
        }

    LMI_ASSERT(11 == Month);
    return NextYearsProjectedCoiCharge * InforceLivesEoy();
}

//============================================================================
// The experience-rating mortality reserve isn't actually held in
// individual certificates: it really exists only at the case level.
// Yet it is apportioned among certificates in order to conform to the
// design invariant that a composite is a weighted sum of cells.
//
// The return value, added across cells, should reproduce the total
// total reserve at the case level, as the caller may assert.
//
double AccountValue::ApportionNetMortalityReserve
    (double reserve_per_life_inforce
    )
{
    if
        (   ItLapsed
        ||  BasicValues::GetLength() <= Year
        ||  !yare_input_.UsePartialMortality
        ||  !yare_input_.UseExperienceRating
        ||  mce_gen_curr != GenBasis_
        )
        {
        return 0.0;
        }

    double inforce_factor =
        (0.0 != yare_input_.NumberOfIdenticalLives)
        ? InforceLivesEoy() / yare_input_.NumberOfIdenticalLives
        : 0.0
        ;

    double apportioned_reserve = reserve_per_life_inforce * inforce_factor;

    // The experience-rating reserve can't be posted to the ledger in
    // FinalizeYear(), which is run before the reserve is calculated.
    // The projected COI charge and K factor are posted to the ledger
    // here as well, simply for uniformity.
    VariantValues().ExperienceReserve [Year] = apportioned_reserve;
    VariantValues().ProjectedCoiCharge[Year] = NextYearsProjectedCoiCharge;
    VariantValues().KFactor           [Year] = case_k_factor;

    return apportioned_reserve * yare_input_.NumberOfIdenticalLives;
}

/// Beginning of year inforce lives, reflecting lapses and survivorship.

double AccountValue::InforceLivesBoy() const
{
    bool const b {ItLapsed || BasicValues::GetLength() <= Year};
    return b ? 0.0 : partial_mortality_lx.at(Year);
}

/// End of year inforce lives, reflecting lapses and survivorship.

double AccountValue::InforceLivesEoy() const
{
    bool const b {ItLapsed || BasicValues::GetLength() <= Year};
    return b ? 0.0 : partial_mortality_lx.at(1 + Year);
}

//============================================================================
void AccountValue::CoordinateCounters()
{
    MonthsSinceIssue = Month + 12 * Year;

    if(daily_interest_accounting)
        {
        calendar_date current_anniversary = add_years
            (yare_input_.EffectiveDate
            ,Year
            ,true
            );
        calendar_date next_anniversary = add_years
            (yare_input_.EffectiveDate
            ,1 + Year
            ,true
            );
        // This alternative
        //   days_in_policy_year = current_anniversary.days_in_year();
        // is not used: a policy issued in a leap year after its leap
        // day is unaffected by that leap day in the approximate
        // algorithm used here.
        days_in_policy_year =
                next_anniversary.julian_day_number()
            -   current_anniversary.julian_day_number()
            ;
        LMI_ASSERT(365 <= days_in_policy_year && days_in_policy_year <= 366);

        calendar_date current_monthiversary = add_years_and_months
            (yare_input_.EffectiveDate
            ,Year
            ,Month
            ,true
            );
        calendar_date next_monthiversary = add_years_and_months
            (yare_input_.EffectiveDate
            ,Year
            ,1 + Month
            ,true
            );
        days_in_policy_month =
                next_monthiversary.julian_day_number()
            -   current_monthiversary.julian_day_number()
            ;

        LMI_ASSERT(28 <= days_in_policy_month && days_in_policy_month <= 31);

//        LMI_ASSERT
//            (   days_in_policy_month
//            ==  current_monthiversary.days_in_month()
//            );
/*
// This would be wrong for ill-conditioned effective dates.
// Example:
//   effective date 2000-03-29
//   tenth    monthiversary 2001-01-29
//   eleventh monthiversary 2001-02-28 (not the twenty-ninth)
        if(days_in_policy_month !=  current_monthiversary.days_in_month())
            {
            alarum()
                << Year << " Year; " << Month << " Month\n"
                << current_monthiversary.year() << '-'
                << current_monthiversary.month() << '-'
                << current_monthiversary.day() << " calendar date BOM\n"
                << days_in_policy_month << " days_in_policy_month does not equal\n"
                << current_monthiversary.days_in_month() << " current_monthiversary.days_in_month()\n"
                << LMI_FLUSH
                ;
            }
*/
        }
}
