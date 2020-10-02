/*
 * IEEE754 constexpr parser toy. IEEE754 details.
 * Thanks to Algolia for giving me the opportunity to develop this toy!
 * @maintainer Xavier Roche (xavier dot roche at algolia.com)
 */
#pragma once

/**
 * IEEE754 Helpers.
 * References:
 * <https://en.wikipedia.org/wiki/Double-precision_floating-point_format>
 * <http://krashan.ppa.pl/articles/stringtofloat/>
 * <https://babbage.cs.qc.cuny.edu/IEEE-754/>
 */

#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <tuple>
#include <type_traits>

namespace ieee754toy {

/**
 * IEEE754 traits. The traits allows to handle any IEE754 width.
 *
 * @comment Type Traits related floating-point type
 * @comment IntegerType Integral type used to represent the floating-point type in IEEE754.
 * The integral number if a bitfield (platform-endianness) with the following layout:
 *   [sign][exponentBits][mantissaBits]
 *   The sign is always 1 bit.
 * @comment Mantissa Integral type used to represent the mantissa in either two or ten exponent base.
 * @comment Exponent Integral type used to represent the exponent in either two or ten exponent base.
 * @comment ReducedMantissa Integral type used to handle the mantissa when converting exponent base, without losing
 * precision.
 * @comment mantissaBits Number of bits for mantissa in IEE754
 * @comment exponentBits Number of bits for exponent in IEE754
 **/
template<typename T>
struct IEEE754Traits;

/** IEEE754 Single precision (aka. "float") **/
template<>
struct IEEE754Traits<float>
{
    using Type = float;
    using IntegerType = std::uint32_t;

    using Mantissa = std::uint32_t;
    using Exponent = std::int16_t;

    using ReducedMantissa = std::uint64_t;

    static constexpr std::size_t mantissaBits = 23;
    static constexpr std::size_t exponentBits = 8;
};

/** IEEE754 Double precision (aka "double") **/
template<>
struct IEEE754Traits<double>
{
    using Type = double;
    using IntegerType = std::uint64_t;

    using Mantissa = std::uint64_t;
    using Exponent = std::int32_t;

    using ReducedMantissa = __uint128_t;

    static constexpr std::size_t mantissaBits = 52;
    static constexpr std::size_t exponentBits = 11;
};

/** An IEEE754 binary (2-based) representation. **/
template<typename N>
struct IEEE754BinaryNumber
{
    using Base = IEEE754Traits<N>;
    using Mantissa = typename Base::Mantissa;
    using Exponent = typename Base::Exponent;
    using Integer = typename Base::IntegerType;

    /** The IEEE754 integer value **/
    Integer value;

    /** Base exponent **/
    static constexpr Exponent exponentBase = (Exponent{ 1 } << (Base::exponentBits - 1)) - 1;

    /** Maximum exponent value **/
    static constexpr Exponent exponentMax = (Exponent{ 1 } << (Base::exponentBits)) - exponentBase - 2;

    /** Minimum exponent value **/
    static constexpr Exponent exponentMin = 1 - exponentBase;

    /** Base exponent for subnormal numbers **/
    static constexpr Exponent exponentSubnormalBase = 1 - exponentBase;

    /** Minimum overall subnormal exponent (ie. when least significant bit is the only one set) **/
    static constexpr Exponent exponentSubnormalMin = exponentSubnormalBase - Base::mantissaBits;

    /**
     * Return the packed IEEE754 number, as the integer representation.
     * @param negative If @c true, the number is negative
     * @param mantissa The final mantissa
     * @param exponent The final exponent
     * @return The IEEE754 integer value. To get the double value, simply copy the inner bits as is.
     * @comment As reminder, the integral representation follows:
     *   Integer == [sign][exponentBits][mantissaBits]
     **/
    static constexpr Integer number(bool negative, Mantissa mantissa, Exponent exponent)
    {
        const Integer negativePart = negative ? (Integer{ 1 } << (Base::mantissaBits + Base::exponentBits)) : 0;
        const Integer exponentPart = static_cast<Integer>(exponent) << Base::mantissaBits;
        const auto mantissaPart = static_cast<Integer>(mantissa);
        assert(mantissaPart < (Integer{ 1 } << Base::mantissaBits));
        return negativePart | exponentPart | mantissaPart;
    }

    /**
     * Return zero.
     * @param negative If @c true, the number is negative (zero can be negative in floating-point)
     * @return The IEEE754 integer value. To get the double value, simply copy the inner bits as is.
     **/
    static constexpr Integer zero(bool negative = false) { return number(negative, 0, 0); }

    /**
     * Return infinity.
     * @param negative If @c true, the number is negative
     * @return The IEEE754 integer value. To get the double value, simply copy the inner bits as is.
     **/
    static constexpr Integer infinity(bool negative = false)
    {
        return number(negative, 0, ((1 << ieee754toy::IEEE754Traits<N>::exponentBits) - 1));
    }

    /**
     * Return NaN.
     * @return The IEEE754 integer value. To get the double value, simply copy the inner bits as is.
     **/
    static constexpr Integer nan()
    {
        return number(false, 1, ((1 << ieee754toy::IEEE754Traits<N>::exponentBits) - 1));
    }

    // Static assertions
    static_assert(sizeof(typename Base::Type) == sizeof(typename Base::IntegerType));
    static_assert(sizeof(typename Base::Type) * 8 == Base::mantissaBits + Base::exponentBits + 1);
    static_assert(sizeof(Mantissa) * 8 >= Base::mantissaBits);
    static_assert(sizeof(Exponent) * 8 >= Base::exponentBits);
    static_assert(sizeof(typename Base::ReducedMantissa) * 8 >= Base::mantissaBits);
};

// Basic IEEE754 checks.
static_assert(IEEE754BinaryNumber<double>::exponentBase == 1023);
static_assert(IEEE754BinaryNumber<double>::exponentMax == 1023);
static_assert(IEEE754BinaryNumber<double>::exponentMin == -1022);
static_assert(IEEE754BinaryNumber<double>::exponentSubnormalBase == -1022);
static_assert(IEEE754BinaryNumber<double>::exponentSubnormalMin == -1074);

/** A number, exploded into sign, mantissa and exponent. The base of the exponent is specified. **/
template<typename N, std::size_t Base>
struct IEEE754Number
{
    using Traits = IEEE754Traits<N>;
    using Mantissa = typename Traits::Mantissa;
    using Exponent = typename Traits::Exponent;
    using Integer = typename Traits::IntegerType;
    using ReducedMantissa = typename Traits::ReducedMantissa;

    /** Number of bits of precision for internal computation on the mantissa. **/
    static constexpr Exponent reducedMantissaBits = sizeof(ReducedMantissa) * 8;

    /**
     * Create a new IEE754 number.
     * @param n Negative sign
     * @param m Mantissa, normalized leading 1
     * @param e Exponent
     * @warning The specified parameters are in final binary format representation.
     **/
    constexpr IEEE754Number(bool n, Mantissa m, Exponent e)
      : negative(n)
      , mantissa(m)
      , exponent(e)
    {}

    /**
     * Convert the current number to base-2.
     * @warning The only supported converion currently is from base 10 to base 2.
     *
     * @comment Thanks to Grzegorz Kraszewski for his valuable explanation on exponent reduction
     * <http://krashan.ppa.pl/articles/stringtofloat/>
     */
    constexpr ieee754toy::IEEE754Number<N, 2> convertTwobase() const;

    /**
     * Augment the ten-exponent using factor of tenFactor (10**tenFactor), reducing two-exponent by a factor of
     * twoexponent (2**twoexponent)
     **/
    template<typename ieee754toy::IEEE754Number<N, Base>::Exponent tenFactor,
             typename ieee754toy::IEEE754Number<N, Base>::Exponent twoFactor>
    constexpr void increaseTenExponent(ReducedMantissa& varmantissa,
                                       Exponent& tenexponent,
                                       Exponent& twoexponent) const;

    /**
     * Reduce the ten-exponent using factor of tenFactor (10**tenFactor), increasing two-exponent by a factor of
     * twoexponent (2**twoexponent)
     **/
    template<typename ieee754toy::IEEE754Number<N, Base>::Exponent tenFactor,
             typename ieee754toy::IEEE754Number<N, Base>::Exponent twoFactor>
    constexpr void decreaseTenExponent(ReducedMantissa& varmantissa,
                                       Exponent& tenexponent,
                                       Exponent& twoexponent) const;

    /**
     * Convert this number to an integral floating-point representation.
     * @warning Only available for base 2 exponents.
     **/
    constexpr Integer toIEEE754() const;

    /**
     * Convert this IEEE754 integral number to a floating-point representation.
     * @warning Only available for base 2 exponents.
     **/
    static inline N toFloat(Integer i)
    {
        assert(Base == 2);
        const union
        {
            Integer i;
            N f;
        } u = { .i = i };
        static_assert(sizeof(u.i) == sizeof(u.f));
        return u.f;
    }

    /**
     * Convert this number to a floating-point representation.
     * @warning Only available for base 2 exponents.
     **/
    inline N toFloat() const { return toFloat(toIEEE754()); }

    /**
     * Return zero.
     * @param negative If @c true, the number is negative (zero can be negative in floating-point)
     * @return The IEEE754 float value.
     **/
    static constexpr N zero(bool negative = false) { return toFloat(IEEE754BinaryNumber<N>::zero(negative)); }

    /**
     * Return infinity.
     * @param negative If @c true, the number is negative
     * @return The IEEE754 float value.
     **/
    static constexpr N infinity(bool negative = false)
    {
        return toFloat(IEEE754BinaryNumber<N>::infinity(negative));
        ;
    }

    /**
     * Return NaN.
     * @return The IEEE754 float value.
     **/
    static constexpr N nan() { return toFloat(IEEE754BinaryNumber<N>::nan()); }

    /** Negative ? **/
    bool negative;

    /** Mantissa **/
    Mantissa mantissa;

    /** Exponent **/
    Exponent exponent;
};

/*
 * Divide by a divisor, rounding half to even.
 */
template<typename Integer, typename Divisor>
inline constexpr void divideBy(Integer& number, const Divisor& divisor)
{
    if ((number % divisor) != divisor / 2) {
        number /= divisor;
    }
    // Half-way case
    else {
        number /= divisor;

        // Round half-to-even
        if ((number & 1) != 0) {
            number++;
        }
    }
}

template<typename T>
inline constexpr T power(const T& t, std::size_t exp)
{
    T ret = 1;
    for (std::size_t i = 0; i < exp; i++) {
        ret *= t;
    }
    return ret;
}
static_assert(power(2, 4) == 16);

template<typename N, std::size_t Base>
template<typename ieee754toy::IEEE754Number<N, Base>::Exponent tenFactor,
         typename ieee754toy::IEEE754Number<N, Base>::Exponent twoFactor>
constexpr void ieee754toy::IEEE754Number<N, Base>::decreaseTenExponent(ReducedMantissa& varmantissa,
                                                                       Exponent& tenexponent,
                                                                       Exponent& twoexponent) const
{
    static_assert(power(ReducedMantissa{ 2 }, twoFactor) >= power(ReducedMantissa{ 10 }, tenFactor));
    using BinaryNumber = ieee754toy::IEEE754BinaryNumber<N>;
    constexpr auto tenMultiplier = power(ReducedMantissa{ 10 }, tenFactor);
    constexpr auto twoMultiplier = ReducedMantissa{ 1 } << twoFactor;

    // Decrease 10-base exponent
    while (tenexponent >= tenFactor) {
        // Are we overflowing the varmantissa ?
        if (varmantissa > std::numeric_limits<ReducedMantissa>::max() / twoMultiplier) {
            // Overflow, bail out
            if (twoexponent + twoFactor > BinaryNumber::exponentMax) {
                tenexponent = 0; // Finished
                varmantissa = 1;
                twoexponent = BinaryNumber::exponentMax + 1; // +/-Inf
                return;
            }

            // Divide the varmantissa
            divideBy(varmantissa, twoMultiplier);

            // Multiply through two-exponent
            twoexponent += twoFactor;
        }

        // Divide by decreasing ten-exponent
        tenexponent -= tenFactor;

        // Multiply through varmantissa
        varmantissa *= tenMultiplier;
    }
}

template<typename N, std::size_t Base>
template<typename ieee754toy::IEEE754Number<N, Base>::Exponent tenFactor,
         typename ieee754toy::IEEE754Number<N, Base>::Exponent twoFactor>
constexpr void ieee754toy::IEEE754Number<N, Base>::increaseTenExponent(ReducedMantissa& varmantissa,
                                                                       Exponent& tenexponent,
                                                                       Exponent& twoexponent) const
{
    static_assert(power(ReducedMantissa{ 2 }, twoFactor) >= power(ReducedMantissa{ 10 }, tenFactor));
    using BinaryNumber = ieee754toy::IEEE754BinaryNumber<N>;
    constexpr auto tenMultiplier = power(ReducedMantissa{ 10 }, tenFactor);
    constexpr auto twoMultiplier = ReducedMantissa{ 1 } << twoFactor;

    // Decrease 10-base exponent
    while (tenexponent <= -tenFactor) {
        // Do we have room on the left for the varmantissa ?
        while (varmantissa <= std::numeric_limits<ReducedMantissa>::max() / twoMultiplier) {
            // Guaranteed underflow, bail out. We need to take in account the accumulated bits in the mantissa
            // as it can be reduced.
            if (twoexponent < BinaryNumber::exponentSubnormalMin - reducedMantissaBits + twoexponent) {
                tenexponent = 0; // Finished
                varmantissa = 0;
                twoexponent = 0; // +/-0
                return;
            }

            // Multiply the varmantissa
            varmantissa *= twoMultiplier;

            // Divide through two-exponent
            twoexponent -= twoFactor;
        }

        // Multiply by increasing ten-exponent
        tenexponent += tenFactor;

        // Divide through varmantissa
        divideBy(varmantissa, tenMultiplier);
    }
}

template<typename N, std::size_t Base>
inline constexpr ieee754toy::IEEE754Number<N, 2> ieee754toy::IEEE754Number<N, Base>::convertTwobase() const
{
    if constexpr (Base == 2) {
        return *this;
    } else if constexpr (Base == 10) {
        using Number = ieee754toy::IEEE754Number<N, Base>;
        using BinaryNumber = ieee754toy::IEEE754BinaryNumber<N>;

        constexpr std::size_t mantissaBits = Number::Traits::mantissaBits;

        // Principle: we have a number that is:
        // v = mantissa · 10^tenexponent
        //
        // We first introduce a two-exponent which is initially zero (2^0 == 1)
        // v = mantissa · 10^tenexponent · 2^twoexponent
        //
        // We will then iterate and make this tenexponent zero by multiplying and/or dividing with very simple
        // arithmetic rules:
        // Multiply/Divide by ten can be done
        // - By multiplying/dividing the mantissa
        // - By incrementing/decrementing the ten-exponent
        // Multiply/Divide by two can be done
        // - By multiplying/dividing the mantissa
        // - By incrementing/decrementing the two-exponent
        //
        // Our goal is to decrease tenexponent if it is positive, and increase it if it is negative.
        // Each time we divide the mantissa, we have a risk of losing precision. This is why we need a wider
        // mantissa.

        // Expand ten-exponent toa larger width to have more precision
        Exponent tenexponent = exponent;

        // initially twoexponent is zero
        Exponent twoexponent = 0;

        // expand variable mantissa precision to avoid loss of precision during iterations
        ReducedMantissa varmantissa = mantissa;

        // Decrease 10-base exponent
        if (tenexponent > 0) {
            // Execute large steps first to be faster, then decrease
            decreaseTenExponent<9, 30>(varmantissa, tenexponent, twoexponent);
            decreaseTenExponent<6, 20>(varmantissa, tenexponent, twoexponent);
            decreaseTenExponent<3, 10>(varmantissa, tenexponent, twoexponent);
            decreaseTenExponent<1, 4>(varmantissa, tenexponent, twoexponent);
        }
        // Increase 10-base exponent
        else if (tenexponent < 0) {
            // Execute large steps first to be faster, then decrease
            increaseTenExponent<9, 30>(varmantissa, tenexponent, twoexponent);
            increaseTenExponent<6, 20>(varmantissa, tenexponent, twoexponent);
            increaseTenExponent<3, 10>(varmantissa, tenexponent, twoexponent);
            increaseTenExponent<1, 4>(varmantissa, tenexponent, twoexponent);
        }

        // At this stage we have no longer ten-exponent: we have only a two-exponent number
        assert(tenexponent == 0);

        // Zero is zero
        if (varmantissa == 0) {
            return ieee754toy::IEEE754Number<N, 2>(negative, 0, 0);
        }

        // Now we have a two-exponent number, let's normalize it (ie. find the leftmost bit equal to 1)

        constexpr std::size_t oneForEncoded = 1; // One bit is encoded through exponent
        constexpr std::size_t mantissaBitsKept = mantissaBits + oneForEncoded;

        // Now keep only leftmost 53 bit of varmantissa. First bit is implicitly equal to one, unless we have very
        // small (subnormal) numbers.

        // If no shift is done, leading bit at leftmost position is a 1^(mantissaBits + oneForEncoded - 1)
        Exponent position = mantissaBits + oneForEncoded - 1;

        // Attempt to reduce varmantissa to have at most <mantissaBits + 1> precision when 1 is leading
        // Exponent shift = 0;
        // for (; (varmantissa >> shift) >= (ReducedMantissa{ 1 } << mantissaBitsKept); shift++)
        //     ;
        // if (shift != 0) {
        //     divideBy(varmantissa, ReducedMantissa{ 1 } << shift);
        //     position += shift;
        // }
        while (varmantissa >= (ReducedMantissa{ 1 } << mantissaBitsKept)) {
            divideBy(varmantissa, 2);
            position++;
        }

        // Attempt to get the leading bit on leftmost position, unless we hit the exponent limit (subnormal case)
        while (varmantissa < (Mantissa{ 1 } << (mantissaBitsKept - 1)) &&
               twoexponent + position > BinaryNumber::exponentMin) {
            varmantissa *= 2;
            position--;
        }

        // Fix exponent after shift
        twoexponent += position;

        // We need to take care of possibly overflowing exponent, too.
        while (twoexponent < BinaryNumber::exponentMin) {
            divideBy(varmantissa, 2);
            twoexponent++;
        }

        return ieee754toy::IEEE754Number<N, 2>(negative, varmantissa, twoexponent);
    }
}

template<typename N, std::size_t Base>
constexpr typename ieee754toy::IEEE754Number<N, Base>::Integer ieee754toy::IEEE754Number<N, Base>::toIEEE754()
    const
{
    assert(Base == 2);

    // Mantissa
    // The format is written with the significand having an implicit integer bit of value 1
    // Exponent:
    // 00000000000=000 is used to represent a signed zero (if F = 0) and subnormals (if F ≠ 0)
    // 00000000001=001 2^{1-1023}=2^{-1022} (smallest exponent)
    // 01111111111=3ff 2^{1023-1023}=2^{0} (zero offset)
    // 11111111110=7fe 2^{2046-1023}=2^{1023} (highest exponent)
    // 11111111111=7ff is used to represent ∞ (if F = 0) and NaNs (if F ≠ 0)

    using Number = ieee754toy::IEEE754Number<N, Base>;
    using BinaryNumber = ieee754toy::IEEE754BinaryNumber<N>;

    constexpr auto mantissaBits = Number::Traits::mantissaBits;

    // Zero (can be positive or negative)
    if (mantissa == 0 || exponent < BinaryNumber::exponentSubnormalMin) {
        return BinaryNumber::zero(negative);
    }

    // Overflow (+/-Inf)
    if (exponent > BinaryNumber::exponentMax) {
        return BinaryNumber::infinity(negative);
    }

    // Leading extra bit is 1: regular encoding
    if ((mantissa & (Mantissa{ 1 } << mantissaBits)) != 0) {
        // We don't need the leading bit (53th) as it is implicitly 1 in IEEE754
        const auto normalizedMantissa = mantissa & ~(Mantissa{ 1 } << mantissaBits);

        assert(exponent <= BinaryNumber::exponentMax);
        assert(exponent >= BinaryNumber::exponentMin);
        assert(exponent + BinaryNumber::exponentBase != 0);
        return BinaryNumber::number(negative, normalizedMantissa, exponent + BinaryNumber::exponentBase);
    }

    // At this point, this is a subnormal, we expect a fixed exponent
    assert(exponent == BinaryNumber::exponentSubnormalBase);

    return BinaryNumber::number(negative, mantissa, 0);
}

}; // namespace ieee754toy
