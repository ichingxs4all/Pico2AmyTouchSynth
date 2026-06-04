// menu.ino — Effect-parameter menu
//
// Enter / exit: double-click the button from play mode.
// Inside the menu:
//   Left  knob  – scroll items  (ADC → item index)
//   Click       – enter / exit edit for the current item
//   Right knob  – change value while editing  (relative delta, no jump)
//
// ── Adding a new parameter ────────────────────────────────────────────────────
//   1. Add a storage variable if needed.
//   2. Write an apply() helper.
//   3. Append one row to MENU_ITEMS[] and bump NUM_MENU_ITEMS.

#include "menu.h"
#include "effects.h"

// ── Storage for parameters not already tracked as globals ─────────────────────
static float mChorusDelay    = 320.0f;
static float mChorusFreq     = 0.5f;
static float mChorusDepth    = 0.5f;
static float mReverbLiveness = 0.85f;
static float mReverbDamping  = 0.5f;
static float mReverbXover    = 3000.0f;

// ── Effect apply helpers ──────────────────────────────────────────────────────
static void applyChorusLevel(float v)    { chorusLevel = v; amy_set_chorus_level(v); }
static void applyChorusDelay(float v)    { mChorusDelay = v;    amy_event e = amy_default_event(); e.chorus_max_delay = (int)v;  amy_add_event(&e); }
static void applyChorusFreq(float v)     { mChorusFreq  = v;    amy_event e = amy_default_event(); e.chorus_lfo_freq  = v;       amy_add_event(&e); }
static void applyChorusDepth(float v)    { mChorusDepth = v;    amy_event e = amy_default_event(); e.chorus_depth     = v;       amy_add_event(&e); }
static void applyEchoLevel(float v)      { echoLevel  = v; amy_set_echo_level(v); }
static void applyReverbLevel(float v)    { reverbLevel = v; amy_set_reverb_level(v); }
static void applyReverbLiveness(float v) { mReverbLiveness = v; amy_event e = amy_default_event(); e.reverb_liveness = v;        amy_add_event(&e); }
static void applyReverbDamping(float v)  { mReverbDamping  = v; amy_event e = amy_default_event(); e.reverb_damping  = v;        amy_add_event(&e); }
static void applyReverbXover(float v)    { mReverbXover    = v; amy_event e = amy_default_event(); e.reverb_xover_hz = (int)v;   amy_add_event(&e); }

// ── Knob-assignment helpers ───────────────────────────────────────────────────
static void applyLeftKnobAssign (float v) { leftKnobAssign  = v; saveSettings(); }
static void applyRightKnobAssign(float v) { rightKnobAssign = v; saveSettings(); }

// ── Restore-defaults item ─────────────────────────────────────────────────────
static float     mRestoreDummy   = 0.0f;
static const char *kRestoreNames[] = { "No", "Yes - confirm" };
static void applyRestore(float v) {
    if (v >= 0.5f) {
        resetToDefaults();
        mRestoreDummy = 0.0f;
    }
}

// ── Menu table ────────────────────────────────────────────────────────────────
// Items 0-1 are knob assignments; items 2-8 are effect params; item 9 is restore.
// To add a new effect: append a row and bump NUM_MENU_ITEMS.
#define NUM_MENU_ITEMS 10

static MenuItem MENU_ITEMS[NUM_MENU_ITEMS] = {
    //  label             value ptr          min   max                  isInt  names           apply
    { "Left Knob",       &leftKnobAssign,   0,    KNOB_FUNC_COUNT-1,   true,  kKnobFuncNames, applyLeftKnobAssign  },
    { "Right Knob",      &rightKnobAssign,  0,    KNOB_FUNC_COUNT-1,   true,  kKnobFuncNames, applyRightKnobAssign },
    { "Chorus Level",    &chorusLevel,      0,    1,                   false, nullptr,         applyChorusLevel     },
    { "Chorus Delay",    &mChorusDelay,     0,    800,                 true,  nullptr,         applyChorusDelay     },
    { "Chorus Freq",     &mChorusFreq,      0,    5,                   false, nullptr,         applyChorusFreq      },
    { "Chorus Depth",    &mChorusDepth,     0,    1,                   false, nullptr,         applyChorusDepth     },
    { "Echo Level",      &echoLevel,        0,    1,                   false, nullptr,         applyEchoLevel       },
    { "Reverb Level",    &reverbLevel,      0,    1,                   false, nullptr,         applyReverbLevel     },
    { "Reverb Liveness", &mReverbLiveness,  0,    1,                   false, nullptr,         applyReverbLiveness  },
    { "Restore Defaults",&mRestoreDummy,    0,    1,                   true,  kRestoreNames,   applyRestore         },
};

// ── Menu navigation state ─────────────────────────────────────────────────────
// menuMode / menuItemIdx / menuEditing are declared in Pico2AmyTouchSynth.ino.

static int   menuLastLeftIdx = -1;
static int   menuAnchorKnob  = 0;
static float menuAnchorVal   = 0.0f;

// ── Enter / exit ──────────────────────────────────────────────────────────────
void menuEnter() {
    menuMode        = true;
    menuEditing     = false;
    menuItemIdx     = 0;
    menuLastLeftIdx = -1;
    displayNeedsUpdate = true;
}

void menuExit() {
    menuMode    = false;
    menuEditing = false;
    saveSettings();
    displayNeedsUpdate = true;
    updateDisplay();
}

// ── Click: toggle edit ────────────────────────────────────────────────────────
void menuClick(int currentRightKnob) {
    if (!menuEditing) {
        menuEditing   = true;
        menuAnchorKnob = currentRightKnob;
        menuAnchorVal  = *MENU_ITEMS[menuItemIdx].value;
    } else {
        menuEditing = false;
    }
    displayNeedsUpdate = true;
}

// ── Knob update ───────────────────────────────────────────────────────────────
void menuUpdate(int smoothedLeft, int smoothedRight) {
    // Left → scroll items
    int idx = map(smoothedLeft, 0, ADC_MAX, 0, NUM_MENU_ITEMS);
    idx = constrain(idx, 0, NUM_MENU_ITEMS - 1);
    if (idx != menuLastLeftIdx) {
        if (menuEditing) menuEditing = false;
        menuItemIdx     = idx;
        menuLastLeftIdx = idx;
        displayNeedsUpdate = true;
    }

    // Right → change value (relative delta from anchor)
    if (menuEditing) {
        MenuItem &item  = MENU_ITEMS[menuItemIdx];
        float range  = item.maxVal - item.minVal;
        float delta  = (float)(smoothedRight - menuAnchorKnob) / (float)ADC_MAX * range;
        float newVal = menuAnchorVal + delta;
        if (item.isInt) newVal = roundf(newVal);
        newVal = constrain(newVal, item.minVal, item.maxVal);
        float thresh = item.isInt ? 0.5f : FX_LEVEL_CHANGE_THRESHOLD;
        if (fabsf(newVal - *item.value) >= thresh) {
            *item.value = newVal;
            item.apply(newVal);
            displayNeedsUpdate = true;
        }
    }
}

// ── OLED menu screen ──────────────────────────────────────────────────────────
void drawMenuDisplay() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    MenuItem &item = MENU_ITEMS[menuItemIdx];

    // Header
    display.setCursor(0, 0);
    display.print("MENU");
    if (menuEditing) display.print(" [EDIT]");
    char ctr[8];
    snprintf(ctr, sizeof(ctr), "%d/%d", menuItemIdx + 1, NUM_MENU_ITEMS);
    display.setCursor(OLED_WIDTH - (int)strlen(ctr) * 6, 0);
    display.print(ctr);

    display.drawFastHLine(0, 9, OLED_WIDTH, SSD1306_WHITE);

    // Item label
    display.setCursor(0, 11);
    display.print(item.label);

    // Value bar
    float norm = (*item.value - item.minVal) / (item.maxVal - item.minVal);
    norm = constrain(norm, 0.0f, 1.0f);
    int filled = (int)(norm * (OLED_WIDTH - 2));
    display.drawRect(0, 23, OLED_WIDTH, 9, SSD1306_WHITE);
    if (filled > 0) display.fillRect(1, 24, filled, 7, SSD1306_WHITE);

    // Numeric / named value
    display.setCursor(0, 35);
    if (item.valueNames != nullptr) {
        int vi = constrain((int)*item.value, 0, (int)item.maxVal);
        display.print(item.valueNames[vi]);
    } else if (item.isInt) {
        display.print((int)*item.value);
    } else {
        display.print(*item.value, 2);
    }

    display.drawFastHLine(0, 45, OLED_WIDTH, SSD1306_WHITE);

    // Footer
    display.setCursor(0, 47);
    display.print(menuEditing ? "R:change  B:done" : "L:scroll  B:edit");
    display.setCursor(0, 56);
    display.print("dbl-click: exit menu");

    display.display();
    displayNeedsUpdate = false;
}
