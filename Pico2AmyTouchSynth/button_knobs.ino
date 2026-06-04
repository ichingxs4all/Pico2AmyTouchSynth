// button_knobs.ino
//
// Button modes (priority order):
//
//   Menu mode  (enter / exit with double-click)
//     Left  knob  – scroll menu items
//     Right knob  – change editing item's value (relative delta)
//     Single click – toggle edit on current item
//     Double-click – exit menu
//
//   Patch-select mode  (entered by holding ≥ BTN_HOLD_MS)
//     Left  knob  – bank
//     Right knob  – slot within bank
//     Release     – load chosen patch
//
//   Play mode  (default)
//     Left  knob  – chorus level
//     Right knob  – reverb level
//     Double-click – enter menu
//
// Button click / double-click / hold detection
// ─────────────────────────────────────────────
// Click     : press+release in < BTN_CLICK_MAX_MS
// Double-click: second press starts within BTN_DBLCLICK_MS of the first release
// Hold      : button held ≥ BTN_HOLD_MS without releasing
//
// BTN_HOLD_MS > BTN_CLICK_MAX_MS so a hold never mis-fires as a click.
// BTN_DBLCLICK_MS > BTN_CLICK_MAX_MS so the double-click window is long
// enough to land a second press even after a slow first release.
//
// btnDoubleClickConsumed suppresses the release of the second click so it
// doesn't accidentally trigger a single-click action.

#define ADC_MAX            4095
#define ADC_SCALE            16
#define KNOB_EMA_ALPHA_NUM    3
#define KNOB_EMA_ALPHA_DEN   20

#define BTN_CLICK_MAX_MS   300   // release before this → click
#define BTN_HOLD_MS        500   // still held after this → patch-select
#define BTN_DBLCLICK_MS    400   // second press within this of first release → double-click

// ── EMA state ─────────────────────────────────────────────────────────────────
static int32_t emaLeft  = 0;
static int32_t emaRight = 0;

static inline int32_t emaStep(int32_t acc, int raw) {
    return acc * (KNOB_EMA_ALPHA_DEN - KNOB_EMA_ALPHA_NUM) / KNOB_EMA_ALPHA_DEN
         + (int32_t)raw * ADC_SCALE * KNOB_EMA_ALPHA_NUM / KNOB_EMA_ALPHA_DEN;
}
static inline int emaValue(int32_t acc) { return (int)(acc / ADC_SCALE); }

// ── Button timing state ───────────────────────────────────────────────────────
static uint32_t btnPressTime           = 0;
static uint32_t btnFirstReleaseTime    = 0;
static bool     btnWaitingDbl          = false;
static bool     btnDoubleClickConsumed = false;

// ── Setup ─────────────────────────────────────────────────────────────────────
void setupButtonKnobs() {
    analogReadResolution(12);
    pinMode(PIN_BTN, INPUT_PULLUP);

    int rawL = analogRead(PIN_KNOB_LEFT);
    int rawR = analogRead(PIN_KNOB_RIGHT);
    emaLeft  = (int32_t)rawL * ADC_SCALE;
    emaRight = (int32_t)rawR * ADC_SCALE;

    lastBtnState = digitalRead(PIN_BTN);

    float initChorus = (float)rawL / ADC_MAX;
    float initReverb = (float)rawR / ADC_MAX;
    amy_set_chorus_level(initChorus);
    amy_set_reverb_level(initReverb);
    chorusLevel = initChorus;
    reverbLevel = initReverb;
}

// ── Main update ───────────────────────────────────────────────────────────────
void updateButtonKnobs() {
    uint32_t now         = millis();
    bool     btnState    = digitalRead(PIN_BTN);
    bool     fallingEdge = (lastBtnState == HIGH && btnState == LOW);
    bool     risingEdge  = (lastBtnState == LOW  && btnState == HIGH);
    bool     btnHeld     = (btnState == LOW);

    // ── Press edge ────────────────────────────────────────────────────────────
    if (fallingEdge) {
        btnPressTime = now;

        if (btnWaitingDbl && (now - btnFirstReleaseTime < (uint32_t)BTN_DBLCLICK_MS)) {
            // Double-click confirmed
            btnWaitingDbl          = false;
            btnDoubleClickConsumed = true;   // suppress the upcoming release
            if (menuMode) {
                menuExit();
            } else if (!patchSelectMode) {
                menuEnter();
            }
        }
    }

    // ── Release edge ──────────────────────────────────────────────────────────
    if (risingEdge) {
        if (btnDoubleClickConsumed) {
            btnDoubleClickConsumed = false;   // eat this release, do nothing

        } else {
            uint32_t held = now - btnPressTime;

            if (patchSelectMode && !menuMode) {
                patchSelectMode = false;
                if (patchKnobMoved) {
                    // Knob was turned during hold → load the chosen patch
                    currentPatchIndex = bankSlotToIndex(currentBank, currentSlot);
                    loadPatch(currentPatchIndex);
                    displayNeedsUpdate = true;
                    updateDisplay();
                }

            } else if (held < (uint32_t)BTN_CLICK_MAX_MS) {
                // Quick click – open double-click window
                btnFirstReleaseTime = now;
                btnWaitingDbl       = true;

                // In menu mode also immediately act as a single click (toggle edit)
                if (menuMode) {
                    menuClick(emaValue(emaRight));
                }
            }
            // Held between CLICK_MAX and HOLD: do nothing (medium press, ignored)
        }
    }

    // ── Double-click window expired → resolve as single click ────────────────
    if (btnWaitingDbl && (now - btnFirstReleaseTime >= (uint32_t)BTN_DBLCLICK_MS)) {
        btnWaitingDbl = false;
        // Single click in play mode: no action currently assigned
    }

    // ── Hold threshold → enter patch-select (play mode only) ─────────────────
    if (btnHeld && !menuMode && !patchSelectMode) {
        if ((now - btnPressTime) >= (uint32_t)BTN_HOLD_MS) {
            patchSelectMode  = true;
            patchKnobMoved   = false;
            lastLeftKnob     = -1;
            lastRightKnob    = -1;
            displayNeedsUpdate = true;
        }
    }

    lastBtnState = btnState;

    // ── EMA ───────────────────────────────────────────────────────────────────
    emaLeft  = emaStep(emaLeft,  analogRead(PIN_KNOB_LEFT));
    emaRight = emaStep(emaRight, analogRead(PIN_KNOB_RIGHT));

    int smoothedLeft  = emaValue(emaLeft);
    int smoothedRight = emaValue(emaRight);

    // ── Knob routing ──────────────────────────────────────────────────────────
    if (menuMode) {
        menuUpdate(smoothedLeft, smoothedRight);

    } else if (patchSelectMode) {
        int bank = map(smoothedLeft, 0, ADC_MAX, 0, NUM_BANKS);
        bank = constrain(bank, 0, NUM_BANKS - 1);
        if (bank != lastLeftKnob) {
            patchKnobMoved = true;
            lastLeftKnob = bank;
            currentBank  = bank;
            int slots = bankPatchCount(currentBank);
            if (currentSlot >= slots) currentSlot = slots - 1;
            displayNeedsUpdate = true;
            updateDisplay();
        }

        int totalSlots = bankPatchCount(currentBank);
        int slot = (totalSlots > 1) ? map(smoothedRight, 0, ADC_MAX, 0, totalSlots) : 0;
        slot = constrain(slot, 0, totalSlots - 1);
        if (slot != lastRightKnob) {
            patchKnobMoved = true;
            lastRightKnob = slot;
            currentSlot   = slot;
            displayNeedsUpdate = true;
            updateDisplay();
        }

    } else {
        // Play mode: chorus (left) and reverb (right)
        float newChorus = (float)smoothedLeft  / ADC_MAX;
        float newReverb = (float)smoothedRight / ADC_MAX;

        if (fabsf(newChorus - chorusLevel) >= FX_LEVEL_CHANGE_THRESHOLD) {
            chorusLevel = newChorus;
            amy_set_chorus_level(chorusLevel);
            amy_set_echo_level(chorusLevel);
            displayNeedsUpdate = true;
        }

        if (fabsf(newReverb - reverbLevel) >= FX_LEVEL_CHANGE_THRESHOLD) {
            reverbLevel = newReverb;
            amy_set_reverb_level(reverbLevel);
            displayNeedsUpdate = true;
        }
    }
}
