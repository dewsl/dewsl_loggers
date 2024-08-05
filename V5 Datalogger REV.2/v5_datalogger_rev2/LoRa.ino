/// Initiates LoRa Radio
///
/// Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
/// Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
/// Transmit power can be set from 5 to 23 dB with RFM95/96/97/98 with PA_BOOST transmitter pin.
/// The default transmitter power is 13dBm, using PA_BOOST.
///
void LoRaInit() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delayMillis(10);
  digitalWrite(RFM95_RST, HIGH);
  delayMillis(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed"); 
    // set error flag here for next action
  }
  Serial.println("LoRa radio init OK!");

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    // set error flag here for next action
  }
  Serial.print("Set Freq to: ");
  Serial.println(RF95_FREQ);
  rf95.setTxPower(23, false);
}


/// Single instance of sending data thru LoRa.
/// 
/// @param radiopacket - pointer for data to be sent thru LoRa
///
void sendThruLoRa(const char *radiopacket) {
  uint8_t payload[RH_RF95_MAX_MESSAGE_LEN];
  int sendRetryLimit = 3;
  int i = 0, j = 0;

  for (i = 0; i < sizeof(payload); i++) {
    payload[i] = 0x00;
  }
  for (j = 0; j < sizeof(payload); j++) {
    payload[j] = (uint8_t)radiopacket[j];
  }
  payload[j] = 0x00;

  debugPrint("Sending to LoRa: ");
  debugPrintln((char *)payload);
  rf95.send(payload, sizeof(payload));  // sending data to LoRa
  rf95.waitPacketSent();       // Waiting for packet to complete...
}

//used by routers
bool sendThruLoRaWithAck(const char* payloadToSend, uint32_t responseWaitTime, uint8_t retryCount) {
  char ackResponseBuffer[300];
  char validResponse[30];
  bool noResponse = true;
<<<<<<< HEAD
  sprintf(validResponse, "%s%s",flashLoggerName.sensorA,ackKey);
  validResponse[strlen(validResponse)+1]=0x00;
=======
  sprintf(validResponse, "%s%s",flashLoggerName.sensorNameList[0],ackKey);
>>>>>>> 741137fe53ab096d63f884a8d994883109a9f9ea
  
  for (int retryIndex = 0; retryIndex <= retryCount; retryIndex++) {
    sendThruLoRa(payloadToSend);
    unsigned long ackWaitStart = millis();
    while (millis() - ackWaitStart < responseWaitTime && noResponse) {
      debugPrintln("Checking response..");
      receiveLoRaData(ackResponseBuffer, sizeof(ackResponseBuffer), responseWaitTime);
      debugPrint(ackResponseBuffer);
      // debugPrintln(validResponse);
      if (inputHas(ackResponseBuffer, validResponse)) {
        debugPrintln(" << acknowledged by gateway");
      
        if (inputHas(ackResponseBuffer, "~ROUTER~")) {
          routerProcessOTAflag = true;
          sprintf(routerOTACommand, ackResponseBuffer);
          
        }
          


        noResponse = false;
        break;
      } else debugPrintln("");
    }
    if (!noResponse) break;
  }
  return !noResponse;
}


/// Single instance of receiving data thru LoRa.
///
/// @param receiveContainer - container for the incoming LoRa data
/// @param receiveContainerSize - size of the data container
///
void receiveLoRaData(char* receiveContainer, uint16_t receiveContainerSize, unsigned long waitDuration) {
  uint8_t receiveBuffer[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t bufferLength = sizeof(receiveBuffer);
  bool waitDataFlag = true;
  unsigned long LoRaWaitStart = millis();
  
  for (int i = 0; i < receiveContainerSize; i++) receiveContainer[i]=0x00;
  // char payloadBuffer[RH_RF95_MAX_MESSAGE_LEN+1];
  while (millis() - LoRaWaitStart < waitDuration && waitDataFlag) {
    if (rf95.available()) {
      if (rf95.recv(receiveBuffer, &bufferLength)) {
        int l = 0;
        for (l = 0; l < bufferLength; ++l) {
          receiveContainer[l] = (uint8_t)receiveBuffer[l];
        }
        receiveContainer[l] = 0x00;
      }
    }
    if (strlen(receiveContainer)) waitDataFlag = false;
  }
}

/// Waits for LoRa data from router(s) until data sending is ended or timeout is reached.
/// Build gateway-router info as LoRa data is received
///
/// @param receiverWaitDuration max duration of receiver wait time
/// @param routerCount number of routers expected to send data
/// @param receiveMode determined how to process incoming LoRa data
///
/// Mode 0: [Normal operation] Received data are filtered and added to SMS stack for sending.
/// Mode 1: [Testing mode for LoRa send/receive] Data received are filtered but not added to the sending stack.
/// Mode 2: [Listen mode; LoRa send/receive] Data received displayed but are not filtered and are not added to SMS stack.
void waitForLoRaRouterData(unsigned long receiverWaitDuration, uint8_t routerCount, uint8_t receiveMode) {
  char gatewayDataDump[200];
  char routerNames[routerCount][20];
  unsigned long waitStart = millis();
  char loRaBuffer[1000];
  uint8_t routerNameIndex = 100;
  uint8_t nameIndexLimit = 100;     // temporary limit
  uint8_t voltCount = 0;
  uint8_t routerPos;
  // char senderLabelBuffer[20];
  char sendAck[50];
  int RSSIbuffer = 0;
  int RSSIContainer[routerCount];
  float voltContainer[routerCount];  

<<<<<<< HEAD
  //insert function here to populate routerData
  if (routerCount == 1) {
    sprintf(routerNames[0], "%s", flashLoggerName.sensorB);
  } else if (routerCount == 2) {
    sprintf(routerNames[0], "%s", flashLoggerName.sensorB);
    sprintf(routerNames[1], "%s", flashLoggerName.sensorC);
  } else if (routerCount == 3) {
    sprintf(routerNames[0], "%s", flashLoggerName.sensorB);
    sprintf(routerNames[1], "%s", flashLoggerName.sensorC);
    sprintf(routerNames[2], "%s", flashLoggerName.sensorD);
  }
=======
  for (int r = 0; r <= routerCount;r++) RSSIContainer[r]=0;
  for (int v = 0; v <= routerCount;v++) voltContainer[v]=0;
  bool voltFlag = false;

>>>>>>> 741137fe53ab096d63f884a8d994883109a9f9ea
  debugPrintln("Waiting for LoRa transmission from listed router(s): ");
  for (int r=1; r<=savedRouterCount.read(); r++) debugPrintln(flashLoggerName.sensorNameList[r]);
  while (millis() - waitStart < receiverWaitDuration) {

    receiveLoRaData(loRaBuffer, sizeof(loRaBuffer), 30000);        // receive instances of lora data here
    debugPrintln("~");                                               // waiting/recieving indicator

    //filters data depending on saved datalogger names
<<<<<<< HEAD
    if (receiveMode == 0 || receiveMode == 1) receiveType = loRaFilterPass(loRaBuffer, sizeof(loRaBuffer));
    if (receiveType >= 0 && receiveType < 99 ) {  // receive type depends on list position of saved router name
      debugPrintln(loRaBuffer);
      debugPrint("Received data from ");
      debugPrintln(routerNames[receiveType]);
      sprintf(sendAck, "%s%s", routerNames[receiveType],ackKey);
      sendAck[strlen(sendAck)+1];
      sendThruLoRa(sendAck);
      if (strstr(loRaBuffer, "VOLT")) { // in case of termitating string MADTB*VOLT:12.33*200214111000
=======
    if (receiveMode == 0 || receiveMode == 1) routerNameIndex = loRaFilterPass(loRaBuffer, sizeof(loRaBuffer));
    //  if filter below
    //  0 - 99 indicates list position of saved router name
    //  100+ it is not incuded in the loggername list as self[0] or routers[1-99]
    if (routerNameIndex > 0 && routerNameIndex < nameIndexLimit ) {   
      //RECEIVE IDENTIFIED HERE
      LEDReceive();
      debugPrint("Received data from ");
      debugPrintln(flashLoggerName.sensorNameList[routerNameIndex]);
      debugPrintln(loRaBuffer);
      RSSIContainer[routerNameIndex] = rf95.lastRssi();
      debugPrint("Signal Loss: "); 
      debugPrintln(RSSIContainer[routerNameIndex]);

      //STORE IDENTIFIED DATA HERE
      if (inputHas(loRaBuffer, "*VOLT")) { // in case of termitating string MADTB*VOLT:12.33*
        voltFlag = true;            // set volt flag
>>>>>>> 741137fe53ab096d63f884a8d994883109a9f9ea
        debugPrintln("Router info:");
        voltContainer[routerNameIndex] = parseVoltage(loRaBuffer, sizeof(loRaBuffer));

        debugPrint("Supply voltage: "); 
        debugPrintln(voltContainer[routerNameIndex]);
   
        voltCount++;
      } else  {
        if (receiveMode == 0) addToSMSStack(loRaBuffer);      // adds data from routers to sending stack
      }
<<<<<<< HEAD
=======

      //SEND ACKNOWLEDGMENT RESPONSE HERE
      char tsNetBuffer[50];
      getNetworkFormatTimeStamp(tsNetBuffer, sizeof(tsNetBuffer));
      if (voltFlag && routerOTAflag) sprintf(sendAck, "%s%s~ROUTER~%s~%s", flashLoggerName.sensorNameList[routerNameIndex],ackKey,routerOTACommand,tsNetBuffer); // sample: LTEG^REC'D_~ROUTER~RESET~24/07/08,10:30:00
      else sprintf(sendAck, "%s%s", flashLoggerName.sensorNameList[routerNameIndex],ackKey);
      sendThruLoRa(sendAck);

    } else {
      // ano gusto mo gawin sa rejected na transmissions ?
>>>>>>> 741137fe53ab096d63f884a8d994883109a9f9ea
    }
    if (voltCount == routerCount) break;
    // else () {  // deal with junk here
    // do something with the strings that didn't get through the filter?
    // } 
  }

  routerOTAflag = false;      // reset router OTA flag
  for (int r=0;r<sizeof(routerOTACommand);r++) routerOTACommand[r]=0x00; // clears global router OTA container
  
  if (receiveMode == 0) {
    // build gateway data here
    char numBuffer[10];

    strcpy(gatewayDataDump, "GATEWAY*RSSI,");
    strncat(gatewayDataDump, flashLoggerName.sensorA, 3);
    strcat(gatewayDataDump, ",");

    for (byte rCount = 1; rCount <= routerCount; rCount++) {   
      strcat(gatewayDataDump, flashLoggerName.sensorNameList[rCount]);    // adds router name first
      strcat(gatewayDataDump, ",");                                       // delimiter
      if (RSSIContainer[rCount] != 0) {
        sprintf(numBuffer, "%d", RSSIContainer[rCount]);
        strcat(gatewayDataDump, numBuffer);
      }
      strcat(gatewayDataDump, ",");
      if (voltContainer[rCount] != 0) {
        sprintf(numBuffer, "%0.2f", voltContainer[rCount]);
        strcat(gatewayDataDump, numBuffer);
      }
      strcat(gatewayDataDump, ",");
    }
    getTimeStamp(_timestamp, sizeof(_timestamp));
    strncat(gatewayDataDump, _timestamp, strlen(_timestamp));
    addToSMSStack(gatewayDataDump);
    debugPrintln(gatewayDataDump);
  }
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

/// Generates an acknowlegment key used for confirming successful transmission of sensor/datalogger data
/// 
/// Gataway modes: The function searches for an instance the router name of from the reference string to be sent back as an acknowledgement.
/// Router mode: Generates an acknowledgement key that will be compared with the acknowledgement key to be received from the gateway
/// 
/// @param keyContainer - pointer of the container for the acknowledgement key that will be generated
/// @param referenceString - used by gateways to indentify if transmission came from a listed router
///
void key_gen(char *keyContainer, char *referenceString) {
  char referenceBuffer[sizeof(referenceString)+1];
  char keyBuffer[20];
  // strncpy(toFind, ">>", 2);
  
  if (savedDataLoggerMode.read() == 2) {  //  add other router modes here
    //  gets first charaters (depending on router char length) from reference to generate acknowledgement key
    strncpy(keyBuffer, referenceString, (strlen(flashLoggerName.sensorA)));   
  } else {  // gateway modes
    //  gets first charaters (depending on router char length) from reference to generate acknowledgement key
    
    strncpy(keyBuffer, referenceString, (strlen(flashLoggerName.sensorB)));

  }    
  // ackMsg[strlen(ackMsg)+1] = 0x00;
  // strcat(ackMsg, ackKey);
  // ackMsg[strlen(ackMsg)+1] = 0x00; 
}

/// Checks whether received data is from a valid datalogger.
/// Router modes retuns 0 for valid acknowledgement.
/// Gateways return the list position of matched router
/// Currently limited to 3 routers, but can easily be extended
///
/// @param  payloadToCheck received LoRa data to check
/// @param  sizeOfPayload array size of receive data containe
///
int loRaFilterPass(char* payloadToCheck, int sizeOfPayload) {
  uint8_t payloadType = 0;
  uint8_t returnType = 99;       // default for routers
  char payloadBuffer[sizeOfPayload+1];
  char matchKey[20];

  sprintf(payloadBuffer, payloadToCheck);
  payloadBuffer[strlen(payloadBuffer)+1]=0x00;

<<<<<<< HEAD
  if (loggerWithGSM(savedDataLoggerMode.read()))  {  
    sprintf(matchKey, ">>%s",flashLoggerName.sensorB);
    if(strstr(payloadBuffer, matchKey)) return 0;
    sprintf(matchKey, ">>%s",flashLoggerName.sensorC);
    if(strstr(payloadBuffer, matchKey)) return 1;
    sprintf(matchKey, ">>%s",flashLoggerName.sensorD);
    if(strstr(payloadBuffer, matchKey)) return 2;
    // add router names here if necessary

  } else  {        // all routers
    sprintf(matchKey, "%s%s",flashLoggerName.sensorA,ackKey);
    if (strstr(payloadBuffer, matchKey)) return 0;
  }
  return returnType;
=======
  // remove first ">>" characters
  // tokenize using "*" as delimiter to get the name?

  for (byte rIndex = 0; rIndex <= savedRouterCount.read(); rIndex++) {
    if (inputHas(payloadBuffer, flashLoggerName.sensorNameList[rIndex])) return rIndex; // returns index of datalogger name
  }
  return 199;    
  // return 199 if payload is not found/invalid
  // or any value higher than the max router count
>>>>>>> 741137fe53ab096d63f884a8d994883109a9f9ea
}

void extractRouterName(char *nameContainer, char * referenceString) {
  char referenceBuffer[strlen(referenceString)+1];
  sprintf(referenceBuffer, referenceString);
  referenceBuffer[strlen(referenceBuffer)+1] = 0x00;
  uint8_t tokenIndex = 0;
  char *tokenBuffer = strtok(referenceBuffer, ">*,");
  while(tokenBuffer != NULL && tokenIndex == 0) {
    tokenIndex++;
    sprintf(nameContainer, tokenBuffer);
    nameContainer[strlen(nameContainer)+1] = 0x00;
    break;
  }
}
// MADTB*VOLT:12.33*200214111000
void generateVoltString (char* stringContainer) {
  char tsbuffer[15];
  getTimeStamp(tsbuffer, sizeof(tsbuffer));
<<<<<<< HEAD
  sprintf(stringContainer, ">>%s*VOLT:%.2f*%s",flashLoggerName.sensorA, readBatteryVoltage(savedBatteryType.read()),tsbuffer);
  // stringBUffer[strlen(stringBUffer)+1] = 0x00;
  // sprintf(voltData,"%.2f", readBatteryVoltage(savedBatteryType.read()));
  // strcpy(stringBUffer, ">>");
  // strcat(stringBUffer, flashLoggerName.sensorA);
  // strcat(stringBUffer,"*VOLT:");
  // strncat(stringBUffer, voltData, strlen(voltData)); 
  // strcat(stringBUffer, "*");
  // strncat(stringBUffer, tsbuffer, strlen(tsbuffer));

}

void runLoRaOTA(char * ackSegment) {
  // TESTA^REC'D_RESET~
  // TESTA^REC'D_SETDATETIME~YY,MM,DD,hh,mm,ss,dd
  char segmentBuffer[100];
  sprintf(segmentBuffer,"%s",ackSegment);  //ilipat sa buffer para pwede i-compare
  if (strcmp(segmentBuffer,"RESET")) {
    debugPrintln("Datalogger will reset");  // replace this with
    delayMillis(1000);
    // add notification fnction here 
    NVIC_SystemReset();

  } else if (strcmp(segmentBuffer,"SETDATETIME")) {

    char *_keyword = strtok(segmentBuffer + 11, "~");
    char *YY = strtok(NULL, ",");
    char *MM = strtok(NULL, ",");
    char *DD = strtok(NULL, ",");
    char *hh = strtok(NULL, ",");
    char *mm = strtok(NULL, ",");
    char *ss = strtok(NULL, ",");
    char *dd = strtok(NULL, ",");

    int _YY = atoi(YY);
    int _MM = atoi(MM);
    int _DD = atoi(DD);
    int _hh = atoi(hh);
    int _mm = atoi(mm);
    int _ss = atoi(ss);
    int _dd = atoi(dd);

    setRTCDateTime(_YY, _MM, _DD, _hh, _mm, _ss, _dd);
    getTimeStamp(_timestamp, sizeof(_timestamp));
    debugPrint("Current timestamp: ");
    debugPrintln(_timestamp);
  }
=======
  sprintf(stringContainer, ">>%s*VOLT:%.2f*%s",flashLoggerName.sensorNameList[0], readBatteryVoltage(savedBatteryType.read()),tsbuffer);
>>>>>>> 741137fe53ab096d63f884a8d994883109a9f9ea
}