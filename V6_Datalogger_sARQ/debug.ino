/// returns FALSE if function timeout occurs; otherwise TRUE
void getSerialInput(char* inputBuffer, int bufferLength, int inputTimeout) {
  int bufferIndex = 0;
  unsigned long readStart = millis();
  char charbuf;

  for (int i = 0; i < bufferLength; i++) inputBuffer[i] = 0x00;
  while (millis() - readStart < inputTimeout) {
    if (Serial.available() > 0) {
      charbuf = Serial.read();
      // buf = Serial.read();
      if (charbuf == '\n') {
        inputBuffer[bufferIndex] = 0x00;
        if (sizeof(inputBuffer) > 0) break;
      } else if (charbuf == '\r') {
        break;
      } else {
        inputBuffer[bufferIndex] = charbuf;
        bufferIndex++;
      }
    }
    // return true;
  }
  // return false;
}

void debugFunction() {
  char serialLineInput[1000];
  char inputBuffer[1000];
  bool debugProcess = true;
  bool inputTimedOut = false;
  unsigned long debugModeStart = millis();
  char GSMResponseBuffer[1000];

  debugPrintln("DEBUG Mode");
  while (debugProcess) {
    for (int i = 0; i < sizeof(serialLineInput); i++) serialLineInput[i] = 0x00;            //  reset input buffer
    //  Waiting for serial input...
    getSerialInput(serialLineInput, sizeof(serialLineInput), DEBUGTIMEOUT);                 //  store "input" on input buffer                                                          
    if (strlen(serialLineInput) != 0) {                                                     //  filters for empty input buffer
      debugModeStart = millis();                                                            //  reset debug timer with non-empty serial input
      debugPrintln(serialLineInput);      // remove this later
    }

    /// DEBUG MENU OPTIONS

    if (inputIs(serialLineInput, "A")) {
      Serial.println("OPERATION HERE...");
      // Operation(flashServerNumber.dataServer);
      debugModeStart = millis();  //update start of timeout counter
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "B")) {
      // Serial.println("Debug OPTION B");
      uint8_t RainCollectorType = EEPROM.read(RAIN_COLLECTOR_TYPE);
      Serial.print("\nCollector type: ");
      char rainMsg[30];
      if (RainCollectorType == 0) sprintf(rainMsg, "Pronamic (0.5mm/tip)\nRain tip count: %d\nEquivalent: %0.2fmm", tipCount, (tipCount * 0.5));
      else if (RainCollectorType == 1) sprintf(rainMsg, "Davis (0.2mm/tip)\nRain tip count: %d\nEquivalent: %0.2fmm", tipCount, (tipCount * 0.2));
      Serial.println(rainMsg);
      delayMillis(20);
      resetRainTips();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "BB")) {
      rainTest();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "C")) {
      printMenu();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "CC")) {
      // printExtraCommands();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "D")) {

      Serial.print("Saved data logger mode: ");
      Serial.println(EEPROM.readByte(DATALOGGER_MODE));
      printLoggerModes();
      if (changeParameter()) {
        updateLoggerMode();
      }
      // if (loggerNameChange) {
      //   loggerNameChange = false;
      //   scalableUpdateSensorNames();
      // }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));
    
    //  else if (inputIs(serialLineInput, "E")) {

    //   Serial.print("Current timestamp: ");
    //   getTimeStamp(_timestamp, sizeof(_timestamp));
    //   Serial.println(_timestamp);
    //   if (changeParameter()) {
    //     setupTime();
    //     Serial.print("New timestamp: ");
    //     Serial.println(_timestamp);
    //   }
    //   debugModeStart = millis();
    //   Serial.println(F("------------------------------------------------------"));

    // } else if (inputIs(serialLineInput, "F")) {

    //   if (savedDataLoggerMode.read() != ROUTERMODE) {
    //     Serial.println("Checking network time..");
    //     updateTimeWithGPRS();
    //     debugModeStart = millis();
    //     Serial.println(F("------------------------------------------------------"));
    //   } else Serial.println("Can't check network time using ROUTER mode");

    } else if (inputIs(serialLineInput, "G")) {

      EEPROM.get(DATALOGGER_NAME, flashLoggerName);
      getLoggerModeAndName();
      if (changeParameter()) {
        scalableUpdateSensorNames();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "H")) {
      char serverBuffer[20];  // container for server number, might be modified
      EEPROM.get(SERVER_NUMBER, serverBuffer);
      Serial.print("Saved Server Number: ");
      if (strlen(serverBuffer) == 0) {  //  check for first boot if server number is not yet set
        Serial.println("[NOT SET]");    //   and prints a notice
        Serial.println("## Default server GLOBE2 will be used ## ");
      } else {
        if (strlen(serverBuffer) == 11 || strlen(serverBuffer) == 13) {  // crude check for 09xx and +639xx based sa length, pwede pa ito palitan ng mas specific approach
          checkServerNumber(serverBuffer);                               // replaces number in buffer with name if found
          Serial.println(serverBuffer);
          // do nothing yet.. pwede i-convert
        } else Serial.println("## Default server GLOBE2 will be used ## ");  // prints out a notice
      }
      Serial.println("");
      Serial.println("Default server numbers:");
      Serial.println("GLOBE1 - 09175972526 ; GLOBE2 - 09175388301");
      Serial.println("SMART1 - 09088125642 ; SMART2 - 09088125639");

      if (changeParameter()) {
        updateServerNumber();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    // } else if (inputIs(serialLineInput, "I")) {

    //   GSMReset();
    //   debugModeStart = millis();
    //   Serial.println(F("------------------------------------------------------"));

    // } else if (inputIs(serialLineInput, "J")) {

    //   Serial.print("Saved Rain Collector Type: ");
    //   Serial.println(savedRainCollectorType.read());
    //   Serial.println("[0] Pronamic Rain Collector (0.5mm/tip)");
    //   Serial.println("[1] DAVIS Rain Collector (0.2mm/tip)");
    //   Serial.println("[2] Generic Rain Collector (1.0/tip)");
    //   if (changeParameter()) {
    //     updateRainCollectorType();
    //   }
    //   debugModeStart = millis();
    //   Serial.println(F("------------------------------------------------------"));

    //   } else if (inputIs(serialLineInput, "LBT_TOGGLE")) {

    //   Serial.print("Listen Mode ");
    //   if (listenMode.read()) {
    //     Serial.println("ENABLED");
    //     Serial.println("Disable Listen Mode?");
    //   } 
    //   else {
    //     Serial.println("DISABLED");
    //     Serial.println("Enable Listen Mode?");
    //     // Serial.println("Datalogger will reset afterwards");
    //   }
      
    //   if (changeParameter()) {
    //     if ( listenMode.read()) {
    //       listenMode.write(false);
    //       Serial.println("Listen Mode DISABLED");
    //       delayMillis(1000);
    //     }
    //     else {
    //       listenMode.write(true);
    //       Serial.println("Listen Mode ENABLED");
    //       updateListenKey();
    //       delayMillis(1000);
    //       // Serial.println("   Datalogger will reset..");
    //       // delayMillis(1000);
    //       // NVIC_SystemReset();                             
    //      } 
    //   }
    //   debugModeStart = millis();
    //   Serial.println(F("------------------------------------------------------"));

    // } else if (inputIs(serialLineInput, "RAIN_DATA")) {

    //   Serial.print("Rain data type to send: ");
    //   Serial.println(savedRainCollectorType.read());
    //   Serial.println("[0] Sends converted \"mm\" equivalent");
    //   Serial.println("[1] Sends RAW TIP COUNT");
    //   if (changeParameter()) {
    //     updateRainDataType();
    //   }
    //   debugModeStart = millis();
    //   Serial.println(F("------------------------------------------------------"));

    // } else if (inputIs(serialLineInput, "K")) {

    //   Serial.print("Saved SLEEP/WAKE interval: ");
    //   Serial.println(savedAlarmInterval.read());
    //   printRTCIntervalEquivalent();
    //   if (changeParameter()) {
    //     setAlarmInterval();
    //   }
    //   debugModeStart = millis();
    //   Serial.println(F("------------------------------------------------------"));

    // } else if (inputIs(serialLineInput, "L")) {

    //   // converted to battery voltage input either 12v or 4.2v
    //   Serial.print("Battery voltage reference: ");
    //   Serial.println(readBatteryVoltage(savedBatteryType.read()));
    //   Serial.println("[0] 12V Lead Acid battery");
    //   Serial.println("[1] 4.2V Li-Ion battery");
    //   if (changeParameter()) {
    //     setBatteryType();
    //   }



    //   debugModeStart = millis();
    //   Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "M")) {
      char savedServerNumber[15];
      if (loggerWithGSM(EEPROM.readByte(DATALOGGER_MODE))) {
        EEPROM.get(SERVER_NUMBER, savedServerNumber);
        Serial.print("Send custom SMS to server: ");
        Serial.println(savedServerNumber);
      } else {
        Serial.println("Broadcast custom message thru LoRa: ");
      }
      testSendToServer();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    // } else if (inputIs(serialLineInput, "N")) {

    //   Serial.print("Saved GSM power mode: ");
    //   Serial.println(savedGSMPowerMode.read());
    //   Serial.println("[0] Always ON");                                                 //  Typically ~9-12mA @ 13v when idle (arQ mode), with short duration spikes up to 25-40mA around every 10-30 seconds (approximate).
    //   Serial.println("[1] Low-power Mode (Always ON, but GSM SLEEPS when inactive)");  //  ~0-16mA @ 13v when idle (arQ mode). Typically 0mA, with very shord duration spikes of ~4-7mA around every 3-10 secs (approximate) and 11-17mA spikes around every 30-50sec (approximate).
    //   Serial.println("[2] Power Saving Mode");                                         //  ~0-8mA @ 13V when idle (arQ mode). Initially, around ~7-8mA yung idle, pero after ~1-2hrs ay nagiging 0mA (sabi ni sir Don baka daw µA na yung current draw) yung idle current draw. Either very efficient yung low power mode or napapagod lang yung power supply magdispaly ng mababang values...
    //   if (changeParameter()) {
    //     setGSMPowerMode();
    //   }
    //   debugModeStart = millis();
    //   Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "O")) {

      Serial.println("Input manual GSM Commands");
      Serial.println("\"EXIT\" to close");
      unsigned long manualStart = millis();
      char manualCommandInput[100];
      char serialInput[100];
      char GSMResponse[500];
      while (millis() - manualStart < DEBUGTIMEOUT) {
  
        getSerialInput(manualCommandInput, sizeof(manualCommandInput), DEBUGTIMEOUT);
        Serial.println(manualCommandInput);
        if (inputIs(manualCommandInput, "EXIT")) break;
        if (inputIs(manualCommandInput, "SEND")) GSMSerial.write(26);
        else {
          strcat(manualCommandInput, "\r");
          manualCommandInput[strlen(manualCommandInput)] = 0x00;
          GSMSerial.write(manualCommandInput);
        }
        delayMillis(1000);
        GSMAnswer(GSMResponse, sizeof(GSMResponse));
        Serial.println(GSMResponse);
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));
    } 
    
    // else if (inputIs(serialLineInput, "P")) {

    //   flashCommands = savedCommands.read();
    //   Serial.print("Current sensor command: ");
    //   if (strlen(flashCommands.sensorCommand) == 0) Serial.println("[NOT SET]");
    //   else Serial.println(flashCommands.sensorCommand);
    //   if (changeParameter()) {
    //     updatSavedCommand();
    //   }
    //   debugModeStart = millis();
    //   Serial.println(F("------------------------------------------------------"));

    // } else if (inputIs(serialLineInput, "Q")) {

    //   GSMSerial.flush();
    //   Serial.println("TEXT MODE IN USE:");
    //   Serial.println("To send a message, follow the format below:");
    //   Serial.println("09123456789>>Message to send");
    //   Serial.println("Accepted number formats: 09XXXXXXXXX or +639XXXXXXXXX");
    //   Serial.println("IMPORTANT: Input \"EXIT\" to quit text mode.");
    //   textMode();
    //   debugModeStart = millis();
    //   Serial.println(F("------------------------------------------------------"));

    // } else if (inputIs(serialLineInput, "R")) {

    //   Serial.println("UPDATE SELF-RESET ALARM***");
    //   Serial.print("Current Reset Alarm Time (Military time): ");
    //   if (savedLoggerResetAlarm.read() == 0 || savedLoggerResetAlarm.read() > 2400) Serial.println("0000");
    //   else Serial.println(savedLoggerResetAlarm.read());
    //   if (changeParameter()) {
    //     setResetAlarmTime();
    //   }
    //   Serial.println(F("------------------------------------------------------"));


    // } else if (inputIs(serialLineInput, "X") || inputIs(serialLineInput, "EXIT")) {

    //   CheckingSavedParameters();
    //   Serial.println("Quitting debug mode...");
    //   resetRainTips();
    //   if (loggerWithGSM(savedDataLoggerMode.read())) deleteMessageInbox();
    //   else digitalWrite(GSMPWR, LOW);  //should turn off GSM module
    //   setNextAlarm(savedAlarmInterval.read());
    //   debugProcess = false;
    //   debugMode = false;
    //   // USBDevice.detach();
    //   Serial.println(F("------------------------------------------------------"));

    // }

    else if (inputIs(serialLineInput, "?")) {
      savedParameters();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));
    }

    /// DEBUG MODE RUN TIME CHECK
    if ((millis() - debugModeStart) >= DEBUGTIMEOUT) {
      debugModeStart = millis();
      debugProcess = false;
      Serial.println(F("TIMED OUT: Exiting debug mode"));
      Serial.println(F("------------------------------------------------------"));
      break;
    }


    // if (strlen(serialLineInput) != 0) {                                                     //  
    //   debugPrint(serialLineInput);
    //   debugModeStart = millis();
    //   if(inputIs(serialLineInput, "xnd")) {
    //     GSMSerial.write(26);  
    //   } else {
    //     sprintf(inputBuffer, "%s\r", serialLineInput);
    //     GSMSerial.write(inputBuffer);
    //   }
    //   delayMillis(1000);
    //   GSMAnswer(GSMResponseBuffer, sizeof(GSMResponseBuffer));
    //   if (strlen(GSMResponseBuffer) != 0) debugPrint(GSMResponseBuffer);
    //   if (strlen(GSMResponseBuffer) != 0) GSMResponseBuffer[0]=0x00;
    // }

  }
}

bool inputIs(const char* inputFromSerial, const char* expectedInput) {
  bool correctInput = false;
  if ((strstr(inputFromSerial, expectedInput)) && (strlen(expectedInput) == strlen(inputFromSerial))) {
    correctInput = true;
  }
  return correctInput;
}

bool changeParameter() {
  int changeParamTimeout = 20000;  //20 sec to wait for parameter change confirmation
  unsigned long waitStart = millis();
  int bufIndex = 0;
  char paramBuf;
  char changeBuffer[10];
  bool paramBool = false;

  Serial.println(" ");
  Serial.println("Enter C to change:");

  getSerialInput(changeBuffer, sizeof(changeBuffer), changeParamTimeout);

  if (inputIs(changeBuffer, "C") && strlen(changeBuffer) == 1) {
    Serial.print("\n");
    paramBool = true;
  } else {
    Serial.println(" ");
    Serial.print("\n");
    Serial.println("Change cancelled.");
    paramBool = false;
  }
  return paramBool;
}

void updateLoggerMode() {

  unsigned long updateStart = millis();
  int updateTimeout = 60000;
  int loggerModeBuffer = 0;
  char addOnBuffer[10];
  uint8_t initialLoggerMode = EEPROM.readByte(DATALOGGER_MODE);
  uint8_t initialRouterCount = EEPROM.readByte(ROUTER_COUNT);

  // printLoggerModes();
  Serial.print("Enter datalogger mode: ");
  getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
  loggerModeBuffer = atoi(addOnBuffer);

  Serial.println(loggerModeBuffer);


  if (loggerModeBuffer == ARQMODE || loggerModeBuffer > 2) {      // arQ modes and false inputs
    
    // promt when defaulted to stand-alone mode for invalid inputs
    if (loggerModeBuffer > 2) Serial.println("Invalid datalogger mode value; Defaulted to Stand-alone datalogger");  

    if (initialRouterCount != 0) EEPROM.writeByte(ROUTER_COUNT, 0);  // resets router count to prevent values from being carried over in case of mode change
    Serial.print("   Datalogger with Subsurface Sensor? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      EEPROM.writeBool(SUBSURFACE_SENSOR_FLAG, true);
      EEPROM.commit();
    }
    else {
      EEPROM.writeBool(SUBSURFACE_SENSOR_FLAG, false);
      EEPROM.commit();
    }
    Serial.print("   Datalogger with UBLOX module? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      EEPROM.writeBool(UBLOX_FLAG, true);
      EEPROM.commit();
    }
    else {
      EEPROM.writeBool(UBLOX_FLAG, false);
      EEPROM.commit();
    }
    EEPROM.writeBool(LISTEN_MODE, false);        // prevents LBT mode from being carried over after a mode change
    EEPROM.commit();
  }

  else if (loggerModeBuffer == GATEWAYMODE) {  // gateways and routers
    Serial.print("   Gateway with subsurface sensor? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      EEPROM.writeBool(SUBSURFACE_SENSOR_FLAG, true);
      EEPROM.commit();
    }
    else {
      EEPROM.writeBool(SUBSURFACE_SENSOR_FLAG, false);
      EEPROM.commit();
    }
    Serial.print("   Gateway with UBLOX module? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      EEPROM.writeBool(UBLOX_FLAG, true);
      EEPROM.commit();
    }
    else {
      EEPROM.writeBool(UBLOX_FLAG, false);
      EEPROM.commit();
    }
    Serial.print("   Gateway broadcast command* [for LBT router(s)] [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      EEPROM.writeBool(LISTEN_MODE, true);        
      EEPROM.commit(); 
      Serial.println("      *Listen-Before-Talk [LBT] should be enabled on ROUTER(S)");
    }  
    else {
      EEPROM.writeBool(LISTEN_MODE, false);        
      EEPROM.commit();
    }
    Serial.print("   Input router count: ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    uint8_t inputCount = atoi(addOnBuffer);
    if (inputCount > 7) Serial.println(" Hehe ");
    if (inputCount == 0) inputCount = 1;                                                                                               // should not accept ZERO as router count
    if (inputCount > (arrayCount(flashLoggerName.sensorNameList) - 1)) inputCount = (arrayCount(flashLoggerName.sensorNameList) - 1);  // limited to the number of rows to array holder max usable index
    
    EEPROM.writeByte(ROUTER_COUNT, inputCount);
    EEPROM.commit();
    
    Serial.println(inputCount);

  } else if (loggerModeBuffer == ROUTERMODE) {
    if (EEPROM.readByte(ROUTER_COUNT) != 0) { EEPROM.writeByte(ROUTER_COUNT, 0); EEPROM.commit();}  // resets router count to prevent values from being carried over in case of mode change
    Serial.print("   Router with Subsurface Sensor? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      EEPROM.writeBool(SUBSURFACE_SENSOR_FLAG, true);
      EEPROM.commit();
    }
    else {
      EEPROM.writeBool(SUBSURFACE_SENSOR_FLAG, false);
      EEPROM.commit();
    }
    Serial.print("   Router with UBLOX module? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      EEPROM.writeBool(UBLOX_FLAG, true);
      EEPROM.commit();
    }
    else {
      EEPROM.writeBool(UBLOX_FLAG, false);
      EEPROM.commit();
    }
    Serial.print("   Router mode LISTEN-BEFORE-TALK*? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      EEPROM.writeBool(LISTEN_MODE, true);        
      EEPROM.commit(); 
      Serial.println("      *BROADCAST COMMAND should be enabled on GATEWAY*");
    }  
    else {
      EEPROM.writeBool(LISTEN_MODE, false);        
      EEPROM.commit();
    }  
  }

  Serial.println("Datalogger mode updated");
  Serial.println("");

  EEPROM.writeByte(DATALOGGER_MODE, loggerModeBuffer);
  EEPROM.commit();

  // prompts a change of names if datalogger modes are changed
  if ((initialLoggerMode != GATEWAYMODE && EEPROM.readByte(DATALOGGER_MODE) == GATEWAYMODE) ||                    // if initally non-gateway to gateway type
      (initialLoggerMode == GATEWAYMODE && EEPROM.readByte(DATALOGGER_MODE) != GATEWAYMODE) ||                    // if initially gateway type to non-gateway type
      (initialRouterCount != EEPROM.readByte(ROUTER_COUNT))) {                                                    // if router count was changed
    for (byte rPos = 0; rPos < initialRouterCount; rPos++) flashLoggerName.sensorNameList[rPos][0] = 0x00;  // obscure previous name list
    loggerNameChange = true;                                                                                // starts name change function after function end
  }
  if (!loggerWithGSM(EEPROM.readByte(DATALOGGER_MODE))) GSMOff();
}

bool loggerWithGSM(uint8_t dMode) {
  if (dMode == ARQMODE || dMode == GATEWAYMODE ) return true;  // list here modes with GSM module
  else return false;
}

void scalableUpdateSensorNames() {

  uint8_t currentLoggerMode = EEPROM.readByte(DATALOGGER_MODE);
  uint8_t expectedRouter = 0;
  char nameBuffer[20];

  EEPROM.get(DATALOGGER_NAME, flashLoggerName); // reload global variable container with flash values

  // get logger mode check if gateway or standalone
  // standalone datalogger and gateway names always occupy index 0 of multidim array
  // get serial input for index 0
  // if standalone, return
  // if gateway. ask for router count input
  // loop through router allocated index (index 1 and above for router list) of array until router count input is reached

  if (currentLoggerMode == GATEWAYMODE) {  // gateways
    Serial.print("Input GATEWAY name: ");
    getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
    if (strlen(nameBuffer) == 0) sprintf(nameBuffer, "TESG");
    Serial.println(nameBuffer);
    sprintf(flashLoggerName.sensorNameList[0], nameBuffer);
    for (byte listPos = 1; listPos <= EEPROM.readByte(ROUTER_COUNT); listPos++) {  // router name positioning starts at index 1, 0 is self
      Serial.print("Input name of ROUTER ");
      Serial.print(listPos);
      Serial.print(": ");
      getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
      if (strlen(nameBuffer) == 0) sprintf(nameBuffer, "TEST%d", listPos);
      Serial.println(nameBuffer);
      sprintf(flashLoggerName.sensorNameList[listPos], nameBuffer);
    }
  } else {  // stand-alone mode or router modes
    Serial.print("Input DATALOGGER name: ");
    getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
    if (strlen(nameBuffer) == 0) sprintf(nameBuffer, "TESTA");
    Serial.println(nameBuffer);
    sprintf(flashLoggerName.sensorNameList[0], nameBuffer);
  }  // other standalone routers

  EEPROM.put(DATALOGGER_NAME, flashLoggerName);
  EEPROM.commit();
  updateListenKey();

}

