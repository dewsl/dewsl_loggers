/// Initiates LoRa Radio
///
/// Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
/// Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
/// Transmit power can be set from 5 to 23 dB with RFM95/96/97/98 with PA_BOOST transmitter pin.
/// The default transmitter power is 13dBm, using PA_BOOST.
///
void LoRaInit(uint32_t initWaitTime) {
  resetWatchdog();
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delayMillis(10);
  digitalWrite(RFM95_RST, HIGH);
  delayMillis(10);
  bool loraInitOK = false;
  unsigned long initWaitStart = millis();
  while (millis() - initWaitStart < initWaitTime && !loraInitOK) {
    resetWatchdog();
    if (rf95.init()) {
      Serial.println("LoRa radio init OK!"); 
      if (!rf95.setFrequency(RF95_FREQ)) Serial.println("set frequency failed");
      else  {Serial.print("Set Freq to: "); Serial.println(RF95_FREQ); }
      rf95.setTxPower(23, false);
      break;
    }
    else Serial.println("LoRa radio init failed"); 
    delayMillis(1000);
    // set error flag here for next action
    resetWatchdog();
  }
  resetWatchdog();
}


/// Single instance of sending data thru LoRa.
/// 
/// @param radiopacket - pointer for data to be sent thru LoRa
///
void sendThruLoRa(const char *radiopacket) {
  resetWatchdog();
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
  if (!rf95.send(payload, sizeof(payload))) debugPrint("packet sending failed");  // sending data to LoRa
  resetWatchdog();
  rf95.waitPacketSent();       // Waiting for packet to complete...
  resetWatchdog();
}

//used by routers
bool sendThruLoRaWithAck(const char* payloadToSend, uint32_t responseWaitTime, uint8_t retryCount) {
  resetWatchdog();
  char ackResponseBuffer[300];
  char validResponse[30];
  bool noResponse = true;
  sprintf(validResponse, "%s%s",flashLoggerName.sensorNameList[0],ackKey);
  
  for (int retryIndex = 0; retryIndex <= retryCount; retryIndex++) {
    sendThruLoRa(payloadToSend);
    unsigned long ackWaitStart = millis();
    while (millis() - ackWaitStart < responseWaitTime && noResponse) {
      resetWatchdog();
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
      resetWatchdog();
    }
    if (!noResponse) break;
  }
  resetWatchdog();
  return !noResponse;
}


/// Single instance of receiving data thru LoRa.
///
/// @param receiveContainer - container for the incoming LoRa data
/// @param receiveContainerSize - size of the data container
///
void receiveLoRaData(char* receiveContainer, uint16_t receiveContainerSize, unsigned long waitDuration) {
  resetWatchdog();
  uint8_t receiveBuffer[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t bufferLength = sizeof(receiveBuffer);
  bool waitDataFlag = true;
  unsigned long LoRaWaitStart = millis();
  
  for (int i = 0; i < receiveContainerSize; i++) receiveContainer[i]=0x00;
  // char payloadBuffer[RH_RF95_MAX_MESSAGE_LEN+1];
  while (millis() - LoRaWaitStart < waitDuration && waitDataFlag) {
    resetWatchdog();
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
    resetWatchdog();
  }
  resetWatchdog();
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
void waitForLoRaRouterData(unsigned long receiverWaitDuration, int routerCount, uint8_t receiveMode) {
  resetWatchdog();
  char gatewayDataDump[200];
  char routerNames[routerCount][20];
  char loRaBuffer[1000];
  uint8_t routerNameIndex = 100;
  uint8_t nameIndexLimit = 100;     // temporary limit
  uint8_t voltCount = 0;
  char sendAck[50];
  int RSSIbuffer = 0;
  int RSSIContainer[routerCount];
  float voltContainer[routerCount];  

  for (int r = 0; r <= routerCount;r++) RSSIContainer[r]=0;
  for (int v = 0; v <= routerCount;v++) voltContainer[v]=0;
  bool voltFlag = false;

  // debugPrint("Router count: ");
  // debugPrintln(savedRouterCount.read());
  debugPrintln("Listed router(s): ");
  for (int routerPos=1; routerPos<=savedRouterCount.read(); routerPos++) debugPrintln(flashLoggerName.sensorNameList[routerPos]);
  debugPrintln("");
  debugPrint("Wait time limit: ");
  debugPrint(receiverWaitDuration/1000/60);
  debugPrintln(" minute(s)");
  debugPrintln("Waiting for transmission... ");

  unsigned long routerWaitStart = millis();
  // Serial.println(routerWaitStart);

  while (millis() - routerWaitStart < receiverWaitDuration) {
    debugPrintln("~");
    resetWatchdog();
    receiveLoRaData(loRaBuffer, sizeof(loRaBuffer), 30000);        // receive instances of lora data here
    debugPrintln("~");                                               // waiting/recieving indicator

    //filters data depending on saved datalogger names
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
      RSSIbuffer = rf95.lastRssi();
      if (RSSIbuffer > -100) RSSIContainer[routerNameIndex] = RSSIbuffer;
      debugPrint("Signal Loss: "); 
      debugPrintln(RSSIbuffer);
      RSSIbuffer = 0;

      //STORE IDENTIFIED DATA HERE
      if (inputHas(loRaBuffer, "*VOLT")) { // in case of termitating string MADTB*VOLT:12.33*
        voltFlag = true;            // set volt flag
        debugPrintln("Router info:");
        voltContainer[routerNameIndex] = parseVoltage(loRaBuffer, sizeof(loRaBuffer));

        debugPrint("Supply voltage: "); 
        debugPrintln(voltContainer[routerNameIndex]);
   
        voltCount++;
      } else  {
        if (receiveMode == 0) addToSMSStack(loRaBuffer);      // adds data from routers to sending stack
      }

      //SEND ACKNOWLEDGMENT RESPONSE HERE
      char tsNetBuffer[50];
      getNetworkFormatTimeStamp(tsNetBuffer, sizeof(tsNetBuffer));
      if (voltFlag && routerOTAflag) sprintf(sendAck, "%s%s~ROUTER~%s~%s", flashLoggerName.sensorNameList[routerNameIndex],ackKey,routerOTACommand,tsNetBuffer); // sample: LTEG^REC'D_~ROUTER~RESET~24/07/08,10:30:00
      else sprintf(sendAck, "%s%s", flashLoggerName.sensorNameList[routerNameIndex],ackKey);
      sendThruLoRa(sendAck);

    } else {
      // ano gusto mo gawin sa rejected na transmissions ?
    }
    if (voltCount == routerCount) {
      debugPrint("Router check count complete.."); 
      break;
    }
    // else () {  // deal with junk here
    // do something with the strings that didn't get through the filter?
    // } 
    resetWatchdog();
  }

  routerOTAflag = false;      // reset router OTA flag
  for (int r=0;r<sizeof(routerOTACommand);r++) routerOTACommand[r]=0x00; // clears global router OTA container
  
  if (receiveMode == 0) {
    // build gateway data here
    char numBuffer[10];

    strcpy(gatewayDataDump, "GATEWAY*RSSI,");
    strncat(gatewayDataDump, flashLoggerName.sensorNameList[0], 3);
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
  resetWatchdog();
}

float parseVoltage(char* stringToParse, int stringContainerSize) {
  resetWatchdog();
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
  resetWatchdog();
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
  resetWatchdog();
  char referenceBuffer[sizeof(referenceString)+1];
  char keyBuffer[20];
  // strncpy(toFind, ">>", 2);
  
  if (savedDataLoggerMode.read() == 2) {  //  add other router modes here
    //  gets first charaters (depending on router char length) from reference to generate acknowledgement key
    strncpy(keyBuffer, referenceString, (strlen(flashLoggerName.sensorNameList[0])));   
  } else {  // gateway modes
    //  gets first characters (depending on router char length) from reference to generate acknowledgement key
    //  alternative ito sa tokenization kasi nanggaling tayo sa strtok
    strncpy(keyBuffer, referenceString, (strlen(flashLoggerName.sensorNameList[1]))); // bakit 1 ito? ang assumption ay pare parehas lang ang length ng router names kaya ito ang ginamit na reference [temporary].. 
  }    
  resetWatchdog();
}

/// Checks whether received data is from a valid datalogger.
/// Router modes retuns 0 for valid acknowledgement.
/// Gateways return the list position of matched router
///
/// @param  payloadToCheck received LoRa data to check
/// @param  sizeOfPayload array size of receive data containe
///
int loRaFilterPass(char* payloadToCheck, int sizeOfPayload) {
  resetWatchdog();
  uint8_t payloadType = 0;
  char payloadBuffer[sizeOfPayload+1];

  sprintf(payloadBuffer, payloadToCheck);

  // remove first ">>" characters
  // tokenize using "*" as delimiter to get the name?

  for (byte rIndex = 0; rIndex <= savedRouterCount.read(); rIndex++) {
    resetWatchdog();
    if (inputHas(payloadBuffer, flashLoggerName.sensorNameList[rIndex])) return rIndex; // returns index of datalogger name
  }
  resetWatchdog();
  return MAX_ROUTER_COUNT+99;    
  //  return any value higher than the max router count
  //  default return value unless router name is found on sensorNameList
  
}

void extractRouterName(char *nameContainer, char * referenceString) {
  resetWatchdog();
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
  resetWatchdog();
}
// MADTB*VOLT:12.33*200214111000
void generateVoltString (char* stringContainer) {
  resetWatchdog();
  char tsbuffer[15];
  getTimeStamp(tsbuffer, sizeof(tsbuffer));
  delayMillis(1000);
  sprintf(stringContainer, ">>%s*VOLT:%.2f*%s",flashLoggerName.sensorNameList[0], readBatteryVoltage(savedBatteryType.read()),tsbuffer);
  resetWatchdog();
}