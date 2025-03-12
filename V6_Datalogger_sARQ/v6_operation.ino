void Operation(const char * operationServerNumber) {
  
  char infoSMS[100];

  //  reload global variables
  uint8_t dataloggerMode = EEPROM.readByte(DATALOGGER_MODE);
  EEPROM.get(DATALOGGER_NAME, flashLoggerName);
  getTimeStamp(_timestamp, sizeof(_timestamp));
  clearGlobalSMSDump();

  //  main operation here
  switch(dataloggerMode) { 
    case ARQMODE: Serial.println("AQRMODE"); break; // nothing much
      Serial.println("ARQ MODE");    
    case GATEWAYMODE:
      Serial.println("GATEWAY MODE");
      debugPrintln("Waiting for router data..");    //  We'd assume that if its a gateway, there should be a router
      waitForLoRaRouterData(MAX_GATEWAY_WAIT_TIME, EEPROM.readByte(ROUTER_COUNT), 0);
      //  wake
      //  get router data
      break;     
    case ROUTERMODE: Serial.println("ROUTER MODE"); break;
      Serial.println("ROUTER MODE");
    default:
      Serial.println("DEFAULT MODE");
      //  Should be the same as ARQ mode unless a new mode is necessary
      //  
  }
  if (EEPROM.readByte(SUBSURFACE_SENSOR_FLAG)) getSSMData(_globalSMSDump);  
  if (EEPROM.readByte(UBLOX_FLAG)) getUBLOXData(_globalSMSDump);                
  generateInfoMessage(infoSMS);  
  addToSMSStack(infoSMS);
  // send everything here
}

void operation_test() {
  //  load saved parameters to local variables
  //  
  char dummyServer[15];
  char savedTimeStamp[15];
  char messageWrapper[200];

  digitalWrite(AUX_TRIG, HIGH);
  GSMInit();
  delayMillis(1000);
  
  EEPROM.get(SERVER_NUMBER, dummyServer);
  EEPROM.get(DATALOGGER_NAME, flashLoggerName);
  getTimeStamp(savedTimeStamp, sizeof(savedTimeStamp));

  // sprintf(messageWrapper, "%s*SLEEP/WAKE_TEST_%d*%d*%s",flashLoggerName.sensorNameList[0], alarmCount, tipCount, savedTimeStamp);
  // sprintf(messageWrapper, "%s*SLEEP/WAKE_TEST_%d*%d*%s",flashLoggerName.sensorNameList[0], alarmCount, (RTC_SLOW_MEM[EDGE_COUNT] & 0xFFFF)/2, savedTimeStamp);
  generateInfoMessage(messageWrapper);

  if (sendThruGSM(messageWrapper,dummyServer)) debugPrintln("Message sent");
  else {GSMReset(); if (sendThruGSM(messageWrapper,dummyServer)) debugPrintln("Message sent");}

  digitalWrite(AUX_TRIG, LOW);
  alarmCount++;
  delayMillis(1000);
}

// void generateVoltString (char* stringContainer) {
//   EEPROM.get(DATALOGGER_NAME, flashLoggerName);
//   getTimeStamp(_timestamp, sizeof(_timestamp));
//   delayMillis(1000);
//   sprintf(stringContainer, ">>%s*VOLT:%.2f*%s",flashLoggerName.sensorNameList[0], readBatteryVoltage(savedBatteryType.read()),_timestamp);
// }

float inputVoltage(float Vmon, long res1, long res2) {
  //  based off Vmax of 14V
  float inputVoltage = ((Vmon * (res1+res2))/res2);
}

float parseVoltage(char* stringToParse, int stringContainerSize) {
  int i = 0;
  float voltageParsed=0;
  char parseBuffer[stringContainerSize];
  sprintf(parseBuffer, stringToParse);
  char *buff = strtok(parseBuffer, ":*");
  while (buff != NULL) {
    if (i==2) {
      voltageParsed = atof(buff);
      debugPrintln(buff);
      break;
    }
    buff = strtok(NULL, ":*");
    i++;
  }
  return voltageParsed;
}