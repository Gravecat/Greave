// world/mobile.cc -- The Mobile class defines entities that can move and interact with the game world.
// Copyright (c) 2020-2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#include "actions/arena.h"
#include "actions/combat.h"
#include "core/core.h"
#include "core/strx.h"
#include "world/mobile.h"


// The SQL table construction string for the buffs table.
constexpr char Buff::SQL_BUFFS[] = "CREATE TABLE buffs ( owner INTEGER, power INTEGER, sql_id INTEGER PRIMARY KEY UNIQUE NOT NULL, time INTEGER, type INTEGER NOT NULL )";

// The SQL table construction string for the mobiles table.
constexpr char Mobile::SQL_MOBILES[] = "CREATE TABLE mobiles ( action_timer REAL, equipment INTEGER UNIQUE, gender INTEGER, hostility TEXT, hp INTEGER NOT NULL, hp_max INTEGER NOT NULL, id INTEGER UNIQUE NOT NULL, inventory INTEGER UNIQUE, location INTEGER NOT NULL, metadata TEXT, name TEXT, parser_id INTEGER, score INTEGER, spawn_room INTEGER, species TEXT NOT NULL, sql_id INTEGER PRIMARY KEY UNIQUE NOT NULL, stance INTEGER, tags TEXT )";


// Loads this Buff from a save file.
std::shared_ptr<Buff> Buff::load(SQLite::Statement &query)
{
    auto new_buff = std::make_shared<Buff>();
    if (!query.isColumnNull("power")) new_buff->power = query.getColumn("power").getUInt();
    if (!query.isColumnNull("time")) new_buff->time = query.getColumn("time").getUInt();
    new_buff->type = static_cast<Buff::Type>(query.getColumn("type").getUInt());
    return new_buff;
}

// Saves this Buff to a save file.
void Buff::save(std::shared_ptr<SQLite::Database> save_db, uint32_t owner_id)
{
    SQLite::Statement query(*save_db, "INSERT INTO BUFFS ( owner, power, sql_id, time, type ) VALUES ( :owner, :power, :sql_id, :time, :type )");
    query.bind(":owner", owner_id);
    if (power) query.bind(":power", power);
    query.bind(":sql_id", core()->sql_unique_id());
    if (time != UINT16_MAX) query.bind(":time", time);
    query.bind(":type", static_cast<int>(type));
    query.exec();
}


// Constructor, sets default values.
Mobile::Mobile() : action_timer_(0), equipment_(std::make_shared<Inventory>(Inventory::PID_PREFIX_EQUIPMENT)), gender_(Gender::IT), id_(0), inventory_(std::make_shared<Inventory>(Inventory::PID_PREFIX_INVENTORY)), location_(0), parser_id_(0), score_(0), spawn_room_(0), stance_(CombatStance::BALANCED)
{
    hp_[0] = hp_[1] = HP_DEFAULT;
}

// Adds a Mobile (or the player, with ID 0) to this Mobile's hostility list.
void Mobile::add_hostility(uint32_t mob_id)
{
    // Check if this Mobile is already on the hostility vector.
    for (auto h : hostility_)
        if (h == mob_id) return;

    // If not, add 'em to the list!
    hostility_.push_back(mob_id);
}

// Adds a second to this Mobile's action timer.
void Mobile::add_second() { if (++action_timer_ > ACTION_TIMER_CAP_MAX) action_timer_ = ACTION_TIMER_CAP_MAX; }

// Adds to this Mobile's score.
void Mobile::add_score(int score) { score_ += score; }

// Returns the number of seconds needed for this Mobile to make an attack.
float Mobile::attack_speed() const
{
    auto main_hand = equipment_->get(EquipSlot::HAND_MAIN);
    auto off_hand = equipment_->get(EquipSlot::HAND_OFF);
    const bool main_can_attack = (main_hand && main_hand->type() == ItemType::WEAPON);
    const bool off_can_attack = (off_hand && off_hand->type() == ItemType::WEAPON);

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

    return speed * Combat::BASE_ATTACK_SPEED_MULTIPLIER;
}

// Returns the modified chance to block for this Mobile, based on equipped gear.
float Mobile::block_mod() const
{
    float mod_perc = 100.0f;
    for (size_t i = 0; i < equipment_->count(); i++)
        mod_perc += equipment_->get(i)->block_mod();
    return mod_perc / 100.0f;
}

// Returns a pointer to a specified Buff.
std::shared_ptr<Buff> Mobile::buff(Buff::Type type) const
{
    for (auto b : buffs_)
        if (b->type == type) return b;
    return nullptr;
}

// Returns the power level of the specified buff/debuff.
uint32_t Mobile::buff_power(Buff::Type type) const
{
    auto b = buff(type);
    if (b) return b->power;
    else return 0;
}

// Returns the time remaining for the specifieid buff/debuff.
uint16_t Mobile::buff_time(Buff::Type type) const
{
    auto b = buff(type);
    if (b) return b->time;
    else return 0;
}

// Checks if this Mobile has enough action timer built up to perform an action.
bool Mobile::can_perform_action(float time) const { return action_timer_ >= time; }

// Checks how much weight this Mobile is carrying.
uint32_t Mobile::carry_weight() const { return inventory_->weight() + equipment_->weight(); }

// Clears a specified buff/debuff from the Actor, if it exists.
void Mobile::clear_buff(Buff::Type type)
{
    for (size_t i = 0; i < buffs_.size(); i++)
    {
        if (buffs_.at(i)->type == type)
        {
            buffs_.erase(buffs_.begin() + i);
            return;
        }
    }
}

// Clears a metatag from an Mobile. Use with caution!
void Mobile::clear_meta(const std::string &key) { metadata_.erase(key); }

// Clears a MobileTag from this Mobile.
void Mobile::clear_tag(MobileTag the_tag)
{
    if (!(tags_.count(the_tag) > 0)) return;
    tags_.erase(the_tag);
}

// Causes this mobile to die and leave a corpse behind.
void Mobile::die(bool death_message)
{
    const auto world = core()->world();
    if (is_player()) return;    // The player's death is handled elsewhere.
    const bool unliving = tag(MobileTag::Unliving);
    if (death_message && location_ == world->player()->location())
    {
        std::string death_message = "{U}" + name(NAME_FLAG_CAPITALIZE_FIRST | NAME_FLAG_THE);
        if (unliving) death_message += " is destroyed!";
        else death_message += " is slain!";
        core()->message(death_message);
    }
    world->player()->add_score(score_);
    if (spawn_room_) world->get_room(spawn_room_)->clear_tag(RoomTag::MobSpawned);
    if (tag(MobileTag::ArenaFighter)) Arena::combatant_died();

    // Add all equipped items to inventory.
    while (equipment_->count())
    {
        auto item = equipment_->get(0);
        if (!item->tag(ItemTag::NoLoot)) inventory_->add_item(item);
        equipment_->remove_item(0);
    }

    // Remove any other NoLoot tagged items.
    for (size_t i = 0; i < inventory_->count(); i++)
    {
        if (inventory_->get(i)->tag(ItemTag::NoLoot))
        {
            inventory_->erase(i);
            i--;
        }
    }

    // Create a corpse, dump all inventory/equipment items in there by reassigning the old inventory pointer.
    auto new_corpse = world->get_item("CORPSE");
    new_corpse->set_name("{r}" + name(NAME_FLAG_POSSESSIVE | NAME_FLAG_NO_COLOUR) + (unliving ? " {R}remains" : " {R}corpse"));
    if (inventory_->count()) new_corpse->assign_inventory(inventory_);
    world->get_room(location_)->inv()->add_item(new_corpse);

    // Now we can dispose of the old mobile.
    world->remove_mobile(id_);
}

// Returns the modified chance to dodge for this Mobile, based on equipped gear.
float Mobile::dodge_mod() const
{
    float mod_perc = 100.0f;
    for (size_t i = 0; i < equipment_->count(); i++)
        mod_perc += equipment_->get(i)->dodge_mod();
    return mod_perc / 100.0f;
}

// Returns a pointer to the Movile's equipment.
const std::shared_ptr<Inventory> Mobile::equ() const { return equipment_; }

// Retrieves the anatomy vector for this Mobile.
const std::vector<std::shared_ptr<BodyPart>>& Mobile::get_anatomy() const { return core()->world()->get_anatomy(species_); }

// Checks if this Actor has the specified buff/debuff active.
bool Mobile::has_buff(Buff::Type type) const
{
    for (auto b : buffs_)
        if (b->type == type) return true;
    return false;
}

// Returns a gender string (he/she/it/they/etc.)
std::string Mobile::he_she() const
{
    switch (gender_)
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
    switch (gender_)
    {
        case Gender::FEMALE: return "her";
        case Gender::MALE: return "his";
        case Gender::IT: return "its";
        case Gender::THEY: return "their";
        default: return "its";
    }
}

// Returns the hostility vector.
const std::vector<uint32_t>& Mobile::hostility_vector() const { return hostility_; }

// Retrieves the HP (or maximum HP) of this Mobile.
int Mobile::hp(bool max) const { return hp_[max ? 1 : 0]; }

// Retrieves the unique ID of this Mobile.
uint32_t Mobile::id() const { return id_; }

// Returns a pointer to the Mobile's Inventory.
const std::shared_ptr<Inventory> Mobile::inv() const { return inventory_; }

// Checks if this Mobile is dead.
bool Mobile::is_dead() const { return hp_[0] <= 0; }

// Is this Mobile hostile to the player?
bool Mobile::is_hostile() const
{
    if (tag(MobileTag::AggroOnSight)) return true;
    for (auto h : hostility_)
        if (h == 0) return true;
    return false;
}

// Returns true if this Mobile is a Player, false if not.
bool Mobile::is_player() const { return false; }

// Loads a Mobile.
uint32_t Mobile::load(std::shared_ptr<SQLite::Database> save_db, uint32_t sql_id)
{
    uint32_t inventory_id = 0, equipment_id = 0;
    SQLite::Statement query(*save_db, "SELECT * FROM mobiles WHERE sql_id = :sql_id");
    query.bind(":sql_id", sql_id);
    if (query.executeStep())
    {
        if (!query.isColumnNull("action_timer")) action_timer_ = query.getColumn("action_timer").getDouble();
        if (!query.isColumnNull("equipment")) equipment_id = query.getColumn("equipment").getUInt();
        if (!query.isColumnNull("gender")) gender_ = static_cast<Gender>(query.getColumn("gender").getInt());
        if (!query.isColumnNull("hostility")) hostility_ = StrX::stoi_vec(StrX::string_explode(query.getColumn("hostility").getString(), " "));
        hp_[0] = query.getColumn("hp").getInt();
        hp_[1] = query.getColumn("hp_max").getInt();
        id_ = query.getColumn("id").getUInt();
        if (!query.isColumnNull("inventory")) inventory_id = query.getColumn("inventory").getUInt();
        location_ = query.getColumn("location").getUInt();
        if (!query.getColumn("metadata").isNull()) StrX::string_to_metadata(query.getColumn("metadata").getString(), metadata_);
        if (!query.isColumnNull("name")) name_ = query.getColumn("name").getString();
        if (!query.isColumnNull("parser_id")) parser_id_ = query.getColumn("parser_id").getInt();
        if (!query.isColumnNull("score")) score_ = query.getColumn("score").getUInt();
        if (!query.isColumnNull("spawn_room")) spawn_room_ = query.getColumn("spawn_room").getUInt();
        species_ = query.getColumn("species").getString();
        if (!query.isColumnNull("stance")) stance_ = static_cast<CombatStance>(query.getColumn("stance").getInt());
        if (!query.isColumnNull("tags")) StrX::string_to_tags(query.getColumn("tags").getString(), tags_);
    }
    else throw std::runtime_error("Could not load mobile data!");

    if (inventory_id) inventory_->load(save_db, inventory_id);
    if (equipment_id) equipment_->load(save_db, equipment_id);

    // Load any and all buffs/debuffs.
    SQLite::Statement buff_query(*save_db, "SELECT * FROM buffs WHERE owner = :sql_id");
    buff_query.bind(":sql_id", sql_id);
    while (buff_query.executeStep())
        buffs_.push_back(Buff::load(buff_query));

    return sql_id;
}

// Retrieves the location of this Mobile, in the form of a Room ID.
uint32_t Mobile::location() const { return location_; }

// The maximum weight this Mobile can carry.
uint32_t Mobile::max_carry() const { return BASE_CARRY_WEIGHT; }

// Retrieves Mobile metadata.
std::string Mobile::meta(const std::string &key) const
{
    if (metadata_.find(key) == metadata_.end()) return "";
    std::string result = metadata_.at(key);
    StrX::find_and_replace(result, "_", " ");
    return result;
}

// Retrieves metadata, in float format.
float Mobile::meta_float(const std::string &key) const
{
    const std::string key_str = meta(key);
    if (!key_str.size()) return 0.0f;
    else return std::stof(key_str);
}

// Retrieves metadata, in int format.
int Mobile::meta_int(const std::string &key) const
{
    const std::string key_str = meta(key);
    if (!key_str.size()) return 0;
    else return std::stoi(key_str);
}

// Retrieves metadata, in unsigned 32-bit integer format.
uint32_t Mobile::meta_uint(const std::string &key) const
{
    const std::string key_str = meta(key);
    if (!key_str.size()) return 0;
    else return std::stoul(key_str);
}

// Accesses the metadata map directly. Use with caution!
std::map<std::string, std::string>* Mobile::meta_raw() { return &metadata_; }

// Retrieves the name of this Mobile.
std::string Mobile::name(int flags) const
{
    if (!name_.size()) return "";
    const bool a = ((flags & Mobile::NAME_FLAG_A) == Mobile::NAME_FLAG_A);
    const bool the = ((flags & Mobile::NAME_FLAG_THE) == Mobile::NAME_FLAG_THE);
    const bool capitalize_first = ((flags & Mobile::NAME_FLAG_CAPITALIZE_FIRST) == Mobile::NAME_FLAG_CAPITALIZE_FIRST);
    const bool health = ((flags & Mobile::NAME_FLAG_HEALTH) == Mobile::NAME_FLAG_HEALTH);
    const bool possessive = ((flags & Mobile::NAME_FLAG_POSSESSIVE) == Mobile::NAME_FLAG_POSSESSIVE);
    const bool plural = ((flags & Mobile::NAME_FLAG_PLURAL) == Mobile::NAME_FLAG_PLURAL);
    const bool no_colour = ((flags & Mobile::NAME_FLAG_NO_COLOUR) == Mobile::NAME_FLAG_NO_COLOUR);

    std::string ret = name_;
    if (the && !tag(MobileTag::ProperNoun)) ret = "the " + name_;
    else if (a && !tag(MobileTag::ProperNoun))
    {
        if (StrX::is_vowel(name_.at(0))) ret = "an " + name_;
        else ret = "a " + name_;
    }
    if (capitalize_first && ret[0] >= 'a' && ret[0] <= 'z') ret[0] -= 32;
    if (possessive)
    {
        if (ret.back() == 's') ret += "'";
        else ret += "'s";
    }
    else if (plural && ret.back() != 's' && !tag(MobileTag::PluralName)) ret += "s";

    if (health)
    {
        std::string health_str, health_str_unliving;
        std::vector<std::string> health_vec;
        const float hp_perc = static_cast<float>(hp()) / static_cast<float>(hp(true));
        if (hp_perc <= 0.1f)
        {
            health_str = "{R}close to death{w}";
            health_str_unliving = "{R}close to collapse{w}";
        }
        else if (hp_perc <= 0.2f)
        {
            health_str = "{R}badly injured{w}";
            health_str_unliving = "{R}badly damaged{w}";
        }
        else if (hp_perc <= 0.5f)
        {
            health_str = "{Y}injured{w}";
            health_str_unliving = "{Y}damaged{w}";
        }
        else if (hp_perc <= 0.75f)
        {
            health_str = "{Y}bruised{w}";
            health_str_unliving = "{Y}scratched{w}";
        }
        else if (hp_perc < 1 && tag(MobileTag::Coward)) health_str = health_str_unliving = "{Y}shaken{w}";
        if (health_str.size())
        {
            if (tag(MobileTag::Unliving)) health_vec.push_back(health_str_unliving);
            else health_vec.push_back(health_str);
        }
        if (has_buff(Buff::Type::BLEED)) health_vec.push_back("{R}bleeding{w}");
        if (has_buff(Buff::Type::POISON)) health_vec.push_back("{G}poisoned{w}");
        if (health_vec.size()) ret += " (" + StrX::comma_list(health_vec, StrX::CL_OXFORD_COMMA) + ")";
    }

    if (no_colour) ret = StrX::strip_ansi(ret);
    return ret;
}

// Generates a new parser ID for this Item.
void Mobile::new_parser_id() { parser_id_ = core()->rng()->rnd(0, 999) + (1000 * Inventory::PID_PREFIX_MOBILE); }

// Returns the modified chance to parry for this Mobile, based on equipped gear.
float Mobile::parry_mod() const
{
    float mod_perc = 100.0f;
    for (size_t i = 0; i < equipment_->count(); i++)
        mod_perc += equipment_->get(i)->parry_mod();
    return mod_perc / 100.0f;
}

// Retrieves the current ID of this Item, for parser differentiation.
uint16_t Mobile::parser_id() const { return parser_id_; }

// Causes time to pass for this Mobile.
bool Mobile::pass_time(float seconds, bool interruptable)
{
    // For the player, time passes in the world itself.
    if (is_player())
    {
        if (!seconds) core()->guru()->nonfatal("Attempt to pass 0 seconds on player character.", Guru::GURU_WARN);
        return core()->world()->time_weather()->pass_time(seconds, interruptable);
    }

    // For NPCs, any action clears their action timer.
    action_timer_ = 0;
    return true;
}

// Reduces this Mobile's hit points.
void Mobile::reduce_hp(int amount, bool death_message)
{
    hp_[0] -= amount;
    set_buff(Buff::Type::RECENT_DAMAGE, DAMAGE_DEBUFF_TIME, 0, false, false);
    if (is_player()) return;                // The player character's death is handled elsewhere.
    clear_buff(Buff::Type::RECENTLY_FLED);  // Cowardly NPCs fleeing in fear should be able to flee again when taking damage.
    if (hp_[0] > 0) return;                // Everything below this point deals with the Mobile dying.
    die(death_message);
}

// Restores a specified amount of hit points.
int Mobile::restore_hp(int amount)
{
    int missing = hp_[1] - hp_[0];
    if (missing < amount) amount = missing;
    hp_[0] += missing;
    return missing;
}

// Saves this Mobile.
uint32_t Mobile::save(std::shared_ptr<SQLite::Database> save_db)
{
    const uint32_t inventory_id = inventory_->save(save_db);
    const uint32_t equipment_id = equipment_->save(save_db);

    const uint32_t sql_id = core()->sql_unique_id();
    SQLite::Statement query(*save_db, "INSERT INTO mobiles ( action_timer, equipment, gender, hostility, hp, hp_max, id, inventory, location, metadata, name, parser_id, score, spawn_room, species, sql_id, stance, tags ) VALUES ( :action_timer, :equipment, :gender, :hostility, :hp, :hp_max, :id, :inventory, :location, :metadata, :name, :parser_id, :score, :spawn_room, :species, :sql_id, :stance, :tags )");
    if (action_timer_) query.bind(":action_timer", action_timer_);
    if (equipment_id) query.bind(":equipment", equipment_id);
    if (gender_ != Gender::IT) query.bind(":gender", static_cast<int>(gender_));
    if (hostility_.size()) query.bind(":hostility", StrX::collapse_vector(hostility_));
    query.bind(":hp", hp_[0]);
    query.bind(":hp_max", hp_[1]);
    query.bind(":id", id_);
    if (inventory_id) query.bind(":inventory", inventory_id);
    query.bind(":location", location_);
    if (metadata_.size()) query.bind(":metadata", StrX::metadata_to_string(metadata_));
    if (name_.size()) query.bind(":name", name_);
    if (parser_id_) query.bind(":parser_id", parser_id_);
    if (score_) query.bind(":score", score_);
    if (spawn_room_) query.bind(":spawn_room", spawn_room_);
    query.bind(":species", species_);
    query.bind(":sql_id", sql_id);
    if (stance_ != CombatStance::BALANCED) query.bind(":stance", static_cast<int>(stance_));
    const std::string tags = StrX::tags_to_string(tags_);
    if (tags.size()) query.bind(":tags", tags);
    query.exec();

    // Save any and all buffs/debuffs.
    for (auto b : buffs_)
        b->save(save_db, sql_id);

    return sql_id;
}

// Checks this Mobile's score.
uint32_t Mobile::score() const { return score_; }

// Sets a specified buff/debuff on the Actor, or extends an existing buff/debuff.
void Mobile::set_buff(Buff::Type type, uint16_t time, uint32_t power, bool additive_power, bool additive_time)
{
    for (auto b : buffs_)
    {
        if (b->type == type)
        {
            if (time != UINT16_MAX)
            {
                if (additive_time) b->time += time;
                else if (b->time < time) b->time = time;
            }
            if (additive_power) b->power += power;
            else if (b->power < power) b->power = power;
            return;
        }
    }
    auto new_buff = std::make_shared<Buff>();
    new_buff->type = type;
    new_buff->time = time;
    new_buff->power = power;
    buffs_.push_back(new_buff);
}

// Sets the gender of this Mobile.
void Mobile::set_gender(Gender gender) { gender_ = gender; }

// Sets the current (and, optionally, maximum) HP of this Mobile.
void Mobile::set_hp(int hp, int hp_max)
{
    hp_[0] = hp;
    if (hp_max) hp_[1] = hp_max;
}

// Sets this Mobile's unique ID.
void Mobile::set_id(uint32_t new_id) { id_ = new_id; }

// Sets the location of this Mobile with a Room ID.
void Mobile::set_location(uint32_t rooid_)
{
    location_ = rooid_;
    if (is_player()) core()->world()->recalc_active_rooms();
}

// As above, but with a string Room ID.
void Mobile::set_location(const std::string &rooid_)
{
    if (!rooid_.size()) set_location(0);
    else set_location(StrX::hash(rooid_));
}

// Adds Mobile metadata.
void Mobile::set_meta(const std::string &key, std::string value)
{
    StrX::find_and_replace(value, " ", "_");
    if (metadata_.find(key) == metadata_.end()) metadata_.insert(std::pair<std::string, std::string>(key, value));
    else metadata_.at(key) = value;
}

// As above, but with an integer value.
void Mobile::set_meta(const std::string &key, int value) { set_meta(key, std::to_string(value)); }

// As above again, but this time for floats.
void Mobile::set_meta(const std::string &key, float value) { set_meta(key, StrX::ftos(value, 1)); }

// As above, but with an unsigned 32-bit integer.
void Mobile::set_meta_uint(const std::string &key, uint32_t value) { set_meta(key, std::to_string(value)); }

// Sets the name of this Mobile.
void Mobile::set_name(const std::string &name) { name_ = name; }

// Sets this Mobile's spawn room.
void Mobile::set_spawn_room(uint32_t id) { spawn_room_ = id; }

// Sets the species of this Mobile.
void Mobile::set_species(const std::string &species) { species_ = species; }

// Sets this Mobile's combat stance.
void Mobile::set_stance(CombatStance stance) { stance_ = stance; }

// Sets a MobileTag on this Mobile.
void Mobile::set_tag(MobileTag the_tag)
{
    if (tags_.count(the_tag) > 0) return;
    tags_.insert(the_tag);
}

// Checks the species of this Mobile.
std::string Mobile::species() const { return species_; }

// Checks this Mobile's combat stance.
CombatStance Mobile::stance() const { return stance_; }

// Checks if a MobileTag is set on this Mobile.
bool Mobile::tag(MobileTag the_tag) const { return (tags_.count(the_tag) > 0); }

// Triggers a single bleed tick.
bool Mobile::tick_bleed(uint32_t power, uint16_t time)
{
    if (!power || tag(MobileTag::ImmunityBleed)) return true;
    const auto room = core()->world()->get_room(location_);
    const bool fatal = (static_cast<int>(power) >= hp_[0]);

    room->add_scar(Room::ScarType::BLOOD, SCAR_BLEED_INTENSITY_FROM_BLEED_TICK);
    if (is_player())
    {
        core()->message("{r}You are {R}bleeding {r}rather badly. {w}[{R}-" + std::to_string(power) + "{w}]");
        if (fatal)
        {
            core()->message("{0}{R}You've lost too much blood and collapse, bleeding out on the ground.");
            core()->world()->player()->set_death_reason("died from excessive blood loss");
        }
    }
    else
    {
        const std::shared_ptr<Player> player = core()->world()->player();
        if (player->location() == location_ && room->light() >= Room::LIGHT_VISIBLE) core()->message("{r}" + name(NAME_FLAG_CAPITALIZE_FIRST | NAME_FLAG_THE) + " {r}is {R}bleeding {r}rather badly. {w}[{R}-" + std::to_string(power) + "{w}]");
    }
    reduce_hp(power);
    if (!fatal && is_player() && time == 1) core()->message("{r}Your wounds stop bleeding.");
    return !fatal;
}

// Reduce the timer on all buffs.
void Mobile::tick_buffs()
{
    for (size_t i = 0; i < buffs_.size(); i++)
    {
        if (buffs_.at(i)->time == UINT16_MAX) continue;
        const auto type = buffs_.at(i)->type;

        switch (type)
        {
            case Buff::Type::BLEED:
                if (!tick_bleed(buffs_.at(i)->power, buffs_.at(i)->time)) return;
                break;
            case Buff::Type::POISON:
                if (!tick_poison(buffs_.at(i)->power, buffs_.at(i)->time)) return;
                break;
            default: break;
        }

        if (buffs_.at(i)->time == 1)
        {
            switch (type)
            {
                case Buff::Type::CD_CAREFUL_AIM: core()->message("{m}The {M}CarefulAim {m}ability is ready to use again."); break;
                case Buff::Type::CD_EYE_FOR_AN_EYE: core()->message("{m}The {M}EyeForAnEye {m}ability is ready to use again."); break;
                case Buff::Type::CD_GRIT: core()->message("{m}The {M}Grit {m}ability is ready to use again."); break;
                case Buff::Type::CD_HEADLONG_STRIKE: core()->message("{m}The {M}HeadlongStrike {m}ability is ready to use again."); break;
                case Buff::Type::CD_LADY_LUCK: core()->message("{m}The {M}LadyLuck {m}ability is ready to use again."); break;
                case Buff::Type::CD_QUICK_ROLL: core()->message("{m}The {M}QuickRoll {m}ability is ready to use again."); break;
                case Buff::Type::CD_RAPID_STRIKE: core()->message("{m}The {M}RapidStrike {m}ability is ready to use again."); break;
                case Buff::Type::CD_SHIELD_WALL: core()->message("{m}The {M}ShieldWall {m}ability is ready to use again."); break;
                case Buff::Type::CD_SNAP_SHOT: core()->message("{m}The {M}SnapShot {m}ability is ready to use again."); break;
                default: break;
            }
        }

        if (!--buffs_.at(i)->time)
            buffs_.erase(buffs_.begin() + i--);
    }
}

// Regenerates HP over time.
void Mobile::tick_hp_regen()
{
    if (has_buff(Buff::Type::RECENT_DAMAGE)) return;
    if (hp_[0] > 0 && hp_[0] < hp_[1])
        hp_[0]++;
}

// Triggers a single poison tick.
bool Mobile::tick_poison(uint32_t power, uint16_t time)
{
    if (!power || tag(MobileTag::ImmunityPoison)) return true;
    const auto room = core()->world()->get_room(location_);
    const bool fatal = (static_cast<int>(power) >= hp_[0]);

    if (is_player())
    {
        core()->message("{g}You feel deathly ill from the {G}poison {g}in your veins. {w}[{G}-" + std::to_string(power) + "{w}]");
        if (fatal)
        {
            core()->message("{0}{G}The poison running through your veins is too much, and your body shuts down.");
            core()->world()->player()->set_death_reason("succumbed to poison");
        }
    }
    else
    {
        const std::shared_ptr<Player> player = core()->world()->player();
        if (player->location() == location_ && room->light() >= Room::LIGHT_VISIBLE) core()->message("{g}" + name(NAME_FLAG_CAPITALIZE_FIRST | NAME_FLAG_THE) + " {g}takes damage from {G}poison{g}. {w}[{G}-" + std::to_string(power) + "{w}]");
    }
    reduce_hp(power);
    if (!fatal && is_player() && time == 1) core()->message("{g}You feel much better as the poison fades from your system.");
    return !fatal;
}

// Checks if a mobile is using at least one melee weapon.
bool Mobile::using_melee() const
{
    const auto main_hand = equ()->get(EquipSlot::HAND_MAIN);
    const auto off_hand = equ()->get(EquipSlot::HAND_OFF);
    if (main_hand && main_hand->type() == ItemType::WEAPON && main_hand->subtype() == ItemSub::MELEE) return true;
    if (main_hand && main_hand->tag(ItemTag::TwoHanded)) return false;
    if (off_hand && off_hand->type() == ItemType::WEAPON && off_hand->subtype() == ItemSub::MELEE) return true;
    return false;
}

// Checks if a mobile is using at least one ranged weapon.
bool Mobile::using_ranged() const
{
    const auto main_hand = equ()->get(EquipSlot::HAND_MAIN);
    const auto off_hand = equ()->get(EquipSlot::HAND_OFF);
    if (main_hand && main_hand->type() == ItemType::WEAPON && main_hand->subtype() == ItemSub::RANGED) return true;
    if (main_hand && main_hand->tag(ItemTag::TwoHanded)) return false;
    if (off_hand && off_hand->type() == ItemType::WEAPON && off_hand->subtype() == ItemSub::RANGED) return true;
    return false;
}

// Checks if a mobile is using a shield.
bool Mobile::using_shield() const
{
    const auto off_hand = equ()->get(EquipSlot::HAND_OFF);
    if (off_hand && off_hand->type() == ItemType::SHIELD) return true;
    return false;
}
