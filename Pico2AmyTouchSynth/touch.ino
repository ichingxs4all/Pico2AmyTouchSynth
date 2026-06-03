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
            // Compute normalised velocity (0.0–1.0) from how hard the pad was hit
            int rawOver  = touches[i].raw_value - touches[i].threshold;
            if (rawOver < 0)          rawOver = 0;
            if (rawOver > VELOCITY_RAW_MAX) rawOver = VELOCITY_RAW_MAX;
            float vel = (float)rawOver / VELOCITY_RAW_MAX;
            if (vel < 0.05f) vel = 0.05f;   // minimum audible velocity

            padActive[i] = true;

            amy_event e = amy_default_event();
            e.voices[0] = i;
            e.midi_note = PAD_NOTE[i];
            e.velocity  = vel;
            amy_add_event(&e);

            if (debug) {
                Serial.print("pad ");
                Serial.print(i);
                Serial.print("  vel=");
                Serial.println(vel);
            }
            digitalWrite(LED_BUILTIN, HIGH);
        }

        if (touches[i].released()) {
            padActive[i] = false;

            amy_event e = amy_default_event();
            e.voices[0] = i;
            e.midi_note = PAD_NOTE[i];
            e.velocity  = 0.0f;
            amy_add_event(&e);

            digitalWrite(LED_BUILTIN, LOW);
        }
    }
}
