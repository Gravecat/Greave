// world/world.cc -- The World class defines the game world as a whole and handles the passage of time, as well as keeping track of the player's current activities.
// Copyright (c) 2020-2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#include "3rdparty/yaml-cpp/yaml.h"
#include "actions/look.h"
#include "core/bones.h"
#include "core/core.h"
#include "core/filex.h"
#include "core/mathx.h"
#include "core/strx.h"
#include "world/world.h"


// The SQL construction table for the world data.
constexpr char World::SQL_WORLD[] = "CREATE TABLE world ( mob_unique_id INTEGER PRIMARY KEY UNIQUE NOT NULL )";

// Lookup table for converting DamageType text names into enums.
const std::map<std::string, DamageType> World::DAMAGE_TYPE_MAP = { { "acid", DamageType::ACID }, { "ballistic", DamageType::BALLISTIC }, { "crushing", DamageType::CRUSHING }, { "edged", DamageType::EDGED }, { "explosive", DamageType::EXPLOSIVE }, { "energy", DamageType::ENERGY }, { "kinetic", DamageType::KINETIC }, { "piercing", DamageType::PIERCING }, { "plasma", DamageType::PLASMA }, { "poison", DamageType::POISON }, { "rending", DamageType::RENDING } };

// Lookup table for converting EquipSlot text names into enums.
const std::map<std::string, EquipSlot> World::EQUIP_SLOT_MAP = { { "about", EquipSlot::ABOUT_BODY }, { "armour", EquipSlot::ARMOUR }, { "body", EquipSlot::BODY }, { "feet", EquipSlot::FEET }, { "hands", EquipSlot::HANDS }, { "head", EquipSlot::HEAD }, { "held", EquipSlot::HAND_MAIN } };

// Lookup table for converting ItemSub text names into enums.
const std::map<std::string, ItemSub> World::ITEM_SUBTYPE_MAP = { { "arrow", ItemSub::ARROW }, { "bolt", ItemSub::BOLT }, { "booze", ItemSub::BOOZE }, { "clothing", ItemSub::CLOTHING }, { "corpse", ItemSub::CORPSE }, { "healing", ItemSub::HEALING }, { "heavy", ItemSub::HEAVY }, { "light", ItemSub::LIGHT }, { "medium", ItemSub::MEDIUM }, { "melee", ItemSub::MELEE }, { "none", ItemSub::NONE }, { "ranged", ItemSub::RANGED }, { "unarmed", ItemSub::UNARMED }, { "water_container", ItemSub::WATER_CONTAINER } };

// Lookup table for converting ItemTag text names into enums.
const std::map<std::string, ItemTag> World::ITEM_TAG_MAP = { { "ammoarrow", ItemTag::AmmoArrow }, { "ammobolt", ItemTag::AmmoBolt }, { "discardwhenempty", ItemTag::DiscardWhenEmpty }, { "handandahalf", ItemTag::HandAndAHalf }, { "noa", ItemTag::NoA }, { "noammo", ItemTag::NoAmmo }, { "noloot", ItemTag::NoLoot }, { "offhandonly", ItemTag::OffHandOnly }, { "pluralname", ItemTag::PluralName }, { "preferoffhand", ItemTag::PreferOffHand }, { "propernoun", ItemTag::ProperNoun }, { "stackable", ItemTag::Stackable }, { "tavernonly", ItemTag::TavernOnly }, { "twohanded", ItemTag::TwoHanded } };

// Lookup table for converting ItemType text names into enums.
const std::map<std::string, ItemType> World::ITEM_TYPE_MAP = { { "ammo", ItemType::AMMO }, { "armour", ItemType::ARMOUR }, { "container", ItemType::CONTAINER }, { "drink", ItemType::DRINK }, { "food", ItemType::FOOD }, { "key", ItemType::KEY }, { "light", ItemType::LIGHT }, { "none", ItemType::NONE }, { "potion", ItemType::POTION }, { "shield", ItemType::SHIELD }, { "weapon", ItemType::WEAPON } };

// Lookup table for converting textual light levels (e.g. "bright") to integer values.
const std::map<std::string, uint8_t> World::LIGHT_LEVEL_MAP = { { "bright", 7 }, { "dim", 5 }, { "wilderness", 5 }, { "dark", 3 }, { "none", 0 } };

// Lookup table for converting LinkTag text names into enums.
const std::map<std::string, LinkTag> World::LINK_TAG_MAP = { { "autoclose", LinkTag::AutoClose }, { "autolock", LinkTag::AutoLock }, { "decline", LinkTag::Decline }, { "doormetal", LinkTag::DoorMetal }, { "doorshop", LinkTag::DoorShop }, { "doublelength", LinkTag::DoubleLength }, { "hidden", LinkTag::Hidden }, { "incline", LinkTag::Incline }, { "lockable", LinkTag::Lockable },  { "locked", LinkTag::LockedByDefault }, { "lockstrong", LinkTag::LockStrong }, { "lockswhenclosed", LinkTag::LocksWhenClosed }, { "lockweak", LinkTag::LockWeak }, { "noblockexit", LinkTag::NoBlockExit }, { "nomobroam", LinkTag::NoMobRoam }, { "ocean", LinkTag::Ocean }, { "open", LinkTag::Open }, { "openable", LinkTag::Openable }, { "permalock", LinkTag::Permalock }, { "sky", LinkTag::Sky}, { "sky2", LinkTag::Sky2 }, { "sky3", LinkTag::Sky3 }, { "triplelength", LinkTag::TripleLength }, { "window", LinkTag::Window } };

// Lookup table for converting MobileTag text names into enums.
const std::map<std::string, MobileTag> World::MOBILE_TAG_MAP = { { "aggroonsight", MobileTag::AggroOnSight }, { "agile", MobileTag::Agile }, { "anemic", MobileTag::Anemic }, { "beast", MobileTag::Beast}, { "brawny", MobileTag::Brawny }, { "cannotblock", MobileTag::CannotBlock }, { "cannotdodge", MobileTag::CannotDodge }, { "cannotopendoors", MobileTag::CannotOpenDoors }, { "cannotparry", MobileTag::CannotParry }, { "clumsy", MobileTag::Clumsy }, { "coward", MobileTag::Coward }, { "feeble", MobileTag::Feeble }, { "immunitybleed", MobileTag::ImmunityBleed }, { "immunitypoison", MobileTag::ImmunityPoison }, { "mighty", MobileTag::Mighty }, { "pluralname", MobileTag::PluralName }, { "propernoun", MobileTag::ProperNoun }, { "puny", MobileTag::Puny }, { "randomgender", MobileTag::RandomGender }, { "strong", MobileTag::Strong }, { "unliving", MobileTag::Unliving }, { "vigorous", MobileTag::Vigorous } };

// Lookup table for converting RoomTag text names into enums.
const std::map<std::string, RoomTag> World::ROOM_TAG_MAP = { { "arena", RoomTag::Arena }, { "canseeoutside", RoomTag::CanSeeOutside }, { "churchaltar", RoomTag::ChurchAltar }, { "digok", RoomTag::DigOK }, { "gamepoker", RoomTag::GamePoker }, { "gameslots", RoomTag::GameSlots }, { "gross", RoomTag::Gross }, { "heatedinterior", RoomTag::HeatedInterior }, { "hidecampfirescar", RoomTag::HideCampfireScar }, { "indoors", RoomTag::Indoors }, { "maze", RoomTag::Maze }, { "nexus", RoomTag::Nexus }, { "noexplorecredit", RoomTag::NoExploreCredit }, { "permacampfire", RoomTag::PermaCampfire }, { "private", RoomTag::Private }, { "radiationlight", RoomTag::RadiationLight }, { "shop", RoomTag::Shop }, { "shopbuyscontraband", RoomTag::ShopBuysContraband }, { "shoprespawningowner", RoomTag::ShopRespawningOwner }, { "sleepok", RoomTag::SleepOK }, { "sludgepit", RoomTag::SludgePit }, { "smelly", RoomTag::Smelly }, { "tavern", RoomTag::Tavern }, { "trees", RoomTag::Trees }, { "underground", RoomTag::Underground }, { "verywide", RoomTag::VeryWide }, { "waterclean", RoomTag::WaterClean }, { "waterdeep", RoomTag::WaterDeep }, { "watersalt", RoomTag::WaterSalt }, { "watershallow", RoomTag::WaterShallow }, { "watertainted", RoomTag::WaterTainted }, { "wide", RoomTag::Wide } };

// Lookup table for converting textual room security (e.g. "anarchy") to enum values.
const std::map<std::string, Room::Security> World::SECURITY_MAP = { { "anarchy", Room::Security::ANARCHY }, { "low", Room::Security::LOW }, { "high", Room::Security::HIGH }, { "sanctuary", Room::Security::SANCTUARY }, { "inaccessible", Room::Security::INACCESSIBLE } };

// A list of all valid keys in area YAML files.
const std::set<std::string> World::VALID_YAML_KEYS_AREAS = { "desc", "exits", "light", "metadata", "name", "security", "shop_type", "spawn_mobs", "tags" };

// A list of all valid keys in item YAML files.
const std::set<std::string> World::VALID_YAML_KEYS_ITEMS = { "ammo_power", "bleed", "block_mod", "capacity", "charge", "crit", "damage_type", "desc", "dodge_mod", "liquid", "metadata", "name", "parry_mod", "poison", "power", "rare", "slot", "speed", "stack", "tags", "type", "value", "warmth", "weight" };

// A list of all valid keys in mobile YAML files.
const std::set<std::string> World::VALID_YAML_KEYS_MOBS = { "gear", "hp", "name", "score", "species", "tags" };


// Constructor, loads the room YAML data.
World::World() : mob_unique_id_(0), old_light_level_(0), old_location_(0), player_(std::make_shared<Player>()), time_weather_(std::make_shared<TimeWeather>())
{
    load_room_pool();
    load_item_pool();
    load_mob_pool();
    load_anatomy_pool();
    load_generic_descs();
    load_lists();
    load_skills();
}

// Attempts to scan a room for the active rooms list. Only for internal use with recalc_active_rooms().
void World::active_room_scan(uint32_t target, uint32_t depth)
{
    if (active_rooms_.count(target)) return;       // Ignore any room already on the active list.

    const auto room = get_room(target);
    active_rooms_.insert(target);                  // Add this room to the active list.
    if (depth + 1 >= ROOM_SCAN_DISTANCE) return;    // Just stop here if we're past the scan limit.
    for (unsigned int i = 0; i < Room::ROOM_LINKS_MAX; i++)
    {
        if (room->fake_link(i)) continue;           // Ignore empty links, links to FALSE_ROOM, etc.
        const uint32_t link = room->link(i);
        active_room_scan(link, depth + 1);
    }
}

// Retrieve a list of all active rooms.
std::set<uint32_t> World::active_rooms() const { return active_rooms_; }

// Adds a Mobile to the world.
void World::add_mobile(std::shared_ptr<Mobile> mob)
{
    bool parser_id_valid;
    int tries = 0;
    do
    {
        parser_id_valid = true;
        if (!mob->parser_id()) parser_id_valid = false;
        else
        {
            for (size_t i = 0; i < mobiles_.size(); i++)
                if (mobiles_.at(i)->parser_id() == mob->parser_id()) parser_id_valid = false;
        }
        if (!parser_id_valid) mob->new_parser_id();
    } while (!parser_id_valid && ++tries < 100000);
    if (!mob->id()) mob->set_id(++mob_unique_id_);
    mobiles_.push_back(mob);
}

// Retrieves a generic description string.
std::string World::generic_desc(const std::string &id) const
{
    auto it = generic_descs_.find(id);
    if (it == generic_descs_.end())
    {
        core()->guru()->nonfatal("Invalid generic description requested: " + id, Guru::GURU_ERROR);
        return "-";
    }
    return it->second;
}

// Retrieves a copy of the anatomy data for a given species.
const std::vector<std::shared_ptr<BodyPart>>& World::get_anatomy(const std::string &id) const
{
    if (anatomy_pool_.count(id) == 0) throw std::runtime_error("Could not find species ID: " + id);
    return anatomy_pool_.at(id);
}

// Retrieves a specified Item by ID.
const std::shared_ptr<Item> World::get_item(const std::string &item_id, int stack_size) const
{
    if (!item_id.size()) throw std::runtime_error("Blank item ID requested.");
    const auto it = item_pool_.find(StrX::hash(item_id));
    if (it == item_pool_.end()) throw std::runtime_error("Invalid item ID requested: " + item_id);
    auto copy = std::make_shared<Item>(*it->second);
    if (stack_size > 0) copy->set_stack(stack_size);
    return copy;
}

// Retrieves a specified List by ID.
std::shared_ptr<List> World::get_list(const std::string &list_id) const
{
    if (list_pool_.count(list_id) == 0) throw std::runtime_error("Could not find list ID: " + list_id);
    return std::make_shared<List>(*list_pool_.at(list_id));
}

// Retrieves a specified Mobile by ID.
const std::shared_ptr<Mobile> World::get_mob(const std::string &mob_id) const
{
    if (!mob_id.size()) throw std::runtime_error("Blank mobile ID requested.");
    const uint32_t id_hash = StrX::hash(mob_id);
    const auto it = mob_pool_.find(id_hash);
    if (it == mob_pool_.end()) throw std::runtime_error("Invalid mobile ID requested: " + mob_id);
    auto new_mob = std::make_shared<Mobile>(*it->second);

    if (new_mob->tag(MobileTag::RandomGender))
    {
        if (core()->rng()->rnd(2) == 1) new_mob->set_gender(Gender::FEMALE);
        else new_mob->set_gender(Gender::MALE);
    }

    // If this Mobile has a gear list, equip it now.
    const std::string gear_list_str = mob_gear_.at(id_hash);
    if (gear_list_str.size())
    {
        auto gear_list = get_list(gear_list_str);
        bool main_hand_used = false;
        for (size_t i = 0; i < gear_list->size(); i++)
        {
            std::string gear_str = gear_list->at(i).str;
            if (gear_str == "-" || !gear_str.size()) continue;
            else if (gear_str[0] == '+')
            {
                auto sublist = get_list(gear_str.substr(1));
                gear_list->merge_with(sublist);
                continue;
            }
            const auto new_item = get_item(gear_str, gear_list->at(i).count);
            new_mob->equ()->add_item(new_item);
            if (new_item->equip_slot() == EquipSlot::HAND_MAIN)
            {
                if (main_hand_used) new_item->set_equip_slot(EquipSlot::HAND_OFF);
                else main_hand_used = true;
            }
        }
    }

    // Mixes up this mobile's stats.
    int hp = MathX::mixup(new_mob->hp());
    new_mob->set_hp(hp, hp);

    return new_mob;
}

// Retrieves a specified Room by ID.
const std::shared_ptr<Room> World::get_room(uint32_t room_id) const
{
    const auto it = room_pool_.find(room_id);
    if (it == room_pool_.end()) throw std::runtime_error("Invalid room ID requested: " + std::to_string(room_id));
    return it->second;
}

// As above, but with a Room ID string.
const std::shared_ptr<Room> World::get_room(const std::string &room_id) const
{
    if (!room_id.size()) throw std::runtime_error("Blank room ID requested.");
    else return get_room(StrX::hash(room_id));
}

// Returns a specified shop, or creates a new shop if this ID doesn't yet exist.
const std::shared_ptr<Shop> World::get_shop(uint32_t id)
{
    const auto it = shops_.find(id);
    if (it == shops_.end())
    {
        auto new_shop = std::make_shared<Shop>(id);
        new_shop->restock();
        shops_.insert(std::make_pair(id, new_shop));
        return new_shop;
    }
    else return it->second;
}

// Retrieves the XP gain multiplier for a specified skill.
float World::get_skill_multiplier(const std::string &skill)
{
    const auto it = skills_.find(skill);
    if (it == skills_.end())
    {
        core()->guru()->nonfatal("Invalid skill requested: " + skill, Guru::GURU_ERROR);
        return 0;
    }
    return it->second.xp_multi;
}

// Retrieves the name of a specified skill.
std::string World::get_skill_name(const std::string &skill)
{
    const auto it = skills_.find(skill);
    if (it == skills_.end())
    {
        core()->guru()->nonfatal("Invalid skill requested: " + skill, Guru::GURU_ERROR);
        return "[error]";
    }
    return it->second.name;
}

// Checks if a specified item ID exists.
bool World::item_exists(const std::string &str) const { return item_pool_.count(StrX::hash(str)); }

// Loads the World and all things within it.
void World::load(std::shared_ptr<SQLite::Database> save_db)
{
    core()->messagelog()->load(save_db);

    SQLite::Statement world_query(*save_db, "SELECT * FROM world");
    if (!world_query.executeStep()) throw std::runtime_error("Unable to retrieve world data!");
    mob_unique_id_ = world_query.getColumn("mob_unique_id").getUInt();

    for (auto room : room_pool_)
    {
        room.second->load(save_db);
        // Check if the Room has the SaveActive tag; if so, add it to the active rooms list, then remove the tag.
        if (room.second->tag(RoomTag::SaveActive))
        {
            active_rooms_.insert(room.first);
            room.second->clear_tag(RoomTag::SaveActive);
        }
    }
    const uint32_t player_sql_id = player_->load(save_db, 0);
    time_weather_->load(save_db);

    SQLite::Statement mob_query(*save_db, "SELECT sql_id FROM mobiles WHERE sql_id != :sql_id ORDER BY sql_id ASC");
    mob_query.bind(":sql_id", std::to_string(player_sql_id));
    while (mob_query.executeStep())
    {
        auto new_mob = std::make_shared<Mobile>();
        new_mob->load(save_db, mob_query.getColumn("sql_id").getUInt());
        add_mobile(new_mob);
    }

    SQLite::Statement shop_query(*save_db, "SELECT id FROM shops ORDER BY id ASC");
    while (shop_query.executeStep())
    {
        const uint32_t shop_id = shop_query.getColumn("id").getUInt();
        auto new_shop = std::make_shared<Shop>(shop_id);
        new_shop->load(save_db);
        shops_.insert(std::make_pair(shop_id, new_shop));
    }
}

// Triggers events that happen during the main loop, just after player input.
void World::main_loop_events_post_input()
{
    // Check to see if the light level has changed.
    if (player_->location() == old_location_)
    {
        const auto room = get_room(old_location_);
        int new_light = room->light();
        if (old_light_level_ >= Room::LIGHT_VISIBLE && new_light < Room::LIGHT_VISIBLE) core()->message("{u}You are plunged into {B}darkness{u}!");
        else if (old_light_level_ < Room::LIGHT_VISIBLE && new_light >= Room::LIGHT_VISIBLE)
        {
            core()->message("{U}You can now see {W}clearly{U}!");
            ActionLook::look();
        }
    }
}

// Triggers events that happen during the main loop, just before player input.
void World::main_loop_events_pre_input()
{
    // The Grit buff falls off as soon as it's the player's turn to act, if they took damage.
    if (player_->has_buff(Buff::Type::GRIT) && player_->tag(MobileTag::Success_Grit))
    {
        player_->clear_tag(MobileTag::Success_Grit);
        player_->clear_buff(Buff::Type::GRIT);
    }

    // Similarly, QuickRoll falls off if it was used.
    if (player_->has_buff(Buff::Type::QUICK_ROLL) && player_->tag(MobileTag::Success_QuickRoll))
    {
        player_->clear_tag(MobileTag::Success_QuickRoll);
        player_->clear_buff(Buff::Type::QUICK_ROLL);
    }

    // As does ShieldWall.
    if (player_->has_buff(Buff::Type::SHIELD_WALL) && player_->tag(MobileTag::Success_ShieldWall))
    {
        player_->clear_tag(MobileTag::Success_ShieldWall);
        player_->clear_buff(Buff::Type::SHIELD_WALL);
    }

    // For checking if the light level has changed due to something that happened this turn.
    old_location_ = player_->location();
    const auto room = get_room(old_location_);
    old_light_level_ = room->light();
}

// Loads the anatomy YAML data into memory.
void World::load_anatomy_pool()
{
    try
    {
        const YAML::Node yaml_anatomies = YAML::LoadFile("data/misc/anatomy.yml");
        for (auto a : yaml_anatomies)
        {
            // First, determine the species ID.
            const std::string species_id = a.first.as<std::string>();

            std::vector<std::shared_ptr<BodyPart>> anatomy_vec;

            // Cycle over the body parts.
            for (auto bp : a.second)
            {
                auto new_bp = std::make_shared<BodyPart>();
                if (!bp.second.IsSequence() || bp.second.size() != 2)
                {
                    core()->guru()->nonfatal("Anatomy data incorrect for " + species_id, Guru::GURU_CRITICAL);
                    continue;
                }
                new_bp->name = bp.first.as<std::string>();
                new_bp->hit_chance = bp.second[0].as<int>();
                const std::string target = bp.second[1].as<std::string>();
                if (target == "body") new_bp->slot = EquipSlot::BODY;
                else if (target == "head") new_bp->slot = EquipSlot::HEAD;
                else if (target == "feet") new_bp->slot = EquipSlot::FEET;
                else if (target == "hands") new_bp->slot = EquipSlot::HANDS;
                else
                {
                    core()->guru()->nonfatal("Could not determine body part armour target for " + species_id + ": " + target, Guru::GURU_CRITICAL);
                    continue;
                }
                anatomy_vec.push_back(new_bp);
            }

            anatomy_pool_.insert(std::pair<std::string, std::vector<std::shared_ptr<BodyPart>>>(species_id, anatomy_vec));
        }
    } catch (std::exception& e)
    {
        throw std::runtime_error("Error while loading data/misc/anatomy.yml: " + std::string(e.what()));
    }
}

// Loads the generic descriptions YAML data into memory.
void World::load_generic_descs()
{
    try
    {
        const YAML::Node yaml_descs = YAML::LoadFile("data/misc/generic-descriptions.yml");
        for (auto desc : yaml_descs)
            generic_descs_.insert(std::make_pair(desc.first.as<std::string>(), desc.second.as<std::string>()));
    }
    catch (std::exception& e)
    {
        throw std::runtime_error("Error while loading data/misc/generic-descriptions.yml: " + std::string(e.what()));
    }
}

// Loads the Item YAML data into memory.
void World::load_item_pool()
{
    std::string current_file;
    try
    {
        const std::vector<std::string> item_files = FileX::files_in_dir("data/items", true);
        for (auto item_file : item_files)
        {
            current_file = item_file;
            const YAML::Node yaml_items = YAML::LoadFile("data/items/" + item_file);
            for (auto item : yaml_items)
            {
                const YAML::Node item_data = item.second;

                // Create a new Item object.
                const std::string item_id_str = item.first.as<std::string>();
                const uint32_t item_id = StrX::hash(item_id_str);
                const auto new_item(std::make_shared<Item>());

                // Verify all keys in this file.
                for (auto key_value : item_data)
                {
                    const std::string key = key_value.first.as<std::string>();
                    if (VALID_YAML_KEYS_ITEMS.find(key) == VALID_YAML_KEYS_ITEMS.end())
                        core()->guru()->nonfatal("Invalid key in item YAML data (" + key + "): " + item_id_str, Guru::GURU_WARN);
                }

                // Check to make sure there are no hash collisions.
                if (item_pool_.find(item_id) != item_pool_.end()) throw std::runtime_error("Item ID hash conflict: " + item_id_str);

                // The Item's type and subtype.
                if (!item_data["type"]) throw std::runtime_error("Missing item type: " + item_id_str);
                std::string item_type_str, item_subtype_str;
                if (item_data["type"].IsSequence())
                {
                    const unsigned int seq_size = item_data["type"].size();
                    if (seq_size < 1 || seq_size > 2) throw std::runtime_error("Item type data malforned: " + item_id_str);
                    item_type_str = item_data["type"][0].as<std::string>();
                    if (seq_size == 2) item_subtype_str = item_data["type"][1].as<std::string>();
                }
                else item_type_str = item_data["type"].as<std::string>();
                ItemType type = ItemType::NONE;
                ItemSub subtype = ItemSub::NONE;
                if (item_type_str.size())
                {
                    const auto it = ITEM_TYPE_MAP.find(item_type_str);
                    if (it == ITEM_TYPE_MAP.end()) core()->guru()->nonfatal("Invalid item type on " + item_id_str + ": " + item_type_str, Guru::GURU_ERROR);
                    else type = it->second;
                }
                if (item_subtype_str.size())
                {
                    const auto it = ITEM_SUBTYPE_MAP.find(item_subtype_str);
                    if (it == ITEM_SUBTYPE_MAP.end()) core()->guru()->nonfatal("Invalid item subtype on " + item_id_str + ": " + item_subtype_str, Guru::GURU_ERROR);
                    else subtype = it->second;
                }
                new_item->set_type(type, subtype);

                // The Item's tags, if any.
                if (item_data["tags"])
                {
                    if (!item_data["tags"].IsSequence()) core()->guru()->nonfatal("{r}Malformed item tags: " + item_id_str, Guru::GURU_ERROR);
                    else for (auto tag : item_data["tags"])
                    {
                        const std::string tag_str = StrX::str_tolower(tag.as<std::string>());
                        const auto tag_it = ITEM_TAG_MAP.find(tag_str);
                        if (tag_it == ITEM_TAG_MAP.end()) core()->guru()->nonfatal("Unrecognized item tag (" + tag_str + "): " + item_id_str, Guru::GURU_ERROR);
                        else new_item->set_tag(tag_it->second);
                    }
                }

                // The Item's metadata, if any.
                if (item_data["metadata"]) StrX::string_to_metadata(item_data["metadata"].as<std::string>(), *new_item->meta_raw());

                // The Item's name.
                if (!item_data["name"]) throw std::runtime_error("Missing item name: " + item_id_str);
                if (item_data["name"].IsSequence())
                {
                    const unsigned int seq_size = item_data["name"].size();
                    if (seq_size < 1 || seq_size > 2) throw std::runtime_error("Item name data malforned: " + item_id_str);
                    new_item->set_name(item_data["name"][0].as<std::string>());
                    if (seq_size == 2) new_item->set_meta("plural_name", item_data["name"][1].as<std::string>());
                }
                else new_item->set_name(item_data["name"].as<std::string>());

                // The Item's damage type, if any.
                if (item_data["damage_type"])
                {
                    const std::string damage_type = item_data["damage_type"].as<std::string>();
                    const auto type_it = DAMAGE_TYPE_MAP.find(damage_type);
                    if (type_it == DAMAGE_TYPE_MAP.end()) core()->guru()->nonfatal("Unrecognized damage type (" + damage_type + "): " + item_id_str, Guru::GURU_ERROR);
                    else new_item->set_meta("damage_type", static_cast<int>(type_it->second));
                }

                // The item's block% modifier, if a ny.
                if (item_data["block_mod"]) new_item->set_meta("block_mod", item_data["block_mod"].as<int>());

                // The item's dodge% modifier, if any.
                if (item_data["dodge_mod"]) new_item->set_meta("dodge_mod", item_data["dodge_mod"].as<int>());

                // The item's parry% modifier, if any.
                if (item_data["parry_mod"]) new_item->set_meta("parry_mod", item_data["parry_mod"].as<int>());

                // The Item's critical power, if any.
                if (item_data["crit"]) new_item->set_meta("crit", item_data["crit"].as<int>());

                // The Item's speed, if any.
                if (item_data["speed"]) new_item->set_meta("speed", item_data["speed"].as<float>());

                // The Item's capacity, if any.
                if (item_data["capacity"]) new_item->set_meta("capacity", item_data["capacity"].as<int>());

                // The Item's charge, if any.
                if (item_data["charge"]) new_item->set_meta("charge", item_data["charge"].as<int>());

                // The Item's EquipSlot, if any.
                if (item_data["slot"])
                {
                    const std::string slot_str = item_data["slot"].as<std::string>();
                    const auto slot_it = EQUIP_SLOT_MAP.find(slot_str);
                    if (slot_it == EQUIP_SLOT_MAP.end()) core()->guru()->nonfatal("Unrecognized equipment slot (" + slot_str + "): " + item_id_str, Guru::GURU_ERROR);
                    else
                    {
                        EquipSlot chosen_slot = slot_it->second;
                        if (new_item->type() == ItemType::SHIELD && new_item->equip_slot() == EquipSlot::HAND_MAIN) chosen_slot = EquipSlot::HAND_OFF;
                        new_item->set_meta("slot", static_cast<int>(chosen_slot));
                    }
                }

                // The Item's power, if any.
                if (item_data["power"]) new_item->set_meta("power", item_data["power"].as<int>());

                // The Item's ammunition power, if any.
                if (item_data["ammo_power"]) new_item->set_meta("ammo_power", item_data["ammo_power"].as<float>());

                // The Item's warmth rating, if any.
                if (item_data["warmth"]) new_item->set_meta("warmth", item_data["warmth"].as<int>());

                // The Item's bleed chance, if any.
                if (item_data["bleed"]) new_item->set_meta("bleed", item_data["bleed"].as<int>());

                // The Item's poison chance, if any.
                if (item_data["poison"]) new_item->set_meta("poison", item_data["poison"].as<int>());

                // The Item's liquid type, if any.
                if (item_data["liquid"]) new_item->set_meta("liquid", item_data["liquid"].as<std::string>());

                // The Item's description, if any.
                if (!item_data["desc"]) core()->guru()->nonfatal("Missing description for item " + item_id_str, Guru::GURU_WARN);
                else
                {
                    const std::string desc = item_data["desc"].as<std::string>();
                    if (desc != "-") new_item->set_description(desc);
                }

                // The Item's value.
                unsigned int item_value = 0;
                if (!item_data["value"]) core()->guru()->nonfatal("Missing value for item " + item_id_str, Guru::GURU_WARN);
                else
                {
                    const std::string value_str = item_data["value"].as<std::string>();
                    if (value_str.size() && value_str != "0" && value_str != "-")
                    {
                        std::vector<std::string> coins_split = StrX::string_explode(value_str, " ");
                        while (coins_split.size())
                        {
                            const std::string coin_str = coins_split.at(0);
                            coins_split.erase(coins_split.begin());
                            if (coin_str.size() < 2) throw std::runtime_error("Malformed item value string on " + item_id);
                            const char currency = coin_str[coin_str.size() - 1];
                            unsigned int currency_amount = std::stoi(coin_str.substr(0, coin_str.size() - 1));
                            if (currency == 'c') item_value += currency_amount;
                            else if (currency == 's') item_value += currency_amount * 10;
                            else if (currency == 'g') item_value += currency_amount * 1000;
                            else if (currency == 'm') item_value += currency_amount * 1000000;
                            else throw std::runtime_error("Malformed item value string on " + item_id);
                        }
                        if (!item_value) throw std::runtime_error("Null coin value on " + item_id);
                    }
                }
                new_item->set_value(item_value);

                // The Item's rarity.
                if (!item_data["rare"]) core()->guru()->nonfatal("Missing rarity for item " + item_id_str, Guru::GURU_WARN);
                else new_item->set_rare(item_data["rare"].as<int>());

                // The Item's weight.
                if (!item_data["weight"]) core()->guru()->nonfatal("Missing weight for item " + item_id_str, Guru::GURU_ERROR);
                else new_item->set_weight(item_data["weight"].as<uint32_t>());

                // The Item's stack size, if any.
                if (item_data["stack"])
                {
                    if (!new_item->tag(ItemTag::Stackable)) core()->guru()->nonfatal("Stack size specified for nonstackable item: " + item_id_str, Guru::GURU_ERROR);
                    new_item->set_stack(item_data["stack"].as<uint32_t>());
                }

                // Add the new Item to the item pool.
                item_pool_.insert(std::make_pair(item_id, new_item));
            }
        }
    }
    catch (std::exception& e)
    {
        if (current_file.size()) throw std::runtime_error("YAML error while loading data/items/" + current_file + ": " + std::string(e.what()));
        else throw e;
    }
}

// Loads the List YAML data into memory.
void World::load_lists()
{
    std::string current_file;
    try
    {
        const std::vector<std::string> list_files = FileX::files_in_dir("data/lists", true);
        for (auto list_file : list_files)
        {
            current_file = list_file;
            const YAML::Node yaml_lists = YAML::LoadFile("data/lists/" + list_file);
            for (auto list : yaml_lists)
            {
                // First, determine the List's ID.
                const std::string list_id = list.first.as<std::string>();

                // Get the rest of the data.
                const YAML::Node yaml_list = list.second;
                if (!yaml_list.IsSequence()) throw std::runtime_error("Invalid list data for list " + list_id);

                auto new_list = std::make_shared<List>();
                bool is_count = false;
                ListEntry new_list_entry;
                for (auto le : yaml_list)
                {
                    if (is_count)
                    {
                        new_list_entry.count = le.as<int>();
                        new_list->push_back(new_list_entry);
                        is_count = false;
                    }
                    else
                    {
                        const std::string str = le.as<std::string>();
                        new_list_entry.str = str;
                        if (str.size() && (str[0] == '#' || str[0] == '+' || str[0] == '&'))
                        {
                            new_list_entry.count = -1;
                            new_list->push_back(new_list_entry);
                        }
                        else is_count = true;
                    }
                }
                if (is_count) throw std::runtime_error("Invalid list length: " + list_id);
                list_pool_.insert(std::pair<std::string, std::shared_ptr<List>>(list_id, new_list));
            }
        }
    } catch (std::exception& e)
    {
        if (current_file.size()) throw std::runtime_error("Error while loading data/lists/" + current_file + ": " + std::string(e.what()));
        else throw e;
    }
}

// Loads the Mobile YAML data into memory.
void World::load_mob_pool()
{
    std::string current_file;
    try
    {
        const std::vector<std::string> mobile_files = FileX::files_in_dir("data/mobiles", true);
        for (auto mobile_file : mobile_files)
        {
            current_file = mobile_file;
            const YAML::Node yaml_mobiles = YAML::LoadFile("data/mobiles/" + mobile_file);
            for (auto mobile : yaml_mobiles)
            {
                const YAML::Node mobile_data = mobile.second;

                // Create a new Mobile object, and remember its unique ID.
                const std::string mobile_id_str = mobile.first.as<std::string>();
                const uint32_t mobile_id = StrX::hash(mobile_id_str);
                const auto new_mob(std::make_shared<Mobile>());

                // Verify all keys in this file.
                for (auto key_value : mobile_data)
                {
                    const std::string key = key_value.first.as<std::string>();
                    if (VALID_YAML_KEYS_MOBS.find(key) == VALID_YAML_KEYS_MOBS.end())
                        core()->guru()->nonfatal("Invalid key in mobile YAML data (" + key + "): " + mobile_id_str, Guru::GURU_WARN);
                }

                // Check to make sure there are no hash collisions.
                if (mob_pool_.find(mobile_id) != mob_pool_.end()) throw std::runtime_error("Mobile ID hash conflict: " + mobile_id_str);

                // The Mobile's name.
                if (!mobile_data["name"]) core()->guru()->nonfatal("Missing mobile name: " + mobile_id_str, Guru::GURU_ERROR);
                else new_mob->set_name(mobile_data["name"].as<std::string>());

                // The Mobile's hit points.
                if (!mobile_data["hp"]) core()->guru()->nonfatal("Missing mobile hit points: "+ mobile_id_str, Guru::GURU_ERROR);
                else new_mob->set_hp(mobile_data["hp"].as<int>(), mobile_data["hp"].as<int>());

                // The Mobile's score, if any.
                if (mobile_data["score"]) new_mob->add_score(mobile_data["score"].as<int>());

                // The Mobile's species.
                if (!mobile_data["species"]) core()->guru()->nonfatal("Missing species: " + mobile_id_str, Guru::GURU_CRITICAL);
                else new_mob->set_species(mobile_data["species"].as<std::string>());

                // The Mobile's tags, if any.
                if (mobile_data["tags"])
                {
                    if (!mobile_data["tags"].IsSequence()) core()->guru()->nonfatal("{r}Malformed mobile tags: " + mobile_id_str, Guru::GURU_ERROR);
                    else for (auto tag : mobile_data["tags"])
                    {
                        const std::string tag_str = StrX::str_tolower(tag.as<std::string>());
                        const auto tag_it = MOBILE_TAG_MAP.find(tag_str);
                        if (tag_it == MOBILE_TAG_MAP.end()) core()->guru()->nonfatal("Unrecognized mobile tag (" + tag_str + "): " + mobile_id_str, Guru::GURU_ERROR);
                        else new_mob->set_tag(tag_it->second);
                    }
                }

                // The Mobile's gear list.
                std::string gear_list;
                if (mobile_data["gear"]) gear_list = mobile_data["gear"].as<std::string>();

                // Add the Mobile to the mob pool.
                mob_pool_.insert(std::make_pair(mobile_id, new_mob));
                mob_gear_.insert(std::make_pair(mobile_id, gear_list));
            }
        }
    }
    catch (std::exception& e)
    {
        if (current_file.size()) throw std::runtime_error("YAML error while loading data/mobiles/" + current_file + ": " + std::string(e.what()));
        else throw e;
    }
}

// Loads the Room YAML data into memory.
void World::load_room_pool()
{
    std::string current_file;
    try
    {
        const std::vector<std::string> area_files = FileX::files_in_dir("data/areas", true);
        for (auto area_file : area_files)
        {
            current_file = area_file;
            const YAML::Node yaml_rooms = YAML::LoadFile("data/areas/" + area_file);
            for (auto room : yaml_rooms)
            {
                const YAML::Node room_data = room.second;

                // Create a new Room object, and set its unique ID.
                const std::string room_id = room.first.as<std::string>();
                const auto new_room(std::make_shared<Room>(room_id));

                // Verify all keys in this file.
                for (auto key_value : room_data)
                {
                    const std::string key = key_value.first.as<std::string>();
                    if (VALID_YAML_KEYS_AREAS.find(key) == VALID_YAML_KEYS_AREAS.end())
                        core()->guru()->nonfatal("Invalid key in room YAML data (" + key + "): " + room_id, Guru::GURU_WARN);
                }

                // Check to make sure there are no hash collisions.
                if (room_pool_.find(new_room->id()) != room_pool_.end()) throw std::runtime_error("Room ID hash conflict: " + room_id);

                // The Room's long and short names.
                if (!room_data["name"] || room_data["name"].size() < 2) core()->guru()->nonfatal("Missing or invalid room name(s): " + room_id, Guru::GURU_ERROR);
                else new_room->set_name(room_data["name"][0].as<std::string>(), room_data["name"][1].as<std::string>());

                // The Room's description.
                if (!room_data["desc"]) core()->guru()->nonfatal("Missing room description: " + room_id, Guru::GURU_WARN);
                else
                {
                    const std::string desc = room_data["desc"].as<std::string>();
                    if (desc != "-") new_room->set_desc(desc);
                }

                // Links to other Rooms.
                if (room_data["exits"])
                {
                    for (unsigned int e = 0; e < Room::ROOM_LINKS_MAX; e++)
                    {
                        const Direction dir = static_cast<Direction>(e);
                        const std::string dir_str = StrX::dir_to_name(dir);
                        if (room_data["exits"][dir_str]) new_room->set_link(dir, room_data["exits"][dir_str].as<std::string>());
                    }
                }

                // The light level of the Room.
                if (!room_data["light"]) core()->guru()->nonfatal("Missing room light level: " + room_id, Guru::GURU_ERROR);
                else
                {
                    const std::string light_str = room_data["light"].as<std::string>();
                    auto level_it = LIGHT_LEVEL_MAP.find(light_str);
                    if (level_it == LIGHT_LEVEL_MAP.end()) core()->guru()->nonfatal("Invalid light level value: " + room_id, Guru::GURU_ERROR);
                    else new_room->set_base_light(level_it->second);
                }

                // The security level of this Room.
                if (!room_data["security"]) core()->guru()->nonfatal("Missing room security level: " + room_id, Guru::GURU_ERROR);
                else
                {
                    const std::string sec_str = room_data["security"].as<std::string>();
                    auto sec_it = SECURITY_MAP.find(sec_str);
                    if (sec_it == SECURITY_MAP.end()) core()->guru()->nonfatal("Invalid security level value: " + room_id, Guru::GURU_ERROR);
                    else new_room->set_security(sec_it->second);
                }

                // Room tags, if any.
                if (room_data["tags"])
                {
                    if (!room_data["tags"].IsSequence()) core()->guru()->nonfatal("{r}Malformed room tags: " + room_id, Guru::GURU_ERROR);
                    else for (auto tag : room_data["tags"])
                    {
                        const std::string tag_str = StrX::str_tolower(tag.as<std::string>());
                        bool directional_tag = false;
                        int dt_int = 0, dt_offset = 0;

                        if (tag_str.size() > 9)
                        {
                            if (tag_str.substr(0, 9) == "northeast")
                            {
                                directional_tag = true;
                                dt_int = static_cast<unsigned int>(Direction::NORTHEAST);
                                dt_offset = 9;
                            }
                            else if (tag_str.substr(0, 9) == "northwest")
                            {
                                directional_tag = true;
                                dt_int = static_cast<unsigned int>(Direction::NORTHWEST);
                                dt_offset = 9;
                            }
                            else if (tag_str.substr(0, 9) == "southeast")
                            {
                                directional_tag = true;
                                dt_int = static_cast<unsigned int>(Direction::SOUTHEAST);
                                dt_offset = 9;
                            }
                            else if (tag_str.substr(0, 9) == "southwest")
                            {
                                directional_tag = true;
                                dt_int = static_cast<unsigned int>(Direction::SOUTHWEST);
                                dt_offset = 9;
                            }
                        }
                        if (tag_str.size() > 5 && !directional_tag)
                        {
                            if (tag_str.substr(0, 5) == "north")
                            {
                                directional_tag = true;
                                dt_int = static_cast<unsigned int>(Direction::NORTH);
                                dt_offset = 5;
                            }
                            else if (tag_str.substr(0, 5) == "south")
                            {
                                directional_tag = true;
                                dt_int = static_cast<unsigned int>(Direction::SOUTH);
                                dt_offset = 5;
                            }
                        }
                        if (tag_str.size() > 4 && !directional_tag)
                        {
                            if (tag_str.substr(0, 4) == "east")
                            {
                                directional_tag = true;
                                dt_int = static_cast<unsigned int>(Direction::EAST);
                                dt_offset = 4;
                            }
                            else if (tag_str.substr(0, 4) == "west")
                            {
                                directional_tag = true;
                                dt_int = static_cast<unsigned int>(Direction::WEST);
                                dt_offset = 4;
                            }
                            else if (tag_str.substr(0, 4) == "down")
                            {
                                directional_tag = true;
                                dt_int = static_cast<unsigned int>(Direction::DOWN);
                                dt_offset = 4;
                            }
                        }
                        if (tag_str.size() > 2 && !directional_tag)
                        {
                            if (tag_str.substr(0, 2) == "up")
                            {
                                directional_tag = true;
                                dt_int = static_cast<unsigned int>(Direction::UP);
                                dt_offset = 2;
                            }
                        }

                        if (!directional_tag)
                        {
                            const auto tag_it = ROOM_TAG_MAP.find(tag_str);
                            if (tag_it == ROOM_TAG_MAP.end()) core()->guru()->nonfatal("Unrecognized room tag (" + tag_str + "): " + room_id, Guru::GURU_WARN);
                            else new_room->set_tag(tag_it->second);
                        }
                        else
                        {
                            const std::string dtag_str = tag_str.substr(dt_offset);
                            const auto dtag_it = LINK_TAG_MAP.find(dtag_str);
                            if (dtag_it == LINK_TAG_MAP.end()) core()->guru()->nonfatal("Unrecognized link tag (" + dtag_str + "): " + room_id, Guru::GURU_WARN);
                            else
                            {
                                const LinkTag lt = dtag_it->second;
                                switch (lt)
                                {
                                    case LinkTag::Lockable:
                                    case LinkTag::Window:
                                        new_room->set_link_tag(dt_int, LinkTag::Openable);
                                        break;
                                    case LinkTag::LockedByDefault:
                                        new_room->set_link_tag(dt_int, LinkTag::Lockable);
                                        new_room->set_link_tag(dt_int, LinkTag::Openable);
                                        break;
                                    case LinkTag::Open:
                                        new_room->set_link_tag(dt_int, LinkTag::Openable);
                                        break;
                                    default: break;
                                }
                                new_room->set_link_tag(dt_int, lt);
                            }
                        }
                    }
                }

                // Mobile spawns, if any.
                if (room_data["spawn_mobs"])
                {
                    if (room_data["spawn_mobs"].IsSequence())
                    {
                        for (auto e : room_data["spawn_mobs"])
                            new_room->add_mob_spawn(e.as<std::string>());
                    }
                    else new_room->add_mob_spawn(room_data["spawn_mobs"].as<std::string>());
                }

                // The Room's metadata, if any.
                if (room_data["metadata"]) StrX::string_to_metadata(room_data["metadata"].as<std::string>(), *new_room->meta_raw());

                // The room's  shop type, if any.
                if (room_data["shop_type"]) new_room->set_meta("shop_type", room_data["shop_type"].as<std::string>());

                // Clear the meta changed tag, since this is static data.
                new_room->clear_tag(RoomTag::MetaChanged);

                // Add the new Room to the room pool.
                room_pool_.insert(std::make_pair(new_room->id(), new_room));
            }
        }
    }
    catch (std::exception& e)
    {
        if (current_file.size()) throw std::runtime_error("YAML error while loading data/areas/" + current_file + ": " + std::string(e.what()));
        else throw e;
    }
}

// Laods the skills YAML data into memory.
void World::load_skills()
{
    try
    {
        const YAML::Node skills_yaml = YAML::LoadFile("data/misc/skills.yml");
        for (const auto skill : skills_yaml)
        {
            const std::string skill_id = skill.first.as<std::string>();
            const YAML::Node skill_data = skill.second;
            if (!skill_data["name"]) throw std::runtime_error("Skill name not specified: " + skill_id);
            if (!skill_data["xp_multi"]) throw std::runtime_error("Skill XP multiplier not specified: " + skill_id);
            SkillData new_skill = { skill_data["name"].as<std::string>(), skill_data["xp_multi"].as<float>() };
            skills_.insert(std::make_pair(skill_id, new_skill));
        }
    }
    catch (std::exception& e)
    {
        throw std::runtime_error("YAML error while loading data/misc/skills.yml: " + std::string(e.what()));
    }
}

// Returns the number of Mobiles currently active.
size_t World::mob_count() const { return mobiles_.size(); }

// Checks if a specified mobile ID exists.
bool World::mob_exists(const std::string &str) const { return mob_pool_.count(StrX::hash(str)); }

// Retrieves a Mobile by vector position.
const std::shared_ptr<Mobile> World::mob_vec(size_t vec_pos) const
{
    if (vec_pos >= mobiles_.size()) throw std::runtime_error("Invalid mobile vector position.");
    return mobiles_.at(vec_pos);
}

// Sets up for a new game.
void World::new_game()
{
    player_->set_meta_uint("bones_id", Bones::unique_id());
    player_->set_location("BRASS_DIRK");
    starter_equipment("STARTING_GEAR");
    ActionLook::look();
}

// Retrieves a pointer to the Player object.
const std::shared_ptr<Player> World::player() const { return player_; }

// Recalculates the list of active rooms.
void World::recalc_active_rooms()
{
    std::set<uint32_t> old_active_rooms = active_rooms_;   // Make a copy of the currently active rooms, so we can see what changed.
    active_rooms_.clear();
    active_room_scan(player()->location(), 0);

    // Ping any rooms that have become active.
    for (auto room : active_rooms_)
        if (!old_active_rooms.count(room)) get_room(room)->activate();

    // Ping any rooms that have become inactive.
    for (auto room : old_active_rooms)
        if (!active_rooms_.count(room)) get_room(room)->deactivate();
}

// Removes a Mobile from the world.
void World::remove_mobile(size_t id)
{
    for (size_t i = 0; i < mobiles_.size(); i++)
    {
        if (mobiles_.at(i)->id() == id)
        {
            mobiles_.erase(mobiles_.begin() + i);
            return;
        }
    }
    core()->guru()->nonfatal("Attempt to remove mobile that does not exist in the world.", Guru::GURU_ERROR);
}

// Checks if a room is currently active.
bool World::room_active(uint32_t id) const { return active_rooms_.count(id); }

// Checks if a specified room ID exists.
bool World::room_exists(const std::string &str) const { return room_pool_.count(StrX::hash(str)); }

// Saves the World and all things within it.
void World::save(std::shared_ptr<SQLite::Database> save_db)
{
    save_db->exec(Buff::SQL_BUFFS);
    save_db->exec(Item::SQL_ITEMS);
    save_db->exec(MessageLog::SQL_MSGLOG);
    save_db->exec(Mobile::SQL_MOBILES);
    save_db->exec(Player::SQL_PLAYER);
    save_db->exec(Player::SQL_SKILLS);
    save_db->exec(Room::SQL_ROOMS);
    save_db->exec(Shop::SQL_SHOPS);
    save_db->exec(TimeWeather::SQL_HEARTBEATS);
    save_db->exec(TimeWeather::SQL_TIME_WEATHER);
    save_db->exec(SQL_WORLD);

    SQLite::Statement query(*save_db, "INSERT INTO world ( mob_unique_id ) VALUES ( :mob_unique_id )");
    query.bind(":mob_unique_id", mob_unique_id_);
    query.exec();

    player_->save(save_db);
    core()->messagelog()->save(save_db);
    time_weather_->save(save_db);

    for (auto room : room_pool_)
    {
        // Temporarily tag the room with SaveActive, if it's in the active rooms list.
        const bool is_active = room_active(room.first);
        if (is_active) room.second->set_tag(RoomTag::SaveActive);
        room.second->save(save_db);
        if (is_active) room.second->clear_tag(RoomTag::SaveActive);
    }

    for (auto mob : mobiles_)
        mob->save(save_db);

    for (auto shop : shops_)
        shop.second->save(save_db);
}

// Assigns the player starter equipment from a list.
void World::starter_equipment(const std::string &list_name)
{
    const auto list = get_list(list_name);
    for (size_t i = 0; i < list->size(); i++)
    {
        const auto item = get_item(list->at(i).str, list->at(i).count);
        if (item->type() == ItemType::WEAPON || item->type() == ItemType::ARMOUR || item->type() == ItemType::SHIELD)
        {
            if (item->type() == ItemType::SHIELD) item->set_equip_slot(EquipSlot::HAND_OFF);
            player()->equ()->add_item(item);
        }
        else player()->inv()->add_item(item);
    }
}

// Gets a pointer to the TimeWeather object.
const std::shared_ptr<TimeWeather> World::time_weather() const { return time_weather_; }
