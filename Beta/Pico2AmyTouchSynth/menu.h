#pragma once

// ── MenuItem ──────────────────────────────────────────────────────────────────
struct MenuItem {
    const char  *label;       // shown in menu (≤ 20 chars)
    float       *value;       // pointer to the live variable
    float        minVal;
    float        maxVal;
    bool         isInt;       // display / step as integer
    const char **valueNames;  // if non-null, show name at (int)*value
    void       (*apply)(float);
};

// ── Knob function codes ────────────────────────────────────────────────────────
// Stored as float in leftKnobAssign / rightKnobAssign for MenuItem compatibility.
enum KnobFunc {
    KNOB_PITCHBEND = 0,
    KNOB_MODULATION,
    KNOB_CHORUS,
    KNOB_ECHO,
    KNOB_REVERB,
    KNOB_FUNC_COUNT
};
