void setupButtonKnobs(){
   // Button
  pinMode(PIN_BTN, INPUT_PULLUP);
}

void updateButtonKnobs(){
  // ── Button ────────────────────────────────────────────────────────────
  bool btnState = digitalRead(PIN_BTN);  // LOW = pressed
  bool btnPressed = (btnState == LOW);
  bool btnReleased = (lastBtnState == LOW && btnState == HIGH);

  if (btnPressed && !patchSelectMode) {
    patchSelectMode = true;
    lastKnobValue = -1;
    displayNeedsUpdate = true;
  }
  if (btnReleased && patchSelectMode) {
    patchSelectMode = false;
    loadPatch(currentPatchIndex);
    displayNeedsUpdate = true;
    updateDisplay();
  }
  lastBtnState = btnState;

  // ── Patch selection via left knob ─────────────────────────────────────
  if (patchSelectMode) {
    int raw = analogRead(PIN_KNOB_LEFT);  // 0–4095 (12-bit ADC)
    int idx = map(raw, 0, 4095, 0, NUM_PATCHES - 1);
    idx = constrain(idx, 0, NUM_PATCHES - 1);
    if (idx != lastKnobValue) {
      lastKnobValue = idx;
      currentPatchIndex = idx;
      displayNeedsUpdate = true;
      updateDisplay();
    }
  }
}