void doCalibrate(){
  for (int i = 0; i < touch_count; i++) {
            digitalWrite(LED_BUILTIN, HIGH);
            touches[i].recalibrate();
            touch_threshold[i] = touches[i].threshold;
            if(debug)Serial.println(touch_threshold[i]);
          }
          if(debug)Serial.println("Calibrated");
          delay(1000);
          digitalWrite(LED_BUILTIN, LOW);
}
