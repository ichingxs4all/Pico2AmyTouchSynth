#pragma once

void doCalibrate(){
    for (int i = 0; i < touch_count; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        touches[i].recalibrate();
#ifdef DEBUG
        Serial.println(touches[i].threshold);
#endif
    }
#ifdef DEBUG
    Serial.println("Calibrated");
#endif
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
}
