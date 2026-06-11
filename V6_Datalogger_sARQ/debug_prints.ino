void printMenu() {
  debugPrintln("------------------------------------------------------");
  debugPrint("Firmware Version: ");
  debugPrint(FIRMWAREVERSION);
  debugPrintln("β");
  dateTimeNow();
  delayMillis(1000);
  debugPrintln("------------------------------------------------------");
  debugPrintln("[?] Print stored config parameters.");
  debugPrintln("[A] Test OPERATION function");
  debugPrintln("[B] Read rain gauge tips");
  debugPrintln("[C] Print this menu");
  debugPrintln("[D] Change LOGGER MODE");
  debugPrintln("[E] Set date and time manually");
  debugPrintln("[F] Set date and time using GPRS");
  debugPrintln("[G] Change DATALOGGER NAMES");
  debugPrintln("[H] Change SERVER NUMBER");
  debugPrintln("[I] Reset GSM");
  debugPrintln("[J] Set rain collector type.");
  debugPrintln("[K] Change alarm interval.");
  // Serial.println(F("[L] Set battery type (4.2V Li-ion / 12V Lead Acid)"));
  debugPrintln("[M] Send CUSTOM SMS to SERVER");
  // Serial.println(F("[N] Set GSM POWER MODE"));
  debugPrintln("[O] Manual GSM commands");
  // Serial.println(F("[P] Change SENSLOPE command."));
  // Serial.println(F("[Q] Text [Thread] Mode"));
  // Serial.println(F("[R] Update SELF RESET alarm time."));
  // Serial.println(F("[X] Exit Debug mode."));
  debugPrintln(" ");
  debugPrintln("------------------------------------------------------");
}

void printLoggerModes() {
  debugPrintln("[0] Stand-alone Datalogger (arQ mode)");  // arQ like function only: Includes rain gauge only (GSM), sa ngayon kasama yung mgay UBLOX dito, technicall sa gateway dapat sya..
  debugPrintln("[1] Gateway mode");                       // anything that send data to other datalogger through LoRa; Includes rain gauge only (LoRa)
  debugPrintln("[2] Router mode");                      // anything that wait for other datalogger LoRa data
  // Serial.println("[4] Rain gauge sensor only - GSM");      // same as gateway mode with no routers, sensor, or ublox module
  // Serial.println("[5] Rain gauge sensor only - Router");   // same with [2] but no sensors or ublox
}

void getLoggerModeAndName() {
  uint8_t mode = fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0);
  char printBuffer[50];
  char nameBuffer[10];
  
  getNameFromList(0, printBuffer);
  debugPrint("Datalogger name: ");
  debugPrintln(printBuffer);

  if (mode == GATEWAYMODE) {  //gateways
    debugPrint("\t\t GATEWAY MODE ");
    if (fetchParam(paramStorage, SUBSURFACE_SENSOR_FLAG, false)) debugPrint("with Subsurface Sensor ");
    if (fetchParam(paramStorage, UBLOX_FLAG, false)) debugPrint("+ UBLOX Module: ");
    if (fetchParam(paramStorage, SUBSURFACE_SENSOR_FLAG, false) == false && fetchParam(paramStorage, UBLOX_FLAG, false) == false) debugPrint("(Rain gauge only) ");
    debugPrint("with ");
    debugPrint(fetchParam(paramStorage, ROUTER_COUNT, (uint8_t)0));
    debugPrintln(" Router(s) ");
    if (fetchParam(paramStorage, LISTEN_MODE, false)) debugPrintln("\t\t [Broadcasts Router Commands]");
    // Serial.println("");
    debugPrint("\t\t Gateway name ");
    getNameFromList(0, printBuffer);
    debugPrintln(printBuffer);
    for (byte rCount = 1; rCount <= fetchParam(paramStorage, ROUTER_COUNT, (uint8_t)0); rCount++) {
      getNameFromList(rCount, nameBuffer);
      sprintf(printBuffer, "\t\t Router %d: %s", rCount, nameBuffer);
      debugPrintln(printBuffer);
    }
    if (fetchParam(paramStorage, LISTEN_MODE, false)) debugPrintln("\t\t [Listen Mode ENABLED]");
    

  } else {  // other standalone dataloggers
    if (mode == STANDALONE) {
      debugPrint("\t\t STAND-ALONE DATALOGGER ");
      if (fetchParam(paramStorage, SUBSURFACE_SENSOR_FLAG, false)) debugPrint("with Subsurface Sensor ");
      if (fetchParam(paramStorage, UBLOX_FLAG, false)) debugPrint("+ UBLOX Module: ");
      if (fetchParam(paramStorage, SUBSURFACE_SENSOR_FLAG, false) == false && fetchParam(paramStorage, UBLOX_FLAG, false) == false) debugPrint("(Rain gauge only) ");
    } else if (mode == ROUTERMODE) {
      Serial.print("\t\t ROUTER MODE ");
      if (fetchParam(paramStorage, SUBSURFACE_SENSOR_FLAG, false)) debugPrint("with Subsurface Sensor ");
      if (fetchParam(paramStorage, UBLOX_FLAG, false)) debugPrint("+ UBLOX Module: ");
      if (fetchParam(paramStorage, SUBSURFACE_SENSOR_FLAG, false) == false && fetchParam(paramStorage, UBLOX_FLAG, false) == false) debugPrintln("(Rain gauge only) ");
      if (fetchParam(paramStorage, LISTEN_MODE, false)) debugPrintln("[Listen Mode ENABLED]");
    }
  }
  debugPrintln("");
}

void savedParameters() {
  
  //  update global variables [from flash] that will be used
  char nvsServerNumber[15];
  char nvsSensorCommand[15];
  // fetchParam(paramStorage,SERVER_NUMBER,nvsServerNumber, sizeof(nvsServerNumber));
  // fetchParam(paramStorage,SENSOR_COMMAND,sensorCommand, sizeof(sensorCommand));
  fetchParam(paramStorage,SERVER_NUMBER, nvsServerNumber, sizeof(nvsServerNumber));
  fetchParam(paramStorage,SENSOR_COMMAND, nvsSensorCommand, sizeof(nvsSensorCommand));

  debugPrintln("------------      STORED  PARAMETERS      ------------");
  debugPrintln("------------------------------------------------------");
  debugPrintln("");
  debugPrintln(">>>>> ");
  getLoggerModeAndName();

  debugPrintln("");
  // printDateTime();  //  Shows an easily readable datetime format
  dateTimeNow();

  debugPrint("Wake interval:\t ");
  int alarmInterval = fetchParam(paramStorage, ALARM_INTERVAL,(uint8_t)0);                         //  Shows periodic alarm interval
  if (alarmInterval == 0) debugPrintln("30 minutes (hh:00 & hh:30)");  //
  else if (alarmInterval == 1) debugPrintln("30 minutes (hh:15 & hh:45)");
  else if (alarmInterval == 2) debugPrintln("15 minutes (hh:00, hh:15, hh:30, hh:45)");
  else if (alarmInterval == 3) debugPrintln("10 minutes (hh:00, hh:10, hh:20, ... )");
  else if (alarmInterval == 4) debugPrintln("5 minutes (hh:00, hh:05, hh:10, ... )");
  else debugPrintln("Default 30 minutes (hh:00 & hh:30)");
  displayNextAlarm();
  if (fetchParam(paramStorage, SUBSURFACE_SENSOR_FLAG, false)) {
    debugPrint("Sensor command:\t ");
    if (strlen(nvsSensorCommand) == 0) debugPrintln("[DEFAULT SET] - ARQCM6T");
    else debugPrintln(nvsSensorCommand);
  }

  debugPrint("Rain collector:\t ");
  if (fetchParam(paramStorage, RAIN_COLLECTOR_TYPE, (uint8_t)0) == 0) debugPrintln("Pronamic (0.5mm/tip)");
  else if (fetchParam(paramStorage, RAIN_COLLECTOR_TYPE, (uint8_t)0) == 1) debugPrintln("DAVIS (0.2mm/tip)");

  debugPrint("Rain data type:\t ");
  if (fetchParam(paramStorage, RAIN_DATA_TYPE, (uint8_t)0) == 0) debugPrintln("Sends converted \"mm\" equivalent");
  else debugPrintln("Sends RAW TIP COUNT");

  // debugPrint("Battery type:\t ");
  // if (fetchParam(paramStorage, BATTERY_TYPE, (uint8_t)0) == 1) debugPrintln("Li-ion");
  // else debugPrintln("Lead acid");
  debugPrint("Input Voltage:\t ");
  debugPrint(readINA219VoltageCurrent(0));
  debugPrintln("V");
  debugPrint("Current Draw:\t ");
  debugPrint(readINA219VoltageCurrent(1));
  debugPrintln("mA");
  // debugPrint(readVMonADC());
  // readVMonADC();
  // readADC();
  

  debugPrint("RTC temperature: ");
  if (!rtc.lostPower()) {
    debugPrint(readRTCTemp());
    debugPrintln("°C");
  }
  else debugPrintln("N/A");

  if (fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0) != ROUTERMODE) {

    debugPrint("Gsm power mode:\t ");
    if (fetchParam(paramStorage, POWER_SAVING_MODE, (uint8_t)0) == 1) debugPrintln("Low-power Mode (Always ON, but SLEEPS when inactive)");
    else if (fetchParam(paramStorage, POWER_SAVING_MODE, (uint8_t)0) == 2) debugPrintln("Power-saving mode");  // GSM module is ACTIVE when sending data, otherwise GSM module is turned OFF.
    else debugPrintln("Always ON");

    debugPrint("Server number:\t ");
    if (strlen(nvsServerNumber) == 0) {
      debugPrint(defaultServerNumber);
      debugPrintln(" [Default]");
    } else {
      debugPrint(nvsServerNumber);
      debugPrint(" ");
      checkSender(nvsServerNumber);
    }
    

  }
  debugPrintln(""); 
}

void checkSender(char* senderNum) {
  if(inputHas(senderNum,"9762481329")) debugPrintln("[DAN]");
  else if(inputHas(senderNum,"9175972526")) debugPrintln("[GLOBE1]");
  else if(inputHas(senderNum,"9175388301")) debugPrintln("[GLOBE2]");
  else if(inputHas(senderNum,"9476873967")) debugPrintln("[KATE]");
  else if(inputHas(senderNum,"9458057992")) debugPrintln("[KIM]");
  else if(inputHas(senderNum,"9770452845")) debugPrintln("[SAM]");
  else if(inputHas(senderNum,"9088125642")) debugPrintln("[SMART1]");
  else if(inputHas(senderNum,"9088125639")) debugPrintln("[SMART2]");
  else if(inputHas(senderNum,"9053648335")) debugPrintln("[WEB]");
  else if(inputHas(senderNum,"9179995183")) debugPrintln("[CHI]");
}

void printRTCIntervalEquivalent() {
  debugPrintln("[0] 30-minutes from 0th minute (0,30)");
  debugPrintln("[1] 30-minutes from 15th minute (15,45)");
  debugPrintln("[2] 15-minutes from 0th minute (0,15,30,45)");
  debugPrintln("[3] 10-minutes from 0th minute (0,10,20,30..)");
  debugPrintln("[4] 5-minutes from 0th minute (0,5,10,15...)");
}