void initializeULPProgram() {

  // if (RTC_SLOW_MEM[ULP_INIT_MARKER_ADDR] == ULP_INIT_MARKER) {
  //   Serial.println("ULP tip counter program already initialized.");
  //   return;
  // }
  memset(RTC_SLOW_MEM, 0, CONFIG_ULP_COPROC_RESERVE_MEM);  // set to zeros, optional
  size_t size = sizeof(ulp_program) / sizeof(ulp_insn_t);
  esp_err_t err = ulp_process_macros_and_load(SLOW_PROG_ADDR, ulp_program, &size);  // offset by PROG_ADDR

  if (err == ESP_OK) {
    debugPrintln("ULP tip counter program loaded.");
  } else if (err == ESP_ERR_NO_MEM) {
    debugPrintln("ERROR: Not enough memory to load ULP tip counter program.");
  } else {
    debugPrint("ERROR: ULP tip counter program load returned an error ");
    debugPrintln(err);
  }

  // rtc_gpio_pulldown_dis(GPIO_SENSOR_PIN);  // disable the pull-down resistor
  // rtc_gpio_pullup_en(GPIO_SENSOR_PIN);     // enable the pull-up resistor
  // rtc_gpio_hold_en(GPIO_SENSOR_PIN);       // required to maintain pull-up

  // RTC_SLOW_MEM[ULP_INIT_MARKER_ADDR] = ULP_INIT_MARKER;
}


void resetRainULP() {

  // extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
  // extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

  
  // size_t size = (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t);
  // ulp_load_binary(ulp_main_bin_start, size);
  // size_t size = sizeof(ulp_program) / sizeof(ulp_insn_t);
  // ulp_process_macros_and_load(SLOW_PROG_ADDR, ulp_program, &size);
  // RTC_SLOW_MEM[EDGE_COUNT]=0;
  // memset(RTC_SLOW_MEM, 0, CONFIG_ULP_COPROC_RESERVE_MEM);  
  // initializeULPProgram();
  
  // ulp_run(SLOW_PROG_ADDR);  // Start the ULP tip counter program with offset
  // initializeULPProgram();
  // ulp_reset();
  delayMillis(75);
  ulp_run(SLOW_PROG_ADDR);      // Start the ULP program with offset
}

void InitializeRainULP(byte RAIN_SIG) {
  pinMode(RAIN_SIG, INPUT);                                                  //  set pin as INPUT
  
  debugPrint("RTC GPIO Index = ");
  debugPrintln(rtc_io_number_get(GPIO_SENSOR_PIN));

  Serial.print("Rain tip counter initialization test: ");
  Serial.println(RTC_SLOW_MEM[EDGE_COUNT]&0xFFFF);   //  this should/might generate some value at the start but not after
  // attachInterrupt(digitalPinToInterrupt(RAIN_SIG), rainISR, FALLING);

  rtc_gpio_pulldown_dis(GPIO_SENSOR_PIN);  // disable the pull-down resistor
  rtc_gpio_pullup_en(GPIO_SENSOR_PIN);     // enable the pull-up resistor
  rtc_gpio_hold_en(GPIO_SENSOR_PIN);       // required to maintain pull-up
}

// void InitializeRainULP(byte RAIN_SIG) {
//   pinMode(RAIN_SIG, INPUT);

//   debugPrint("RTC GPIO Index = ");
//   debugPrintln(rtc_io_number_get(GPIO_SENSOR_PIN));

//   debugPrint("GPIO36 level = ");
//   debugPrintln(digitalRead(RAIN_PIN));

//   debugPrint("RTC[10] = ");
//   debugPrintln(RTC_SLOW_MEM[10]);

//   debugPrint("RTC[EDGE_COUNT] = ");
//   debugPrintln(RTC_SLOW_MEM[EDGE_COUNT]);

//   debugPrint("RTC[48] = ");
//   debugPrintln(RTC_SLOW_MEM[48]);

//   debugPrint("RTC[49] = ");
//   debugPrintln(RTC_SLOW_MEM[49]);

//   debugPrint("RTC[50] = ");
//   debugPrintln(RTC_SLOW_MEM[50]);

//   debugPrint("RTC[51] = ");
//   debugPrintln(RTC_SLOW_MEM[51]);

//   debugPrint("RTC[52] = ");
//   debugPrintln(RTC_SLOW_MEM[52]);

//   debugPrint("EDGE_COUNT = ");
//   debugPrintln(EDGE_COUNT);

//   Serial.print("Rain tip counter initialization test: ");
//   Serial.println(RTC_SLOW_MEM[EDGE_COUNT]);;

//   rtc_gpio_pulldown_dis(GPIO_SENSOR_PIN);
//   rtc_gpio_pullup_en(GPIO_SENSOR_PIN);
//   rtc_gpio_hold_en(GPIO_SENSOR_PIN);
// }

void rainTest() {
  bool runTest = true;
  uint8_t prevBuffer = tipCount; //get first value
  uint8_t currentBuffer = 0;// get next value
  debugPrint("Max tip for test limit: ");
  debugPrint(MAX_TEST_LIMIT);
  while (runTest) {

    if (prevBuffer > currentBuffer) {
      debugPrint("Rain tips ");
      debugPrint(tipCount);
    }
    if (tipCount == MAX_TEST_LIMIT) {
      debugPrintln("Rain test end");
      runTest = false;
      break;
    }
  } 
}

void updateRainCollectorType() {
  unsigned long updateStart = millis();
  int updateTimeout = 60000;
  uint8_t collectorTypeBuf = 0;
  uint8_t currentType = fetchParam(paramStorage, RAIN_COLLECTOR_TYPE, (uint8_t)0);
  if (currentType == 0) debugPrintln("Pronamic (0.5mm/tip)");
  else if (currentType == 1) debugPrintln("Davis (0.2mm/tip)");

  debugPrint("Input rain collector type: ");
  while (millis() - updateStart < updateTimeout) {
    if (Serial.available() > 0) {
      collectorTypeBuf = Serial.parseInt();
      break;
    }
    if (BTSerial.available() > 0) {
      collectorTypeBuf = BTSerial.parseInt();
      break;
    }
  }
  debugPrintln(collectorTypeBuf);
  if (collectorTypeBuf > 2) {
    debugPrint("Invalid value outside index; rain collector type unchanged.");
  } else if (collectorTypeBuf == currentType) {
    debugPrintln("Rain collector type unchanged");
  } else {
    storeParam(paramStorage, RAIN_COLLECTOR_TYPE, (uint8_t)collectorTypeBuf);
    delayMillis(500);
    debugPrintln("Rain collector type updated");
  }
}

void updateRainDataType() {
  unsigned long updateStart = millis();
  int updateTimeout = 60000;
  int newDataType = 0;
  uint8_t currentType = fetchParam(paramStorage, RAIN_DATA_TYPE, (uint8_t)0);
  if (currentType == 0) debugPrintln("Sends converted \"mm\" equivalent");
  else if (currentType == 1) debugPrintln("Sends RAW TIP COUNT");


  debugPrint("Input rain data type to send:");
  while (millis() - updateStart < updateTimeout) {
    if (Serial.available() > 0) {
      newDataType = Serial.parseInt();
      break;
    }
    if (BTSerial.available() > 0) {
      newDataType = BTSerial.parseInt();
      break;
    }
  }
  debugPrintln(newDataType);
  if (newDataType > 1) {
    debugPrint("Invalid value, rain data type unchanged");
  } else if (newDataType == currentType){
    debugPrintln("Rain data type unchanged");
  } else {
    storeParam(paramStorage, RAIN_DATA_TYPE, (uint8_t)newDataType);
    delayMillis(500);
    debugPrintln("Rain data type updated");
  }
}