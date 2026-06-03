void setupAMY(){

//AMY CONFIG
amy_config_t amy_config = amy_default_config();
amy_config.features.startup_bleep = 1;
amy_config.features.default_synths = 1;

//AMY uses a I2S DAC
amy_config.audio = AMY_AUDIO_IS_I2S;
amy_config.i2s_bclk = PIN_I2S_BCLK;
amy_config.i2s_lrc = PIN_I2S_LRC;
amy_config.i2s_dout = PIN_I2S_DOUT;

// Switch to UART0 (midi_uart = 0).
// midi_in must be the RX pin. On UART0, RX is GP17, and TX is GP16. You had midi_in = 16, but GP16 is UART0's TX — it can't receive. So midi_in should be 17.
amy_config.midi      = AMY_MIDI_IS_UART;
amy_config.midi_uart = 0;   // GP16/GP17 live on UART0, not UART1
amy_config.midi_out  = 16;  // UART0 TX
amy_config.midi_in   = 17;  // UART0 RX  ← this is the pin that listens

amy_start(amy_config);

if(debug) Serial.print("AMY started");
 
 // Load default patch
loadPatch(currentPatchIndex);

}

