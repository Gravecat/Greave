// core/core-constants.h -- A few constants that are used elsewhere in the code, such as the version number, the number of exits from rooms, etc.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#ifndef GREAVE_CORE_CORE_CONSTANTS_H
#define GREAVE_CORE_CORE_CONSTANTS_H

#include <cstdint>


struct CoreConstants
{
    static constexpr uint32_t   SAVE_VERSION =      83;     // The version number for saved game files. This should increment when old saves can no longer be loaded.
    static constexpr uint32_t   TAGS_PERMANENT =    10000;  // The tag number at which tags are considered permanent.
    static const char           GAME_VERSION[];             // The game's version number.
};

#endif  // GREAVE_CORE_CORE_CONSTANTS_H
