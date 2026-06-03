/**
 * Pico 2 Amy Touch Synth
 *
 * AMY-powered polyphonic synthesizer for pico_test_synth2 (Pico 2 / RP2350).
 *
 * Hardware (from README):
 *   GP28  - middle button (hold to enter patch-select mode)
 *   GP27  - right knob   (ADC)
 *   GP26  - left knob    (ADC) <-- used for patch selection
 *   GP22  - I2S DATA
 *   GP21  - I2S LRC  (must be BCLK + 1)
 *   GP20  - I2S BCLK
 *   GP19  - I2C SCL (OLED)
 *   GP18  - I2C SDA (OLED)
 *   GP17  - TRS UART MIDI in
 *   GP16  - TRS UART MIDI out
 *   GP0–GP15 - 16 capacitive touch pads (each needs a 1MΩ pull-DOWN resistor to GND)
 *              NOTE: RP2350 has a hardware issue with low-value pull-downs, so this
 *              sketch uses TouchyTouch in PULLUP mode — use 1MΩ pull-UP resistors
 *              to 3.3V instead (or the pico_test_synth2 PCB already handles this).
 *
 * Dependencies (install via Arduino Library Manager):
 *   - AMY Synthesizer         (shorepine/amy)
 *   - TouchyTouch             (todbot/TouchyTouch  v1.2.1+)
 *   - Adafruit SSD1306        (128x64 OLED over I2C)
 *   - Adafruit GFX Library
 *   - arduino-pico board package (earlephilhower)
 *
 * Board: "Raspberry Pi Pico 2" (Tools > Board > Raspberry Pi RP2350 Boards)
 */

#include <AMY-Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TouchyTouch.h>

// ── Pin definitions ──────────────────────────────────────────────────────────
#define PIN_BTN 28         // middle button (active-LOW via internal pull-up)
#define PIN_KNOB_LEFT 26   // ADC – patch selection knob
#define PIN_KNOB_RIGHT 27  // ADC – spare (e.g. volume / mod)

#define PIN_I2S_BCLK 20
#define PIN_I2S_LRC 21  // must equal BCLK + 1 for AMY
#define PIN_I2S_DOUT 22

#define PIN_I2C_SDA 18
#define PIN_I2C_SCL 19

#define PIN_MIDI_RX 17
#define PIN_MIDI_TX 16

// ── OLED ─────────────────────────────────────────────────────────────────────
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire1, -1);

// ── AMY patch table ──────────────────────────────────────────────────────────
// Patches 0–127   : Juno-106 analog emulation
// Patches 128–255 : DX7 FM emulation
// Patch 256       : built-in piano
struct Patch {
  uint16_t number;
  const char *name;
};

static const Patch PATCHES[] = {
  // ── Juno-106 presets (representative selection) ──
  { 0, "Juno Brass" },
  { 1, "Juno Strings" },
  { 2, "Juno Pad" },
  { 3, "Juno Sweep" },
  { 4, "Juno Bass" },
  { 5, "Juno Pulse" },
  { 6, "Juno Lead" },
  { 7, "Juno Choir" },
  { 8, "Juno Poly" },
  { 9, "Juno Organ" },
  { 10, "Juno Arpeggio" },
  { 11, "Juno Flute" },
  { 12, "Juno Keys" },
  { 13, "Juno Atmosphere" },
  { 14, "Juno Crystals" },
  { 15, "Juno Warmth" },
  // ── DX7 FM presets (representative selection) ──
  { 128, "DX7 E.Piano 1" },
  { 129, "DX7 E.Piano 2" },
  { 130, "DX7 Brass" },
  { 131, "DX7 Marimba" },
  { 132, "DX7 Vibraphone" },
  { 133, "DX7 Strings" },
  { 134, "DX7 Bass" },
  { 135, "DX7 Organ" },
  { 136, "DX7 Flute" },
  { 137, "DX7 Bell" },
  { 138, "DX7 Clav" },
  { 139, "DX7 Harmonic" },
  // ── Piano & extras ──
  { 256, "Grand Piano" },
};

static const int NUM_PATCHES = sizeof(PATCHES) / sizeof(PATCHES[0]);

// ── TouchyTouch objects – one per pad ────────────────────────────────────────
// RP2350 has a hardware errata with low-value pull-downs, so we pass
// TOUCHYTOUCH_PULLUP as the second argument (added in TouchyTouch v1.2.0).
// This means wire your 1MΩ resistors to 3.3V (pull-up) rather than GND.


// ── Synth state ───────────────────────────────────────────────────────────────
int currentPatchIndex = 0;
int lastKnobValue = -1;
bool patchSelectMode = false;
bool lastBtnState = HIGH;

// Per-pad voice tracking (one AMY voice per pad, index matches pad)
static bool padActive[16] = { false };

// MIDI note mapping for 16 pads (two chromatic octaves from C3)
static const uint8_t PAD_NOTE[16] = {
  48, 50, 52, 53, 55, 57, 59, 60,  // C3 – C4 (white keys)
  62, 64, 65, 67, 69, 71, 72, 74   // D4 – D5
};


const int touch_pins[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
//const int touch_pins[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22 };
int touch_velocity[] = { 100,100,100,100,100,100,100,100,100,100,100,100,100,100, 100,100};
int touch_threshold[]={ 1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000, 1000,1000};
const int touch_count = sizeof(touch_pins) / sizeof(int);
const int touch_threshold_adjust = 100;
const bool touch_black_keys[] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1 }; // 1, 3, 6, 8, 10, 13, 14}
TouchyTouch touches[touch_count];

bool displayNeedsUpdate = false;
bool debug = true;

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
  if(debug){
    Serial.begin(115200);
    Serial.println("Pico 2 Amy Touch Synth");
    Serial.print(NUM_PATCHES);
  }

  setupTouch();
  setupButtonKnobs();
  setupDisplay();
  setupAMY();

  delay(500);
  updateDisplay();
}

// ── Loop ──────────────
void loop() {
  
  updateTouchInputs();
  updateButtonKnobs();

  if (displayNeedsUpdate && !patchSelectMode) updateDisplay();

  amy_update();
}
