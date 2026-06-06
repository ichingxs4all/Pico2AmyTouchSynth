#pragma once

// amy.ino

void setupAMY() {
    amy_config_t amy_config = amy_default_config();

    amy_config.features.startup_bleep  = 1;
    amy_config.features.default_synths = 1;

    // I2S DAC
    amy_config.audio    = AMY_AUDIO_IS_I2S;
    amy_config.i2s_bclk = PIN_I2S_BCLK;
    amy_config.i2s_lrc  = PIN_I2S_LRC;
    amy_config.i2s_dout = PIN_I2S_DOUT;

    // UART MIDI  (UART0: RX = GP17, TX = GP16)
    amy_config.midi      = AMY_MIDI_IS_UART;
    amy_config.midi_uart = 0;
    amy_config.midi_out  = PIN_MIDI_TX;   // GP16
    amy_config.midi_in   = PIN_MIDI_RX;   // GP17

    amy_start(amy_config);

#ifdef DEBUG
    Serial.println("AMY started");
#endif

    //config_chorus(0.75f, 320, 0.5f, 0.5f);
    //config_echo(0.5f, 150.0f, 160.0f, 0.5f, 0.0f);
    //config_reverb(0.5f, 0.85f, 0.5f, 3000.0f);


    // Load default patch (bank 0, slot 0)
    loadPatch(currentPatchIndex);
}
