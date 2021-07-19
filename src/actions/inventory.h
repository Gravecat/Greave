// actions/inventory.h -- Actions related to inventory management, picking up and dropping items, etc.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#ifndef GREAVE_ACTIONS_INVENTORY_H_
#define GREAVE_ACTIONS_INVENTORY_H_

#include "core/parser.h"
#include "world/mobile.h"

#include <cstddef>
#include <memory>


class ActionInventory
{
public:
    static void check_inventory();                                                              // Checks to see what's being carried.
    static void drop(std::shared_ptr<Mobile> mob, size_t item_pos, int count, bool confirm);    // Drops an item on the ground.
    static bool equip(std::shared_ptr<Mobile> mob, size_t item_pos, bool confirm);              // Wields or wears an equippable item.
    static void equipment();                                                                    // Checks to see what's wielded and/or worn.
    static void take(std::shared_ptr<Mobile> mob, ParserTarget target_type, size_t item_pos, size_t item_pos_second, int count, bool confirm);    // Takes an item from the ground or a container.
    static bool unequip(std::shared_ptr<Mobile> mob, size_t item_pos, bool confirm);            // Unequips a worn or wielded item.
    static bool unequip(std::shared_ptr<Mobile> mob, EquipSlot slot, bool confirm);             // As above, but specifying an EquipSlot.

private:
    static constexpr float  TIME_DROP_ITEM =        1;      // The time taken (in seconds) to drop an item on the ground.
    static constexpr float  TIME_EQUIP_ABOUT =      20;     // The time taken (in seconds) to equip something about the body, like a cloak.
    static constexpr float  TIME_EQUIP_ARMOUR =     180;    // The time taken (in seconds) to equip armour worn over the body, like a breastplate.
    static constexpr float  TIME_EQUIP_BODY =       300;    // The time taken (in seconds) to equip armour worn against the body, like a hauberk.
    static constexpr float  TIME_EQUIP_FEET =       30;     // The time taken (in seconds) to equip boots or other things worn on the feet.
    static constexpr float  TIME_EQUIP_HANDS =      6;      // The time taken (in seconds) to equip gloves or something else worn on the hands.
    static constexpr float  TIME_EQUIP_HEAD =       2;      // The time taken (in seconds) to equip a helmet or so mething else worn on the head.
    static constexpr float  TIME_EQUIP_WEAPON =     0.6f;   // The time taken (in seconds) to equip a weapon or something else held in the hand.
    static constexpr float  TIME_GET_ITEM =         5;      // The time taken (in seconds) to pick up an item from the ground.
    static constexpr float  TIME_UNEQUIP_ABOUT =    10;     // The time taken (in seconds) to unequip something about the body, like a cloak.
    static constexpr float  TIME_UNEQUIP_ARMOUR =   120;    // The time taken (in seconds) to unequip armour worn over the body, like a breastplate.
    static constexpr float  TIME_UNEQUIP_BODY =     180;    // The time taken (in seconds) to unequip armour worn against the body, like a hauberk.
    static constexpr float  TIME_UNEQUIP_FEET =     15;     // The time taken (in seconds) to unequip boots or other things worn on the feet.
    static constexpr float  TIME_UNEQUIP_HANDS =    4;      // The time taken (in seconds) to unequip gloves or something else worn on the hands.
    static constexpr float  TIME_UNEQUIP_HEAD =     2;      // The time taken (in seconds) to unequip a helmet or so mething else worn on the head.
    static constexpr float  TIME_UNEQUIP_WEAPON =   0.5f;   // The time taken (in seconds) to unequip a weapon or something else held in the hand.

    static bool count_check(std::shared_ptr<Item> item, int count); // Checks that a player-input count is a valid number.
    static void weight_and_money();                                 // Shows the total carry weight and currency the Mobile is carrying.
};

#endif  // GREAVE_ACTIONS_INVENTORY_H_
