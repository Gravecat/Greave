// actions/potions.cc -- Consuming magical potions for various effects.
// Copyright (c) 2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#include "actions/potions.h"
#include "core/core.h"
#include "core/strx.h"


// Drinks a specified inventory item.
void ActionPotions::drink(size_t inv_pos, bool confirm)
{
    const auto player = core()->world()->player();
    const auto item = player->inv()->get(inv_pos);
    if (item->type() != ItemType::POTION)
    {
        core()->message("{u}That isn't something you can drink!");
        return;
    }

    if (!player->pass_time(item->speed(), !confirm))
    {
        core()->parser()->interrupted("drink your " + item->name(Item::NAME_FLAG_NO_COLOUR | Item::NAME_FLAG_NO_COUNT));
        return;
    }
    if (player->is_dead()) return;

    std::string drink_str = "{m}You quickly chug {M}" + item->name(Item::NAME_FLAG_NO_COUNT | Item::NAME_FLAG_THE);

    switch (item->subtype())
    {
        case ItemSub::HEALING:
        {
            const bool is_bleeding = player->has_buff(Buff::Type::BLEED);
            const bool was_recently_wounded = player->has_buff(Buff::Type::RECENT_DAMAGE);
            if ((player->hp() >= player->hp(true) || !item->power()) && !is_bleeding && !was_recently_wounded) drink_str += "{m}... but nothing happens.";
            else
            {
                if (is_bleeding) player->clear_buff(Buff::Type::BLEED);
                if (was_recently_wounded) player->clear_buff(Buff::Type::RECENT_DAMAGE);
                player->restore_hp(item->power());
                drink_str += ". {G}Your " + StrX::rainbow_text("wounds", "Rr") + " {G}begin to magically heal!";
            }
            break;
        }
        default: drink_str += "{m}... but nothing happens."; break;
    }
    core()->message(drink_str);

    if (item->tag(ItemTag::Stackable) && item->stack() > 1) item->set_stack(item->stack() - 1);
    else player->inv()->erase(inv_pos);
}
