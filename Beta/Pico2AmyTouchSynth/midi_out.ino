// midi_out.ino — Hardware MIDI output helpers
//
// Writes directly into AMY's already-configured UART0 TX FIFO using the
// Pico SDK uart_putc_raw().  No re-initialisation needed; AMY sets uart0
// to 31250 baud on PIN_MIDI_TX (GP16) during amy_start().
// TX and RX are independent so this does not affect AMY's MIDI RX.

#include <hardware/uart.h>

static void midiWriteByte(uint8_t b) {
    uart_putc_raw(uart0, b);
}

void midiNoteOn(uint8_t ch, uint8_t note, uint8_t vel) {
    midiWriteByte(0x90 | (ch & 0x0F));
    midiWriteByte(note & 0x7F);
    midiWriteByte(vel  & 0x7F);
}

void midiNoteOff(uint8_t ch, uint8_t note) {
    midiWriteByte(0x80 | (ch & 0x0F));
    midiWriteByte(note & 0x7F);
    midiWriteByte(0x00);
}

// value: -8192 (full down) … 0 (centre) … +8191 (full up)
void midiPitchBend(uint8_t ch, int value) {
    int v = constrain(value + 8192, 0, 16383);
    midiWriteByte(0xE0 | (ch & 0x0F));
    midiWriteByte(v & 0x7F);
    midiWriteByte((v >> 7) & 0x7F);
}

// cc: controller number (1 = modulation), val: 0-127
void midiCC(uint8_t ch, uint8_t cc, uint8_t val) {
    midiWriteByte(0xB0 | (ch & 0x0F));
    midiWriteByte(cc  & 0x7F);
    midiWriteByte(val & 0x7F);
}
