#pragma once

// One row in the MENU_ITEMS[] table.
struct MenuItem {
    const char *label;      // shown on OLED (keep ≤ 20 chars)
    float      *value;      // pointer to the live variable
    float       minVal;
    float       maxVal;
    bool        isInt;      // true → display and step as a whole number
    void      (*apply)(float); // called whenever the value changes
};
