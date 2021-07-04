// world/mobile.cpp -- The Mobile class defines entities that can move and interact with the game world.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#include "3rdparty/SQLiteCpp/SQLiteCpp.h"
#include "core/core.hpp"
#include "core/random.hpp"
#include "core/strx.hpp"
#include "world/inventory.hpp"
#include "world/item.hpp"
#include "world/player.hpp"
#include "world/room.hpp"
#include "world/time-weather.hpp"
#include "world/world.hpp"


const uint32_t Mobile::BASE_CARRY_WEIGHT =      30000;  // The maximum amount of weight a Mobile can carry, before modifiers.

// Flags for the name() function.
const int Mobile::NAME_FLAG_A =                 1;  // Precede the Mobile's name with 'a' or 'an', unless the name is a proper noun.
const int Mobile::NAME_FLAG_CAPITALIZE_FIRST =  2;  // Capitalize the first letter of the Mobile's name (including the "The") if set.
const int Mobile::NAME_FLAG_NO_COLOUR =         4;  // Strip colour codes from the name.
const int Mobile::NAME_FLAG_PLURAL =            8;  // Return a plural of the Mobile's name (e.g. apple -> apples).
const int Mobile::NAME_FLAG_POSSESSIVE =        16; // Change the Mobile's name to a possessive noun (e.g. goblin -> goblin's).
const int Mobile::NAME_FLAG_THE =               32; // Precede the Mobile's name with 'the', unless the name is a proper noun.

// The SQL table construction string for Mobiles.
const std::string   Mobile::SQL_MOBILES =   "CREATE TABLE mobiles ( action_timer REAL, equipment INTEGER UNIQUE, gender INTEGER, hostility TEXT, hp INTEGER NOT NULL, "
    "hp_max INTEGER NOT NULL, id INTEGER UNIQUE NOT NULL, inventory INTEGER UNIQUE, location INTEGER NOT NULL, name TEXT, parser_id INTEGER, spawn_room INTEGER, "
    "species TEXT NOT NULL, sql_id INTEGER PRIMARY KEY UNIQUE NOT NULL, tags TEXT )";


// Constructor, sets default values.
Mobile::Mobile() : m_action_timer(0), m_equipment(std::make_shared<Inventory>()), m_gender(Gender::IT), m_id(0), m_inventory(std::make_shared<Inventory>()), m_location(0),
    m_parser_id(0), m_spawn_room(0)
{
    m_hp[0] = m_hp[1] = 100;
}

// Adds a Mobile (or the player, with ID 0) to this Mobile's hostility list.
void Mobile::add_hostility(uint32_t mob_id)
{
    // Check if this Mobile is already on the hostility vector.
    for (auto h : m_hostility)
        if (h == mob_id) return;

    // If not, add 'em to the list!
    m_hostility.push_back(mob_id);
}

// Returns the number of seconds needed for this Mobile to make an attack.
float Mobile::attack_speed() const
{
    auto main_hand = m_equipment->get(EquipSlot::HAND_MAIN);
    auto off_hand = m_equipment->get(EquipSlot::HAND_OFF);
    const bool main_can_attack = (main_hand && main_hand->type() == ItemType::WEAPON && main_hand->subtype() == ItemSub::MELEE);
    const bool off_can_attack = (off_hand && off_hand->type() == ItemType::WEAPON && off_hand->subtype() == ItemSub::MELEE);

    // Attack speed is the slowest of the equipped weapons.
    float speed = 0.0f;
    if (main_can_attack) speed = main_hand->speed();
    if (off_can_attack && off_hand->speed() > speed) speed = off_hand->speed();
    if (!main_can_attack && !off_can_attack) speed = 1.0f;

    if (!speed)
    {
        speed = 1.0f;
        throw std::runtime_error("Cannot determine attack speed for " + name() + "!");
    }

    return speed;
}

// Returns the modified chance to block for this Mobile, based on equipped gear.
float Mobile::block_mod() const
{
    float mod_perc = 100.0f;
    for (unsigned int i = 0; i < m_equipment->count(); i++)
        mod_perc += m_equipment->get(i)->block_mod();
    return mod_perc / 100.0f;
}

// Checks if the Mobile's action timer is ready.
bool Mobile::can_act() const { return m_action_timer >= 0; }

// Checks how much weight this Mobile is carrying.
uint32_t Mobile::carry_weight() const
{
    uint32_t total_weight = 0;
    for (unsigned int i = 0; i < m_inventory->count(); i++)
        total_weight += m_inventory->get(i)->weight();
    for (unsigned int i = 0; i < m_equipment->count(); i++)
        total_weight += m_equipment->get(i)->weight();
    return total_weight;
}

// Clears a MobileTag from this Mobile.
void Mobile::clear_tag(MobileTag the_tag)
{
    if (!(m_tags.count(the_tag) > 0)) return;
    m_tags.erase(the_tag);
}

// Returns the modified chance to dodge for this Mobile, based on equipped gear.
float Mobile::dodge_mod() const
{
    float mod_perc = 100.0f;
    for (unsigned int i = 0; i < m_equipment->count(); i++)
        mod_perc += m_equipment->get(i)->dodge_mod();
    return mod_perc / 100.0f;
}

// Returns a pointer to the Movile's equipment.
const std::shared_ptr<Inventory> Mobile::equ() const { return m_equipment; }

// Retrieves the anatomy vector for this Mobile.
const std::vector<std::shared_ptr<BodyPart>>& Mobile::get_anatomy() const { return core()->world()->get_anatomy(m_species); }

// Returns a gender string (he/she/it/they/etc.)
std::string Mobile::he_she() const
{
    switch (m_gender)
    {
        case Gender::FEMALE: return "she";
        case Gender::MALE: return "he";
        case Gender::IT: return "it";
        case Gender::THEY: return "they";
        default: return "it";
    }
}

// Returns a gender string (his/her/its/their/etc.)
std::string Mobile::his_her() const
{
    switch (m_gender)
    {
        case Gender::FEMALE: return "her";
        case Gender::MALE: return "his";
        case Gender::IT: return "its";
        case Gender::THEY: return "their";
        default: return "its";
    }
}

// Returns the hostility vector.
const std::vector<uint32_t>& Mobile::hostility_vector() const { return m_hostility; }

// Retrieves the HP (or maximum HP) of this Mobile.
int Mobile::hp(bool max) const { return m_hp[max ? 1 : 0]; }

// Retrieves the unique ID of this Mobile.
uint32_t Mobile::id() const { return m_id; }

// Returns a pointer to the Mobile's Inventory.
const std::shared_ptr<Inventory> Mobile::inv() const { return m_inventory; }

// Checks if this Mobile is dead.
bool Mobile::is_dead() const { return m_hp[0] <= 0; }

// Is this Mobile hostile to the player?
bool Mobile::is_hostile() const
{
    if (tag(MobileTag::AggroOnSight)) return true;
    for (auto h : m_hostility)
        if (h == 0) return true;
    return false;
}

// Returns true if this Mobile is a Player, false if not.
bool Mobile::is_player() const { return false; }

// Loads a Mobile.
uint32_t Mobile::load(std::shared_ptr<SQLite::Database> save_db, uint32_t sql_id)
{
    uint32_t inventory_id = 0, equipment_id = 0;
    SQLite::Statement query(*save_db, "SELECT * FROM mobiles WHERE sql_id = ?");
    query.bind(1, sql_id);
    if (query.executeStep())
    {
        if (!query.isColumnNull("action_timer")) m_action_timer = query.getColumn("action_timer").getDouble();
        if (!query.isColumnNull("equipment")) equipment_id = query.getColumn("equipment").getUInt();
        if (!query.isColumnNull("gender")) m_gender = static_cast<Gender>(query.getColumn("gender").getInt());
        if (!query.isColumnNull("hostility")) m_hostility = StrX::stoi_vec(StrX::string_explode(query.getColumn("hostility").getString(), " "));
        m_hp[0] = query.getColumn("hp").getInt();
        m_hp[1] = query.getColumn("hp_max").getInt();
        m_id = query.getColumn("id").getUInt();
        if (!query.isColumnNull("inventory")) inventory_id = query.getColumn("inventory").getUInt();
        m_location = query.getColumn("location").getUInt();
        if (!query.isColumnNull("name")) m_name = query.getColumn("name").getString();
        if (!query.isColumnNull("parser_id")) m_parser_id = query.getColumn("parser_id").getInt();
        if (!query.isColumnNull("spawn_room")) m_spawn_room = query.getColumn("spawn_room").getUInt();
        m_species = query.getColumn("species").getString();
        if (!query.isColumnNull("tags")) StrX::string_to_tags(query.getColumn("tags").getString(), m_tags);
    }
    else throw std::runtime_error("Could not load mobile data!");

    if (inventory_id) m_inventory->load(save_db, inventory_id);
    if (equipment_id) m_equipment->load(save_db, equipment_id);

    return sql_id;
}

// Retrieves the location of this Mobile, in the form of a Room ID.
uint32_t Mobile::location() const { return m_location; }

// The maximum weight this Mobile can carry.
uint32_t Mobile::max_carry() const { return BASE_CARRY_WEIGHT; }

// Retrieves the name of this Mobile.
std::string Mobile::name(int flags) const
{
    if (!m_name.size()) return "";
    const bool a = ((flags & Mobile::NAME_FLAG_A) == Mobile::NAME_FLAG_A);
    const bool the = ((flags & Mobile::NAME_FLAG_THE) == Mobile::NAME_FLAG_THE);
    const bool capitalize_first = ((flags & Mobile::NAME_FLAG_CAPITALIZE_FIRST) == Mobile::NAME_FLAG_CAPITALIZE_FIRST);
    const bool possessive = ((flags & Mobile::NAME_FLAG_POSSESSIVE) == Mobile::NAME_FLAG_POSSESSIVE);
    const bool plural = ((flags & Mobile::NAME_FLAG_PLURAL) == Mobile::NAME_FLAG_PLURAL);
    const bool no_colour = ((flags & Mobile::NAME_FLAG_NO_COLOUR) == Mobile::NAME_FLAG_NO_COLOUR);

    std::string ret = m_name;
    if (the && !tag(MobileTag::ProperNoun)) ret = "the " + m_name;
    else if (a && !tag(MobileTag::ProperNoun))
    {
        if (StrX::is_vowel(m_name.at(0))) ret = "an " + m_name;
        else ret = "a " + m_name;
    }
    if (capitalize_first && ret[0] >= 'a' && ret[0] <= 'z') ret[0] -= 32;
    if (possessive)
    {
        if (ret.back() == 's') ret += "'";
        else ret += "'s";
    }
    else if (plural && ret.back() != 's' && !tag(MobileTag::PluralName)) ret += "s";
    if (no_colour) ret = StrX::strip_ansi(ret);
    return ret;
}

// Generates a new parser ID for this Item.
void Mobile::new_parser_id() { m_parser_id = core()->rng()->rnd(1, 9999); }

// Returns the modified chance to parry for this Mobile, based on equipped gear.
float Mobile::parry_mod() const
{
    float mod_perc = 100.0f;
    for (unsigned int i = 0; i < m_equipment->count(); i++)
        mod_perc += m_equipment->get(i)->parry_mod();
    return mod_perc / 100.0f;
}

// Retrieves the current ID of this Item, for parser differentiation.
uint16_t Mobile::parser_id() const { return m_parser_id; }

// Causes time to pass for this Mobile.
bool Mobile::pass_time(float seconds)
{
    // For the player, time passes in the world itself.
    if (is_player()) return core()->world()->time_weather()->pass_time(seconds);

    // For NPCs, we'll just take the time from their action timer.
    m_action_timer -= seconds;
    return true;
}

// Reduces this Mobile's hit points.
void Mobile::reduce_hp(int amount)
{
    m_hp[0] -= amount;
    if (m_action_timer < -10.0f) m_action_timer = -10.0f;   // Kludge to allow NPCs to react more quickly when engaged in combat after arriving in a room.
    if (m_hp[0] > 0 || is_player()) return; // The player character's death is handled elsewhere.

    if (m_location == core()->world()->player()->location())
    {
        std::string death_message = "{U}" + name(NAME_FLAG_CAPITALIZE_FIRST | NAME_FLAG_THE);
        if (tag(MobileTag::Unliving)) death_message += " is destroyed!";
        else death_message += " is slain!";
        core()->message(death_message, Show::RESTING, Wake::NEVER);
    }
    if (m_spawn_room) core()->world()->get_room(m_spawn_room)->clear_tag(RoomTag::MobSpawned);
    core()->world()->remove_mobile(m_id);
}

// Restores time for this Mobile's action timer.
void Mobile::restore_action_timer(float amount) { m_action_timer += amount; }

// Restores a specified amount of hit points.
int Mobile::restore_hp(int amount)
{
    int missing = m_hp[1] - m_hp[0];
    if (missing < amount) amount = missing;
    m_hp[0] += missing;
    return missing;
}

// Saves this Mobile.
uint32_t Mobile::save(std::shared_ptr<SQLite::Database> save_db)
{
    const uint32_t inventory_id = m_inventory->save(save_db);
    const uint32_t equipment_id = m_equipment->save(save_db);

    const uint32_t sql_id = core()->sql_unique_id();
    SQLite::Statement query(*save_db, "INSERT INTO mobiles ( action_timer, equipment, gender, hostility, hp, hp_max, id, inventory, location, name, parser_id, spawn_room, "
        "species, sql_id, tags ) VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? )");
    if (m_action_timer) query.bind(1, m_action_timer);
    if (equipment_id) query.bind(2, equipment_id);
    if (m_gender != Gender::IT) query.bind(3, static_cast<int>(m_gender));
    if (m_hostility.size()) query.bind(4, StrX::collapse_vector(m_hostility));
    query.bind(5, m_hp[0]);
    query.bind(6, m_hp[1]);
    query.bind(7, m_id);
    if (inventory_id) query.bind(8, inventory_id);
    query.bind(9, m_location);
    if (m_name.size()) query.bind(10, m_name);
    if (m_parser_id) query.bind(11, m_parser_id);
    if (m_spawn_room) query.bind(12, m_spawn_room);
    query.bind(13, m_species);
    query.bind(14, sql_id);
    const std::string tags = StrX::tags_to_string(m_tags);
    if (tags.size()) query.bind(15, tags);
    query.exec();
    return sql_id;
}

// Sets the current (and, optionally, maximum) HP of this Mobile.
void Mobile::set_hp(int hp, int hp_max)
{
    m_hp[0] = hp;
    if (hp_max) m_hp[1] = hp_max;
}

// Sets this Mobile's unique ID.
void Mobile::set_id(uint32_t new_id) { m_id = new_id; }

// Sets the location of this Mobile with a Room ID.
void Mobile::set_location(uint32_t room_id)
{
    m_location = room_id;
    if (is_player()) core()->world()->recalc_active_rooms();
}

// As above, but with a string Room ID.
void Mobile::set_location(const std::string &room_id)
{
    if (!room_id.size()) set_location(0);
    else set_location(StrX::hash(room_id));
}

// Sets the name of this Mobile.
void Mobile::set_name(const std::string &name) { m_name = name; }

// Sets this Mobile's spawn room.
void Mobile::set_spawn_room(uint32_t id) { m_spawn_room = id; }

// Sets the species of this Mobile.
void Mobile::set_species(const std::string &species) { m_species = species; }

// Sets a MobileTag on this Mobile.
void Mobile::set_tag(MobileTag the_tag)
{
    if (m_tags.count(the_tag) > 0) return;
    m_tags.insert(the_tag);
}

// Checks the species of this Mobile.
std::string Mobile::species() const { return m_species; }

// Checks if a MobileTag is set on this Mobile.
bool Mobile::tag(MobileTag the_tag) const { return (m_tags.count(the_tag) > 0); }
