// actions/abilities.h -- Special abilities which can be used in combat.
// Copyright (c) 2020-2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#ifndef GREAVE_ACTIONS_ABILITIES_H_
#define GREAVE_ACTIONS_ABILITIES_H_

#include <cstddef>


class Abilities
{
public:
    static constexpr float  HEADLONG_STRIKE_ATTACK_SPEED =  20; // The % of an attack's normal speed that it takes to do a Headlong Strike attack.
    static constexpr float  RAPID_STRIKE_ACCURACY_PENALTY = 20; // The % accuracy penalty for a Rapid Strike.
    static constexpr float  RAPID_STRIKE_ATTACK_SPEED =     20; // The % of an attack's normal speed that it takes to do a Rapid Strike attack.
    static constexpr float  SNAP_SHOT_ACCURACY_PENALTY =    20; // The % accuracy penalty for a Snap Shot.
    static constexpr float  SNAP_SHOT_ATTACK_SPEED =        20; // The % of an attack's normal speed that it takes to do a Snap Shot attack.

    static void abilities();                                    // Check cooldowns and availability of abilities.
    static void careful_aim(bool confirm);                      // Attempt to use the Careful Aim ability.
    static void eye_for_an_eye(bool confirm);                   // Attempt to use the Eye for an Eye ability.
    static void grit(bool confirm);                             // Attempt to use the Grit ability.
    static void headlong_strike(size_t target, bool confirm);   // Attempt to use the HeadlongStrike ability.
    static void lady_luck(size_t target, bool confirm);         // Attempt to use the Lady Luck ability.
    static void quick_roll(bool confirm);                       // Attempt to use the Quick Roll ability.
    static void rapid_strike(size_t target);                    // Attempt to use the Rapid Strike ability.
    static void shield_wall(bool confirm);                      // Attempt to use the Shield Wall ability.
    static void snap_shot(size_t target);                       // Attempt to use the Snap Shot ability.

private:
    static constexpr int    CAREFUL_AIM_BONUS_HIT =         25; // The bonus hit% chance from using the Careful Aim ability.
    static constexpr int    CAREFUL_AIM_COOLDOWN =          8;  // The length of the Careful Aim cooldown.
    static constexpr int    CAREFUL_AIM_LENGTH =            2;  // How many buff ticks the Careful Aim ability lasts for.
    static constexpr int    CAREFUL_AIM_MP_COST =           20; // The mana point cost for the Careful Aim ability.
    static constexpr float  CAREFUL_AIM_TIME =              2;  // The time taken by the Careful Aim ability.
    static constexpr int    EYE_FOR_AN_EYE_COOLDOWN =       30; // The cooldown for the Eye For An Eye ability.
    static constexpr int    EYE_FOR_AN_EYE_HP_COST =        30; // The hit points cost for using Eye for an Eye.
    static constexpr int    EYE_FOR_AN_EYE_LENGTH =         10; // The length of time the Eye For An Eye buff remains when activated but unused.
    static constexpr int    EYE_FOR_AN_EYE_MULTI =          5;  // The damage multiplier for the Eye For An Eye ability.
    static constexpr int    GRIT_COOLDOWN =                 5;  // The cooldown for the Grit ability.
    static constexpr int    GRIT_DAMAGE_REDUCTION =         30; // The % of damage reduced by using the Grit ability.
    static constexpr int    GRIT_LENGTH =                   30; // The Grit ability lasts this long, or until the player is hit by an attack.
    static constexpr int    GRIT_SP_COST =                  30; // The stamina point cost for the Grit ability.
    static constexpr float  GRIT_TIME =                     2;  // The time taken by using the Grit ability.
    static constexpr int    HEADLONG_STRIKE_COOLDOWN =      6;  // The cooldown for the Headlong Strike ability.
    static constexpr int    HEADLONG_STRIKE_HP_COST =       10; // The hit points cost to use the Headlong Strike abiliy.
    static constexpr int    LADY_LUCK_COOLDOWN =            20; // The cooldown for the Lady Luck ability.
    static constexpr int    LADY_LUCK_LENGTH =              60; // The buff/debuff time for the Lady Luck ability.
    static constexpr int    LADY_LUCK_MP_COST =             50; // The mana cost for using the Lady Luck ability.
    static constexpr float  LADY_LUCK_TIME =                2;  // The time taken by using the Lady Luck ability.
    static constexpr int    QUICK_ROLL_BONUS_DODGE =        40; // The bonus dodge% chance from using the Quick Roll ability.
    static constexpr int    QUICK_ROLL_COOLDOWN =           8;  // The cooldown for the Quick Roll ability.
    static constexpr int    QUICK_ROLL_LENGTH =             5;  // The length of time the Quick Roll buff remains when activated, but before an enemy attack is made.
    static constexpr int    QUICK_ROLL_SP_COST =            25; // The stamina point cost for the Quick Roll ability.
    static constexpr float  QUICK_ROLL_TIME =               4;  // The time it takes to do a Quick Roll.
    static constexpr int    RAPID_STRIKE_COOLDOWN =         6;  // The cooldown for the Rapid Strike ability.
    static constexpr int    RAPID_STRIKE_SP_COST =          50; // The stamina points cost for the Rapid Strike ability.
    static constexpr int    SHIELD_WALL_BLOCK_BONUS =       70; // The % bonus to blocking an attack with Shield Wall.
    static constexpr int    SHIELD_WALL_COOLDOWN =          6;  // The cooldwon for the Shield Wall ability.
    static constexpr int    SHIELD_WALL_LENGTH =            20; // The length of time the Shield Wall buff remains while activated, but before an enemy attack is made.
    static constexpr int    SHIELD_WALL_SP_COST=            20; // The stamina points cost for the Shield Wall ability.
    static constexpr float  SHIELD_WALL_TIME =              2;  // The time taken to use the Shield Wall ability.
    static constexpr int    SNAP_SHOT_COOLDOWN =            6;  // The cooldown for the Snap Shot ability.
    static constexpr int    SNAP_SHOT_SP_COST =             50; // The stamina points cost for the Snap Shot ability.
};

#endif  // GREAVE_ACTIONS_ABILITIES_H_
