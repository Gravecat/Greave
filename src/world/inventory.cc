// world/inventory.cc -- The Inventory class stores a collection of Items, and handles stacking, organizing, saving/loading, etc.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#include "core/core.h"
#include "world/inventory.h"

#include <algorithm>


// Creates a new, blank inventory.
Inventory::Inventory(uint8_t pid_prefix) : pid_prefix_(pid_prefix) { }

// Adds an Item to this Inventory (this will later handle auto-stacking, etc.)
void Inventory::add_item(std::shared_ptr<Item> item, bool force_stack)
{
    // Checks if there's anything else here that can be stacked.
    if (force_stack || item->tag(ItemTag::Stackable))
    {
        for (size_t i = 0; i < items_.size(); i++)
        {
            if (!force_stack && !items_.at(i)->tag(ItemTag::Stackable)) continue;
            if (item->is_identical(items_.at(i)))
            {
                items_.at(i)->set_stack(item->stack() + items_.at(i)->stack());

                // Compare appraised values, and pick the most accurate of the two.
                const int appraised_value_a = items_.at(i)->meta_int("appraised_value");
                const int appraised_value_b = item->meta_int("appraised_value");
                if (appraised_value_a != appraised_value_b)
                {
                    if (!appraised_value_a) items_.at(i)->set_meta("appraised_value", appraised_value_b);
                    else
                    {
                        const int diff_a = std::abs(appraised_value_a - static_cast<int>(items_.at(i)->value(true)));
                        const int diff_b = std::abs(appraised_value_b - static_cast<int>(items_.at(i)->value(true)));
                        if (diff_a > diff_b) items_.at(i)->set_meta("appraised_value", appraised_value_b);
                    }
                }
                return;
            }
        }
    }

    update_prefix(item);
    items_.push_back(item);
}

// Updates the prefix of an item to match this inventory.
void Inventory::update_prefix(std::shared_ptr<Item> item) const
{
    // Check the item's parser ID. If it's unset, or if another item in the inventory shares its ID, we'll need a new one. Infinite loops relying on RNG to break out are VERY BAD so let's put a threshold on this bad boy.
    int tries = 0;
    item->set_parser_id_prefix(pid_prefix_);
    while (parser_id_exists(item->parser_id()) && ++tries < 10000)
        item->new_parser_id(pid_prefix_);
}

// As above, but generates a new Item from a template with a specified ID.
void Inventory::add_item(const std::string &id, bool force_stack) { add_item(core()->world()->get_item(id), force_stack); }

// Locates the position of an ammunition item used by the specified weapon.
size_t Inventory::ammo_pos(std::shared_ptr<Item> item)
{
    if (item->subtype() != ItemSub::RANGED || item->tag(ItemTag::NoAmmo)) return SIZE_MAX;
    ItemSub ammo_type = ItemSub::NONE;
    if (item->tag(ItemTag::AmmoArrow)) ammo_type = ItemSub::ARROW;
    else if (item->tag(ItemTag::AmmoBolt)) ammo_type = ItemSub::BOLT;
    else throw std::runtime_error("Could not determine ammo type for " + item->name());
    for (size_t i = 0; i < items_.size(); i++)
    {
        const auto inv_item = items_.at(i);
        if (inv_item->type() == ItemType::AMMO && inv_item->subtype() == ammo_type) return i;
    }
    return SIZE_MAX;
}

// Erases everything from this inventory.
void Inventory::clear() { items_.clear(); }

// Returns the number of Items in this Inventory.
size_t Inventory::count() const { return items_.size(); }

// Deletes an Item from this Inventory.
void Inventory::erase(size_t pos)
{
    if (pos >= items_.size()) throw std::runtime_error("Invalid inventory position requested.");
    items_.erase(items_.begin() + pos);
}

// Retrieves an Item from this Inventory.
std::shared_ptr<Item> Inventory::get(size_t pos) const
{
    if (pos >= items_.size()) throw std::runtime_error("Invalid inventory position requested.");
    return items_.at(pos);
}

// As above, but retrieves an item based on a given equipment slot.
std::shared_ptr<Item> Inventory::get(EquipSlot es) const
{
    for (auto item : items_)
        if (item->equip_slot() == es) return item;
    return nullptr;
}

// Loads an Inventory from the save file.
void Inventory::load(std::shared_ptr<SQLite::Database> save_db, uint32_t sql_id)
{
    items_.clear();
    SQLite::Statement query(*save_db, "SELECT sql_id FROM items WHERE owner_id = :owner_id ORDER BY sql_id ASC");
    query.bind(":owner_id", sql_id);
    bool loaded_items = false;
    while (query.executeStep())
    {
        auto new_item = Item::load(save_db, query.getColumn("sql_id").getUInt());
        items_.push_back(new_item);
        loaded_items = true;
    }
    if (!loaded_items) throw std::runtime_error("Could not load inventory data " + std::to_string(sql_id));
}

// Checks if a given parser ID already exists on an Item in this Inventory.
bool Inventory::parser_id_exists(uint16_t id) const
{
    for (auto item : items_)
        if (item->parser_id() == id) return true;
    return false;
}

// Removes an Item from this Inventory.
void Inventory::remove_item(size_t pos)
{
    if (pos >= items_.size()) throw std::runtime_error("Attempt to remove item with invalid inventory position.");
    items_.erase(items_.begin() + pos);
}

// As above, but with a specified equipment slot.
void Inventory::remove_item(EquipSlot es)
{
    for (size_t i = 0; i < items_.size(); i++)
    {
        if (items_.at(i)->equip_slot() == es)
        {
            remove_item(i);
            return;
        }
    }
    core()->guru()->nonfatal("Attempt to remove empty equipment slot item.", Guru::GURU_ERROR);
}

// Saves this Inventory, returns its SQL ID.
uint32_t Inventory::save(std::shared_ptr<SQLite::Database> save_db)
{
    if (!items_.size()) return 0;
    const uint32_t sql_id = core()->sql_unique_id();
    for (size_t i = 0; i < items_.size(); i++)
        items_.at(i)->save(save_db, sql_id);
    return sql_id;
}

// Sets the parser ID prefix.
void Inventory::set_prefix(uint8_t prefix)
{
    pid_prefix_ = prefix;
    for (auto item : items_)
        update_prefix(item);
}

// Sorts the inventory into alphabetical order.
void Inventory::sort()
{
    // Quick and dirty bubble sort. It does the job.
    bool sorted = false;
    do
    {
        sorted = false;
        for (size_t i = 0; i < items_.size() - 1; i++)
        {
            const size_t j = i + 1;
            if (items_.at(i)->name(Item::NAME_FLAG_NO_COLOUR | Item::NAME_FLAG_NO_COUNT) > items_.at(j)->name(Item::NAME_FLAG_NO_COLOUR | Item::NAME_FLAG_NO_COUNT))
            {
                std::iter_swap(items_.begin() + i, items_.begin() + j);
                sorted = true;
            }
        }
    } while (sorted);
}

// Returns the weight of all items in this inventory.
uint32_t Inventory::weight() const
{
    uint32_t total_weight = 0;
    for (auto item : items_)
        total_weight += item->weight();
    return total_weight;
}
