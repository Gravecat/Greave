// actions/potions.h -- Consuming magical potions for various effects.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#ifndef GREAVE_ACTIONS_POTIONS_H_
#define GREAVE_ACT

#include <cstddef>


class ActionPotions
{
public:
    static void drink(size_t inv_pos, bool confirm);    // Drinks a specified inventory item.

private:
    static constexpr int    HYDRATION_FROM_POTION = 1;  // The hydration gained from drinking a potion.
};

#endif  // GREAVE_ACTIONS_POTIONS_H_
