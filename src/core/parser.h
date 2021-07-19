// core/parser.h -- The command parser! Converts player input into commands that the game can understand.
// Copyright (c) 2021 Raine "Gravecat" Simmons and the Greave contributors. Licensed under the GNU Affero General Public License v3 or any later version.

#ifndef GREAVE_CORE_PARSER_H_
#define GREAVE_CORE_PARSER_H_

#include "world/room.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>


enum ParserTarget : uint8_t { TARGET_NONE = 0, TARGET_EQUIPMENT = 1, TARGET_INVENTORY = 2, TARGET_MOBILE = 4, TARGET_ROOM = 8, TARGET_SHOP = 16, TARGET_CONTAINER_ROOM = 32, TARGET_CONTAINER_INVENTORY = 64, TARGET_UNCLEAR = 128 };

class Parser
{
public:
                Parser();                               // Constructor, sets up the parser.
    void        confirm_message();                      // Tells the player how to confirm a command.
    void        interrupted(const std::string &action); // The player was interrupted trying to perform an action.
    void        parse(std::string input);               // Parses input from the player!
    Direction   parse_direction(const std::string &dir) const;  // Parses a string into a Direction enum.
    int32_t     parse_int(const std::string &s);        // Wrapper function to check for out of range values.

private:
    enum class ParserCommand : uint16_t { NONE, ABILITIES, ADD_MONEY, ATTACK, BROWSE, BUY, CAREFUL_AIM, CLOSE, COLOUR_TEST, DIRECTION, DRINK, DROP, EAT, EMPTY, EQUIP, EQUIPMENT, EXAMINE, EXCLAIM, EXITS, EYE_FOR_AN_EYE, FILL, GO, GRIT, HASH, HEADLONG_STRIKE, HEAL_CHEAT, HELP, INVENTORY, LADY_LUCK, LOCK, LOOK, MIXUP, MIXUP_BIG, NO, OPEN, PARTICIPATE, QUICK_ROLL, RAPID_STRIKE, SAVE, SCORE, SELL, SHIELD_WALL, SKILLS, SNAP_SHOT, SPAWN_ITEM, SPAWN_MOBILE, STANCE, STATUS, SWEAR, TAKE, TELEPORT, TIME, UNEQUIP, UNLOCK, VOMIT, WAIT, WEATHER, XYZZY, YES, QUIT };
    enum class SpecialState : uint8_t { NONE, QUIT_CONFIRM, DISAMBIGUATION };

    struct ParserCommandData
    {
        bool                        any_length;
        bool                        direction_match;
        ParserCommand               command;
        std::string                 first_word;
        bool                        target_match;
        std::vector<std::string>    words;
    };

    struct ParserSearchResult
    {
        int             score;
        std::string     name, name_np;
        uint32_t        parser_id;
        size_t          target, target_second;
        ParserTarget    type;
        int             count;
    };

    void                add_command(const std::string &text, ParserCommand cmd);            // Adds a command to the parser.
    ParserSearchResult  parse_target(std::vector<std::string> input, ParserTarget target);  // Attempts to match a name to a given target.
    void                parse_pcd(const std::string &first_word, const std::vector<std::string> &words, ParserCommandData pcd, bool confirm);   // Parses a known command.

    std::vector<ParserCommandData>  commands_;      // The commands the parser can understand.
    std::string                     last_input_;    // The last raw input from the player.
    SpecialState                    special_state_; // Special parser states, such as waiting for the player to confirm something.
};

#endif  // GREAVE_CORE_PARSER_H_
