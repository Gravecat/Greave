// world/player.cc -- The Player class is derived from Mobile, and defines the player character in the game world.
// Copyright (c) 2021 Raine "Gravecat" Simmons and the Greave contributors. Licensed under the GNU Affero General Public License v3 or any later version.

#include <cmath>

#include "actions/eat-drink.h"
#include "core/core.h"
#include "world/player.h"


// The SQL table construction string for the player data.
constexpr char Player::SQL_PLAYER[] = "CREATE TABLE player ( blood_tox INTEGER, hunger INTEGER NOT NULL, mob_target INTEGER, money INTEGER NOT NULL, mp INTEGER NOT NULL, mp_max INTEGER NOT NULL, sp INTEGER NOT NULL, sp_delay INTEGER, sp_max INTEGER NOT NULL, sql_id INTEGER PRIMARY KEY UNIQUE NOT NULL, thirst INTEGET NOT NULL )";

// The SQL table construction string for the player skills data.
constexpr char Player::SQL_SKILLS[] = "CREATE TABLE skills ( id TEXT PRIMARY KEY UNIQUE NOT NULL, level INTEGER NOT NULL, xp REAL )";


// Constructor, sets default values.
Player::Player() : blood_tox_(0), death_reason_("the will of the gods"), hunger_(HUNGER_MAX), mob_target_(0), money_(0), thirst_(THIRST_MAX)
{
    set_species("humanoid");
    set_name("Player");
    mp_[0] = mp_[1] = MP_DEFAULT;
    sp_[0] = sp_[1] = SP_DEFAULT;
    sp_[2] = 0;
}

// Eats food, increasing the hunger counter.
void Player::add_food(int power) { hunger_ += power; }

// Adds money to the player's wallet.
void Player::add_money(uint32_t amount)
{
    // Avoid integer overflow.
    if (money_ + amount < money_)
    {
        core()->guru()->nonfatal("Intercepted money integer overflow!", Guru::GURU_WARN);
        money_ = UINT32_MAX;
    }
    else money_ += amount;
}

// Drinks some water, increasing the thirst counter.
void Player::add_water(int power)
{
    thirst_ += power;
    if (thirst_ > THIRST_MAX) thirst_ = THIRST_MAX;
}

// Retrieves the player's blood toxicity level.
int Player::blood_tox() const { return blood_tox_; }

// Gets the clothing warmth level from the Player.
int Player::clothes_warmth() const
{
    int warmth = 0;
    for (size_t i = 0; i < equipment_->count(); i++)
        warmth += equipment_->get(i)->warmth();
    return warmth;
}

// Retrieves the player's death reason.
std::string Player::death_reason() const { return death_reason_; }

// Gains experience in a skill.
void Player::gain_skill_xp(const std::string& skill_id, float xp)
{
    if (is_dead()) return;
    xp *= core()->world()->get_skill_multiplier(skill_id);
    if (xp <= 0)
    {
        if (xp < 0) core()->guru()->nonfatal("Attempt to give negative XP in " + skill_id, Guru::GURU_WARN);
        return;
    }
    auto it = skill_xp_.find(skill_id);
    if (it == skill_xp_.end())
    {
        skill_xp_.insert(std::pair<std::string, float>(skill_id, xp));
        it = skill_xp_.find(skill_id); // We'll need to refer to this in the level-up code below.
    }
    else it->second += xp;

    int current_level = skill_level(skill_id);
    if (!current_level) skill_levels_.insert(std::pair<std::string, int>(skill_id, 0));
    bool level_increased = false;
    while (it->second > 0)
    {
        const float xp_to_next_level = BASE_SKILL_COST_MULTIPLIER * std::pow(current_level + BASE_SKILL_COST_LEVEL_OFFSET, 2);
        if (it->second >= xp_to_next_level)
        {
            it->second -= xp_to_next_level;
            current_level++;
            level_increased = true;
            skill_levels_.find(skill_id)->second = current_level;
        }
        else break;
    }
    if (level_increased)
    {
        if (skill_id == "TOUGHNESS")
        {
            recalc_max_hp();
            core()->message("{G}You feel more resilient! Your {C}toughness {G}has increased to {C}" + std::to_string(current_level) + "{G}!");
        }
        else core()->message("{G}Your skill in {C}" + core()->world()->get_skill_name(skill_id) + " {G}has increased to {C}" + std::to_string(current_level) + "{G}!");
    }
}

// Checks the current hunger level.
int Player::hunger() const { return hunger_; }

// The player gets a little more hungry.
void Player::hunger_tick()
{
    switch (--hunger_)
    {
        case 0:
            core()->message("{y}You collapse from {Y}starvation{y}, too weak to keep going.");
            set_death_reason("starved to death");
            break;
        case 1: case 2: core()->message("{Y}You are starving to death!"); break;
        case 3: case 4: core()->message("{Y}You almost collapse from the hunger pain!"); break;
        case 5: case 6: core()->message("{Y}You are desperately hungry!"); break;
        case 7: case 8: core()->message("{Y}You are ravenously hungry!"); break;
        case 9: case 10: core()->message("{y}Your stomach rumbles loudly!"); break;
        case 11: case 12: core()->message("{y}Your stomach rumbles quietly."); break;
        case 14: core()->message("{y}You're starting to feel peckish."); break;
    }
}

// Increases the player's blood toxicity.
void Player::increase_tox(int power)
{
    const auto rng = core()->rng();
    int old_tox = blood_tox_;
    blood_tox_ += power;
    if (blood_tox_ >= BLOOD_TOX_POISON_LEVEL && rng->rnd(BLOOD_TOX_POISON_CHANCE) == 1)
    {
        set_buff(Buff::Type::POISON, rng->rnd(BLOOD_TOX_POISON_TIME_RNG) + BLOOD_TOX_POISON_TIME_BASE, rng->rnd(BLOOD_TOX_POISON_POWER_RNG) + BLOOD_TOX_POISON_POWER_BASE, true, true);
        core()->message("{G}You feel deathly ill, your stomach churning violently!");
        return;
    }
    if (blood_tox_ >= BLOOD_TOX_VOMIT_LEVEL && rng->rnd(BLOOD_TOX_VOMIT_CHANCE) == 1)
    {
        ActionEatDrink::vomit(true);
        return;
    }
    if (old_tox < BLOOD_TOX_WARNING && blood_tox_ >= BLOOD_TOX_WARNING) core()->message("{g}Your stomach churns and you feel horrible.");
}

// Checks if this Player is dead.
bool Player::is_dead() const
{
    if (hunger_ < 1 || thirst_ < 1) return true;
    return Mobile::is_dead();
}

// Returns true if this Mobile is a Player, false if not.
bool Player::is_player() const { return true; }

// Loads the Player data.
uint32_t Player::load(std::shared_ptr<SQLite::Database> save_db, uint32_t sql_id)
{
    SQLite::Statement query(*save_db, "SELECT * FROM player");
    if (query.executeStep())
    {
        if (!query.isColumnNull("blood_tox")) blood_tox_ = query.getColumn("blood_tox").getInt();
        hunger_ = query.getColumn("hunger").getInt();
        if (!query.isColumnNull("mob_target")) mob_target_ = query.getColumn("mob_target").getUInt();
        money_ = query.getColumn("money").getUInt();
        mp_[0] = query.getColumn("mp").getInt();
        mp_[1] = query.getColumn("mp_max").getInt();
        sp_[0] = query.getColumn("sp").getInt();
        if (!query.isColumnNull("sp_delay")) sp_[2] = query.getColumn("sp_delay").getInt();
        sp_[1] = query.getColumn("sp_max").getInt();
        sql_id = query.getColumn("sql_id").getUInt();
        thirst_ = query.getColumn("thirst").getInt();
    }
    else throw std::runtime_error("Could not load player data!");

    SQLite::Statement skill_query(*save_db, "SELECT * FROM skills");
    while (skill_query.executeStep())
    {
        const std::string skill_id = skill_query.getColumn("id").getString();
        skill_levels_.insert(std::make_pair(skill_id, skill_query.getColumn("level").getInt()));
        if (!skill_query.isColumnNull("xp")) skill_xp_.insert(std::make_pair(skill_id, skill_query.getColumn("xp").getDouble()));
    }

    return Mobile::load(save_db, sql_id);
}

// The maximum weight the player can carry.
uint32_t Player::max_carry() const
{
    const uint32_t base_carry = Mobile::max_carry();
    return base_carry + std::round(base_carry * (static_cast<float>(skill_level("HAULING")) / SKILL_HAULING_DIVISOR));
}

// Retrieves the Mobile target if it's still valid, or sets it to 0 if not.
uint32_t Player::mob_target()
{
    if (mob_target_)
    {
        for (size_t i = 0; i < core()->world()->mob_count(); i++)
        {
            const auto mob = core()->world()->mob_vec(i);
            if (mob->id() == mob_target_)
            {
                if (mob->location() == location_) return mob_target_;
                else break;
            }
        }
        mob_target_ = 0;   // If we couldn't make a match, or the matched Mobile is no longer here, just clear the target.
    }
    return mob_target_;
}

// Check how much money we're carrying.
uint32_t Player::money() const { return money_; }

// Retrieves the MP (or maximum MP) of the player.
int Player::mp(bool max) const { return mp_[max ? 1 : 0]; }

// Recalculates maximum HP, after toughness skill gains.
void Player::recalc_max_hp()
{
    const float old_hpm = hp_[1];

    // Recalculate the new maximum HP value.
    hp_[1] = (HP_PER_TOUGHNESS * skill_level("TOUGHNESS")) + HP_DEFAULT;

    // If max HP has been gained, increase current HP by the difference.
    const float diff = hp_[1] - old_hpm;
    hp_[0] += diff;
}

// Reduces the player's hit points.
void Player::reduce_hp(int amount, bool death_message)
{
    if (amount >= hp_[0] && tag(MobileTag::ArenaFighter)) core()->message("{m}The last thing you hear as your lifeless body hits the ground is the sadistic cheering of the crowd and the victorious yell of your opponent.");
    Mobile::reduce_hp(amount, death_message);
    if (hp_[0] > 0)
    {
        const float damage_perc = static_cast<float>(amount) / static_cast<float>(hp_[1]);
        gain_skill_xp("TOUGHNESS", damage_perc * TOUGHNESS_GAIN_MODIFIER);
    }
}

// Reduces the player's mana points.
void Player::reduce_mp(int amount)
{
    if (amount <= 0) return;
    if (amount > mp_[0]) mp_[0] = 0;
    else mp_[0] -= amount;
}

// Reduces the player's stamina points.
void Player::reduce_sp(int amount)
{
    if (amount <= 0) return;
    if (amount > sp_[0]) sp_[0] = 0;
    else sp_[0] -= amount;
}

// Removes money from the player.
void Player::remove_money(uint32_t amount)
{
    if (amount > money_)
    {
        money_ = 0;
        core()->guru()->nonfatal("Attempt to remove more money than the player owns!", Guru::GURU_ERROR);
    }
    else money_ -= amount;
}

// Restores the player's mana points.
void Player::restore_mp(int amount)
{
    if (amount <= 0) return;
    if (amount + mp_[0] >= mp_[1]) mp_[0] = mp_[1];
    else mp_[0] += amount;
}

// Restores the player's stamina points.
void Player::restore_sp(int amount)
{
    if (amount <= 0) return;
    if (amount + sp_[0] >= sp_[1]) sp_[0] = sp_[1];
    else sp_[0] += amount;
}

// Saves this Player.
uint32_t Player::save(std::shared_ptr<SQLite::Database> save_db)
{
    const uint32_t sql_id = Mobile::save(save_db);
    SQLite::Statement query(*save_db, "INSERT INTO player ( blood_tox, hunger, mob_target, money, mp, mp_max, sp, sp_delay, sp_max, sql_id, thirst ) VALUES ( :blood_tox, :hunger, :mob_target, :money, :mp, :mp_max, :sp, :sp_delay, :sp_max, :sql_id, :thirst )");
    if (blood_tox_) query.bind(":blood_tox", blood_tox_);
    query.bind(":hunger", hunger_);
    if (mob_target_) query.bind(":mob_target", mob_target_);
    query.bind(":money", money_);
    query.bind(":mp", mp_[0]);
    query.bind(":mp_max", mp_[1]);
    query.bind(":sp", sp_[0]);
    if (sp_[2]) query.bind(":sp_delay", sp_[2]);
    query.bind(":sp_max", sp_[1]);
    query.bind(":sql_id", sql_id);
    query.bind(":thirst", thirst_);
    query.exec();

    for (const auto &kv : skill_levels_)
    {
        SQLite::Statement skill_query(*save_db, "INSERT INTO skills ( id, level, xp ) VALUES ( :id, :level, :xp )");
        skill_query.bind(":id", kv.first);
        skill_query.bind(":level", kv.second);
        const auto it = skill_xp_.find(kv.first);
        if (it != skill_xp_.end()) skill_query.bind(":xp", it->second);
        skill_query.exec();
    }

    return sql_id;
}

// Sets the reason for this Player dying.
void Player::set_death_reason(const std::string &reason) { death_reason_ = reason; }

// Sets a new Mobile target.
void Player::set_mob_target(uint32_t target) { mob_target_ = target; }

// Returns the skill level of a specified skill of this Player.
int Player::skill_level(const std::string &skill_id) const
{
    const auto it = skill_levels_.find(skill_id);
    if (it == skill_levels_.end()) return 0;
    else return it->second;
}

// Returns read-only access to the player's skill levels.
const std::map<std::string, int>& Player::skill_map() const { return skill_levels_; }

// Retrieves the SP (or maximum SP) of the player.
int Player::sp(bool max) const { return sp_[max ? 1 : 0]; }

// Checks the current thirst level.
int Player::thirst() const { return thirst_; }

// The player gets a little more thirsty.
void Player::thirst_tick()
{
    switch (--thirst_)
    {
        case 0:
            core()->message("{u}You collapse from {U}severe dehydration{u}.");
            set_death_reason("died from dehydration");
            break;
        case 1: case 2: core()->message("{U}You are dying of dehydration!"); break;
        case 3: case 4: core()->message("{U}Your throat is so parched it's painful!"); break;
        case 5: case 6: core()->message("{U}You are desperately thirsty!"); break;
        case 7: case 8: core()->message("{U}You are extremely thirsty!"); break;
        case 9: case 10: core()->message("{u}Your mouth feels very dry."); break;
        case 11: case 12: core()->message("{u}You really want something to drink."); break;
        case 14: core()->message("{u}You're starting to feel a little thirsty."); break;
    }
}

// Reduces blood toxicity.
void Player::tick_blood_tox() { if (blood_tox_ > 0) blood_tox_--; }

// Regenerates HP over time.
void Player::tick_hp_regen()
{
    if (hp_[0] < hp_[1])
    {
        core()->world()->time_weather()->increase_heartbeat(TimeWeather::Heartbeat::HUNGER, REGEN_TIME_COST_HUNGER);
        core()->world()->time_weather()->increase_heartbeat(TimeWeather::Heartbeat::THIRST, REGEN_TIME_COST_THIRST);
    }
    Mobile::tick_hp_regen();
}
// Regenerates MP over time.
void Player::tick_mp_regen() { restore_mp(MP_REGEN_PER_TICK); }

// Regenerates SP over time.
void Player::tick_sp_regen() {
    if(hunger()>HUNGER_MAX){
        //Integer division means we have to track remainder
        sp_[2] += SP_REGEN_PER_TICK;
        restore_sp(sp_[2]/SP_REGEN_BLOAT_DIVISOR);
        sp_[2] %= SP_REGEN_BLOAT_DIVISOR;
    } else{restore_sp(SP_REGEN_PER_TICK);}
}

// Checks if the player is wearing a certain type of armour (light/medium/heavy).
bool Player::wearing_armour(ItemSub type)
{
    if (type == ItemSub::NONE) return (!wearing_armour(ItemSub::HEAVY) && !wearing_armour(ItemSub::MEDIUM) && !wearing_armour(ItemSub::LIGHT));
    const auto armour = equ()->get(EquipSlot::ARMOUR);
    const auto body = equ()->get(EquipSlot::BODY);
    const auto shield = equ()->get(EquipSlot::HAND_OFF);

    if (body && body->subtype() == type) return true;
    if (armour && armour->subtype() == type) return true;
    if (shield && shield->type() == ItemType::SHIELD && shield->subtype() == type) return true;
    return false;
}
