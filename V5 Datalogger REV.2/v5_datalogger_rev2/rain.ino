void rainInit(int rainINTPin ) {
  pinMode(rainINTPin, INPUT_PULLUP);
  REG_EIC_INTFLAG = EIC_INTFLAG_EXTINT5;    // clears interrupt flag
  attachInterrupt(digitalPinToInterrupt(rainINTPin), rainISR, FALLING);
}

void rainISR() {
  int countDownMS = Watchdog.enable(16000);  // max of 16 seconds
  // if (!WDT->STATUS.bit.SYNCBUSY)                // Check if the WDT registers are synchronized
  // WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;       // Clear the watchdog timer
  LEDOn();
  // const unsigned int DEBOUNCE_TIME = 75;  // 40
  // int collectorType = savedRainCollectorType.read();
  // unsigned long interrupt_time = millis();
  // if (interrupt_time - _last_interrupt_time > DEBOUNCE_TIME) {
  _rainTips += 1;
  debugPrint("Rain tips: ");
  debugPrintln(_rainTips);
  // }
  // _last_interrupt_time = interrupt_time;
  LEDOff();
}

void updateRainCollectorType() {
  resetWatchdog();
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
  resetWatchdog();
}

void updateRainDataType() {
  resetWatchdog();
  unsigned long updateStart = millis();
  int updateTimeout = 60000;
  int newDataType = 0;
  uint8_t currentType = savedRainSendType.read();
  if (currentType == 0) debugPrintln("Sends converted \"mm\" equivalent");
  else if (currentType == 1) debugPrintln("Sends RAW TIP COUNT");


  debugPrint("Input rain data type to send:");
  while (millis() - updateStart < updateTimeout) {
    if (Serial.available() > 0) {
      newDataType = Serial.parseInt();
      break;
    }
    resetWatchdog();
  }
  debugPrintln(newDataType);
  if (newDataType > 1) {
    debugPrint("Invalid value, rain data type unchanged");
  } else if (newDataType == currentType){
    debugPrintln("Rain data type unchanged");
  } else {
    savedRainSendType.write(newDataType);
    delayMillis(500);
    debugPrintln("Rain data type updated");
  }
  resetWatchdog();
}

void resetRainTips() {
  _rainTips = 0.00;
  delayMillis(75);
  resetWatchdog();
}