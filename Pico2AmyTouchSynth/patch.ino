// ── Helper: load current patch into AMY ──────────────────────────────────────
void loadPatch(int patchIdx) {
    uint16_t patchNum = PATCHES[patchIdx].number;

    // Silence all active voices first
    for (int v = 0; v < 16; v++) {
        if (padActive[v]) {
            amy_event e = amy_default_event();
            e.voices[0] = v;
            e.velocity  = 0;
            amy_add_event(&e);
            padActive[v] = false;
        }
    }

    // Configure each voice with the new patch
    for (int v = 0; v < 16; v++) {
        amy_event e = amy_default_event();
        e.voices[0]    = v;
        e.patch_number = patchNum;
        amy_add_event(&e);
    }
}
