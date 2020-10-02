/*
 * IEEE754 constexpr parser toy. Static tests.
 * Thanks to Algolia for giving me the opportunity to develop this toy!
 * @maintainer Xavier Roche (xavier dot roche at algolia.com)
 */

#include "NumericalParser.h"
#include <array>
#include <tuple>

using namespace ieee754toy;

// Helper to have constexpr arrays
template<typename T, size_t N, std::size_t... S>
constexpr auto toArray(const T (&s)[N], std::index_sequence<S...>)
{
    return std::array<T, sizeof...(S)>{ { s[S]... } };
}

/* Transform a string literal into a std::array<AByte> suitable for std::span constructor. */
template<typename T, size_t N>
constexpr auto toArray(const T (&s)[N])
{
    return toArray(s, std::make_index_sequence<N - 1>());
}

// Convert a std::tuple<std::size_t, Number<...>> into a flat std::tuple<std::size_t, bool, uint64_t int64_t> tuple
template<typename T>
static constexpr auto unpack(const T& t)
{
    auto [parsed, num] = t;
    auto [negative, mantissa, exponent] = num;
    return std::tuple(parsed, negative, mantissa, exponent);
}

void testParseDoubleStatic()
{
    static_assert(unpack(NumericalParser<const char>("", 0).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0, 0));
    static_assert(unpack(NumericalParser<const char>(toArray("--126")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0.0, 0));
    static_assert(unpack(NumericalParser<const char>(toArray("-+126")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0.0, 0));
    static_assert(unpack(NumericalParser<const char>(toArray("1.1.1")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0.0, 0));

    static_assert(unpack(NumericalParser<const char>(toArray("-126")).parseMantissaExponent()) ==
                  std::make_tuple(4, true, 126.0, 0));
    static_assert(unpack(NumericalParser<const char>(toArray("126")).parseMantissaExponent()) ==
                  std::make_tuple(3, false, 126.0, 0));
    static_assert(unpack(NumericalParser<const char>(toArray("+126")).parseMantissaExponent()) ==
                  std::make_tuple(4, false, 126.0, 0));
    static_assert(unpack(NumericalParser<const char>(toArray("0")).parseMantissaExponent()) ==
                  std::make_tuple(1, false, 0.0, 0));
    static_assert(unpack(NumericalParser<const char>(toArray("-0")).parseMantissaExponent()) ==
                  std::make_tuple(2, true, 0.0, 0));

    static_assert(unpack(NumericalParser<const char>(toArray("1.0")).parseMantissaExponent()) ==
                  std::make_tuple(3, false, 10, -1));
    static_assert(unpack(NumericalParser<const char>(toArray("-1.23456")).parseMantissaExponent()) ==
                  std::make_tuple(8, true, 123456, -5));
    static_assert(unpack(NumericalParser<const char>(toArray("-1.2345678901234567890")).parseMantissaExponent()) ==
                  std::make_tuple(22, true, 12345678901234567890UL, -19));
    static_assert(unpack(NumericalParser<const char>(toArray("-123456.78901234567890")).parseMantissaExponent()) ==
                  std::make_tuple(22, true, 12345678901234567890UL, -14));
    static_assert(unpack(NumericalParser<const char>(toArray("-1234567890123456.7890")).parseMantissaExponent()) ==
                  std::make_tuple(22, true, 12345678901234567890UL, -4));
    static_assert(unpack(NumericalParser<const char>(toArray("-12345678901234567890")).parseMantissaExponent()) ==
                  std::make_tuple(21, true, 12345678901234567890UL, 0));
    static_assert(
        std::get<2>(unpack(NumericalParser<const char>(toArray("-1234567890123456789000000000000000000000"))
                               .parseMantissaExponent())) == 12345678901234567890UL);
    static_assert(unpack(NumericalParser<const char>(toArray("-1234567890123456789000000000000000000000"))
                             .parseMantissaExponent()) == std::make_tuple(41, true, 12345678901234567890UL, 20));

    static_assert(unpack(NumericalParser<const char>(toArray("1.2e+2")).parseMantissaExponent()) ==
                  std::make_tuple(6, false, 12, 1));
    static_assert(unpack(NumericalParser<const char>(toArray("1.2e+200")).parseMantissaExponent()) ==
                  std::make_tuple(8, false, 12, 199));
    static_assert(unpack(NumericalParser<const char>(toArray("1.2e+2000")).parseMantissaExponent()) ==
                  std::make_tuple(9, false, 12, 1999));
    static_assert(unpack(NumericalParser<const char>(toArray("1.2e-2")).parseMantissaExponent()) ==
                  std::make_tuple(6, false, 12, -3));
    static_assert(unpack(NumericalParser<const char>(toArray("1.2e-200")).parseMantissaExponent()) ==
                  std::make_tuple(8, false, 12, -201));
    static_assert(unpack(NumericalParser<const char>(toArray("-1.2e-2")).parseMantissaExponent()) ==
                  std::make_tuple(7, true, 12, -3));
    static_assert(unpack(NumericalParser<const char>(toArray("-1.2e-200")).parseMantissaExponent()) ==
                  std::make_tuple(9, true, 12, -201));
    static_assert(
        unpack(NumericalParser<const char>(toArray("3.141592653589793238462643383279502884197169399375105820974"))
                   .parseMantissaExponent()) == std::make_tuple(59, false, 3141592653589793238, -18));
    static_assert(
        unpack(NumericalParser<const char>(toArray("3141592653589793238462643383279502884197169399375105820974"))
                   .parseMantissaExponent()) == std::make_tuple(58, false, 3141592653589793238, 58 - 19));
    static_assert(
        unpack(NumericalParser<const char>(toArray("4.9406564584124654E-324")).parseMantissaExponent()) ==
        std::make_tuple(23, false, 49406564584124654, -324 - 16));
    static_assert(unpack(NumericalParser<const char>(toArray("NaN")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0, 0));
    static_assert(unpack(NumericalParser<const char>(toArray("+Inf")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0, 0));
    static_assert(unpack(NumericalParser<const char>(toArray("-Inf")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0, 0));
}

void testParseDoubleStatic16()
{
    static_assert(unpack(NumericalParser<const char16_t>(u"", 0).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0.0, 0));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"--126")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0.0, 0));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"-+126")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0.0, 0));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"1.1.1")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0.0, 0));

    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"-126")).parseMantissaExponent()) ==
                  std::make_tuple(4, true, 126.0, 0));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"126")).parseMantissaExponent()) ==
                  std::make_tuple(3, false, 126.0, 0));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"+126")).parseMantissaExponent()) ==
                  std::make_tuple(4, false, 126.0, 0));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"0")).parseMantissaExponent()) ==
                  std::make_tuple(1, false, 0.0, 0));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"-0")).parseMantissaExponent()) ==
                  std::make_tuple(2, true, 0.0, 0));

    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"1.0")).parseMantissaExponent()) ==
                  std::make_tuple(3, false, 10, -1));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"-1.23456")).parseMantissaExponent()) ==
                  std::make_tuple(8, true, 123456, -5));
    static_assert(
        unpack(NumericalParser<const char16_t>(toArray(u"-1.2345678901234567890")).parseMantissaExponent()) ==
        std::make_tuple(22, true, 12345678901234567890UL, -19));
    static_assert(
        unpack(NumericalParser<const char16_t>(toArray(u"-123456.78901234567890")).parseMantissaExponent()) ==
        std::make_tuple(22, true, 12345678901234567890UL, -14));
    static_assert(
        unpack(NumericalParser<const char16_t>(toArray(u"-1234567890123456.7890")).parseMantissaExponent()) ==
        std::make_tuple(22, true, 12345678901234567890UL, -4));
    static_assert(
        unpack(NumericalParser<const char16_t>(toArray(u"-12345678901234567890")).parseMantissaExponent()) ==
        std::make_tuple(21, true, 12345678901234567890UL, 0));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"-1234567890123456789000000000000000000000"))
                             .parseMantissaExponent()) == std::make_tuple(41, true, 12345678901234567890UL, 20));

    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"1.2e+2")).parseMantissaExponent()) ==
                  std::make_tuple(6, false, 12, 1));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"1.2e+200")).parseMantissaExponent()) ==
                  std::make_tuple(8, false, 12, 199));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"1.2e+2000")).parseMantissaExponent()) ==
                  std::make_tuple(9, false, 12, 1999));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"1.2e-2")).parseMantissaExponent()) ==
                  std::make_tuple(6, false, 12, -3));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"1.2e-200")).parseMantissaExponent()) ==
                  std::make_tuple(8, false, 12, -201));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"-1.2e-2")).parseMantissaExponent()) ==
                  std::make_tuple(7, true, 12, -3));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"-1.2e-200")).parseMantissaExponent()) ==
                  std::make_tuple(9, true, 12, -201));
    static_assert(unpack(NumericalParser<const char16_t>(
                             toArray(u"3.141592653589793238462643383279502884197169399375105820974"))
                             .parseMantissaExponent()) == std::make_tuple(59, false, 3141592653589793238, -18));
    static_assert(
        unpack(
            NumericalParser<const char16_t>(toArray(u"3141592653589793238462643383279502884197169399375105820974"))
                .parseMantissaExponent()) == std::make_tuple(58, false, 3141592653589793238, 58 - 19));
    static_assert(
        unpack(NumericalParser<const char16_t>(toArray(u"4.9406564584124654E-324")).parseMantissaExponent()) ==
        std::make_tuple(23, false, 49406564584124654, -324 - 16));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"NaN")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0, 0));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"+Inf")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0, 0));
    static_assert(unpack(NumericalParser<const char16_t>(toArray(u"-Inf")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0, 0));
}

void testParseDoubleStatic32()
{
    static_assert(unpack(NumericalParser<const char32_t>(U"", 0).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0.0, 0));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"--126")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0.0, 0));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"-+126")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0.0, 0));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"1.1.1")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0.0, 0));

    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"-126")).parseMantissaExponent()) ==
                  std::make_tuple(4, true, 126.0, 0));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"126")).parseMantissaExponent()) ==
                  std::make_tuple(3, false, 126.0, 0));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"+126")).parseMantissaExponent()) ==
                  std::make_tuple(4, false, 126.0, 0));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"0")).parseMantissaExponent()) ==
                  std::make_tuple(1, false, 0.0, 0));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"-0")).parseMantissaExponent()) ==
                  std::make_tuple(2, true, 0.0, 0));

    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"1.0")).parseMantissaExponent()) ==
                  std::make_tuple(3, false, 10, -1));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"-1.23456")).parseMantissaExponent()) ==
                  std::make_tuple(8, true, 123456, -5));
    static_assert(
        unpack(NumericalParser<const char32_t>(toArray(U"-1.2345678901234567890")).parseMantissaExponent()) ==
        std::make_tuple(22, true, 12345678901234567890UL, -19));
    static_assert(
        unpack(NumericalParser<const char32_t>(toArray(U"-123456.78901234567890")).parseMantissaExponent()) ==
        std::make_tuple(22, true, 12345678901234567890UL, -14));
    static_assert(
        unpack(NumericalParser<const char32_t>(toArray(U"-1234567890123456.7890")).parseMantissaExponent()) ==
        std::make_tuple(22, true, 12345678901234567890UL, -4));
    static_assert(
        unpack(NumericalParser<const char32_t>(toArray(U"-12345678901234567890")).parseMantissaExponent()) ==
        std::make_tuple(21, true, 12345678901234567890UL, 0));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"-1234567890123456789000000000000000000000"))
                             .parseMantissaExponent()) == std::make_tuple(41, true, 12345678901234567890UL, 20));

    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"1.2e+2")).parseMantissaExponent()) ==
                  std::make_tuple(6, false, 12, 1));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"1.2e+200")).parseMantissaExponent()) ==
                  std::make_tuple(8, false, 12, 199));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"1.2e+2000")).parseMantissaExponent()) ==
                  std::make_tuple(9, false, 12, 1999));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"1.2e-2")).parseMantissaExponent()) ==
                  std::make_tuple(6, false, 12, -3));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"1.2e-200")).parseMantissaExponent()) ==
                  std::make_tuple(8, false, 12, -201));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"-1.2e-2")).parseMantissaExponent()) ==
                  std::make_tuple(7, true, 12, -3));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"-1.2e-200")).parseMantissaExponent()) ==
                  std::make_tuple(9, true, 12, -201));
    static_assert(unpack(NumericalParser<const char32_t>(
                             toArray(U"3.141592653589793238462643383279502884197169399375105820974"))
                             .parseMantissaExponent()) == std::make_tuple(59, false, 3141592653589793238, -18));
    static_assert(unpack(NumericalParser<const char32_t>(
                             toArray(U"3141592653589793238462643383279502884197169399375105820974"))
                             .parseMantissaExponent()) == std::make_tuple(58, false, 3141592653589793238, 39));
    static_assert(
        unpack(NumericalParser<const char32_t>(toArray(U"4.9406564584124654E-324")).parseMantissaExponent()) ==
        std::make_tuple(23, false, 49406564584124654, -324 - 16));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"NaN")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0, 0));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"+Inf")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0, 0));
    static_assert(unpack(NumericalParser<const char32_t>(toArray(U"-Inf")).parseMantissaExponent()) ==
                  std::make_tuple(0, false, 0, 0));
}

void testParseDoubleLimits()
{
    // Note: 18446744073709551615 == 2**64-1
    static_assert(unpack(NumericalParser<const char>(toArray("18446744073709551614")).parseMantissaExponent()) ==
                  std::make_tuple(20, false, 18446744073709551614UL, 0));
    static_assert(unpack(NumericalParser<const char>(toArray("18446744073709551615")).parseMantissaExponent()) ==
                  std::make_tuple(20, false, 18446744073709551615UL, 0));
    static_assert(unpack(NumericalParser<const char>(toArray("18446744073709551616")).parseMantissaExponent()) ==
                  std::make_tuple(20, false, 1844674407370955162UL, 1));
    static_assert(unpack(NumericalParser<const char>(toArray("184467440737095516150")).parseMantissaExponent()) ==
                  std::make_tuple(21, false, 18446744073709551615UL, 1));
    static_assert(unpack(NumericalParser<const char>(toArray("184467440737095516154")).parseMantissaExponent()) ==
                  std::make_tuple(21, false, 18446744073709551615UL, 1));
    static_assert(unpack(NumericalParser<const char>(toArray("184467440737095516155")).parseMantissaExponent()) ==
                  std::make_tuple(21, false, 1844674407370955162UL, 2));
    static_assert(unpack(NumericalParser<const char>(toArray("184467440737095516156")).parseMantissaExponent()) ==
                  std::make_tuple(21, false, 1844674407370955162UL, 2));
    static_assert(unpack(NumericalParser<const char>(toArray("184467440737095516166")).parseMantissaExponent()) ==
                  std::make_tuple(21, false, 1844674407370955162UL, 2));
    static_assert(
        unpack(NumericalParser<const char>(toArray("18446744073709551615111")).parseMantissaExponent()) ==
        std::make_tuple(23, false, 18446744073709551615UL, 3));
    static_assert(
        unpack(NumericalParser<const char>(toArray("18446744073709551615411")).parseMantissaExponent()) ==
        std::make_tuple(23, false, 18446744073709551615UL, 3));
    static_assert(
        unpack(NumericalParser<const char>(toArray("18446744073709551615499")).parseMantissaExponent()) ==
        std::make_tuple(23, false, 18446744073709551615UL, 3));
    static_assert(
        unpack(NumericalParser<const char>(toArray("18446744073709551615511")).parseMantissaExponent()) ==
        std::make_tuple(23, false, 1844674407370955162UL, 4));

    static_assert(unpack(NumericalParser<const char>(toArray("100000000000000011110")).parseMantissaExponent()) ==
                  std::make_tuple(21, false, 10000000000000001111UL, 1));

    // This is round to nearest even
    static_assert(unpack(NumericalParser<const char>(toArray("100000000000000011105")).parseMantissaExponent()) ==
                  std::make_tuple(21, false, 10000000000000001110UL, 1));
}

template<typename T>
constexpr inline auto toMantissaExponent(const T& s)
{
    const NumericalParser<const char> parser(s);
    const auto [parsed, number] = parser.parseMantissaExponent<double>();
    return number;
}

template<typename T>
constexpr inline std::uint64_t toIEEE754Double(const T& s)
{
    return toMantissaExponent(s).convertTwobase().toIEEE754();
}

void testParseDoubleStaticIEEE754()
{
    // Values are based on an external sources:
    // <https://babbage.cs.qc.cuny.edu/IEEE-754/>

    static_assert(toMantissaExponent(toArray("0")).convertTwobase().exponent == 0);
    static_assert(toMantissaExponent(toArray("0")).convertTwobase().mantissa == 0);

    static_assert(toMantissaExponent(toArray("1")).convertTwobase().exponent == 0);
    static_assert(toMantissaExponent(toArray("1")).convertTwobase().mantissa ==
                  0b10000000000000000000000000000000000000000000000000000UL);

    static_assert(toMantissaExponent(toArray("2")).convertTwobase().exponent == 1);
    static_assert(toMantissaExponent(toArray("2")).convertTwobase().mantissa ==
                  0b10000000000000000000000000000000000000000000000000000UL);

    static_assert(toMantissaExponent(toArray("10")).convertTwobase().exponent == 3);
    static_assert(toMantissaExponent(toArray("10")).convertTwobase().mantissa ==
                  0b10100000000000000000000000000000000000000000000000000UL);

    static_assert(toMantissaExponent(toArray("10000")).convertTwobase().exponent == 13);
    static_assert(toMantissaExponent(toArray("10000")).convertTwobase().mantissa ==
                  0b10011100010000000000000000000000000000000000000000000UL);

    static_assert(toMantissaExponent(toArray("100000000000000")).convertTwobase().exponent == 46);
    static_assert(toMantissaExponent(toArray("100000000000000")).convertTwobase().mantissa ==
                  0b10110101111001100010000011110100100000000000000000000UL);

    static_assert(toMantissaExponent(toArray("12345678901234567890123456789012345678901234567890123456789012345678"
                                             "90123456789012345678901234567890"))
                      .convertTwobase()
                      .exponent == 329);
    static_assert(toMantissaExponent(toArray("12345678901234567890123456789012345678901234567890123456789012345678"
                                             "90123456789012345678901234567890"))
                      .convertTwobase()
                      .mantissa == 0b0010000011111110000010111010000101111111010001101001);

    static_assert(toMantissaExponent(toArray("0.123456789012345678901234567890123456789012345678901234"))
                      .convertTwobase()
                      .exponent == -4);
    static_assert(toMantissaExponent(toArray("0.123456789012345678901234567890123456789012345678901234"))
                      .convertTwobase()
                      .mantissa == 0b11111100110101101110100110111010001101111011001011111UL);

    static_assert(toMantissaExponent(toArray("1.0976931348623157E308")).convertTwobase().exponent == 1023);
    static_assert(toMantissaExponent(toArray("1.0976931348623157E308")).convertTwobase().mantissa ==
                  0b10011100010100010001001010101011101001000110100101001UL);

    static_assert(toMantissaExponent(toArray("0.123456789012345678901234567890123456789012345678901234E-123"))
                      .convertTwobase()
                      .exponent == -412);
    static_assert(toMantissaExponent(toArray("0.123456789012345678901234567890123456789012345678901234E-123"))
                      .convertTwobase()
                      .mantissa == 0b10100111001001000001110010110111101110101110110000000UL);

    static_assert(toMantissaExponent(toArray("0.123456789012345678901234567890123456789012345678901234E-300"))
                      .convertTwobase()
                      .exponent == -1000);
    static_assert(toMantissaExponent(toArray("0.123456789012345678901234567890123456789012345678901234E-300"))
                      .convertTwobase()
                      .mantissa == 0b10101001010100110010011100011010010111010000011010011UL);

    static_assert(toMantissaExponent(toArray("3."
                                             "14159265358979323846264338327950288419716939937510582097494459230781"
                                             "64062862089986280348253421170679"))
                      .convertTwobase()
                      .exponent == 1);
    static_assert(toMantissaExponent(toArray("3."
                                             "14159265358979323846264338327950288419716939937510582097494459230781"
                                             "64062862089986280348253421170679"))
                      .convertTwobase()
                      .mantissa == 0b11001001000011111101101010100010001000010110100011000UL);

    static_assert(1.00000000000000011100 == 1);
    static_assert(toMantissaExponent(toArray("1.00000000000000011100")).convertTwobase().exponent == 0);

    static_assert(1.00000000000000011105 > 1);
    static_assert(toMantissaExponent(toArray("1.00000000000000011105")).convertTwobase().exponent == 0);
    static_assert(toMantissaExponent(toArray("1.00000000000000011105")).convertTwobase().mantissa ==
                  0b10000000000000000000000000000000000000000000000000001UL);
    static_assert(toMantissaExponent(toArray("1.00000000000000011105")).convertTwobase().mantissa ==
                  0b10000000000000000000000000000000000000000000000000000UL);

    static_assert(1.00000000000000011110 > 1);
    static_assert(toMantissaExponent(toArray("1.00000000000000011110")).convertTwobase().exponent == 0);
    static_assert(toMantissaExponent(toArray("1.00000000000000011110")).convertTwobase().mantissa ==
                  0b10000000000000000000000000000000000000000000000000001UL);
    static_assert(toMantissaExponent(toArray("1.00000000000000011110")).convertTwobase().mantissa ==
                  0b10000000000000000000000000000000000000000000000000000UL);

    static_assert(1.000000000000000148 > 1);
    static_assert(toMantissaExponent(toArray("1.000000000000000148")).convertTwobase().exponent == 0);
    static_assert(toMantissaExponent(toArray("1.000000000000000148")).convertTwobase().mantissa ==
                  0b10000000000000000000000000000000000000000000000000001UL);

    static_assert(1.000000000000000149 > 1);
    static_assert(toMantissaExponent(toArray("1.000000000000000149")).convertTwobase().exponent == 0);
    static_assert(toMantissaExponent(toArray("1.000000000000000149")).convertTwobase().mantissa ==
                  0b10000000000000000000000000000000000000000000000000001UL);

    // <https://en.wikipedia.org/wiki/Double-precision_floating-point_format>

    // Min. subnormal positive double
    static_assert(toMantissaExponent(toArray("4.9406564584124654E-324")).convertTwobase().exponent == -1022);
    static_assert(toMantissaExponent(toArray("4.9406564584124654E-324")).convertTwobase().mantissa == 1);

    // Max. subnormal double
    static_assert(toMantissaExponent(toArray("2.2250738585072009E-308")).convertTwobase().exponent == -1022);
    static_assert(toMantissaExponent(toArray("2.2250738585072009E-308")).convertTwobase().mantissa ==
                  0b01111111111111111111111111111111111111111111111111111UL);

    // Min. normal positive double
    static_assert(toMantissaExponent(toArray("2.2250738585072014E-308")).convertTwobase().exponent == -1022);
    static_assert(toMantissaExponent(toArray("2.2250738585072014E-308")).convertTwobase().mantissa ==
                  0b10000000000000000000000000000000000000000000000000000UL);

    // Max. Double
    static_assert(toMantissaExponent(toArray("1.7976931348623157E308")).convertTwobase().exponent == 1023);
    static_assert(toMantissaExponent(toArray("1.7976931348623157E308")).convertTwobase().mantissa ==
                  0b11111111111111111111111111111111111111111111111111111UL);
}

void testParseDoubleStaticIEEE754Binary()
{
    // Values are based on an external sources:
    // <https://babbage.cs.qc.cuny.edu/IEEE-754/>
    // Values are all "Round to the Nearest Value" as glibc's strtod()

    static_assert(toMantissaExponent(toArray("0")).convertTwobase().toIEEE754() == 0x0);
    static_assert(toMantissaExponent(toArray("1")).convertTwobase().toIEEE754() == 0x3FF0000000000000);
    static_assert(toMantissaExponent(toArray("5")).convertTwobase().toIEEE754() == 0x4014000000000000);
    static_assert(toMantissaExponent(toArray("-0")).convertTwobase().toIEEE754() == 0x8000000000000000);
    static_assert(toMantissaExponent(toArray("-1")).convertTwobase().toIEEE754() == 0xBFF0000000000000);
    static_assert(toMantissaExponent(toArray("-5")).convertTwobase().toIEEE754() == 0xC014000000000000);

    static_assert(toMantissaExponent(toArray("4.9406564584124654E-325")).convertTwobase().toIEEE754() == 0x0);
    static_assert(toMantissaExponent(toArray("4.9406564584124654E-324")).convertTwobase().toIEEE754() == 0x1);
    static_assert(toMantissaExponent(toArray("4.9406564584124654E-323")).convertTwobase().toIEEE754() == 0xA);
    static_assert(toMantissaExponent(toArray("4.9406564584124654E-322")).convertTwobase().toIEEE754() == 0x64);
    static_assert(toMantissaExponent(toArray("4.9406564584124654E-321")).convertTwobase().toIEEE754() == 0x3e8);
    static_assert(toMantissaExponent(toArray("4.9406564584124654E-320")).convertTwobase().toIEEE754() == 0x2710);
    static_assert(toMantissaExponent(toArray("4.9406564584124654E-224")).convertTwobase().toIEEE754() ==
                  0x119249AD2594C37D);
    static_assert(toMantissaExponent(toArray("4.9406564584124654E-124")).convertTwobase().toIEEE754() ==
                  0x2654E718D7D7625A);
    static_assert(toMantissaExponent(toArray("4.9406564584124654E-24")).convertTwobase().toIEEE754() ==
                  0x3B17E43C8800759C);

    static_assert(toMantissaExponent(toArray("3.1415926535897932")).convertTwobase().toIEEE754() ==
                  0x400921FB54442D18);
    static_assert(toMantissaExponent(toArray("3.1415926535897932E+10")).convertTwobase().toIEEE754() ==
                  0x421D4223FC1F977B);
    static_assert(toMantissaExponent(toArray("3.1415926535897932E-10")).convertTwobase().toIEEE754() ==
                  0x3DF596BF8CE7631E);
    static_assert(toMantissaExponent(toArray("3.1415926535897932E+100")).convertTwobase().toIEEE754() ==
                  0x54CCB9F5C3F2EB84);
    static_assert(toMantissaExponent(toArray("3.1415926535897932E-100")).convertTwobase().toIEEE754() ==
                  0x2B45FD17AE3BF80C);

    static_assert(toMantissaExponent(toArray("3.1415926535897932E+200")).convertTwobase().toIEEE754() ==
                  0x69906ABDE5F4E1B5);
    static_assert(toMantissaExponent(toArray("3.1415926535897932E+300")).convertTwobase().toIEEE754() ==
                  0x7E52C3AE4DD16CAF);
    static_assert(toMantissaExponent(toArray("3.1415926535897932E+400")).convertTwobase().toIEEE754() ==
                  0x7FF0000000000000);
    static_assert(toMantissaExponent(toArray("-3.1415926535897932E+200")).convertTwobase().toIEEE754() ==
                  0xE9906ABDE5F4E1B5);
    static_assert(toMantissaExponent(toArray("-3.1415926535897932E+300")).convertTwobase().toIEEE754() ==
                  0xFE52C3AE4DD16CAF);
    static_assert(toMantissaExponent(toArray("-3.1415926535897932E+400")).convertTwobase().toIEEE754() ==
                  0xFFF0000000000000);

    // <https://sourceware.org/bugzilla/show_bug.cgi?id=3479
    // "default IEEE rounding should have rounded up to 0x42c0000000000002 (nearest-ties-to-even)"
    // Note that V8 engine does not seem to have this got right (returning 0x42c0000000000001)
    static_assert(toMantissaExponent(toArray("3.518437208883201171875E+013")).convertTwobase().toIEEE754() ==
                  0x42c0000000000002);

    static_assert(
        toMantissaExponent(
            toArray(
                "0."
                "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
                "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
                "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
                "0000000000000000000000000000000024703282292062327208828439643411068618252990130716238221279284125"
                "0337753635104375932649918180817996189898282347722858865463328355177969898199387398005390939063150"
                "3565951557022639229085839244910518443593180284993653615250031937045767824"))
            .convertTwobase()
            .toIEEE754() == 0);

    static_assert(toMantissaExponent(toArray("1.00000005960464477550")).convertTwobase().toIEEE754() ==
                  0x3FF0000010000000);
    static_assert(toMantissaExponent(toArray("1.0000000596046447755")).convertTwobase().toIEEE754() ==
                  0x3FF0000010000000);
    static_assert(toMantissaExponent(toArray("1.000000059604644776")).convertTwobase().toIEEE754() ==
                  0x3FF0000010000000);
    static_assert(toMantissaExponent(toArray("1.000000059604644775")).convertTwobase().toIEEE754() ==
                  0x3FF0000010000000);
    static_assert(toMantissaExponent(toArray("1.00000005960464478")).convertTwobase().toIEEE754() ==
                  0x3FF0000010000000);
    static_assert(toMantissaExponent(toArray("1.0000000596046448")).convertTwobase().toIEEE754() ==
                  0x3FF0000010000000);
    static_assert(toMantissaExponent(toArray("1.000000059604645")).convertTwobase().toIEEE754() ==
                  0x3FF0000010000001);

    static_assert(toMantissaExponent(toArray("1.00000005960464")).convertTwobase().toIEEE754() ==
                  0x3FF000000FFFFFEA);
    static_assert(toMantissaExponent(toArray("1.0000000596046")).convertTwobase().toIEEE754() ==
                  0x3FF000000FFFFF36);
    static_assert(toMantissaExponent(toArray("1.000000059605")).convertTwobase().toIEEE754() ==
                  0x3FF0000010000640);
    static_assert(toMantissaExponent(toArray("1.00000005960")).convertTwobase().toIEEE754() == 0x3FF000000FFFAE4A);
    static_assert(toMantissaExponent(toArray("1.0000000596")).convertTwobase().toIEEE754() == 0x3FF000000FFFAE4A);
    static_assert(toMantissaExponent(toArray("1.000000060")).convertTwobase().toIEEE754() == 0x3FF00000101B2B2A);
    static_assert(toMantissaExponent(toArray("1.00000006")).convertTwobase().toIEEE754() == 0x3FF00000101B2B2A);
    static_assert(toMantissaExponent(toArray("1.0000001")).convertTwobase().toIEEE754() == 0x3FF000001AD7F29B);
    static_assert(toMantissaExponent(toArray("1.000000")).convertTwobase().toIEEE754() == 0x3FF0000000000000);

    static_assert(toMantissaExponent(toArray("1.00000000000000022204460492503")).convertTwobase().toIEEE754() ==
                  0x3FF0000000000001);
    static_assert(toMantissaExponent(toArray("1.000000000000000111")).convertTwobase().toIEEE754() ==
                  0x3FF0000000000000);
    static_assert(toMantissaExponent(toArray("1.000000000000000111019999")).convertTwobase().toIEEE754() ==
                  0x3FF0000000000000);
    static_assert(toMantissaExponent(toArray("1.000000000000000111022")).convertTwobase().toIEEE754() ==
                  0x3FF0000000000000);

    static_assert(toMantissaExponent(toArray("1.00000000000000011102230246252")).convertTwobase().toIEEE754() ==
                  0x3FF0000000000001);
    static_assert(toMantissaExponent(toArray("1.00000000000000011105")).convertTwobase().toIEEE754() ==
                  0x3FF0000000000001);
    static_assert(toMantissaExponent(toArray("1.00000000000000011113072267976")).convertTwobase().toIEEE754() ==
                  0x3FF0000000000001);

    // <https://en.wikipedia.org/wiki/Double-precision_floating-point_format>

    // Min. subnormal positive double
    static_assert(toMantissaExponent(toArray("4.9406564584124654E-324")).convertTwobase().toIEEE754() == 1);

    // Max. subnormal double
    static_assert(toMantissaExponent(toArray("2.2250738585072009E-308")).convertTwobase().toIEEE754() ==
                  0x000FFFFFFFFFFFFF);

    // Min. normal positive double
    static_assert(toMantissaExponent(toArray("2.2250738585072014E-308")).convertTwobase().toIEEE754() ==
                  0x0010000000000000);

    // Max. Double
    static_assert(toMantissaExponent(toArray("1.7976931348623157E308")).convertTwobase().toIEEE754() ==
                  0x7FEFFFFFFFFFFFFF);
}
