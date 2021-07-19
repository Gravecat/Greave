// actions/inventory.c -- Actions related to inventory management, picking up and dropping items, etc.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#include "actions/inventory.h"
#include "core/core.h"
#include "core/strx.h"


// Checks to see what's being carried.
void ActionInventory::check_inventory()
{
    const auto player = core()->world()->player();
    const auto inventory = player->inv();
    const size_t inv_size = inventory->count();

    if (inv_size)
    {
        core()->message("{G}You are carrying:");
        for (size_t i = 0; i < inv_size; i++)
            core()->message("{0}" + inventory->get(i)->name(Item::NAME_FLAG_FULL_STATS | Item::NAME_FLAG_ID));
    }
    else core()->message("{y}You are not carrying anything.");

    weight_and_money();
}

// Checks that a player-input count is a valid number.
bool ActionInventory::count_check(std::shared_ptr<Item> item, int count)
{
    if (count == -1) return true;
    if (count < 1)
    {
        core()->message("{y}That isn't a valid number.");
        return false;
    }
    if (!item->tag(ItemTag::Stackable))
    {
        if (count != 1)
        {
            core()->message("{y}" + item->name(Item::NAME_FLAG_THE | Item::NAME_FLAG_CAPITALIZE_FIRST) + " {y}isn't something you can stack!");
            return false;
        }
        return true;
    }
    if (item->stack() < static_cast<unsigned int>(count))
    {
        core()->message("{y}There aren't that many " + item->name(Item::NAME_FLAG_PLURAL | Item::NAME_FLAG_NO_COUNT) + "{y}.");
        return false;
    }
    return true;
}

// Drops an item on the ground.
void ActionInventory::drop(std::shared_ptr<Mobile> mob, size_t item_pos, int count, bool confirm)
{
    const std::shared_ptr<Item> item = mob->inv()->get(item_pos);
    const std::shared_ptr<Room> room = core()->world()->get_room(mob->location());
    const bool stackable = item->tag(ItemTag::Stackable);
    if (!count_check(item, count)) return;

    if (!mob->pass_time(TIME_DROP_ITEM, !confirm))
    {
        core()->parser()->interrupted("drop " + item->stack_name(count, Item::NAME_FLAG_THE));
        return;
    }
    if (mob->is_dead()) return;

    const bool drop_all = (!stackable) || (count == -1 || count == static_cast<int>(item->stack()));
    if (drop_all)
    {
        mob->inv()->erase(item_pos);
        room->inv()->add_item(item);
    }
    else
    {
        auto split = item->split(count);
        room->inv()->add_item(split);
    }
    if (mob->is_player()) core()->message("{u}You drop " + item->stack_name(count, Item::NAME_FLAG_THE) + " {u}on the ground.");
    // todo: add message for NPCs dropping items
}

// Wields or wears an equippable item.
bool ActionInventory::equip(std::shared_ptr<Mobile> mob, size_t item_pos, bool confirm)
{
    const auto inv = mob->inv();
    const auto equ = mob->equ();
    const auto item = inv->get(item_pos);
    EquipSlot slot = item->equip_slot();
    const bool main_used = (equ->get(EquipSlot::HAND_MAIN) != nullptr);
    const bool off_used = (equ->get(EquipSlot::HAND_OFF) != nullptr);
    const bool two_handed_equipped = main_used && equ->get(EquipSlot::HAND_MAIN)->tag(ItemTag::TwoHanded);
    const bool two_handed_item = item->tag(ItemTag::TwoHanded);
    const bool prefer_off_hand = item->tag(ItemTag::PreferOffHand);
    const bool off_hand_only = item->tag(ItemTag::OffHandOnly);

    // Determine which slot to use for held items, since they may go in either hand.
    if (slot == EquipSlot::HAND_MAIN || slot == EquipSlot::HAND_OFF)
    {
        // Two-handed items require both hands to be empty.
        if (two_handed_item)
        {
            // If anything fails on unequipping, just bow out here.
            if ((main_used && !unequip(mob, EquipSlot::HAND_MAIN, confirm)) || (off_used && !unequip(mob, EquipSlot::HAND_OFF, confirm))) return false;
            slot = EquipSlot::HAND_MAIN;
        }
        // Normal one-handed equipping.
        else if (!prefer_off_hand && !off_hand_only)
        {
            // First, check if the main hand is free.
            if (!main_used) slot = EquipSlot::HAND_MAIN;

            // Failing that, check the off-hand is free *and* that the main-hand isn't two-handed.
            else if (!two_handed_equipped && !off_used) slot = EquipSlot::HAND_OFF;

            // Okay, both hands are used. First, try to unequip the main hand.
            else if (unequip(mob, EquipSlot::HAND_MAIN, confirm)) slot = EquipSlot::HAND_MAIN;

            // No luck? Okay, try to unequip the off-hand, if we're not using a two-hander.
            else if (!two_handed_equipped && unequip(mob, EquipSlot::HAND_OFF, confirm)) slot = EquipSlot::HAND_OFF;

            // Just give up if we can't even do that.
            else return false;
        }
        // Prefer off-hand is the same rules as above, just flipped.
        else
        {
            if (!two_handed_equipped && !off_used) slot = EquipSlot::HAND_OFF;
            else if (!main_used && !off_hand_only) slot = EquipSlot::HAND_MAIN;
            else if (!two_handed_equipped && unequip(mob, EquipSlot::HAND_OFF, confirm)) slot = EquipSlot::HAND_OFF;
            else if (!off_hand_only && unequip(mob, EquipSlot::HAND_MAIN, confirm)) slot = EquipSlot::HAND_MAIN;
            else return false;
        }

        // Update the Item's equipment slot.
        item->set_equip_slot(slot);
    }

    // Extra check for layered armour.
    if (slot == EquipSlot::BODY && equ->get(EquipSlot::ARMOUR))
    {
        if (mob->is_player())
        {
            core()->message("{y}You'll need to remove your {Y}" + equ->get(EquipSlot::ARMOUR)->name() + " {y}first.");
            return false;
        }
        // For NPCs, just unequip the outer armour first.
        if (!unequip(mob, EquipSlot::ARMOUR, confirm)) return false;
    }

    // Attempt to unequip the item currently using the slot. If it's not possible (cursed?), just stop here.
    if (equ->get(slot) != nullptr && !unequip(mob, slot, confirm)) return false;

    std::string action = "wear", slot_name;
    float time_taken = 0;
    switch (slot)
    {
        case EquipSlot::NONE: case EquipSlot::_END: throw std::runtime_error("Attempt to equip item into null slot.");
        case EquipSlot::ABOUT_BODY:
            slot_name = "about %your% body";
            time_taken = TIME_EQUIP_ABOUT;
            break;
        case EquipSlot::ARMOUR:
            slot_name = "over %your% body";
            time_taken = TIME_EQUIP_ARMOUR;
            break;
        case EquipSlot::BODY:
            slot_name = "on %your% body";
            time_taken = TIME_EQUIP_BODY;
            break;
        case EquipSlot::FEET:
            slot_name = "on %your% feet";
            time_taken = TIME_EQUIP_FEET;
            break;
        case EquipSlot::HAND_MAIN:
            if (two_handed_item || (item->tag(ItemTag::HandAndAHalf) && !mob->equ()->get(EquipSlot::HAND_OFF))) slot_name = "in both hands";
            else slot_name = "in %your% main hand";
            action = "hold";
            time_taken = TIME_EQUIP_WEAPON;
            break;
        case EquipSlot::HAND_OFF:
            slot_name = "in %your% off-hand";
            action = "hold";
            time_taken = TIME_EQUIP_WEAPON;
            break;
        case EquipSlot::HANDS:
            slot_name = "on %your% hands";
            time_taken = TIME_EQUIP_HANDS;
            break;
        case EquipSlot::HEAD:
            slot_name = "on %your% head";
            time_taken = TIME_EQUIP_HEAD;
            break;
    }
    if (!time_taken) core()->guru()->nonfatal("Could not determine equip time for " + item->name(), Guru::GURU_ERROR);
    // todo: messages for NPCs equipping gear
    StrX::find_and_replace(slot_name, "%your%", "your");

    if (!mob->pass_time(time_taken, !confirm))
    {
        core()->parser()->interrupted(action + " " + item->name(Item::NAME_FLAG_THE));
        return false;
    }
    if (mob->is_dead()) return false;
    core()->message("{U}You " + action + " " + item->name(Item::NAME_FLAG_THE) + " {U}" + slot_name + ".");

    const auto room = core()->world()->get_room(mob->location());
    equ->add_item(item);
    inv->remove_item(item_pos);

    return true;
}

// Checks to see what's wielded and/or worn.
void ActionInventory::equipment()
{
    const auto player = core()->world()->player();
    const auto equ = player->equ();
    if (!equ->count())
    {
        core()->message("{y}You aren't {Y}wearing or wielding {y}anything.");
        return;
    }

    core()->message("{G}Your equipment:");
    for (int i = 1; i < static_cast<int>(EquipSlot::_END); i++)
    {
        const EquipSlot es = static_cast<EquipSlot>(i);
        const auto item = equ->get(es);
        if (!item) continue;
        std::string slot_name;
        switch (es)
        {
            case EquipSlot::NONE: case EquipSlot::_END: break;
            case EquipSlot::ABOUT_BODY: slot_name = "about body"; break;
            case EquipSlot::ARMOUR: slot_name = "over body"; break;
            case EquipSlot::BODY: slot_name = "on body"; break;
            case EquipSlot::FEET: slot_name = "on feet"; break;
            case EquipSlot::HAND_MAIN:
                if (item->tag(ItemTag::TwoHanded) || (item->tag(ItemTag::HandAndAHalf) && !equ->get(EquipSlot::HAND_OFF))) slot_name = "in both hands";
                else slot_name = "in main hand";
                break;
            case EquipSlot::HAND_OFF:
                if (item->tag(ItemTag::HandAndAHalf) && !equ->get(EquipSlot::HAND_MAIN)) slot_name = "in both hands";
                else slot_name = "in off-hand";
                break;
            case EquipSlot::HANDS: slot_name = "on hands"; break;
            case EquipSlot::HEAD: slot_name = "on head"; break;
        }
        core()->message("{0}" + item->name(Item::NAME_FLAG_FULL_STATS | Item::NAME_FLAG_ID) + " {B}(" + slot_name + ")");
    }

    weight_and_money();
}

// Takes an item from the ground.
void ActionInventory::take(std::shared_ptr<Mobile> mob, ParserTarget target_type, size_t item_pos, size_t item_pos_second, int count, bool confirm)
{
    const std::shared_ptr<Room> room = core()->world()->get_room(mob->location());
    const bool container = (target_type == ParserTarget::TARGET_CONTAINER_INVENTORY || target_type == ParserTarget::TARGET_CONTAINER_ROOM);
    std::shared_ptr<Item> container_item = nullptr;
    if (target_type == ParserTarget::TARGET_CONTAINER_ROOM) container_item = room->inv()->get(item_pos_second);
    else if (target_type == ParserTarget::TARGET_CONTAINER_INVENTORY) container_item = core()->world()->player()->inv()->get(item_pos_second);
    const std::shared_ptr<Inventory> inv = (container ? container_item->inv() : room->inv());

    std::shared_ptr<Item> item = inv->get(item_pos);
    const bool stackable = item->tag(ItemTag::Stackable);
    if (!count_check(item, count)) return;
    std::string take_str = (container ? "take " : "pick up ");
    std::string from_str = (container ? " {u}from " + container_item->name(Item::NAME_FLAG_THE) + "{u}." : "{u}.");

    int weight = item->weight(true);
    if (stackable)
    {
        if (count == -1) weight = item->weight(false);
        else weight *= count;
    }
    if (mob->carry_weight() + weight > mob->max_carry())
    {
        core()->message("{y}You can't carry that much!");
        return;
    }

    if (!mob->pass_time(TIME_GET_ITEM, !confirm))
    {
        core()->parser()->interrupted(take_str + item->stack_name(count, Item::NAME_FLAG_THE));
        return;
    }
    if (mob->is_dead()) return;

    const bool get_all = (!stackable) || (count == -1 || count == static_cast<int>(item->stack()));
    if (get_all)
    {
        inv->erase(item_pos);
        mob->inv()->add_item(item);
        if (container && !inv->count()) container_item->assign_inventory(nullptr);
    }
    else
    {
        auto split = item->split(count);
        mob->inv()->add_item(split);
    }
    if (mob->is_player()) core()->message("{u}You " + take_str + item->stack_name(count, Item::NAME_FLAG_THE) + from_str);
    // todo: add message for NPCs taking items
}

// Unequips a worn or wielded item.
bool ActionInventory::unequip(std::shared_ptr<Mobile> mob, size_t item_pos, bool confirm)
{
    const auto equ = mob->equ();
    const auto inv = mob->inv();
    if (item_pos >= equ->count()) throw std::runtime_error("Invalid equipment vector position.");
    const auto item = equ->get(item_pos);
    const EquipSlot slot = item->equip_slot();

    if (slot == EquipSlot::BODY && equ->get(EquipSlot::ARMOUR))
    {
        if (mob->is_player())
        {
            core()->message("{y}You'll need to remove your {Y}" + equ->get(EquipSlot::ARMOUR)->name() + " {y}first.");
            return false;
        }
        // For NPCs, just unequip the outer armour first.
        if (!unequip(mob, EquipSlot::ARMOUR, confirm)) return false;
    }

    // todo: add message for NPCs unequipping items
    std::string action = "remove";
    if (slot == EquipSlot::HAND_MAIN || slot == EquipSlot::HAND_OFF) action = "stop holding";
    float time_taken = 0;
    switch (slot)
    {
        case EquipSlot::NONE: case EquipSlot::_END: throw std::runtime_error("Invalid equipment slot.");
        case EquipSlot::ABOUT_BODY: time_taken = TIME_UNEQUIP_ABOUT; break;
        case EquipSlot::ARMOUR: time_taken = TIME_UNEQUIP_ARMOUR; break;
        case EquipSlot::BODY: time_taken = TIME_UNEQUIP_BODY; break;
        case EquipSlot::FEET: time_taken = TIME_UNEQUIP_FEET; break;
        case EquipSlot::HAND_MAIN: case EquipSlot::HAND_OFF: time_taken = TIME_UNEQUIP_WEAPON; break;
        case EquipSlot::HANDS: time_taken = TIME_UNEQUIP_HANDS; break;
        case EquipSlot::HEAD: time_taken = TIME_UNEQUIP_HEAD; break;
    }
    if (!time_taken) core()->guru()->nonfatal("Could not determine unequip time for " + item->name(), Guru::GURU_ERROR);
    if (!mob->pass_time(time_taken, !confirm))
    {
        core()->parser()->interrupted(action + " " + item->name(Item::NAME_FLAG_THE));
        return false;
    }
    if (mob->is_dead()) return false;

    core()->message("{U}You " + action + " your " + item->name() + "{U}.");
    equ->remove_item(item_pos);
    inv->add_item(item);
    return true;
}

// As above, but specifying an EquipSlot.
bool ActionInventory::unequip(std::shared_ptr<Mobile> mob, EquipSlot slot, bool confirm)
{
    const auto inv = mob->equ();
    for (size_t i = 0; i < inv->count(); i++)
        if (inv->get(i)->equip_slot() == slot) return unequip(mob, i, confirm);
    return false;
}

// Shows the total carry weight and currency the Mobile is carrying.
void ActionInventory::weight_and_money()
{
    const auto player = core()->world()->player();
    if (player->carry_weight() || player->inv()->count())
        core()->message("{0}{c}Total weight: {C}" + StrX::intostr_pretty(player->carry_weight()) + "{c}/{C}" + StrX::intostr_pretty(player->max_carry()) + " {c}pacs.");

    if (!player->money()) return;
    core()->message("{0}{c}Money carried: " + StrX::mgsc_string(player->money(), StrX::MGSC::SHORT) + "{c}.");
}
