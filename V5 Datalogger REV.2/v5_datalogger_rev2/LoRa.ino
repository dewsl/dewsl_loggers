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
void sendThruLoRa(char *radiopacket) {
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
bool sendThruLoRaWithAck(char* payloadToSend, uint32_t responseWaitTime, uint8_t retryCount) {
  char ackResponseBuffer[300];
  char validResponse[30];
  bool noResponse = true;
  sprintf(validResponse, "%s%s",flashLoggerName.sensorNameList[0],ackKey);
  validResponse[strlen(validResponse)+1]=0x00;
  
  for (int retryIndex = 0; retryIndex <= retryCount; retryIndex++) {
    sendThruLoRa(payloadToSend);
    unsigned long ackWaitStart = millis();
    while (millis() - ackWaitStart < responseWaitTime && noResponse) {
      debugPrintln("Checking response..");
      receiveLoRaData(ackResponseBuffer, sizeof(ackResponseBuffer), 5000);
      // debugPrintln(ackResponseBuffer);
      // debugPrintln(validResponse);
      if (strstr(ackResponseBuffer, validResponse)) {
        noResponse = false;
        break;
      }
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
/// Mode 2: [Listen mode LoRa send/receive] Data received displayed but are not filtered and are not added to SMS stack.
void waitForLoRaRouterData(unsigned long receiverWaitDuration, uint8_t routerCount, uint8_t receiveMode) {
  char gatewayDataDump[200];
  char routerNames[routerCount][20];
  unsigned long waitStart = millis();
  char loRaBuffer[1000];
  uint8_t receiveType = 99;
  uint8_t voltCount = 0;
  uint8_t routerPos;
  // char senderLabelBuffer[20];
  char sendAck[50];
  int RSSIbuffer = 0;
  int RSSIContainer[routerCount];
  float voltContainer[routerCount];  

  debugPrintln("Waiting for LoRa transmission from listed router(s): ");
  while (millis() - waitStart < receiverWaitDuration) {
    receiveLoRaData(loRaBuffer, sizeof(loRaBuffer), 30000);        //receive instances of lora data here
    debugPrint(".");
    //filters data depending on saved datalogger names
    if (receiveMode == 0 || receiveMode == 1) receiveType = loRaFilterPass(loRaBuffer, sizeof(loRaBuffer));
    if (receiveType > 0 && receiveType < 99 ) {  // receive type depends on list position of saved router name
      debugPrintln(loRaBuffer);
      debugPrint("Received data from ");
      debugPrintln(flashLoggerName.sensorNameList[receiveType]);
      sprintf(sendAck, "%s%s", flashLoggerName.sensorNameList[receiveType],ackKey);
      sendAck[strlen(sendAck)+1] = 0x00;
      sendThruLoRa(sendAck);
      if (strstr(loRaBuffer, "VOLT")) { // in case of termitating string MADTB*VOLT:12.33*200214111000
        debugPrintln("Router info:");
        voltContainer[receiveType] = parseVoltage(loRaBuffer, sizeof(loRaBuffer));
        RSSIContainer[receiveType] = rf95.lastRssi();

        debugPrint("Supply voltage: "); 
        debugPrintln(voltContainer[receiveType]);
        debugPrint("Signal Loss: "); 
        debugPrintln(RSSIContainer[receiveType]);     
        voltCount++;
      } else  {
        if (receiveMode == 0) addToSMSStack(loRaBuffer);      // adds data from routers to sending stack
      }
    } else {
      // ano gusto mo gawin sa rejected na transmissions ?
    }
    if (voltCount == routerCount) break;
    // else () {  // deal with junk here
    // do something with the strings that didn't get through the filter?
    // } 
  }
  if (receiveMode == 0) {
    // build gateway data here
    char numBuffer[10];

    strcpy(gatewayDataDump, "GATEWAY*RSSI,");
    strncat(gatewayDataDump, flashLoggerName.sensorNameList[0], 3);
    strcat(gatewayDataDump, ",");

    for (byte rCount = 0; rCount < routerCount; rCount++) {
      strcat(gatewayDataDump, routerNames[rCount]);
      strcat(gatewayDataDump, ",");
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
    strncpy(keyBuffer, referenceString, (strlen(flashLoggerName.sensorNameList[0])));   
  } else {  // gateway modes
    //  gets first charaters (depending on router char length) from reference to generate acknowledgement key
    
    strncpy(keyBuffer, referenceString, (strlen(flashLoggerName.sensorNameList[1]))); // bakit 1 ito? ang assumption ay pare parehas lang ang length ng router names kaya ito ang ginamit na reference.. 

  }    
  // ackMsg[strlen(ackMsg)+1] = 0x00;
  // strcat(ackMsg, ackKey);
  // ackMsg[strlen(ackMsg)+1] = 0x00; 
}

/// Checks whether received data is from a valid datalogger.
/// Router modes retuns 0 for valid acknowledgement.
/// Gateways return the list position of matched router
///
/// @param  payloadToCheck received LoRa data to check
/// @param  sizeOfPayload array size of receive data containe
///
int loRaFilterPass(char* payloadToCheck, int sizeOfPayload) {
  uint8_t payloadType = 0;
  char payloadBuffer[sizeOfPayload+1];

  sprintf(payloadBuffer, payloadToCheck);
  payloadBuffer[strlen(payloadBuffer)+1]=0x00;

  for (byte rIndex = 0; rIndex < savedRouterCount.read(); rIndex++) {
    if (strstr(payloadBuffer, flashLoggerName.sensorNameList[rIndex])) return rIndex; // returns index of datalogger name
  }
  return 99;    // return 99 if payload is not found/invalid
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
  // char stringBUffer[50];
  char tsbuffer[15];
  getTimeStamp(tsbuffer, sizeof(tsbuffer));
  sprintf(stringContainer, ">>%s*VOLT:%.2f*%s",flashLoggerName.sensorNameList[0], readBatteryVoltage(savedBatteryType.read()),tsbuffer);
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
}