
void setupDisplay(){

  // I2C + OLED
  Wire1.setSDA(PIN_I2C_SDA);
  Wire1.setSCL(PIN_I2C_SCL);
  Wire1.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    if(debug)Serial.println("OLED not found");
  }

  display.setRotation(2);  // rotated 180
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print("AMY Synth");
  display.display();
}


// ── Helper: draw the current state on the OLED ───────────────────────────────
void updateDisplay() {
    display.clearDisplay();

    if (patchSelectMode) {
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.print("SELECT PATCH");

        // Progress bar
        int barW = map(currentPatchIndex, 0, NUM_PATCHES - 1, 0, OLED_WIDTH);
        display.drawRect(0, 12, OLED_WIDTH, 6, SSD1306_WHITE);
        display.fillRect(0, 12, barW, 6, SSD1306_WHITE);

        // Patch number
        display.setCursor(0, 22);
        display.print("# ");
        display.print(PATCHES[currentPatchIndex].number);

        // Patch name (big)
        display.setTextSize(2);
        display.setCursor(0, 36);
        display.print(PATCHES[currentPatchIndex].name);
    } else {
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.print("PATCH:");

        display.setTextSize(2);
        display.setCursor(0, 12);
        // Truncate to fit 128px at size-2 (≈10 chars)
        char nameBuf[11];
        strncpy(nameBuf, PATCHES[currentPatchIndex].name, 10);
        nameBuf[10] = '\0';
        display.print(nameBuf);

        // Active pad indicators at the bottom
        for (int i = 0; i < 16; i++) {
            int x = (i % 8) * 15 + 2;
            int y = (i < 8) ? 46 : 56;
            if (padActive[i]) {
                display.fillRect(x, y, 12, 7, SSD1306_WHITE);
            } else {
                display.drawRect(x, y, 12, 7, SSD1306_WHITE);
            }
        }
    }

    display.display();
    displayNeedsUpdate = false;
}