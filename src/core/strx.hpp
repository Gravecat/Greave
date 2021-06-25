// core/strx.hpp -- Various utility functions that deal with string manipulation/conversion.
// Copyright (c) 2020-2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#pragma once
#include "core/core.hpp"


class StrX
{
public:
    static const int    CL_FLAG_USE_AND, CL_FLAG_SQL_MODE;  // comma_list() flags

    enum class DirNameType : uint8_t { NORMAL, TO_THE, TO_THE_ALT };

    static std::string  comma_list(std::vector<std::string> vec, unsigned int flags = 0);   // Converts a vector to a comma-separated list.
    static std::string  dir_to_name(Direction dir, DirNameType dnt = DirNameType::NORMAL);  // Converts a direction enum into a string.
    static std::string  dir_to_name(uint8_t dir, DirNameType dnt = DirNameType::NORMAL);    // As above, but with an integer instead of an enum.
    static bool         find_and_replace(std::string &input, const std::string &to_find, const std::string &to_replace);    // Find and replace one string with another.
    static uint32_t     hash(const std::string &str);               // FNV string hash function.
    static uint32_t     htoi(const std::string &hex_str);           // Converts a hex string back to an integer.
    static std::string  itoh(unsigned int num, uint32_t min_len);   // Converts an integer into a hex string.
    static std::string  str_tolower(std::string str);               // Converts a string to lower-case.
    static std::string  str_toupper(std::string str);               // Converts a string to upper-case.
    static std::vector<std::string> string_explode(std::string str, const std::string &separator);          // String split/explode function.
    static std::vector<std::string> string_explode_colour(const std::string &str, unsigned int line_len);   // Similar to string_explode(), but takes colour into account, and wraps to a given line.
    static unsigned int strlen_colour(const std::string &str);      // Returns the length of a string, taking colour tags into account.    
    static unsigned int word_count(const std::string &str, const std::string &word);    // Returns a count of the amount of times a string is found in a parent string.

    template<class T> static void string_to_tags(const std::string &tag_string, std::set<T> &tags)
    {
        if (!tag_string.size()) return;
        std::vector<std::string> split_tags = string_explode(tag_string, " ");
        for (auto tag : split_tags)
            tags.insert(static_cast<T>(htoi(tag)));
    }

    template<class T> static std::string tags_to_string(std::set<T> tags)
    {
        if (!tags.size()) return "";
        std::string tags_str;
        for (auto tag : tags)
            if (static_cast<unsigned int>(tag) < Core::TAGS_PERMANENT) tags_str += itoh(static_cast<long long>(tag), 1) + " ";
        if (tags_str.size()) tags_str.pop_back();   // Strip off the excess space at the end.
        return tags_str;
    }
};
