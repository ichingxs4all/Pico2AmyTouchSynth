// button_knobs.ino
//
// Button modes (priority):
//   Menu mode      — double-click to enter/exit
//     left knob    → scroll items
//     click        → toggle edit
//     right knob   → change value (relative delta)
//     double-click → exit menu
//
//   Patch-select   — hold ≥ BTN_HOLD_MS to enter, release to confirm
//     left knob    → bank
//     right knob   → slot
//
//   Play mode      — default
//     left/right knobs → assigned function (pitchbend / mod / effect level)
//     double-click → enter menu
//
// Timing:
//   BTN_CLICK_MAX_MS  — release before this = click (not a hold)
//   BTN_HOLD_MS       — still held after this = patch-select  (> CLICK_MAX)
//   BTN_DBLCLICK_MS   — second press within this of first release = double-click

#define ADC_MAX            4095
#define ADC_SCALE            16
#define KNOB_EMA_ALPHA_NUM    3
#define KNOB_EMA_ALPHA_DEN   20

#define BTN_CLICK_MAX_MS   300
#define BTN_HOLD_MS        500
#define BTN_DBLCLICK_MS    400

// ── EMA ───────────────────────────────────────────────────────────────────────
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

// ── Knob last-value tracking (for change detection) ──────────────────────────
static int16_t lastPitchBendL = 0, lastPitchBendR = 0;
static uint8_t lastCCL = 0xFF, lastCCR = 0xFF;   // 0xFF = uninitialised

// ── Setup ─────────────────────────────────────────────────────────────────────
void setupButtonKnobs() {
    analogReadResolution(12);
    pinMode(PIN_BTN, INPUT_PULLUP);

    int rawL = analogRead(PIN_KNOB_LEFT);
    int rawR = analogRead(PIN_KNOB_RIGHT);
    emaLeft  = (int32_t)rawL * ADC_SCALE;
    emaRight = (int32_t)rawR * ADC_SCALE;
    lastBtnState = digitalRead(PIN_BTN);

    // Seed effect levels from knob positions (used when assigned to effects)
    chorusLevel = (float)rawL / ADC_MAX;
    reverbLevel = (float)rawR / ADC_MAX;
    leftKnobValue  = 0.5f;
    rightKnobValue = 0.5f;
}

// ── Apply one knob's assigned function ───────────────────────────────────────
static void applyKnob(int assign, int smoothedADC, float &knobValue,
                      int16_t &lastPB, uint8_t &lastCC) {
    switch (assign) {
        case KNOB_PITCHBEND: {
            int pb = map(smoothedADC, 0, ADC_MAX, -8192, 8191);
            if (abs(pb - (int)lastPB) > 64) {
                lastPB = (int16_t)pb;
                knobValue = (float)(pb + 8192) / 16383.0f;
                // Send to hardware MIDI out
                midiPitchBend(0, pb);
                // Apply directly to every active AMY voice so internal notes
                // are bent too (MIDI TX is not looped back into AMY's RX).
                // AMY normalises pitch bend as pb / (6 * 8192) semitones.
                float amyPb = (float)pb / (6.0f * 8192.0f);
                for (int v = 0; v < 16; v++) {
                    if (padActive[v]) {
                        amy_event ae = amy_default_event();
                        ae.voices[0] = v;
                        ae.pitch_bend = amyPb;
                        amy_add_event(&ae);
                    }
                }
                displayNeedsUpdate = true;
            }
            break;
        }
        case KNOB_MODULATION: {
            uint8_t cc = (uint8_t)(smoothedADC >> 5);   // 0-127
            if (cc != lastCC) {
                lastCC = cc;
                knobValue = cc / 127.0f;
                midiCC(0, 1, cc);
                displayNeedsUpdate = true;
            }
            break;
        }
        case KNOB_CHORUS: {
            float v = (float)smoothedADC / ADC_MAX;
            if (fabsf(v - chorusLevel) >= FX_LEVEL_CHANGE_THRESHOLD) {
                chorusLevel = v;
                knobValue   = v;
                amy_set_chorus_level(v);
                uint8_t ccv = (uint8_t)(v * 127.0f);
                if (ccv != lastCC) { lastCC = ccv; midiCC(0, 93, ccv); }
                displayNeedsUpdate = true;
            }
            break;
        }
        case KNOB_ECHO: {
            float v = (float)smoothedADC / ADC_MAX;
            if (fabsf(v - echoLevel) >= FX_LEVEL_CHANGE_THRESHOLD) {
                echoLevel = v;
                knobValue = v;
                amy_set_echo_level(v);
                uint8_t ccv = (uint8_t)(v * 127.0f);
                if (ccv != lastCC) { lastCC = ccv; midiCC(0, 95, ccv); }
                displayNeedsUpdate = true;
            }
            break;
        }
        case KNOB_REVERB: {
            float v = (float)smoothedADC / ADC_MAX;
            if (fabsf(v - reverbLevel) >= FX_LEVEL_CHANGE_THRESHOLD) {
                reverbLevel = v;
                knobValue   = v;
                amy_set_reverb_level(v);
                uint8_t ccv = (uint8_t)(v * 127.0f);
                if (ccv != lastCC) { lastCC = ccv; midiCC(0, 91, ccv); }
                displayNeedsUpdate = true;
            }
            break;
        }
    }
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
            btnWaitingDbl          = false;
            btnDoubleClickConsumed = true;
            if (menuMode)           menuExit();
            else if (!patchSelectMode) menuEnter();
        }
    }

    // ── Release edge ──────────────────────────────────────────────────────────
    if (risingEdge) {
        if (btnDoubleClickConsumed) {
            btnDoubleClickConsumed = false;

        } else {
            uint32_t held = now - btnPressTime;
            if (patchSelectMode && !menuMode) {
                patchSelectMode   = false;
                currentPatchIndex = bankSlotToIndex(currentBank, currentSlot);
                loadPatch(currentPatchIndex);
                saveSettings();
                displayNeedsUpdate = true;
                updateDisplay();
            } else if (held < (uint32_t)BTN_CLICK_MAX_MS) {
                btnFirstReleaseTime = now;
                btnWaitingDbl       = true;
                if (menuMode) menuClick(emaValue(emaRight));
            }
        }
    }

    // Double-click window expired without second press
    if (btnWaitingDbl && (now - btnFirstReleaseTime >= (uint32_t)BTN_DBLCLICK_MS)) {
        btnWaitingDbl = false;
    }

    // Hold threshold → patch-select (play mode only)
    if (btnHeld && !menuMode && !patchSelectMode) {
        if ((now - btnPressTime) >= (uint32_t)BTN_HOLD_MS) {
            patchSelectMode = true;
            lastLeftKnob    = -1;
            lastRightKnob   = -1;
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
        int bank = constrain(map(smoothedLeft, 0, ADC_MAX, 0, NUM_BANKS), 0, NUM_BANKS - 1);
        if (bank != lastLeftKnob) {
            lastLeftKnob = bank;
            currentBank  = bank;
            int slots = bankPatchCount(currentBank);
            if (currentSlot >= slots) currentSlot = slots - 1;
            displayNeedsUpdate = true;
        }
        int totalSlots = bankPatchCount(currentBank);
        int slot = (totalSlots > 1) ? map(smoothedRight, 0, ADC_MAX, 0, totalSlots) : 0;
        slot = constrain(slot, 0, totalSlots - 1);
        if (slot != lastRightKnob) {
            lastRightKnob = slot;
            currentSlot   = slot;
            displayNeedsUpdate = true;
        }

    } else {
        // Play mode: route each knob through its assigned function
        applyKnob((int)leftKnobAssign,  smoothedLeft,  leftKnobValue,  lastPitchBendL, lastCCL);
        applyKnob((int)rightKnobAssign, smoothedRight, rightKnobValue, lastPitchBendR, lastCCR);
    }
}
