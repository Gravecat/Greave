// world/mobile.h -- The Mobile class defines entities that can move and interact with the game world.
// Copyright (c) 2020-2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#ifndef GREAVE_WORLD_MOBILE_H_
#define GREAVE_WORLD_MOBILE_H_

#include "world/inventory.h"

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>


enum class Gender : uint8_t { FEMALE, MALE, IT, THEY };

enum class CombatStance : uint8_t { BALANCED, AGGRESSIVE, DEFENSIVE };

enum class MobileTag : uint16_t { None = 0,

    // Tags that affect the Mobile's name.
    PluralName,         // This Mobile's name is a plural (e.g. "pack of rats").
    ProperNoun,         // This Mobile's name is a proper noun (e.g. Smaug).

    // Tags that affect the Mobile's abilities or stats in combat.
    CannotBlock,        // This Mobile is unable to block attacks.
    CannotDodge,        // This Mobile is unable to dodge attacks.
    CannotParry,        // This Mobbile is unable to parry melee attacks.
    Agile,              // Agile Mobiles are harder to hit in melee combat (0.8x hit chance), and more likely to parry attacks (1.5x chance).
    Clumsy,             // The opposite of Agile, Clumsy Mobiles are easier to hit in melee (1.25x hit chance) and suffer a penalty to parrying (0.5x chance).
    Anemic,             // Anemic Mobiles deal less damage with melee attacks (0.5x).
    Feeble,             // Feeble Mobiles deal less damage with melee attacks (0.75x).
    Puny,               // Puny Mobiles deal less damage with melee attacks (0.9x).
    Strong,             // Strong Mobiles deal more damage with melee attacks (1.1x).
    Brawny,             // Brawny Mobiles deal more damage with melee attacks (1.25x).
    Vigorous,           // Vigorous Mobiles deal more damage with melee attacks (1.5x).
    Mighty,             // Mighty Mobiles deal more damage with melee attacks (2x).

    // Tags that determine the Mobile's general state of being.
    Beast,              // This Mobile is a beast or creature; it has body-parts rather than equipment.
    ImmunityBleed,      // This Mobile is unable to bleed.
    ImmunityPoison,     // This Mobile is immune to being poisoned.
    RandomGender,       // This Mobile can be assigned a random gender.
    Unliving,           // This Mobile is a construct or other unliving entity.

    // Tags regarding the Mobile's AI and behaviour.
    AggroOnSight,       // This Mobile will attack the player on sight.
    CannotOpenDoors,    // This Mobile cannot open doors.
    Coward,             // This Mobile will try to run rather than fight.
    Resting,            // This Mobile is currently resting.

    // Temporary tags assigned by the game.
    ArenaFighter,       // This mobile is your opponent in an arena fight. (Or, when set on the player, they are currently engaged in an arena fight.)
    Boxcars,            // The mobile is about to make an automatic critical hit.
    FreeAttack,         // This mobile is allowed to make a free attack, without causing time to pass.
    HeadlongStrike,     // This mobile is making a rapid, dangerous melee attack.
    RapidStrike,        // The mobile is making a rapid, inaccurate melee attack.
    SnakeEyes,          // This mobile is likely to take a critical hit.
    SnapShot,           // This mobiile is making a rapid, inaccurate ranged attack.
    Success_EFAE,       // The mobile successfully performed an Eye for an Eye attack.
    Success_Grit,       // The mobile successfully absorbed damage with the Grit ability.
    Success_QuickRoll,  // The mobile successfully used QuickRoll to give a bonus to dodging an attack.
    Success_ShieldWall, // The mobile successfully used ShieldWall to give a bonus to blocking an attack.
};

struct BodyPart
{
    uint8_t     hit_chance; // The hit chance for this body part.
    std::string name;       // The name of this body part.
    EquipSlot   slot;       // The EquipSlot associated with this body part.
};

struct Buff
{
    enum class Type : uint8_t { NONE, BLEED, CAREFUL_AIM, CD_CAREFUL_AIM, CD_EYE_FOR_AN_EYE, CD_GRIT, CD_HEADLONG_STRIKE, CD_LADY_LUCK, CD_QUICK_ROLL, CD_RAPID_STRIKE, CD_SHIELD_WALL, CD_SNAP_SHOT, EYE_FOR_AN_EYE, GRIT, POISON, QUICK_ROLL, RECENT_DAMAGE, RECENTLY_FLED, SHIELD_WALL };

    static const char SQL_BUFFS[];  // The SQL table construction string for the buffs table.

    static std::shared_ptr<Buff>    load(SQLite::Statement &query); // Loads this Buff from a save file.
    void    save(std::shared_ptr<SQLite::Database> save_db, uint32_t owner_id); // Saves this Buff to a save file.

    uint32_t    power = 0;              // The power level of this buff/debuff.
    uint16_t    time = UINT16_MAX;      // The time remaining on this buff/debuff, or UINT16_MAX for effects that expire on special circumstances.
    Type        type = Type::NONE;      // The type of buff/debuff.
};


class Mobile
{
public:
    // Flags for the name() function.
    static constexpr int    NAME_FLAG_A =                   (1 << 0);   // Precede the mobile's name with 'a' or 'an', unless the name is a proper noun.
    static constexpr int    NAME_FLAG_CAPITALIZE_FIRST =    (1 << 1);   // Capitalize the first letter of the mobile's name (including the "The") if set.
    static constexpr int    NAME_FLAG_HEALTH =              (1 << 2);   // Display the mobile's health in brackets after its name.
    static constexpr int    NAME_FLAG_NO_COLOUR =           (1 << 3);   // Strip colour codes from the name.
    static constexpr int    NAME_FLAG_PLURAL =              (1 << 4);   // Return a plural of the mobile's name (e.g. apple -> apples).
    static constexpr int    NAME_FLAG_POSSESSIVE =          (1 << 5);   // Change the mobile's name to a possessive noun (e.g. goblin -> goblin's).
    static constexpr int    NAME_FLAG_THE =                 (1 << 6);   // Precede the mobile's name with 'the', unless the name is a proper noun.
    static const char       SQL_MOBILES[];                              // The SQL table construction string for the mobiles table.

                        Mobile();                                   // Constructor, sets default values.
    void                add_hostility(uint32_t mob_id);             // Adds a Mobile (or the player, with ID 0) to this Mobile's hostility list.
    void                add_second();                               // Adds a second to this Mobile's action timer.
    void                add_score(int score);                       // Adds to this Mobile's score.
    float               attack_speed() const;                       // Returns the number of seconds needed for this Mobile to make an attack.
    float               block_mod() const;                          // Returns the modified chance to block for this Mobile, based on equipped gear.
    uint32_t            buff_power(Buff::Type type) const;          // Returns the power level of the specified buff/debuff.
    uint16_t            buff_time(Buff::Type type) const;           // Returns the time remaining for the specifieid buff/debuff.
    bool                can_perform_action(float time) const;       // Checks if this Mobile has enough action timer built up to perform an action.
    uint32_t            carry_weight() const;                       // Checks how much weight this Mobile is carrying.
    void                clear_buff(Buff::Type type);                // Clears a specified buff/debuff from the Actor, if it exists.
    void                clear_meta(const std::string &key);         // Clears a metatag from a Mobile. Use with caution!
    void                clear_tag(MobileTag the_tag);               // Clears an MobileTag from this Mobile.
    void                die(bool death_message = true);             // Causes this mobile to die and leave a corpse behind.
    float               dodge_mod() const;                          // Returns the modified chance to dodge for this Mobile, based on equipped gear.
    const std::shared_ptr<Inventory>    equ() const;                // Returns a pointer to the Movile's equipment.
    const std::vector<std::shared_ptr<BodyPart>>& get_anatomy() const;  // Retrieves the anatomy vector for this Mobile.
    bool                has_buff(Buff::Type type) const;            // Checks if this Actor has the specified buff/debuff active.
    std::string         he_she() const;                             // Returns a gender string (he/she/it/they/etc.)
    std::string         his_her() const;                            // Returns a gender string (his/her/its/their/etc.)
    const std::vector<uint32_t>&    hostility_vector() const;       // Returns the hostility vector.
    int                 hp(bool max = false) const;                 // Retrieves the HP (or maximum HP) of this Mobile.
    uint32_t            id() const;                                 // Retrieves the unique ID of this Mobile.
    const std::shared_ptr<Inventory>    inv() const;                // Returns a pointer to the Mobile's Inventory.
    virtual bool        is_dead() const;                            // Checks if this Mobile is dead.
    bool                is_hostile() const;                         // Is this Mobile hostile to the player?
    virtual bool        is_player() const;                          // Returns true if this Mobile is a Player, false if not.
    virtual uint32_t    load(std::shared_ptr<SQLite::Database> save_db, uint32_t sql_id);   // Loads a Mobile.
    uint32_t            location() const;                           // Retrieves the location of this Mobile, in the form of a Room ID.
    virtual uint32_t    max_carry() const;                          // The maximum weight this mobile can carry.
    std::string         meta(const std::string &key) const;         // Retrieves Mobile metadata.
    float               meta_float(const std::string &key) const;   // Retrieves metadata, in float format.
    int                 meta_int(const std::string &key) const;     // Retrieves metadata, in int format.
    uint32_t            meta_uint(const std::string &key) const;    // Retrieves metadata, in unsigned 32-bit integer format.
    std::map<std::string, std::string>* meta_raw();                 // Accesses the metadata map directly. Use with caution!
    std::string         name(int flags = 0) const;                  // Retrieves the name of this Mobile.
    void                new_parser_id();                            // Generates a new parser ID for this Mobile.
    float               parry_mod() const;                          // Returns the modified chance to parry for this Mobile, based on equipped gear.
    uint16_t            parser_id() const;                          // Retrieves the current ID of this Mobile, for parser differentiation.
    bool                pass_time(float seconds = 0.0f, bool interruptable = true); // Causes time to pass for this Mobile.
    virtual void        reduce_hp(int amount, bool death_message = true);   // Reduces this Mobile's hit points.
    int                 restore_hp(int amount);                     // Restores a specified amount of hit points.
    virtual uint32_t    save(std::shared_ptr<SQLite::Database> save_db);    // Saves this Mobile.
                        // Sets a specified buff/debuff on the Actor, or extends an existing buff/debuff.
    uint32_t            score() const;                              // Checks this Mobile's score.
    void                set_buff(Buff::Type type, uint16_t time = UINT16_MAX, uint32_t power = 0, bool additive_power = false, bool additive_time = true);  // Sets a specified buff/debuff on the Actor, or extends an existing buff/debuff.
    void                set_gender(Gender gender);                  // Sets the gender of this Mobile.
    void                set_hp(int hp, int hp_max = 0);             // Sets the current (and, optionally, maximum) HP of this Mobile.
    void                set_id(uint32_t new_id);                    // Sets this Mobile's unique ID.
    void                set_location(uint32_t room_id);             // Sets the location of this Mobile with a Room ID.
    void                set_location(const std::string &room_id);   // As above, but with a string Room ID.
    void                set_meta(const std::string &key, std::string value);    // Adds Item metadata.
    void                set_meta(const std::string &key, int value);            // As above, but with an integer value.
    void                set_meta(const std::string &key, float value);          // As above again, but this time for floats.
    void                set_meta_uint(const std::string &key, uint32_t value);  // As above, but with an unsigned 32-bit integer.
    void                set_name(const std::string &name);          // Sets the name of this Mobile.
    void                set_spawn_room(uint32_t id);                // Sets this Mobile's spawn room.
    void                set_species(const std::string &species);    // Sets the species of this Mobile.
    void                set_stance(CombatStance stance);            // Sets this Mobile's combat stance.
    void                set_tag(MobileTag the_tag);                 // Sets a MobileTag on this Mobile.
    std::string         species() const;                            // Checks the species of this Mobile.
    CombatStance        stance() const;                             // Checks this Mobile's combat stance.
    bool                tag(MobileTag the_tag) const;               // Checks if a MobileTag is set on this Mobile.
    bool                tick_bleed(uint32_t power, uint16_t time);  // Triggers a single bleed tick.
    void                tick_buffs();                               // Reduce the timer on all buffs.
    virtual void        tick_hp_regen();                            // Regenerates HP over time.
    bool                tick_poison(uint32_t power, uint16_t time); // Triggers a single poison tick.
    bool                using_melee() const;                        // Checks if a mobile is using at least one melee weapon.
    bool                using_ranged() const;                       // Checks if a mobile is using at least one ranged weapon.
    bool                using_shield() const;                       // Checks if a mobile is using a shield.

protected:
    static constexpr float  ACTION_TIMER_CAP_MAX =                  3600;   // The maximum value the action timer can ever reach.
    static constexpr int    BASE_CARRY_WEIGHT =                     30000;  // The maximum amount of weight a Mobile can carry, before modifiers.
    static constexpr int    DAMAGE_DEBUFF_TIME =                    60;     // How long the damage debuff that prevents HP regeneration lasts.
    static constexpr int    HP_DEFAULT =                            100;    // The default HP value for mobiles.
    static constexpr int    SCAR_BLEED_INTENSITY_FROM_BLEED_TICK =  1;      // Blood type scar intensity caused by each tick of the player or an NPC bleeding.

    std::shared_ptr<Buff>   buff(Buff::Type type) const;    // Returns a pointer to a specified Buff.

    float                               action_timer_;  // 'Charges up' with time, to allow NPCs to perform timed actions.
    std::vector<std::shared_ptr<Buff>>  buffs_;         // Any and all buffs or debuffs on this Mobile.
    std::shared_ptr<Inventory>          equipment_;     // The Items currently worn or wielded by this Mobile.
    Gender                              gender_;        // The gender of this Mobile.
    std::vector<uint32_t>               hostility_;     // The hostility vector keeps track of who this Mobile is angry with.
    int                                 hp_[2];         // The current and maxmum hit points of this Mobile.
    uint32_t                            id_;            // The Mobile's unique ID.
    std::shared_ptr<Inventory>          inventory_;     // The Items being carried by this Mobile.
    uint32_t                            location_;      // The Room that this Mobile is currently located in.
    std::map<std::string, std::string>  metadata_;      // The Mobile's metadata, if any.
    std::string                         name_;          // The name of this Mobile.
    uint16_t                            parser_id_;     // The semi-unique ID of this Mobile, for parser differentiation.
    uint32_t                            score_;         // Either the score value for killing this Mobile; or, for the Player, their current total score.
    uint32_t                            spawn_room_;    // The Room that spawned this Mobile.
    std::string                         species_;       // Ths species type of this Mobile.
    CombatStance                        stance_;        // The Mobile's current combat stance.
    std::set<MobileTag>                 tags_;          // Any and all tags on this Mobile.
};

#endif  // GREAVE_WORLD_MOBILE_H_
