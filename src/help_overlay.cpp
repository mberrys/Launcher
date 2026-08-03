// help_overlay.cpp
//
// Draws the `opt` cheat sheet. See help_overlay.h for the contract and
// help_content.cpp for the text.
//
// Two constraints shape the drawing code. The tft wrapper in src/tft.h has no
// portable clipping or off-screen sprite support across the ~40 display drivers
// this firmware builds for, so the panel scrolls a whole row at a time rather
// than by pixels -- that way no glyph is ever half-drawn at a viewport edge.
// And nothing here may allocate: the row list is a fixed static array.
#include "help_overlay.h"

#include "display.h"
#include "idf/launcher_platform.h"
#include "powerSave.h"
#include <cstring>
#include <globals.h>

namespace {

constexpr uint8_t kMaxDepth = 6;
const HelpScreen *sStack[kMaxDepth] = {nullptr};
uint8_t sDepth = 0;

} // namespace

void helpPushScreen(const HelpScreen *screen) {
    if (sDepth < kMaxDepth) sStack[sDepth] = screen;
    // Keep counting past the array so pops stay balanced; deeper screens just
    // reuse the deepest one we had room to record.
    if (sDepth < 0xFF) sDepth++;
}

void helpPopScreen() {
    if (sDepth > 0) sDepth--;
}

#if defined(HEADLESS)

bool helpOverlayCheck() { return false; }

#else

namespace {

const HelpScreen *currentScreen() {
    if (sDepth == 0) return &kHelpGlobal;
    const uint8_t idx = sDepth <= kMaxDepth ? sDepth : kMaxDepth;
    const HelpScreen *screen = sStack[idx - 1];
    return screen ? screen : &kHelpGlobal;
}

constexpr int kRowPitch = 12; // FP text is LH (8) tall; 12 gives it room to breathe
constexpr int kKeyCols = 11;  // width of the key column, in characters
constexpr int kPadX = 6;      // panel edge to text
constexpr int kGutter = 6;    // right-hand strip reserved for the scrollbar

constexpr uint32_t kTickMs = 40;       // overlay frame time
constexpr uint32_t kDwellMs = 1600;    // pause at the top and bottom of a long list
constexpr uint32_t kStepMs = 900;      // time each intermediate row stays put
constexpr uint32_t kMaxHoldMs = 30000; // give up if a release event is ever missed

enum RowType : uint8_t {
    RowHeader, // a section title
    RowEntry,  // first line of an entry: keys + description
    RowWrap,   // continuation of the description above
};

struct Row {
    const char *text;
    const char *keys;
    uint8_t len;
    uint8_t type;
};

constexpr uint8_t kMaxRows = 48;
Row sRows[kMaxRows];

struct Layout {
    int panelX, panelY, panelW, panelH;
    int textLeft, descX, contentRight;
    int bodyY, bodyH;
    int titleSize;
    uint8_t descCols, keyCols, rowsPerPage;
};

// Splits `desc` across as many rows as it takes to fit `cols` characters,
// breaking on spaces where possible. Only the first row carries the keys.
uint8_t appendWrapped(uint8_t n, const char *keys, const char *desc, uint8_t cols) {
    const char *p = desc;
    bool first = true;

    while (n < kMaxRows) {
        while (*p == ' ') p++;
        if (*p == '\0') break;

        size_t take = strlen(p);
        if (take > cols) {
            take = cols;
            size_t brk = take;
            while (brk > 0 && p[brk] != ' ') brk--;
            if (brk > 0) take = brk; // a single word longer than the column is hard-broken
        }

        sRows[n].type = first ? RowEntry : RowWrap;
        sRows[n].keys = first ? keys : "";
        sRows[n].text = p;
        sRows[n].len = static_cast<uint8_t>(take);
        n++;
        p += take;
        first = false;
    }

    // An entry with keys but no description still deserves its line.
    if (first && n < kMaxRows && keys && keys[0] != '\0') {
        sRows[n].type = RowEntry;
        sRows[n].keys = keys;
        sRows[n].text = "";
        sRows[n].len = 0;
        n++;
    }
    return n;
}

uint8_t buildRows(const HelpScreen *screen, uint8_t descCols) {
    uint8_t n = 0;
    for (uint8_t s = 0; s < screen->count && n < kMaxRows; s++) {
        const HelpSection &section = screen->sections[s];
        if (section.title && section.title[0] != '\0' && n < kMaxRows) {
            sRows[n].type = RowHeader;
            sRows[n].keys = "";
            sRows[n].text = section.title;
            sRows[n].len = static_cast<uint8_t>(strlen(section.title));
            n++;
        }
        for (uint8_t e = 0; e < section.count && n < kMaxRows; e++) {
            n = appendWrapped(n, section.entries[e].keys, section.entries[e].desc, descCols);
        }
    }
    return n;
}

Layout computeLayout(const HelpScreen *screen) {
    Layout l;
    l.panelX = 3;
    l.panelY = 3;
    l.panelW = tftWidth - 6;
    l.panelH = tftHeight - 6;

    l.textLeft = l.panelX + kPadX;
    l.contentRight = l.panelX + l.panelW - kPadX - kGutter;

    const int usable = l.contentRight - l.textLeft;
    int keyW = kKeyCols * LW;
    if (keyW > usable / 2) keyW = usable / 2;
    if (keyW < 3 * LW) keyW = 3 * LW;
    l.keyCols = static_cast<uint8_t>(keyW / LW);
    l.descX = l.textLeft + keyW + 8; // gap wide enough to read as two columns

    int cols = (l.contentRight - l.descX) / LW;
    if (cols < 4) cols = 4;
    if (cols > 60) cols = 60;
    l.descCols = static_cast<uint8_t>(cols);

    // Big title when it fits, small one when it doesn't.
    const int titleW = static_cast<int>(strlen(screen->title)) * LW * FM;
    l.titleSize = titleW <= (l.panelW - 2 * kPadX) ? FM : FP;

    l.bodyY = l.panelY + 5 + LH * l.titleSize + 6;
    l.bodyH = (l.panelY + l.panelH) - 5 - l.bodyY;
    int rows = l.bodyH / kRowPitch;
    if (rows < 1) rows = 1;
    l.rowsPerPage = static_cast<uint8_t>(rows);
    return l;
}

void drawFrame(const Layout &l, const HelpScreen *screen) {
    tft->fillScreen(BGCOLOR);
    tft->drawRoundRect(l.panelX, l.panelY, l.panelW, l.panelH, 5, FGCOLOR);

    tft->setTextSize(l.titleSize);
    tft->setTextColor(FGCOLOR, BGCOLOR);
    tft->drawString(screen->title, l.textLeft, l.panelY + 5);

    tft->fillRect(l.textLeft, l.bodyY - 5, l.contentRight - l.textLeft, 1, ALCOLOR);
}

void drawBody(const Layout &l, uint8_t total, uint8_t top) {
    tft->fillRect(l.panelX + 1, l.bodyY, l.panelW - 2, l.bodyH, BGCOLOR);

    const uint16_t descColor = getComplementaryColor(BGCOLOR);
    const uint16_t dimColor = FGCOLOR - 0x2222;

    char text[72];
    char keys[16];

    tft->setTextSize(FP);
    for (uint8_t i = 0; i < l.rowsPerPage && static_cast<uint16_t>(top + i) < total; i++) {
        const Row &row = sRows[top + i];
        const int y = l.bodyY + i * kRowPitch + 2;

        size_t len = row.len;
        if (len >= sizeof(text)) len = sizeof(text) - 1;
        memcpy(text, row.text, len);
        text[len] = '\0';

        if (row.type == RowHeader) {
            tft->setTextColor(ALCOLOR, BGCOLOR);
            tft->drawString(text, l.textLeft, y);
            const int ruleX = l.textLeft + static_cast<int>(len) * LW * FP + 5;
            if (ruleX < l.contentRight) { tft->fillRect(ruleX, y + 3, l.contentRight - ruleX, 1, dimColor); }
            continue;
        }

        if (row.keys[0] != '\0') {
            size_t klen = strlen(row.keys);
            if (klen > l.keyCols) klen = l.keyCols;
            if (klen >= sizeof(keys)) klen = sizeof(keys) - 1;
            memcpy(keys, row.keys, klen);
            keys[klen] = '\0';
            tft->setTextColor(FGCOLOR, BGCOLOR);
            tft->drawString(keys, l.textLeft, y);
        }
        tft->setTextColor(descColor, BGCOLOR);
        tft->drawString(text, l.descX, y);
    }

    if (total <= l.rowsPerPage) return;

    // Scrollbar, only worth drawing when there is something below the fold.
    const int barX = l.panelX + l.panelW - 6;
    tft->fillRect(barX, l.bodyY, 2, l.bodyH, dimColor);
    int thumbH = l.bodyH * l.rowsPerPage / total;
    if (thumbH < 8) thumbH = 8;
    if (thumbH > l.bodyH) thumbH = l.bodyH;
    const int steps = total - l.rowsPerPage;
    const int thumbY = l.bodyY + (steps > 0 ? (l.bodyH - thumbH) * top / steps : 0);
    tft->fillRect(barX, thumbY, 2, thumbH, ALCOLOR);
}

} // namespace

bool helpOverlayCheck() {
    if (!OptHeld) return false;

    const HelpScreen *screen = currentScreen();
    const Layout l = computeLayout(screen);
    const uint8_t total = buildRows(screen, l.descCols);

    uint8_t top = 0;
    drawFrame(l, screen);
    drawBody(l, total, top);
    tft->display(false);

    const uint32_t startedAt = launcherMillis();
    uint32_t nextStep = startedAt + kDwellMs;
    while (OptHeld) {
        // A dropped key-up event would otherwise trap the device in here.
        if (launcherMillis() - startedAt > kMaxHoldMs) break;
        wakeUpScreen(); // the panel is meant to be read; don't let the screen dim
#ifdef DONT_USE_INPUT_TASK
        InputHandler();
#endif
        if (total > l.rowsPerPage) {
            const uint32_t now = launcherMillis();
            if (static_cast<int32_t>(now - nextStep) >= 0) {
                if (top + l.rowsPerPage >= total) {
                    top = 0; // wrapped past the end, start over
                    nextStep = now + kDwellMs;
                } else {
                    top++;
                    nextStep = now + (top + l.rowsPerPage >= total ? kDwellMs : kStepMs);
                }
                drawBody(l, total, top);
                tft->display(false);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(kTickMs));
    }

    // Hand the caller a clean slate. Several redraw paths (drawMainMenu,
    // drawOptions with a border) only paint their own widgets and would otherwise
    // leave the panel's title and rules showing around the edges. There is no
    // framebuffer to restore from, so a bordered pop-up loses whatever backdrop it
    // was floating over -- the same thing loopOptions already does on exit.
    tft->fillScreen(BGCOLOR);

    // Nothing that happened while the panel was up belongs to the screen below.
    resetGlobals();
    return true;
}

#endif // HEADLESS
