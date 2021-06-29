// actions/cheat.hpp -- Cheating, debugging and testing commands.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#pragma once
#include "core/greave.hpp"


class ActionsCheat
{
public:
    static void spawn_item(std::string item);   // Attempts to spawn an item.
    static void teleport(std::string dest);     // Attemtps to teleport to another room.
};