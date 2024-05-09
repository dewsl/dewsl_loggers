void LEDInit() {
  //init LED pin as output
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

//  Turn on LED
void LEDOn() {
  digitalWrite(LED_BUILTIN, HIGH);
}

//  Turn off LED
void LEDOff() {
  digitalWrite(LED_BUILTIN, LOW);
}

//  LED send notification
//  2/30 blinkCount/blinkDurationInterval
void LEDSend() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delayMillis(30);
    digitalWrite(LED_BUILTIN, LOW);
    if (i + 1 < 2) {
      delayMillis(30);
    }
  }
}

//  LED receive notification
//  3/60 blinkCount/blinkDurationInterval
void LEDReceive() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delayMillis(60);
    digitalWrite(LED_BUILTIN, LOW);
    if (i + 1 < 3) {
      delayMillis(60);
    }
  }
}

//  LED sleep/wake notification
//  5/100 blinkCount/blinkDurationInterval
void LEDSleepWake() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delayMillis(100);
    digitalWrite(LED_BUILTIN, LOW);
    if (i + 1 < 5) {
      delayMillis(100);
    }
  }
}

//  Manual LED parameter input
//  custom blinkCount/blinkDurationInterval
void LEDManual(int blinkCount, int blinkDurationInterval) {
  for (int i = 0; i < blinkCount; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delayMillis(blinkDurationInterval);
    digitalWrite(LED_BUILTIN, LOW);
    if (i + 1 < blinkCount) {
      delayMillis(blinkDurationInterval);
    }
  }
}