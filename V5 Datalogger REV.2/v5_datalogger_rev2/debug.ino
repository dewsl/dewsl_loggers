void getSerialInput(char* inputBuffer, int bufferLength, int inputTimeout) {
  resetWatchdog();

  int bufferIndex = 0;
  unsigned long readStart = millis();
  char charbuf;

  for (int i = 0; i < bufferLength; i++) inputBuffer[i] = 0x00;
  while (millis() - readStart < inputTimeout) {
    resetWatchdog();
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
        // readStart = millis();
      }
    }
    resetWatchdog();
  }
  // strncpy(inputBuffer, readSerialBuffer, strlen(readSerialBuffer));
  resetWatchdog();
}

void debugFunction() {
  resetWatchdog();
  char serialLineInput[1000];
  bool debugProcess = true;
  unsigned long debugModeStart = millis();

  while (debugProcess) {
    resetWatchdog();
    for (int i = 0; i < sizeof(serialLineInput); i++) serialLineInput[i] = 0x00;

    getSerialInput(serialLineInput, sizeof(serialLineInput), 60000);

    // updates time-out with every input & resets watchdog.
    if (strlen(serialLineInput) != 0) {
      debugModeStart = millis();
      resetWatchdog();
    }

    //  processes serial input accordingly
    if (inputIs(serialLineInput, "A")) {
      resetWatchdog();
      Operation(flashServerNumber.dataServer);
      debugModeStart = millis();  //update start of timeout counter
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "B")) {
      resetWatchdog();
      uint8_t RainCollectorType = savedRainCollectorType.read();
      Serial.print("\nCollector type: ");
      char rainMsg[30];
      if (RainCollectorType == 0) sprintf(rainMsg, "Pronamic (0.5mm/tip)\nRain tips: %0.2f tips\nEquivalent: %0.2fmm", _rainTips, (_rainTips * 0.5));
      else if (RainCollectorType == 1) sprintf(rainMsg, "Davis (0.2mm/tip)\nRain tips: %0.2f tips\nEquivalent: %0.2fmm", _rainTips, (_rainTips * 0.2));
      Serial.println(rainMsg);
      delayMillis(20);
      resetRainTips();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "C")) {
      resetWatchdog();
      printMenu();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "CC")) {
      resetWatchdog();
      printExtraCommands();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "D")) {
      resetWatchdog();
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
      resetWatchdog();
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
      resetWatchdog();
      if (savedDataLoggerMode.read() != ROUTERMODE) {
        Serial.println("Checking network time..");
        updateTimeWithGPRS();
        debugModeStart = millis();
        Serial.println(F("------------------------------------------------------"));
      } else Serial.println("Can't check network time using ROUTER mode");

    } else if (inputIs(serialLineInput, "G")) {
      resetWatchdog();
      flashLoggerName = savedLoggerName.read();
      getLoggerModeAndName();
      if (changeParameter()) {
        scalableUpdateSensorNames();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "H")) {
      resetWatchdog();
      char serverBuffer[20];  // container for server number, might be modified
      sprintf(serverBuffer, flashServerNumber.dataServer);
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

    } else if (inputIs(serialLineInput, "I")) {
      resetWatchdog();
      GSMReset();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "J")) {
      resetWatchdog();
      Serial.print("Saved Rain Collector Type: ");
      Serial.println(savedRainCollectorType.read());
      Serial.println("[0] Pronamic Rain Collector (0.5mm/tip)");
      Serial.println("[1] DAVIS Rain Collector (0.2mm/tip)");
      Serial.println("[2] Generic Rain Collector (1.0/tip)");
      if (changeParameter()) {
        updateRainCollectorType();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

      } else if (inputIs(serialLineInput, "LBT_TOGGLE")) {
      resetWatchdog();
      Serial.print("Listen Mode ");
      if (listenMode.read()) {
        Serial.println("ENABLED");
        Serial.println("Disable Listen Mode?");
      } 
      else {
        Serial.println("DISABLED");
        Serial.println("Enable Listen Mode?");
        // Serial.println("Datalogger will reset afterwards");
      }
      
      if (changeParameter()) {
        if ( listenMode.read()) {
          listenMode.write(false);
          Serial.println("Listen Mode DISABLED");
          delayMillis(1000);
        }
        else {
          listenMode.write(true);
          Serial.println("Listen Mode ENABLED");
          updateListenKey();
          delayMillis(1000);
          // Serial.println("   Datalogger will reset..");
          // delayMillis(1000);
          // NVIC_SystemReset();                             
         } 
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "RAIN_DATA")) {
      resetWatchdog();
      Serial.print("Rain data type to send: ");
      Serial.println(savedRainCollectorType.read());
      Serial.println("[0] Sends converted \"mm\" equivalent");
      Serial.println("[1] Sends RAW TIP COUNT");
      if (changeParameter()) {
        updateRainDataType();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "K")) {
      resetWatchdog();
      Serial.print("Saved SLEEP/WAKE interval: ");
      Serial.println(savedAlarmInterval.read());
      printRTCIntervalEquivalent();
      if (changeParameter()) {
        setAlarmInterval();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "L")) {
      resetWatchdog();
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
      resetWatchdog();
      if (loggerWithGSM(savedDataLoggerMode.read())) {
        flashServerNumber = savedServerNumber.read();
        Serial.print("Send custom SMS to server: ");
        Serial.println(flashServerNumber.dataServer);
      } else {
        Serial.println("Broadcast custom message thru LoRa: ");
      }
      testSendToServer();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "N")) {
      resetWatchdog();
      Serial.print("Saved GSM power mode: ");
      Serial.println(savedGSMPowerMode.read());
      Serial.println("[0] Always ON");                                                 //  Typically ~9-12mA @ 13v when idle (arQ mode), with short duration spikes up to 25-40mA around every 10-30 seconds (approximate).
      Serial.println("[1] Low-power Mode (Always ON, but GSM SLEEPS when inactive)");  //  ~0-16mA @ 13v when idle (arQ mode). Typically 0mA, with very shord duration spikes of ~4-7mA around every 3-10 secs (approximate) and 11-17mA spikes around every 30-50sec (approximate).
      Serial.println("[2] Power Saving Mode");                                         //  ~0-8mA @ 13V when idle (arQ mode). Initially, around ~7-8mA yung idle, pero after ~1-2hrs ay nagiging 0mA (sabi ni sir Don baka daw µA na yung current draw) yung idle current draw. Either very efficient yung low power mode or napapagod lang yung power supply magdispaly ng mababang values...
      if (changeParameter()) {
        setGSMPowerMode();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "O")) {
      resetWatchdog();
      Serial.println("Input manual GSM Commands");
      Serial.println("\"EXIT\" to close");
      unsigned long manualStart = millis();
      char manualCommandInput[100];
      char GSMResponse[500];
      while (millis() - manualStart < DEBUGTIMEOUT) {
        resetWatchdog();
        getSerialInput(manualCommandInput, sizeof(manualCommandInput), DEBUGTIMEOUT);
        if (inputIs(manualCommandInput, "EXIT")) break;
        strcat(manualCommandInput, "\r");
        manualCommandInput[strlen(manualCommandInput)] = 0x00;
        GSMSerial.write(manualCommandInput);
        delayMillis(1000);
        GSMAnswer(GSMResponse, sizeof(GSMResponse));
        Serial.println(GSMResponse);
        resetWatchdog();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "P")) {
      resetWatchdog();
      flashCommands = savedCommands.read();
      Serial.print("Current sensor command: ");
      if (strlen(flashCommands.sensorCommand) == 0) Serial.println("[NOT SET]");
      else Serial.println(flashCommands.sensorCommand);
      if (changeParameter()) {
        updatSavedCommand();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "Q")) {
      resetWatchdog();
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
      resetWatchdog();
      Serial.println("UPDATE SELF-RESET ALARM***");
      Serial.print("Current Reset Alarm Time (Military time): ");
      if (savedLoggerResetAlarm.read() == 0 || savedLoggerResetAlarm.read() > 2400) Serial.println("0000");
      else Serial.println(savedLoggerResetAlarm.read());
      if (changeParameter()) {
        setResetAlarmTime();
      }
      Serial.println(F("------------------------------------------------------"));


    } else if (inputIs(serialLineInput, "X") || inputIs(serialLineInput, "EXIT")) {
      resetWatchdog();
      CheckingSavedParameters();
      Serial.println("Quitting debug mode...");
      resetRainTips();
      if (loggerWithGSM(savedDataLoggerMode.read())) deleteMessageInbox();
      else digitalWrite(GSMPWR, LOW);  //should turn off GSM module
      setNextAlarm(savedAlarmInterval.read());
      debugProcess = false;
      debugMode = false;
      // USBDevice.detach();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "SKIP_EXIT")) {
      resetWatchdog();
      // CheckingSavedParameters();
      Serial.println("Quitting debug mode...");
      resetRainTips(); //reset rain tips
      // DELETE all SIM message or turn off GSM module
      if (loggerWithGSM(savedDataLoggerMode.read())) deleteMessageInbox();
      else digitalWrite(GSMPWR, LOW);
      // if LBT is used, skip alarm setting
      if (listenMode.read()) Serial.println("LBT enabled");
      else {
        setNextAlarm(savedAlarmInterval.read());
        // rtc.clearINTStatus();
      }
      debugProcess = false;
      debugMode = false;
      // USBDevice.detach();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "SHORT_SLEEP_INTERVAL")) {
      resetWatchdog();
      Serial.print("Saved short sleep interval: ");
      Serial.println(savedShortSleepInterval.read());
      if (changeParameter()) {
        setShortSleepInterval();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "LBT_TEST")) {
      operationFlag = false;
      resetWatchdog();
      Serial.println("Listen Before Talk test");
      Serial.print("short sleep duration: ");
      while (1) {
        LEDOn();
        resetWatchdog();
        scanForCommand(1500, 3000);
        LEDOff();
        resetWatchdog();
        if (operationFlag) {
          Serial.print("wait before starting...");
          resetWatchdog();
          delayMillis(LBT_BROADCAST);  //wait time should be greater or equal to the broadcast duration
          resetWatchdog();}
        if (operationFlag) {
          operationFlag = false;
          Operation(flashServerNumber.dataServer);
        }
        Serial.println("sleep... ");
        resetWatchdog();
        // Watchdog.sleep(10000);
        delayMillis(10000);
        resetWatchdog();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "LORA_BROADCAST")) {
      broadcastLoRaKey(random(100, 500), LBT_BROADCAST);
      Operation(flashServerNumber.dataServer);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "LISTEN_TEST")) {
      resetWatchdog();
      char payloadContainer[200];
      Serial.print(F("Simple broadcast thru LoRa"));
      Serial.print("Input data to send: ");
      getSerialInput(payloadContainer, sizeof(payloadContainer), 60000);
      Serial.println(payloadContainer);
      while (1) {
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "LORA_SEND_TEST")) {
      resetWatchdog();
      char payloadContainer[200];
      Serial.print(F("Simple broadcast thru LoRa"));
      Serial.print("Input data to send: ");
      getSerialInput(payloadContainer, sizeof(payloadContainer), 60000);
      Serial.println(payloadContainer);
      sendThruLoRa(payloadContainer);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "LORA_SEND_TEST_2")) {
      resetWatchdog();
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
      resetWatchdog();
      Serial.println(F("Listens for any broadcasted LoRa data"));
      Serial.print(F("Timeout: "));
      Serial.print(MAX_GATEWAY_WAIT);
      Serial.println(F("ms"));
      Serial.println("Waiting for LoRa transmission: ");
      char payloadContainer[RH_RF95_MAX_MESSAGE_LEN + 1];
      receiveLoRaData(payloadContainer, sizeof(payloadContainer), MAX_GATEWAY_WAIT);
      Serial.println(payloadContainer);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "LORA_WAIT_TEST_2")) {
      resetWatchdog();
      uint8_t receiveType = 99;
      Serial.println(F("Listens for any broadcasted LoRa data from the listed router(s)"));
      Serial.print(F("Timeout: "));
      Serial.print(MAX_GATEWAY_WAIT);
      Serial.println(F("ms"));
      // Serial.println("Waiting for LoRa transmission from listed router(s): ");
      if (savedRouterCount.read() == 0) Serial.println(F("No listed router(s), this might not work..."));
      waitForLoRaRouterData(MAX_GATEWAY_WAIT, 2, 1);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "OTA_TEST")) {
      resetWatchdog();
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

    } else if (inputIs(serialLineInput, "OTA_COMMANDS")) {
      resetWatchdog();
      Serial.println("Over-the-air (OTA) Commands List:");
      Serial.println("");
      Serial.println("KEYWORD\t\t\t\t\tDESCRIPTION");
      Serial.println(F("-------------------------------------------------------------------------------------------------------------------------------"));
      Serial.println("REGISTER:SENSLOPE:\t\t\t*For checking code version..");
      Serial.println("CONFIG:SENSLOPE:\t\t\tSends all saved parameters");
      Serial.println("SENSORPOLL:SENSLOPE:\t\t\tSimulates the periodic operation function");
      Serial.println("READRAINTIPS:SENSLOPE:\t\t\tReads rain collector type and stored 'tip' value");
      Serial.println("SETRAINCOLLECTOR:SENSLOPE:\t\tUpdates the current/saved rain collector  type usng the additional parameter");
      Serial.println("SETSENDINGTIME:SENSLOPE:\t\tUpdates the current/saved send interval type usng the additional parameter");
      Serial.println("MODECHANGE:SENSLOPE:\t\t\tChanges the datalogger mode using additional parameter. !!USE WITH CAUTION!!");
      Serial.println("SETDATETIME:SENSLOPE:\t\t\tReplaces current/saved timestamp with the additional parameter. Time format is YYYY,MM,DD,HH,MM,SS,dd");
      Serial.println("FETCHGPRSTIME:SENSLOPE:\t\t\tAttempts to update datalogger timestamp using GPRS");
      Serial.println("SMSTIMEUPDATE:SENSLOPE:\t\t\tUpdates timestamp using SMS 'send' timestamp");
      Serial.println("CHECKTIMESTAMP:SENSLOPE:\t\tChecks the current/saved timestamp of the datalogger");
      Serial.println("DATALOGGERNAME:SENSLOPE:\t\tReplaces current datalogger name with additional parameter");
      Serial.println("SERVERNUMBER:SENSLOPE:\t\t\tReplaces the current server number with the additional parameter");
      Serial.println("?SERVERNUMBER:SENSLOPE:\t\t\tChecks current/saved server number of the device");
      Serial.println("RESET:SENSLOPE:\t\t\t\t\tResets the microcontroller; similar to pressing the reset button");
      Serial.println("RESETGSM:SENSLOPE:\t\t\tResets the GSM module only");
      Serial.println("SETBATTERYTYPE:SENSLOPE:\t\tReplaces current battery type with additional parameter");
      Serial.println("SETGSMPOWERMODE:SENSLOPE:\t\t[experimental]Replaces current [saved] GSM power mode with additional parameter");
      Serial.println("CMD?:SENSLOPE:\t\t\t\tCheck current [saved] sensor command");
      Serial.println("CMD:SENSLOPE:\t\t\t\tReplaces current [saved] sensor comand with additional parameter [ARQCMD6T or ARQCMD6S]");
      Serial.println("");
      Serial.println(">>>>>> COMMANDS BELOW WORKS FOR GATEWAYS ONLY");
      Serial.println("ROUTER:SENSLOPE:RESETDATALOGGER\t\tGateway will attempt to reset the routers after next data collection");
      Serial.println("ROUTER:SENSLOPE:UPDATETIMESTAMP\t\tGateway will attempt to sync gateway and router timestamps");

      debugModeStart = millis();
      Serial.println(F("-------------------------------------------------------------------------------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "DUE_CONFIG")) {
      resetWatchdog();
      char sdConfigLineBuffer[300];
      const char* sd_cmd = "ARQCMD6C/121212121212\n";
      Serial.println("Checking DUE config...");
      digitalWrite(DUETRIG, HIGH);
      delayMillis(1000);
      DUESerial.begin(DUEBAUD);
      delayMillis(500);
      unsigned long sdConfigStart = millis();
      int sdWaitTime = 10000;
      DUESerial.write(sd_cmd);
      bool readConfig = true;
      while (millis() - sdConfigStart < sdWaitTime && readConfig) {
        resetWatchdog();
        for (int i = 0; i < sizeof(sdConfigLineBuffer); ++i) sdConfigLineBuffer[i] = 0x00;

        DUESerial.readBytesUntil('\n', sdConfigLineBuffer, sizeof(sdConfigLineBuffer));

        if (strlen(sdConfigLineBuffer) > 0) {
          Serial.println(sdConfigLineBuffer);
          if (inputHas(sdConfigLineBuffer, "END OF CONFIG"))
            sdConfigStart = millis();
        }
        resetWatchdog();
      }
      DUESerial.end();
      digitalWrite(DUETRIG, LOW);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "UPDATE_DUE_CONFIG")) {
      resetWatchdog();
      char sdConfigLineBuffer[300];
      Serial.println("Updating custom due config using saved datalogger name");
      Serial.println("Note: there is a slight change this will not work if the serial connection is unstable or serial junk characters.");
      sprintf(sdConfigLineBuffer, "ARQCMD6D/%s", flashLoggerName.sensorNameList[0]);
      digitalWrite(DUETRIG, HIGH);
      delayMillis(1000);
      DUESerial.begin(DUEBAUD);
      delayMillis(2000);
      unsigned long sdConfigStart = millis();
      int sdWaitTime = 10000;
      DUESerial.write(sdConfigLineBuffer);
      // DUESerial.write("\n");
      bool readConfig = true;
      while (millis() - sdConfigStart < sdWaitTime && readConfig) {
        resetWatchdog();
        for (int i = 0; i < sizeof(sdConfigLineBuffer); ++i) sdConfigLineBuffer[i] = 0x00;
        DUESerial.readBytesUntil('\n', sdConfigLineBuffer, sizeof(sdConfigLineBuffer));

        if (strlen(sdConfigLineBuffer) > 0) {
          Serial.println(sdConfigLineBuffer);
          if (inputHas(sdConfigLineBuffer, "END OF UPDATE\n"))
            sdConfigStart = millis();
        }
        resetWatchdog();
      }
      DUESerial.end();
      digitalWrite(DUETRIG, LOW);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "DUE_SERIAL")) {
      resetWatchdog();
      Serial.println("Opens");
      Serial.println("\"EXIT_DUE\" to close");
      unsigned long manualStart = millis();
      char dueCommandInput[100];
      char DUEResponse[500];
      while (millis() - manualStart < DEBUGTIMEOUT) {
        resetWatchdog();
        getSerialInput(dueCommandInput, sizeof(dueCommandInput), DEBUGTIMEOUT);
        if (inputIs(dueCommandInput, "EXIT_DUE")) break;
        strcat(dueCommandInput, "\n");
        dueCommandInput[strlen(dueCommandInput)] = 0x00;
        DUESerial.write(dueCommandInput);
        // delayMillis(1000);
        while (DUESerial.available() > 0) {
          DUESerial.readBytesUntil('\n', DUEResponse, sizeof(DUEResponse));
          Serial.println(DUEResponse);
          if (inputIs(DUEResponse, "OK")) break;
        }
        resetWatchdog();
      }
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "VOLT_STRING")) {
      resetWatchdog();
      char voltContainter[100];
      generateVoltString(voltContainter);
      Serial.println(voltContainter);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "INFO_STRING")) {
      resetWatchdog();
      Serial.println("Generating info strings");
      char infoStrContainer[100];
      generateInfoMessage(infoStrContainer);
      infoStrContainer[strlen(infoStrContainer)] = 0x00;
      Serial.println(infoStrContainer);
      generateVoltString(infoStrContainer);
      Serial.println(infoStrContainer);
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "CHECK_OTA")) {
      resetWatchdog();
      checkForOTAMessages();
      // do nothing
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "BUILD_PARAM_SMS")) {
      resetWatchdog();
      char paramContainer[1000];
      buidParamSMS(paramContainer);
      Serial.println(paramContainer);
      if (loggerWithGSM(savedDataLoggerMode.read())) {
        if (sendThruGSM(paramContainer, flashServerNumber.dataServer)) debugPrintln(">> Parameter message sent to server");
        else debugPrintln(">> Parameter message NOT sent");
      }
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "BEACON_MODE")) {
      resetWatchdog();
      char beaconMsg[100];
      uint32_t pingCount = 0;

      while (1) {

        delayMillis(30000);
        sprintf(beaconMsg, "");
        pingCount++;
      }
      Serial.println(F("------------------------------------------------------"));

    } else if (inputIs(serialLineInput, "?")) {
      resetWatchdog();
      savedParameters();
      debugModeStart = millis();
      Serial.println(F("------------------------------------------------------"));
    }
    if ((millis() - debugModeStart) > DEBUGTIMEOUT) {
      resetWatchdog();
      debugModeStart = millis();
      debugProcess = false;
      Serial.println(F("Debug Menu Timeout!"));
      Serial.println(F("------------------------------------------------------"));
      break;
    }
  }

  if (!debugProcess) {
    resetWatchdog();
    debugMode = false;
    // Serial.println(F("------------------------------------------------------"));
    Serial.println(F("Exiting DEBUG MENU"));
    Serial.println(F("------------------------------------------------------"));
    if (listenMode.read() == false) setNextAlarm(savedAlarmInterval.read());
    delayMillis(75);
    // rtc.clearINTStatus();
    if (loggerWithGSM(savedDataLoggerMode.read()))
      if (GSMInit()) Serial.println("GSM READY");
    GSMPowerModeSet();
    return;
  }
}

void printMenu() {
  resetWatchdog();
  char timeString[100];
  Serial.println(F("------------------------------------------------------"));
  Serial.print(F("Firmware Version: "));
  Serial.print(FIRMWAREVERSION);
  Serial.println("β");
  // Serial.print("Current datetime: ");
  printDateTime();
  delayMillis(1000);
  Serial.println(F("------------------------------------------------------"));
  Serial.println(F("[?] Print stored config parameters."));
  Serial.println(F("[A] Test OPERATION function"));
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
  Serial.println(F("[Q] Text [Thread] Mode"));
  Serial.println(F("[R] Update SELF RESET alarm time."));
  Serial.println(F("[X] Exit Debug mode."));
  Serial.println(F(" "));
  Serial.println(F("------------------------------------------------------"));
  resetWatchdog();
}

void printRTCIntervalEquivalent() {
  resetWatchdog();
  // Serial.println("------------------------------------------------");
  Serial.println("[0] 30-minutes from 0th minute (0,30)");
  Serial.println("[1] 15-minutes from 0th minute (0,15,30,45)");
  Serial.println("[2] 10-minutes from 0th minute (0,10,20,30..)");
  Serial.println("[3] 5-minutes from 0th minute (0,5,10,15...)");
  Serial.println("[4] 3-minutes from 0th minute (0,3,6,9,12...)");
  Serial.println("[5] 30-minutes from 15th minute (15,45)");
  // Serial.println("------------------------------------------------");
  resetWatchdog();
}

void printLoggerModes() {
  resetWatchdog();
  Serial.println("[0] Stand-alone Datalogger (arQ mode)");  // arQ like function only: Includes rain gauge only (GSM), sa ngayon kasama yung mgay UBLOX dito, technicall sa gateway dapat sya..
  Serial.println("[1] Gateway mode");                       // anything that send data to other datalogger through LoRa; Includes rain gauge only (LoRa)
  Serial.println("[2] Router mode");                      // anything that wait for other datalogger LoRa data
  // Serial.println("[4] Rain gauge sensor only - GSM");      // same as gateway mode with no routers, sensor, or ublox module
  // Serial.println("[5] Rain gauge sensor only - Router");   // same with [2] but no sensors or ublox
  resetWatchdog();
}

void printExtraCommands() {
  resetWatchdog();

  Serial.println(F("RAIN_DATA           Change Rain data type to send"));  //
  Serial.println(F("CHECK_OTA           Check and execute [the first] OTA command in SIM inbox"));
  Serial.println(F("OTA_COMMANDS        Display accepted OTA commands"));
  Serial.println(F("OTA_TEST            Manual input of OTA command"));
  Serial.println(F("LORA_SEND_TEST      Simple [single instance] broadcast thru LoRa"));
  Serial.println(F("LORA_SEND_TEST_2    Simulates sending from ROUTER to GATEWAY [with acknowledgement]"));
  Serial.println(F("LORA_WAIT_TEST      Listens for any broadcasted LoRa data"));
  Serial.println(F("LORA_WAIT_TEST_2    Listens for any broadcasted LoRa data from the listed router(s)"));
  Serial.println(F("VOLT_STRING         Generate VOLT string"));
  Serial.println(F("INFO_STRING         Generate datalogger INFO & VOLT string"));
  Serial.println(F("BUILD_PARAM_SMS     Consolidate all [relevant] saved parameters into one string "));
  Serial.println(F("DUE_CONFIG          Checks saved config on custom due [requires 12V power]"));
  Serial.println(F("UPDATE_DUE_CONFIG   Updates name on custom due name using datalogger name"));
  Serial.println(F("LBT_TOGGLE          Toggles Listen Mode ON or OFF"));
  Serial.println(F(" "));
  Serial.println(F("------------------------------------------------------"));
  resetWatchdog();
}

void updateLoggerMode() {
  resetWatchdog();
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

  Serial.println(loggerModeBuffer);


  if (loggerModeBuffer == ARQMODE || loggerModeBuffer > 2) {      // arQ modes and false inputs
    
    // promt when defaulted to stand-alone mode for invalid inputs
    if (loggerModeBuffer > 2) Serial.println("Invalid datalogger mode value; Defaulted to Stand-alone datalogger");  

    if (savedRouterCount.read() != 0) savedRouterCount.write(0);  // resets router count to prevent values from being carried over in case of mode change
    Serial.print("   Datalogger with Subsurface Sensor? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) hasSubsurfaceSensorFlag.write(99);
    else hasSubsurfaceSensorFlag.write(0);
    Serial.print("   Datalogger with UBLOX module? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) hasUbloxRouterFlag.write(99);
    else hasUbloxRouterFlag.write(0);
    listenMode.write(false);        // prevents LBT mode from being carried over after a mode change
  }

  else if (loggerModeBuffer == GATEWAYMODE) {  // gateways and routers
    Serial.print("   Gateway with subsurface sensor? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) hasSubsurfaceSensorFlag.write(99);
    else hasSubsurfaceSensorFlag.write(0);
    Serial.print("   Gateway with UBLOX module? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) hasUbloxRouterFlag.write(99);
    else hasUbloxRouterFlag.write(0);
    Serial.print("   Gateway broadcast command* [for LBT router(s)] [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      listenMode.write(true);
      Serial.println("      *Listen-Before-Talk [LBT] should be enabled on ROUTER(S)");
    }  
    else listenMode.write(false);
    Serial.print("   Input router count: ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    uint8_t inputCount = atoi(addOnBuffer);
    if (inputCount > 7) Serial.println(" Hehe ");
    if (inputCount == 0) inputCount = 1;                                                                                               // should not accept ZERO as router count
    if (inputCount > (arrayCount(flashLoggerName.sensorNameList) - 1)) inputCount = (arrayCount(flashLoggerName.sensorNameList) - 1);  // limited to the number of rows to array holder max usable index
    savedRouterCount.write(inputCount);
    Serial.println(inputCount);

  } else if (loggerModeBuffer == ROUTERMODE) {
    if (savedRouterCount.read() != 0) savedRouterCount.write(0);  // resets router count to prevent values from being carried over in case of mode change
    Serial.print("   Router with Subsurface Sensor? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
        hasSubsurfaceSensorFlag.write(99);
    } else {  // Only ask if Ublox if Subsurface is not selected
        hasSubsurfaceSensorFlag.write(0);
        Serial.print("   Router with UBLOX module? [Y/N] ");
        getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
        Serial.println(addOnBuffer);
        if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) hasUbloxRouterFlag.write(99);
        else hasUbloxRouterFlag.write(0);
    }
    Serial.print("   Router mode LISTEN-BEFORE-TALK*? [Y/N] ");
    getSerialInput(addOnBuffer, sizeof(addOnBuffer), 60000);
    Serial.println(addOnBuffer);
    if ((inputIs(addOnBuffer, "Y")) || (inputIs(addOnBuffer, "y"))) {
      listenMode.write(true);
      Serial.println("      *BROADCAST COMMAND should be enabled on GATEWAY*");
    }
    else listenMode.write(false);     
  }

  Serial.println("Datalogger mode updated");
  Serial.println("");

  savedDataLoggerMode.write(loggerModeBuffer);

  // prompts a change of names if datalogger modes are changed
  if ((initialLoggerMode != GATEWAYMODE && savedDataLoggerMode.read() == GATEWAYMODE) ||                    // if initally non-gateway to gateway type
      (initialLoggerMode == GATEWAYMODE && savedDataLoggerMode.read() != GATEWAYMODE) ||                    // if initially gateway type to non-gateway type
      (initialRouterCount != savedRouterCount.read())) {                                                    // if router count was changed
    for (byte rPos = 0; rPos < initialRouterCount; rPos++) flashLoggerName.sensorNameList[rPos][0] = 0x00;  // obscure previous name list
    loggerNameChange = true;                                                                                // starts name change function after function end
  }
  if (!loggerWithGSM(savedDataLoggerMode.read())) GSMOff();
  resetWatchdog();
}

bool inputIs(const char* inputFromSerial, const char* expectedInput) {
  resetWatchdog();
  bool correctInput = false;
  if ((strstr(inputFromSerial, expectedInput)) && (strlen(expectedInput) == strlen(inputFromSerial))) {
    correctInput = true;
  }
  return correctInput;
  resetWatchdog();
}

bool inputHas(const char* inputToCheck, const char* expectedInputSegment) {
  resetWatchdog();
  bool correctInput = false;
  char * pointerResult = strstr(inputToCheck, expectedInputSegment);
  if (pointerResult != NULL) correctInput = true;
  else (correctInput = false);
  resetWatchdog();
  return correctInput;
}


void delayMillis(int delayDuration) {
  resetWatchdog();
  int maxDelayDuration = 15000;  //15sec
  bool maxDurationReached = false;
  unsigned long timeStart = millis();
  while (!maxDurationReached) {
    resetWatchdog();
    if ((millis() - timeStart) > maxDelayDuration) {
      Serial.println("Max delay duration: 15000");
      return;
    }
    if ((millis() - timeStart) > delayDuration) {
      return;
    }

    //
  }
  resetWatchdog();
}

bool changeParameter() {
  resetWatchdog();
  int changeParamTimeout = 20000;  //20 sec to wait for parameter change confirmation
  unsigned long waitStart = millis();
  int bufIndex = 0;
  char paramBuf;
  char changeBuffer[10];

  Serial.println(" ");
  Serial.println("Enter C to change:");

  getSerialInput(changeBuffer, sizeof(changeBuffer), changeParamTimeout);

  if (changeBuffer[0] == 'C' && strlen(changeBuffer) == 1) {
    Serial.print("\r");
    return true;
  } else {
    Serial.println(" ");
    Serial.print("\r");
    Serial.println("Change cancelled.");
    return false;
  }
  resetWatchdog();
}

void savedParameters() {
  resetWatchdog();
  //  update global variables [from flash] that will be used
  flashLoggerName = savedLoggerName.read();
  flashServerNumber = savedServerNumber.read();
  flashCommands = savedCommands.read();

  // compute next alarm
  DateTime now = rtc.now();
  uint8_t nextAlarmMinuteBuffer = nextAlarm((int)(now.minute()), savedAlarmInterval.read());
  uint8_t nextAlarmHourBuffer = now.hour();
  uint8_t timeOfDayIndex2 = 0;
  const char* timeOfDayEq[3] = { "AM", "PM" };                                                                //  default identifier is AM
  if (now.minute() > nextAlarmMinuteBuffer) nextAlarmHourBuffer++;                                            //  indicates next hour for alarm
  if (nextAlarmHourBuffer > 11 && nextAlarmHourBuffer < 24) timeOfDayIndex2 = 1;                              //  change daytime identifier to PM if true
  if (nextAlarmHourBuffer > 12 && nextAlarmHourBuffer <= 24) nextAlarmHourBuffer = nextAlarmHourBuffer - 12;  //  subtract 12 from 24hr format to get 12hr format


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
  printDateTime();  //  Shows an easily readable datetime format

  // Serial.println(now.year());
  if (now.year()%1000 < 30) { // assumes that you're not a time traveler. This should work until 2030
    Serial.print("Next alarm:\t ");
    char nextAlarmBuffer[20];
    if (nextAlarmMinuteBuffer != 0) sprintf(nextAlarmBuffer, "%d:%02d %s", nextAlarmHourBuffer, nextAlarmMinuteBuffer, timeOfDayEq[timeOfDayIndex2]);  //  Shows next computed alarm based on the alamr interval and current time
    else sprintf(nextAlarmBuffer, "%d:00 %s", nextAlarmHourBuffer, timeOfDayEq[timeOfDayIndex2]);                                                      // do ko sure kung paano gawing zero padded yung zero na hindi ginagawang char
    Serial.print(nextAlarmBuffer);
    if (listenMode.read() && (savedDataLoggerMode.read()==ROUTERMODE)) Serial.print(" [DISABLED]");
    Serial.println("");
  }
  

  Serial.print("Wake interval:\t ");
  int alarmInterval = savedAlarmInterval.read();                         //  Shows periodic alarm interval
  if (savedDataLoggerMode.read() == GATEWAYMODE || listenMode.read() == false) {  // should update this later; consider..
    if (alarmInterval == 0) Serial.println("30 minutes (hh:00 & hh:30)");  //
    else if (alarmInterval == 1) Serial.println("15 minutes (hh:00, hh:15, hh:30, hh:45)");
    else if (alarmInterval == 2) Serial.println("10 minutes (hh:00, hh:10, hh:20, ... )");
    else if (alarmInterval == 3) Serial.println("5 minutes (hh:00, hh:05, hh:10, ... )");
    else if (alarmInterval == 4) Serial.println("3 minutes (hh:00, hh:03, hh:06, ... )");
    else if (alarmInterval == 5) Serial.println("30 minutes (hh:15 & hh:45)");
    else Serial.println("Default 30 minutes (hh:00 & hh:30)");
  } else {
    if (savedDataLoggerMode.read() == ROUTERMODE) Serial.println("Wakes on gateway command");
  }
  if (hasSubsurfaceSensorFlag.read() == 99) {
    Serial.print("Sensor command:\t ");
    if (strlen(flashCommands.sensorCommand) == 0) Serial.println("[NOT SET] Default - ARQCM6T");
    else Serial.println(flashCommands.sensorCommand);
  }

  Serial.print("Rain collector:\t ");
  if (savedRainCollectorType.read() == 0) Serial.println("Pronamic (0.5mm/tip)");
  else if (savedRainCollectorType.read() == 1) Serial.println("DAVIS (0.2mm/tip)");

  Serial.print("Rain data type:\t ");
  if (savedRainSendType.read() == 0) Serial.println("Sends converted \"mm\" equivalent");
  else Serial.println("Sends RAW TIP COUNT");

  Serial.print("Battery type:\t ");
  if (savedBatteryType.read() == 1) Serial.println("Li-ion");
  else Serial.println("Lead acid");
  Serial.print("Input Voltage:\t ");
  Serial.print(readBatteryVoltage(savedBatteryType.read()));
  Serial.println("V");

  
  Serial.print("RTC temperature: ");
  Serial.print(readRTCTemp());
  Serial.println("°C");

  if (savedDataLoggerMode.read() != ROUTERMODE) {

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
      if (strlen(serverNameBuf) != strlen(flashServerNumber.dataServer)) {
        Serial.print(" [");
        Serial.print(serverNameBuf);
        Serial.println("]");
      }
    }

    char gsmCSQResponse[200];
    GSMSerial.write("AT+CSQ\r");
    delayMillis(1000);
    while (GSMSerial.available() > 0) {
      if (GSMGetResponse(gsmCSQResponse, sizeof(gsmCSQResponse), "+CSQ: ", 1000)) {
        resetWatchdog();
        int CSQval = parseCSQ(gsmCSQResponse);
        if (CSQval > 0) {
          // if (Serial) Serial.print("Checking GSM network signal..");
          debugPrint("CSQ: ");
          debugPrintln(CSQval);
        }
      }
    }
    for (int c = 0; c < sizeof(gsmCSQResponse); c++) gsmCSQResponse[c] = 0x00;
    GSMSerial.write("AT+COPS?\r");
    delayMillis(1000);
    while (GSMSerial.available() > 0) {
      if (GSMGetResponse(gsmCSQResponse, sizeof(gsmCSQResponse), "+COPS: 0,1,\"", 1000)) {
        uint8_t netIndex = 0;
        char* _netInfo = strtok(gsmCSQResponse, "\"");
        while (_netInfo != NULL) {
          if (netIndex == 1) {
            debugPrint("Network registered: ");
            debugPrintln(_netInfo);
          }
          netIndex++;
          _netInfo = strtok(NULL, "\"");
        }
      }
    }
  }
  debugPrintln("");
  resetWatchdog();
}

void scalableUpdateSensorNames() {
  resetWatchdog();
  uint8_t currentLoggerMode = savedDataLoggerMode.read();
  uint8_t expectedRouter = 0;
  char nameBuffer[20];

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
    sprintf(flashLoggerName.sensorNameList[0], "%s", nameBuffer);

    uint8_t routerStartIndex = 1;
    if (hasUbloxRouterFlag.read() == 99) {
      Serial.print("Input name for Ublox Module: ");
      getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
      if (strlen(nameBuffer) == 0) sprintf(nameBuffer, "TESUA");
      Serial.println(nameBuffer);
      sprintf(flashLoggerName.sensorNameList[1], "%s", nameBuffer);
      routerStartIndex = 2;
    }
    expectedRouter = savedRouterCount.read();

    for (byte listPos = routerStartIndex; listPos < routerStartIndex + expectedRouter; listPos++) {
      Serial.print("Input name of ROUTER ");
      Serial.print(listPos - routerStartIndex + 1);
      Serial.print(": ");
      getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
      if (strlen(nameBuffer) == 0) sprintf(nameBuffer, "TEST%d", listPos - routerStartIndex + 1);
      Serial.println(nameBuffer);
      sprintf(flashLoggerName.sensorNameList[listPos], "%s", nameBuffer);
    }
  }  else if (currentLoggerMode == ROUTERMODE) { // Router Mode
    if (hasUbloxRouterFlag.read() == 99) {
      Serial.print("Input name for Ublox Module: ");
      getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
      if (strlen(nameBuffer) == 0) sprintf(nameBuffer, "TESU");
      Serial.println(nameBuffer);
      sprintf(flashLoggerName.sensorNameList[0], "%s", nameBuffer);
    } else {
      hasUbloxRouterFlag.write(0);
      Serial.print("Input ROUTER name: ");
      getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
      if (strlen(nameBuffer) == 0) sprintf(nameBuffer, "TESR");
      Serial.println(nameBuffer);
      sprintf(flashLoggerName.sensorNameList[0], "%s", nameBuffer);
    }
  } else {  // Standalone (ARQ Mode)
    Serial.print("Input DATALOGGER name: ");
    getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
    if (strlen(nameBuffer) == 0) sprintf(nameBuffer, "TESTA");
    Serial.println(nameBuffer);
    sprintf(flashLoggerName.sensorNameList[0], "%s", nameBuffer);

    if (hasUbloxRouterFlag.read() == 99) {
      Serial.print("Input name for Ublox Module: ");
      getSerialInput(nameBuffer, sizeof(nameBuffer), 60000);
      if (strlen(nameBuffer) == 0) sprintf(nameBuffer, "TESUA");
      Serial.println(nameBuffer);
      sprintf(flashLoggerName.sensorNameList[1], "%s", nameBuffer);
    }
  }
  savedLoggerName.write(flashLoggerName);
  updateListenKey();
  resetWatchdog();
}
///
/// 
void getLoggerModeAndName() {
  resetWatchdog();
  uint8_t mode = savedDataLoggerMode.read();
  char printBuffer[50];

  if (mode == GATEWAYMODE) {  // gateways
    Serial.print("GATEWAY MODE ");
    if (hasSubsurfaceSensorFlag.read() == 99) Serial.print("with Subsurface Sensor ");
    if (hasUbloxRouterFlag.read() == 99) Serial.print("+ UBLOX Module ");
    if (hasSubsurfaceSensorFlag.read() != 99 && hasUbloxRouterFlag.read() != 99) Serial.print("(Rain gauge only) ");
    Serial.print("with ");
    Serial.print(savedRouterCount.read());
    Serial.println(" Router(s) ");
    if (listenMode.read()) debugPrintln(" [Broadcasts Router Commands]");

    Serial.print("Gateway name ");
    Serial.println(flashLoggerName.sensorNameList[0]);

    uint8_t nameIndex = 1;
    if (hasUbloxRouterFlag.read() == 99) {
      Serial.print("Ublox name: ");
      Serial.println(flashLoggerName.sensorNameList[nameIndex]);
      nameIndex++;
    }

    for (byte rCount = 1; rCount <= savedRouterCount.read(); rCount++) {
      sprintf(printBuffer, "Router %d: %s", rCount, flashLoggerName.sensorNameList[nameIndex]);
      Serial.println(printBuffer);
      nameIndex++;
    }
    if (listenMode.read()) debugPrintln("[Listen Mode ENABLED]");

  } else {  // Standalone Dataloggers or Router Mode
    Serial.print((mode == ARQMODE) ? "STAND-ALONE DATALOGGER " : "ROUTER MODE ");

    if (mode == ARQMODE) {
      if (hasSubsurfaceSensorFlag.read() == 99) Serial.print("with Subsurface Sensor ");
      if (hasUbloxRouterFlag.read() == 99) Serial.print("+ UBLOX Module ");

    } else if (mode == ROUTERMODE) {
      // if (hasSubsurfaceSensorFlag.read() == 99) Serial.print("with Subsurface Sensor ");
      if (hasSubsurfaceSensorFlag.read() == 99) {
        Serial.print("with Subsurface Sensor ");
        hasUbloxRouterFlag.write(0);  // Disable Ublox if Subsurface Sensor exists
      }

      // if (hasUbloxRouterFlag.read() == 99) Serial.print(" UBLOX Module ");
      if (hasUbloxRouterFlag.read() == 99) {
        Serial.print(" UBLOX Module ");
        hasSubsurfaceSensorFlag.write(0);  // Disable Subsurface Sensor if Ublox exists
      }
    }
    
    if (hasSubsurfaceSensorFlag.read() != 99 && hasUbloxRouterFlag.read() != 99) Serial.print("(Rain gauge only) ");   
    if (mode == ROUTERMODE && listenMode.read()) debugPrintln("[Listen Mode ENABLED]");
    Serial.println("\nDatalogger name: " + String(flashLoggerName.sensorNameList[0]));
    
    if (hasUbloxRouterFlag.read() == 99) {
        Serial.print("Ublox name: ");
        Serial.println(flashLoggerName.sensorNameList[mode == ROUTERMODE ? 0 : 1]);
    }
  }
  resetWatchdog();
}

void updateBatteryType() {
  resetWatchdog();
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
  } else if (batteryTypeBuf == currentType) {
    Serial.println("Battrey type unchanged");
  } else {
    savedBatteryType.write(batteryTypeBuf);
    delayMillis(500);
    Serial.println("Battrey type updated");
  }
  resetWatchdog();
}

float readBatteryVoltage(uint8_t ver) {
  resetWatchdog();
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
  resetWatchdog();
  return measuredVBat;
}

void setBatteryType() {
  resetWatchdog();
  int intervalBuffer = 0;
  unsigned long intervalWait = millis();
  Serial.print("Enter battery type: ");
  while (millis() - intervalWait < 60000) {
    if (Serial.available() > 0) {
      intervalBuffer = Serial.parseInt();
      if (intervalBuffer > 1) {
        Serial.println("Invalid value, battery type unchanged.");
        resetWatchdog();
        return;
      }
      savedBatteryType.write(intervalBuffer);
      Serial.print("Updated battery type: ");
      Serial.println(savedBatteryType.read());
      break;
    }
  }
  resetWatchdog();
}


/// Consolidates all saved parameters to a char array for sending
/// @param paramSMSContainer - container of consilidated parameters
void buidParamSMS(char* paramSMSContainer) {
  resetWatchdog();
  // char paramSMSBuffer[500];
  // char smsBulderBuffer[500];
  flashLoggerName = savedLoggerName.read();
  flashServerNumber = savedServerNumber.read();
  flashCommands = savedCommands.read();
  char routerNames[100],
    hasSubSensor[5],
    hasUblox[5],
    arQCommand[20],
    alarmIntervalDef[30],
    rainCollector[30],
    rainData[20],
    batteryType[10],
    powerMode[20],
    serverNumber[30];

  strcpy(routerNames, "");  // preload
  if (savedRouterCount.read() > 0) {
    for (int r = 0; r < savedRouterCount.read(); r++) {
      strcat(routerNames, flashLoggerName.sensorNameList[r + 1]);
      strcat(routerNames, "  ");
    }
  } else strcpy(routerNames, "N/A");
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
  if (alarmInterval == 0) strcpy(alarmIntervalDef, "every 30 minutes");
  else if (alarmInterval == 1) strcpy(alarmIntervalDef, "every 15 minutes");
  else if (alarmInterval == 2) strcpy(alarmIntervalDef, "every 10 minutes");
  else if (alarmInterval == 3) strcpy(alarmIntervalDef, "every 5 minutes");
  else if (alarmInterval == 4) strcpy(alarmIntervalDef, "every 3 minutes");
  else if (alarmInterval == 5) strcpy(alarmIntervalDef, "every 30 minutes (15&45)");
  else strcpy(alarmIntervalDef, "Default 30 minutes");
  debugPrintln("Fetching rain-gauge-related things...");
  if (savedRainCollectorType.read() == 0) strcpy(rainCollector, "Pronamic (0.5mm/tip)");
  else if (savedRainCollectorType.read() == 1) strcpy(rainCollector, "DAVIS (0.2mm/tip)");
  if (savedRainSendType.read() == 0) strcpy(rainData, "Converted \"mm\"");
  else strcpy(rainData, "RAW TIP COUNT");
  if (savedBatteryType.read() == 1) strcpy(batteryType, "Li-ion");
  else strcpy(batteryType, "Lead acid");
  debugPrintln("Computing input voltage..");
  float inputVoltage = readBatteryVoltage(savedBatteryType.read());
  debugPrintln("Fetching GSM power mode..");
  if (savedGSMPowerMode.read() == 1) strcpy(powerMode, "Low-power Mode");
  else if (savedGSMPowerMode.read() == 2) strcpy(powerMode, "Power-saving Mode");  // GSM module is ACTIVE when sending data, otherwise GSM module is turned OFF.
  else strcpy(powerMode, "Always ON");
  debugPrintln("Fetching server number..");
  if (strlen(flashServerNumber.dataServer) == 0) strcpy(serverNumber, "Default - 09175972526");
  else strcpy(serverNumber, flashServerNumber.dataServer);

  debugPrintln("Building parameter SMS...");
  debugPrintln("");
  // this will be long..
  sprintf(paramSMSContainer,
          "Saved Timetstamp: %s\nDatalogger Name: %s\nDatalogger Mode: %d\nSubsurface Sensor: %s\nHas UBLOX module: %s\nRouter Name(s): %s\nSensor command: %s\nWake Interval: %s\nRain Collector: %s\nRain Data: %s\nBattery Type: %s\nInput Voltage: %0.2fV\nGSM Power Mode: %s\nServer Number: %s",
          _timestamp, flashLoggerName.sensorNameList[0], savedDataLoggerMode.read(), hasSubSensor, hasUblox, routerNames, arQCommand, alarmIntervalDef, rainCollector, rainData, batteryType, inputVoltage, powerMode, serverNumber);
  resetWatchdog();
}



/// Why? Why not?
/// Masyado lang mahaba kung buong "Dynaslope" name ang ilalagay.. pero pwede naman...
void introMSG() {
  resetWatchdog();
  // Serial.println("");
  Serial.println(" ██████████   █████ █████ ██████   █████   █████████  ");
  delayMillis(350);
  Serial.println("░░███░░░░███ ░░███ ░░███ ░░██████ ░░███   ███░░░░░███ ");
  delayMillis(200);
  Serial.println(" ░███   ░░███ ░░███ ███   ░███░███ ░███  ░███    ░███ ");
  delayMillis(150);
  Serial.println(" ░███    ░███  ░░█████    ░███░░███░███  ░███████████ ");
  delayMillis(100);
  Serial.println(" ░███    ░███   ░░███     ░███ ░░██████  ░███░░░░░███ ");
  delayMillis(50);
  Serial.println(" ░███    ███     ░███     ░███  ░░█████  ░███    ░███ ");
  delayMillis(10);
  Serial.println(" ██████████      █████    █████  ░░█████ █████   █████");
  Serial.println("░░░░░░░░░░      ░░░░░    ░░░░░    ░░░░░ ░░░░░   ░░░░░ ");
  delayMillis(1000);
  delayMillis(750);
  delayMillis(500);
  delayMillis(100);
  resetWatchdog();
}


/// Config validation
void CheckingSavedParameters() {
  char Params[1000];
  char staff[20];
  const char rcvr[] = "09476873967";
  bool debugProcess = true;
  char serialInput[1000];
  unsigned long debugModeStart = millis();

  flashLoggerName = savedLoggerName.read();
  flashServerNumber = savedServerNumber.read();
  flashCommands = savedCommands.read();

  while (debugProcess) {
    while (true) {
      Serial.print("Saved data logger mode: ");
      char savedDataLoggerModeChar = savedDataLoggerMode.read();
      int savedDataLoggerMode = savedDataLoggerModeChar;
      Serial.print(savedDataLoggerMode);
      Serial.println(" ");


      // Integrated printDataLoggerDescription function
      switch (savedDataLoggerMode) {
        case ARQMODE:
          Serial.println("Stand-alone Datalogger (arQ mode)");
          break;
        // case 1:
        //   Serial.println("arQ mode + UBLOX Rover");
        //   break;
        case GATEWAYMODE:
          Serial.println("Gateway mode");
          break;
        case ROUTERMODE:
          Serial.println("Router mode");
          break;
        // case 4:
        //   Serial.println("Rain gauge sensor only - GSM");
        //   break;
        // case 5:
        //   Serial.println("Rain gauge sensor only - Router");
        //   break;
        default:
          Serial.println("N/A");
          break;
      }

      delay(1000);

      Serial.print("Is data logger mode correct? [Y/N]: ");
      getSerialInput(serialInput, sizeof(serialInput), 60000);
      delay(1000);
      Serial.println(serialInput);

      if (inputIs(serialInput, "N") || inputIs(serialInput, "n")) {
        delay(500);
        printLoggerModes();
        delay(500);
        updateLoggerMode();
        Serial.println("------------------------------------------------------");

      } else if (inputIs(serialInput, "Y") || inputIs(serialInput, "y")) {
        Serial.println("------------------------------------------------------");
        delay(500);
        break;
      }
    }

    // Execute only if data logger mode is 0
    int currentDataLoggerMode = savedDataLoggerMode.read();
    if (currentDataLoggerMode == 0 || currentDataLoggerMode == 1) {
      while (true) {
        Serial.print("Current server number: ");
        Serial.println(flashServerNumber.dataServer);
        delay(500);
        Serial.print("Is server number correct? [Y/N]: ");
        getSerialInput(serialInput, sizeof(serialInput), 60000);
        delay(500);
        Serial.println(serialInput);

        if (inputIs(serialInput, "N") || inputIs(serialInput, "n")) {
          delay(500);
          updateServerNumber();
          Serial.println("------------------------------------------------------");
        } else if (inputIs(serialInput, "Y") || inputIs(serialInput, "y")) {
          Serial.println("------------------------------------------------------");
          delay(500);
          break;
        }
      }
    }

    while (true) {
      Serial.print("Logger name: ");
      getLoggerModeAndName();
      //Serial.println(flashLoggerName.sensorNameList[0]);
      //Serial.println(flashLoggerName);
      delay(500);
      Serial.print("Is logger name correct? [Y/N]: ");
      getSerialInput(serialInput, sizeof(serialInput), 60000);
      delay(500);
      Serial.println(serialInput);

      if (inputIs(serialInput, "N") || inputIs(serialInput, "n")) {
        delay(500);
        scalableUpdateSensorNames();
        Serial.println("------------------------------------------------------");
      } else if (inputIs(serialInput, "Y") || inputIs(serialInput, "y")) {
        Serial.println("------------------------------------------------------");
        delay(500);
        break;
      }
    }

    while (true) {
      Serial.print("Current timestamp: ");
      getTimeStamp(_timestamp, sizeof(_timestamp));
      Serial.println(_timestamp);
      Serial.print("Is timestamp correct? [Y/N]: ");
      getSerialInput(serialInput, sizeof(serialInput), 60000);
      delay(500);
      Serial.println(serialInput);

      if (inputIs(serialInput, "N") || inputIs(serialInput, "n")) {
        delay(500);
        setupTime();
        Serial.println("------------------------------------------------------");
      } else if (inputIs(serialInput, "Y") || inputIs(serialInput, "y")) {
        Serial.println("------------------------------------------------------");
        delay(500);
        break;
      }
    }

    while (true) {
      Serial.print("Saved alarm interval: ");
      char savedAlarmIntervalChar = savedAlarmInterval.read();
      int savedAlarmInterval = savedAlarmIntervalChar;
      switch (savedAlarmInterval) {
        case 0:
          Serial.println("30-minutes from 0th minute (0,30)");
          break;
        case 1:
          Serial.println("15-minutes from 0th minute (0,15,30,45)");
          break;
        case 2:
          Serial.println("10-minutes from 0th minute (0,10,20,30..)");
          break;
        case 3:
          Serial.println("5-minutes from 0th minute (0,5,10,15...)");
          break;
        case 4:
          Serial.println("3-minutes from 0th minute (0,3,6,9,12...)");
          break;
        case 5:
          Serial.println("30-minutes from 15th minute (15,45)");
          break;
        default:
          Serial.println("Invalid mode");
          break;
      }
      delay(500);
      Serial.print("Is wake interval correct? [Y/N]: ");
      getSerialInput(serialInput, sizeof(serialInput), 60000);
      delay(500);
      Serial.println(serialInput);

      if (inputIs(serialInput, "N") || inputIs(serialInput, "n")) {
        printRTCIntervalEquivalent();
        setAlarmInterval();
        delay(500);
        Serial.println("------------------------------------------------------");
      } else if (inputIs(serialInput, "Y") || inputIs(serialInput, "y")) {
        Serial.println("------------------------------------------------------");
        delay(500);
        break;
      }
    }
    while (true) {
      Serial.print("Current sensor command: ");
      Serial.println(flashCommands.sensorCommand);
      Serial.println("ARQCMD6T: Tilt sensor only");
      Serial.println("ARQCMD6TS: Tilt sensor with SOMS");
      delay(500);
      Serial.print("Is sensor command correct? [Y/N]: ");
      getSerialInput(serialInput, sizeof(serialInput), 60000);
      delay(500);
      Serial.println(serialInput);

      if (inputIs(serialInput, "N") || inputIs(serialInput, "n")) {
        delay(500);
        updatSavedCommand();
        Serial.println("------------------------------------------------------");
      } else if (inputIs(serialInput, "Y") || inputIs(serialInput, "y")) {
        Serial.println("------------------------------------------------------");
        delay(500);
        break;
      }
    }

    while (true) {
      Serial.print("Saved Rain Collector Type: ");
      char savedRainCollectorTypeChar = savedRainCollectorType.read();
      int savedRainCollectorType = savedRainCollectorTypeChar;
      switch (savedRainCollectorType) {
        case 0:
          Serial.println("Pronamic Rain Collector (0.5mm/tip)");
          break;
        case 1:
          Serial.println("DAVIS Rain Collector (0.2mm/tip)");
          break;
        case 2:
          Serial.println("Generic Rain Collector (1.0/tip)");
          break;
        default:
          Serial.println("Invalid mode");
          break;
      }
      delay(500);
      Serial.print("Is Rain collector correct? [Y/N]: ");
      getSerialInput(serialInput, sizeof(serialInput), 60000);
      delay(500);
      Serial.println(serialInput);

      if (inputIs(serialInput, "N") || inputIs(serialInput, "n")) {
        Serial.println("[0] Pronamic Rain Collector (0.5mm/tip)");
        Serial.println("[1] DAVIS Rain Collector (0.2mm/tip)");
        Serial.println("[2] Generic Rain Collector (1.0/tip)");
        delay(500);
        updateRainCollectorType();
        Serial.println("------------------------------------------------------");
      } else if (inputIs(serialInput, "Y") || inputIs(serialInput, "y")) {
        Serial.println("------------------------------------------------------");
        delay(500);
        break;
      }
    }

    while (true) {
      Serial.print("Saved battery type: ");
      char savedBatteryTypeChar = savedBatteryType.read();
      int savedBatteryType = savedBatteryTypeChar;
      switch (savedBatteryType) {
        case 0:
          Serial.println("12V Lead Acid battery");
          break;
        case 1:
          Serial.println("4.2V Li-Ion battery");
          break;
        default:
          Serial.println("Invalid mode");
          break;
      }
      delay(500);
      Serial.print("Is battery type correct? [Y/N]: ");
      getSerialInput(serialInput, sizeof(serialInput), 60000);
      delay(500);
      Serial.println(serialInput);

      if (inputIs(serialInput, "N") || inputIs(serialInput, "n")) {
        Serial.println("[0] 12V Lead Acid battery");
        Serial.println("[1] 4.2V Li-Ion battery");
        delay(500);
        updateBatteryType();
        Serial.println("------------------------------------------------------");
      } else if (inputIs(serialInput, "Y") || inputIs(serialInput, "y")) {
        Serial.println("------------------------------------------------------");
        delay(500);
        break;
      }
    }

    if (currentDataLoggerMode == 0 || currentDataLoggerMode == 1 || currentDataLoggerMode == 3 || currentDataLoggerMode == 4 || currentDataLoggerMode == 5) {
      Serial.print("Enter maintenance staff's initials: ");
      getSerialInput(staff, sizeof(staff), 60000);
      buidParamSMS(Params);
      strcat(Params, " - ");
      strcat(Params, staff);
      sendThruGSM(Params, rcvr);
      delay(2000);

      Serial.println("Exit debug mode");
      debugProcess = false;
      debugMode = false;
      deleteMessageInbox();
      Serial.println(F("----------------------------------------------"));
    } else {
      Serial.println("Exit debug mode");
      debugProcess = false;
      debugMode = false;
      deleteMessageInbox();
      Serial.println(F("----------------------------------------------"));
    }
  }
}
