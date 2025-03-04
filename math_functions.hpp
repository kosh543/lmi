// Miscellaneous mathematical operations.
//
// Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022 Gregory W. Chicares.
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

#ifndef math_functions_hpp
#define math_functions_hpp

#include "config.hpp"

#include <algorithm>                    // max(), min(), transform()
#include <cmath>                        // fabs(), signbit()
#include <limits>
#include <numeric>                      // midpoint(), partial_sum()
#include <stdexcept>
#include <type_traits>                  // is_, has_, make_ ...
#include <vector>

namespace lmi
{
      float expm1(      float z);
      float log1p(      float z);
     double expm1(     double z);
     double log1p(     double z);
long double expm1(long double z);
long double log1p(long double z);
} // namespace lmi

// TODO ?? Write functions here for other refactorable uses of
// std::pow() throughout lmi, to facilitate reuse and unit testing.

/// Forward partial summation.

template<typename T>
std::vector<T>& fwd_sum(std::vector<T>& v)
{
    std::partial_sum(v.begin(), v.end(), v.begin());
    return v;
}

/// Backward partial summation.

template<typename T>
std::vector<T>& back_sum(std::vector<T>& v)
{
    std::partial_sum(v.rbegin(), v.rend(), v.rbegin());
    return v;
}

// Some of these provide the typedefs that std::unary_function or
// std::binary_function would have provided, because they're still
// required for std::binder1st() or std::binder2nd(), or for PETE.

/// Divide integers, rounding away from zero.
///
/// This floating-point analogue may be useful for cross checking:
///   double z = (double)numerator / (double)denominator;
///   return (T) (0 < z) ? std::ceil(z) : std::floor(z);

template<typename T>
inline T outward_quotient(T numerator, T denominator)
{
    static_assert(std::is_integral_v<T>);

    if(0 == denominator)
        {
        throw std::domain_error("Denominator is zero.");
        }

    // "INT_MIN / -1" would overflow; but "false/bool(-1)" would not,
    // hence the "T(-1) < 0" test.
    constexpr T min = std::numeric_limits<T>::lowest();
    if(min == numerator && T(-1) < 0 && T(-1) == denominator)
        {
        throw std::domain_error("Division might overflow.");
        }

    T x = numerator / denominator;
    T y = 0 != numerator % denominator;
    return (0 < numerator == 0 < denominator) ? x + y : x - y;
}

/// Absolute value of relative error.
///
/// Sometimes one of the arguments is a canonical expected value, and
///   (observed-expected)/expected
/// is wanted, with due regard to sign; in that case, code (o-e)/e
/// directly. This function template, OTOH, is designed for the more
/// general case where neither argument is favored; then a denominator
/// must be chosen somehow, and the algebraic sign of the return value
/// would generally not be useful.
///
/// Here the smaller of the absolute values of the arguments is chosen
/// as the denominator. Alternatively, the greater, or some average,
/// could have been chosen; but choosing the smaller gives a more
/// conservative (i.e., larger) value.
///
/// Only floating point arguments are allowed, because no compelling
/// use case for integer types is foreseen. Similarly, both arguments
/// are constrained to be of the same type, because allowing different
/// types seems unimportant.

template<typename T>
/*constexpr*/ T relative_error(T t, T u)
{
    static_assert(std::is_floating_point_v<T>);
    constexpr T inf {std::numeric_limits<T>::infinity()};
    auto const denominator {std::min(std::fabs(t), std::fabs(u))};
    return
          (T{} == t && T{} == u) ? T{}
        : (T{} == denominator)   ? inf
        :                          std::fabs(t - u) / denominator
        ;
}

/// Signed zeros, for comparison tests.

enum signed_zero
    {pos0 // positive zero
    ,neg0 // negative zero
    };

template<typename T>
bool operator==(T t, signed_zero z)
{
    static_assert(std::is_floating_point_v<T>);
    bool const st = std::signbit(t);
    bool const sz =
          (pos0 == z) ? false
        : (neg0 == z) ? true
        : throw std::domain_error("outside signed_zero domain")
        ;
    return T(0) == t && sz == st;
}

/// Algebraic sign of argument.
///
/// Return value is of same type as argument, as for many members
/// of std::numeric_limits. Thus, (t * signum(t)) is of type T,
/// which would not always be the case if an int were returned.

template<typename T>
T signum(T t)
{
    static_assert(std::is_arithmetic_v<T>);
    return (0 == t) ? 0 : std::signbit(t) ? -1 : 1;
}

/// (Unsigned) absolute value of a signed integer.
///
/// Returns the absolute value of a signed integer, as a value of the
/// corresponding unsigned type. Notably, the additive inverse of a
/// negative integer is this absolute value.
///
/// Motivation: Attempting to take the absolute value of INT_MIN
/// directly, as "std::abs(INT_MIN)", entails undefined behavior.
/// So does attempting to take the additive inverse of INT_MIN by
/// direct negation, as "-INT_MIN".
///
/// Asserts that both integer types have no padding, to rule out the
///   UINT_MAX == INT_MAX == -(INT_MIN+1)
/// case that Daniel Fischer points out somewhere on the web.
///
/// Postconditions: for r = u_abs(t),
///  - r is of an unsigned type
///  - sizeof r == sizeof t
/// and, if t is negative, then
///  - 0 == t + r == r + t [for t < 0; incidentally, also for 0 == t]
///
/// If the cast on the last line is removed, then unary '-' performs
/// integral promotion on its operand and returns a result of the
/// promoted type, which is (signed) 'int' if type T is narrower than
/// 'int', so the conditional operator returns a result of that type,
/// which the function implicitly casts to its (narrower) return type.
/// Thus, the cast has no actual effect; it serves only to clarify the
/// intention.

template<typename T>
constexpr std::make_unsigned_t<T> u_abs(T t)
{
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_signed_v<T>);
    using U = std::make_unsigned_t<T>;
    static_assert(std::has_unique_object_representations_v<T>);
    static_assert(std::has_unique_object_representations_v<U>);
    U const u {static_cast<U>(t)};
    return (t < 0) ? static_cast<U>(-u) : u;
}

// Actuarial functions.
//
// Some inputs are nonsense, like interest rates less than 100%.
// Contemporary compilers usually handle such situations without
// raising a hardware exception. Trapping invalid input would add a
// runtime overhead of about twenty percent (measured with gcc-3.4.2);
// this is judged not to be worthwhile.
//
// Typically, the period 'n' is a constant known at compile time, so
// it is makes sense for it to be a non-type template parameter. To
// support some old <functional> code, specializations for the most
// common case, where 'n' equals twelve, are provided with the
// typedefs that std::unary_function formerly provided.
//
// General preconditions: 0 < 'n'; -1.0 <= 'i'; T is floating point.
//
// Implementation note: greater accuracy and speed are obtained by
// applying the transformation
//   (1+i)^n - 1 <-> expm1(log1p(i) * n)
// to naive power-based formulas.

template<typename T, int n>
struct i_upper_n_over_n_from_i
{
    static_assert(std::is_floating_point_v<T>);
    static_assert(0 < n);
    T operator()(T i) const
        {
        if(i < -1.0)
            {
            throw std::domain_error("i is less than -100%.");
            }

        if(-1.0 == i)
            {
            return -1.0;
            }

        // naively:    (1+i)^(1/n) - 1
        return lmi::expm1(lmi::log1p(i) / n);
        }
};

template<typename T>
struct i_upper_12_over_12_from_i
{
    using argument_type = T;
    using result_type   = T;
    static_assert(std::is_floating_point_v<T>);
    T operator()(T i) const
        {
        return i_upper_n_over_n_from_i<T,12>()(i);
        }
};

template<typename T, int n>
struct i_from_i_upper_n_over_n
{
    static_assert(std::is_floating_point_v<T>);
    static_assert(0 < n);
    T operator()(T i) const
        {
        // naively:    (1+i)^n - 1
        return lmi::expm1(lmi::log1p(i) * n);
        }
};

template<typename T>
struct i_from_i_upper_12_over_12
{
    static_assert(std::is_floating_point_v<T>);
    T operator()(T i) const
        {
        return i_from_i_upper_n_over_n<T,12>()(i);
        }
};

template<typename T, int n>
struct d_upper_n_from_i
{
    static_assert(std::is_floating_point_v<T>);
    static_assert(0 < n);
    T operator()(T i) const
        {
        if(i < -1.0)
            {
            throw std::domain_error("i is less than -100%.");
            }

        if(-1.0 == i)
            {
            throw std::range_error("i equals -100%.");
            }

        // naively:    n * (1 - (1+i)^(-1/n))
        return -n * lmi::expm1(lmi::log1p(i) / -n);
        }
};

template<typename T>
struct d_upper_12_from_i
{
    static_assert(std::is_floating_point_v<T>);
    T operator()(T i) const
        {
        return d_upper_n_from_i<T,12>()(i);
        }
};

/// Annual net from annual gross rate, with two different kinds of
/// decrements. See the interest-rate class for the motivation.
///
/// Additional precondition: arguments are not such as to cause the
/// result to be less than -1.0 .

template<typename T, int n>
struct net_i_from_gross
{
    static_assert(std::is_floating_point_v<T>);
    static_assert(0 < n);
    T operator()(T i, T spread, T fee) const
        {
        // naively:
        //   (1
        //   +   (1+     i)^(1/n)
        //   -   (1+spread)^(1/n)
        //   -         fee *(1/n)
        //   )^n - 1
        return lmi::expm1
            (
            n * lmi::log1p
                (   lmi::expm1(lmi::log1p(i)      / n)
                -   lmi::expm1(lmi::log1p(spread) / n)
                -              fee                / n
                )
            );
        }
};

/// Convert q to a monthly COI rate.
///
/// The COI rate is the monthly equivalent of q divided by one minus
/// itself, because deducting the COI charge at the beginning of the
/// month increases the amount actually at risk--see:
///   https://lists.nongnu.org/archive/html/lmi/2009-09/msg00001.html
///
/// The value of 'q' might exceed unity, for example if guaranteed COI
/// rates for simplified issue are 125% of 1980 CSO, so that case is
/// accommodated. A value of zero might arise from a partial-mortality
/// multiplier that equals zero for some or all durations, and that
/// case arises often enough to merit a special optimization.
///
/// Preconditions:
///   'max_coi' is in [0.0, 1.0]
///   'q' is nonnegative
/// An exception is thrown if any precondition is violated.
///
/// If 'q' exceeds unity, then 'max_coi' is returned. Notionally, 'q'
/// is a probability and cannot exceed unity, but it doesn't seem
/// implausible to most actuaries to set q to 125% of 1980 CSO and
/// expect it to limit itself.

template<typename T>
struct coi_rate_from_q
{
    using first_argument_type  = T;
    using second_argument_type = T;
    using result_type          = T;
    static_assert(std::is_floating_point_v<T>);
    T operator()(T q, T max_coi) const
        {
        if(!(0.0 <= max_coi && max_coi <= 1.0))
            {
            throw std::runtime_error("Maximum COI rate out of range.");
            }

        if(q < 0.0)
            {
            throw std::domain_error("q is negative.");
            }

        if(0.0 == q)
            {
            return 0.0;
            }
        else if(1.0 <= q)
            {
            return max_coi;
            }
        else
            {
            // naively:    1 - (1-q)^(1/12)
            T monthly_q = -lmi::expm1(lmi::log1p(-q) / 12);
            if(T(1) == monthly_q)
                {
                throw std::logic_error("Monthly q equals unity.");
                }
            monthly_q = monthly_q / (T(1) - monthly_q);
            return std::min(max_coi, monthly_q);
            }
        }
};

/// Midpoint for illustration reg.
///
/// Section 7(C)(1)(c)(ii) prescribes an "average" without specifying
/// which average to use. The arithmetic mean is used here because
/// that seems to be the most common practice. On the other hand, a
/// strong case can be made for using the geometric mean, at least
/// with interest and mortality rates.

template<typename T>
void assign_midpoint
    (std::vector<T>      & out
    ,std::vector<T> const& in_0
    ,std::vector<T> const& in_1
    )
{
    if(in_0.size() != in_1.size())
        {
        throw std::runtime_error("Vector addends are of unequal length.");
        }
    out.resize(in_0.size());
    std::transform
        (in_0.begin()
        ,in_0.end()
        ,in_1.begin()
        ,out.begin()
        ,[=](T t0, T t1) {return std::midpoint(t0, t1);}
        );
}

#endif // math_functions_hpp
