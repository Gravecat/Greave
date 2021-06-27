// actions/travel.cpp -- Actions allowing the player and NPCs to move around the game world.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#include "actions/doors.hpp"
#include "actions/look.hpp"
#include "actions/travel.hpp"
#include "core/core.hpp"
#include "core/strx.hpp"
#include "world/mobile.hpp"
#include "world/room.hpp"
#include "world/world.hpp"


// Attempts to move from one Room to another.
bool ActionTravel::travel(std::shared_ptr<Mobile> mob, Direction dir)
{
    const uint32_t mob_loc = mob->location();
    const std::shared_ptr<Room> room = core()->world()->get_room(mob_loc);
    const bool is_player = (mob->type() == Mobile::Type::PLAYER);
    const uint32_t room_link = room->link(dir);

    if (!room_link)
    {
        if (is_player) core()->message("{y}You cannot travel {Y}" + StrX::dir_to_name(dir, StrX::DirNameType::TO_THE_ALT) + "{y}.");
        // todo: NPC failure messages
        return false;
    }
    else if (room_link == Room::UNFINISHED)
    {
        if (is_player) core()->message("{y}That part of the game is {Y}curently unfinished{y}. Please come back later.");
        return false;
    }
    else if (room_link == Room::BLOCKED)
    {
        if (is_player) core()->message("{y}You are {Y}unable to proceeed {y}any further in that direction.");
        return false;
    }

    if (room->link_tag(dir, LinkTag::Openable) && !room->link_tag(dir, LinkTag::Open))
    {
        if (is_player) core()->message("{m}(first opening the " + room->door_name(dir) + ")");
        const bool opened = ActionDoors::open_or_close(mob, dir, true);
        if (!opened) return false;
    }

    mob->set_location(room_link);
    if (is_player) ActionLook::look(mob);
    return true;
}