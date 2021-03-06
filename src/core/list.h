// core/list.h -- Generic list of strings, which may or may not contain links to other lists.
// Copyright (c) 2020-2021 Raine "Gravecat" Simmons. All rights reserved.

#ifndef GREAVE_CORE_LIST_H_
#define GREAVE_CORE_LIST_H_

#include <cstddef>
#include <memory>
#include <string>
#include <vector>


struct ListEntry
{
    uint32_t    count;
    std::string str;
};

class List
{
public:
    ListEntry   at(size_t pos, bool nofollow = false) const;    // Returns the element at the given position of the List.
    bool        contains(const std::string &query) const;       // Checks to see if an entry exists on this List.
    void        merge_with(std::shared_ptr<List> second_list);  // Merges a second List into this List.
    void        push_back(ListEntry item);                      // Adds a new item to an existing List.
    ListEntry   rnd() const;                                    // Returns a random element from the List, parsing any sub-lists in the process.
    size_t      size() const;                                   // Returns the size of the List.

private:
    static constexpr int    LIST_RARITY_UNCOMMON =  5;      // Rarity value (1 in X chance) of an uncommon item being chosen from a weighted list.
    static constexpr int    LIST_RARITY_RARE =      12;     // As above, but for rare list items.
    static constexpr int    LIST_RARITY_SPECIAL =   100;    // As above, for special list items.

    std::vector<ListEntry>  data_;  // The list's data, a vector.
};

#endif  // GREAVE_CORE_LIST_H_
