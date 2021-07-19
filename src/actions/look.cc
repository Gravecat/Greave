// actions/look.cc -- Look around you. Just look around you.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#include "actions/look.h"
#include "core/core.h"
#include "core/mathx.h"
#include "core/strx.h"


// Examines an Item or Mobile.
void ActionLook::examine(ParserTarget target_type, size_t target, size_t target_second)
{
    const auto world = core()->world();
    const auto player = world->player();
    switch (target_type)
    {
        case ParserTarget::TARGET_CONTAINER_INVENTORY: examine_item(player->inv()->get(target_second)->inv()->get(target)); break;
        case ParserTarget::TARGET_CONTAINER_ROOM: examine_item(world->get_room(player->location())->inv()->get(target_second)->inv()->get(target)); break;
        case ParserTarget::TARGET_EQUIPMENT: examine_item(player->equ()->get(target)); break;
        case ParserTarget::TARGET_INVENTORY: examine_item(player->inv()->get(target)); break;
        case ParserTarget::TARGET_MOBILE: examine_mobile(world->mob_vec(target)); break;
        case ParserTarget::TARGET_ROOM: examine_item(world->get_room(player->location())->inv()->get(target)); break;
        case ParserTarget::TARGET_SHOP: examine_item(world->get_shop(player->location())->inv()->get(target)); break;
        default: core()->guru()->nonfatal("Invalid examine target.", Guru::GURU_ERROR);
    }
}

// Examines an Item.
void ActionLook::examine_item(std::shared_ptr<Item> target)
{
    const int appraised_value = target->appraised_value();
    const bool plural_name = target->tag(ItemTag::PluralName) || (target->tag(ItemTag::Stackable) && target->stack() > 1);
    const std::string it_has_string_caps = (plural_name ? "They have" : "It has");
    const std::string it_is_string= (plural_name ? "they are" : "it is");
    const std::string it_is_string_caps = (plural_name ? "They are" : "It is");
    const std::string this_is_string_caps = (plural_name ? "These are" : "This is");
    const std::string it_uses_string_caps = (plural_name ? "They use" : "It uses");
    const std::string it_boosts_string_caps = (plural_name ? "They {G}boost" : "It {G}boosts");
    const std::string it_reduces_string_caps = (plural_name ? "They {Y}reduce" : "It {Y}reduces");
    const std::string it_can_string_caps = (plural_name ? "They can" : "It can");
    const std::string it_weighs_string_caps = (plural_name ? "They weigh" : "It weighs");

    auto rarity_msg = [&target, &it_is_string_caps, &it_is_string]() -> std::string {
        switch (target->rare())
        {
            case 1: return it_is_string_caps + " commonplace and inexpensive. ";
            case 2: return it_is_string_caps + " fairly common. ";
            case 3: return "The craftsmanship is decent. ";
            case 4: return "{G}" + it_is_string_caps + " of fine quality.{w} ";
            case 5: return "{G}" + it_is_string_caps + " of excellent quality.{w} ";
            case 6: return "{G}" + it_is_string_caps + " of exceptional quality.{w} ";
            case 7: return "{G}The craftsmanship is superb!{w} ";
            case 8: return "{G}The craftsmanship is of masterful quality!{w} ";
            case 9: return "{G}" + it_is_string_caps + " the stuff of legends!{w} ";
            case 10: return "{G}This is a fabled artifact!{w} ";
            case 11: return "{G}You can scarecely believe " + it_is_string + " real!{w} ";
            case 12: return "{G}This is truly an artifact of the gods!{w} ";
            default: core()->guru()->nonfatal("Invalid rarity value!", Guru::GURU_WARN);
        }
        return "";
    };

    core()->message("You are looking at: " + target->name(Item::NAME_FLAG_FULL_STATS | Item::NAME_FLAG_ID | Item::NAME_FLAG_RARE));
    if (target->desc().size()) core()->message("{0}" + target->desc());
    std::string stat_string;
    switch (target->type())
    {
        case ItemType::AMMO:
        {
            stat_string = this_is_string_caps + " {U}ammunition {w}that can be fired from ";
            if (target->tag(ItemTag::AmmoArrow)) stat_string += "a bow. ";
            else if (target->tag(ItemTag::AmmoBolt)) stat_string += "a crossbow. ";
            else throw std::runtime_error("Unknown ammo type: " + target->name());

            std::vector<std::string> damage_str_vec;
            damage_str_vec.push_back(it_has_string_caps + "a damage multiplier of {U}" + StrX::ftos(target->ammo_power()) + "x{w}");
            if (target->crit()) damage_str_vec.push_back("a critical hit bonus of {U}" + std::to_string(target->crit()) + "%{w}");
            if (target->bleed()) damage_str_vec.push_back("a bleeding bonus of {U}" + std::to_string(target->bleed()) + "%{w}");
            if (target->poison()) damage_str_vec.push_back("a poison bonus of {U}" + std::to_string(target->poison()) + "%{w}");
            stat_string += rarity_msg() + StrX::comma_list(damage_str_vec, StrX::CL_AND | StrX::CL_OXFORD_COMMA) + ". ";

            break;
        }
        case ItemType::ARMOUR:
        {
            switch (target->subtype())
            {
                case ItemSub::CLOTHING: stat_string = this_is_string_caps + " {U}clothing {w}that can be worn"; break;
                case ItemSub::HEAVY: stat_string = this_is_string_caps + " {U}heavy armour {w}that can be worn"; break;
                case ItemSub::LIGHT: stat_string = this_is_string_caps + " {U}lightweight armour {w}that can be worn"; break;
                case ItemSub::MEDIUM: stat_string = this_is_string_caps + " {U}medium armour {w}that can be worn"; break;
                default: break;
            }
            std::string slot;
            switch (target->equip_slot())
            {
                case EquipSlot::ABOUT_BODY: slot = "about the body"; break;
                case EquipSlot::ARMOUR: slot = "over your body"; break;
                case EquipSlot::BODY: slot = "on your body"; break;
                case EquipSlot::FEET: slot = "on your feet"; break;
                case EquipSlot::HANDS: slot = "on your hands"; break;
                case EquipSlot::HEAD: slot = "on your head"; break;
                default: break;
            }
            stat_string += " {U}" + slot + "{w}. " + rarity_msg() + it_has_string_caps + " an armour value of {U}" + std::to_string(target->power());
            const int warmth = target->warmth();
            if (warmth) stat_string += "{w}, and a warmth rating of {U}" + std::to_string(warmth);
            stat_string += "{w}. ";
            break;
        }
        case ItemType::CONTAINER: break;
        case ItemType::DRINK:
        {
            switch (target->subtype())
            {
                case ItemSub::BOOZE: stat_string = this_is_string_caps + " an {U}alcoholic beverage{w}. "; break;
                case ItemSub::WATER_CONTAINER: stat_string = this_is_string_caps + " a {U}water container{w}. "; break;
                default: break;
            }
            const int capacity = target->capacity(), charge = target->charge();
            stat_string += rarity_msg() + it_has_string_caps + " a capacity of {U}" + std::to_string(capacity) + (capacity > 1 ? " units" : " unit") + "{w}";
            if (charge)
            {
                stat_string += ", and currently holds {U}" + std::to_string(charge) + (charge > 1 ? " units of " : " unit of ") + target->liquid_type() + "{w}, and will take {U}" + StrX::time_string_rough(target->speed()) + " {w}to drink. ";
                if (target->subtype() == ItemSub::BOOZE) stat_string += it_has_string_caps + " a potency rating of {U}" + std::to_string(target->power()) + "{w}. ";
            }
            else stat_string += ", and is currently {U}empty{w}. ";
            break;
        }
        case ItemType::FOOD:
        {
            stat_string = this_is_string_caps + " something you can {U}consume{w}. " + rarity_msg();
            stat_string += it_has_string_caps + " a food value of {U}" + std::to_string(target->power()) + "{w}, and will take {U}" + StrX::time_string_rough(target->speed()) + " {w}to eat. ";
            break;
        }
        case ItemType::KEY: stat_string = this_is_string_caps + " a {U}key {w}which can unlock certain doors. " + rarity_msg(); break;
        case ItemType::LIGHT:
            stat_string = this_is_string_caps + " a {U}light source {w}which can be held. " + rarity_msg();
            stat_string += "It provides a brightness level of {Y}" + std::to_string(target->power()) + "{w} when used. ";
            break;
        case ItemType::NONE: break;
        case ItemType::SHIELD:
            stat_string = this_is_string_caps + " a {U}shield {w}which can be wielded. " + rarity_msg();
            stat_string += "It has an armour value of {U}" + std::to_string(target->power()) + "{w}. ";
            break;
        case ItemType::WEAPON:
        {
            switch (target->subtype())
            {
                case ItemSub::MELEE: stat_string = this_is_string_caps + " a {U}melee weapon {w}which can be wielded. "; break;
                case ItemSub::RANGED: stat_string = this_is_string_caps + " a {U}ranged weapon {w}which can be widled. "; break;
                default: break;
            }
            stat_string += rarity_msg();
            if (target->tag(ItemTag::TwoHanded)) stat_string += it_is_string_caps + " heavy and requires {U}two hands {w}to wield. ";
            else if (target->tag(ItemTag::HandAndAHalf)) stat_string += it_is_string_caps + " versatile and can be wielded in {U}either one or two hands{w}. ";
            std::string damage_type_str;
            switch (target->damage_type())
            {
                case DamageType::ACID: damage_type_str = "acid"; break;
                case DamageType::BALLISTIC: damage_type_str = "ballistic"; break;
                case DamageType::CRUSHING: damage_type_str = "crushing"; break;
                case DamageType::EDGED: damage_type_str = "edged"; break;
                case DamageType::ENERGY: damage_type_str = "energy"; break;
                case DamageType::EXPLOSIVE: damage_type_str = "explosive"; break;
                case DamageType::KINETIC: damage_type_str = "kinetic"; break;
                case DamageType::PIERCING: damage_type_str = "piercing"; break;
                case DamageType::PLASMA: damage_type_str = "plasma"; break;
                case DamageType::POISON: damage_type_str = "poison"; break;
                case DamageType::RENDING: damage_type_str = "randing"; break;
                default: core()->guru()->nonfatal("Unable to determine item damage type: " + target->name(), Guru::GURU_ERROR);
            }
            std::vector<std::string> damage_str_vec;
            damage_str_vec.push_back(it_has_string_caps + " a damage value of {U}" + std::to_string(target->power()) + " " + damage_type_str + "{w}");
            damage_str_vec.push_back("a speed of {U}" + StrX::ftos(target->speed(), true) + "{w}");
            if (target->crit()) damage_str_vec.push_back("a critical hit chance of {U}" + std::to_string(target->crit()) + "%{w}");
            if (target->bleed()) damage_str_vec.push_back("a {U}" + std::to_string(target->bleed()) + "%{w} chance to cause bleeding wounds");
            if (target->poison()) damage_str_vec.push_back("a {U}" + std::to_string(target->poison()) + "%{w} chance to inflict poison");
            stat_string += StrX::comma_list(damage_str_vec, StrX::CL_AND | StrX::CL_OXFORD_COMMA) + ". ";

            if (target->tag(ItemTag::AmmoArrow)) stat_string += it_uses_string_caps + " {U}arrows {w}for ammunition. ";
            if (target->tag(ItemTag::AmmoBolt)) stat_string += it_uses_string_caps + " {U}bolts {w}for ammunition. ";
            break;
        }
    }

    const int dodge_mod = target->dodge_mod();
    if (dodge_mod > 0) stat_string += it_boosts_string_caps + " your chance to dodge {w}by {G}" + std::to_string(dodge_mod) + "%{w}. ";
    else if (dodge_mod < 0) stat_string += it_reduces_string_caps + " your chance to dodge {w}by {R}" + std::to_string(-dodge_mod) + "%{w}. ";

    const int parry_mod = target->parry_mod();
    if (parry_mod > 0) stat_string += it_boosts_string_caps + " your chance to parry {w}by {G}" + std::to_string(parry_mod) + "%{w}. ";
    else if (parry_mod < 0) stat_string += it_reduces_string_caps + " your chance to parry {w}by {R}" + std::to_string(-parry_mod) + "%{w}. ";

    const int block_mod = target->block_mod();
    if (block_mod > 0) stat_string += it_boosts_string_caps + " your chance to shield-block {w}by {G}" + std::to_string(block_mod) + "%{w}. ";
    else if (block_mod < 0) stat_string += it_reduces_string_caps + " your chance to shield-block {w}by {R}" + std::to_string(-block_mod) + "%{w}. ";

    const bool stackable = target->tag(ItemTag::Stackable);
    if (stackable) stat_string += it_can_string_caps + " be {U}stacked {w}with other identical items. ";

    uint32_t weight = MathX::fuzz(target->weight()), weight_individual = MathX::fuzz(target->weight(true));
    stat_string += it_weighs_string_caps + " around {U}" + StrX::intostr_pretty(weight) + (weight > 1 ? " pacs" : " pac");
    if (stackable && target->stack() > 1) stat_string += " {w}(around {U}" + StrX::intostr_pretty(weight_individual) + (weight_individual > 1 ? " pacs" : " pac") + " {w}each)";
    else stat_string += "{w}";

    const uint32_t actual_value = target->value();
    const uint32_t diff = std::abs(static_cast<int64_t>(actual_value) - static_cast<int64_t>(appraised_value));
    std::string appraise_str;
    if (diff >= 10000) appraise_str = "{M}you make a wild guess {w}and assume ";
    else if (diff >= 1000) appraise_str = "{R}at a rough guess {w}you think ";
    else if (diff >= 100) appraise_str = "{Y}you think {w}";

    if (!appraised_value) stat_string += (stackable ? "{w}, and {y}aren't worth anything{w}." : "{w}, and {y}isn't worth anything{w}. ");
    else stat_string += (stackable || plural_name ? "{w}, and " + appraise_str + "they're worth around {U}" : "{w}, and " + appraise_str + "it's worth around {U}") + StrX::mgsc_string(appraised_value, StrX::MGSC::LONG) + "{w}. ";

    bool inv_list = false;

    if (target->type() == ItemType::CONTAINER && target->subtype() == ItemSub::CORPSE)
    {
        if (target->inv() && target->inv()->count()) inv_list = true;
        else stat_string += "{y}It doesn't contain anything. ";
    }

    if (stat_string.size())
    {
        stat_string.pop_back();
        core()->message("{0}" + stat_string);
    }

    if (inv_list)
    {
        core()->message("{C}" + target->name(Item::NAME_FLAG_THE | Item::NAME_FLAG_CAPITALIZE_FIRST) + " {C}contains: ");
        for (size_t i = 0; i < target->inv()->count(); i++)
            core()->message("{0}" + target->inv()->get(i)->name(Item::NAME_FLAG_CORE_STATS | Item::NAME_FLAG_ID));
    }
}

// Examines a Mobile.
void ActionLook::examine_mobile(std::shared_ptr<Mobile> target)
{
    core()->message("You are looking at: " + target->name());
}

// Take a look around at your surroundings.
void ActionLook::look()
{
    const auto world = core()->world();
    const auto player = world->player();
    const auto room = world->get_room(player->location());

    if (room->light() < Room::LIGHT_VISIBLE)
    {
        core()->message("{U}Darkness");
        core()->message("{0}```{u}It is {B}pitch black{u}, and you can see {B}nothing{u}. You are likely to be eaten by a grue.");
        return;
    }

    room->set_tag(RoomTag::Explored);

    // Room name and description.
    core()->message("{G}" + room->name());
    core()->message("{0}```" + room->desc());

    // Show any scar effects on the room.
    const std::string scar_desc = room->scar_desc();
    if (scar_desc.size()) core()->message("{0}```" + scar_desc);

    // Special features on this room.
    if (room->tag(RoomTag::WaterClean)) core()->message("{0}```{U}There is clean water nearby that you could gather with a water container.");
    if (room->tag(RoomTag::Arena)) core()->message("{0}```{c}If you wish, you can {C}PARTICIPATE {c}in a fight.");
    if (room->tag(RoomTag::Shop)) core()->message("{0}```{c}You can {C}BUY{c}, {C}SELL{c}, or {C}BROWSE {c}the shop's stock.");

    // Show the obvious exits.
    obvious_exits(true);

    // Items nearby.
    if (room->inv()->count())
    {
        std::vector<std::string> items_nearby;
        for (size_t i = 0; i < room->inv()->count(); i++)
            items_nearby.push_back(room->inv()->get(i)->name(Item::NAME_FLAG_CORE_STATS) + "{w}");
        core()->message("{0}{g}```Items: {w}" + StrX::comma_list(items_nearby, StrX::CL_AND));
    }

    // Mobiles nearby.
    std::vector<std::string> mobs_nearby;
    for (size_t i = 0; i < world->mob_count(); i++)
    {
        const auto world_mob = world->mob_vec(i);
        if (!world_mob) continue; // Ignore any nullptr Mobiles.
        if (world_mob->location() != player->location()) continue;  // Ignore any Mobiles not in this Room.
        mobs_nearby.push_back((world_mob->is_hostile() ? "{R}" : "{Y}") + world_mob->name(Mobile::NAME_FLAG_NO_COLOUR | Mobile::NAME_FLAG_HEALTH) + "{w}");
    }
    if (mobs_nearby.size())
    {
        StrX::collapse_list(mobs_nearby);
        core()->message("{0}{g}```Nearby: {w}" + StrX::comma_list(mobs_nearby, StrX::CL_AND));
    }

    // Check surrounding rooms, to see if anyone is within sight.
    std::vector<std::string> adjacent_mobs;
    for (int i = 0; i < Room::ROOM_LINKS_MAX; i++)
    {
        if (room->fake_link(i)) continue;   // If it goes nowhere, ignore it.
        if (room->link_tag(i, LinkTag::Openable) && !room->link_tag(i, LinkTag::Open)) continue;    //  Ignore closed doors.
        if (room->link_tag(i, LinkTag::Hidden)) continue;    // Ignore hidden links.
        std::vector<std::string> adjacent_this_direction;
        const auto adjacent_room = world->get_room(room->link(i));
        if (adjacent_room->light() < Room::LIGHT_VISIBLE) continue; // Can't see into dark rooms!
        for (size_t m = 0; m < world->mob_count(); m++)
        {
            const auto mob = world->mob_vec(m);
            const std::string colour = (mob->is_hostile() ? "{R}" : "{Y}");
            if (mob->location() == adjacent_room->id()) adjacent_this_direction.push_back(colour + mob->name(Mobile::NAME_FLAG_NO_COLOUR) + "{w}");
        }
        if (!adjacent_this_direction.size()) continue;
        StrX::collapse_list(adjacent_this_direction);
        std::string mob_list = StrX::comma_list(adjacent_this_direction, StrX::CL_AND);
        Direction dir = static_cast<Direction>(i);
        if (dir == Direction::UP) mob_list += " above";
        else if (dir == Direction::DOWN) mob_list += " below";
        else mob_list += " to the " + StrX::dir_to_name(dir);
        adjacent_mobs.push_back(mob_list);
    }
    if (adjacent_mobs.size()) core()->message("{0}{g}```Adjacent: {w}" + StrX::comma_list(adjacent_mobs, StrX::CL_AND));
}

// Lists the exits from this area.
void ActionLook::obvious_exits(bool indent)
{
    const auto player = core()->world()->player();
    const auto room = core()->world()->get_room(player->location());
    if (room->light() < Room::LIGHT_VISIBLE)
    {
        core()->message(std::string(indent ? "{0}" : "") + "{u}It's so {B}dark{u}, you can't see where the exits are!");
        return;
    }

    std::vector<std::string> exits_vec;
    for (int e = 0; e < Room::ROOM_LINKS_MAX; e++)
    {
        uint32_t room_link = room->link(e);
        if (!room_link || room_link == Room::BLOCKED) continue;
        if (room->link_tag(e, LinkTag::Hidden)) continue;   // Never list hidden exits.
        std::string exit_name = "{c}" + StrX::dir_to_name(e);
        const std::string door_name = room->door_name(e);

        if (room_link == Room::UNFINISHED)  // An exit that is due to be finished later.
            exit_name = "{r}(" + StrX::dir_to_name(e) + "){c}";
        else if (room_link == Room::FALSE_ROOM)
        {
            if (room->link_tag(e, LinkTag::KnownLocked)) exit_name += " {m}[locked<>]{c}";
            else exit_name += " {u}[closed<>]{c}";
        }
        else
        {
            const std::shared_ptr<Room> link_room = core()->world()->get_room(room_link);
            if (link_room->tag(RoomTag::Explored) && !link_room->tag(RoomTag::Maze)) exit_name += " {B}(" + link_room->name(true) + "){c}";
            if (room->link_tag(e, LinkTag::KnownLocked)) exit_name += " {m}[locked<>]{c}";
            else if (room->link_tag(e, LinkTag::Openable))
            {
                if (room->link_tag(e, LinkTag::Open)) exit_name += " {u}[open<>]{c}";
                else exit_name += " {u}[closed<>]{c}";
            }
        }

        if (door_name == "door" || door_name == "metal door") StrX::find_and_replace(exit_name, "<>", "");   // Don't specify 'door' if it's just a door.
        else StrX::find_and_replace(exit_name, "<>", " " + door_name);  // Any other 'door' types (windows, grates, etc.) should be specified.

        exits_vec.push_back(exit_name);
    }
    if (exits_vec.size()) core()->message(std::string(indent ? "{0}```" : "") + "{g}Obvious exits: " + StrX::comma_list(exits_vec, StrX::CL_AND));
}
