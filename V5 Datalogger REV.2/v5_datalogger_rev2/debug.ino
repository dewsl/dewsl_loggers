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
      Serial.println(F("----------------------------------------------"));

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
      Serial.println(F("----------------------------------------------"));
      
    } else if (inputIs(serialLineInput, "C")) {
      printMenu();
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));

    } else if (inputIs(serialLineInput, "D")) {
      Serial.print("Saved data logger mode: ");
      Serial.println(savedDataLoggerMode.read());
      printLoggerModes();
      if (changeParameter()) {
        updateLoggerMode();
      }
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));

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
      Serial.println(F("----------------------------------------------"));

    } else if (inputIs(serialLineInput, "F")) {
      Serial.println("Run debug menu F");
      updateTimeWithGPRS();
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));
      
    } else if (inputIs(serialLineInput, "G")) {

      flashLoggerName = savedLoggerName.read();
      getLoggerModeAndName();
      if (changeParameter()) {
        updateSensorNames();
      }
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));

    } else if (inputIs(serialLineInput, "H")) {
      char currentServer[20];
      strcpy(currentServer, flashServerNumber.dataServer);
      if (strlen(currentServer) != 11 || strlen(currentServer) != 12) {
        Serial.println("## Default server in use ## ");  
        strcpy(currentServer, defaultServerNumber);
        currentServer[strlen(currentServer)+1];
      } 
      Serial.print("Saved Server Number: ");
      Serial.println(currentServer);
      if (changeParameter()) {
        updateServerNumber();
      }
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));

    } else if (inputIs(serialLineInput, "I")) {
      Serial.println("Resetting the GSM module...");
      GSMReset();
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));

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
      Serial.println(F("----------------------------------------------"));

    } else if (inputIs(serialLineInput, "K")) {
      Serial.println("Run debug menu K");
      Serial.print("Saved alarm interval: ");
      Serial.println(savedAlarmInterval.read());
      printRTCIntervalEquivalent();
      if (changeParameter()) {
        setAlarmInterval();
      }
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));

    } else if (inputIs(serialLineInput, "L")) {
      Serial.println("Run debug menu L");

    } else if (inputIs(serialLineInput, "M")) {
      flashServerNumber = savedServerNumber.read();
      Serial.print("Send custom SMS to server: ");
      Serial.println(flashServerNumber.dataServer);
      testSendToServer();
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));

    } else if (inputIs(serialLineInput, "N")) {
      Serial.println("Run debug menu N");
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
      Serial.println(F("----------------------------------------------"));

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
      Serial.println(F("----------------------------------------------"));


    } else if (inputIs(serialLineInput, "Q")) {
      Serial.println("Run debug menu Q");
      GSMSerial.flush();
      if (changeParameter()) {
        Serial.println("Text mode in use:");
        Serial.println("To send a message, use the format:\t09123456789>>Message_to_send");
        Serial.println("use 09XXXXXXXXX or +639XXXXXXXXX number format");
        Serial.println("IMPORTANT: Use \"EXIT\" to quit text mode.");
        textMode();
      } 
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));

    } else if (inputIs(serialLineInput, "X")) {
      Serial.println(savedParameters());
      Serial.println("Backup SD card data");
      Serial.println("Put stickers");
      Serial.println(F("----------------------------------------------"));
      Serial.println("Are all parameters saved correct and complete?");
      if (inputIs(serialInput, "Yes")){
      Serial.println("Exit debug mode");
      debugProcess = false;
      debugMode = false;
      deleteMessageInbox();
      Serial.println(F("----------------------------------------------"));
      }
      else {
        Serial.println("Parameters to change:")
        // lagay dito yung mga babaguhin
      }



    } else if (inputIs(serialLineInput, "LORA_SEND_TEST")) {
      char payloadContainer[200];
      Serial.print("Input LoRa test data: ");
      getSerialInput(payloadContainer, sizeof(payloadContainer), 60000);
      Serial.println(payloadContainer);
      sendThruLoRa(payloadContainer);
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));

    } else if (inputIs(serialLineInput, "LORA_SEND_2_TEST")) {
      char payloadContainer[200];
      Serial.print("Input LoRa test data: ");
      getSerialInput(payloadContainer, sizeof(payloadContainer), 60000);
      Serial.println(payloadContainer);
      sendThruLoRaWithAck(payloadContainer, 2000, 0);
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));

    } else if (inputIs(serialLineInput, "LORA_WAIT_TEST")) {
      Serial.println("Waiting for LoRa transmission: ");
      char payloadContainer[RH_RF95_MAX_MESSAGE_LEN+1];
      receiveLoRaData(payloadContainer, sizeof(payloadContainer), 180000);
      Serial.println(payloadContainer);
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));
    
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
      Serial.println(F("----------------------------------------------"));

    } else if (inputIs(serialLineInput, "VOLT_STRING")) {
      char voltContainter[100];
      generateVoltString(voltContainter);
      Serial.println(voltContainter);
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));

      } else if (inputIs(serialLineInput, "INFO_STRING")) {
      char infoContainter[100];
      // generateVoltString(voltContainter);
      generateInfoMessage(infoContainter);
      infoContainter[strlen(infoContainter)+1]=0x00;
      Serial.println(infoContainter);
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));

    } else if (inputIs(serialLineInput, "CHECK_OTA")) {
      checkOTACommand();
      deleteMessageInbox();
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));

    } else if (inputIs(serialLineInput, "?")) {
      savedParameters();
      debugModeStart = millis();
      Serial.println(F("----------------------------------------------"));
    }
    if ((millis() - debugModeStart) > DEBUGTIMEOUT) {
      debugModeStart = millis();
      debugProcess = false;
      Serial.println(F("Debug Menu Timeout!"));
      Serial.println(F("----------------------------------------------"));
      break;
    }
  }
  
  if (!debugProcess) {
      debugMode = false;
      Serial.println(F("----------------------------------------------"));
      Serial.println(F("Exiting from DEBUG MENU"));
      Serial.println(F("----------------------------------------------"));
      setNextAlarm(savedAlarmInterval.read());
      delayMillis(75);
      rtc.clearINTStatus();
      GSMInit();
      return;
  }
}

void printMenu() {
  Serial.println(F("----------------------------------------------"));
  Serial.print(F("Firmware Version: "));
  Serial.print(FIRMWAREVERSION);
  Serial.println("α");
  getTimeStamp(_timestamp, sizeof(_timestamp));
  Serial.print(F("Real time clock: "));
  Serial.println(_timestamp);
  Serial.println(F("----------------------------------------------"));
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
  Serial.println(F("----------------------------------------------"));
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
  Serial.println("[0] arQ mode: Sending sensor data using GSM");           // arQ like function only
  Serial.println("[1] Gateway mode with Subsurface Sensor and 1 Router");  // arQ + LoRa rx
  Serial.println("[2] Router mode");                                       // TX LoRa
  Serial.println("[3] Gateway mode with 1 Router");
  Serial.println("[4] Gateway mode with 2 Routers");
  Serial.println("[5] Gateway mode with 3 Routers");
  Serial.println("[6] Rain gauge sensor only - GSM");
}

void updateLoggerMode() {
  unsigned long updateStart = millis();
  int updateTimeout = 60000;
  int loggerModeBuffer = 0;
  int currentMode = savedDataLoggerMode.read();
  Serial.print("Enter datalogger mode: ");
  while (millis() - updateStart < updateTimeout) {
    if (Serial.available() > 0) {
      loggerModeBuffer = Serial.parseInt();
      break;
    }
  }
  Serial.println(loggerModeBuffer);
  if (currentMode ==  loggerModeBuffer) {
    //no change
  } else if (loggerModeBuffer > 9) {        //restricts values above 8
    Serial.println("Unchanged: Invalid datalogger mode value");
  } else {
    savedDataLoggerMode.write(loggerModeBuffer);
    Serial.println("Datalogger Mode updated");
  }

}

void getSerialInput(char* inputBuffer, int bufferLength, int inputTimeout) {
  int bufferIndex = 0;
  unsigned long readStart = millis();
  char charbuf;

  for (int i = 0; i < bufferLength; i++) inputBuffer[i] = 0x00;

  while (millis() - readStart < inputTimeout) {
    if (Serial.available() > 0 ) {
      charbuf = Serial.read();
      // buf = Serial.read();
      if (charbuf =='\n') {
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
  // while (millis() - waitStart < changeParamTimeout) {
    
  //   // buf = Serial.read();
  //   if (Serial.available() > 0 ) {
  //     paramBuf = Serial.read();
  //     if ((paramBuf =='\n' || paramBuf == '\r') && paramBuf > 0) {
  //       changeBuffer[bufIndex] = 0x00;
  //       break;
  //     } else {
  //       changeBuffer[bufIndex] = paramBuf;
  //       bufIndex++;
  //       // waitStart = millis();
  //     }
  //   }
  // }
  // Serial.println(changeBuffer);
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

  Serial.println(F("------------  STORED  PARAMETERS  ------------"));
  Serial.println(F("----------------------------------------------"));
    Serial.print("");
    getTimeStamp(_timestamp, sizeof(_timestamp));
    Serial.print("Real time clock: ");
    Serial.println(_timestamp);
    Serial.print("Send interval:\t ");
    int alarmInterval = savedAlarmInterval.read();
    if (alarmInterval == 0)       Serial.println("30 minutes (hh:00 & hh:30)");
    else if (alarmInterval == 1)  Serial.println("15 minutes (hh:00, hh:15, hh:30, hh:45)");
    else if (alarmInterval == 2)  Serial.println("10 minutes (hh:00, hh:10, hh:20, ... )");
    else if (alarmInterval == 3)  Serial.println("5 minutes (hh:00, hh:05, hh:10, ... )");
    else if (alarmInterval == 4)  Serial.println("3 minutes (hh:00, hh:03, hh:06, ... )");
    else if (alarmInterval == 5)  Serial.println("30 minutes (hh:15 & hh:45)");
    else                          Serial.println("Default 30 minutes (hh:00 & hh:30)");
    
    Serial.print("Logger mode:\t [");
    Serial.print(savedDataLoggerMode.read());
    Serial.print("] ");
    getLoggerModeAndName();
    Serial.print("Sensor command:\t ");
    if (strlen(flashCommands.sensorCommand) == 0) Serial.println("[NOT SET] Defaults to \"T\"");
    else Serial.println(flashCommands.sensorCommand);
    
    Serial.print("Rain collector:\t ");
    if (savedRainCollectorType.read() == 0)       Serial.println("Pronamic (0.5mm/tip)");
    else if (savedRainCollectorType.read() == 1)  Serial.println("DAVIS (0.2mm/tip)");
    
    Serial.print("Rain data type:\t ");
    if (savedRainSendType.read()==0)  Serial.println("Converted \"mm\" equivalent");
    else                              Serial.println("Spoon TIP COUNT");

    Serial.print("Input Voltage:\t ");
    Serial.print(readBatteryVoltage(savedBatteryType.read()));
    Serial.println("v");
    Serial.print("Battery type:\t ");
    if (savedBatteryType.read() == 1) Serial.println("Li-ion");
    else  Serial.println("Lead acid");

    Serial.print("RTC temperature: ");
    Serial.println(readRTCTemp());
    Serial.print("");
    
    if (savedDataLoggerMode.read() != 2) {
      
      Serial.print("Server number:\t ");
      if (strlen(flashServerNumber.dataServer) == 0) {
        Serial.print("[Default] ");
        Serial.println(defaultServerNumber);
      } else Serial.println(flashServerNumber.dataServer); 
  

      Serial.print("Gsm power mode:\t ");
      if (savedGSMPowerMode.read() == 1) Serial.println("Low-power Mode (Always ON, but SLEEPS when inactive)");
      else if (savedGSMPowerMode.read() == 2) Serial.println("Power-saving mode");  // GSM module is ACTIVE when sending data, otherwise GSM module is turned OFF.
      else Serial.println("Always ON");

      
      GSMSerial.write("AT+CSQ;+COPS?\r");
      delayMillis(1000);
      char gsmCSQResponse[200];
      GSMAnswer(gsmCSQResponse, sizeof(gsmCSQResponse));
      Serial.println(gsmCSQResponse);
    }
}

void getLoggerModeAndName() {
  uint8_t mode = savedDataLoggerMode.read();
  char dataloggerA[10] = "[NOT SET]";
  char dataloggerB[10] = "[NOT SET]";
  char dataloggerC[10] = "[NOT SET]";
  char dataloggerD[10] = "[NOT SET]";
  if (strlen(flashLoggerName.sensorA) != 0) strcpy(dataloggerA, flashLoggerName.sensorA);
  if (strlen(flashLoggerName.sensorB) != 0) strcpy(dataloggerB, flashLoggerName.sensorB);
  if (strlen(flashLoggerName.sensorC) != 0) strcpy(dataloggerC, flashLoggerName.sensorC);
  if (strlen(flashLoggerName.sensorD) != 0) strcpy(dataloggerD, flashLoggerName.sensorD);

  if (mode == 0) {
    Serial.println("arQ mode");
    Serial.print("Logger name:     ");
    Serial.println(dataloggerA);
  } else if (mode == 1) {
    Serial.println("Gateway mode Subsurface Sensor and 1 Router");
    Serial.print("Gateway sensor name: ");
    Serial.println(dataloggerA);
    Serial.print("Remote sensor name: ");
    Serial.println(dataloggerB);
  } else if (mode == 3) {
    Serial.println("Gateway mode with 1 Router");
    Serial.print("Gateway name:    ");
    Serial.println(dataloggerA);
    Serial.print("Remote Sensor A: ");
    Serial.println(dataloggerB);
  } else if (mode == 4) {
    Serial.println("Gateway mode with 2 Routers");
    Serial.print("Gateway name:    ");
    Serial.println(dataloggerA);
    Serial.print("Remote Sensor A: ");
    Serial.println(dataloggerB);
    Serial.print("Remote Sensor B: ");
    Serial.println(dataloggerC);
  } else if (mode == 5) {
    Serial.println("Gateway mode with 3 Routers");
    Serial.print("Gateway name:    ");
    Serial.println(dataloggerA);
    Serial.print("Remote Sensor A: ");
    Serial.println(dataloggerB);
    Serial.print("Remote Sensor B: ");
    Serial.println(dataloggerC);
    Serial.print("Remote Sensor C: ");
    Serial.println(dataloggerD);
  } else if (mode == 6) {
    Serial.println("Rain gauge only (GSM)");
    Serial.print("Logger name:     ");
    Serial.println(dataloggerA);
  } else if (mode == 7) {         // arQ mode GNSS sensor
    Serial.println("GNSS sensor only (GSM)");
    Serial.print("Logger name:     ");
    Serial.println(dataloggerA);
  } else if (mode == 8) {         // router mode [2]??
    Serial.println("GNSS sensor only (Tx)");
    Serial.print("Remote Sensor name:     ");
    Serial.println(dataloggerA);
  } else if (mode == 9) {      // Parehas lang ito function ng gateway with 1 router(pero GNSS)???
    Serial.println("Gateway Rain Gauge with GNSS Sensor");
    Serial.print("Gateway sensor name: ");
    Serial.println(flashLoggerName.sensorA);
    Serial.print("Sensor name (GNSS): ");
    Serial.println(flashLoggerName.sensorB);
  } else if (mode == 10) {
    Serial.println("Gateway with Subsurface sensor and GNSS Sensor"); // datalogger Mode 1??
    Serial.print("Gateway sensor name: ");
    Serial.println(flashLoggerName.sensorA);
    Serial.print("Sensor name (GNSS): ");
    Serial.println(flashLoggerName.sensorB);
  } else {  //mode 2
    Serial.println("Router mode");
    Serial.print("Logger name:     ");
    Serial.println(flashLoggerName.sensorA);
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

