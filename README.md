# IEEE754 constexpr parser toy

## Goal

Being able to parse a floating-point number, and convert if into a IEE754 integral integer, including in a `constexpr` environment.

See [example](main.cpp).

## Features

* Type-agnostic parsers (can parse `char`, `char16_t`, `char32_t`, etc.)
* Can operate on ranges
* Compile-time evaluations (`constexpr`)

## The Solution

We are first parsing the mantissa and exponent (see [`parseMantissaExponent`](include/NumericalParser.h)), and convert the two-exponent iteratively using a compensation method (see [`convertTwobase`](include/IEEE754.h)). While the method is probably not the fastest method, it is simple enough to fit gently with `constexpr` context.

The resulting code can be used to parse at compile-time double numbers:

```c++
static_assert(toMantissaExponent(toArray("0.123456789012345678901234567890123456789012345678901234"))
                  .convertTwobase()
                  .mantissa == 0b11111100110101101110100110111010001101111011001011111UL);
```

## Logic

* `NumericalParser::toDouble` : We parse the IEEE754 formatted string using several `constexpr` helpers:
![Format](https://upload.wikimedia.org/wikipedia/commons/a/a9/IEEE_754_Double_Floating_Point_Format.svg)
  * `NumericalParser::parseMantissaExponent` : Parse both mantissa and exponent (eg. `12.3456E+12`) and return a tuple
    * The number of characters parsed (0 for error)
    * The sign (`true` for negative)
    * The mantissa parsed
    * The final exponent extracted from the mantissa and the exponent
      * `NumericalParser::parseMantissa` : Parse a mantissa (eg. `12.3456`) and return a tuple
        * The number of characters parsed (0 for error)
        * The sign (`true` for negative)
        * The mantissa parsed
        * The exponent extracted from the mantissa (eg. 1 with 100 zeros will yield an exponent)
      * `NumericalParser::parseExponent` : Parse an exponent (eg. `+12`) and return a tuple
        * The number of characters parsed (0 for error)
        * The sign (`true` for negative)
        * The exponent
* `IEEE754Number::convertTwobase` : We then convert the ten-based mantissa/exponent into two-based version, using an iterative method
    *  We consider the broken-down number as
        `v = mantissa · 10^tenexponent`
    *  We first introduce a two-exponent which is initially zero (2^0 == 1)
        `v = mantissa · 10^tenexponent · 2^twoexponent`
    *  `IEEE754Number::decreaseTenExponent` : We will then iterate and make this tenexponent zero by multiplying and/or dividing with very simple arithmetic rules:
        *  Multiply/Divide by ten can be done
            * By multiplying/dividing the mantissa
            * By incrementing/decrementing the ten-exponent
        *  Multiply/Divide by two can be done
            * By multiplying/dividing the mantissa
            * By incrementing/decrementing the two-exponent

    Our goal is to decrease tenexponent if it is positive, and increase it if it is negative.
    Each time we divide the mantissa, we have a risk of losing precision. This is why we need a wider (128-bit for `double`) mantissa.
* `IEEE754BinaryNumber::number` : We return the `double` number which is then packed mantissa/exponent info

## Why Not `std::from_chars` ?

An easier alternative would have been to use `std::from_chars`. However, this suffers from several issues:

* Does not handle `char16_t` or higher
* Can't be adapted to parse integers and/or floating-point numbers at once
* Not even available in clang 10

## Requirements

A modern compiler (`clang` or `gcc`) with C++20 support (for `std::span`, the rest is C++17)

You may have to adapt the [`CMakeLists`](CMakeLists.txt)) file.

## Current State

[Static tests](tests/IEEE754Tests.h) are almost passing, but some corner-cases are still erroneous, possibly due to the incorrect rounding strategy.

## References

* [String To Floating Point Number Conversion](http://krashan.ppa.pl/articles/stringtofloat/)
* [IEEE_754](https://en.wikipedia.org/wiki/IEEE_754)
* [Double-precision floating-point format](https://en.wikipedia.org/wiki/Double-precision_floating-point_format)

## Thanks

* Thanks to Grzegorz Kraszewski for his valuable explanation on exponent reduction
* Thanks to Algolia to let us explore this kind of topics!
