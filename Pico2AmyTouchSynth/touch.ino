
void setupTouch(){
  // TOUCH
  for (int i = 0; i < touch_count; i++) {
    touches[i].begin(touch_pins[i],10,true);
    touches[i].threshold += touch_threshold_adjust;  // make a bit more noise-proof
  }
}

void updateTouchInputs() {
 
for ( int i = 0; i < touch_count; i++) {
    touches[i].update();
    if(touches[i].touched()){
      int touchval = touches[i].raw_value / 75;
      if (touchval > 127) touchval = 127;
    }

    if ( touches[i].pressed() ) {
      touch_velocity[i]= touches[i].raw_value - touches[i].threshold;
      // Note-on
      padActive[i] = true;
      amy_event e = amy_default_event();
      e.voices[0] = i;
      e.midi_note = PAD_NOTE[i];
      e.velocity = 1.0f;
      amy_add_event(&e);
      if(debug) Serial.println(i);
      displayNeedsUpdate = true;
      digitalWrite(LED_BUILTIN, HIGH);
    }

    if ( touches[i].released() ) {
      // Note-off
      padActive[i] = false;
      amy_event e = amy_default_event();
      e.voices[0] = i;
      e.midi_note = PAD_NOTE[i];
      e.velocity = 0.0f;
      amy_add_event(&e);
      displayNeedsUpdate = true;
      digitalWrite(LED_BUILTIN, LOW);
    }

  }
}