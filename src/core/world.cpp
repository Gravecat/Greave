// core/world.cpp -- The World class defines the game world as a whole and handles the passage of time, as well as keeping track of the player's current activities.
// Copyright (c) 2020-2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#include "uni/uni-core.hpp"


// Constructor, sets default values.
World::World() { }

// The main game loop.
void World::main_loop()
{
    // bröther may i have some lööps
    while (true)
    {
        const std::string input = core()->messagelog()->render_message_log();
    }
}
