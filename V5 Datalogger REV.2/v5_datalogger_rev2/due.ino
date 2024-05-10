void dueInit (uint8_t dueTrigPin) {
  pinMode(dueTrigPin, OUTPUT);
  digitalWrite(dueTrigPin, LOW);
}

void dueDataCollection(int samplingTimeout) {
  bool readDueData = true;
  int lineBufferLength = 500;
  char lineBuffer[lineBufferLength];
  char commandContainer[100];
  int lineBufferIndex;
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
  commandContainer[strlen(commandContainer)]=0x00;
  Serial.println(commandContainer);
  DUESerial.write(commandContainer);
  while (readDueData) {
    
    lineBufferIndex = 0;
    
    if (millis() - samplingStart > samplingTimeout) {
      Serial.printf("Sampling timed out!");
      readDueData = false;
      break;
    }
    for (int i=0; i < lineBufferLength; i++) { 
      lineBuffer[i] = 0x00;
    }

    DUESerial.readBytesUntil('\n', lineBuffer, lineBufferLength);
    lineBuffer[strlen(lineBuffer)] = 0x00;
    
    // Keeps updating timeout start as long as there is input from dueSerial
    if (strlen(lineBuffer) > 0 ) {
      Serial.println(lineBuffer);
      samplingStart = millis();
    }

    if (strstr(lineBuffer,"STOPLORA")) {
      readDueData = false;
      break;  
    }
    
    if (strstr(lineBuffer, ">>"))  {
      addToSMSStack(lineBuffer);
      DUESerial.write("OK");
    }
  } 

  digitalWrite(DUETRIG, LOW);
  DUESerial.end();
  Serial.println("Data collection finished!");
  // Serial.println(readBuffer);
}

void updatSavedCommand() {
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
}