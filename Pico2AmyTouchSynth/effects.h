#pragma once

// ── effects.h ─────────────────────────────────────────────────────────────────
// Global effect parameters and send helpers for AMY chorus and reverb.
//
// AMY wire-protocol reference (from docs/api.md):
//
//   Chorus:  x<level>,<delay>,<freq>,<depth>Z
//     level  – output mix level   (0.0 = off,  1.0 = full)
//     delay  – max delay samples  (default 320)
//     freq   – LFO rate Hz        (default 0.5)
//     depth  – proportion of max delay (default 0.5)
//
//   Reverb:  y<level>,<liveness>,<damping>,<xover>Z
//     level    – output mix level (0.0 = off,  1.0 = full)
//     liveness – decay time       (0.0–1.0, default 0.85; 1 = longest)
//     damping  – HF decay         (default 0.5)
//     xover    – damping crossover Hz (default 3000)
//
// Only level is changed by the knobs; the other parameters stay at their
// AMY defaults so the character of both effects is fixed and musical.
#include <AMY-Arduino.h>
// ── Default / fixed effect parameters ────────────────────────────────────────
#define FX_CHORUS_DELAY   320    // samples (AMY default)
#define FX_CHORUS_FREQ    0.5f   // LFO Hz  (AMY default)
#define FX_CHORUS_DEPTH   0.5f   // proportion of max delay (AMY default)

#define FX_REVERB_LIVENESS 0.85f  // decay time   (AMY default)
#define FX_REVERB_DAMPING  0.5f   // HF damping   (AMY default)
#define FX_REVERB_XOVER    3000   // crossover Hz (AMY default)

// ── Initial levels (knob start position) ─────────────────────────────────────
#define FX_CHORUS_DEFAULT_LEVEL  0.0f   // off until knob is moved
#define FX_REVERB_DEFAULT_LEVEL  0.0f   // off until knob is moved
#define FX_ECHO_DEFAULT_LEVEL  0.0f   // off until knob is moved
// ── Change-detection threshold ────────────────────────────────────────────────
// Avoid sending AMY events on every loop tick when the knob is still.
// 0.01f = 1% change in level (4096 ADC steps / ~41 steps per bucket).
#define FX_LEVEL_CHANGE_THRESHOLD 0.01f

// ── Helpers ───────────────────────────────────────────────────────────────────

// Send chorus level to AMY (other parameters stay at defaults).
// level: 0.0 (off) – 1.0 (full)
inline void amy_set_chorus_level(float level) {
        amy_event e = amy_default_event();
            e.chorus_level = level;
            amy_add_event(&e);  
}

// Send reverb level to AMY (other parameters stay at defaults).
// level: 0.0 (off) – 1.0 (full)
inline void amy_set_reverb_level(float level) {
    amy_event e = amy_default_event();
            e.reverb_level = level;
            amy_add_event(&e);
}

// Send reverb level to AMY (other parameters stay at defaults).
// level: 0.0 (off) – 1.0 (full)
inline void amy_set_echo_level(float level) {
    amy_event e = amy_default_event();
            e.echo_level = level;
            amy_add_event(&e);
}
