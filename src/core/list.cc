// core/list.cc -- Generic list of strings, which may or may not contain links to other lists.
// Copyright (c) 2020-2021 Raine "Gravecat" Simmons. All rights reserved.

#include "core/core.h"
#include "core/list.h"


// Returns the element at the given position of the List.
ListEntry List::at(size_t pos, bool nofollow) const
{
    if (pos >= data_.size()) throw std::runtime_error("Invalid list position: " + std::to_string(pos));
    if (data_.at(pos).str[0] == '#' && !nofollow) return core()->world()->get_list(data_.at(pos).str.substr(1))->rnd();
    else if (data_.at(pos).str[0] == '&' && !nofollow)
    {
        const int roll = core()->rng()->rnd(LIST_RARITY_RARE);
        std::string rarity = "COMMON";
        if (roll == 1)
        {
            if (core()->rng()->rnd(LIST_RARITY_SPECIAL) == 1) rarity = "SPECIAL";
            else rarity = "RARE";
        }
        else if (roll >= 2 && roll <= LIST_RARITY_UNCOMMON) rarity = "UNCOMMON";
        return core()->world()->get_list(data_.at(pos).str.substr(1) + "_" + rarity)->rnd();
    }
    else return data_.at(pos);
}

// Checks to see if an entry exists on this List.
bool List::contains(const std::string &query) const
{
    for (auto le : data_)
    {
        std::string list_data = le.str;
        if (!list_data.size()) continue;
        if (list_data[0] == '#' && core()->world()->get_list(list_data.substr(1))->contains(query)) return true;
        else if (list_data[0] == '&')
        {
            const std::string sublist_name = list_data.substr(1);
            if (core()->world()->get_list(sublist_name + "_COMMON")->contains(query)) return true;
            if (core()->world()->get_list(sublist_name + "_UNCOMMON")->contains(query)) return true;
            if (core()->world()->get_list(sublist_name + "_RARE")->contains(query)) return true;
            if (core()->world()->get_list(sublist_name + "_SPECIAL")->contains(query)) return true;
        }
        else if (list_data == query) return true;
    }
    return false;
}

// Merges a second List into this List.
void List::merge_with(std::shared_ptr<List> second_list)
{
    for (size_t i = 0; i < second_list->size(); i++)
    {
        ListEntry new_entry = second_list->at(i, true);
        data_.push_back(new_entry);
    }
}

// Adds a new item to an existing List.
void List::push_back(ListEntry item) { data_.push_back(item); }

// Returns a random element from the list, parsing any sub-lists in the process.
ListEntry List::rnd() const
{
    auto list_copy = std::make_shared<List>(*this);
    while (true)
    {
        if (!list_copy->size()) throw std::runtime_error("Could not find suitable result on list.");
        const size_t choice = core()->rng()->rnd(0, list_copy->data_.size() - 1);
        ListEntry result = list_copy->data_.at(choice);
        if (result.str.size() && result.str[0] == '#') return core()->world()->get_list(result.str.substr(1))->rnd();
        else if (result.str.size() && result.str[0] == '&')
        {
            const int roll = core()->rng()->rnd(LIST_RARITY_RARE);
            std::string rarity = "COMMON";
            if (roll == 1)
            {
                if (core()->rng()->rnd(LIST_RARITY_SPECIAL) == 1) rarity = "SPECIAL";
                else rarity = "RARE";
            }
            else if (roll >= 2 && roll <= LIST_RARITY_UNCOMMON) rarity = "UNCOMMON";
            return core()->world()->get_list(result.str.substr(1) + "_" + rarity)->rnd();
        }
        else return result;
    }
}

// Returns the size of the List.
size_t List::size() const { return data_.size(); }
