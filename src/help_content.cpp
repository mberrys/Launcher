// help_content.cpp
//
// The text shown by the `opt` overlay, one table per screen. Everything here is
// static const: the strings live in flash and nothing is copied to the heap.
//
// Keys are named as they are printed on the Cardputer. The mapping from those
// keys to this firmware's navigation globals lives in
// boards/m5stack-cardputer/interface.cpp: ';' = Up, '.' = Down, ',' = Prev,
// '/' = Next, Enter = Sel, '`' = Esc, Backspace = Esc + del, G0 = Sel.
//
// Keep `keys` to 11 characters or fewer -- that is the width of the key column.
// Descriptions wrap automatically, so they can be as long as they need to be.
#include "help_overlay.h"

#define HELP_SECTION(name, arr) {name, arr, (uint8_t)(sizeof(arr) / sizeof((arr)[0]))}
#define HELP_SCREEN(title, arr) {title, arr, (uint8_t)(sizeof(arr) / sizeof((arr)[0]))}

// ---------------------------------------------------------------- fallback --
static const HelpEntry kGlobalMove[] = {
    {"; .", "Move up / down" },
    {", /", "Previous / next"},
};
static const HelpEntry kGlobalAct[] = {
    {"Enter",    "Select"             },
    {"` or Del", "Back"               },
    {"Hold opt", "Show these controls"},
};
static const HelpSection kGlobalSections[] = {
    HELP_SECTION("Move", kGlobalMove),
    HELP_SECTION("Act", kGlobalAct),
};
const HelpScreen kHelpGlobal = HELP_SCREEN("Controls", kGlobalSections);

// ------------------------------------------------------------- boot screen --
static const HelpEntry kBootLaunch[] = {
    {"1 - 9 , 0", "Boot the app in that slot"},
    {"Any key",   "Boot the current app"     },
};
static const HelpEntry kBootMenu[] = {
    {"Enter",    "Open the Launcher menu"},
    {"Side btn", "Open the Launcher menu"},
};
static const HelpEntry kBootNote[] = {
    {"", "Left alone, the current app boots on its own after a few seconds"},
};
static const HelpSection kBootSections[] = {
    HELP_SECTION("Boot", kBootLaunch),
    HELP_SECTION("Launcher", kBootMenu),
    HELP_SECTION("Note", kBootNote),
};
const HelpScreen kHelpBoot = HELP_SCREEN("Boot Screen", kBootSections);

// -------------------------------------------------------------- home screen --
static const HelpEntry kHomeMove[] = {
    {", /", "Previous / next tile"},
    {"; .", "Up / down a row"     },
};
static const HelpEntry kHomeOpen[] = {
    {"Enter",    "Open the selected tile"},
    {"Side btn", "Same as Enter"         },
};
static const HelpEntry kHomeShortcut[] = {
    {"a - z",
     "Boot the .bin bound to that key. Bind one from the SD browser, or from Settings > Manage shortcuts"},
};
static const HelpSection kHomeSections[] = {
    HELP_SECTION("Move", kHomeMove),
    HELP_SECTION("Open", kHomeOpen),
    HELP_SECTION("Shortcuts", kHomeShortcut),
};
const HelpScreen kHelpHome = HELP_SCREEN("Home", kHomeSections);

// ----------------------------------------------------------- generic menus --
static const HelpEntry kListMove[] = {
    {"; .", "Move up / down"},
    {", /", "Move up / down"},
};
static const HelpEntry kListAct[] = {
    {"Enter",    "Run the highlighted row"},
    {"` or Del", "Back / close"           },
};
static const HelpSection kListSections[] = {
    HELP_SECTION("Move", kListMove),
    HELP_SECTION("Act", kListAct),
};
const HelpScreen kHelpListMenu = HELP_SCREEN("Menu", kListSections);

// -------------------------------------------------------------- SD browser --
static const HelpEntry kSdMove[] = {
    {"; . , /", "Move through the list"},
};
static const HelpEntry kSdOpen[] = {
    {"Enter",      "Open the folder, or pick the file"                       },
    {"Hold Enter", "File actions: install, rename, copy, delete, bind to key"},
};
static const HelpEntry kSdBack[] = {
    {"` or Del", "Up one folder. At the top, back to the menu"},
    {"> Back",   "The last row does the same thing"           },
};
static const HelpSection kSdSections[] = {
    HELP_SECTION("Move", kSdMove),
    HELP_SECTION("Open", kSdOpen),
    HELP_SECTION("Back", kSdBack),
};
const HelpScreen kHelpSdBrowser = HELP_SCREEN("SD Browser", kSdSections);

// ---------------------------------------------------------------- settings --
static const HelpEntry kSettingsMove[] = {
    {"; . , /", "Move through the list"},
};
static const HelpEntry kSettingsAct[] = {
    {"Enter",    "Open the row, or flip an [x] setting on the spot"},
    {"` or Del", "Back to the main menu"                           },
};
static const HelpSection kSettingsSections[] = {
    HELP_SECTION("Move", kSettingsMove),
    HELP_SECTION("Act", kSettingsAct),
};
const HelpScreen kHelpSettings = HELP_SCREEN("Settings", kSettingsSections);

// ------------------------------------------------------------- app actions --
static const HelpEntry kAppMove[] = {
    {"; . , /", "Move through the actions"},
};
static const HelpEntry kAppAct[] = {
    {"Enter",    "Run it: launch, rename, back up, restore or delete the app"},
    {"` or Del", "Cancel"                                                    },
};
static const HelpSection kAppSections[] = {
    HELP_SECTION("Move", kAppMove),
    HELP_SECTION("Act", kAppAct),
};
const HelpScreen kHelpAppActions = HELP_SCREEN("App Actions", kAppSections);

// ----------------------------------------------------------- firmware list --
static const HelpEntry kFwMove[] = {
    {"; . , /", "Move through the firmware"},
};
static const HelpEntry kFwAct[] = {
    {"Enter",    "See the versions for this firmware"},
    {"` or Del", "Back to the main menu"             },
};
static const HelpEntry kFwRows[] = {
    {"", "[Next Page] and [Previous Page] page the list"},
    {"", "[Refine Search] filters it by name"           },
};
static const HelpSection kFwSections[] = {
    HELP_SECTION("Move", kFwMove),
    HELP_SECTION("Act", kFwAct),
    HELP_SECTION("Rows", kFwRows),
};
const HelpScreen kHelpFirmwareList = HELP_SCREEN("OTA List", kFwSections);

// ---------------------------------------------------------- version picker --
static const HelpEntry kVerMove[] = {
    {", /", "Older / newer version"},
};
static const HelpEntry kVerAct[] = {
    {"Enter",    "Install it, download it to SD, or add it to favourites"},
    {"` or Del", "Back to the list"                                      },
};
static const HelpSection kVerSections[] = {
    HELP_SECTION("Choose", kVerMove),
    HELP_SECTION("Act", kVerAct),
};
const HelpScreen kHelpVersionPicker = HELP_SCREEN("Versions", kVerSections);

// -------------------------------------------------------------- text entry --
static const HelpEntry kTextType[] = {
    {"a - z 0 - 9", "Insert that character"              },
    {"shift + key", "Uppercase, or the symbol on the key"},
    {"fn + shift",  "Caps lock on / off"                 },
};
static const HelpEntry kTextEdit[] = {
    {"Del", "Backspace"},
};
static const HelpEntry kTextDone[] = {
    {"Enter",    "Confirm and return"           },
    {"Side btn", "Same as Enter"                },
    {"fn + `",   "Cancel and keep the old value"},
};
static const HelpSection kTextSections[] = {
    HELP_SECTION("Type", kTextType),
    HELP_SECTION("Edit", kTextEdit),
    HELP_SECTION("Finish", kTextDone),
};
const HelpScreen kHelpTextInput = HELP_SCREEN("Text Entry", kTextSections);

// -------------------------------------------------------------- calculator --
static const HelpEntry kCalcType[] = {
    {"0 - 9  .", "Digits and the decimal point"        },
    {"+ - * /",  "Operators"                           },
    {"( )",      "Brackets, on shift + 9 and shift + 0"},
};
static const HelpEntry kCalcAct[] = {
    {"Enter", "Work out the expression"},
    {"Del",   "Backspace"              },
    {"`",     "Exit to the menu"       },
};
static const HelpSection kCalcSections[] = {
    HELP_SECTION("Type", kCalcType),
    HELP_SECTION("Act", kCalcAct),
};
const HelpScreen kHelpCalculator = HELP_SCREEN("Calculator", kCalcSections);

// ------------------------------------------------------------------ web UI --
static const HelpEntry kWebStop[] = {
    {"Enter",    "Stop the server and go back to the menu"},
    {"Side btn", "Same as Enter"                          },
};
static const HelpEntry kWebNote[] = {
    {"", "The address, user and password on screen are what you log in with"},
    {"", "No other key does anything while the server is up"                },
};
static const HelpSection kWebSections[] = {
    HELP_SECTION("Stop", kWebStop),
    HELP_SECTION("Note", kWebNote),
};
const HelpScreen kHelpWebUi = HELP_SCREEN("Web UI", kWebSections);

// ------------------------------------------------------------- USB storage --
static const HelpEntry kMscStop[] = {
    {"` or Del", "Eject and go back to the menu"},
};
static const HelpEntry kMscNote[] = {
    {"", "The SD card is mounted on the host over USB until you leave"},
};
static const HelpSection kMscSections[] = {
    HELP_SECTION("Eject", kMscStop),
    HELP_SECTION("Note", kMscNote),
};
const HelpScreen kHelpMassStorage = HELP_SCREEN("USB Storage", kMscSections);
