void getSerialInput(char* inputBuffer, int bufferLength, int inputTimeout) {
  int bufferIndex = 0;
  unsigned long readStart = millis();
  char charbuf;

  for (int i = 0; i < bufferLength; i++) inputBuffer[i] = 0x00;

  while (millis() - readStart < inputTimeout) {
    if (Serial.available() > 0 ) {
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
        // readStart = millis();
      }
    }
  }
  // strncpy(inputBuffer, readSerialBuffer, strlen(readSerialBuffer));
}

void debugFunction() {
  char serialLineInput[1000];
  bool debugProcess = true;
  unsigned long debugModeStart = millis();

  while (debugProcess) {   

    for (int i = 0; i < sizeof(serialLineInput); i++) serialLineInput[i] = 0x00;

    getSerialInput(serialLineInput, sizeof(serialLineInput), 60000);           
    
    // updates time-out with every input.
    if (strlen(serialLineInput) != 0) {
      debugModeStart = millis();                               
    }

    //  processes serial input accordingly
    if (inputIs(serialLineInput, "A")) {
      Serial.println("Run debug menu A");
      // dueDataCollection(dataDump, 2000, SAMPLINGTIMEOUT);
      Operation(flashServerNumber.dataServer);
      debugModeStart = millis(); //update start of timeout counter
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "B")) {
      uint8_t RainCollectorType = savedRainCollectorType.read();
      Serial.print("\nCollector type: ");
      char rainMsg[30];
      if (RainCollectorType == 0) sprintf(rainMsg, "Pronamic (0.5mm/tip)\nRain tips: %0.2f tips\nEquivalent: %0.2fmm",_rainTips,(_rainTips*0.5));
      else if (RainCollectorType == 1)  sprintf(rainMsg, "Davis (0.2mm/tip)\nRain tips: %0.2f tips\nEquivalent: %0.2fmm",_rainTips,(_rainTips*0.2));
      Serial.println(rainMsg);
      delayMillis(20);
      resetRainTips();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));
      
    } else if (inputIs(serialLineInput, "C")) {
      printMenu();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "D")) {
      Serial.print("Saved data logger mode: ");
      Serial.println(savedDataLoggerMode.read());
      printLoggerModes();
      if (changeParameter()) {
        updateLoggerMode();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "E")) {
      Serial.println("Run debug menu E");
      Serial.print("Current timestamp: ");
      getTimeStamp(_timestamp, sizeof(_timestamp));
      Serial.println(_timestamp);
      if (changeParameter()) {
        setupTime();
        Serial.print("New timestamp: ");
        Serial.println(_timestamp);
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "F")) {
      if (savedDataLoggerMode.read() != 2) {
        Serial.println("Checking network time..");
        updateTimeWithGPRS();
        debugModeStart = millis();
        Serial.println(F("------------------------------------------------------"));
      } else Serial.println("Can't check network time using ROUTER mode");
      
    } else if (inputIs(serialLineInput, "G")) {

      flashLoggerName = savedLoggerName.read();
      getLoggerModeAndName();
      if (changeParameter()) {
        scalableUpdateSensorNames();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "H")) {
      char serverBuffer[20];                                                  // container for server number, might be modified
      sprintf(serverBuffer, flashServerNumber.dataServer);
      Serial.print("Saved Server Number: ");
      if (strlen(serverBuffer) == 0) {                                        //  check for first boot if server number is not yet set
        Serial.println("[NOT SET]");                                          //   and prints a notice
        Serial.println("## Default server GLOBE2 will be used ## ");
      } else {
        if (strlen(serverBuffer) == 11 || strlen(serverBuffer) == 13) {       // crude check for 09xx and +639xx based sa length, pwede pa ito palitan ng mas specific approach
          checkServerNumber(serverBuffer);                                    // replaces number in buffer with name if found
          Serial.println(serverBuffer);
        // do nothing yet.. pwede i-convert
        } else Serial.println("## Default server GLOBE2 will be used ## ");   // prints out a notice
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

    } else if (inputIs(serialLineInput, "I")) {
      Serial.println("Resetting the GSM module...");
      GSMReset();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "J")) {
      Serial.print("Saved Rain Collector Type: ");
      Serial.println(savedRainCollectorType.read());
      Serial.println("[0] Pronamic Rain Collector (0.5mm/tip)");
      Serial.println("[1] DAVIS Rain Collector (0.2mm/tip)");
      Serial.println("[2] Generic Rain Collector (1.0/tip)");
      if (changeParameter())  {
        updateRainCollectorType();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "K")) {
      Serial.println("Run debug menu K");
      Serial.print("Saved alarm interval: ");
      Serial.println(savedAlarmInterval.read());
      printRTCIntervalEquivalent();
      if (changeParameter()) {
        setAlarmInterval();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "L")) {
      // converted to battery voltage input either 12v or 4.2v
      Serial.print("Battery voltage reference: ");
      Serial.println(readBatteryVoltage(savedBatteryType.read()));
      Serial.println("[0] 12 Lead Acid battery");
      Serial.println("[1] 4.2 Li-Ion battery");
      if (changeParameter()) {
      setBatteryType();
      }



      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));
    } else if (inputIs(serialLineInput, "M")) {
      flashServerNumber = savedServerNumber.read();
      Serial.print("Send custom SMS to server: ");
      Serial.println(flashServerNumber.dataServer);
      testSendToServer();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "N")) {
      Serial.print("Saved GSM power mode: ");
      Serial.println(savedGSMPowerMode.read());
      Serial.println("[0] Always ON");
      Serial.println("[1] Low-power Mode (Always ON, but GSM SLEEPS when inactive)");
      Serial.println("[2] Power Saving Mode");
      if (changeParameter()) {
      setGSMPowerMode();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));



    } else if (inputIs(serialLineInput, "O")) {
      Serial.println("Input manual GSM Commands");
      unsigned long manualStart = millis();
      char manualCommandInput[100];
      char GSMResponse[500];
      while (millis() - manualStart < DEBUGTIMEOUT) {
        getSerialInput(manualCommandInput, sizeof(manualCommandInput), DEBUGTIMEOUT);
        if (inputIs(manualCommandInput, "EXIT")) break;
        strcat(manualCommandInput, "\r");
        manualCommandInput[strlen(manualCommandInput)+1]=0x00;
        GSMSerial.write(manualCommandInput);
        delayMillis(1000);
        GSMAnswer(GSMResponse, sizeof(GSMResponse));
        Serial.println(GSMResponse);
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "P")) {
        Serial.println("Run debug menu P");
        flashCommands = savedCommands.read();
        Serial.print("Current sensor command: ");
        if (strlen(flashCommands.sensorCommand)==0) Serial.println("[NOT SET]");
        else Serial.println(flashCommands.sensorCommand);
        if (changeParameter()) {
          updatSavedCommand();
        }


      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));


    } else if (inputIs(serialLineInput, "Q")) {
      GSMSerial.flush();
      Serial.println("TEXT MODE IN USE:");
      Serial.println("To send a message, follow the format below:");
      Serial.println("09123456789>>Message to send");
      Serial.println("Accepted number formats: 09XXXXXXXXX or +639XXXXXXXXX");
      Serial.println("IMPORTANT: Input \"EXIT\" to quit text mode.");
      textMode();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "X" ) || inputIs(serialLineInput, "EXIT" ) ) {
      Serial.println("Exit debug mode");
      debugProcess = false;
      debugMode = false;
      deleteMessageInbox();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "LORA_SEND_TEST")) {
      char payloadContainer[200];
      Serial.print("Input LoRa test data: ");
      getSerialInput(payloadContainer, sizeof(payloadContainer), 60000);
      Serial.println(payloadContainer);
      sendThruLoRa(payloadContainer);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "LORA_SEND_2_TEST")) {
      char payloadContainer[200];
      Serial.print("Input LoRa test data: ");
      getSerialInput(payloadContainer, sizeof(payloadContainer), 60000);
      Serial.println(payloadContainer);
      sendThruLoRaWithAck(payloadContainer, 2000, 0);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "LORA_WAIT_TEST")) {
      Serial.println("Waiting for LoRa transmission: ");
      char payloadContainer[RH_RF95_MAX_MESSAGE_LEN+1];
      receiveLoRaData(payloadContainer, sizeof(payloadContainer), 180000);
      Serial.println(payloadContainer);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));
    
    } else if (inputIs(serialLineInput, "LORA_WAIT_2_TEST")) {
      uint8_t receiveType = 99;
      char debugBuffer[500];
      // Serial.println("Waiting for LoRa transmission from listed router(s): ");
      waitForLoRaRouterData(200000,2,1);
      // char payloadContainer[RH_RF95_MAX_MESSAGE_LEN+1];
      // receiveLoRaData(payloadContainer, sizeof(payloadContainer));
      // Serial.println(payloadContainer);
      // receiveType = loRaFilterPass(payloadContainer, sizeof(payloadContainer));
      // Serial.println(receiveType);
      // if (receiveType > 0) {  
      //   Serial.print("Receieved from ");
      //   extractRouterName(debugBuffer, payloadContainer);
      //   Serial.println(debugBuffer);
      // }
      // if (strstr(debugBuffer, ">>VOLT")) {
      
      // }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "VOLT_STRING")) {
      char voltContainter[100];
      generateVoltString(voltContainter);
      Serial.println(voltContainter);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

      } else if (inputIs(serialLineInput, "INFO_STRING")) {
      char infoContainter[100];
      // generateVoltString(voltContainter);
      generateInfoMessage(infoContainter);
      infoContainter[strlen(infoContainter)+1]=0x00;
      Serial.println(infoContainter);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "CHECK_OTA")) {
      checkOTACommand();
      deleteMessageInbox();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "?")) {
      savedParameters();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));
    }
    if ((millis() - debugModeStart) > DEBUGTIMEOUT) {
      debugModeStart = millis();
      debugProcess = false;
      Serial.println(F("Debug Menu Timeout!"));
      Serial.println(F("------------------------------------------------------"));
      break;
    }
  }
  
  if (!debugProcess) {
      debugMode = false;
      Serial.println(F("------------------------------------------------------"));
      Serial.println(F("Exiting from DEBUG MENU"));
      Serial.println(F("------------------------------------------------------"));
      setNextAlarm(savedAlarmInterval.read());
      delayMillis(75);
      rtc.clearINTStatus();
      GSMInit();
      return;
  }
}

void printMenu() {
  char timeString[100];
  Serial.println(F("------------------------------------------------------"));
  Serial.print(F("Firmware Version: "));
  Serial.print(FIRMWAREVERSION);
  Serial.println("α");
  Serial.print("Current datetime: ");
  printDateTime();
  Serial.println(F("------------------------------------------------------"));
  Serial.println(F("[?] Print stored config parameters."));
  Serial.println(F("[A] Get data from customDue"));
  Serial.println(F("[B] Read rain gauge tips"));
  Serial.println(F("[C] Print this menu"));
  Serial.println(F("[D] Change LOGGER MODE"));
  Serial.println(F("[E] Set date and time manually"));
  Serial.println(F("[F] Set date and time using GPRS"));
  Serial.println(F("[G] Change DATALOGGER NAMES"));
  Serial.println(F("[H] Change SERVER NUMBER"));
  Serial.println(F("[I] Reset GSM"));
  Serial.println(F("[J] Set rain collector type."));
  Serial.println(F("[K] Change sending interval."));
  Serial.println(F("[L] Set battery type (4.2V Li-ion / 12V Lead Acid)"));
  Serial.println(F("[M] Send CUSTOM SMS to SERVER"));
  Serial.println(F("[N] Set GSM POWER MODE"));
  Serial.println(F("[O] Manual GSM commands"));
  Serial.println(F("[P] Change SENSLOPE command."));
  Serial.println(F("[Q] Text Mode."));
  Serial.println(F("[X] Exit Debug mode."));
  Serial.println(F(" "));
  Serial.println(F("------------------------------------------------------"));
}

void printRTCIntervalEquivalent() {
  // Serial.println("------------------------------------------------");
  Serial.println("[0] 30-minutes from 0th minute (0,30)");
  Serial.println("[1] 15-minutes from 0th minute (0,15,30,45)");
  Serial.println("[2] 10-minutes from 0th minute (0,10,20,30..)");
  Serial.println("[3] 5-minutes from 0th minute (0,5,10,15...)");
  Serial.println("[4] 3-minutes from 0th minute (0,3,6,9,12...)");
  Serial.println("[5] 30-minutes from 15th minute (15,45)");
  // Serial.println("------------------------------------------------");
}

void printLoggerModes() {
  Serial.println("[0] arQ mode - Standalone Datalogger");           // arQ like function only
  Serial.println("[1] arQ mode + UBLOX Rover");                    // arQ like + Ublox router
  Serial.println("[2] Router mode");                                // anything that send data through LoRa
  Serial.println("[3] Gateway mode");                               // anything that recevies LoRa data
  Serial.println("[4] Rain gauge sensor only - GSM");
  Serial.println("[5] Rain gauge sensor only - Router");
}

void updateLoggerMode() {
  unsigned long updateStart = millis();
  int updateTimeout = 60000;
  int loggerModeBuffer = 0;
  char addOnBuffer[10];
  uint8_t initialLoggerMode = savedDataLoggerMode.read();
  uint8_t initialRouterCount = savedRouterCount.read();
  

  Serial.print("Enter datalogger mode: ");
  getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
  loggerModeBuffer = atoi(addOnBuffer);

  // while (millis() - updateStart < updateTimeout) {
  //   if (Serial.available() > 0) {
  //     loggerModeBuffer = atoi(Serial.read());
  //     Serial.println("");
  //     break;
  //   }
  // }
  
  Serial.println(loggerModeBuffer);

  if (loggerModeBuffer > 6) {        //rejects values above 5
    Serial.println("Unchanged: Invalid datalogger mode value");
    return;
  }

  if (loggerModeBuffer == 3) {    // gateways and routers
    Serial.print("   Gateway with subsurface sensor? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer,"Y")) || (inputIs(addOnBuffer,"y"))) hasSubsurfaceSensorFlag.write(99);
    Serial.print("   Gateway with UBLOX rover? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer,"Y")) || (inputIs(addOnBuffer,"y"))) hasUbloxRouterFlag.write(99);
    Serial.print("   Input router count: ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    uint8_t rcount = atoi(addOnBuffer);
    if (rcount == 0) rcount = 1;  // should not accept ZERO as router count
    if (rcount > (arrayCount(flashLoggerName.sensorNameList)-1)) rcount = (arrayCount(flashLoggerName.sensorNameList)-1);   // limited to the number of rows of array holder 
    savedRouterCount.write(rcount);
    Serial.println(rcount);
  
  } else if (loggerModeBuffer == 2) {
    Serial.print("   Router with Subsurface Sensor? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer,"Y")) || (inputIs(addOnBuffer,"y"))) hasSubsurfaceSensorFlag.write(99);
    Serial.print("   Router with UBLOX Rover? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer,"Y")) || (inputIs(addOnBuffer,"y"))) hasUbloxRouterFlag.write(99);

  } else if (loggerModeBuffer == 1) {     // arQ mode + UBLOX rover
    hasUbloxRouterFlag.write(99);
  }
  else {   // other non-gateway and non router datalogger
  }
  Serial.println("Datalogger mode updated");
  Serial.println("");

  savedDataLoggerMode.write(loggerModeBuffer);

  // prompts a change of names if datalogger modes are changed
  if ((initialLoggerMode != 3 && savedDataLoggerMode.read() == 3) ||     // if initally non-gateway to gateway type
      (initialLoggerMode == 3 && savedDataLoggerMode.read() != 3) ||     // if initially gateway type to non-gateway type
      (initialRouterCount != savedRouterCount.read())) {                  // if router count was changed
      for (byte rCount = 0; rCount < initialRouterCount; rCount++) flashLoggerName.sensorNameList[rCount][0] = 0x00;  // obscure previous name list 
      scalableUpdateSensorNames();
    }
}

bool inputIs(const char *inputFromSerial, const char* expectedInput) {
  bool correctInput = false;
  if ((strstr(inputFromSerial,expectedInput)) && (strlen(expectedInput) == strlen (inputFromSerial))) {
    correctInput = true;
  }
  return correctInput;
}


void delayMillis(int delayDuration) {
  int maxDelayDuration = 15000; //15sec
  bool maxDurationReached = false;
  unsigned long timeStart = millis();
  while (!maxDurationReached) {
    if ((millis() - timeStart) > maxDelayDuration) {
      Serial.println("Max delay duration: 15000");
      return;
    }
    if ((millis() - timeStart) > delayDuration) {
      return;
    }
    
    //
  }
}

bool changeParameter() {
  int changeParamTimeout = 20000;  //20 sec to wait for parameter change confirmation
  unsigned long waitStart = millis();
  int bufIndex = 0;
  char paramBuf;
  char changeBuffer[10];

  Serial.println(" ");
  Serial.println("Enter C to change:");

  getSerialInput(changeBuffer, sizeof(changeBuffer), changeParamTimeout);

  if ((changeBuffer[0] == 'C' || changeBuffer[0] == 'c') && strlen(changeBuffer) == 1) {
    Serial.print("\r");
    return true;
  } else {
    Serial.println(" ");
    Serial.print("\r");
    Serial.println("Change cancelled.");
    return false;
  }
}

void savedParameters() {
  flashLoggerName = savedLoggerName.read();
  flashServerNumber = savedServerNumber.read();
  flashCommands = savedCommands.read();

  Serial.println(F("------------      STORED  PARAMETERS      ------------"));
  Serial.println(F("------------------------------------------------------"));
    Serial.println("");
    Serial.print("Date & Time:\t ");
    printDateTime();
    getTimeStamp(_timestamp, sizeof(_timestamp));
    Serial.print("Timestamp:\t ");
    Serial.println(_timestamp);
    Serial.println("");
    Serial.print(">>>> ");
    getLoggerModeAndName();
    Serial.print("Sensor command:\t ");
    if (strlen(flashCommands.sensorCommand) == 0) Serial.println("[NOT SET] Default - ARQCM6T");
    else Serial.println(flashCommands.sensorCommand);

    Serial.print("Wake interval:\t ");
    int alarmInterval = savedAlarmInterval.read();
    if (alarmInterval == 0)       Serial.println("30 minutes (hh:00 & hh:30)");
    else if (alarmInterval == 1)  Serial.println("15 minutes (hh:00, hh:15, hh:30, hh:45)");
    else if (alarmInterval == 2)  Serial.println("10 minutes (hh:00, hh:10, hh:20, ... )");
    else if (alarmInterval == 3)  Serial.println("5 minutes (hh:00, hh:05, hh:10, ... )");
    else if (alarmInterval == 4)  Serial.println("3 minutes (hh:00, hh:03, hh:06, ... )");
    else if (alarmInterval == 5)  Serial.println("30 minutes (hh:15 & hh:45)");
    else                          Serial.println("Default 30 minutes (hh:00 & hh:30)");
    
    Serial.print("Rain collector:\t ");
    if (savedRainCollectorType.read() == 0)       Serial.println("Pronamic (0.5mm/tip)");
    else if (savedRainCollectorType.read() == 1)  Serial.println("DAVIS (0.2mm/tip)");
    
    Serial.print("Rain data type:\t ");
    if (savedRainSendType.read()==0)  Serial.println("Converted \"mm\" equivalent");
    else                              Serial.println("RAW TIP COUNT");

    Serial.print("Battery type:\t ");
    if (savedBatteryType.read() == 1) Serial.println("Li-ion");
    else  Serial.println("Lead acid");
    Serial.print("Input Voltage:\t ");
    Serial.print(readBatteryVoltage(savedBatteryType.read()));
    Serial.println("V");


    if (_timestamp[0] == '2') {  // temporay check for timestamp validity
      Serial.print("RTC temperature: ");
      Serial.print(readRTCTemp());
      Serial.println("°C");
    }
    
    if (savedDataLoggerMode.read() != 2) {
      
      Serial.print("Gsm power mode:\t ");
      if (savedGSMPowerMode.read() == 1) Serial.println("Low-power Mode (Always ON, but SLEEPS when inactive)");
      else if (savedGSMPowerMode.read() == 2) Serial.println("Power-saving mode");  // GSM module is ACTIVE when sending data, otherwise GSM module is turned OFF.
      else Serial.println("Always ON");
      
      Serial.print("Server number:\t ");
      if (strlen(flashServerNumber.dataServer) == 0) {
        Serial.print(defaultServerNumber);
        Serial.println(" [Default]");
      } else Serial.println(flashServerNumber.dataServer); 
  
      GSMSerial.write("AT+CSQ;+COPS?\r");
      delayMillis(1000);
      char gsmCSQResponse[200];
      GSMAnswer(gsmCSQResponse, sizeof(gsmCSQResponse));
      Serial.println(gsmCSQResponse);
    }
    Serial.println("");
}
// ito yung luma na sensor namess
// void updateSensorNames() {

//   uint8_t currentLoggerMode = savedDataLoggerMode.read();

//   char dataLoggerNameA[10];
//   char dataLoggerNameB[10];
//   char dataLoggerNameC[10];
//   char dataLoggerNameD[10];

//   if (currentLoggerMode == 1) {
//         Serial.print("Input name of Gateway sensor: ");
//         getSerialInput(dataLoggerNameA, sizeof(dataLoggerNameA), 60000);
//         Serial.println(dataLoggerNameA);
//         if (strlen(dataLoggerNameA) > 0) strncpy(flashLoggerName.sensorA, dataLoggerNameA, strlen(dataLoggerNameA));
//         Serial.print("Input name of Remote Sensor: ");
//         getSerialInput(dataLoggerNameB, sizeof(dataLoggerNameB), 60000);
//         Serial.println(dataLoggerNameB);
//         if (strlen(dataLoggerNameB) > 0) strncpy(flashLoggerName.sensorB, dataLoggerNameB, strlen(dataLoggerNameB));

//       } else if (currentLoggerMode == 3) {
//         Serial.print("Input name of Gateway: ");
//         getSerialInput(dataLoggerNameA, sizeof(dataLoggerNameA), 60000);
//         Serial.println(dataLoggerNameA);
//         if (strlen(dataLoggerNameA) > 0) strncpy(flashLoggerName.sensorA, dataLoggerNameA, strlen(dataLoggerNameA));
//         Serial.print("Input name of Remote Sensor: ");
//         getSerialInput(dataLoggerNameB, sizeof(dataLoggerNameB), 60000);
//         Serial.println(dataLoggerNameB);
//         if (strlen(dataLoggerNameB) > 0) strncpy(flashLoggerName.sensorB, dataLoggerNameB, strlen(dataLoggerNameB));
//       } else if (currentLoggerMode == 4) {
//         Serial.print("Input name of Gateway: ");
//         getSerialInput(dataLoggerNameA, sizeof(dataLoggerNameA), 60000);
//         Serial.println(dataLoggerNameA);
//         if (strlen(dataLoggerNameA) > 0) strncpy(flashLoggerName.sensorA, dataLoggerNameA, strlen(dataLoggerNameA));
//         Serial.print("Input name of Remote Sensor A: ");
//         getSerialInput(dataLoggerNameB, sizeof(dataLoggerNameB), 60000);
//         Serial.println(dataLoggerNameB);
//         if (strlen(dataLoggerNameB) > 0) strncpy(flashLoggerName.sensorB, dataLoggerNameB, strlen(dataLoggerNameB));
//         Serial.print("Input name of Remote Sensor B: ");
//         getSerialInput(dataLoggerNameC, sizeof(dataLoggerNameC), 60000);
//         Serial.println(dataLoggerNameC);
//         if (strlen(dataLoggerNameC) > 0) strncpy(flashLoggerName.sensorC, dataLoggerNameC, strlen(dataLoggerNameC));
//       } else if (currentLoggerMode == 5) {
//         Serial.print("Input name of Gateway: ");
//         getSerialInput(dataLoggerNameA, sizeof(dataLoggerNameA), 60000);
//         Serial.println(dataLoggerNameA);
//         if (strlen(dataLoggerNameA) > 0) strncpy(flashLoggerName.sensorA, dataLoggerNameA, strlen(dataLoggerNameA));
//         Serial.print("Input name of Remote Sensor A: ");
//         getSerialInput(dataLoggerNameB, sizeof(dataLoggerNameB), 60000);
//         Serial.println(dataLoggerNameB);
//         if (strlen(dataLoggerNameB) > 0) strncpy(flashLoggerName.sensorB, dataLoggerNameB, strlen(dataLoggerNameB));
//         Serial.print("Input name of Remote Sensor B: ");
//         getSerialInput(dataLoggerNameC, sizeof(dataLoggerNameC), 60000);
//         Serial.println(dataLoggerNameC);
//         if (strlen(dataLoggerNameC) > 0) strncpy(flashLoggerName.sensorC, dataLoggerNameC, strlen(dataLoggerNameC));
//         Serial.print("Input name of Remote Sensor C: ");
//         getSerialInput(dataLoggerNameD, sizeof(dataLoggerNameD), 60000);
//         Serial.println(dataLoggerNameD);
//         if (strlen(dataLoggerNameD) > 0) strncpy(flashLoggerName.sensorD, dataLoggerNameD, strlen(dataLoggerNameD));
//       } else {  // 0, 2, 6, 7 routers and arQ mode
//         // Serial.print("Gateway sensor name: ");
//         // Serial.println(get_logger_A_from_flashMem());
//         Serial.print("Input Sensor name: ");
//         getSerialInput(dataLoggerNameA, sizeof(dataLoggerNameA), 60000);
//         Serial.println(dataLoggerNameA);
//         if (strlen(dataLoggerNameA) > 0) strncpy(flashLoggerName.sensorA, dataLoggerNameA, strlen(dataLoggerNameA));
//         else strncpy(flashLoggerName.sensorA, "TESTA", 5);
//       }
//       savedLoggerName.write(flashLoggerName);
// }

void scalableUpdateSensorNames() {

  uint8_t currentLoggerMode = savedDataLoggerMode.read();
  uint8_t expectedRouter = 0;
  char nameBuffer[20];

  // get logger mode check if gateway or standalone
  // standalone datalogger and gateway names always occupy index 0 of multidim array
  // get serial input for index 0
  // if standalone, return
  // if gateway. ask for router count input
  // loop through router allocated index (index 1 and above for router list) of array until router count input is reached

  if (currentLoggerMode == 3)  {               // gateways
    Serial.print("Input GATEWAY name: ");
    getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
    if (strlen(nameBuffer) == 0) sprintf(nameBuffer,"TESG");
    Serial.println(nameBuffer);
    strncpy(flashLoggerName.sensorNameList[0], nameBuffer, strlen(nameBuffer));
    for (byte rCount = 1; rCount <= savedRouterCount.read(); rCount++) {   // router name positining starts at index 1, 0 is self
      Serial.print("Input name of ROUTER ");
      Serial.print(rCount);
      Serial.print(": ");
      getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
      if (strlen(nameBuffer) == 0) sprintf(nameBuffer,"TEST%d",rCount);
      Serial.println(nameBuffer);
      strncpy(flashLoggerName.sensorNameList[rCount], nameBuffer, strlen(nameBuffer));
    }
  } else {
    Serial.print("Input DATALOGGER name: ");
    getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
    if (strlen(nameBuffer) == 0) sprintf(nameBuffer,"TESTA");
    Serial.println(nameBuffer);
    strncpy(flashLoggerName.sensorNameList[0], nameBuffer, strlen(nameBuffer));
  }   // other standalone routers
  
  savedLoggerName.write(flashLoggerName);
}

void getLoggerModeAndName() {
  uint8_t mode = savedDataLoggerMode.read();
  char printBuffer[50];

  if (mode == 3) {      //gateways
    Serial.print("GATEWAY MODE ");
    if (hasSubsurfaceSensorFlag.read() == 99) Serial.print("+ Subsurface Sensor ");
    if (hasSubsurfaceSensorFlag.read() == 99) Serial.print("+ UBLOX Rover: ");
    Serial.print("with ");
    Serial.print(savedRouterCount.read());
    Serial.println(" Router(s)");
    Serial.print("Gateway name ");
    Serial.println(flashLoggerName.sensorNameList[0]);
    for (byte rCount = 1; rCount <= savedRouterCount.read(); rCount++){
      sprintf(printBuffer, "Router %d: %s", rCount, flashLoggerName.sensorNameList[rCount]);
      Serial.println(printBuffer);
    }  
  } else {      // other standalone dataloggers
    if (mode == 0) {
      Serial.println("ARQ MODE");
    } else if (mode == 1) {
      Serial.println("ARQ MODE + UBLOX Rover:");
    } else if (mode == 2) {
      Serial.print("ROUTER MODE ");
      if (hasSubsurfaceSensorFlag.read() == 99) Serial.print("+ Subsurface Sensor ");
      if (hasSubsurfaceSensorFlag.read() == 99) Serial.print("+ UBLOX Rover: ");
      Serial.println("");
    }  else if (mode == 4) {
      Serial.println("Rain gauge only (GSM)");
    } else if (mode == 5) {         // arQ mode GNSS sensor
      Serial.println("Rain gauge only (Router)");  
    }
    Serial.print("Datalogger name: ");
    Serial.println(flashLoggerName.sensorNameList[0]);
  }
}

void updateBatteryType() {
  unsigned long updateStart = millis();
  int updateTimeout = 60000;
  int batteryTypeBuf = 0;
  uint8_t currentType = savedBatteryType.read();
  Serial.print("Input battery type:");
  while (millis() - updateStart < updateTimeout) {
    if (Serial.available() > 0) {
      batteryTypeBuf = Serial.parseInt();
      break;
    }
  }
  Serial.println(batteryTypeBuf);
  if (batteryTypeBuf > 1) {
    Serial.print("Invalid value [>1], current battery type unchanged");
  } else if (batteryTypeBuf == currentType){
    Serial.println("Battrey type unchanged");
  } else {
    savedBatteryType.write(batteryTypeBuf);
    delayMillis(500);
    Serial.println("Battrey type updated");
  }
}

float readBatteryVoltage(uint8_t ver) {
  
  float measuredVBat;

  if (ver == 1) {
    // 4.2 volts
    measuredVBat = analogRead(VBATPIN);  // Measure the battery voltage at pin A7
    measuredVBat *= 2;                   // we divided by 2, so multiply back
    measuredVBat *= 3.3;                 // Multiply by 3.3V, our reference voltage
    measuredVBat /= 1024;                // convert to voltage
    measuredVBat += 0.15;                // between 0.15 and 0.4 drop in schottky diode
  } else {
    /* Voltage Divider 1M and  100k */
    // 12 volts
    measuredVBat = analogRead(VBATEXT);
    measuredVBat *= 3.3;     // reference voltage
    measuredVBat /= 1024.0;  // adc max count
    measuredVBat *= 11.0;    // (100k+1M)/100k
  }
  return measuredVBat;
}

void setBatteryType() {
  int intervalBuffer = 0;
  unsigned long intervalWait = millis();
  Serial.print("Enter battery type: ");
  while (millis() - intervalWait < 60000) {
    if (Serial.available() > 0) {
    intervalBuffer = Serial.parseInt();
    if (intervalBuffer > 1) {
      Serial.println("Invalid value, battery type unchanged.");
      return;
    }
    savedAlarmInterval.write(intervalBuffer);
    Serial.print("Updated battery type: ");
    Serial.println(savedAlarmInterval.read());
    break;
    }
  }
}



/// Why? Why not?
/// Masyado lang mahaba kung buong "Dynaslope" name ang ilalagay.. pero pwede naman.. someday.
void introMSG() {
Serial.println("");  
Serial.println(F(" ██████████   █████ █████ ██████   █████   █████████  "));
delayMillis(350);
Serial.println(F("░░███░░░░███ ░░███ ░░███ ░░██████ ░░███   ███░░░░░███ "));
delayMillis(200);
Serial.println(F(" ░███   ░░███ ░░███ ███   ░███░███ ░███  ░███    ░███ "));
delayMillis(150);
Serial.println(F(" ░███    ░███  ░░█████    ░███░░███░███  ░███████████ "));
delayMillis(100);
Serial.println(F(" ░███    ░███   ░░███     ░███ ░░██████  ░███░░░░░███ "));
delayMillis(50);
Serial.println(F(" ░███    ███     ░███     ░███  ░░█████  ░███    ░███ "));
delayMillis(10);
Serial.println(F(" ██████████      █████    █████  ░░█████ █████   █████"));
Serial.println(F("░░░░░░░░░░      ░░░░░    ░░░░░    ░░░░░ ░░░░░   ░░░░░ "));
                                                      
                                                      
                                                      
}