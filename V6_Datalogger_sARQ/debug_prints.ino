void printMenu() {
  char timeString[100];
  Serial.println(F("------------------------------------------------------"));
  Serial.print(F("Firmware Version: "));
  Serial.print(FIRMWAREVERSION);
  Serial.println("pre-alpha");
  // Serial.print("Current datetime: ");
  // printDateTime();
  delayMillis(1000);
  Serial.println(F("------------------------------------------------------"));
  Serial.println(F("[?] Print stored config parameters."));
  // Serial.println(F("[A] Test OPERATION function"));
  Serial.println(F("[B] Read rain gauge tips"));
  Serial.println(F("[C] Print this menu"));
  Serial.println(F("[D] Change LOGGER MODE"));
  // Serial.println(F("[E] Set date and time manually"));
  // Serial.println(F("[F] Set date and time using GPRS"));
  Serial.println(F("[G] Change DATALOGGER NAMES"));
  Serial.println(F("[H] Change SERVER NUMBER"));
  // Serial.println(F("[I] Reset GSM"));
  // Serial.println(F("[J] Set rain collector type."));
  // Serial.println(F("[K] Change sending interval."));
  // Serial.println(F("[L] Set battery type (4.2V Li-ion / 12V Lead Acid)"));
  Serial.println(F("[M] Send CUSTOM SMS to SERVER"));
  // Serial.println(F("[N] Set GSM POWER MODE"));
  Serial.println(F("[O] Manual GSM commands"));
  // Serial.println(F("[P] Change SENSLOPE command."));
  // Serial.println(F("[Q] Text [Thread] Mode"));
  // Serial.println(F("[R] Update SELF RESET alarm time."));
  // Serial.println(F("[X] Exit Debug mode."));
  Serial.println(F(" "));
  Serial.println(F("------------------------------------------------------"));
}

void printLoggerModes() {
  Serial.println("[0] Stand-alone Datalogger (arQ mode)");  // arQ like function only: Includes rain gauge only (GSM), sa ngayon kasama yung mgay UBLOX dito, technicall sa gateway dapat sya..
  Serial.println("[1] Gateway mode");                       // anything that send data to other datalogger through LoRa; Includes rain gauge only (LoRa)
  Serial.println("[2] Router mode");                      // anything that wait for other datalogger LoRa data
  // Serial.println("[4] Rain gauge sensor only - GSM");      // same as gateway mode with no routers, sensor, or ublox module
  // Serial.println("[5] Rain gauge sensor only - Router");   // same with [2] but no sensors or ublox
}

void getLoggerModeAndName() {
  uint8_t mode = EEPROM.readByte(DATALOGGER_MODE);
  char printBuffer[50];

  EEPROM.get(DATALOGGER_NAME, flashLoggerName);   // reload globals with flash values

  if (mode == GATEWAYMODE) {  //gateways
    Serial.print("GATEWAY MODE ");
    if (EEPROM.read(SUBSURFACE_SENSOR_FLAG)) Serial.print("with Subsurface Sensor ");
    if (EEPROM.read(UBLOX_FLAG)) Serial.print("+ UBLOX Module: ");
    if (EEPROM.read(SUBSURFACE_SENSOR_FLAG) == false && EEPROM.read(UBLOX_FLAG) == false) Serial.print("(Rain gauge only) ");
    Serial.print("with ");
    Serial.print(EEPROM.read(ROUTER_COUNT));
    Serial.println(" Router(s) ");
    if (EEPROM.readBool(LISTEN_MODE)) debugPrintln(" [Broadcasts Router Commands]");
    // Serial.println("");
    Serial.print("Gateway name ");
    Serial.println(flashLoggerName.sensorNameList[0]);
    for (byte rCount = 1; rCount <= EEPROM.read(ROUTER_COUNT); rCount++) {
      sprintf(printBuffer, "Router %d: %s", rCount, flashLoggerName.sensorNameList[rCount]);
      Serial.println(printBuffer);
    }
    if (EEPROM.readBool(LISTEN_MODE)) debugPrintln("[Listen Mode ENABLED]");

  } else {  // other standalone dataloggers
    if (mode == ARQMODE) {
      Serial.print("STAND-ALONE DATALOGGER ");
      if (EEPROM.read(SUBSURFACE_SENSOR_FLAG)) Serial.print("with Subsurface Sensor ");
      if (EEPROM.read(UBLOX_FLAG)) Serial.print("+ UBLOX Module: ");
      if (EEPROM.read(SUBSURFACE_SENSOR_FLAG) == false && EEPROM.read(UBLOX_FLAG) == false) Serial.print("(Rain gauge only) ");
    // } else if (mode == 1) {
    //   Serial.println("ARQ MODE + UBLOX Module:");
    } else if (mode == ROUTERMODE) {
      Serial.print("ROUTER MODE ");
      if (EEPROM.read(SUBSURFACE_SENSOR_FLAG)) Serial.print("with Subsurface Sensor ");
      if (EEPROM.read(UBLOX_FLAG)) Serial.print("+ UBLOX Module: ");
      if (EEPROM.read(SUBSURFACE_SENSOR_FLAG) == false && EEPROM.read(UBLOX_FLAG) == false) Serial.println("(Rain gauge only) ");
      if (EEPROM.readBool(LISTEN_MODE)) debugPrintln("[Listen Mode ENABLED]");
      // Serial.println("");
    // } else if (mode == 4) {       // should remove this later...
    //   Serial.println("RAIN GAUGE ONLY (GSM)");
    // } else if (mode == 5) {       // arQ mode GNSS sensor
    //   Serial.println("RAIN GAUGE ONLY (Router)");
    }
    Serial.println("");
    Serial.print("Datalogger name: ");
    Serial.println(flashLoggerName.sensorNameList[0]);
  }
}

void savedParameters() {
  
  //  update global variables [from flash] that will be used
  char flashServerNumber[15];
  char sensorCommand[15];
  EEPROM.get(DATALOGGER_NAME, flashLoggerName);
  EEPROM.get(SERVER_NUMBER, flashServerNumber);
  EEPROM.get(SENSOR_COMMAND, sensorCommand);

  // compute next alarm
  // DateTime now = rtc.now();
  // uint8_t nextAlarmMinuteBuffer = nextAlarm((int)(now.minute()), savedAlarmInterval.read());
  // uint8_t nextAlarmHourBuffer = now.hour();
  // uint8_t timeOfDayIndex2 = 0;
  // const char* timeOfDayEq[3] = { "AM", "PM" };                                                                //  default identifier is AM
  // if (now.minute() > nextAlarmMinuteBuffer) nextAlarmHourBuffer++;                                            //  indicates next hour for alarm
  // if (nextAlarmHourBuffer > 11 && nextAlarmHourBuffer < 24) timeOfDayIndex2 = 1;                              //  change daytime identifier to PM if true
  // if (nextAlarmHourBuffer > 12 && nextAlarmHourBuffer <= 24) nextAlarmHourBuffer = nextAlarmHourBuffer - 12;  //  subtract 12 from 24hr format to get 12hr format


  Serial.println(F("------------      STORED  PARAMETERS      ------------"));
  Serial.println(F("------------------------------------------------------"));
  Serial.println("");
  Serial.print(">>>>> ");
  getLoggerModeAndName();
  // if (listenMode.read()) {
  //   Serial.println("[Listen Mode Enabled]");
  //   // Serial.print("Listen key: ");
  //   // Serial.println(listenKey);  
  // }
  Serial.println("");
  // printDateTime();  //  Shows an easily readable datetime format
  dateTimeNow();

  // Serial.println(now.year());
  // if (now.year()%1000 < 30) { // assumes that you're not a time traveler. This should work until 2030
  //   Serial.print("Next alarm:\t ");
  //   char nextAlarmBuffer[20];
  //   if (nextAlarmMinuteBuffer != 0) sprintf(nextAlarmBuffer, "%d:%02d %s", nextAlarmHourBuffer, nextAlarmMinuteBuffer, timeOfDayEq[timeOfDayIndex2]);  //  Shows next computed alarm based on the alamr interval and current time
  //   else sprintf(nextAlarmBuffer, "%d:00 %s", nextAlarmHourBuffer, timeOfDayEq[timeOfDayIndex2]);                                                      // do ko sure kung paano gawing zero padded yung zero na hindi ginagawang char
  //   Serial.print(nextAlarmBuffer);
  //   if (EEPROM.readBool(LISTEN_MODE) && (EEPROM.readByte(DATALOGGER_MODE) == ROUTERMODE)) Serial.print(" [DISABLED]");
  //   Serial.println("");
  // }
  

  Serial.print("Wake interval:\t ");
  int alarmInterval = EEPROM.readByte(ALARM_INTERVAL);                         //  Shows periodic alarm interval
  if (EEPROM.readByte(DATALOGGER_MODE) == GATEWAYMODE || EEPROM.readBool(LISTEN_MODE) == false) {  // should update this later; consider..
    if (alarmInterval == 0) Serial.println("30 minutes (hh:00 & hh:30)");  //
    else if (alarmInterval == 1) Serial.println("15 minutes (hh:00, hh:15, hh:30, hh:45)");
    else if (alarmInterval == 2) Serial.println("10 minutes (hh:00, hh:10, hh:20, ... )");
    else if (alarmInterval == 3) Serial.println("5 minutes (hh:00, hh:05, hh:10, ... )");
    else if (alarmInterval == 4) Serial.println("3 minutes (hh:00, hh:03, hh:06, ... )");
    else if (alarmInterval == 5) Serial.println("30 minutes (hh:15 & hh:45)");
    else Serial.println("Default 30 minutes (hh:00 & hh:30)");
  } else {
    if (EEPROM.readByte(DATALOGGER_MODE) == ROUTERMODE) Serial.println("Wakes on gateway command");
  }
  if (EEPROM.readBool(SUBSURFACE_SENSOR_FLAG)) {
    Serial.print("Sensor command:\t ");
    if (strlen(sensorCommand) == 0) Serial.println("[NOT SET] Default - ARQCM6T");
    else Serial.println(sensorCommand);
  }

  Serial.print("Rain collector:\t ");
  if (EEPROM.readByte(RAIN_COLLECTOR_TYPE) == 0) Serial.println("Pronamic (0.5mm/tip)");
  else if (EEPROM.readByte(RAIN_COLLECTOR_TYPE) == 1) Serial.println("DAVIS (0.2mm/tip)");

  Serial.print("Rain data type:\t ");
  if (EEPROM.readByte(RAIN_DATA_TYPE) == 0) Serial.println("Sends converted \"mm\" equivalent");
  else Serial.println("Sends RAW TIP COUNT");

  Serial.print("Battery type:\t ");
  if (EEPROM.readByte(BATTERY_TYPE) == 1) Serial.println("Li-ion");
  else Serial.println("Lead acid");
  Serial.print("Input Voltage:\t ");
  // Serial.print(readBatteryVoltage(EEPROM.readByte(BATTERY_TYPE)));
  Serial.println("N/A");
  Serial.println("V");

  Serial.print("RTC temperature: ");
  // Serial.print(readRTCTemp());
  // Serial.println("°C");
  Serial.println("N/A");

  if (EEPROM.readByte(DATALOGGER_MODE) != ROUTERMODE) {

    Serial.print("Gsm power mode:\t ");
    if (EEPROM.readByte(GSM_POWER_MODE) == 1) Serial.println("Low-power Mode (Always ON, but SLEEPS when inactive)");
    else if (EEPROM.readByte(GSM_POWER_MODE) == 2) Serial.println("Power-saving mode");  // GSM module is ACTIVE when sending data, otherwise GSM module is turned OFF.
    else Serial.println("Always ON");

    Serial.print("Server number:\t ");
    if (strlen(flashServerNumber) == 0) {
      Serial.print(defaultServerNumber);
      Serial.println(" [Default]");
    } else {
      char serverNameBuf[20];
      sprintf(serverNameBuf, flashServerNumber);
      Serial.print(serverNameBuf);
      checkSender(serverNameBuf);
      if (strlen(serverNameBuf) != strlen(flashServerNumber)) {
        Serial.print(" [");
        Serial.print(serverNameBuf);
        Serial.println("]");
      }
    }

    // char gsmCSQResponse[200];
    // GSMSerial.write("AT+CSQ\r");
    // delayMillis(1000);
    // while (GSMSerial.available() > 0) {
    //   if (GSMGetResponse(gsmCSQResponse, sizeof(gsmCSQResponse), "+CSQ: ", 1000)) {
        
    //     int CSQval = parseCSQ(gsmCSQResponse);
    //     if (CSQval > 0) {
    //       // if (Serial) Serial.print("Checking GSM network signal..");
    //       debugPrint("CSQ: ");
    //       debugPrintln(CSQval);
    //     }
    //   }
    // }
    // for (int c = 0; c < sizeof(gsmCSQResponse); c++) gsmCSQResponse[c] = 0x00;
    // GSMSerial.write("AT+COPS?\r");
    // delayMillis(1000);
    // while (GSMSerial.available() > 0) {
    //   if (GSMGetResponse(gsmCSQResponse, sizeof(gsmCSQResponse), "+COPS: 0,1,\"", 1000)) {
    //     uint8_t netIndex = 0;
    //     char* _netInfo = strtok(gsmCSQResponse, "\"");
    //     while (_netInfo != NULL) {
    //       if (netIndex == 1) {
    //         debugPrint("Network registered: ");
    //         debugPrintln(_netInfo);
    //       }
    //       netIndex++;
    //       _netInfo = strtok(NULL, "\"");
    //     }
    //   }
    // }
  }
  debugPrintln(""); 
}

void checkSender(char* senderNum) {
  char senderNumBuffer[20];
  sprintf(senderNumBuffer, "%s", senderNum);
  senderNumBuffer[strlen(senderNumBuffer)+1]=0x00;
  if(strstr(senderNumBuffer,"9762372823")) sprintf(senderNum,"%s","DAN");
  else if(strstr(senderNumBuffer,"9179995183")) sprintf(senderNum,"%s","DON");
  else if(strstr(senderNumBuffer,"9175972526")) sprintf(senderNum,"%s","GLOBE1");
  else if(strstr(senderNumBuffer,"9175388301")) sprintf(senderNum,"%s","GLOBE2");
  else if(strstr(senderNumBuffer,"9476873967")) sprintf(senderNum,"%s","KATE");
  else if(strstr(senderNumBuffer,"9451136212")) sprintf(senderNum,"%s","JAY");
  else if(strstr(senderNumBuffer,"9287706189")) sprintf(senderNum,"%s","JJ");
  else if(strstr(senderNumBuffer,"9293175812")) sprintf(senderNum,"%s","KENNEX");
  else if(strstr(senderNumBuffer,"9458057992")) sprintf(senderNum,"%s","KIM");
  else if(strstr(senderNumBuffer,"9770452845")) sprintf(senderNum,"%s","SAM");
  else if(strstr(senderNumBuffer,"9088125642")) sprintf(senderNum,"%s","SMART1");
  else if(strstr(senderNumBuffer,"9088125639")) sprintf(senderNum,"%s","SMART2");
  else if(strstr(senderNumBuffer,"9669622726")) sprintf(senderNum,"%s","REYN");
  else if(strstr(senderNumBuffer,"9053648335")) sprintf(senderNum,"%s","WEB");
  else if(strstr(senderNumBuffer,"9179995183")) sprintf(senderNum,"%s","CHI");
}