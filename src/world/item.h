// world/item.h -- The Item class is for objects that can be picked up and used by the player or other NPCs.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#ifndef GREAVE_WORLD_ITEM_H_
#define GREAVE_WORLD_ITEM_H_

#include "3rdparty/SQLiteCpp/Database.h"

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>

class Inventory;    // Forward declarations are bad, I know, but this is the only way to avoid item.h and inventory.h trying to include each other.


// Weapon damage types.
enum class DamageType : int8_t { ACID, BALLISTIC, CRUSHING, EDGED, ENERGY, EXPLOSIVE, KINETIC, PIERCING, PLASMA, POISON, RENDING, NONE = -1 };

// The slot that an item is equipped in.
enum class EquipSlot : uint8_t { NONE, HAND_MAIN, HAND_OFF, BODY, ARMOUR, ABOUT_BODY, HEAD, HANDS, FEET, _END };

// ItemType is the primary type of Item (e.g. weapon, food, etc.)
enum class ItemType : uint16_t { NONE, AMMO, ARMOUR, CONTAINER, DRINK, FOOD, KEY, LIGHT, SHIELD, WEAPON };

// ItemSub is for sub-types of items, e.g. a tool could sub-classify itself here.
enum class ItemSub : uint16_t { NONE,
    ARROW, BOLT,                    // AMMO subtypes.
    CLOTHING, HEAVY, LIGHT, MEDIUM, // ARMOUR subtypes.
    CORPSE,                         // CONTAINER subtypes.
    BOOZE, WATER_CONTAINER,         // DRINK subtypes.
    MELEE, RANGED, UNARMED,         // WEAPON subtypes.
    };

enum class ItemTag : uint16_t { _None = 0,  // Do not use this tag, it's just a marker to start the tags below counting from 1.

    // Basic, generic, core details about an item.
    Stackable,          // Can this Item be stacked with others identical to itself?

    // Tags regarding the item's name.
    NoA,                // This Item's name should not be prefaced with 'a' (e.g. 'plate mail armour' instead of 'a plate mail armour').
    PluralName,         // This Item's name is a plural (e.g. "boots").
    ProperNoun,         // This Item's name is a proper noun (e.g. Foehammer).

    // Tags specific to weapons, armour and other equipment.
    AmmoArrow,          // Either this is a weapon that uses arrows, or it's a stack of arrows.
    AmmoBolt,           // Either this is a weapon that uses bolts, or it's a stack of bolts.
    HandAndAHalf,       // Hand-and-a-half weapons can be wielded in either one or both hands.
    NoAmmo,             // This is a ranged weapon that does not require ammo.
    NoLoot,             // This item cannot be looted from corpses.
    OffHandOnly,        // This item can ONLY be equipped in the off-hand.
    PreferOffHand,      // When equipped, this Item prefers to be held in the off-hand.
    TwoHanded,          // This Item requires two hands to wield.

    // Tags specific to consumable items.
    DiscardWhenEmpty,   // Throw this item away automatically when it's empty.
    TavernOnly,         // This item will have to be left behind if you leave a tavern.
};

class Item
{
public:
    // Flags for the name() function.
    static constexpr int    NAME_FLAG_A =                   (1 << 0);   // Much like NAME_FLAG_THE, but using 'a' or 'an' instead of 'the'.
    static constexpr int    NAME_FLAG_CAPITALIZE_FIRST =    (1 << 1);   // Capitalize the first letter of the Item's name (including the "The") if set.
    static constexpr int    NAME_FLAG_CORE_STATS =          (1 << 2);   // Displays core stats on Item names, such as if an Item is glowing.
    static constexpr int    NAME_FLAG_ID =                  (1 << 3);   // Displays an item's ID number, such as {1234}.
    static constexpr int    NAME_FLAG_FULL_STATS =          (1 << 4);   // Displays some basic stats next to an item's name.
    static constexpr int    NAME_FLAG_NO_COLOUR =           (1 << 5);   // Strips colour out of an Item's name.
    static constexpr int    NAME_FLAG_NO_COUNT =            (1 << 6);   // Ignore the stack size on this item.
    static constexpr int    NAME_FLAG_PLURAL =              (1 << 7);   // Return a plural of the Item's name (e.g. apple -> apples).
    static constexpr int    NAME_FLAG_RARE =                (1 << 8);   // Include the Item's rarity value in its name.
    static constexpr int    NAME_FLAG_THE =                 (1 << 9);   // Precede the Item's name with 'the', unless the name is a proper noun.

    static constexpr float  WATER_WEIGHT =                  58.68f;     // The weight of 1 unit of water.
    static const char       SQL_ITEMS[];                                // The SQL table construction string for saving items.

                Item();                                     // Constructor, sets default values.
    float       ammo_power() const;                         // The damage multiplier for ammunition.
    int         appraised_value();                          // Attempts to guess the value of an item.
    float       armour(int bonus_power = 0) const;          // Returns the armour damage reduction value of this Item, if any.
    void        assign_inventory(std::shared_ptr<Inventory> inventory); // Assigns another inventory to this item. Use with caution.
    int         bleed() const;                              // Returns thie bleed chance of this Item, if any.
    int         block_mod() const;                          // Returns the block modifier% for this Item, if any.
    int         capacity() const;                           // Returns this Item's capacity, if any.
    int         charge() const;                             // Returns this Item's charge, if any.
    void        clear_meta(const std::string &key);         // Clears a metatag from an Item. Use with caution!
    void        clear_tag(ItemTag the_tag);                 // Clears a tag on this Item.
    int         crit() const;                               // Retrieves this Item's critical power, if any.
    DamageType  damage_type() const;                        // Retrieves this Item's damage type, if any.
    std::string damage_type_string() const;                 // Returns a string indicator of this Item's damage type (e.g. edged = E)
    std::string desc() const;                               // Retrieves this Item's description.
    int         dodge_mod() const;                          // Returns the dodge modifier% for this Item, if any.
    EquipSlot   equip_slot() const;                         // Checks what slot this Item equips in, if any.
    const std::shared_ptr<Inventory> inv();                 // The inventory of this item, or nullptr if none exists.
    bool        is_identical(std::shared_ptr<Item> item) const; // Checks if this Item is identical to another (except stack size).
    std::string liquid_type() const;                        // Returns the liquid type contained in this Item, if any.
    static std::shared_ptr<Item> load(std::shared_ptr<SQLite::Database> save_db, uint32_t sql_id);  // Loads a new Item from the save file.
    std::string meta(const std::string &key) const;         // Retrieves Item metadata.
    float       meta_float(const std::string &key) const;   // Retrieves metadata, in float format.
    int         meta_int(const std::string &key) const;     // Retrieves metadata, in int format.
    std::map<std::string, std::string>* meta_raw();         // Accesses the metadata map directly. Use with caution!
    std::string name(int flags = 0) const;                  // Retrieves the name of thie Item.
    void        new_inventory();                            // Creates an inventory for this item.
    void        new_parser_id(uint8_t prefix);              // Generates a new parser ID for this Item.
    int         parry_mod() const;                          // Returns the parry% modifier of this Item, if any.
    uint16_t    parser_id() const;                          // Retrieves the current ID of this Item, for parser differentiation.
    int         poison() const;                             // Returns the poison chance of this item, if any.
    int         power() const;                              // Retrieves this Item's power.
    int         rare() const;                               // Retrieves this Item's rarity.
    void        save(std::shared_ptr<SQLite::Database> save_db, uint32_t owner_id); // Saves the Item to the save file.
    void        set_charge(int new_charge);                 // Sets the charge level of this Item.
    void        set_description(const std::string &desc);   // Sets this Item's description.
    void        set_equip_slot(EquipSlot es);               // Sets this Item's equipment slot.
    void        set_liquid(const std::string &new_liquid);  // Sets the liquid contents of this Item.
    void        set_meta(const std::string &key, std::string value);    // Adds Item metadata.
    void        set_meta(const std::string &key, int value);            // As above, but with an integer value.
    void        set_meta(const std::string &key, uint32_t value);       // As above, but with an unsigned integer value.
    void        set_meta(const std::string &key, float value);          // As above again, but this time for floats.
    void        set_name(const std::string &name);          // Sets the name of this Item.
    void        set_parser_id_prefix(uint8_t prefix);       // Sets this item's parser ID prefix.
    void        set_rare(int rarity);                       // Sets this Item's rarity.
    void        set_stack(uint32_t size);                   // Sets the stack size for this Item.
    void        set_tag(ItemTag the_tag);                   // Sets a tag on this Item.
    void        set_type(ItemType type, ItemSub sub = ItemSub::NONE);   // Sets the type of this Item.
    void        set_value(uint32_t val);                    // Sets this Item's value.
    void        set_weight(uint32_t pacs);                  // Sets this Item's weight.
    float       speed() const;                              // Retrieves the speed of this Item.
    std::shared_ptr<Item>    split(int split_count);        // Splits an Item into a stack.
    uint32_t    stack() const;                              // Retrieves the stack size of this Item.
    std::string stack_name(int stack_size, int flags = 0);  // Like name(), but provides an appropriate name for a given stack size. Works on non-stackable items too.
    ItemSub     subtype() const;                            // Returns the ItemSub (sub-type) of this Item.
    bool        tag(ItemTag the_tag) const;                 // Checks if a tag is set on this Item.
    ItemType    type() const;                               // Returns the ItemType of this Item.
    uint32_t    value(bool individual = false) const;       // The Item's value in money.
    int         warmth() const;                             // The Item's warmth rating, if any.
    uint32_t    weight(bool individual = false) const;      // The Item's weight, in pacs.

private:
    static constexpr int    APPRAISAL_BASE_SKILL_REQUIRED = -11;    // The base modifier to appraisal skill required for an item, after taking rarity into account.
    static constexpr int    APPRAISAL_RARITY_MULTIPLIER =   9;      // The multiplier to the appraisal skill required for an item, per rarity level.
    static constexpr int    APPRAISAL_XP_EASY =             1;      // The amount of appraisal XP gained for an easy item appraisal.
    static constexpr int    APPRAISAL_XP_HARD =             5;      // The amount of appraisal XP gained for a difficult item appraisal.

    std::string                         description_;   // The description of this Item.
    std::shared_ptr<Inventory>          inventory_;     // The contents of this item, if any.
    std::map<std::string, std::string>  metadata_;      // The Item's metadata, if any.
    std::string                         name_;          // The name of this Item!
    uint16_t                            parser_id_;     // The semi-unique ID of this Item, for parser differentiation.
    uint8_t                             rarity_;        // The rarity of this Item.
    uint32_t                            stack_;         // If this Item can be stacked, this is how many is in the stack.
    std::set<ItemTag>                   tags_;          // Any and all ItemTags on this Item.
    ItemType                            type_;          // The primary type of this Item.
    ItemSub                             type_sub_;      // The subtype of this Item, if any.
    uint32_t                            value_;         // The value of this Item, if any.
    uint32_t                            weight_;        // The weight of this Item.
};

#endif  // GREAVE_WORLD_ITEM_H_
