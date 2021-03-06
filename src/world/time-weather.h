// world/time-weather.h -- The time and weather system.
// Copyright (c) 2021 Raine "Gravecat" Simmons. All rights reserved.
// Weather system originally based on Keran's MUSH/MUX Weather and Time Code Package Version 4.0 beta, copyright (c) 1996-1998 Keran.

#ifndef GREAVE_WORLD_TIME_WEATHER_H_
#define GREAVE_WORLD_TIME_WEATHER_H_

#include "3rdparty/SQLiteCpp/Database.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>


class TimeWeather
{
public:
    enum Heartbeat : uint32_t { BUFFS, CARRY, DISEASE, HP_REGEN, HUNGER, MOBILE_SPAWN, MP_REGEN, ROOM_SCARS, SP_REGEN, THIRST, _TOTAL };
    enum class LightDark : uint8_t { LIGHT, DARK, NIGHT };
    enum class LunarPhase : uint8_t { NEW, WAXING_CRESCENT, FIRST_QUARTER, WAXING_GIBBOUS, FULL, WANING_GIBBOUS, THIRD_QUARTER, WANING_CRESCENT };
    enum class Season : uint8_t { AUTO, WINTER, SPRING, SUMMER, AUTUMN };
    enum class TimeOfDay : uint8_t { DAWN, SUNRISE, MORNING, NOON, SUNSET, DUSK, NIGHT, MIDNIGHT, DAY };
    enum class Weather : uint8_t { BLIZZARD, STORMY, RAIN, CLEAR, FAIR, OVERCAST, FOG, LIGHTSNOW, SLEET };
    enum Time { SECOND = 1, MINUTE = 60, HOUR = 3600, DAY = 86400 };

    static const char   SQL_HEARTBEATS[];   // SQL table construction string for the heartbeat timers.
    static const char   SQL_TIME_WEATHER[]; // SQL table construction string for time and weather data.

                TimeWeather();                      // Constructor, sets default values.
    Season      current_season() const;             // Gets the current season.
    std::string day_name() const;                   // Returns the name of the current day of the week.
    int         day_of_month() const;               // Returns the current day of the month.
    std::string day_of_month_string() const;        // Returns the day of the month in the form of a string like "1st" or "19th".
    Weather     get_weather() const;                // Gets the current weather, runs fix_weather() internally.
    void        increase_heartbeat(Heartbeat beat, int count);  // Increases a specified heartbeat timer.
    LightDark   light_dark() const;                 // Checks whether it's light or dark right now.
    void        load(std::shared_ptr<SQLite::Database> save_db);    // Loads the time/weather data from disk.
    std::string month_name() const;                 // Returns the name of the current month.
    LunarPhase  moon_phase() const;                 // Gets the current lunar phase.
    bool        pass_time(float seconds, bool interruptable);       // Causes time to pass.
    void        save(std::shared_ptr<SQLite::Database> save_db) const;  // Saves the time/weather data to disk.
    std::string season_str(Season season) const;    // Converts a season enum to a string.
    TimeOfDay   time_of_day(bool fine) const;       // Returns the current time of day (morning, day, dusk, night).
    int         time_of_day_exact() const;          // Returns the exact time of day.
    std::string time_of_day_str(bool fine) const;   // Returns the current time of day as a string.
    uint32_t    time_passed() const;                // Returns the total amount of seconds that passed in the game.
    uint32_t    time_passed_since(uint32_t since) const;    // Checks how much time has passed since a given time integer. Handles integer overflow loops.
    std::string weather_desc() const;               // Returns a weather description for the current time/weather, based on the current season.
    std::string weather_message_colour() const;     // Returns a colour to be used for time/weather messages, based on the time of day.
    std::string weather_str(Weather weather) const; // Converts a weather integer to a string.

private:
    static constexpr int    LUNAR_CYCLE_DAYS =      29;             // How many days are in a lunar cycle?
    static constexpr float  UNINTERRUPTABLE_TIME =  5;              // The maximum amount of time for an action that cannot be interrupted.
    static constexpr int    XP_WHILE_ENCUMBERED =   1;              // How much XP to grant per carry tick for encumbered players.
    static const uint32_t   HEARTBEAT_TIMERS[Heartbeat::_TOTAL];    // The heartbeat timers, for triggering various events at periodic intervals.

    Weather     fix_weather(Weather weather, Season season) const;  // Fixes weather for a specified season, to account for unavailable weather types.
    bool        heartbeat_ready(Heartbeat beat);                    // Checks if a given heartbeat is ready to trigger, and resets its counter.
    void        trigger_event(Season season, std::string *message_to_append, bool silent);  // Triggers a time-change event.
    std::string weather_desc(Season season) const;                  // Returns a weather description for the current time/weather, based on the specified season.

    int         day_;                           // The current day of the year.
    uint32_t    heartbeats_[Heartbeat::_TOTAL]; // The heartbeat timers, for triggering various events at periodic intervals.
    int         moon_;                          // The current moon phase.
    int         time_;                          // The time of day.
    uint32_t    time_passed_;                   // The total # of seconds that have passed since the game started. This will loop every ~136 years, see time_passed().
    float       subsecond_;                     // For counting time passed in amounts of time less than a second.
    Weather     weather_;                       // The current weather.

    std::map<std::string, std::string>  tw_string_map_;         // The time and weather strings from data/misc/weather.yml
    std::vector<std::string>            weather_change_map_;    // Weather change maps, to determine odds of changing to different weather types.
};

#endif  // GREAVE_WORLD_TIME_WEATHER_H_
