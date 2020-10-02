/*
 * IEEE754 constexpr parser toy. Main parser.
 * Thanks to Algolia for giving me the opportunity to develop this toy!
 * @maintainer Xavier Roche (xavier dot roche at algolia.com)
 */
#pragma once

#include "IEEE754.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <tuple>
#include <type_traits>

namespace ieee754toy {

/**
 * Numerical parsing helpers.
 **/
template<typename T>
class NumericalParser : private std::span<T>
{
public:
    /** A base-10 IEEE754 number. **/
    template<typename N>
    using DecimalNumber = struct IEEE754Number<N, 10>;

public:
    /** Create a new numerical parser **/
    template<typename... Ts>
    constexpr NumericalParser(Ts&&... args)
      : std::span<T>(std::forward<Ts>(args)...)
    {}

    /* Extract the mantissa from a double string, and return the result as mantissa and exponent.
     * Return a tuple of the parsed size (zero if error), and the exploded number.
     */
    template<typename N = double>
    constexpr std::tuple<std::size_t, DecimalNumber<N>> parseMantissa() const;

    /* Extract the exponent from a string.
     * Return a tuple of the parsed size (zero if error), and the the exponent.
     */
    template<typename N = double>
    constexpr std::tuple<std::size_t, typename DecimalNumber<N>::Exponent> parseExponent() const;

    /* Extract the mantissa and the exponent from a double string.
     * Return a tuple of the parsed size (zero if error), and the exploded number.
     */
    template<typename N = double>
    constexpr inline std::tuple<std::size_t, DecimalNumber<N>> parseMantissaExponent() const;

    /**
     * Convert the current string into a floating point value
     *
     * @param[out] error Set to @c true upon error
     * @return The parsed double, or @c 0 upon error.
     * @comment The empty string yields 0, and any non-finite (ie. infinite or NaN) yields an error.
     */
    template<typename N = double>
    inline double toDouble(bool& error) const
    {
        return toAnyDouble<N>(error);
    }

    /**
     * Convert the current string into a floating point value
     *
     * @param[in] defaultValue Default value to be returned on invalid value
     * @return The parsed double, or @c defaultValue upon error.
     * @comment The empty string yields 0, and any non-finite (ie. infinite or NaN) yields an error.
     */
    inline double toDouble(double defaultValue = 0.0) const
    {
        bool error = false;
        const auto value = toDouble(error);
        return not error ? value : defaultValue;
    }

    /**
     * Convert the current string into a floating point value of any type.
     * @param[out] error Set to @c true upon error
     * @return The parsed double.
     * @comment Double The returned type
     * @comment IntegralMantissa The intermediate integral unsigned type that can store the complete mantissa part.
     * @comment IntegralExponent The intermediate integral signed type that can store the complete mantissa part.
     */
    template<typename N = double>
    inline N toAnyDouble(bool& error) const;

private:
    using std::span<T>::size;
    using std::span<T>::data;
    using std::span<T>::end;
    using std::span<T>::operator[];

    /* Convert to lowercase. */
    inline static constexpr T toLower(const T c) { return c >= 'A' && c <= 'Z' ? (c + 'a' - 'A') : c; }

    /** Compare to a 8-bit ascii string, case insensitive. **/
    template<std::size_t N>
    inline bool operator==(const char (&str)[N]) const
    {
        static_assert(N != 0);
        constexpr std::size_t Length = N - 1;
        if (size() != Length) {
            return false;
        }
        for (std::size_t i = 0; i < Length; i++) {
            if (toLower(operator[](i)) != toLower(str[i])) {
                return false;
            }
        }
        return true;
    }
};

// Deduction guides.
template<typename Type>
explicit NumericalParser(Type* begin, std::size_t size) -> NumericalParser<Type>;
template<typename Type>
explicit NumericalParser(Type* begin, Type* end) -> NumericalParser<Type>;

/* Extract the mantissa from a double string, and return the result as mantissa and exponent.
 * Return a tuple of the parsed size (zero if error), the sign, the mantissa, and the exponent.
 */
template<typename T>
template<typename N>
constexpr std::tuple<std::size_t, typename NumericalParser<T>::template DecimalNumber<N>>
NumericalParser<T>::parseMantissa() const
{
    using Mantissa = typename DecimalNumber<N>::Mantissa;

    constexpr const auto error = std::make_tuple(std::size_t(0), DecimalNumber<N>(false, 0, 0));

    // Mantissa
    typename DecimalNumber<N>::Mantissa mantissa = 0;

    // Power-of-two-exponent
    typename DecimalNumber<N>::Exponent exponent = 0;

    // If true, mantissa is negative
    bool negative = false;

    // If true, we are beyond a comma
    bool stopExponent = false;

    // If true, mantissa is already too large
    bool stopMantissa = false;

    // Digits seen for mantissa
    bool digits = false;

    for (std::size_t i = 0; i <= size(); i++) {
        const auto c = i < size() ? operator[](i) : 0;
        switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                // If true, mantissa was too large on previous round
                bool justStoppedMantissa = false;

                // New digit
                const unsigned digit = c - '0';
                digits = true;

                // Handle overflows (mul by 10 and add at most 9)
                if (not stopMantissa and mantissa >= std::numeric_limits<Mantissa>::max() / 10) {
                    if (mantissa > std::numeric_limits<Mantissa>::max() / 10 ||
                        (mantissa == std::numeric_limits<Mantissa>::max() / 10 &&
                         mantissa > (std::numeric_limits<Mantissa>::max() - digit) / 10)) {
                        justStoppedMantissa = true;
                        stopMantissa = true;
                    }
                }

                // If not overflowing, accumulate
                if (not stopMantissa) {
                    mantissa *= 10;
                    mantissa += digit;
                } else {
                    // Simply increase exponent
                    exponent++;

                    // We just stopped the mantissa; we need to handle round half to even
                    if (justStoppedMantissa) {
                        // Round to upper value!
                        if (digit > 5 || (digit == 5 && (mantissa & 1) != 0)) {
                            // Take care of overflows
                            if (++mantissa == 0) {
                                // Decrease ten exponent; take max/10
                                mantissa = std::numeric_limits<Mantissa>::max() / 10;
                                exponent++;
                                // Check if the last digit rounded to upper value increases mantissa
                                // This is the case for 64-bit, for example: last digit of 18446744073709551615 is
                                // 5, and 5 rounded to ten is always 10, as the maximum is always even (pwoer of
                                // two minus one).
                                if constexpr (std::numeric_limits<Mantissa>::max() % 10 + 1 >= 5) {
                                    mantissa++;
                                }
                            }
                        }
                    }
                }

                // If beyond comma, decrease exponent
                if (stopExponent) {
                    exponent--;
                }
            } break;
            case '+':
            case '-':
                if (i != 0) {
                    return error;
                }
                negative = c == '-';
                break;
            case '.':
                if (stopExponent) {
                    return error;
                }
                stopExponent = true;
                break;
            default:
                if (not digits) {
                    return error;
                }
                return { i, { negative, mantissa, exponent } };
        }
    }
    assert(!"Unreachable code");
}

/* Extract the exponent from a string.
 * Return a tuple of the parsed size (zero if error), and the the exponent.
 */
template<typename T>
template<typename N>
constexpr std::tuple<std::size_t, typename NumericalParser<T>::template DecimalNumber<N>::Exponent>
NumericalParser<T>::parseExponent() const
{
    using Exponent = typename DecimalNumber<N>::Exponent;

    constexpr const auto error = std::make_tuple(std::size_t(0), Exponent(0));

    // Power-of-two-exponent
    Exponent exponent = 0;

    // If true, mantissa is negative
    bool negative = false;

    for (std::size_t i = 0; i <= size(); i++) {
        const auto c = i < size() ? operator[](i) : 0;
        switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                const auto digit = static_cast<int>(c - '0');

                // Handle overflows
                if (exponent >= std::numeric_limits<Exponent>::max() / 10 - 1) {
                    if (exponent > std::numeric_limits<Exponent>::max() / 10 ||
                        exponent * 10 > std::numeric_limits<Exponent>::max() - digit) {
                        return error;
                    }
                }

                // If not overflowing, accumulate
                exponent *= 10;
                exponent += digit;
            } break;
            case '+':
            case '-':
                if (i != 0) {
                    return error;
                }
                negative = c == '-';
                break;
            default:
                return std::make_tuple(i, not negative ? exponent : -exponent);
        }
    }
    assert(!"Unreachable code");
}

/* Extract the mantissa and the exponent from a double string.
 * Return a tuple of the parsed size (zero if error), the sign, the mantissa, and the exponent.
 */
template<typename T>
template<typename N>
constexpr inline std::tuple<std::size_t, typename NumericalParser<T>::template DecimalNumber<N>>
NumericalParser<T>::parseMantissaExponent() const
{
    constexpr const auto error = std::make_tuple(std::size_t(0), DecimalNumber<N>(false, 0, 0));

    // Parse mantissa. This will also provide a first exponent (for very large numbers typically)
    auto result = parseMantissa<N>();
    auto& [parsed, number] = result;
    if (parsed == 0) {
        return error;
    }

    // Check for an optional explicit exponent
    switch (parsed < size() ? operator[](parsed) : 0) {
        case 'e':
        case 'E': {
            auto& [negative, mantissa, exponent] = number;
            parsed++;
            const auto [parsedExponent, explicitExponent] =
                NumericalParser<T>(data() + parsed, size() - parsed).parseExponent<N>();
            if (parsedExponent == 0) {
                return error;
            }
            parsed += parsedExponent;
            exponent += explicitExponent;
        } break;
    }

    return result;
}

template<typename T>
template<typename N>
inline N NumericalParser<T>::toAnyDouble(bool& error) const
{
    using Number = ieee754toy::IEEE754Number<N, 2>;
    const auto [parsed, number] = parseMantissaExponent<N>();
    error = parsed != size();

    if (not error) {
        return number.convertTwobase().toFloat();
    } else if (*this == "Inf" || *this == "+Inf") {
        error = false;
        return Number::infinity(false);
    } else if (*this == "-Inf") {
        error = false;
        return Number::infinity(true);
    } else if (*this == "NaN") {
        error = false;
        return Number::nan();
    } else {
        return N{};
    }
}

}; // namespace ieee754toy
