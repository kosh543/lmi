// Manage floating-point environment.
//
// Copyright (C) 2004, 2005, 2006 Gregory W. Chicares.
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
// http://savannah.nongnu.org/projects/lmi
// email: <chicares@cox.net>
// snail: Chicares, 186 Belle Woods Drive, Glastonbury CT 06033, USA

// $Id: fenv_lmi.cpp,v 1.16 2006-09-08 22:11:35 chicares Exp $

#ifdef __BORLANDC__
#   include "pchfile.hpp"
#   pragma hdrstop
#endif // __BORLANDC__

#include "fenv_lmi.hpp"

#include "alert.hpp"

#include <iomanip>
#include <sstream>

void fenv_initialize()
{
#if 0 && defined __STDC_IEC_559__
    // The facilities offered by C99's <fenv.h> are useful, but not
    // sufficient: they require no standard facility to set hardware
    // precision, although 7.6/9 provides for extensions like mingw's
    //   fesetenv(FE_PC64_ENV);
    // This block shows what could be accomplished in standard C.
    fenv_t save_env;
    feholdexcept(&save_env);
    fesetround(FE_TONEAREST);
#   if defined __MINGW32__
    fesetenv(FE_PC64_ENV);
#   else  // !defined __MINGW32__
#       error Find a platform-specific way to set hardware precision.
#   endif // !defined __MINGW32__
#elif defined LMI_X86
    x87_control_word(default_x87_control_word());
#else  // Unknown compiler or platform.
#   error Unknown compiler or platform.
#endif // Unknown compiler or platform.
}

e_ieee754_precision fenv_precision()
{
#if defined __BORLANDC__
    unsigned short int pc = (unsigned short int)(MCW_PC) & x87_control_word();
    return
          (PC_24 == pc) ? fe_fltprec
        : (PC_53 == pc) ? fe_dblprec
        : (PC_64 == pc) ? fe_ldblprec
        : fenv_precision_error()
        ;
#elif defined LMI_X86
    return intel_control_word(x87_control_word()).pc();;
#else  // Unknown compiler or platform.
#   error Unknown compiler or platform.
#endif // Unknown compiler or platform.
}

void fenv_precision(e_ieee754_precision precision_mode)
{
#if defined __BORLANDC__
    unsigned short int z =
          (fe_fltprec  == precision_mode) ? (unsigned short int)(PC_24)
        : (fe_dblprec  == precision_mode) ? (unsigned short int)(PC_53)
        : (fe_ldblprec == precision_mode) ? (unsigned short int)(PC_64)
        : fenv_precision_error()
        ;
    _control87(z, MCW_PC);
#elif defined LMI_X86
    intel_control_word control_word(x87_control_word());
    control_word.pc(precision_mode);
    x87_control_word(control_word.cw());
#else  // Unknown compiler or platform.
#   error Unknown compiler or platform.
#endif // Unknown compiler or platform.
}

e_ieee754_rounding fenv_rounding()
{
#if 0 && defined __STDC_IEC_559__
    int z = fegetround();
    return
          (FE_TONEAREST  == z) ? fe_tonearest
        : (FE_DOWNWARD   == z) ? fe_downward
        : (FE_UPWARD     == z) ? fe_upward
        : (FE_TOWARDZERO == z) ? fe_towardzero
        : throw std::runtime_error("Unknown rounding mode.");
        ;
#elif defined __BORLANDC__
    unsigned short int rc = (unsigned short int)(MCW_RC) & x87_control_word();
    return
          (RC_NEAR == rc) ? fe_tonearest
        : (RC_DOWN == rc) ? fe_downward
        : (RC_UP   == rc) ? fe_upward
        : (RC_CHOP == rc) ? fe_towardzero
        : fenv_rounding_error() // TODO ?? Replace with throw-expression.
        ;
#elif defined LMI_X86
    return intel_control_word(x87_control_word()).rc();;
#else  // Unknown compiler or platform.
#   error Unknown compiler or platform.
#endif // Unknown compiler or platform.
}

// This C99 function call
//   fesetround(rounding_mode);
// is nearly equivalent, except that it takes an int argument.
// It is not conditionally used here, because it seems unwise to
// weaken the type safety C++ affords merely to implement this
// one function when its kindred can't be implemented in C99.
//
void fenv_rounding(e_ieee754_rounding rounding_mode)
{
#if 0 && defined __STDC_IEC_559__
    int z =
          (fe_tonearest  == rounding_mode) ? FE_TONEAREST
        : (fe_downward   == rounding_mode) ? FE_DOWNWARD
        : (fe_upward     == rounding_mode) ? FE_UPWARD
        : (fe_towardzero == rounding_mode) ? FE_TOWARDZERO
        : throw std::runtime_error("Unknown rounding mode.");
        ;
    fesetround(z);
#elif defined __BORLANDC__
    unsigned short int z =
          (fe_tonearest  == rounding_mode) ? (unsigned short int)(RC_NEAR)
        : (fe_downward   == rounding_mode) ? (unsigned short int)(RC_DOWN)
        : (fe_upward     == rounding_mode) ? (unsigned short int)(RC_UP)
        : (fe_towardzero == rounding_mode) ? (unsigned short int)(RC_CHOP)
        : fenv_rounding_error() // TODO ?? Replace with throw-expression.
        ;
    _control87(z, MCW_RC);
#elif defined LMI_X86
    intel_control_word control_word(x87_control_word());
    control_word.rc(rounding_mode);
    x87_control_word(control_word.cw());
#else  // Unknown compiler or platform.
#   error Unknown compiler or platform.
#endif // Unknown compiler or platform.
}

bool fenv_is_valid()
{
#if defined LMI_X86
    return default_x87_control_word() == x87_control_word();
#else  // Unknown compiler or platform.
#   error Unknown compiler or platform.
#endif // Unknown compiler or platform.
}

bool fenv_validate()
{
    bool okay = fenv_is_valid();

    if(!okay)
        {
        // Prefer this approach to fatal_error() because this function
        // is intended to be called just before the program closes, at
        // which time it may be unsafe to show messages by ordinary
        // means.
        std::ostringstream oss;
        oss
            << "The floating-point control word was unexpectedly '"
#if defined LMI_X86
            << std::hex << x87_control_word()
#else  // Unknown compiler or platform.
#   error Unknown compiler or platform.
#endif // Unknown compiler or platform.
            << "'."
            << "\nProbably some other program changed this crucial setting."
            << "\nIt has been reset correctly. Rerun any illustration that"
            << "\nwas being run when this message appeared, because it may"
            << "\nbe incorrect."
            << "\n"
            ;
        fenv_initialize();
        safely_show_message(oss.str().c_str());
        }
    return okay;
}

