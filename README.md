# Pico2AmyTouchSynth
Pico 2 Amy synth engine based touch synthesizer

Hardware used is a PCB desinged by TodBot ; the Pico Test Synth https://github.com/todbot/pico_test_synth

Make sure when usign this board to set the solderpad jumpers right on the bottom. 
JP1 = closed and JP2 = open

 * Pico 2 Amy Touch Synth
 *
 * AMY-powered polyphonic synthesizer for pico_test_synth2 (Pico 2 / RP2350).
 *
 *   Hardware pins used
 *
 *   GP28  - middle button (hold to enter patch-select mode)
 *   GP27  - right knob   (ADC) (not used right now)
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
 *              to 3.3V instead (the pico_test_synth2 PCB already handles this).
 *
 * Dependencies (install via Arduino Library Manager):
 *   - AMY Synthesizer         (shorepine/amy)
 *   - TouchyTouch             (todbot/TouchyTouch  v1.2.1+)
 *   - Adafruit SSD1306        (128x64 OLED over I2C)
 *   - Adafruit GFX Library
 *   - arduino-pico board package (earlephilhower)
 *
 * Board: "Raspberry Pi Pico 2" (Tools > Board > Raspberry Pi RP2350 Boards)
 *
 <img width="2048" height="946" alt="Pico2AmyTouchSynth-Top" src="https://github.com/user-attachments/assets/b1e4bf8f-44b0-4547-9cfb-da0b0652bc54" />
<img width="2048" height="946" alt="Pico2AmyTouchSynth-Bottom" src="https://github.com/user-attachments/assets/0eca2823-ab09-4c60-805b-19dfe495f7d4" />
<img width="2048" height="946" alt="Pico2AmyTouchSynth-Bottom-detail" src="https://github.com/user-attachments/assets/1569fc3c-6bba-4e4d-97b3-35862d26e4cb" />

