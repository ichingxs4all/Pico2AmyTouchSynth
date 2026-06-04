/**
 * Pico 2 Amy Touch Synth
 * ──────────────────────────────────────────────────────────────────────────────
 * AMY-powered polyphonic touch synthesizer for the pico_test_synth2 board
 * (Raspberry Pi Pico 2 / RP2350).
 *
 * Hardware:
 *   GP28  – middle button  (hold to enter patch-select mode, release to confirm)
 *   GP27  – right knob ADC (play: reverb level  |  select: patch slot)
 *   GP26  – left  knob ADC (play: chorus level  |  select: bank)
 *   GP22  – I2S DATA out
 *   GP21  – I2S LRC  (must be BCLK + 1 for AMY)
 *   GP20  – I2S BCLK
 *   GP19  – I2C SCL (SSD1306 OLED)
 *   GP18  – I2C SDA (SSD1306 OLED)
 *   GP17  – TRS UART MIDI RX
 *   GP16  – TRS UART MIDI TX
 *   GP0–GP15 – 16 capacitive touch pads
 *              (use 1 MΩ pull-UP resistors to 3.3 V — RP2350 errata)
 *
 * Knob behaviour:
 *   Button NOT held  →  left knob = chorus level,  right knob = reverb level
 *   Button HELD      →  left knob = bank (1–16),   right knob = slot in bank
 *   Release button   →  load chosen patch
 *
 * NOTE: All .ino files in this folder are merged automatically by the Arduino
 *       IDE — no explicit #include needed for the other .ino files.
 *
 * Dependencies (Arduino Library Manager):
 *   – AMY Synthesizer       (shorepine/amy)
 *   – TouchyTouch           (todbot/TouchyTouch v1.2.1+)
 *   – Adafruit SSD1306      (128×64 OLED over I2C)
 *   – Adafruit GFX Library
 *   – arduino-pico board package (earlephilhower)
 *
 * Board: "Raspberry Pi Pico 2"  (Tools › Board › Raspberry Pi RP2350 Boards)
 */
#include <AMY-Arduino.h>
#include "version.h"
#include "effects.h"
#include "menu.h"
#include "patches_table.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TouchyTouch.h>

// ── Pin definitions ───────────────────────────────────────────────────────────
#define PIN_BTN        28   // button – active LOW (internal pull-up)
#define PIN_KNOB_LEFT  26   // ADC – play: chorus level  |  select: bank
#define PIN_KNOB_RIGHT 27   // ADC – play: reverb level  |  select: slot

#define PIN_I2S_BCLK 20
#define PIN_I2S_LRC  21     // must equal BCLK + 1 for AMY
#define PIN_I2S_DOUT 22

#define PIN_I2C_SDA 18
#define PIN_I2C_SCL 19

#define PIN_MIDI_RX 17
#define PIN_MIDI_TX 16

// ── OLED ──────────────────────────────────────────────────────────────────────
#define OLED_WIDTH  128
#define OLED_HEIGHT  64
#define OLED_ADDR   0x3C

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire1, -1);

// ── Touch pads ────────────────────────────────────────────────────────────────
// RP2350 errata: use pull-UP mode (1 MΩ resistors to 3.3 V).
const int  touch_pins[]           = { 0, 1, 2, 3, 4, 5, 6, 7,
                                       8, 9,10,11,12,13,14,15 };
const int  touch_count            = sizeof(touch_pins) / sizeof(int);
const int  touch_threshold_adjust = 100;
TouchyTouch touches[touch_count];

// ── MIDI note map for 16 pads ─────────────────────────────────────────────────
static const uint8_t PAD_NOTE[16] = {
    48, 50, 52, 53, 55, 57, 59, 60,   // C3 – C4  (white keys)
    62, 64, 65, 67, 69, 71, 72, 74    // D4 – D5
};

// ── Synth / UI state ──────────────────────────────────────────────────────────
int  currentBank       = 0;   // 0-based bank index
int  currentSlot       = 0;   // 0-based slot within the bank
int  currentPatchIndex = 0;   // global PATCHES[] index

bool patchSelectMode = false;
bool lastBtnState    = HIGH;  // seeded from real pin in setupButtonKnobs()

// ── Menu state ────────────────────────────────────────────────────────────────
bool menuMode    = false;
int  menuItemIdx = 0;
bool menuEditing = false;

// Smoothed knob tracking (mapped domain); -1 forces first-read
int  lastLeftKnob    = -1;
int  lastRightKnob   = -1;

// ── Effect levels ─────────────────────────────────────────────────────────────
float chorusLevel = FX_CHORUS_DEFAULT_LEVEL;
float echoLevel = FX_ECHO_DEFAULT_LEVEL;
float reverbLevel = FX_REVERB_DEFAULT_LEVEL;

static bool padActive[16] = { false };

bool displayNeedsUpdate = false;
bool debug = true;
int transpose = 12;

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
    if (debug) {
        Serial.begin(115200);
        Serial.print("Pico 2 Amy Touch Synth ");
        Serial.println(VERSION_STRING);
        Serial.print(NUM_PATCHES);
        Serial.print(" patches / ");
        Serial.print(NUM_BANKS);
        Serial.print(" banks / BANK_SIZE=");
        Serial.println(BANK_SIZE);
    }

    setupTouch();
    setupButtonKnobs();   // also seeds chorusLevel / reverbLevel from knob pos
    setupDisplay();
    setupAMY();

    delay(500);
    updateDisplay();
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
    updateTouchInputs();
    updateButtonKnobs();
    if (displayNeedsUpdate && (!patchSelectMode || menuMode)) updateDisplay();
    amy_update();
}
