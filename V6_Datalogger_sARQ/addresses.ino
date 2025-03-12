#define ARQMODE       0
#define GATEWAYMODE   1
#define ROUTERMODE    2

/// This section is used for flash storage adressed definitions as defined below.
/// NOTE: Flash variables can be retained even after new code is uploaded.

#define FLASH_USAGE_INDICATOR   0
#define ALARM_INTERVAL          1
#define DATALOGGER_MODE         2
#define SUBSURFACE_SENSOR_FLAG  3
#define UBLOX_FLAG              4
#define ROUTER_COUNT            5
#define GSM_POWER_MODE          6
#define RAIN_COLLECTOR_TYPE     7
#define RAIN_DATA_TYPE          8
#define BATTERY_TYPE            9
#define LORA_RECEIVE_MODE       10
#define PARAMETER_SET_INDICATOR 11
#define LOGGER_RESET_HH         12
#define LOGGER_RESET_MM         13
#define SENSOR_COMMAND          14
#define SERVER_NUMBER           15    // 15 to 29
#define AUTO_POWER_SAVING       30
#define LISTEN_MODE             31
// #define RESERVED 32
// #define RESERVED 33
// #define RESERVED 34
// #define RESERVED 35
// #define RESERVED 36
// #define RESERVED 37
// #define RESERVED 38
// #define RESERVED 39
#define DATALOGGER_NAME         40    // 40+


/// Overwrites all flash variables with default values
void setDefaultValues(const char * defaultServer) {
  char nameBuffer[10];
  if (EEPROM.readByte(FLASH_USAGE_INDICATOR) != 99) {
    Serial.println("Setting default values..");
    
    EEPROM.writeBool(ALARM_INTERVAL, 0);  EEPROM.commit();
    EEPROM.writeBool(DATALOGGER_MODE, 0); EEPROM.commit();
    EEPROM.writeBool(SUBSURFACE_SENSOR_FLAG, 0);  EEPROM.commit();
    EEPROM.writeBool(UBLOX_FLAG, 0);  EEPROM.commit();
    EEPROM.writeBool(ROUTER_COUNT, 0);  EEPROM.commit();
    EEPROM.writeBool(GSM_POWER_MODE, 0);  EEPROM.commit();
    EEPROM.writeBool(RAIN_COLLECTOR_TYPE, 0);  EEPROM.commit();
    EEPROM.writeBool(RAIN_DATA_TYPE, 0);  EEPROM.commit();
    EEPROM.writeBool(BATTERY_TYPE, 0);  EEPROM.commit();
    EEPROM.writeBool(LORA_RECEIVE_MODE, 0);  EEPROM.commit();
    EEPROM.writeBool(PARAMETER_SET_INDICATOR, 0);  EEPROM.commit();
    EEPROM.writeBool(LOGGER_RESET_HH, 0);  EEPROM.commit();
    EEPROM.writeBool(LOGGER_RESET_MM, 0);  EEPROM.commit();
    EEPROM.put(SERVER_NUMBER, defaultServerNumber); EEPROM.commit();  //
    EEPROM.writeBool(AUTO_POWER_SAVING, 0);  EEPROM.commit();

    for (byte f = 0; f < MAX_NAME_COUNT; f++) {       // to prevent stack smashing when increasing router count
      sprintf(nameBuffer, "DFLT%d", f);
      sprintf(flashLoggerName.sensorNameList[f], nameBuffer);
    }
    EEPROM.put(DATALOGGER_NAME, flashLoggerName); EEPROM.commit();

    EEPROM.writeByte(FLASH_USAGE_INDICATOR, 99);
    EEPROM.commit();

  } else Serial.println("Default values used");
}

// void reloadGlobalVariables() {
//   EEPROM.get(DATALOGGER_NAME, flashLoggerName);
//   getTimeStamp(_timestamp, sizeof(_timestamp));
//   // Add global variables that need to be reloaded here
// }


void initializeULPProgram() {

  // if (RTC_SLOW_MEM[ULP_INIT_MARKER_ADDR] == ULP_INIT_MARKER) {
  //   Serial.println("ULP program already initialized.");
  //   return;
  // }
  memset(RTC_SLOW_MEM, 0, CONFIG_ULP_COPROC_RESERVE_MEM);  // set to zeros, optional
  size_t size = sizeof(ulp_program) / sizeof(ulp_insn_t);
  esp_err_t err = ulp_process_macros_and_load(SLOW_PROG_ADDR, ulp_program, &size);  // offset by PROG_ADDR

  if (err == ESP_OK) {
    Serial.println("ULP program loaded.");
  } else if (err == ESP_ERR_NO_MEM) {
    Serial.println("ERROR: Not enough memory to load ULP program.");
  } else {
    Serial.printf("ERROR: ULP program load returned unexpected error %d\n", err);
  }

  rtc_gpio_pulldown_dis(GPIO_SENSOR_PIN);  // disable the pull-down resistor
  rtc_gpio_pullup_en(GPIO_SENSOR_PIN);     // enable the pull-up resistor
  rtc_gpio_hold_en(GPIO_SENSOR_PIN);       // required to maintain pull-up

  // RTC_SLOW_MEM[ULP_INIT_MARKER_ADDR] = ULP_INIT_MARKER;
}