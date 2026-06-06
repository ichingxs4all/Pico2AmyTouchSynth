#pragma once

// touch.ino
//
// Capacitive pad handling via TouchyTouch.
//
// Changes vs v2:
//   • Removed dead arrays touch_threshold[] and touch_black_keys[].
//   • touch_velocity is now computed and normalised (0.0–1.0) and passed to
//     AMY instead of the previous hardcoded 1.0f.
//   • displayNeedsUpdate is no longer set on press/release — the display no
//     longer shows pad indicators, so those I2C refreshes were wasted.

// Velocity scaling: raw overshoot above threshold, clamped and normalised.
// Typical range observed is 0–500 counts above threshold.
#define VELOCITY_RAW_MAX  500

void setupTouch() {
    for (int i = 0; i < touch_count; i++) {
        touches[i].begin(touch_pins[i], 10, true);
        touches[i].threshold += touch_threshold_adjust;
    }
}

void updateTouchInputs() {
    for (int i = 0; i < touch_count; i++) {
        touches[i].update();

        if (touches[i].pressed()) {
            // Button held + pad 14 → octave down, pad 15 → octave up
            if (patchSelectMode && (i == 14 || i == 15)) {
                if (i == 14 && octave > -4) {
                    octave--;
                } else if (i == 15 && octave < 4) {
                    octave++;
                }
                updateDisplay();
#ifdef DEBUG
                Serial.print("octave=");
                Serial.println(octave);
#endif
                continue;
            }

            // Compute normalised velocity (0.0–1.0) from how hard the pad was hit
            int rawOver  = touches[i].raw_value - touches[i].threshold;
            if (rawOver < 0)          rawOver = 0;
            if (rawOver > VELOCITY_RAW_MAX) rawOver = VELOCITY_RAW_MAX;
            float vel = (float)rawOver / VELOCITY_RAW_MAX;
            if (vel < 0.05f) vel = 0.05f;   // minimum audible velocity

            padActive[i] = true;

            int note = (int)PAD_NOTE[i] + octave * 12;
            if (note < 0)   note = 0;
            if (note > 127) note = 127;

            amy_event e = amy_default_event();
            e.voices[0] = i;
            e.midi_note = (uint8_t)note;
            e.velocity  = vel;
            amy_add_event(&e);

            // Mirror to MIDI out (velocity scaled to 0–127)
            uint8_t midiVel = (uint8_t)(vel * 127.0f);
            if (midiVel < 1) midiVel = 1;
            midiNoteOn((uint8_t)note, midiVel);

#ifdef DEBUG
            Serial.print("pad ");
            Serial.print(i);
            Serial.print("  note=");
            Serial.print(note);
            Serial.print("  vel=");
            Serial.println(vel);
#endif
            digitalWrite(LED_BUILTIN, HIGH);
        }

        if (touches[i].released()) {
            // Octave pads don't send note-off
            if (patchSelectMode && (i == 14 || i == 15)) continue;

            padActive[i] = false;

            int note = (int)PAD_NOTE[i] + octave * 12;
            if (note < 0)   note = 0;
            if (note > 127) note = 127;

            amy_event e = amy_default_event();
            e.voices[0] = i;
            e.midi_note = (uint8_t)note;
            e.velocity  = 0.0f;
            amy_add_event(&e);

            // Mirror note-off to MIDI out
            midiNoteOff((uint8_t)note);

            digitalWrite(LED_BUILTIN, LOW);
        }
    }
}
