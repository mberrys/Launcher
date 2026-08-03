// help_overlay.h
//
// Hold-to-reveal keyboard cheat sheet. While the Cardputer's `opt` key is held
// down (tracked by the board interface in the global OptHeld), a panel is drawn
// over the current screen listing the keyboard controls that are actually live
// on it. Releasing the key drops the panel and the screen underneath repaints.
//
// Screens declare their own content by pushing a HelpScreen for as long as their
// input loop is running -- use HelpScope so the pop happens on every exit path --
// and then call helpOverlayCheck() once per frame from inside that loop.
#ifndef __HELP_OVERLAY_H
#define __HELP_OVERLAY_H

#include <cstdint>

// A single "these keys do this" line. Both strings are static literals; nothing
// here is ever copied to the heap.
struct HelpEntry {
    const char *keys;
    const char *desc;
};

// A titled group of entries, e.g. "Move" / "Open".
struct HelpSection {
    const char *title;
    const HelpEntry *entries;
    uint8_t count;
};

// Everything shown for one screen.
struct HelpScreen {
    const char *title;
    const HelpSection *sections;
    uint8_t count;
};

void helpPushScreen(const HelpScreen *screen);
void helpPopScreen();

// RAII wrapper: push on the way in, pop on every way out (including `goto`s out
// of the loops this firmware is fond of).
struct HelpScope {
    explicit HelpScope(const HelpScreen &screen) { helpPushScreen(&screen); }
    ~HelpScope() { helpPopScreen(); }
    HelpScope(const HelpScope &) = delete;
    HelpScope &operator=(const HelpScope &) = delete;
};

// Cheap no-op unless `opt` is down. When it is down, this blocks drawing and
// auto-scrolling the panel until the key is released, then swallows any pending
// input and returns true so the caller knows to repaint its own screen.
bool helpOverlayCheck();

// Screen content, defined in help_content.cpp.
extern const HelpScreen kHelpGlobal;
extern const HelpScreen kHelpBoot;
extern const HelpScreen kHelpHome;
extern const HelpScreen kHelpListMenu;
extern const HelpScreen kHelpSdBrowser;
extern const HelpScreen kHelpSettings;
extern const HelpScreen kHelpAppActions;
extern const HelpScreen kHelpFirmwareList;
extern const HelpScreen kHelpVersionPicker;
extern const HelpScreen kHelpTextInput;
extern const HelpScreen kHelpCalculator;
extern const HelpScreen kHelpWebUi;
extern const HelpScreen kHelpMassStorage;

#endif
