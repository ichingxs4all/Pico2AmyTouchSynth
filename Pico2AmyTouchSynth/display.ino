// display.ino
//
// SSD1306 128×64 OLED layout
// ──────────────────────────
// textSize(1): 6×8 px per char  →  21 chars per 128-px line
//
// Play mode layout (button not held):
//  Row  0  │ BANK  3/16   SLOT  5/17              │
//  Row  9  ├──────────────────────────────────────┤
//  Row 11  │ patch name line 1  (21 chars)        │
//  Row 20  │ patch name line 2  (21 chars)        │
//  Row 29  │ patch name line 3  if needed         │
//  Row 45  ├──────────────────────────────────────┤
//  Row 47  │ CHO [========  ]  REV [====      ]   │  ← effect bars
//  Row 56  │ PATCH # nnn                          │
//
// Select mode layout (button held):
//  Row  0  │ BANK  3/16   SLOT  5/17              │
//  Row  9  ├──────────────────────────────────────┤
//  Row 11  │ patch name line 1                    │
//  Row 20  │ patch name line 2                    │
//  Row 29  │ patch name line 3 if needed          │
//  Row 45  ├──────────────────────────────────────┤
//  Row 47  │ BNK [bank bar]   SLT [slot bar]      │
//  Row 56  │ PATCH # nnn                          │

// ── Helper: render patch name across up to 3 lines of 21 chars ───────────────
static void printPatchName(const char *name, int startY) {
    char buf[22];
    int  len = strlen(name);
    int  y   = startY;

    for (int line = 0; line < 3 && (line * 21) < len; line++) {
        int offset = line * 21;
        int chars  = len - offset;
        if (chars > 21) chars = 21;

        // Word-wrap: back up to last space if cut falls mid-word
        if (chars == 21 && name[offset + 21] != '\0' && name[offset + 21] != ' ') {
            int wrap = 21;
            while (wrap > 0 && name[offset + wrap] != ' ') wrap--;
            if (wrap > 0) chars = wrap;
        }

        strncpy(buf, name + offset, chars);
        buf[chars] = '\0';

        const char *p = buf;
        if (*p == ' ') p++;   // trim leading space from word-wrap

        display.setCursor(0, y);
        display.print(p);
        y += 9;
    }
}

// ── Helper: draw a labelled progress bar ──────────────────────────────────────
//  label   – short string printed before the bar (e.g. "CHO")
//  x, y    – top-left of the label
//  barW    – inner width of the bar in pixels
//  val     – current value  (0.0 – 1.0 float  OR  integer via overloads below)
static void drawLabelledBar(const char *label, int x, int y, int barW,
                             float val) {
    display.setCursor(x, y);
    display.print(label);
    int lw = strlen(label) * 6;           // label width in pixels (textSize 1)
    int bx = x + lw + 2;
    display.drawRect(bx, y, barW, 7, SSD1306_WHITE);
    int filled = (int)(val * (barW - 2));
    if (filled < 0) filled = 0;
    if (filled > barW - 2) filled = barW - 2;
    if (filled > 0)
        display.fillRect(bx + 1, y + 1, filled, 5, SSD1306_WHITE);
}

static void drawLabelledBarInt(const char *label, int x, int y, int barW,
                                int val, int maxVal) {
    float fval = (maxVal > 1) ? (float)val / (float)(maxVal - 1) : 1.0f;
    drawLabelledBar(label, x, y, barW, fval);
}

// ── setupDisplay ──────────────────────────────────────────────────────────────
void setupDisplay() {
    Wire1.setSDA(PIN_I2C_SDA);
    Wire1.setSCL(PIN_I2C_SCL);
    Wire1.begin();

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        if (debug) Serial.println("OLED not found");
    }

    display.setRotation(2);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.print("AMY Synth ");
    display.print(VERSION_STRING);
    display.display();
}

// ── updateDisplay ─────────────────────────────────────────────────────────────
void updateDisplay() {
    if (menuMode) { drawMenuDisplay(); return; }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    int patchIdx   = bankSlotToIndex(currentBank, currentSlot);
    int totalSlots = bankPatchCount(currentBank);
    int patchNum   = PATCHES[patchIdx].number;
    const char *name = PATCHES[patchIdx].name;

    // ── Row 0: bank / slot header (both modes) ────────────────────────────────
    display.setCursor(0, 0);
    display.print("BANK ");
    display.print(currentBank + 1);
    display.print("/");
    display.print(NUM_BANKS);
    display.print("  SLOT ");
    display.print(currentSlot + 1);
    display.print("/");
    display.print(totalSlots);

    display.drawFastHLine(0, 9, OLED_WIDTH, SSD1306_WHITE);

    // ── Patch name: up to 3 lines of 21 chars ────────────────────────────────
    printPatchName(name, 11);

    // ── Divider above bottom section ──────────────────────────────────────────
    display.drawFastHLine(0, 45, OLED_WIDTH, SSD1306_WHITE);

    // ── Bottom row 47: mode-specific bars ────────────────────────────────────
    if (patchSelectMode) {
        // Bank bar (left side) and slot bar (right side)
        // "BNK" = 3 chars × 6 px = 18 px + 2 gap → bar starts at x=20, w=44
        // "SLT" same on right: starts at x=70
        drawLabelledBarInt("BNK", 0,  47, 44, currentBank, NUM_BANKS);
        drawLabelledBarInt("SLT", 64, 47, 44, currentSlot, totalSlots);
    } else {
        // Chorus bar (left side) and reverb bar (right side)
        drawLabelledBar("CHO", 0,  47, 44, chorusLevel);
        drawLabelledBar("REV", 64, 47, 44, reverbLevel);
    }

    // ── Row 56: patch number (both modes) ────────────────────────────────────
    display.setCursor(0, 56);
    display.print("PATCH # ");
    display.print(patchNum);

    display.display();
    displayNeedsUpdate = false;
}
