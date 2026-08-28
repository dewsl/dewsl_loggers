// bool auxTriggerState = false;
// bool gsmSwitchState = false;


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
        // break;
      } else {
        inputBuffer[bufferIndex] = charbuf;
        bufferIndex++;
      }
    }
    if (BtSerialFlag && BTSerial.available() > 0) {   // this is similar to function overloading but using serial inputs
      charbuf = BTSerial.read();
      // buf = Serial.read();
      if (charbuf == '\n') {
        inputBuffer[bufferIndex] = 0x00;
        if (sizeof(inputBuffer) > 0) break;
      } else if (charbuf == '\r') {
        // break;
      } else {
        inputBuffer[bufferIndex] = charbuf;
        bufferIndex++;
      }
    }
  }
}

void debugFunction() {
  char serialLineInput[1000];
  bool debugProcess = true;
  unsigned long debugModeStart = millis();

  debugPrintln("DEBUG MODE START");

  printMenu();
  while (debugProcess) {
    // digitalWrite(COMM_SW, HIGH);
    // digitalWrite(AUX_TRIG, HIGH);
    for (int i = 0; i < sizeof(serialLineInput); i++) serialLineInput[i] = 0x00;            //  reset input buffer
    //  Waiting for serial input...
    getSerialInput(serialLineInput, sizeof(serialLineInput), DEBUGTIMEOUT);                 //  store "input" on input buffer                                                          
    if (strlen(serialLineInput) != 0) {                                                     //  filters for empty input buffer
      debugModeStart = millis();                                                            //  reset debug timer with non-empty serial input
      // debugPrintln(serialLineInput);      // remove this later
    }

    /// DEBUG MENU OPTIONS

    if (inputIs(serialLineInput, "A")) {
      char testServer[15];
      debugPrintln("OPERATION CYCLE TEST RUN");
      uint8_t testDataloggerMode = fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0);
      fetchParam(paramStorage, SERVER_NUMBER, testServer, sizeof(testServer));
      Operation(testServer, testDataloggerMode);
      debugModeStart = millis();  //update start of timeout counter
      debugPrintln("------------------------------------------------------");

    // } else if (inputIs(serialLineInput, "B")) {
      
    //   uint8_t RainCollectorType = fetchParam(paramStorage, RAIN_COLLECTOR_TYPE, (uint8_t)0);
    //   uint16_t tipEquivalent = (RTC_SLOW_MEM[EDGE_COUNT] & 0xFFFF)/2;   // Beaceuse ULP counts both rising and falling edge of interrupt, this needs to be divided by 2.
    //   debugPrintln("\nCollector type: ");
    //   char rainMsg[100];
    //   if (RainCollectorType == 0) sprintf(rainMsg, "Pronamic (0.5mm/tip)\nRain tip count: %d\nEquivalent: %0.2fmm", tipEquivalent, (tipEquivalent * 0.5));
    //   else if (RainCollectorType == 1) sprintf(rainMsg, "Davis (0.2mm/tip)\nRain tip count: %u\nEquivalent: %0.2fmm", tipEquivalent, (tipEquivalent * 0.2));
    //   debugPrintln(rainMsg);
    //   delayMillis(20);
    //   // resetRainULP();
    //   // RTC_SLOW_MEM[EDGE_COUNT] & 0xFFFF;
    //   debugModeStart = millis();
    //   debugPrintln("------------------------------------------------------");
    
// changed this function "B" for rain tips live monitor

    } else if (inputIs(serialLineInput, "AA")) {
      
      const char* oNumber = "09762481329";
      debugPrint("Rain tip count: ");
      sendSMSDump2("~", oNumber);

    } else if (inputIs(serialLineInput, "B")) {

        uint8_t RainCollectorType = fetchParam(paramStorage, RAIN_COLLECTOR_TYPE, (uint8_t)0);

        resetRainCounter(PCNT_UNIT);

        debugPrintln("\nCollector type:");
        if (RainCollectorType == 0)
            debugPrintln("Pronamic (0.5mm/tip)");
        else if (RainCollectorType == 1)
            debugPrintln("Davis (0.2mm/tip)");

        debugPrintln("Live rain monitor...");
        debugPrintln("Press any key to exit");
        debugPrintln("------------------------------------------------------");

        uint16_t prevTips = getRainCount(PCNT_UNIT, false);

        while (!Serial.available()) {

            uint16_t currentTips = getRainCount(PCNT_UNIT, false);

            if (currentTips != prevTips) {

                prevTips = currentTips;

                debugPrint("Rain tip count: ");
                debugPrintln(currentTips);

                debugPrint("Equivalent: ");

                if (RainCollectorType == 0)
                    debugPrint(currentTips * 0.5);
                else
                    debugPrint(currentTips * 0.2);

                debugPrintln(" mm");
                debugPrintln("------------------------------------------------------");
            }

            delayMillis(100);
        }

        while (Serial.available())
            Serial.read();

        debugModeStart = millis();
        debugPrintln("------------------------------------------------------");

    } else if (inputIs(serialLineInput, "BB")) {
      resetRainCounter(PCNT_UNIT);
      rainTest();
      resetRainCounter(PCNT_UNIT);
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");

    } else if (inputIs(serialLineInput, "C")) {
      printMenu();
      // printMenuBT();
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");

    } else if (inputIs(serialLineInput, "CC")) {
      // printExtraCommands();
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");

    } else if (inputIs(serialLineInput, "D")) {

      debugPrint("Saved data logger mode: ");
      debugPrintln(fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0));
      printLoggerModes();
      if (changeParameter()) {
        updateLoggerMode();
      }
      // if (loggerNameChange) {
      //   loggerNameChange = false;
      //   scalableUpdateSensorNames();
      // }
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");
    
    } else if (inputIs(serialLineInput, "E")) {

      debugPrint("Current timestamp: ");
      getTimeStamp(_timestamp, sizeof(_timestamp));
      debugPrintln(_timestamp);
      if (changeParameter()) {
        setupTime();
        debugPrint("New timestamp: ");
        debugPrintln(_timestamp);
      }
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");

    } else if (inputIs(serialLineInput, "F")) {
      bool GSMOnFlag = true;
      if (fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0) != ROUTERMODE) {
        debugPrintln("Checking network time..");
        if (digitalRead(COMM_SW) == LOW) {
          GSMOnFlag = false;
          digitalWrite(COMM_SW, HIGH);
          delayMillis(500);
          GSMInit();
        } 
        updateTimeWithGPRS();
        debugModeStart = millis();
        if (digitalRead(COMM_SW) == HIGH && !GSMOnFlag) digitalWrite(COMM_SW, LOW);   // GSM remains On if it was initially ON before test send
        debugPrintln("------------------------------------------------------");
      } else debugPrintln("Can't check network time using ROUTER mode");

    } else if (inputIs(serialLineInput, "G")) {
      char nameTemp[10] = "Empty";

      getNameFromList(0, nameTemp);
      Serial.print("Saved datalogger array name: ");
      Serial.println(nameTemp);
      fetchParam(paramStorage, ROUTER_COUNT, (uint8_t)0);

      getLoggerModeAndName();
      if (changeParameter()) {
        updateSensorNames();
      }
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");

    } else if (inputIs(serialLineInput, "H")) {
      char serverBuffer[15];  // container for server number, might be modified
      fetchParam(paramStorage,SERVER_NUMBER, serverBuffer, sizeof(serverBuffer)); 
      debugPrint("Saved Server Number: ");
      if (strlen(serverBuffer) == 0) {  //  check for first boot if server number is not yet set
        debugPrintln("[NOT SET]");    //   and prints a notice
        debugPrintln("## Default server GLOBE2 will be used ## ");
      } else {
        if (strlen(serverBuffer) == 11 || strlen(serverBuffer) == 13) {  // crude check for 09xx and +639xx based sa length, pwede pa ito palitan ng mas specific approach
          checkServerNumber(serverBuffer);                               // replaces number in buffer with name if found
          debugPrintln(serverBuffer);
          // do nothing yet.. pwede i-convert
        } else debugPrintln("## Default server GLOBE2 will be used ## ");  // prints out a notice
      }
      debugPrintln("");
      debugPrintln("Default server numbers:");
      debugPrintln("GLOBE1 - 09175972526 ; GLOBE2 - 09175388301");
      debugPrintln("SMART1 - 09088125642 ; SMART2 - 09088125639");

      if (changeParameter()) {
        updateServerNumber();
      }
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");

    } else if (inputIs(serialLineInput, "I")) {
       
      if (digitalRead(COMM_SW) == HIGH) GSMReset();
      else debugPrintln("UNABLE TO RESET: GSM power is disabled.");
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");

    } else if (inputIs(serialLineInput, "J")) {

      debugPrint("Saved Rain Collector Type: ");
      debugPrintln(fetchParam(paramStorage, RAIN_COLLECTOR_TYPE, (uint8_t)0));
      debugPrintln("[0] Pronamic Rain Collector (0.5mm/tip)");
      debugPrintln("[1] DAVIS Rain Collector (0.2mm/tip)");
      debugPrintln("[2] Generic Rain Collector (1.0/tip)");
      if (changeParameter()) {
        updateRainCollectorType();
      }
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");

    } else if (inputIs(serialLineInput, "JJ")) {

      debugPrint("Rain data type to send: ");
      debugPrintln(fetchParam(paramStorage, RAIN_DATA_TYPE, (uint8_t)0));
      debugPrintln("[0] Sends converted \"mm\" equivalent");
      debugPrintln("[1] Sends RAW TIP COUNT");
      if (changeParameter()) {
        updateRainDataType();
      }
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");

    //   } else if (inputIs(serialLineInput, "LBT_TOGGLE")) {

    //   debugPrint("Listen Mode ");
    //   if (listenMode.read()) {
    //     debugPrintln("ENABLED");
    //     debugPrintln("Disable Listen Mode?");
    //   } 
    //   else {
    //     debugPrintln("DISABLED");
    //     debugPrintln("Enable Listen Mode?");
    //     // debugPrintln("Datalogger will reset afterwards");
    //   }
      
    //   if (changeParameter()) {
    //     if ( listenMode.read()) {
    //       listenMode.write(false);
    //       debugPrintln("Listen Mode DISABLED");
    //       delayMillis(1000);
    //     }
    //     else {
    //       listenMode.write(true);
    //       debugPrintln("Listen Mode ENABLED");
    //       updateListenKey();
    //       delayMillis(1000);
    //       // debugPrintln("   Datalogger will reset..");
    //       // delayMillis(1000);
    //       // NVIC_SystemReset();                             
    //      } 
    //   }
    //   debugModeStart = millis();
    //   debugPrintln(F("------------------------------------------------------"));



    } else if (inputIs(serialLineInput, "K")) {

      debugPrint("Saved SLEEP/WAKE interval: ");
      debugPrintln(fetchParam(paramStorage, ALARM_INTERVAL,(uint8_t)0));
      printRTCIntervalEquivalent();
      if (changeParameter()) {
        setAlarmInterval();
      }
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");

    // } else if (inputIs(serialLineInput, "L")) {

    //   // converted to battery voltage input either 12v or 4.2v
    //   debugPrint("Battery voltage reference: ");
    //   debugPrintln(readBatteryVoltage(savedBatteryType.read()));
    //   debugPrintln("[0] 12V Lead Acid battery");
    //   debugPrintln("[1] 4.2V Li-Ion battery");
    //   if (changeParameter()) {
    //     setBatteryType();
    //   }



    //   debugModeStart = millis();
    //   debugPrintln(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "M")) {
      char sampleSendServer[15];
      bool GSMOnFlag = true;   // to retain GSM state before testing
      if (digitalRead(COMM_SW) == LOW) {
        GSMOnFlag = false;
        digitalWrite(COMM_SW, HIGH);
        delayMillis(500);
        GSMInit();
      } 
      // char savedServerNumber[15];
      if (loggerWithGSM(fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0))) {
        fetchParam(paramStorage,SERVER_NUMBER, sampleSendServer, sizeof(sampleSendServer)); 
        debugPrint("Send custom SMS to server: ");
        debugPrintln(sampleSendServer);
      } else {
        debugPrintln("Broadcast custom message thru LoRa: ");
      }
      testSendToServer();

      if (digitalRead(COMM_SW) == HIGH && !GSMOnFlag) digitalWrite(COMM_SW, LOW);   // GSM remains On if it was initially ON before test send
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");

    // } else if (inputIs(serialLineInput, "N")) {

    //   debugPrint("Saved GSM power mode: ");
    //   debugPrintln(savedGSMPowerMode.read());
    //   debugPrintln("[0] Always ON");                                                 //  Typically ~9-12mA @ 13v when idle (arQ mode), with short duration spikes up to 25-40mA around every 10-30 seconds (approximate).
    //   debugPrintln("[1] Low-power Mode (Always ON, but GSM SLEEPS when inactive)");  //  ~0-16mA @ 13v when idle (arQ mode). Typically 0mA, with very shord duration spikes of ~4-7mA around every 3-10 secs (approximate) and 11-17mA spikes around every 30-50sec (approximate).
    //   debugPrintln("[2] Power Saving Mode");                                         //  ~0-8mA @ 13V when idle (arQ mode). Initially, around ~7-8mA yung idle, pero after ~1-2hrs ay nagiging 0mA (sabi ni sir Don baka daw µA na yung current draw) yung idle current draw. Either very efficient yung low power mode or napapagod lang yung power supply magdispaly ng mababang values...
    //   if (changeParameter()) {
    //     setGSMPowerMode();
    //   }
    //   debugModeStart = millis();
    //   debugPrintln(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "O")) {

      debugPrintln("Input manual GSM Commands");
      debugPrintln("\"EXIT\" to close");
      unsigned long manualStart = millis();
      char manualCommandInput[100];
      char GSMResponse[500];
      while (millis() - manualStart < DEBUGTIMEOUT) {
  
        getSerialInput(manualCommandInput, sizeof(manualCommandInput), DEBUGTIMEOUT);
        debugPrintln(manualCommandInput);
        if (inputIs(manualCommandInput, "EXIT")) break;
        if (inputIs(manualCommandInput, "SEND")) GSMSerial.write(26);
        else {
          strcat(manualCommandInput, "\r");
          manualCommandInput[strlen(manualCommandInput)] = 0x00;
          GSMSerial.write(manualCommandInput);
        }
        delayMillis(1000);
        GSMAnswer(GSMResponse, sizeof(GSMResponse));
        debugPrintln(GSMResponse);
      }
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");
    
    
    } else if (inputIs(serialLineInput, "P")) {

      initINA219();
      delayMillis(1000);
      readINA219();

      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");

    // } else if (inputIs(serialLineInput, "Q")) {

    //   GSMSerial.flush();
    //   debugPrintln("TEXT MODE IN USE:");
    //   debugPrintln("To send a message, follow the format below:");
    //   debugPrintln("09123456789>>Message to send");
    //   debugPrintln("Accepted number formats: 09XXXXXXXXX or +639XXXXXXXXX");
    //   debugPrintln("IMPORTANT: Input \"EXIT\" to quit text mode.");
    //   textMode();
    //   debugModeStart = millis();
    //   debugPrintln(F("------------------------------------------------------"));

    // } else if (inputIs(serialLineInput, "R")) {

    //   debugPrintln("UPDATE SELF-RESET ALARM***");
    //   debugPrint("Current Reset Alarm Time (Military time): ");
    //   if (savedLoggerResetAlarm.read() == 0 || savedLoggerResetAlarm.read() > 2400) debugPrintln("0000");
    //   else debugPrintln(savedLoggerResetAlarm.read());
    //   if (changeParameter()) {
    //     setResetAlarmTime();
    //   }
    //   debugPrintln(F("------------------------------------------------------"));
    
    } else if (inputIs(serialLineInput, "RTC")) {
      setCompileTime();
      debugPrintln("------------------------------------------------------");

    } else if (inputIs(serialLineInput, "X") || inputIs(serialLineInput, "EXIT")) {

      // CheckingSavedParameters();
      debugPrintln("Quitting debug mode...");
      resetRainCounter(PCNT_UNIT);
      // resetRainTips();
      // if (loggerWithGSM(fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0))) deleteMessageInbox();
      // else digitalWrite(AUX_TRIG, LOW);  //should turn off GSM module
      // setNextAlarm(savedAlarmInterval.read());
      debugProcess = false;
      debugExitSkip = true;
      forDeployment = true;
      _debugReq = false;
      break;
      // Serial.end();
      // USBDevice.detach();
      debugPrintln("------------------------------------------------------");

    } else if (inputIs(serialLineInput, "T")) {
      if (gpio_get_level(GPIO_NUM_26) == 1) {debugPrintln("AUX TRIGGER DISABLED"); digitalWrite(AUX_TRIG, LOW);}
      else if (gpio_get_level(GPIO_NUM_26) == 0) {debugPrintln("AUX TRIGGER ENABLED"); digitalWrite(AUX_TRIG, HIGH);}
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------");

    // } else if (inputIs(serialLineInput, "T")) {

    //   auxTriggerState = !auxTriggerState;

    //   if (auxTriggerState) {
    //     debugPrintln("AUX TRIGGER ENABLED");
    //     digitalWrite(AUX_TRIG, HIGH);
    //   } else {
    //     debugPrintln("AUX TRIGGER DISABLED");
    //     digitalWrite(AUX_TRIG, LOW);
    //   }

    //   debugModeStart = millis();
    //   debugPrintln("------------------------------------------------------");    

    } else if (inputIs(serialLineInput, "SW")) {
      if (gpio_get_level(GPIO_NUM_2) == 1) {
        debugPrintln("COMM_SWITCH TRIGGER DISABLED");
        digitalWrite(COMM_SW, LOW);
      }
      else if (gpio_get_level(GPIO_NUM_2) == 0) {
        debugPrintln("COMM_SWITCH TRIGGER ENABLED");
        digitalWrite(COMM_SW, HIGH);
      }
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------"); 

    // } else if (inputIs(serialLineInput, "SW")) {

    //   gsmSwitchState = !gsmSwitchState;

    //   if (gsmSwitchState) {
    //     debugPrintln("COMM_SWITCH TRIGGER ENABLED");
    //     digitalWrite(COMM_SW, HIGH);
    //   } else {
    //     debugPrintln("COMM_SWITCH TRIGGER DISABLED");
    //     digitalWrite(COMM_SW, LOW);
    //   }

    //   debugModeStart = millis();
    //   debugPrintln("------------------------------------------------------");

     

    } else if (inputIs(serialLineInput, "?")) {
      savedParameters();
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------"); 

    } else if (inputIs(serialLineInput, "GSM_VOLTAGE")) {
      GSMVoltage();
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------"); 

    } else if (inputIs(serialLineInput, "LORA_SEND")) {
      char testSend[50] = "sarQ test packet";
      sendThruLoRa(testSend);
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------"); 
    
    } else if (inputIs(serialLineInput, "LORA_RCV")) {
      char receiveContainer[RH_RF95_MAX_MESSAGE_LEN];
      receiveLoRaData(receiveContainer, sizeof(receiveContainer), 120000);
      debugPrintln(receiveContainer);
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------"); 
    
    } else if (inputIs(serialLineInput, "INFO_STRING")) {
      debugPrintln("Generating info strings");
      char infoStrContainer[100];
      generateInfoMessage(infoStrContainer, sizeof(infoStrContainer));
      // infoStrContainer[strlen(infoStrContainer)] = 0x00;
      debugPrintln(infoStrContainer);
      diagnosticCheck(1);
      mDiagnosticBuilder(infoStrContainer, sizeof(infoStrContainer));
      debugPrintln(infoStrContainer);
      // generateVoltString(infoStrContainer);
      // debugPrintln(infoStrContainer);
      debugModeStart = millis();
      debugPrintln("------------------------------------------------------"); 
    }

    /// DEBUG MODE RUN TIME CHECK
    if ((millis() - debugModeStart) >= DEBUGTIMEOUT) {
      debugModeStart = millis();
      debugProcess = false;
      _debugReq = false;
      debugPrintln("TIMED OUT: Exiting debug mode");
      debugPrintln("------------------------------------------------------"); 
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

bool inputHas(const char* inputToCheck, const char* expectedInputSegment) {
  bool correctInput = false;
  char * pointerResult = strstr(inputToCheck, expectedInputSegment);
  if (pointerResult != NULL) correctInput = true;
  else (correctInput = false);
  return correctInput;
}

bool changeParameter() {
  int changeParamTimeout = 20000;  //20 sec to wait for parameter change confirmation
  char changeBuffer[10];
  bool paramBool = false;

  debugPrintln(" ");
  debugPrintln("Enter C to continue:");

  getSerialInput(changeBuffer, sizeof(changeBuffer), changeParamTimeout);

  if (inputIs(changeBuffer, "C") && strlen(changeBuffer) == 1) {
    debugPrint("\n");
    paramBool = true;
  } else {
    debugPrintln(" ");
    debugPrint("\n");
    debugPrintln("cancelled..");
    paramBool = false;
  }
  return paramBool;
}

void updateLoggerMode() {

  int loggerModeBuffer = 0;
  char addOnBuffer[10];
  uint8_t initialLoggerMode = 0;      // default value; this would change depending on what is stored
  uint8_t initialRouterCount = 0;         // default value; this would change depending on what is stored
  fetchParam(paramStorage, DATALOGGER_MODE, initialLoggerMode);
  fetchParam(paramStorage, ROUTER_COUNT, initialRouterCount);

  // printLoggerModes();
  debugPrint("Enter datalogger mode: ");
  getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
  loggerModeBuffer = atoi(addOnBuffer);

  debugPrintln(loggerModeBuffer);


  if (loggerModeBuffer == STANDALONE || loggerModeBuffer > 2) {      // arQ modes and false inputs
    
    // promt when defaulted to stand-alone mode for invalid inputs
    if (loggerModeBuffer > 2) debugPrintln("Invalid datalogger mode value; Defaulted to Stand-alone datalogger");  

    if (initialRouterCount != 0) storeParam(paramStorage, ROUTER_COUNT, (uint8_t)0);  // resets router count to prevent values from being carried over in case of mode change
    debugPrint("   Datalogger with Subsurface Sensor? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    debugPrintln(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      storeParam(paramStorage, SUBSURFACE_SENSOR_FLAG, true);
    }
    else {
      storeParam(paramStorage, SUBSURFACE_SENSOR_FLAG, false);
    }
    debugPrint("   Datalogger with UBLOX module? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    debugPrintln(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      storeParam(paramStorage, UBLOX_FLAG, true);
    }
    else {
      storeParam(paramStorage, UBLOX_FLAG, false);
    }
    storeParam(paramStorage, LISTEN_MODE, false);        // prevents LBT mode from being carried over after a mode change
  }

  else if (loggerModeBuffer == GATEWAYMODE) {  // gateways and routers
    debugPrint("   Gateway with subsurface sensor? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    debugPrintln(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      storeParam(paramStorage, SUBSURFACE_SENSOR_FLAG, true);
    }
    else {
      storeParam(paramStorage, SUBSURFACE_SENSOR_FLAG, false);
    }
    debugPrint("   Gateway with UBLOX module? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    debugPrintln(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      storeParam(paramStorage, UBLOX_FLAG, true);
    }
    else {
      storeParam(paramStorage, UBLOX_FLAG, false);
    }
    debugPrint("   Gateway broadcast command* [for LBT router(s)] [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    debugPrintln(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      storeParam(paramStorage, LISTEN_MODE, true);        
      debugPrintln("      *Listen-Before-Talk [LBT] should be enabled on ROUTER(S)");
    }  
    else {
      storeParam(paramStorage, LISTEN_MODE, false);        
    }
    debugPrint("   Input router count: ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    uint8_t inputCount = atoi(addOnBuffer);
    if (inputCount > 7) debugPrintln(" Hehe ");
    if (inputCount == 0) inputCount = 1;                                                                                               // should not accept ZERO as router count
    if (inputCount > (arrayCount(dataloggerNameList) - 1)) inputCount = (arrayCount(dataloggerNameList) - 1);  // limited to the number of rows to array holder max usable index
    
    storeParam(paramStorage, ROUTER_COUNT, inputCount);
    debugPrintln(inputCount);

  } else if (loggerModeBuffer == ROUTERMODE) {
    uint8_t tempCount = 0;
    fetchParam(paramStorage, ROUTER_COUNT, tempCount);
    if (tempCount != 0) { storeParam(paramStorage, ROUTER_COUNT, (uint8_t)0); }  // resets router count to prevent values from being carried over in case of mode change
    debugPrint("   Router with Subsurface Sensor? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    debugPrintln(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      storeParam(paramStorage, SUBSURFACE_SENSOR_FLAG, true);
    }
    else {
      storeParam(paramStorage, SUBSURFACE_SENSOR_FLAG, false);
    }
    debugPrint("   Router with UBLOX module? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    debugPrintln(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      storeParam(paramStorage, UBLOX_FLAG, true);
    }
    else {
      storeParam(paramStorage, UBLOX_FLAG, false);
    }
    debugPrint("   Router mode LISTEN-BEFORE-TALK*? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    debugPrintln(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      storeParam(paramStorage, LISTEN_MODE, true);        
      debugPrintln("      *BROADCAST COMMAND should be enabled on GATEWAY*");
    }  
    else {
      storeParam(paramStorage, LISTEN_MODE, false);        
    }  
  }

  debugPrintln("Datalogger mode updated");
  debugPrintln("");

  storeParam(paramStorage, DATALOGGER_MODE, (uint8_t)loggerModeBuffer);

  // prompts a change of names if datalogger modes are changed
  if ((initialLoggerMode != GATEWAYMODE && fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0) == GATEWAYMODE) ||                    // if initally non-gateway to gateway type
      (initialLoggerMode == GATEWAYMODE && fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0) != GATEWAYMODE) ||                    // if initially gateway type to non-gateway type
      (initialRouterCount != fetchParam(paramStorage, ROUTER_COUNT, (uint8_t)0))) {                                                    // if router count was changed
    for (byte rPos = 0; rPos < initialRouterCount; rPos++) dataloggerNameList[rPos][0] = 0x00;  // obscure previous name list
    loggerNameChange = true;                                                                                // starts name change function after function end
  }
  if (!loggerWithGSM(dMode()) GSMOff();
}

bool loggerWithGSM(uint8_t dMode) {
  if (dMode == STANDALONE || dMode == GATEWAYMODE ) return true;  // list here modes with GSM module
  else return false;
}


void updateSensorNames() {

  uint8_t currentLoggerMode = fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0);
  char nameBuffer[10];

  

  getNameFromList(0, nameBuffer);
   // reload global variable container with flash values

  // get logger mode check if gateway or standalone
  // standalone datalogger and gateway names always occupy index 0 of multidim array
  // get serial input for index 0
  // if standalone, return
  // if gateway. ask for router count input
  // loop through router allocated index (index 1 and above for router list) of array until router count input is reached

  if (currentLoggerMode == GATEWAYMODE) {  // gateways
    debugPrint("Input GATEWAY name: ");
    getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
    if (strlen(nameBuffer) == 0) sprintf(nameBuffer, "TESG");
    debugPrintln(nameBuffer);
    sprintf(dataloggerNameList[0], nameBuffer);
    for (byte listPos = 1; listPos <= fetchParam(paramStorage, ROUTER_COUNT, (uint8_t)0); listPos++) {  // router name positioning starts at index 1, 0 is self
      debugPrint("Input name of ROUTER ");
      debugPrint(listPos);
      debugPrint(": ");
      getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
      if (strlen(nameBuffer) == 0) sprintf(nameBuffer, "TEST%d", listPos);
      debugPrintln(nameBuffer);
      sprintf(dataloggerNameList[listPos], nameBuffer);
    }
    saveNameArrayToStorage();
  } else {  // stand-alone mode or router modes
    debugPrint("Current DATALOGGER name: ");
    debugPrintln(nameBuffer);
    debugPrint("Input DATALOGGER name: ");
    getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
    if (strlen(nameBuffer) == 0) sprintf(nameBuffer, "TSTXX");
    debugPrintln(nameBuffer);
    putNameOnGlobalList(0, nameBuffer);
    saveNameArrayToStorage();
  }  // other standalone routers

  updateListenKey();
}

bool requestDebug() {
  bool reqState = false;

  debugPrintln("[Input anything to enter debug]");

  unsigned long startWait = millis();
  while (millis() - startWait < 15000 && reqState == false) {
    debugPrint(".");
    delayMillis(1000);
    if (Serial.available() > 0) {
      debugPrintln("");
      debugPrintln("Debug requested..");
      _debugReq = true;
      reqState = true;
    }
  }

  debugPrintln("\n------------------------------------------------------");
  Serial.flush();
  while (Serial.available()) Serial.read();
  return reqState;
}
