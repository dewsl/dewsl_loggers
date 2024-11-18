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
      rf95.setCADTimeout(5000);
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
  // check channel activity before sending

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
bool receiveLoRaData(char* receiveContainer, uint16_t receiveContainerSize, unsigned long waitDuration) {
  resetWatchdog();
  bool receiveStatus = false;
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
    if (strlen(receiveContainer)) {waitDataFlag = false; receiveStatus=true;}
    resetWatchdog();
  }
  resetWatchdog();
  return receiveStatus;
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
/// Mode 2: [Listen only; LoRa send/receive] Data received displayed but are not filtered and are not added to SMS stack.
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
  int endCounter[routerCount+1];  // this will not use index zero. the ff can be simplified using multidimensional array, but this is easier to read..
  int RSSIContainer[routerCount+1];
  float voltContainer[routerCount+1];
  int endCount = 0;

  for (int e = 0; e <= routerCount;e++) endCounter[e]=0;
  for (int r = 0; r <= routerCount;r++) RSSIContainer[r]=0;
  for (int v = 0; v <= routerCount;v++) voltContainer[v]=0;

  bool voltFlag = false;

  debugPrint("Router count: ");
  debugPrintln(savedRouterCount.read());
  debugPrintln("Listed router(s): ");
  for (int routerPos=1; routerPos <= savedRouterCount.read(); routerPos++) debugPrintln(flashLoggerName.sensorNameList[routerPos]);
  debugPrintln("");
  debugPrint("Wait time limit: ");
  debugPrint(receiverWaitDuration/1000/60);
  debugPrintln(" minute(s)");
  debugPrintln("Waiting for transmission... ");
  unsigned long routerWaitStart = millis();

  // Serial.println(routerWaitStart);

  while (millis() - routerWaitStart < receiverWaitDuration) {
    for (int lb = 0; lb < sizeof(loRaBuffer); lb++) loRaBuffer[lb]=0x00;  // clear buffer
    debugPrintln("~");
    resetWatchdog();
    receiveLoRaData(loRaBuffer, sizeof(loRaBuffer), 30000);        // receive instances of lora data here
    debugPrintln("~");                                               // waiting/recieving indicator

    // filters data depending on saved datalogger names
    if (receiveMode == 0 || receiveMode == 1) routerNameIndex = loRaFilterPass(loRaBuffer, sizeof(loRaBuffer));
    //  if filter below
    //  0 - 99 indicates list position of saved router name
    //  0 = self
    //  1 to 99 are router index on loggernames list () 
    //  100+ it is not incuded in the loggername list
    if (routerNameIndex > 0 && routerNameIndex < nameIndexLimit ) {   
      //RECEIVE IDENTIFIED HERE
      LEDReceive();
      debugPrint("Received data from ");
      debugPrintln(flashLoggerName.sensorNameList[routerNameIndex]);
      debugPrint("Router index: ");
      debugPrintln(routerNameIndex);
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

        endCounter[routerNameIndex] = 1; // test array for counting "VOLT" string

      } else  {
        if (receiveMode == 0) addToSMSStack(loRaBuffer);      // adds data from routers to sending stack
      }

      //SEND ACKNOWLEDGMENT RESPONSE HERE
      if (voltFlag && routerOTAflag)  {
        voltFlag = false;
        char tsNetBuffer[50];
        getNetworkFormatTimeStamp(tsNetBuffer, sizeof(tsNetBuffer));
        sprintf(sendAck, "%s%s~ROUTER~%s~%s", flashLoggerName.sensorNameList[routerNameIndex],ackKey,routerOTACommand,tsNetBuffer); // sample: LTEG^REC'D_~ROUTER~RESET~24/07/08,10:30:00
      } 
      else sprintf(sendAck, "%s%s", flashLoggerName.sensorNameList[routerNameIndex],ackKey);
      sendThruLoRa(sendAck);

    } else {
      // ano gusto mo gawin sa rejected na transmissions ?
    }
    endCount = 0;                               // reset coutner 'endCount' before  (re)counting
    for (int endCheck = 0; endCheck <= routerCount; endCheck++) {
      endCount = endCount + endCounter[endCheck];
      // debugPrint("endCount: ");
      // debugPrintln(endCount);
      // debugPrintln(endCounter[endCheck]);
    }
    // debugPrint("Volt count: ");
    // debugPrintln(endCount);
    // debugPrint("Router count: ");
    // debugPrintln(routerCount);

    if (routerCount == endCount) {
      debugPrintln("Router check count complete.."); 
      byte eCount = 0;
      for (int eIndex = 0; eIndex <= routerCount;eIndex++) endCounter[eIndex]=0;
      debugPrint("Volt count: ");
      debugPrintln(endCount);
      debugPrint("end string count: ");
      debugPrintln(endCount);
      break;
    }
    // else () {  // deal with junk here
    // do something with the strings that didn't get through the filter?
    // } 
    resetWatchdog();
  }

  routerOTAflag = false;      // reset router OTA flag
  for (int rO=0;rO<sizeof(routerOTACommand);rO++) routerOTACommand[rO]=0x00; // clears global router OTA container
  
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
  
  if (savedDataLoggerMode.read() == ROUTERMODE) {  //  add other router modes here
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

void broadcastLoRaKey(int pingInterval, unsigned long broadcastDuration) {
      resetWatchdog();
      int pingCounter = 0;
      debugPrint("broadcast LoRa key at specified interval (milliseconds):");
      debugPrint(pingInterval);
      // pingInterval = Serial.parseInt();
      Serial.println(pingInterval);
      char pingMsgBuffer[100];
      char pingReply[100];
      unsigned long broadcastStart = millis();
      sprintf(pingMsgBuffer, "CMD*");                                           // 
      for (int RCount = 1; RCount <= savedRouterCount.read(); RCount ++ ) {     // iterte through all saved router names
        strcat(pingMsgBuffer, LBTKey);                                        
        pingMsgBuffer[strlen(pingMsgBuffer)] = 0x00;
        strncat(pingMsgBuffer, flashLoggerName.sensorNameList[RCount], strlen(flashLoggerName.sensorNameList[RCount]));
        pingMsgBuffer[strlen(pingMsgBuffer)] = 0x00;
        strcat(pingMsgBuffer, ";");                   // sub command delimiter; any symbol can be used except '*'
        pingMsgBuffer[strlen(pingMsgBuffer)] = 0x00;
      }
      strcat(pingMsgBuffer, "*");                     // end delimiter here  
      pingMsgBuffer[strlen(pingMsgBuffer)] = 0x00;    // term
      getTimeStamp(_timestamp, sizeof(_timestamp));   // update global timstamp variable holder
      strcat(pingMsgBuffer, _timestamp);              // include timestamp here for time update for dataloggers
      pingMsgBuffer[strlen(pingMsgBuffer)] = 0x00;

      while(millis() - broadcastStart < broadcastDuration) {
        // sprintf(pingMsgBuffer, "START:WEBTA %d", pingCounter);
        sendThruLoRa(pingMsgBuffer);
        LEDSend();
        // delayMillis(pingInterval);
        // if (receiveLoRaData(pingReply, sizeof(pingReply), pingInterval)) debugPrintln(pingReply);
        pingCounter++;
      }
}

///  Single instance of listem mode scan; this just keep repeating
///  Should be followed by sleep [shot sleep duation]
///  Normal sleep should be replaced with watchdog sleep
///  @param scanDuration scan duration [in milliseconds] for checking channel activity
///  @param keywordWaitDuation wait duration [in milliseconds] to receive a valid keyword after channel activity has been detected
void scanForCommand(int scanDuration, int keywordWaitDuration) {  // 1000,2000, 6000
  resetWatchdog();
  char receiveBuffer[500];
  int scanTimeout = scanDuration;
  unsigned long scanStart = 0;
  bool channelActivity = false;
  for (int rb = 0; rb < sizeof(receiveBuffer); rb ++) receiveBuffer[rb] = 0x00;
  // put your main code here, to run repeatedly:
  scanStart = millis();
  // while(millis() - scanStart < scanTimeout && !channelActivity) {  // scan timeout: 2sec
  while(millis() - scanStart < scanTimeout) {  // scan timeout: 2sec
    LEDOn();
    // rf95.init();
    if (rf95.isChannelActive()) {
      LEDReceive();
      channelActivity = true;
      debugPrintln("Channel activity detected");
      }
    // rf95.waitCAD();
    else debugPrintln("Channel empty.."); // end here if no broadcast detected; then restart scan loop
    delayMillis(random(50,200));         // add small random delay to break synchronicity
    // debugPrintln("Channel active");
      // debugPrintln("False");
    LEDOff();
  
    if (channelActivity) {
      channelActivity = false;
      debugPrint("Checking listen key: ");
      debugPrintln(listenKey);

      receiveLoRaData(receiveBuffer, sizeof(receiveBuffer), keywordWaitDuration);                           //wait for keyword transmission if channel activity is detected 
      if (inputHas(receiveBuffer,listenKey))  {
        debugPrint("Router wake command: ");   // continue processing if keyword is found
        debugPrintln(receiveBuffer); 
          char *receiveBufferTS = strtok(receiveBuffer, "*");
          byte receiveTsIndex = 0;
          while (receiveBufferTS != NULL) {
            if (receiveTsIndex==2) {              // timestamp should be found on the 3rd segment (index 2) (CMD*START;XXX*000000000000)
              if (!checkTimeSync(receiveBufferTS)) updateTsDataFormat(receiveBufferTS);     // if this works, save it on a variable instead of executing here
              else debugPrintln("Time sync OK (±2min)..");
              break;
            }
            receiveBufferTS = strtok(NULL, "*");
            receiveTsIndex++;
          }
        // get timestamp from command string
        // compare current timestap and ts from cmd string      
        // update timestamp if neccessary
        debugPrintln("Operation Start");
        operationFlag = true;   // trigger operation function
        break;
      }
      else debugPrintln("No valid command found.");                                                         // else gate; if transmission is not relevant return to sleep or scan?          
    }
  }
  resetWatchdog();
}

void updateListenKey() {
  flashLoggerName = savedLoggerName.read();
  sprintf(listenKey, "%s%s",LBTKey, flashLoggerName.sensorNameList[0]);
}