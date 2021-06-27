// core/mathx.hpp -- Various utility functions that deal with math and number-related things.
// Copyright (c) 2009-2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#pragma once
#include "core/greave.hpp"


class MathX
{
public:
    static Direction    dir_invert(Direction dir);                  // Inverts a Direction enum (north becomes south, etc.)
    static uint8_t      dir_invert(uint8_t dir);                    // As above, but using integers.
    static double       round_to(double num, unsigned int digits);  // Rounds a float to a specified number of digits.
    static float        round_to_two(float num);                    // Rounds a float to two decimal places.
};
