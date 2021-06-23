// core/utility.hpp -- Miscellaneous utility functions which don't really fit into any one specific place.
// Copyright (c) 2020-2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#pragma once
#include "core/greave.hpp"


class Util
{
public:
    static void         delete_file(const std::string &filename);   // Deletes a specified file.
    static std::string  dir_to_name(Direction dir);                 // Converts a direction enum into a string.
    static bool         directory_exists(const std::string &dir);   // Check if a directory exists.
    static bool         file_exists(const std::string &file);       // Checks if a file exists.
    static std::vector<std::string> files_in_dir(const std::string &directory, bool recursive = false); // Returns a list of files in a given directory.
    static bool         find_and_replace(std::string &input, const std::string &to_find, const std::string &to_replace);    // Find and replace one string with another.
    static unsigned int hash(const std::string &str);               // FNV string hash function.
    static uint32_t     htoi(const std::string &hex_str);           // Converts a hex string back to an integer.
    static void         make_dir(const std::string &dir);           // Makes a new directory, if it doesn't already exist.
    static std::string  str_tolower(std::string str);               // Converts a string to lower-case.
    static std::vector<std::string> string_explode(std::string str, const std::string &separator);          // String split/explode function.
    static std::vector<std::string> string_explode_colour(const std::string &str, unsigned int line_len);   // Similar to string_explode(), but takes colour into account, and wraps to a given line.
    static unsigned int strlen_colour(const std::string &str);      // Returns the length of a string, taking colour tags into account.    
    static unsigned int word_count(const std::string &str, const std::string &word);    // Returns a count of the amount of times a string is found in a parent string.
};
