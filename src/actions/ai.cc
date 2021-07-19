// actions/ai.cc -- NPC AI actions and behaviour.
// Copyright (c) 2020-2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#include "actions/ai.h"
#include "actions/combat.h"
#include "actions/travel.h"
#include "core/core.h"


// Processes AI for a specific active Mobile.
void AI::tick_mob(std::shared_ptr<Mobile> mob, uint32_t)
{
    mob->add_second();  // This is called every second, per active Mobile.
    const auto rng = core()->rng();
    const uint32_t location = mob->location();
    const uint32_t player_location = core()->world()->player()->location();
    const auto room = core()->world()->get_room(location);

    // Scan the Mobile's hostility vector, looking for anyone they're hostile towards.
    std::shared_ptr<Mobile> attack_target = nullptr;
    for (auto h : mob->hostility_vector())
    {
        if (h == 0 && mob->location() == core()->world()->player()->location())
        {
            attack_target = core()->world()->player();
            break;
        }
        else for (size_t m = 0; m < core()->world()->mob_count(); m++)
        {
            const auto check_mob = core()->world()->mob_vec(m);
            if (check_mob->id() == h && check_mob->location() == mob->location())
            {
                attack_target = check_mob;
                break;
            }
        }
    }
    if (attack_target)
    {
        // Fleeing happens regardless of the action timer. This is a concession to allow mobiles to even have a chance of realistically running away. Penalizing their action timer after fleeing leaves all sorts of problems, such as the mobile left standing there defenseless while the player character beats on them. It's not an ideal solution, but this is really the best I can do for now. This will need to be balanced better later.
        if (mob->tag(MobileTag::Coward))
        {
            // Check if we've fled recently.
            if (mob->has_buff(Buff::Type::RECENTLY_FLED))
            {
                if (mob->can_perform_action(FLEE_TIME))
                {
                    if (location == player_location) core()->message("{u}" + mob->name(Mobile::NAME_FLAG_THE) + " {u}cowers in fear!");
                    mob->pass_time();
                }
                return;
            }
            else if (mob->can_perform_action(FLEE_TIME))
            {
                // Attempt a safe travel; if it fails, panic and attempt a more dangerous exit.
                if (location == player_location) core()->message("{U}" + mob->name(Mobile::NAME_FLAG_THE) + " {U}flees in a blind panic!");
                if (!travel_randomly(mob, true))
                {
                    mob->pass_time();
                    if (location == player_location) core()->message("{0}{u}... But " + mob->he_she() + " can't get away!");
                }
                mob->set_buff(Buff::Type::RECENTLY_FLED, FLEE_DEBUFF_TIME);
                return;
            }
            else return;    // If they don't have enough action time available to flee, just wait and charge it up, it won't take long.
        }

        // The rest of the rules here apply to non-cowardly Mobiles.

        // Check if this Mobile wants to change its combat stance.
        if (mob->can_perform_action(Combat::STANCE_CHANGE_TIME))
        {
            CombatStance desired_stance = mob->stance();

            const float hp_percent = (mob->hp(false) / mob->hp(true)) * 100.0f;
            const float target_hp_percent = (attack_target->hp(false) / attack_target->hp(true)) * 100.0f;
            const float hp_ratio = hp_percent / target_hp_percent;
            if (hp_percent <= STANCE_DEFENSIVE_HP_PERCENT) desired_stance = CombatStance::DEFENSIVE;
            else if (target_hp_percent <= STANCE_AGGRESSIVE_HP_PERCENT) desired_stance = CombatStance::AGGRESSIVE;
            else if (hp_ratio <= STANCE_DEFENSIVE_HP_RATIO) desired_stance = CombatStance::DEFENSIVE;
            else if (hp_ratio >= STANCE_AGGRESSIVE_HP_RATIO) desired_stance = CombatStance::AGGRESSIVE;

            // Chance to attempt to counter the target's stance.
            else if (core()->rng()->rnd(STANCE_COUNTER_CHANCE) == 1)
            {
                switch (attack_target->stance())
                {
                    case CombatStance::BALANCED: desired_stance = CombatStance::DEFENSIVE; break;
                    case CombatStance::DEFENSIVE: desired_stance = CombatStance::AGGRESSIVE; break;
                    case CombatStance::AGGRESSIVE: desired_stance = CombatStance::BALANCED; break;
                }
            }

            // Chance to just pick a stance randomly. Sometimes, the unexpected can be useful!
            else if (core()->rng()->rnd(STANCE_RANDOM_CHANCE) == 1) desired_stance = static_cast<CombatStance>(core()->rng()->rnd(0, 2));

            if (desired_stance != mob->stance())
            {
                Combat::change_stance(mob, desired_stance);
                return;
            }
        }

        // For non-cowardly NPCs, we'll wait until there's sufficient action time available to perform an attack. If not, just wait until there is. This will prevent angry NPCs from just doing something else entirely, instead of winding up to attack.
        if (mob->can_perform_action(mob->attack_speed())) Combat::attack(mob, attack_target);
        return;
    }

    if (mob->tag(MobileTag::AggroOnSight) && rng->rnd(AGGRO_CHANCE) == 1 && location == player_location)
    {
        // Unlike the code above -- which handles NPCs that have a specific hatred for a specific mobile (or the player), this is more of a general 'picking a fight' situation. If action time isn't available, we'll allow the option to do something else in the meantime, because this particular mobile isn't hellbent on unleashing limitless unlimited unprecedented eternal terrible violence on anyone *in particular*.
        if (mob->can_perform_action(mob->attack_speed()))
        {
            Combat::attack(mob, core()->world()->player());
            return;
        }
    }

    if (rng->rnd(TRAVEL_CHANCE) == 1 && !mob->has_buff(Buff::Type::RECENTLY_FLED))
    {
        // This is another concession I'm making for mobiles -- all exits will 'cost' the same, while for the player, 'longer' exits cost more. Why? Because this AI code is ticking once an in-game second, it'll end up heavily favouring the shorter exit routs as soon as the action time is available, which will result in much less interesting AI behaviour.
        if (mob->can_perform_action(ActionTravel::TRAVEL_TIME_NORMAL))
        {
            // If the attempt fails (no valid exits, etc.) we'll just continue looking for more actions to perform.
            if (travel_randomly(mob, false)) return;
        }
    }

}

// Ticks all the mobiles in active rooms.
void AI::tick_mobs()
{
    for (size_t m = 0; m < core()->world()->mob_count(); m++)
        tick_mob(core()->world()->mob_vec(m), m);
}

// Sends the Mobile in a random direction.
bool AI::travel_randomly(std::shared_ptr<Mobile> mob, bool allow_dangerous_exits)
{
    const uint32_t location = mob->location();
    const auto room = core()->world()->get_room(location);
    if (room->tag(RoomTag::NoRoam_Temp)) return false;

    std::vector<int> viable_exits;
    for (int i = 0; i < Room::ROOM_LINKS_MAX; i++)
    {
        if (room->fake_link(i)) continue;
        if (!allow_dangerous_exits && room->dangerous_link(i)) continue;
        if (room->link_tag(i, LinkTag::Locked)) continue;
        if (mob->tag(MobileTag::CannotOpenDoors) && room->link_tag(i, LinkTag::Openable) && !room->link_tag(i, LinkTag::Open)) continue;
        if (core()->world()->get_room(room->link(i))->tag(RoomTag::NoRoam_Temp)) continue;
        viable_exits.push_back(i);
    }
    if (viable_exits.size()) return ActionTravel::travel(mob, static_cast<Direction>(viable_exits.at(core()->rng()->rnd(0, viable_exits.size() - 1))), true);
    else return false;
}
