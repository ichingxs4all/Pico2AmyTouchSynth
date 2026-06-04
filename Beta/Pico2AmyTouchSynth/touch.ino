// touch.ino
//
// Capacitive pad handling via TouchyTouch.
//
// Octave shifting: while the button is held, pad 14 lowers the octave by 1
// and pad 15 raises it by 1.  Those two pads do not play a note in that case.
// All notes are offset by the global `transpose` semitone value.
// `transpose` starts at 12 so the default range is one octave above the base
// PAD_NOTE[] table (PAD_NOTE[0]=48 + 12 = 60 = C4).

#define VELOCITY_RAW_MAX  500
#define TRANSPOSE_MIN    -48   // PAD_NOTE[0] + TRANSPOSE_MIN = 0
#define TRANSPOSE_MAX     48   // PAD_NOTE[15] + TRANSPOSE_MAX = 122, safely ≤ 127

void setupTouch() {
    for (int i = 0; i < touch_count; i++) {
        touches[i].begin(touch_pins[i], 10, true);
        touches[i].threshold += touch_threshold_adjust;
    }
}

void updateTouchInputs() {
    bool btnHeld = (digitalRead(PIN_BTN) == LOW);

    for (int i = 0; i < touch_count; i++) {
        touches[i].update();

        if (touches[i].pressed()) {
            // ── Octave shift (pads 14 / 15 while button held) ─────────────────
            if (btnHeld) {
                if (i == 14) {
                    transpose = constrain(transpose - 12, TRANSPOSE_MIN, TRANSPOSE_MAX);
                    //displayNeedsUpdate = true;
                    updateDisplay();
                    continue;   // don't play a note
                }
                if (i == 15) {
                    transpose = constrain(transpose + 12, TRANSPOSE_MIN, TRANSPOSE_MAX);
                    //displayNeedsUpdate = true;
                    updateDisplay();
                    continue;
                }
            }

            // ── Normal note-on ────────────────────────────────────────────────
            int rawOver = touches[i].raw_value - touches[i].threshold;
            if (rawOver < 0)               rawOver = 0;
            if (rawOver > VELOCITY_RAW_MAX) rawOver = VELOCITY_RAW_MAX;
            float vel = (float)rawOver / VELOCITY_RAW_MAX;
            if (vel < 0.05f) vel = 0.05f;

            uint8_t note    = (uint8_t)constrain(PAD_NOTE[i] + transpose, 0, 127);
            uint8_t midiVel = (uint8_t)constrain((int)(vel * 127.0f), 1, 127);
            padActive[i] = true;

            // Send to AMY internal synth
            amy_event e = amy_default_event();
            e.voices[0] = i;
            e.midi_note = note;
            e.velocity  = vel;
            amy_add_event(&e);

            // Send note-on to hardware MIDI out
            midiNoteOn(0, note, midiVel);

            if (debug) {
                Serial.print("pad "); Serial.print(i);
                Serial.print("  note="); Serial.print(note);
                Serial.print("  vel="); Serial.println(vel);
            }
            digitalWrite(LED_BUILTIN, HIGH);
        }

        if (touches[i].released()) {
            padActive[i] = false;

            uint8_t note = (uint8_t)constrain(PAD_NOTE[i] + transpose, 0, 127);

            // Send note-off to hardware MIDI out
            midiNoteOff(0, note);

            amy_event e = amy_default_event();
            e.voices[0] = i;
            e.midi_note = note;
            e.velocity  = 0.0f;
            amy_add_event(&e);

            digitalWrite(LED_BUILTIN, LOW);
        }
    }
}
