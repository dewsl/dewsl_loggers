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

    } else if (inputIs(serialLineInput, "CC")) {
      printExtraCommands();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "D")) {
      Serial.print("Saved data logger mode: ");
      Serial.println(savedDataLoggerMode.read());
      printLoggerModes();
      if (changeParameter()) {
        updateLoggerMode();
      }
      if (loggerNameChange) {
        loggerNameChange = false;
        scalableUpdateSensorNames();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "E")) {
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
      Serial.print("Saved SLEEP/WAKE interval: ");
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
      Serial.println("[0] 12V Lead Acid battery");
      Serial.println("[1] 4.2V Li-Ion battery");
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
      Serial.println("\"EXIT\" to close");
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

    } else if (inputIs(serialLineInput, "R")) {
      Serial.println("UPDATE RESET ALARM***");
      Serial.print("Current Reset Alarm Time (Military time): ");
      if (savedLoggerResetAlarm.read() == 0 || savedLoggerResetAlarm.read() > 2400) Serial.println("0000");
      else Serial.println(savedLoggerResetAlarm.read());
      if (changeParameter()) {
      setResetAlarmTime();
      }
      Serial.println(F("------------------------------------------------------"));


    } else if (inputIs(serialLineInput, "X" ) || inputIs(serialLineInput, "EXIT" ) ) {
      Serial.println("Quitting debug mode...");
      debugProcess = false;
      debugMode = false;
      if (loggerWithGSM(savedDataLoggerMode.read())) deleteMessageInbox();
      else digitalWrite(GSMPWR, LOW); //should turn off GSM module
      // Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "LORA_SEND_TEST")) {
      char payloadContainer[200];
      Serial.print(F("Simple broadcast thru LoRa"));
      Serial.print("Input data to send: ");
      getSerialInput(payloadContainer, sizeof(payloadContainer), 60000);
      Serial.println(payloadContainer);
      sendThruLoRa(payloadContainer);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "LORA_SEND_TEST_2")) {
      char payloadContainer[200];
      Serial.print(F("Simulates sending from ROUTER to GATEWAY [with acknowledgement]"));
      Serial.print(F("THIS datalogger must be listed as a ROUTER on the intended RECEIVER"));
      Serial.print(F("Input data to send: "));
      getSerialInput(payloadContainer, sizeof(payloadContainer), 60000);
      Serial.println(payloadContainer);
      sendThruLoRaWithAck(payloadContainer, 2000, 0);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "LORA_WAIT_TEST")) {
      Serial.println(F("Listens for any broadcasted LoRa data"));
      Serial.print(F("Timeout: "));
      Serial.print(MAX_GATWAY_WAIT);
      Serial.println(F("ms"));
      Serial.println("Waiting for LoRa transmission: ");
      char payloadContainer[RH_RF95_MAX_MESSAGE_LEN+1];
      receiveLoRaData(payloadContainer, sizeof(payloadContainer), MAX_GATWAY_WAIT);
      Serial.println(payloadContainer);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));
    
    } else if (inputIs(serialLineInput, "LORA_WAIT_TEST_2")) {
      uint8_t receiveType = 99;
      Serial.println(F("Listens for any broadcasted LoRa data from the listed router(s)"));
      Serial.print(F("Timeout: "));
      Serial.print(MAX_GATWAY_WAIT);
      Serial.println(F("ms"));
      // Serial.println("Waiting for LoRa transmission from listed router(s): ");
      if (savedRouterCount.read() == 0) Serial.println(F("No listed router(s), this might not work..."));
      waitForLoRaRouterData(MAX_GATWAY_WAIT,2,1);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "OTA_TEST")) {
      char payloadContainer[200];
      char tsBuf[50];
      Serial.print("Input FULL OTA command string: ");
      getSerialInput(payloadContainer, sizeof(payloadContainer), 60000);
      Serial.println(payloadContainer);
      getNetworkFormatTimeStamp(tsBuf, sizeof(tsBuf));
      findOTACommand(payloadContainer, "NANEEE", tsBuf);
      // sendThruLoRaWithAck(payloadContainer, 2000, 0);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "VOLT_STRING")) {
      char voltContainter[100];
      generateVoltString(voltContainter);
      Serial.println(voltContainter);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

      } else if (inputIs(serialLineInput, "INFO_STRING")) {
      Serial.println("Generating info strings");
      char infoContainter[100];
      generateInfoMessage(infoContainter);
      infoContainter[strlen(infoContainter)+1]=0x00;
      Serial.println(infoContainter);
      generateVoltString(infoContainter);
      Serial.println(infoContainter);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "CHECK_OTA")) {
      checkForOTAMessages();
        // do nothing
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));
    
    } else if (inputIs(serialLineInput, "BUILD_PARAM_SMS")) {
      char smsContainer[1000];
      buidParamSMS(smsContainer);
      Serial.println(smsContainer);
      if (sendThruGSM(smsContainer,flashServerNumber.dataServer)) debugPrintln("Message sent");
      else debugPrintln("Message NOT sent?");
      // debugPrintln("Send Fisnifh");0
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
      // Serial.println(F("------------------------------------------------------"));
      Serial.println(F("Exiting DEBUG MENU"));
      Serial.println(F("------------------------------------------------------"));
      // setNextAlarm(savedAlarmInterval.read());
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
  Serial.println("β");
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
  Serial.println(F("[R] Update SELF RESET alarm time."));
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
  Serial.println("[1] arQ mode + UBLOX Rover");                     // arQ like + Ublox router
  Serial.println("[2] Router mode");                                // anything that send data through LoRa
  Serial.println("[3] Gateway mode");                               // anything that recevies LoRa data
  Serial.println("[4] Rain gauge sensor only - GSM");               // same as gateway mode with no routers, sensor, or ublox module
  Serial.println("[5] Rain gauge sensor only - Router");            // same with [2] but no sensors or ublox
}

void printExtraCommands() {
  Serial.println(F("CHECK_OTA"));
  Serial.println(F("OTA_TEST"));
  Serial.println(F("LORA_SEND_TEST"));
  Serial.println(F("LORA_SEND_TEST_2"));
  Serial.println(F("LORA_WAIT_TEST"));
  Serial.println(F("LORA_WAIT_TEST_2"));
  Serial.println(F("VOLT_STRING"));
  Serial.println(F("INFO_STRING"));
  Serial.println(F("BUILD_PARAM_SMS"));
  Serial.println(F(" "));
  Serial.println(F("------------------------------------------------------"));
}

void updateLoggerMode() {
  unsigned long updateStart = millis();
  int updateTimeout = 60000;
  int loggerModeBuffer = 0;
  char addOnBuffer[10];
  uint8_t initialLoggerMode = savedDataLoggerMode.read();
  uint8_t initialRouterCount = savedRouterCount.read();
  
  // printLoggerModes();
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
    if (savedRouterCount.read() != 0) savedRouterCount.write(0);                                // resets router count
    Serial.print("   Router with Subsurface Sensor? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer,"Y")) || (inputIs(addOnBuffer,"y"))) hasSubsurfaceSensorFlag.write(99);
    else hasSubsurfaceSensorFlag.write(0);
    Serial.print("   Router with UBLOX Rover? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer,"Y")) || (inputIs(addOnBuffer,"y"))) hasUbloxRouterFlag.write(99);
    else hasUbloxRouterFlag.write(0);

  } else if (loggerModeBuffer == 1) {     // arQ mode + UBLOX rover
    if (savedRouterCount.read() != 0) savedRouterCount.write(0);                                // resets router count
    hasUbloxRouterFlag.write(99);
  }
  else {   // other non-gateway and non router datalogger
    if (savedRouterCount.read() != 0) savedRouterCount.write(0);                                // resets router count
  }
  Serial.println("Datalogger mode updated");
  Serial.println("");

  savedDataLoggerMode.write(loggerModeBuffer);

  // prompts a change of names if datalogger modes are changed
  if ((initialLoggerMode != 3 && savedDataLoggerMode.read() == 3) ||     // if initally non-gateway to gateway type
      (initialLoggerMode == 3 && savedDataLoggerMode.read() != 3) ||     // if initially gateway type to non-gateway type
      (initialRouterCount != savedRouterCount.read())) {                  // if router count was changed
      for (byte rCount = 0; rCount < initialRouterCount; rCount++) flashLoggerName.sensorNameList[rCount][0] = 0x00;  // obscure previous name list 
      loggerNameChange = true;  // starts name change function after function end
    }
}

bool inputIs(const char *inputFromSerial, const char* expectedInput) {
  bool correctInput = false;
  if ((strstr(inputFromSerial,expectedInput)) && (strlen(expectedInput) == strlen (inputFromSerial))) {
    correctInput = true;
  }
  return correctInput;
}

bool inputHas(const char *inputToCheck, const char* expectedInputSegment) {
  bool correctInput = false;
  if (strstr(inputToCheck,expectedInputSegment)) correctInput = true;
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
  //  update global variables [from flash] that will be used
  flashLoggerName = savedLoggerName.read();
  flashServerNumber = savedServerNumber.read();
  flashCommands = savedCommands.read();
  DateTime now = rtc.now();

  Serial.println(F("------------      STORED  PARAMETERS      ------------"));
  Serial.println(F("------------------------------------------------------"));
  Serial.println("");
  Serial.print(">>>>> ");
  getLoggerModeAndName();
  Serial.println("");
  Serial.print("Date & Time:\t ");
  printDateTime();
  getTimeStamp(_timestamp, sizeof(_timestamp));                                             //  Shows an easily readable datetime format 
  Serial.print("Next alarm\t hh:");
  Serial.println(nextAlarm((int)(now.minute()),savedAlarmInterval.read()));                   //  Shows next computed alarm based on the alamr interval and current time
  // Serial.print("Timestamp:\t ");                                                
  // Serial.println(_timestamp);                                                               //  Shows the timestamp format appended to data
  Serial.print("Wake interval:\t "); 
  int alarmInterval = savedAlarmInterval.read();                                            //  Shows periodic alarm interval
  if (alarmInterval == 0)       Serial.println("30 minutes (hh:00 & hh:30)");               //
  else if (alarmInterval == 1)  Serial.println("15 minutes (hh:00, hh:15, hh:30, hh:45)");
  else if (alarmInterval == 2)  Serial.println("10 minutes (hh:00, hh:10, hh:20, ... )");
  else if (alarmInterval == 3)  Serial.println("5 minutes (hh:00, hh:05, hh:10, ... )");
  else if (alarmInterval == 4)  Serial.println("3 minutes (hh:00, hh:03, hh:06, ... )");
  else if (alarmInterval == 5)  Serial.println("30 minutes (hh:15 & hh:45)");
  else                          Serial.println("Default 30 minutes (hh:00 & hh:30)");
  
  Serial.print("Sensor command:\t ");
  if (strlen(flashCommands.sensorCommand) == 0) Serial.println("[NOT SET] Default - ARQCM6T");
  else Serial.println(flashCommands.sensorCommand);
  
  Serial.print("Rain collector:\t ");
  if (savedRainCollectorType.read() == 0)       Serial.println("Pronamic (0.5mm/tip)");
  else if (savedRainCollectorType.read() == 1)  Serial.println("DAVIS (0.2mm/tip)");
  
  Serial.print("Rain data type:\t ");
  if (savedRainSendType.read()==0)  Serial.println("Sends converted \"mm\" equivalent");
  else                              Serial.println("Sends RAW TIP COUNT");

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
    } else {
      char serverNameBuf[20];
      sprintf(serverNameBuf, flashServerNumber.dataServer);
      Serial.print(serverNameBuf);
      checkSender(serverNameBuf);
      if (strlen(serverNameBuf) != strlen(flashServerNumber.dataServer)){
        Serial.print(" [");
        Serial.print(serverNameBuf);
        Serial.println("]");
      }
    } 

    GSMSerial.write("AT+CSQ;+COPS?\r");
    delayMillis(1000);
    char gsmCSQResponse[200];
    GSMAnswer(gsmCSQResponse, sizeof(gsmCSQResponse));
    Serial.println(gsmCSQResponse);
  }
  Serial.println("");
}

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
    Serial.print("Gateway name ");
    Serial.println(flashLoggerName.sensorNameList[0]);
    for (byte rCount = 1; rCount <= savedRouterCount.read(); rCount++){
      sprintf(printBuffer, "Router %d: %s", rCount, flashLoggerName.sensorNameList[rCount]);
      Serial.println(printBuffer);
    } 
    Serial.print("GATEWAY MODE ");
    if (hasSubsurfaceSensorFlag.read() == 99) Serial.print("with Subsurface Sensor ");
    if (hasUbloxRouterFlag.read() == 99) Serial.print("+ UBLOX Rover: ");
    Serial.print("and ");
    Serial.print(savedRouterCount.read());
    Serial.println(" Router(s)"); 
  } else {      // other standalone dataloggers
    Serial.print("Datalogger name: ");
    Serial.println(flashLoggerName.sensorNameList[0]);
    if (mode == 0) {
      Serial.println("ARQ MODE");
    } else if (mode == 1) {
      Serial.println("ARQ MODE + UBLOX Rover:");
    } else if (mode == 2) {
      Serial.print("ROUTER MODE ");
      if (hasSubsurfaceSensorFlag.read() == 99) Serial.print("with Subsurface Sensor ");
      if (hasUbloxRouterFlag.read() == 99) Serial.print("+ UBLOX Rover: ");
      Serial.println("");
    }  else if (mode == 4) {
      Serial.println("Rain gauge only (GSM)");
    } else if (mode == 5) {         // arQ mode GNSS sensor
      Serial.println("Rain gauge only (Router)");  
    }
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
    savedBatteryType.write(intervalBuffer);
    Serial.print("Updated battery type: ");
    Serial.println(savedBatteryType.read());
    break;
    }
  }
}


/// Consolidates all saved parameters to a char array for sending
/// @param paramSMSContainer - container of consilidated parameters
void buidParamSMS(char* paramSMSContainer) {
  // char paramSMSBuffer[500];
  // char smsBulderBuffer[500];
  flashLoggerName = savedLoggerName.read();
  flashServerNumber = savedServerNumber.read();
  flashCommands = savedCommands.read();
  char  routerNames[100],
        hasSubSensor[5],
        hasUblox[5],
        arQCommand[20],
        alarmIntervalDef[30],
        rainCollector[30],
        rainData[20],
        batteryType[10],
        powerMode[20],
        serverNumber[30];

  strcpy(routerNames,""); // preload
  if (savedRouterCount.read() > 0) {
    for (int r=0; r < savedRouterCount.read(); r++) {
      strcat(routerNames,flashLoggerName.sensorNameList[r+1]);
      strcat(routerNames,"  ");
    }
  } else strcpy(routerNames,"N/A"); 
  debugPrintln("Fetching timestamp..");
  getTimeStamp(_timestamp, sizeof(_timestamp));
  debugPrintln("Fetching sensors flag..");
  if (hasSubsurfaceSensorFlag.read() == 99) strcpy(hasSubSensor, "YES");
  else strcpy(hasSubSensor, "NO");
  if (hasUbloxRouterFlag.read() == 99) strcpy(hasUblox, "YES");
  else strcpy(hasUblox, "NO");
  debugPrintln("Fetching sensor command..");
  if (strlen(flashCommands.sensorCommand) == 0) strcpy(arQCommand, "default-ARQCMD6T");
  else strcpy(arQCommand, flashCommands.sensorCommand);
  debugPrintln("Fetching alarm interval..");
  int alarmInterval = savedAlarmInterval.read();
  if (alarmInterval == 0)       strcpy(alarmIntervalDef,"every 30 minutes");
  else if (alarmInterval == 1)  strcpy(alarmIntervalDef,"every 15 minutes");
  else if (alarmInterval == 2)  strcpy(alarmIntervalDef,"every 10 minutes");
  else if (alarmInterval == 3)  strcpy(alarmIntervalDef,"every 5 minutes");
  else if (alarmInterval == 4)  strcpy(alarmIntervalDef,"every 3 minutes");
  else if (alarmInterval == 5)  strcpy(alarmIntervalDef,"every 30 minutes (15&45)");
  else                          strcpy(alarmIntervalDef,"Default 30 minutes");
  debugPrintln("Fetching rain-gauge-related things...");
  if (savedRainCollectorType.read() == 0)       strcpy(rainCollector,"Pronamic (0.5mm/tip)");
  else if (savedRainCollectorType.read() == 1)  strcpy(rainCollector,"DAVIS (0.2mm/tip)");
  if (savedRainSendType.read() == 0)  strcpy(rainData,"Converted \"mm\"");
  else                                strcpy(rainData,"RAW TIP COUNT");
  if (savedBatteryType.read() == 1) strcpy(batteryType,"Li-ion");
  else                              strcpy(batteryType,"Lead acid");
  debugPrintln("Computing input voltage..");
  float inputVoltage = readBatteryVoltage(savedBatteryType.read());
  debugPrintln("Fetching GSM power mode..");
  if (savedGSMPowerMode.read() == 1)      strcpy(powerMode,"Low-power Mode");
  else if (savedGSMPowerMode.read() == 2) strcpy(powerMode,"Power-saving Mode");  // GSM module is ACTIVE when sending data, otherwise GSM module is turned OFF.
  else                                    strcpy(powerMode,"Always ON");
  debugPrintln("Fetching server number..");
  if (strlen(flashServerNumber.dataServer) == 0) strcpy(serverNumber,"Default - 09175972526");
  else                                           strcpy(serverNumber,flashServerNumber.dataServer);


  debugPrintln("Building parameter SMS...");
  debugPrintln("");
  // this will be long..
  sprintf(paramSMSContainer,
          "Saved Timetstamp: %s\nDatalogger Name: %s\nDatalogger Mode: %d\nSubsurface Sensor: %s\nHas UBLOX module: %s\nRouter Name(s): %s\nSensor command: %s\nWake Interval: %s\nRain Collector: %s\nRain Data: %s\nBattery Type: %s\nInput Voltage: %0.2fV\nGSM Power Mode: %s\nServer Number: %s",
          _timestamp,flashLoggerName.sensorNameList[0],savedDataLoggerMode.read(),hasSubSensor,hasUblox,routerNames,arQCommand,alarmIntervalDef,rainCollector,rainData,batteryType,inputVoltage,powerMode,serverNumber
          );
   
  
}



/// Why? Why not?
/// Masyado lang mahaba kung buong "Dynaslope" name ang ilalagay.. pero pwede naman...
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