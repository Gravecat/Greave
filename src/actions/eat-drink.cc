// actions/eat-drink.cc -- Eating food and drinking beverages.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#include "actions/eat-drink.h"
#include "actions/potions.h"
#include "core/core.h"


// Drinks a specified inventory item.
void ActionEatDrink::drink(size_t inv_pos, bool confirm)
{
    const auto player = core()->world()->player();
    const auto item = player->inv()->get(inv_pos);
    const int liquid_available = item->charge();
    if (item->type() != ItemType::DRINK)
    {
        if (item->type() == ItemType::POTION) ActionPotions::drink(inv_pos, confirm);
        else core()->message("{u}That isn't something you can drink!");
        return;
    }
    if (!liquid_available)
    {
        core()->message("{u}Your " + item->name(Item::NAME_FLAG_NO_COUNT) + " {u}is empty!");
        return;
    }

    const int thirst = player->thirst();
    const int water_space_left_in_body = std::max(1, 20 - thirst);
    const int liquid_consumed = std::min(water_space_left_in_body, liquid_available);
    if (thirst >= 20 && !confirm)
    {
        core()->message("{u}But you're not at all thirsty! Are you sure you want to drink anyway?");
        core()->parser()->confirm_message();
        return;
    }

    if (!player->pass_time(item->speed(), !confirm))
    {
        core()->parser()->interrupted("drink");
        return;
    }
    if (player->is_dead()) return;

    std::string water_str;
    if (liquid_consumed == liquid_available) water_str = "{U}You drink the last of the ";
    else water_str = "{U}You drink some ";
    water_str += "{C}" + item->liquid_type() + " {U}from your " + item->name(Item::NAME_FLAG_NO_COUNT) + "{U}";
    if (thirst + liquid_consumed <= 14) water_str += ", but you're still thirsty...";
    else if (thirst <= 14 && thirst + liquid_consumed > 14) water_str += ", quenching your thirst.";
    else water_str += ".";
    core()->message(water_str);
    player->add_water(liquid_consumed);
    if (item->subtype() == ItemSub::BOOZE) player->increase_tox(item->power());
    item->set_charge(liquid_available - liquid_consumed);
    if (liquid_consumed == liquid_available)
    {
        item->set_liquid("");
        if (item->tag(ItemTag::DiscardWhenEmpty))
        {
            core()->message("{u}You discard the empty " + item->name(Item::NAME_FLAG_NO_COLOUR | Item::NAME_FLAG_NO_COUNT) + ".");
            player->inv()->erase(inv_pos);
        }
    }
}

// Eats a specified inventory item.
void ActionEatDrink::eat(size_t inv_pos, bool confirm)
{
    const auto player = core()->world()->player();
    const auto item = player->inv()->get(inv_pos);
    if (item->type() != ItemType::FOOD)
    {
        core()->message("{y}That isn't edible!");
        return;
    }

    const int old_hunger = player->hunger();
    const int new_hunger = old_hunger + item->power();
    if (new_hunger > 20 && !confirm)
    {
        core()->message("{y}You're not really hungry enough to fit all that in. Are you sure you want to force it?");
        core()->parser()->confirm_message();
        return;
    }

    if (!player->pass_time(item->speed(), !confirm))
    {
        core()->parser()->interrupted("eat");
        return;
    }
    if (player->is_dead()) return;

    const bool last_item = (!item->tag(ItemTag::Stackable) || item->stack() == 1);
    std::string eat_str;
    if (old_hunger <= 4) eat_str = "{U}You wolf down ";
    else if (old_hunger <= 8) eat_str = "{U}You devour ";
    else if (new_hunger > 20) eat_str = "{U}You force yourself to eat ";
    else eat_str = "{U}You eat ";
    if (last_item) eat_str += "the last of your " + item->name(Item::NAME_FLAG_PLURAL | Item::NAME_FLAG_NO_COUNT);
    else eat_str  += item->name(Item::NAME_FLAG_THE | Item::NAME_FLAG_NO_COUNT);
    if (old_hunger <= 12 && new_hunger > 14) eat_str += "{U}. {G}That hit the spot!";
    else if (new_hunger <= 12) eat_str += "{U}, but you're {c}still hungry{U}...";
    else if (new_hunger >= 25) eat_str += "{U}, feeling {c}extremely full and bloated{U}.";
    else if (new_hunger > 20) eat_str += "{U}, feeling {c}extremely full{U}.";
    else eat_str += "{U}, feeling satiated.";
    core()->message(eat_str);

    player->add_food(item->power());
    if (last_item) player->inv()->remove_item(inv_pos);
    else item->set_stack(item->stack() - 1);

    if ((new_hunger >= 25 && core()->rng()->rnd(VOMIT_CHANCE_BLOAT_MAJOR) == 1) || (new_hunger > 20 && new_hunger < 25 && core()->rng()->rnd(VOMIT_CHANCE_BLOAT_MINOR) == 1)) vomit(true);
}

// Empties a liquid container.
void ActionEatDrink::empty(size_t inv_pos, bool confirm)
{
    const auto player = core()->world()->player();
    const auto item = player->inv()->get(inv_pos);

    if (item->type() != ItemType::DRINK)
    {
        core()->message("{u}That isn't something you can empty.");
        return;
    }
    if (!item->charge())
    {
        core()->message("{u}There's nothing left to empty.");
        return;
    }

    if (!player->pass_time(TIME_EMPTY_CONTAINER, !confirm))
    {
        core()->parser()->interrupted("empty " + item->name(Item::NAME_FLAG_NO_COUNT | Item::NAME_FLAG_NO_COLOUR | Item::NAME_FLAG_THE));
        return;
    }
    if (player->is_dead()) return;

    core()->world()->get_room(player->location())->add_scar(Room::ScarType::WATER, item->charge());
    core()->message("{U}You empty out all the " + item->liquid_type() + " from " + item->name(Item::NAME_FLAG_NO_COUNT | Item::NAME_FLAG_NO_COLOUR | Item::NAME_FLAG_THE) + ".");

    if (item->tag(ItemTag::DiscardWhenEmpty)) player->inv()->erase(inv_pos);
    else
    {
        item->set_charge(0);
        item->clear_meta("liquid");
    }
}

// Fills a liquid container.
void ActionEatDrink::fill(size_t inv_pos, bool confirm)
{
    const auto player = core()->world()->player();
    const auto item = player->inv()->get(inv_pos);
    const auto room = core()->world()->get_room(player->location());

    if (item->type() != ItemType::DRINK || item->subtype() != ItemSub::WATER_CONTAINER)
    {
        core()->message("{u}That isn't something you can fill.");
        return;
    }
    if (!room->tag(RoomTag::WaterClean))
    {
        core()->message("{u}There isn't a source of water here.");
        return;
    }

    if (!player->pass_time(TIME_FILL_CONTAINER, !confirm))
    {
        core()->parser()->interrupted("fill " + item->name(Item::NAME_FLAG_NO_COUNT | Item::NAME_FLAG_NO_COLOUR | Item::NAME_FLAG_THE));
        return;
    }
    if (player->is_dead()) return;

    item->set_charge(item->capacity());
    if (!item->liquid_type().size()) item->set_liquid("water");
    core()->message("{U}You fill " + item->name(Item::NAME_FLAG_NO_COUNT | Item::NAME_FLAG_NO_COLOUR | Item::NAME_FLAG_THE) + " with water.");
}

// Loses the contents of your stomach.
void ActionEatDrink::vomit(bool confirm)
{
    if (!confirm)
    {
        core()->message("{g}Forcing yourself to vomit would probably be bad for your health.");
        core()->parser()->confirm_message();
        return;
    }
    core()->message("{g}You retch violently, {G}vomiting {g}all over the floor!");

    auto player = core()->world()->player();
    int food_loss = core()->rng()->rnd(VOMIT_FOOD_LOSS_MAX);
    int water_loss = core()->rng()->rnd(VOMIT_WATER_LOSS_MAX);
    if (player->hunger() > 20) food_loss += player->hunger() - 20;
    if (player->hunger() - food_loss < VOMIT_MINIMUM_FOOD_REMAINING) food_loss = player->hunger() - VOMIT_MINIMUM_FOOD_REMAINING;
    if (player->thirst() - water_loss < VOMIT_MINIMUM_WATER_REMAINING) water_loss = player->thirst() - VOMIT_MINIMUM_WATER_REMAINING;
    if (food_loss > 0) player->add_food(-food_loss);
    if (water_loss > 0) player->add_water(-water_loss);
    core()->world()->get_room(player->location())->add_scar(Room::ScarType::VOMIT, VOMIT_SCAR_INTENSITY);
}
