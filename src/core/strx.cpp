// core/strx.cpp -- Various utility functions that deal with string manipulation/conversion.
// Copyright (c) 2020-2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#include "core/core.hpp"
#include "core/guru.hpp"
#include "core/strx.hpp"

#include <algorithm>
#include <sstream>


const int StrX::CL_FLAG_USE_AND = 1, StrX::CL_FLAG_SQL_MODE = 2;    // comma_list() flags


std::string StrX::comma_list(std::vector<std::string> vec, unsigned int flags)
{
	const bool use_and = ((flags & CL_FLAG_USE_AND) == CL_FLAG_USE_AND);
	const bool sql_mode = ((flags & CL_FLAG_SQL_MODE) == CL_FLAG_SQL_MODE);
	if (!vec.size())
	{
		core()->guru()->nonfatal("Empty vector provided to comma_list!", Guru::WARN);
		return "";
	}
	if (vec.size() == 1) return vec.at(0);
	std::string plus = " and ";
	if (!use_and)
	{
		if (sql_mode) plus = ", ";
		else plus = " or ";
	}
	else if (vec.size() == 2) return vec.at(0) + plus + vec.at(1);

	std::string str;
	for (unsigned int i = 0; i < vec.size(); i++)
	{
		str += vec.at(i);
		if (i < vec.size() - 1)
		{
			if (i == vec.size() - 2) str += plus;
			else str += ", ";
		}
	}

	return str;
}

// Converts a direction enum into a string.
std::string StrX::dir_to_name(Direction dir, bool elaborate)
{
    std::string prefix;
    if (elaborate)
    {
        if (dir == Direction::UP) return "above";
        else if (dir == Direction::DOWN) return "below";
        else prefix = "the ";
    }
    switch(dir)
    {
        case Direction::NORTH: return prefix + "north";
        case Direction::SOUTH: return prefix + "south";
        case Direction::EAST: return prefix + "east";
        case Direction::WEST: return prefix + "west";
        case Direction::NORTHEAST: return prefix + "northeast";
        case Direction::NORTHWEST: return prefix + "northwest";
        case Direction::SOUTHEAST: return prefix + "southeast";
        case Direction::SOUTHWEST: return prefix + "southwest";
        case Direction::UP: return "up";
        case Direction::DOWN: return "down";
        case Direction::NONE: return "????";
        default:
            throw std::runtime_error("Invalid direction enum: " + std::to_string(static_cast<int>(dir)));
            return "";
	}
}

// As above, but with an integer instead of an enum.
std::string StrX::dir_to_name(uint8_t dir, bool elaborate) { return dir_to_name(static_cast<Direction>(dir), elaborate); }

// Find and replace one string with another.
bool StrX::find_and_replace(std::string &input, const std::string &to_find, const std::string &to_replace)
{
    std::string::size_type pos = 0;
    const std::string::size_type find_len = to_find.length(), replace_len = to_replace.length();
    if (find_len == 0) return false;
    bool found = false;
    while ((pos = input.find(to_find, pos)) != std::string::npos)
    {
        found = true;
        input.replace(pos, find_len, to_replace);
        pos += replace_len;
    }
    return found;
}

// FNV string hash function.
uint32_t StrX::hash(const std::string &str)
{
    uint32_t result = 2166136261U;
    std::string::const_iterator end = str.end();
    for (std::string::const_iterator iter = str.begin(); iter != end; ++iter)
        result = 127 * result + static_cast<unsigned char>(*iter);
    return result;
}

// Converts a hex string back to an integer.
uint32_t StrX::htoi(const std::string &hex_str)
{
    std::stringstream ss;
    ss << std::hex << hex_str;
    uint32_t result;
    ss >> result;
    return result;
}

// Converts a string to lower-case.
std::string StrX::str_tolower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

// String split/explode function.
std::vector<std::string> StrX::string_explode(std::string str, const std::string &separator)
{
    std::vector<std::string> results;

    std::string::size_type pos = str.find(separator, 0);
    const int pit = separator.length();

    while(pos != std::string::npos)
    {
        if (pos == 0) results.push_back("");
        else results.push_back(str.substr(0, pos));
        str.erase(0, pos + pit);
        pos = str.find(separator, 0);
    }
    results.push_back(str);

    return results;
}

// Similar to string_explode(), but takes colour and high/low-ASCII tags into account, and wraps to a given line length.
std::vector<std::string> StrX::string_explode_colour(const std::string &str, unsigned int line_len)
{
    std::vector<std::string> output;

    // Check to see if the line of text has the no-split tag at the start.
    if (str.size() >= 3)
    {
        if (!str.substr(0, 3).compare("{_}"))
        {
            output.push_back(str.substr(3));
            return output;
        }
    }

    // Check to see if the line is too short to be worth splitting.
    if (strlen_colour(str) <= line_len && str.find("{nl}") != std::string::npos && str.find("{lb}") != std::string::npos)
    {
        output.push_back(str);
        return output;
    }

    // Split the string into individual words.
    std::vector<std::string> words = string_explode(str, " ");

    // Keep track of the current line and our position on it.
    unsigned int current_line = 0, line_pos = 0;
    std::string last_colour = "{w}";    // The last colour tag we encountered; white by default.

    // Start with an empty string.
    output.push_back("");

    for (auto word : words)
    {
        if (word == "{nl}") // Check for new-line marker.
        {
            if (line_pos > 0)
            {
                line_pos = 0;
                current_line += 2;
                output.push_back(" ");
                output.push_back(last_colour);  // Start the line with the last colour tag we saw.
            }
        }
        else if (word == "{lb}")
        {
            if (line_pos > 0)
            {
                line_pos = 0;
                current_line += 1;
                output.push_back(last_colour);  // Start the line with the last colour tag we saw.
            }
        }
        else
        {
            unsigned int length = word.length();    // Find the length of the word.

            const int colour_count = word_count(word, "{"); // Count the colour tags.
            if (colour_count) length -= (colour_count * 3); // Reduce the length if one or more colour tags are found.
            if (length + line_pos >= line_len)  // Is the word too long for the current line?
            {
                line_pos = 0; current_line++;   // CR;LF
                output.push_back(last_colour);  // Start the line with the last colour tag we saw.
            }
            if (colour_count)
            {
                // Duplicate the last-used colour tag.
                const std::string::size_type flo = word.find_last_of("{");
                if (flo != std::string::npos && word.size() >= flo + 3) last_colour = word.substr(flo, 3);
            }
            if (line_pos != 0)  // NOT the start of a new line?
            {
                length++;
                output.at(current_line) += " ";
            }

            // Is the word STILL too long to fit over a single line?
            while (length > line_len)
            {
                const std::string trunc = word.substr(0, line_len);
                word = word.substr(line_len);
                output.at(current_line) += trunc;
                line_pos = 0;
                current_line++;
                output.push_back(last_colour);  // Start the line with the last colour tag we saw.
                length = word.size();   // Adjusts the length for what we have left over.
            }
            output.at(current_line) += word;
            line_pos += length;
        }
    }

    return output;
}

// Returns the length of a string, taking colour and high/low-ASCII tags into account.
unsigned int StrX::strlen_colour(const std::string &str)
{
    unsigned int len = str.size();

    // Count any colour tags.
    const int openers = std::count(str.begin(), str.end(), '{');
    if (openers) len -= openers * 3;

    return len;
}

// Returns a count of the amount of times a string is found in a parent string.
unsigned int StrX::word_count(const std::string &str, const std::string &word)
{
    unsigned int count = 0;
    std::string::size_type word_pos = 0;
    while(word_pos != std::string::npos)
    {
        word_pos = str.find(word, word_pos);
        if (word_pos != std::string::npos)
        {
            count++;
            word_pos += word.length();
        }
    }
    return count;
}
