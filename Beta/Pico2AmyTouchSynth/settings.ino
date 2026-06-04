// settings.ino — Persistent settings via EEPROM (flash emulation on RP2350)
//
// setupSettings() is called BEFORE setupDisplay() and setupAMY().
// It only writes to plain variables — no I2C, no AMY events.  This is the
// crucial rule that prevents boot stalls.
//
// resetToDefaults() is called at runtime from the menu and may freely call
// AMY and display functions.

#include <EEPROM.h>

#define SETTINGS_MAGIC  0xA8D2F301u
#define EEPROM_SIZE     64

struct Settings {
    uint32_t magic;
    int      transpose;
    uint8_t  leftKnobAssign;
    uint8_t  rightKnobAssign;
    int      currentBank;
    int      currentSlot;
};

// ── Write default values into variables — NO AMY / display calls ──────────────
void setDefaultVariables() {
    transpose        = 12;
    leftKnobAssign   = (float)KNOB_PITCHBEND;
    rightKnobAssign  = (float)KNOB_MODULATION;
    currentBank      = 0;
    currentSlot      = 0;
}

// ── Persist current variables to flash ───────────────────────────────────────
void saveSettings() {
    Settings s;
    s.magic           = SETTINGS_MAGIC;
    s.transpose       = transpose;
    s.leftKnobAssign  = (uint8_t)(int)leftKnobAssign;
    s.rightKnobAssign = (uint8_t)(int)rightKnobAssign;
    s.currentBank     = currentBank;
    s.currentSlot     = currentSlot;
    EEPROM.put(0, s);
    EEPROM.commit();
    if (debug) Serial.println("settings saved");
}

// ── Full runtime reset — call only after AMY + display are running ────────────
void resetToDefaults() {
    setDefaultVariables();
    saveSettings();
    currentPatchIndex = bankSlotToIndex(currentBank, currentSlot);
    loadPatch(currentPatchIndex);
    leftKnobValue  = 0.5f;
    rightKnobValue = 0.5f;
    menuMode    = false;
    menuEditing = false;
    displayNeedsUpdate = true;
    updateDisplay();
}

// ── Init: load from EEPROM into variables only (no AMY / display calls) ───────
void setupSettings() {
    EEPROM.begin(EEPROM_SIZE);
    Settings s;
    EEPROM.get(0, s);
    if (s.magic == SETTINGS_MAGIC) {
        transpose       = s.transpose;
        leftKnobAssign  = (float)constrain(s.leftKnobAssign,  0, KNOB_FUNC_COUNT - 1);
        rightKnobAssign = (float)constrain(s.rightKnobAssign, 0, KNOB_FUNC_COUNT - 1);
        currentBank     = constrain(s.currentBank, 0, NUM_BANKS - 1);
        currentSlot     = s.currentSlot;
        if (debug) Serial.println("settings loaded");
    } else {
        if (debug) Serial.println("first boot, writing defaults");
        setDefaultVariables();
        saveSettings();
    }
}
