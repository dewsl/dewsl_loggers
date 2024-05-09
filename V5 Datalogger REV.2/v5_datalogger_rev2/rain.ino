void rainInit(int rainINTPin ) {
  pinMode(rainINTPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(rainINTPin), rainISR, FALLING);
}

void rainISR() {
  LEDOn();
  const unsigned int DEBOUNCE_TIME = 75;  // 40
  int collectorType = savedRainCollectorType.read();
  unsigned long interrupt_time = millis();
  if (interrupt_time - _last_interrupt_time > DEBOUNCE_TIME) {
    _rainTips += 1;
    debugPrint("Rain tips: ");
    debugPrintln(_rainTips);
  }
  _last_interrupt_time = interrupt_time;
  LEDOff();
}

void updateRainCollectorType() {
  unsigned long updateStart = millis();
  int updateTimeout = 60000;
  int collectorTypeBuf = 0;
  uint8_t currentType = savedRainCollectorType.read();
  if (currentType == 0) debugPrintln("Pronamic (0.5mm/tip)");
  else if (currentType == 1) debugPrintln("Davis (0.2mm/tip)");


  debugPrint("Input rain collector type:");
  while (millis() - updateStart < updateTimeout) {
    if (Serial.available() > 0) {
      collectorTypeBuf = Serial.parseInt();
      break;
    }
  }
  debugPrintln(collectorTypeBuf);
  if (collectorTypeBuf > 2) {
    debugPrint("Invalid value [>2], current collector type unchanged");
  } else if (collectorTypeBuf == currentType){
    debugPrintln("Rain collector type unchanged");
  } else {
    savedRainCollectorType.write(collectorTypeBuf);
    delayMillis(500);
    debugPrintln("Rain collector type updated");
  }
}

void resetRainTips() {
  _rainTips = 0.00;
  delayMillis(75);
}