// world/time-weather.cc -- The time and weather system.
// Copyright (c) 2021 Raine "Gravecat" Simmons. All rights reserved.
// Weather system originally based on Keran's MUSH/MUX Weather and Time Code Package Version 4.0 beta, copyright (c) 1996-1998 Keran.

#include "3rdparty/yaml-cpp/yaml.h"
#include "actions/ai.h"
#include "core/core.h"
#include "core/strx.h"
#include "world/time-weather.h"


// SQL table construction string for the heartbeat timers.
constexpr char TimeWeather::SQL_HEARTBEATS[] = "CREATE TABLE heartbeats ( id INTEGER PRIMARY KEY UNIQUE NOT NULL, count INTEGER NOT NULL )";

// SQL table construction string for time and weather data.
const char TimeWeather::SQL_TIME_WEATHER[] = "CREATE TABLE time_weather ( day INTEGER NOT NULL, moon INTEGER NOT NULL, subsecond REAL NOT NULL, time INTEGER PRIMARY KEY UNIQUE NOT NULL, time_total INTEGER NOT NULL, weather INTEGER NOT NULL )";

// The heartbeat timers, for triggering various events at periodic intervals.
const uint32_t TimeWeather::HEARTBEAT_TIMERS[TimeWeather::Heartbeat::_TOTAL] = {
    10 * Time::SECOND,  // BUFFS, for ticking down buffs/debuffs on Mobiles and the Player.
    17 * Time::MINUTE,  // CARRY, increases the player's hauling skill if they're heavily loaded.
    16 * Time::MINUTE,  // DISEASE, ticks diseases and reduces blood toxicity in the player's body.
    2 * Time::MINUTE,   // HP_REGEN, causes health to regenerate over time.
    432 * Time::MINUTE, // HUNGER. Pretty slow, as you can live for a long time without food.
    30 * Time::MINUTE,  // MOBILE_SPAWN, used to trigger Mobiles (re)spawning.
    20 * Time::SECOND,  // MP_REGEN, regenerates mana points over time.
    10 * Time::MINUTE,  // ROOM_SCARS, for decreasing the intensity of room scars.
    10 * Time::SECOND,  // SP_REGEN, regenerates stamina points over time.
    311 * Time::MINUTE, // THIRST. More rapid than hunger.
};


// Constructor, sets default values.
TimeWeather::TimeWeather() : day_(80), moon_(1), time_(39660), time_passed_(0), subsecond_(0), weather_(Weather::FAIR)
{
    weather_change_map_.resize(9);
    try
    {
        const YAML::Node yaml_weather = YAML::LoadFile("data/misc/weather.yml");
        for (auto w : yaml_weather)
        {
            const std::string id = w.first.as<std::string>();
            const std::string text = w.second.as<std::string>();
            if (id.size() == 5 && id.substr(0, 4) == "WMAP")
            {
                const int map_id = id[4] - '0';
                if (map_id < 0 || map_id > 8) throw std::runtime_error("Invalid weather map strings.");
                weather_change_map_.at(map_id) = StrX::decode_compressed_string(text);
            }
            else tw_string_map_.insert(std::pair<std::string, std::string>(id, text));
        }
    }
    catch (std::exception& e)
    {
        throw std::runtime_error("Error while loading data/misc/weather.yml: " + std::string(e.what()));
    }

    // Reset all the heartbeats.
    for (unsigned int h = 0; h < TimeWeather::Heartbeat::_TOTAL; h++)
        heartbeats_[h] = HEARTBEAT_TIMERS[h];
}

// Gets the current season.
TimeWeather::Season TimeWeather::current_season() const
{
    if (day_ > 364) throw std::runtime_error("Impossible day specified!");
    if (day_ < 79) return Season::WINTER;
    else if (day_ < 172) return Season::SPRING;
    else if (day_ <= 266) return Season::SUMMER;
    else if (day_ <= 355) return Season::AUTUMN;
    else return Season::WINTER;
}

// Returns the name of the current day of the week.
std::string TimeWeather::day_name() const
{
    int temp_day = day_;
    while (temp_day > 7) temp_day -= 7;
    switch (temp_day)
    {
        case 1: return "Sunsday";       // Sunday
        case 2: return "Moonsday";      // Monday
        case 3: return "Heavensday";    // Tuesday
        case 4: return "Oathsday";      // Wednesday
        case 5: return "Crownsday";     // Thursday
        case 6: return "Swordsday";     // Friday
        case 7: return "Silversday";    // Saturday
    }
    return "";  // Just to make the compiler shut up.
}

// Returns the current day of the month.
int TimeWeather::day_of_month() const
{
    if (day_ <= 28) return day_;
    int temp_day = day_;
    while (temp_day > 28) temp_day -= 28;
    return temp_day;
}

// Returns the day of the month in the form of a string like "1st" or "19th".
std::string TimeWeather::day_of_month_string() const
{
    const int dom = day_of_month();
    std::string str = std::to_string(dom);
    if (dom == 1 || dom == 21) return str + "st";
    else if (dom == 2 || dom == 22) return str += "nd";
    else if (dom == 3 || dom == 23) return str += "rd";
    return str + "th";
}

// Fixes weather for a specified season, to account for unavailable weather types.
TimeWeather::Weather TimeWeather::fix_weather(TimeWeather::Weather weather, TimeWeather::Season season) const
{
    if (season == Season::SPRING && weather == Weather::SLEET) weather = Weather::RAIN;
    else if (season == Season::SUMMER || season == Season::AUTUMN)
    {
        if (weather == Weather::BLIZZARD) weather = Weather::STORMY;
        else if (weather == Weather::LIGHTSNOW || weather == Weather::SLEET) weather = Weather::RAIN;
    }
    return weather;
}

// Gets the current weather, runs fix_weather() internally.
TimeWeather::Weather TimeWeather::get_weather() const { return fix_weather(weather_, current_season()); }

// Increases a specified heartbeat timer.
void TimeWeather::increase_heartbeat(Heartbeat beat, int count)
{
    if (beat >= Heartbeat::_TOTAL) throw std::runtime_error("Invalid heartbeat ID!");
    heartbeats_[static_cast<int>(beat)] += count;
}

// Checks if a given heartbeat is ready to trigger, and resets its counter.
bool TimeWeather::heartbeat_ready(Heartbeat beat)
{
    if (beat >= Heartbeat::_TOTAL) throw std::runtime_error("Invalid heartbeat ID!");
    if (heartbeats_[beat] >= HEARTBEAT_TIMERS[beat])
    {
        heartbeats_[beat] = 0;
        return true;
    }
    return false;
}

// Checks whether it's light or dark right now.
TimeWeather::LightDark TimeWeather::light_dark() const
{
    if (time_ >= 1285 * Time::MINUTE) return LightDark::NIGHT;
    else if (time_ >= 1140 * Time::MINUTE) return LightDark::DARK;
    else if (time_ >= 420 * Time::MINUTE) return LightDark::LIGHT;
    else if (time_ >= 277 * Time::MINUTE) return LightDark::DARK;
    else return LightDark::NIGHT;
}

// Loads the time/weather data from disk.
void TimeWeather::load(std::shared_ptr<SQLite::Database> save_db)
{
    SQLite::Statement query(*save_db, "SELECT * FROM time_weather");
    if (query.executeStep())
    {
        day_ = query.getColumn("day").getInt();
        moon_ = query.getColumn("moon").getInt();
        subsecond_ = query.getColumn("subsecond").getDouble();
        time_ = query.getColumn("time").getInt();
        time_passed_ = query.getColumn("time_total").getUInt();
        weather_ = static_cast<Weather>(query.getColumn("weather").getInt());
    }
    else throw std::runtime_error("Could not load time and weather data!");

    SQLite::Statement heartbeat_query(*save_db, "SELECT * FROM heartbeats");
    while (heartbeat_query.executeStep())
    {
        uint32_t id = heartbeat_query.getColumn("id").getUInt();
        if (id >= Heartbeat::_TOTAL) throw std::runtime_error("Invalid heartbeat data!");
        heartbeats_[id] = heartbeat_query.getColumn("count").getUInt();
    }
}

// Returns the name of the current month.
std::string TimeWeather::month_name() const
{
    if (day_ <= 28) return "Harrowing";            // January
    else if (day_ <= 56) return "Shadows";         // February
    else if (day_ <= 84) return "the Lord";        // March
    else if (day_ <= 112) return "the Lady";       // April
    else if (day_ <= 140) return "the Fall";       // May
    else if (day_ <= 168) return "Fortune";        // June
    else if (day_ <= 196) return "Fire";           // Sol
    else if (day_ <= 224) return "Gold";           // July
    else if (day_ <= 252) return "Seeking";        // August
    else if (day_ <= 280) return "the Serpent";    // September
    else if (day_ <= 308) return "Crimson";        // October
    else if (day_ <= 336) return "King's Night";   // November
    else return "Frost";                            // December
}

// Gets the current lunar phase.
TimeWeather::LunarPhase TimeWeather::moon_phase() const
{
    switch (moon_)
    {
        case 0: return LunarPhase::NEW;
        case 1: case 2: case 3: case 4: case 5: case 6: return LunarPhase::WAXING_CRESCENT;
        case 7: case 8: case 9: return LunarPhase::FIRST_QUARTER;
        case 10: case 11: case 12: case 13: case 14: return LunarPhase::WAXING_GIBBOUS;
        case 15: return LunarPhase::FULL;
        case 16: case 17: case 18: case 19: case 20: return LunarPhase::WANING_GIBBOUS;
        case 21: case 22: case 23: return LunarPhase::THIRD_QUARTER;
        case 24: case 25: case 26: case 27: case 28: return LunarPhase::WANING_CRESCENT;
        default: throw std::runtime_error("Impossible moon phase!");
    }
}

// Causes time to pass.
bool TimeWeather::pass_time(float seconds, bool interruptable)
{
    if (seconds <= UNINTERRUPTABLE_TIME) interruptable = false;
    const auto world = core()->world();
    const auto player = world->player();
    const auto room = world->get_room(player->location());
    const bool indoors = room->tag(RoomTag::Indoors);
    const bool can_see_outside = room->tag(RoomTag::CanSeeOutside);
    const bool player_is_resting = player->tag(MobileTag::Resting);

    // Determine how many seconds to pass.
    subsecond_ += seconds;
    int seconds_to_add = 0;
    if (subsecond_ >= 1.0f)
    {
        seconds_to_add = std::floor(subsecond_);
        subsecond_ -= seconds_to_add;
    }

    int old_hp = player->hp();
    int old_hunger = player->hunger();
    int old_thirst = player->thirst();
    while (seconds_to_add--)
    {
        if (player->is_dead()) return false;    // Don't pass time if the player is dead.

        // Interrupt the action if the player takes damage.
        if (interruptable)
        {
            const int hp = player->hp();
            const int hunger = player->hunger();
            const int thirst = player->thirst();
            if (hp < old_hp || (hunger < old_hunger && hunger <= 6) || (thirst < old_thirst && thirst <= 6)) return false;
            old_hp = hp;
            old_hunger = hunger;
            old_thirst = thirst;
        }

        time_passed_++;    // The total time passed in the game. This will loop every 136 years, but that's not a problem; see time_passed().

        // Increase all heartbeat timers.
        for (unsigned int h = 0; h < Heartbeat::_TOTAL; h++)
            heartbeats_[h]++;

        // Update the time of day and weather.
        const bool show_weather_messages = (!indoors || can_see_outside);
        TimeOfDay old_time_of_day = time_of_day(true);
        int old_time = time_;
        bool change_happened = false;
        std::string weather_msg;
        if (++time_ >= Time::DAY) time_ -= Time::DAY;
        if (time_ >= 420 * Time::MINUTE && old_time < 420 * Time::MINUTE)   // Trigger moon-phase changing and day-of-year changing at dawn, not midnight.
        {
            if (++day_ > 364) day_ = 1;
            if (++moon_ >= LUNAR_CYCLE_DAYS) moon_ = 0;
            core()->message("{B}It is now " + day_name() + ", the " + day_of_month_string() + " day of " +  month_name() + ".");
        }
        old_time = time_;
        if (time_of_day(true) != old_time_of_day)
        {
            weather_msg = "";
            old_time_of_day = time_of_day(true);
            trigger_event(current_season(), &weather_msg, !show_weather_messages);
            change_happened = show_weather_messages;
        }
        if (change_happened && !player_is_resting) core()->message(weather_message_colour() + weather_msg.substr(1));

        // Runs the AI on all active mobiles.
        AI::tick_mobs();
        if (player->is_dead()) return true;

        std::set<uint32_t> active_rooms;    // This starts empty, but can be re-used if multiple heartbeats need to check active rooms.

        // Scan through all active rooms, respawning NPCs if needed.
        if (heartbeat_ready(Heartbeat::MOBILE_SPAWN))
        {
            if (!active_rooms.size()) active_rooms = world->active_rooms();
            for (auto room_id : active_rooms)
            {
                const auto room = world->get_room(room_id);
                room->respawn_mobs();
            }
        }

        // Reduce room scars on active rooms.
        if (heartbeat_ready(Heartbeat::ROOM_SCARS))
        {
            if (!active_rooms.size()) active_rooms = world->active_rooms();
            for (auto room_id : active_rooms)
            {
                const auto room = world->get_room(room_id);
                room->decay_scars();
            }
        }

        // Reduce timers on buffs for all Mobiles and the Player.
        if (heartbeat_ready(Heartbeat::BUFFS))
        {
            player->tick_buffs();
            if (player->is_dead()) return true;
            for (size_t m = 0; m < world->mob_count(); m++)
                world->mob_vec(m)->tick_buffs();
        }

        // Increases the player's hunger.
        if (heartbeat_ready(Heartbeat::HUNGER))
        {
            player->hunger_tick();
            if (player->is_dead()) return true;
        }

        // Increases the player's thirst.
        if (heartbeat_ready(Heartbeat::THIRST))
        {
            player->thirst_tick();
            if (player->is_dead()) return true;
        }

        // Regenerates hit points over time.
        if (heartbeat_ready(Heartbeat::HP_REGEN))
        {
            player->tick_hp_regen();
            for (unsigned int i = 0; i < world->mob_count(); i++)
                world->mob_vec(i)->tick_hp_regen();
        }

        // Regenerates stamina points over time.
        if (heartbeat_ready(Heartbeat::SP_REGEN)) player->tick_sp_regen();

        // Regenerates mana points over time.
        if (heartbeat_ready(Heartbeat::MP_REGEN)) player->tick_mp_regen();

        // Ticks diseases and reduces blood toxicity.
        if (heartbeat_ready(Heartbeat::DISEASE))
        {
            player->tick_blood_tox();
        }

        // Increases the player's hauling skill if they are over-encumbered.
        if (heartbeat_ready(Heartbeat::CARRY))
            if (player->carry_weight() > std::round(static_cast<float>(player->max_carry()) * 0.75f)) player->gain_skill_xp("HAULING", XP_WHILE_ENCUMBERED);
    }

    return true;
}

// Saves the time/weather data to disk.
void TimeWeather::save(std::shared_ptr<SQLite::Database> save_db) const
{
    SQLite::Statement query(*save_db, "INSERT INTO time_weather ( day, moon, subsecond, time, time_total, weather ) VALUES ( :day, :moon, :subsecond, :time, :time_total, :weather )");
    query.bind(":day", day_);
    query.bind(":moon", moon_);
    query.bind(":subsecond", subsecond_);
    query.bind(":time", time_);
    query.bind(":time_total", time_passed_);
    query.bind(":weather", static_cast<int>(weather_));
    query.exec();

    for (unsigned int h = 0; h < Heartbeat::_TOTAL; h++)
    {
        SQLite::Statement heartbeat_query(*save_db, "INSERT INTO heartbeats ( id, count ) VALUES ( :id, :count )");
        heartbeat_query.bind(":id", h);
        heartbeat_query.bind(":count", heartbeats_[h]);
        heartbeat_query.exec();
    }
}

// Converts a season enum to a string.
std::string TimeWeather::season_str(TimeWeather::Season season) const
{
    switch (season)
    {
        case Season::WINTER: return "WINTER";
        case Season::SPRING: return "SPRING";
        case Season::AUTUMN: return "AUTUMN";
        case Season::SUMMER: return "SUMMER";
        default: throw std::runtime_error("Invalid season specified!");
    }
}

// Returns the current time of day (morning, day, dusk, night).
TimeWeather::TimeOfDay TimeWeather::time_of_day(bool fine) const
{
    if (fine)
    {
        if (time_ >= 1380 * Time::MINUTE) return TimeOfDay::MIDNIGHT;
        else if (time_ >= 1260 * Time::MINUTE) return TimeOfDay::NIGHT;
        else if (time_ >= 1140 * Time::MINUTE) return TimeOfDay::DUSK;
        else if (time_ >= 1020 * Time::MINUTE) return TimeOfDay::SUNSET;
        else if (time_ >= 660 * Time::MINUTE) return TimeOfDay::NOON;
        else if (time_ >= 540 * Time::MINUTE) return TimeOfDay::MORNING;
        else if (time_ >= 420 * Time::MINUTE) return TimeOfDay::SUNRISE;
        else if (time_ >= 300 * Time::MINUTE) return TimeOfDay::DAWN;
        return TimeOfDay::MIDNIGHT;
    } else
    {
        if (time_ >= 1380 * Time::MINUTE) return TimeOfDay::NIGHT;
        if (time_ >= 1140 * Time::MINUTE) return TimeOfDay::DUSK;
        if (time_ >= 540 * Time::MINUTE) return TimeOfDay::DAY;
        if (time_ >= 300 * Time::MINUTE) return TimeOfDay::DAWN;
        return TimeOfDay::NIGHT;
    }
}

// Returns the exact time of day.
int TimeWeather::time_of_day_exact() const { return time_; }

// Returns the current time of day as a string.
std::string TimeWeather::time_of_day_str(bool fine) const
{
    if (fine)
    {
        if (time_ >= 1380 * Time::MINUTE) return "MIDNIGHT";
        else if (time_ >= 1260 * Time::MINUTE) return "NIGHT";
        else if (time_ >= 1140 * Time::MINUTE) return "DUSK";
        else if (time_ >= 1020 * Time::MINUTE) return "SUNSET";
        else if (time_ >= 660 * Time::MINUTE) return "NOON";
        else if (time_ >= 540 * Time::MINUTE) return "MORNING";
        else if (time_ >= 420 * Time::MINUTE) return "SUNRISE";
        else if (time_ >= 300 * Time::MINUTE) return "DAWN";
        return "NIGHT";
    } else
    {
        if (time_ >= 1380 * Time::MINUTE) return "NIGHT";
        if (time_ >= 1140 * Time::MINUTE) return "DUSK";
        if (time_ >= 540 * Time::MINUTE) return "DAY";
        if (time_ >= 300 * Time::MINUTE) return "DAWN";
        return "NIGHT";
    }
}

void TimeWeather::trigger_event(TimeWeather::Season season, std::string *message_to_append, bool silent)
{
    const std::string weather_map = weather_change_map_.at(static_cast<int>(weather_));
    const char new_weather = weather_map[core()->rng()->rnd(0, weather_map.size() - 1)];
    switch (new_weather)
    {
        case 'c': weather_ = Weather::CLEAR; break;
        case 'f': weather_ = Weather::FAIR; break;
        case 'r': weather_ = Weather::RAIN; break;
        case 'F': weather_ = Weather::FOG; break;
        case 'S': weather_ = Weather::STORMY; break;
        case 'o': weather_ = Weather::OVERCAST; break;
        case 'b': weather_ = Weather::BLIZZARD; break;
        case 'l': weather_ = Weather::LIGHTSNOW; break;
        case 'L': weather_ = Weather::SLEET; break;
    }
    if (silent) return;

    // Display an appropriate message for the changing time/weather, if we're outdoors.
    const std::shared_ptr<Room> room = core()->world()->get_room(core()->world()->player()->location());
    const bool indoors = room->tag(RoomTag::Indoors);
    const bool can_see_outside = room->tag(RoomTag::CanSeeOutside);
    if (indoors && !can_see_outside) return;
    const std::string time_message = tw_string_map_.at(time_of_day_str(true) + "_" + weather_str(fix_weather(weather_, season)) + (indoors ? "_INDOORS" : ""));
    if (message_to_append) *message_to_append += " " + time_message;
    else core()->message(weather_message_colour() + time_message);
}

// Returns the total amount of seconds that passed in the game.
uint32_t TimeWeather::time_passed() const { return time_passed_; }

// Checks how much time has passed since a given time integer. Handles integer overflow loops.
uint32_t TimeWeather::time_passed_since(uint32_t since) const
{
    // If the total time hasn't looped yet, no problem! This is easy!
    if (since <= time_passed_) return time_passed_ - since;

    // If not, we'll try to work around the overflow. This ain't great if we overflowed twice, but shouldn't cause too many problems.
    return UINT32_MAX - since + time_passed_;
}

// Returns a weather description for the current time/weather, based on the current season.
std::string TimeWeather::weather_desc() const { return weather_desc(current_season()); }

// Returns a weather description for the current time/weather, based on the specified season.
std::string TimeWeather::weather_desc(TimeWeather::Season season) const
{
    const std::shared_ptr<Room> room = core()->world()->get_room(core()->world()->player()->location());
    const bool trees = room->tag(RoomTag::Trees);
    const bool indoors = room->tag(RoomTag::Indoors);
    const Weather weather = fix_weather(weather_, season);
    std::string desc = tw_string_map_.at(season_str(season) + "_" + time_of_day_str(false) + "_" + weather_str(weather)  + (indoors ? "_INDOORS" : ""));
    if (trees)
    {
        std::string tree_time = "DAY";
        if (time_of_day(false) == TimeOfDay::DUSK || time_of_day(false) == TimeOfDay::NIGHT) tree_time = "NIGHT";
        desc += " " + tw_string_map_.at(season_str(season) + "_" + tree_time + "_" + weather_str(weather) + "_TREES");
    }
    return desc;
}

// Returns a colour to be used for time/weather messages, based on the time of day.
std::string TimeWeather::weather_message_colour() const
{
    switch (light_dark())
    {
        case LightDark::DARK: return "{U}";
        case LightDark::LIGHT: return "{C}";
        case LightDark::NIGHT: return "{u}";
        default: return ""; // Just to keep the compiler happy.
    }
}

// Converts a weather integer to a string.
std::string TimeWeather::weather_str(TimeWeather::Weather weather) const
{
    switch (weather)
    {
        case Weather::BLIZZARD: return "BLIZZARD";
        case Weather::STORMY: return "STORMY";
        case Weather::RAIN: return "RAIN";
        case Weather::CLEAR: return "CLEAR";
        case Weather::FAIR: return "FAIR";
        case Weather::OVERCAST: return "OVERCAST";
        case Weather::FOG: return "FOG";
        case Weather::LIGHTSNOW: return "LIGHTSNOW";
        case Weather::SLEET: return "SLEET";
        default: throw std::runtime_error("Invalid weather specified: " + std::to_string(static_cast<int>(weather)));
    }
}
