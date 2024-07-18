/// LED pinout initialization
void LEDInit() {
  //init LED pin as output
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

///  Turn on LED.
void LEDOn() {
  digitalWrite(LED_BUILTIN, HIGH);
}

///  Turn off LED.
void LEDOff() {
  digitalWrite(LED_BUILTIN, LOW);
}

///  LED send notification.
///  3 quick blinks of duation 30ms with 30ms interval
///  Mas mabagal ito sa receive notification
void LEDSend() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delayMillis(60);
    digitalWrite(LED_BUILTIN, LOW);
    if (i + 1 < 3) {
      delayMillis(60);
    }
  }
}

///  LED receive notification.
///  2/30 blinkCount/blinkDurationInterval.
///  mas mabilis ito sa send notification
void LEDReceive() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delayMillis(30);
    digitalWrite(LED_BUILTIN, LOW);
    if (i + 1 < 2) {
      delayMillis(30);
    }
  }
}

///  Sleep/Wake notification.
///  5 blinks of duation 200 with 200ms interval.
void LEDSleepWake() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delayMillis(200);
    digitalWrite(LED_BUILTIN, LOW);
    if (i + 1 < 5) {
      delayMillis(200);
    }
  }
}

///  Manual LED parameter input.
///  custom blinkCount/blinkDurationInterval.
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

///  Self reset notification
///  two short (100ms) and one long(1s) LED blink
void LEDSelfReset() {
  digitalWrite(LED_BUILTIN, HIGH);
  delayMillis(100);
  digitalWrite(LED_BUILTIN, LOW);
  delayMillis(50);
  digitalWrite(LED_BUILTIN, HIGH);
  delayMillis(100);
  digitalWrite(LED_BUILTIN, LOW);
  delayMillis(50);
  digitalWrite(LED_BUILTIN, HIGH);
  delayMillis(1000);
  digitalWrite(LED_BUILTIN, LOW);

}