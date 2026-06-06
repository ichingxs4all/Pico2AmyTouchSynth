#pragma once

// menu.ino — Effect-parameter menu
//
// Accessed by double-clicking the button from play mode.
// Double-click again to exit.
//
// Inside the menu:
//   Left  knob  – scroll through items  (ADC position → item index)
//   Button click – enter / exit edit for the current item
//   Right knob  – change value while editing  (relative delta, no jump on entry)
//
// ── Adding a new parameter ────────────────────────────────────────────────────
//   1. Declare a float variable below (or reuse an existing global).
//   2. Write a one-line apply() helper that sends it to AMY.
//   3. Append one row to MENU_ITEMS[].  Bump NUM_MENU_ITEMS by 1.
//   Nothing else needs to change.

#include "menu.h"
#include "effects.h"

// ── Storage for parameters not already tracked as globals ─────────────────────
static float mChorusDelay    = 320.0f;
static float mChorusFreq     = 0.5f;
static float mChorusDepth    = 0.5f;
static float mReverbLiveness = 0.85f;
static float mReverbDamping  = 0.5f;
static float mReverbXover    = 3000.0f;

// ── Apply helpers ─────────────────────────────────────────────────────────────
static void applyChorusLevel(float v) {
    chorusLevel = v;
    amy_set_chorus_level(v);
}
static void applyChorusDelay(float v) {
    mChorusDelay = v;
    amy_event e = amy_default_event(); e.chorus_max_delay = (int)v; amy_add_event(&e);
}
static void applyChorusFreq(float v) {
    mChorusFreq = v;
    amy_event e = amy_default_event(); e.chorus_lfo_freq = v; amy_add_event(&e);
}
static void applyChorusDepth(float v) {
    mChorusDepth = v;
    amy_event e = amy_default_event(); e.chorus_depth = v; amy_add_event(&e);
}
static void applyEchoLevel(float v) {
    echoLevel = v;
    amy_set_echo_level(v);
}
static void applyReverbLevel(float v) {
    reverbLevel = v;
    amy_set_reverb_level(v);
}
static void applyReverbLiveness(float v) {
    mReverbLiveness = v;
    amy_event e = amy_default_event(); e.reverb_liveness = v; amy_add_event(&e);
}
static void applyReverbDamping(float v) {
    mReverbDamping = v;
    amy_event e = amy_default_event(); e.reverb_damping = v; amy_add_event(&e);
}
static void applyReverbXover(float v) {
    mReverbXover = v;
    amy_event e = amy_default_event(); e.reverb_xover_hz = (int)v; amy_add_event(&e);
}

// ── Menu table ────────────────────────────────────────────────────────────────
// To add a new item: append a row and bump NUM_MENU_ITEMS.
#define NUM_MENU_ITEMS 9
static MenuItem MENU_ITEMS[NUM_MENU_ITEMS] = {
    // label             value ptr       min     max      isInt  apply fn
    { "Chorus Level",   &chorusLevel,   0.0f,   1.0f,    false, applyChorusLevel    },
    { "Chorus Delay",   &mChorusDelay,  0.0f,   800.0f,  true,  applyChorusDelay    },
    { "Chorus Freq",    &mChorusFreq,   0.0f,   5.0f,    false, applyChorusFreq     },
    { "Chorus Depth",   &mChorusDepth,  0.0f,   1.0f,    false, applyChorusDepth    },
    { "Echo Level",     &echoLevel,     0.0f,   1.0f,    false, applyEchoLevel      },
    { "Reverb Level",   &reverbLevel,   0.0f,   1.0f,    false, applyReverbLevel    },
    { "Reverb Live",    &mReverbLiveness,0.0f,  1.0f,    false, applyReverbLiveness },
    { "Reverb Damp",    &mReverbDamping, 0.0f,  1.0f,    false, applyReverbDamping  },
    { "Reverb Xover",   &mReverbXover,  200.0f, 8000.0f, true,  applyReverbXover    },
};

// ── Menu state ────────────────────────────────────────────────────────────────
// menuMode / menuItemIdx / menuEditing are declared in Pico2AmyTouchSynth.ino

static int   menuLastLeftIdx    = -1;  // last mapped item index; -1 forces first read
static int   menuAnchorKnob     = 0;   // right-knob ADC value when edit was entered
static float menuAnchorVal      = 0.0f;

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
    updateDisplay();
}

// ── Click inside the menu: enter or exit edit for current item ────────────────
void menuClick(int currentRightKnob) {
    if (!menuEditing) {
        menuEditing    = true;
        menuAnchorKnob = currentRightKnob;
        menuAnchorVal  = *MENU_ITEMS[menuItemIdx].value;
    } else {
        menuEditing = false;
    }
    displayNeedsUpdate = true;
}

// ── Knob update (called from updateButtonKnobs every loop tick) ───────────────
void menuUpdate(int smoothedLeft, int smoothedRight) {
    // Left knob → scroll items
    int idx = map(smoothedLeft, 0, ADC_MAX, 0, NUM_MENU_ITEMS);
    idx = constrain(idx, 0, NUM_MENU_ITEMS - 1);
    if (idx != menuLastLeftIdx) {
        if (menuEditing) menuEditing = false;  // scrolling away cancels edit
        menuItemIdx     = idx;
        menuLastLeftIdx = idx;
        displayNeedsUpdate = true;
    }

    // Right knob → change value while editing (relative delta, no jump)
    if (menuEditing) {
        MenuItem &item = MENU_ITEMS[menuItemIdx];
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
// Layout (128×64, textSize 1 = 6×8 px):
//
//  y= 0  MENU [EDIT]            3/9
//  y= 9  ────────────────────────────
//  y=11  item label
//  y=23  [████████████░░░░░░░░░░░░]   full-width value bar, height 9
//  y=35  value as number
//  y=45  ────────────────────────────
//  y=47  L:scroll  B:edit/done
//  y=56  dbl-click: exit menu

void drawMenuDisplay() {
    display.clearDisplay();

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

    // Value bar (full width)
    float norm = (*item.value - item.minVal) / (item.maxVal - item.minVal);
    norm = constrain(norm, 0.0f, 1.0f);
    int filled = (int)(norm * (OLED_WIDTH - 2));
    display.drawRect(0, 23, OLED_WIDTH, 9, SSD1306_WHITE);
    if (filled > 0) display.fillRect(1, 24, filled, 7, SSD1306_WHITE);

    // Numeric value
    display.setCursor(0, 35);
    if (item.isInt) {
        display.print((int)*item.value);
    } else {
        display.print(*item.value, 2);
    }

    display.drawFastHLine(0, 45, OLED_WIDTH, SSD1306_WHITE);

    // Footer hints
    display.setCursor(0, 47);
    display.print(menuEditing ? "R:change  B:done" : "L:scroll  B:edit");
    display.setCursor(0, 56);
    display.print("dbl-click: exit menu");

    display.display();
    displayNeedsUpdate = false;
}
