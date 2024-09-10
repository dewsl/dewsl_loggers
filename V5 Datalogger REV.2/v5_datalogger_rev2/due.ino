void dueInit (uint8_t dueTrigPin) {
  pinMode(dueTrigPin, OUTPUT);
  digitalWrite(dueTrigPin, LOW);
}

void dueDataCollection(int samplingTimeout) {
  resetWatchdog();
  bool readDueData = true;
  // int lineBufferLength = 500;
  char dueLineBuffer[500];
  char commandContainer[100];
  char dueChar;
  int dataSegmentCount;
  unsigned long samplingStart;
  samplingStart = millis();

  flashCommands = savedCommands.read();

  DUESerial.begin(DUEBAUD);
  delayMillis(1000);
  Serial.println("Starting sensor data collection..");
  digitalWrite(DUETRIG, HIGH);
  delayMillis(1000);
  getTimeStamp(_timestamp, sizeof(_timestamp));
  if (strlen(flashCommands.sensorCommand) == 0) sprintf(commandContainer, "ARQCMD6T/%s", _timestamp); //defaults to tilt if sensor command is not set
  else sprintf(commandContainer, "%s/%s", flashCommands.sensorCommand,_timestamp);
  Serial.println(commandContainer);
  DUESerial.write(commandContainer);
  while (readDueData) {
    resetWatchdog();
    
    if (millis() - samplingStart > samplingTimeout) {
      Serial.printf("Sampling timed out!");
      readDueData = false;
      break;
    }

    for (int i=0; i < sizeof(dueLineBuffer); i++)  dueLineBuffer[i] = 0x00;

    DUESerial.readBytesUntil('\n', dueLineBuffer, sizeof(dueLineBuffer));
    
    for (int n = 0; n < sizeof(dueLineBuffer); n++) {
      if (dueLineBuffer[n] == '\n' || dueLineBuffer[n] == '\r') dueLineBuffer[n]=0x00; // Overwrite extra '\n' character from readBytesUntil
    }

    dueLineBuffer[strlen(dueLineBuffer)] = 0x00;
    
    // Keeps updating timeout start as long as there is input from dueSerial
    if (strlen(dueLineBuffer) > 0 ) {
      Serial.println(dueLineBuffer);
      samplingStart = millis();
    }

    if (strstr(dueLineBuffer,"STOPLORA")) {
      readDueData = false;
      break;  
    }
    
    if (strstr(dueLineBuffer, ">>"))  {
      dataSegmentCount++;
      addToSMSStack(dueLineBuffer);
      DUESerial.write("OK");
    }
    resetWatchdog();
  } 

  digitalWrite(DUETRIG, LOW);
  DUESerial.end();

  if (dataSegmentCount == 0) {
    char noDataBUffer[30];
    getTimeStamp(_timestamp, sizeof(_timestamp));
    sprintf(noDataBUffer, "%s*NODATAFROMSENSLOPE*%s",flashLoggerName.sensorNameList[0],_timestamp);
    addToSMSStack(noDataBUffer);
  }

  Serial.println("Data collection finished!");
  resetWatchdog();
}

void updatSavedCommand() {
  resetWatchdog();
  char sensorCommandBuffer[50];
  Serial.print("Input sensor command: ");
  getSerialInput(sensorCommandBuffer, sizeof(sensorCommandBuffer), 60000);
  if (strlen(sensorCommandBuffer) > 0) {
    strncpy(flashCommands.sensorCommand, sensorCommandBuffer, strlen(sensorCommandBuffer));
    Serial.println(flashCommands.sensorCommand);
    Serial.println("Sensor command updated");  
  } else {
    strncpy(flashCommands.sensorCommand, "ARQCMD6T", 8); // default if timed out or invalid input length
    Serial.println("Defaulted to \"ARQCMD6T\"");
  }
  savedCommands.write(flashCommands);
  resetWatchdog();
}