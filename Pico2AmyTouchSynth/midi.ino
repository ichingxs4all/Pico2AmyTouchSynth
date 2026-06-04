// midi.ino
//
// MIDI I/O on two transports simultaneously:
//   1. UART MIDI  – GP16 TX / GP17 RX, 31 250 baud  (TRS jack)
//   2. USB MIDI   – Adafruit TinyUSB  (class-compliant, no driver needed)
//
// Incoming Note-On / Note-Off on either transport is forwarded to AMY.
// All other incoming messages (CC, PC, etc.) are silently ignored for now.

#define MIDI_CHANNEL    0    // 0-based → MIDI channel 1 (omni receive)
#define MIDI_NOTE_ON    0x90
#define MIDI_NOTE_OFF   0x80

// ── MIDI parser state — separate for each transport ──────────────────────────
static uint8_t uartBuf[3], uartLen = 0, uartWant = 0;
static uint8_t usbBuf[3],  usbLen  = 0, usbWant  = 0;

// ── Internal: dispatch a complete 3-byte MIDI message to AMY ─────────────────
static void dispatchMidiMessage(uint8_t status, uint8_t data1, uint8_t data2) {
    uint8_t type = status & 0xF0;

    if (type == MIDI_NOTE_ON  && data2 > 0) {
        float vel = data2 / 127.0f;
        // Use a dedicated AMY voice slot (16) for MIDI-in so it doesn't
        // clobber the 16 pad voices (0-15).
        amy_event e = amy_default_event();
        e.voices[0] = 16;
        e.midi_note = data1;
        e.velocity  = vel;
        amy_add_event(&e);
    } else if (type == MIDI_NOTE_OFF || (type == MIDI_NOTE_ON && data2 == 0)) {
        amy_event e = amy_default_event();
        e.voices[0] = 16;
        e.midi_note = data1;
        e.velocity  = 0.0f;
        amy_add_event(&e);
    }
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setupMidi() {
    // USB MIDI
    TinyUSBDevice.setProductDescriptor("Amy Touch Synth");
    usbMidi.begin();

    // Wait up to ~2 s for the host to enumerate (skipped if no USB host).
    uint32_t t = millis();
    while (!TinyUSBDevice.mounted() && (millis() - t) < 2000) {
        delay(10);
    }

    // UART MIDI
    Serial1.setTX(PIN_MIDI_TX);
    //Serial1.setRX(PIN_MIDI_RX);
    Serial1.begin(31250);
}

// ── Feed one byte into parser state; dispatch when a complete message arrives ─
static void parseByte(uint8_t b,
                      uint8_t *buf, uint8_t &len, uint8_t &want) {
    if (b & 0x80) {
        uint8_t type = b & 0xF0;
        if (type == MIDI_NOTE_ON || type == MIDI_NOTE_OFF) {
            buf[0] = b; len = 1; want = 3;
        } else {
            len = 0; want = 0;
        }
    } else if (want > 0) {
        buf[len++] = b;
        if (len >= want) {
            dispatchMidiMessage(buf[0], buf[1], buf[2]);
            len = 1;   // keep status for running-status
        }
    }
}

// ── Called every loop() — poll both transports for incoming MIDI ──────────────
void updateMidiIn() {
    while (usbMidi.available())
        parseByte((uint8_t)usbMidi.read(),  usbBuf,  usbLen,  usbWant);
    //while (Serial1.available())
    //    parseByte((uint8_t)Serial1.read(),  uartBuf, uartLen, uartWant);
}

// ── Note-out helpers (pad presses → both transports) ─────────────────────────
void midiNoteOn(uint8_t note, uint8_t velocity) {
    if (velocity == 0) { midiNoteOff(note); return; }

    uint8_t status = MIDI_NOTE_ON | MIDI_CHANNEL;

    Serial1.write(status);
    Serial1.write(note     & 0x7F);
    Serial1.write(velocity & 0x7F);

    uint8_t packet[3] = { status, (uint8_t)(note & 0x7F), (uint8_t)(velocity & 0x7F) };
    usbMidi.write(packet, 3);
}

void midiNoteOff(uint8_t note) {
    uint8_t status = MIDI_NOTE_OFF | MIDI_CHANNEL;

    Serial1.write(status);
    Serial1.write(note & 0x7F);
    Serial1.write((uint8_t)0);

    uint8_t packet[3] = { status, (uint8_t)(note & 0x7F), 0 };
    usbMidi.write(packet, 3);
}
