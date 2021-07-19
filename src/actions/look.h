// actions/look.h -- Look around you. Just look around you.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#ifndef GREAVE_ACTIONS_LOOK_H_
#define GREAVE_ACTIONS_LOOK_H_

#include "core/parser.h"
#include "world/mobile.h"

#include <cstddef>
#include <memory>


class ActionLook
{
public:
    static void examine(ParserTarget target_type, size_t target, size_t target_second); // Examines an Item or Mobile.
    static void look();                     // Take a look around at your surroundings.
    static void obvious_exits(bool indent); // Lists the exits from this area.

private:
    static void examine_item(std::shared_ptr<Item> target);     // Examines an Item.
    static void examine_mobile(std::shared_ptr<Mobile> target); // Examines a Mobile.
};

#endif  // GREAVE_ACTIONS_LOOK_H_
