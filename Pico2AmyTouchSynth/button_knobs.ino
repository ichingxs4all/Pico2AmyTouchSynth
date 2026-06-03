// button_knobs.ino
//
// Knob behaviour depends on whether the button is held:
//
//   Button NOT held  (play mode)
//     Left  knob → chorus level  (0.0 – 1.0)
//     Right knob → reverb level  (0.0 – 1.0)
//
//   Button HELD  (patch-select mode)
//     Left  knob → bank  (1 – NUM_BANKS)
//     Right knob → slot  within the selected bank
//     Release button → load chosen patch
//
// Knob smoothing – Exponential Moving Average (EMA)
// ──────────────────────────────────────────────────
// Raw 12-bit ADC samples jitter ±10–30 LSB at a fixed pot position.
// EMA low-pass filters the stream:
//
//   smoothed = smoothed × (1 − α) + raw × α       α = 3/20 = 0.15
//
// The accumulator is kept as fixed-point (×ADC_SCALE) to avoid float
// arithmetic on every loop tick.
//
// No dead-band is applied to mapped output values.  A dead-band of N
// skips every N-th position; EMA alone is sufficient to suppress jitter.

#define ADC_MAX            4095   // 12-bit full-scale
#define ADC_SCALE            16   // fixed-point multiplier for EMA accumulator
#define KNOB_EMA_ALPHA_NUM    3   // α = 3/20 = 0.15
#define KNOB_EMA_ALPHA_DEN   20

// ── EMA state ─────────────────────────────────────────────────────────────────
static int32_t emaLeft  = 0;
static int32_t emaRight = 0;

static inline int32_t emaStep(int32_t acc, int raw) {
    return acc * (KNOB_EMA_ALPHA_DEN - KNOB_EMA_ALPHA_NUM) / KNOB_EMA_ALPHA_DEN
         + (int32_t)raw * ADC_SCALE * KNOB_EMA_ALPHA_NUM / KNOB_EMA_ALPHA_DEN;
}

static inline int emaValue(int32_t acc) { return (int)(acc / ADC_SCALE); }

// ── Setup ─────────────────────────────────────────────────────────────────────
void setupButtonKnobs() {
    analogReadResolution(12);       // RP2350 native 12-bit ADC (0–4095)
    pinMode(PIN_BTN, INPUT_PULLUP);

    // Seed EMA from real pin values so it starts settled
    int rawL = analogRead(PIN_KNOB_LEFT);
    int rawR = analogRead(PIN_KNOB_RIGHT);
    emaLeft  = (int32_t)rawL * ADC_SCALE;
    emaRight = (int32_t)rawR * ADC_SCALE;

    // Seed button state so no spurious release fires at boot
    lastBtnState = digitalRead(PIN_BTN);

    // Apply initial effect levels from boot knob positions
    float initChorus = (float)rawL / ADC_MAX;
    float initReverb = (float)rawR / ADC_MAX;
    amy_set_chorus_level(initChorus);
    amy_set_reverb_level(initReverb);
    chorusLevel = initChorus;
    reverbLevel = initReverb;
}

// ── Main update ───────────────────────────────────────────────────────────────
void updateButtonKnobs() {
    // ── Button ────────────────────────────────────────────────────────────────
    bool btnState    = digitalRead(PIN_BTN);
    bool btnPressed  = (btnState == LOW);
    bool btnReleased = (lastBtnState == LOW && btnState == HIGH);

    if (btnPressed && !patchSelectMode) {
        patchSelectMode = true;
        lastLeftKnob    = -1;   // force first-read evaluation
        lastRightKnob   = -1;
        displayNeedsUpdate = true;
    }

    if (btnReleased && patchSelectMode) {
        patchSelectMode   = false;
        currentPatchIndex = bankSlotToIndex(currentBank, currentSlot);
        loadPatch(currentPatchIndex);
        displayNeedsUpdate = true;
        updateDisplay();
    }

    lastBtnState = btnState;

    // Update EMA accumulators every call regardless of mode
    emaLeft  = emaStep(emaLeft,  analogRead(PIN_KNOB_LEFT));
    emaRight = emaStep(emaRight, analogRead(PIN_KNOB_RIGHT));

    int smoothedLeft  = emaValue(emaLeft);
    int smoothedRight = emaValue(emaRight);

    if (patchSelectMode) {
        // ── Patch-select mode: knobs choose bank and slot ─────────────────────

        int bank = map(smoothedLeft, 0, ADC_MAX, 0, NUM_BANKS);
        bank     = constrain(bank, 0, NUM_BANKS);

        if (bank != lastLeftKnob) {
            lastLeftKnob = bank;
            currentBank  = bank;
            int slots = bankPatchCount(currentBank);
            if (currentSlot >= slots) currentSlot = slots - 1;
            displayNeedsUpdate = true;
            updateDisplay();
        }

        int totalSlots = bankPatchCount(currentBank);
        int slot = (totalSlots > 1)
                     ? map(smoothedRight, 0, ADC_MAX, 0, totalSlots)
                     : 0;
        slot = constrain(slot, 0, totalSlots - 1);

        if (slot != lastRightKnob) {
            lastRightKnob = slot;
            currentSlot   = slot;
            displayNeedsUpdate = true;
            updateDisplay();
        }

    } else {
        // ── Play mode: knobs control chorus and reverb levels ─────────────────

        float newChorus = (float)smoothedLeft  / ADC_MAX;
        float newReverb = (float)smoothedRight / ADC_MAX;

        if (fabsf(newChorus - chorusLevel) >= FX_LEVEL_CHANGE_THRESHOLD) {
            chorusLevel = newChorus;
            Serial.println(chorusLevel);
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
