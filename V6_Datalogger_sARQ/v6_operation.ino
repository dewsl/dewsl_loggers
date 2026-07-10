float parseVoltage(char* stringToParse, int stringContainerSize);

void Operation(const char * operationServerNumber, uint8_t dataloggerMode) {
  
  char infoSMS[200];
  // uint8_t dataloggerMode = fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0);
  // uint8_t powerMode = fetchParam(paramStorage, POWER_SAVING_MODE, (uint8_t)0);
  uint8_t ssmFlag = fetchParam(paramStorage, SUBSURFACE_SENSOR_FLAG, false);
  uint8_t ubloxFlag = fetchParam(paramStorage, UBLOX_FLAG, false);
  
  debugPrintln(">>>>  Operation START");
  GSMOn();                      //  if its is turned OFF caused by power saving; turn it of early to give time to connect


  diagnosticCheck(1);
  
  clearGlobalSMSDump();
  
  //  show timestamp here; it might be helpful..

  //  START OF DATA COLLECTION
  //  PLACE EVERYTHING RELATED TO DATA COLLECTION HERE
  debugPrintln("------------------------------------------------------");
  getLoggerModeAndName();
  if (dataloggerMode == 0 || dataloggerMode == 1) {
    debugPrint("Server number: ");
    debugPrintln(operationServerNumber);
  }
  debugPrintln("------------------------------------------------------");
  //  These are mostly print/labels for now.. unless other other functions are added per datalogger mode
  //  Labels are important so you'd know what you're getting in to...right?
  //  This used to be switch case type, it looks clean but ifs are shorter...for now
  if (dataloggerMode == 0)  {                                                     
    debugPrintln("Starting STAND-ALONE DATALOGGER operation");                    // nothing much yet after this
    // getSSMData();
  }
  else if (dataloggerMode == 1) {
      debugPrintln("Starting GATEWAY operation");
      debugPrintln("Waiting for router data..");                                  //  We'd assume that if its a gateway, there should be a router. Hence, it will wait..
      waitForLoRaRouterData(MAX_GATEWAY_WAIT_TIME, fetchParam(paramStorage, ROUTER_COUNT, (uint8_t)0), 0);
  }
  else if (dataloggerMode == 2) {        
      debugPrintln("Starting ROUTER operation");                                  // nothing much after this also
  }
  else debugPrintln("DATALOGGER MODE ERROR");                                     //  You should not reach this, but try to catch it anyway

  if (ssmFlag) {                                                                  //  this is placed here since all modes can have SSM
    auxPowerOn();
    getSSMData();
    auxPowerOff();
  }
  if (ubloxFlag) debugPrintln("Fetch UBLOX data here");                           //  this is placed here since I assumed all modes 'can' have a UBLOX module
  if (!ssmFlag && !ubloxFlag) debugPrintln("RAIN GAUGE ONLY");                    //  assumption; if it has no ssm or ublox, it operates as rain gauge only
  generateInfoMessage(infoSMS, sizeof(infoSMS));                                  //  datalogger 
  resetRainCounter(PCNT_UNIT);                                                    //  reset rain counter after data is fetched for sending so we wont miss anything 
  addToSMSStack(infoSMS);
  mDiagnosticBuilder(infoSMS, sizeof(infoSMS));
  addToSMSStack(infoSMS);
  // END OF DATA COLLECTION
  
  
  //  START OF DATA SENDING
  //  PLACE EVERYTHING RELATED TO DATA SENDING HERE
  
  if (dataloggerMode == 2) {   // ROUTER
      debugPrintln("Sending data to gateway via LoRa...");
      sendSMSDump("~", operationServerNumber);
  }
  else {   // STANDALONE or GATEWAY

      debugPrintln("Sending packets separately via GSM...");
      sendSMSDump("~", operationServerNumber);

  }
  debugPrintln(">>>>  Operation END");
}  

float inputVoltage(float Vmon, long res1, long res2) {
  //  based off Vmax of 14V
  float inputVoltage = ((Vmon * (res1+res2))/res2);
  return inputVoltage;
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
