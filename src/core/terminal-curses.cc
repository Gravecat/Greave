// core/terminal-curses.cc -- Terminal interface for PDCurses/NCurses. See core/terminal.hpp for a full description of the Terminal class.
// Copyright (c) 2020-2021 Raine "Gravecat" Simmons. Licensed under the GNU Affero General Public License v3 or any later version.

#ifdef GREAVE_INCLUDE_CURSES

#ifdef GREAVE_TOLK
#include "3rdparty/Tolk/Tolk.h"
#endif
#include "core/core.h"
#include "core/core-constants.h"
#include "core/strx.h"
#include "core/terminal-curses.h"

// Include curses.h *last*. Reason being, it attempts to re-#define OK (which has already been defined by SQLiteCpp, which ends up being included by some previous headers). Because Curses #undefines it first and SQLiteCpp doesn't, it's okay to allow SQLiteCpp to define it first, then for Curses to override it -- they both #define it to 0, but more importantly, SQLiteCpp isn't even *used* in terminal-curses.cc aside from its header being included somewhere along the chains of includes above.
// tl;dr: curses.h must be included last here for reasons.
#ifdef GREAVE_TARGET_WINDOWS
#include "3rdparty/PDCurses/curses.h"
#else
#include <curses.h>
#endif


// Constructor, sets up Curses.
TerminalCurses::TerminalCurses()
{
    const std::shared_ptr<Prefs> prefs = core()->prefs();
#ifdef GREAVE_TARGET_WINDOWS
    core()->guru()->log("Setting up PDCurses.");
#else
    core()->guru()->log("Setting up NCurses.");
#endif
    if (!initscr()) throw std::runtime_error("Could not initialize Curses terminal!");
    noecho();
    keypad(stdscr, true);
    curs_set(0);
    start_color();
    if (!can_change_color()) prefs->curses_custom_colours = false;
    if (prefs->curses_custom_colours)
    {
        auto redefine_colour = [this](int colour, std::string value) {
            short r = 255, g = 255, b = 255;
            decode_hex_colour(value, r, g, b);
            init_color(colour, r, g, b);
        };

        redefine_colour(CUSTOM_BLACK, prefs->colour_black);
        redefine_colour(CUSTOM_GREY_DARK, prefs->colour_grey_dark);
        redefine_colour(CUSTOM_RED, prefs->colour_red);
        redefine_colour(CUSTOM_RED_DARK, prefs->colour_red_dark);
        redefine_colour(CUSTOM_GREEN, prefs->colour_green);
        redefine_colour(CUSTOM_GREEN_DARK, prefs->colour_green_dark);
        redefine_colour(CUSTOM_YELLOW, prefs->colour_yellow);
        redefine_colour(CUSTOM_YELLOW_DARK, prefs->colour_yellow_dark);
        redefine_colour(CUSTOM_BLUE, prefs->colour_blue);
        redefine_colour(CUSTOM_BLUE_DARK, prefs->colour_blue_dark);
        redefine_colour(CUSTOM_MAGENTA, prefs->colour_magenta);
        redefine_colour(CUSTOM_MAGENTA_DARK, prefs->colour_magenta_dark);
        redefine_colour(CUSTOM_CYAN, prefs->colour_cyan);
        redefine_colour(CUSTOM_CYAN_DARK, prefs->colour_cyan_dark);
        redefine_colour(CUSTOM_WHITE, prefs->colour_white);
        redefine_colour(CUSTOM_GREY, prefs->colour_grey);
    }
    init_pair(1, COLOR_BLACK, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_CYAN, COLOR_BLACK);
    init_pair(8, COLOR_WHITE, COLOR_BLACK);
    init_pair(9, COLOR_BLACK, COLOR_WHITE);

    if (prefs->curses_custom_colours)
    {
        init_pair(CUSTOM_BLACK, CUSTOM_BLACK, COLOR_BLACK);
        init_pair(CUSTOM_GREY_DARK, CUSTOM_GREY_DARK, COLOR_BLACK);
        init_pair(CUSTOM_RED, CUSTOM_RED, COLOR_BLACK);
        init_pair(CUSTOM_RED_DARK, CUSTOM_RED_DARK, COLOR_BLACK);
        init_pair(CUSTOM_GREEN, CUSTOM_GREEN, COLOR_BLACK);
        init_pair(CUSTOM_GREEN_DARK, CUSTOM_GREEN_DARK, COLOR_BLACK);
        init_pair(CUSTOM_YELLOW, CUSTOM_YELLOW, COLOR_BLACK);
        init_pair(CUSTOM_YELLOW_DARK, CUSTOM_YELLOW_DARK, COLOR_BLACK);
        init_pair(CUSTOM_BLUE, CUSTOM_BLUE, COLOR_BLACK);
        init_pair(CUSTOM_BLUE_DARK, CUSTOM_BLUE_DARK, COLOR_BLACK);
        init_pair(CUSTOM_MAGENTA, CUSTOM_MAGENTA, COLOR_BLACK);
        init_pair(CUSTOM_MAGENTA_DARK, CUSTOM_MAGENTA_DARK, COLOR_BLACK);
        init_pair(CUSTOM_CYAN, CUSTOM_CYAN, COLOR_BLACK);
        init_pair(CUSTOM_CYAN_DARK, CUSTOM_CYAN_DARK, COLOR_BLACK);
        init_pair(CUSTOM_WHITE, CUSTOM_WHITE, COLOR_BLACK);
        init_pair(CUSTOM_GREY, CUSTOM_GREY, COLOR_BLACK);
        init_pair(CUSTOM_WHITE_BG, CUSTOM_BLACK, CUSTOM_WHITE);
    }

#ifdef GREAVE_TARGET_WINDOWS
    std::string ver_str = CoreConstants::GAME_VERSION;
    PDC_set_title(("Greave " + ver_str).c_str());
#endif
}

// Destructor, cleans up Curses.
TerminalCurses::~TerminalCurses()
{
    echo();
    curs_set(1);
    endwin();
}

// Returns the height of a single cell, in pixels. Not used in Curses.
int TerminalCurses::cell_height() const { return 0; }

// Returns a colour pair code.
uint32_t TerminalCurses::colour(Colour col) const
{
    const std::shared_ptr<Prefs> prefs = core()->prefs();
    if (prefs->monochrome_mode) switch(col)
    {
        case Colour::BLACK: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_BLACK) : COLOR_PAIR(1));
        case Colour::BLACK_BOLD: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_GREY_DARK) : COLOR_PAIR(1) | A_BOLD);
        case Colour::RED: case Colour::GREEN: case Colour::YELLOW: case Colour::BLUE: case Colour::MAGENTA: case Colour::CYAN: case Colour::WHITE: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_GREY) : COLOR_PAIR(8));
        case Colour::RED_BOLD: case Colour::GREEN_BOLD: case Colour::YELLOW_BOLD: case Colour::BLUE_BOLD: case Colour::MAGENTA_BOLD: case Colour::CYAN_BOLD: case Colour::WHITE_BOLD: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_WHITE) :COLOR_PAIR(8) | A_BOLD);
        case Colour::WHITE_BG: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_WHITE_BG) : COLOR_PAIR(9) | A_BOLD);
        default: return 0;  // This should be impossible, but keeps the compiler happy.
    }
    else switch(col)
    {
        case Colour::BLACK: return COLOR_PAIR(prefs->curses_custom_colours ? CUSTOM_BLACK : 1);
        case Colour::BLACK_BOLD: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_GREY_DARK) : COLOR_PAIR(1) | A_BOLD);
        case Colour::RED: return COLOR_PAIR(prefs->curses_custom_colours ? CUSTOM_RED_DARK : 2);
        case Colour::RED_BOLD: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_RED) : COLOR_PAIR(2) | A_BOLD);
        case Colour::GREEN: return COLOR_PAIR(prefs->curses_custom_colours ? CUSTOM_GREEN_DARK : 3);
        case Colour::GREEN_BOLD: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_GREEN) : COLOR_PAIR(3) | A_BOLD);
        case Colour::YELLOW: return COLOR_PAIR(prefs->curses_custom_colours ? CUSTOM_YELLOW_DARK : 4);
        case Colour::YELLOW_BOLD: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_YELLOW) : COLOR_PAIR(4) | A_BOLD);
        case Colour::BLUE: return COLOR_PAIR(prefs->curses_custom_colours ? CUSTOM_BLUE_DARK : 5);
        case Colour::BLUE_BOLD: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_BLUE) : COLOR_PAIR(5) | A_BOLD);
        case Colour::MAGENTA: return COLOR_PAIR(prefs->curses_custom_colours ? CUSTOM_MAGENTA_DARK : 6);
        case Colour::MAGENTA_BOLD: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_MAGENTA) : COLOR_PAIR(6) | A_BOLD);
        case Colour::CYAN: return COLOR_PAIR(prefs->curses_custom_colours ? CUSTOM_CYAN_DARK : 7);
        case Colour::CYAN_BOLD: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_CYAN) : COLOR_PAIR(7) | A_BOLD);
        case Colour::WHITE: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_GREY) : COLOR_PAIR(8));
        case Colour::WHITE_BOLD: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_WHITE) : COLOR_PAIR(8) | A_BOLD);
        case Colour::WHITE_BG: return (prefs->curses_custom_colours ? COLOR_PAIR(CUSTOM_WHITE_BG) : COLOR_PAIR(9));
        default: return 0;  // This should be impossible, but keeps the compiler happy.
    }
}

// Clears the screen.
void TerminalCurses::cls() { clear(); }

// Makes the cursor visible or invisible.
void TerminalCurses::cursor(bool visible) { curs_set(visible ? 1 : 0); }

// Decodes a hex-code colour into RGB values.
void TerminalCurses::decode_hex_colour(const std::string &col, short &r, short &g, short &b) const
{
    if (col.size() != 6) return;
    r = StrX::htoi(col.substr(0, 2)) * 3.92f;
    g = StrX::htoi(col.substr(2, 2)) * 3.92f;
    b = StrX::htoi(col.substr(4, 2)) * 3.92f;
}

// Fills a given area in with the specified colour. Currently nonfunctional on Curses.
void TerminalCurses::fill(int, int, int, int, Colour) { }

// Not currently supported by the Curses interface.
int TerminalCurses::get_mouse_x() const { return 0; }

// Not currently supported by the Curses interface.
int TerminalCurses::get_mouse_x_pixel() const { return 0; }

// Not currently supported by the Curses interface.
int TerminalCurses::get_mouse_y() const { return 0; }

// Not currently supported by the Curses interface.
int TerminalCurses::get_mouse_y_pixel() const { return 0; }

// Gets keyboard input from the terminal.
int TerminalCurses::get_key()
{
    int key = getch();
    switch(key)
    {
        case 3: case 4: return Key::CLOSE;      // Ctrl-C or Ctrl-D close the console window.
        case KEY_RESIZE: return Key::RESIZED;   // Window resized event.
        case KEY_UP: return Key::ARROW_UP;
        case KEY_DOWN: return Key::ARROW_DOWN;
        case KEY_LEFT: return Key::ARROW_LEFT;
        case KEY_RIGHT: return Key::ARROW_RIGHT;
        case KEY_HOME: return Key::HOME;
        case KEY_END: return Key::END;
        case KEY_PPAGE: return Key::PAGE_UP;
        case KEY_NPAGE: return Key::PAGE_DOWN;
        case KEY_BACKSPACE: return Key::BACKSPACE;
#ifdef GREAVE_TOLK
        case Key::ESCAPE: Tolk_Silence(); break;
#endif
    }

    if (key > 255 || key < 0) return -1;    // Any other unrecognized keys are just returned as -1.
    return key;
}

// Retrieves the size of the terminal (in cells, not pixels).
void TerminalCurses::get_size(int *w, int *h) const
{
    *w = getmaxx(stdscr);
    *h = getmaxy(stdscr);
}

// Moves the cursor to the specified position.
void TerminalCurses::move_cursor(int x, int y) { move(y, x); }

// Prints a string at a given coordinate on the screen.
void TerminalCurses::print_internal(std::string str, int x, int y, Colour col)
{
    const unsigned long ansi_code = colour(col);
    StrX::find_and_replace(str, "%", "%%");
    attron(ansi_code);
    mvprintw(y, x, str.c_str());
    attroff(ansi_code);
    return;
}

// Prints a character at a given coordinate on the screen.
void TerminalCurses::put(uint16_t letter, int x, int y, Colour col)
{
    if (letter > 255) letter = '?';
    const unsigned long ansi_code = colour(col);
    attron(ansi_code);
    mvaddch(y, x, letter);
    attroff(ansi_code);
}

// Refreshes the screen with changes made.
void TerminalCurses::refresh() { ::refresh(); }

// Returns true if the player uses Ctrl-C, Ctrl-D or escape.
bool TerminalCurses::wants_to_close() const
{
    const char ch = getch();
    return (ch == 3 || ch == 4 || ch == 27);
}
#endif
