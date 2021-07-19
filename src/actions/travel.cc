// actions/travel.cc -- Actions allowing the player and NPCs to move around the game world.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#include "actions/arena.h"
#include "actions/combat.h"
#include "actions/doors.h"
#include "actions/look.h"
#include "actions/travel.h"
#include "core/core.h"
#include "core/mathx.h"
#include "core/strx.h"

#include <cmath>


// Attempts to move from one Room to another.
bool ActionTravel::travel(std::shared_ptr<Mobile> mob, Direction dir, bool confirm)
{
    const auto world = core()->world();
    const uint32_t mob_loc = mob->location();
    const auto player = world->player();
    const uint32_t player_loc = player->location();
    const std::shared_ptr<Room> room = world->get_room(mob_loc);
    const bool is_player = mob->is_player();
    const uint32_t room_link = room->link(dir);
    const bool player_resting = player->tag(MobileTag::Resting);

    if (!room_link)
    {
        if (is_player) core()->message("{y}You cannot travel {Y}" + StrX::dir_to_name(dir, StrX::DirNameType::TO_THE_ALT) + "{y}.");
        return false;
    }
    else if (room_link == Room::UNFINISHED)
    {
        if (is_player) core()->message("{y}That part of the game is {Y}currently unfinished{y}. Please come back later.");
        return false;
    }
    else if (room_link == Room::BLOCKED)
    {
        if (is_player) core()->message("{y}You are {Y}unable to proceeed {y}any further in that direction.");
        return false;
    }

    if (room->link_tag(dir, LinkTag::Openable) && !room->link_tag(dir, LinkTag::Open))
    {
        if (is_player) core()->message("{0}{m}(first opening the " + room->door_name(dir) + ")");
        const bool opened = ActionDoors::open_or_close(mob, dir, true, confirm);
        if (!opened) return false;
    }

    const bool sky = room->link_tag(dir, LinkTag::Sky);
    const bool sky2 = room->link_tag(dir, LinkTag::Sky2);
    const bool sky3 = room->link_tag(dir, LinkTag::Sky3);
    if ((sky || sky2 || sky3) && !confirm)
    {
        core()->message("{r}You risk taking damage or even dying from making a jump like that!");
        core()->parser()->confirm_message();
        return false;
    }

    // Check if the player has any items that will have to be left behind.
    if (is_player && room->tag(RoomTag::Tavern) && !world->get_room(room_link)->tag(RoomTag::Tavern))
    {
        std::vector<size_t> item_vec_ids;
        for (size_t i = 0; i < mob->inv()->count(); i++)
        {
            const auto item = mob->inv()->get(i);
            if (!item->tag(ItemTag::TavernOnly)) continue;
            item_vec_ids.push_back(i);
        }
        if (item_vec_ids.size())
        {
            std::vector<std::string> item_names;
            for (auto vi : item_vec_ids)
                item_names.push_back("{C}" + mob->inv()->get(vi)->name(Item::NAME_FLAG_NO_COLOUR | Item::NAME_FLAG_NO_COUNT) + "{c}");
            StrX::collapse_list(item_names);
            std::string cl_str = StrX::comma_list(item_names, StrX::CL_AND);
            if (!confirm)
            {
                core()->message("{c}You will have to leave behind your " + cl_str + ".");
                core()->parser()->confirm_message();
                return false;
            }
            StrX::find_and_replace(cl_str, "{c}", "{m}");
            StrX::find_and_replace(cl_str, "{C}", "{m}");
            core()->message("{m}(leaving behind your " + cl_str + ")");
            for (ssize_t i = item_vec_ids.size() - 1; i >= 0; i--)
            {
                room->inv()->add_item(mob->inv()->get(item_vec_ids.at(i)));
                mob->inv()->erase(item_vec_ids.at(i));
            }
        }
    }

    // If the player is travelling, temporarily disable mobile roaming in both rooms.
    if (is_player)
    {
        room->set_tag(RoomTag::NoRoam_Temp);
        world->get_room(room_link)->set_tag(RoomTag::NoRoam_Temp);
    }

    // Calculate how long this travel will take.
    float travel_time = TRAVEL_TIME_NORMAL;
    if (room->link_tag(dir, LinkTag::DoubleLength)) travel_time = TRAVEL_TIME_DOUBLE;
    else if (room->link_tag(dir, LinkTag::TripleLength)) travel_time = TRAVEL_TIME_TRIPLE;
    const bool interrupted = (!mob->pass_time(travel_time, !confirm));

    // Again, if it's the player, clear the tags here -- that way, they're cleared regardless of the failure conditions below.
    if (is_player)
    {
        room->clear_tag(RoomTag::NoRoam_Temp);
        world->get_room(room_link)->clear_tag(RoomTag::NoRoam_Temp);
    }

    // Now we can check the failure conditions.
    if (interrupted)
    {
        core()->parser()->interrupted("leave");
        return false;
    }
    if (mob->is_dead()) return false;

    const bool player_can_see = room->light();
    const std::string mob_name_the = (player_can_see ? mob->name(Mobile::NAME_FLAG_THE | Mobile::NAME_FLAG_CAPITALIZE_FIRST) : "Something");
    const std::string mob_name_a = (player_can_see ? mob->name(Mobile::NAME_FLAG_A | Mobile::NAME_FLAG_CAPITALIZE_FIRST) : "Something");

    if (sky || sky2 || sky3)
    {
        if (is_player) core()->message("{C}You take a {U}leap of faith{C}!");
        else if (mob_loc == player_loc && !player_resting)
            core()->message("{U}" + mob_name_the + " {U}takes a leap of faith " + StrX::dir_to_name(dir, StrX::DirNameType::TO_THE_ALT) + "!");
    }
    else if (!is_player && mob_loc == player_loc && !player_resting)
        core()->message("{U}" + mob_name_the + " {U}leaves " + StrX::dir_to_name(dir, StrX::DirNameType::TO_THE_ALT) + ".");

    mob->set_location(room_link);
    if (is_player) ActionLook::look();
    else if (room_link == player_loc && !player_resting)
        core()->message("{U}" + mob_name_a + " {U}arrives " + StrX::dir_to_name(MathX::dir_invert(dir), StrX::DirNameType::FROM_THE_ALT) + ".");

    if (sky || sky2 || sky3)
    {
        int min_perc = 0, rng_perc = 0, blood_intensity = 0, fallen_dist = 0;
        if (sky3)
        {
            min_perc = FALL_3_STOREY_MIN_PERC;
            rng_perc = FALL_3_STOREY_RNG_PERC;
            blood_intensity = FALL_3_STOREY_BLEED;
            fallen_dist = 3;
        }
        else if (sky2)
        {
            min_perc = FALL_2_STOREY_MIN_PERC;
            rng_perc = FALL_2_STOREY_RNG_PERC;
            blood_intensity = FALL_2_STOREY_BLEED;
            fallen_dist = 2;
        }
        else
        {
            min_perc = FALL_1_STOREY_MIN_PERC;
            rng_perc = FALL_1_STOREY_RNG_PERC;
            blood_intensity = FALL_1_STOREY_BLEED;
            fallen_dist = 1;
        }
        float damage_perc = static_cast<float>(min_perc + core()->rng()->rnd(rng_perc)) / 100.0f;
        const int safe_fall = (is_player ? player->skill_level("SAFE_FALL") : 0);
        if (safe_fall > 0) damage_perc -= (static_cast<float>(core()->rng()->rnd(safe_fall)) / 100.0f);

        if (damage_perc > 0)
        {
            uint32_t hp_damage = std::round(static_cast<float>(mob->hp(true)) * damage_perc);
            mob->reduce_hp(hp_damage);
            if (is_player)
            {
                core()->message("{R}You land badly, and the impact " + Combat::damage_str(hp_damage, mob, false) + " {R}you! {W}<{R}-" + StrX::intostr_pretty(hp_damage) + "{W}>");
                if (mob->hp() > 0) player->gain_skill_xp("SAFE_FALL", XP_PER_SAFE_FALL_FAIL * fallen_dist);
            }
            else
            {
                if (room_link == player_loc && !player_resting)
                {
                    if (player_can_see) core()->message("{U}" + mob_name_a + " {U}lands badly nearby with a painful crunch!");
                    else core()->message("{U}You hear the loud crunch of something landing badly nearby!");
                }
            }
            if (!mob->tag(MobileTag::ImmunityBleed))
            {
                world->get_room(mob->location())->add_scar(Room::ScarType::BLOOD, MathX::mixup(blood_intensity, FALL_BLEED_INTENSITY_RANGE));
                mob->set_buff(Buff::Type::BLEED, blood_intensity, hp_damage / core()->rng()->rnd(FALL_BLEED_DIVISOR_MIN, FALL_BLEED_DIVISOR_MAX));
            }
            if (mob->hp() <= 0)
            {
                if (is_player)
                {
                    core()->message("{0}{M}Your bones are shattered from the impact, death is mercifully quick.");
                    player->set_death_reason("took a short walk and a long fall");
                }
                else if (player_can_see && room_link == player_loc && !player_resting) core()->message("{U}" + mob_name_the + " is slain instantly from the impact!");
            }
        }
        else if (is_player)
        {
            core()->message("{g}Despite the distance fallen, you manage to land safely on your feet.");
            player->gain_skill_xp("SAFE_FALL", XP_PER_SAFE_FALL_SUCCESS * fallen_dist);
        }
    }

    if (is_player && mob->tag(MobileTag::ArenaFighter) && world->get_room(mob->location())->tag(RoomTag::Arena)) Arena::reward();

    return true;
}
