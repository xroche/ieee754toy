/*
 * IEEE754 constexpr parser toy. Main program.
 * Thanks to Algolia for giving me the opportunity to develop this toy!
 * @maintainer Xavier Roche (xavier dot roche at algolia.com)
 */

#include "NumericalParser.h"

#include <iostream>
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; i++) {
        ieee754toy::NumericalParser parser(argv[i], strlen(argv[i]));
        bool error{ false };
        auto value = parser.toDouble(error);
        if (not error) {
            std::cout << value << std::endl;
        } else {
            std::cout << "<error>" << std::endl;
        }
    }

    return EXIT_SUCCESS;
}