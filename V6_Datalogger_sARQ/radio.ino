const char ackKey[] = "^REC'D_";
const char LBTKey[] = "START:";
char _routerOTA[100];
char listenKey[50];

void updateListenKey() {
  EEPROM.get(DATALOGGER_NAME, flashLoggerName);
  sprintf(listenKey, "%s%s",LBTKey, flashLoggerName.sensorNameList[0]);
  // rf95.setPreambleLength();
}

void initializeLORA(byte RST_PIN) {
  SPI.begin();
  delayMillis(1000);                // wait must be greater than 10ms before init
  pinMode(VSPI_RST, OUTPUT);
  digitalWrite(VSPI_RST, HIGH);
  delayMillis(1000);
  digitalWrite(VSPI_RST, LOW);       // pull RST low for >100us for manual reset
  delayMillis(500);                 
  digitalWrite(VSPI_RST, HIGH);
  delayMillis(100);                 // wait for >5ms before any operations  

  if (rf98.init()) debugPrintln("LoRa initialization OK!");
  else debugPrintln("LoRa initialization failed");
  if (rf98.setFrequency(RF98_FREQ)) {
    debugPrint("LoRa frequncy set to ");
    debugPrintln(RF98_FREQ);
  } else debugPrint("Frequncy set error");
  rf98.setTxPower(23, false); 
}

void sendThruLoRa(const char *radioPacket) {
  rf98.setModeRx();
  Serial.print("Sending: "); Serial.println(radioPacket);
  if (!rf98.send((uint8_t *)radioPacket, 20)) debugPrint("Radio packet sending failed");
  else debugPrint("Radio packet sent");
  delayMillis(10);
  rf98.waitPacketSent();
  rf98.setModeIdle();
}

bool receiveLoRaData(char* receiveContainer, uint16_t receiveContainerSize, unsigned long waitDuration) {
  bool receiveStatus = false;
  uint8_t receiveBuffer[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t bufferLength = sizeof(receiveBuffer);
  bool waitDataFlag = true;
  unsigned long LoRaWaitStart = millis();
  
  for (int i = 0; i < receiveContainerSize; i++) receiveContainer[i]=0x00;
  // char payloadBuffer[RH_RF95_MAX_MESSAGE_LEN+1];
  rf98.setModeRx();
  while (millis() - LoRaWaitStart < waitDuration && waitDataFlag) {
    if (rf98.available()) {
      if (rf98.recv(receiveBuffer, &bufferLength)) {
        int l = 0;
        for (l = 0; l < bufferLength; ++l) {
          receiveContainer[l] = (uint8_t)receiveBuffer[l];
        }
        receiveContainer[l] = 0x00;
      }
    }
    if (strlen(receiveContainer)) {waitDataFlag = false; receiveStatus=true;}
  }
  rf98.setModeIdle();
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
  debugPrintln(EEPROM.readByte(ROUTER_COUNT));
  debugPrintln("Listed router(s): ");
  for (int routerPos=1; routerPos <= EEPROM.readByte(ROUTER_COUNT); routerPos++) debugPrintln(flashLoggerName.sensorNameList[routerPos]);
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
      // LEDReceive();
      debugPrint("Received data from ");
      debugPrintln(flashLoggerName.sensorNameList[routerNameIndex]);
      debugPrint("Router index: ");
      debugPrintln(routerNameIndex);
      debugPrintln(loRaBuffer);
      RSSIbuffer = rf98.lastRssi();
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
}

int loRaFilterPass(char* payloadToCheck, int sizeOfPayload) {
  uint8_t payloadType = 0;
  char payloadBuffer[sizeOfPayload+1];

  sprintf(payloadBuffer, payloadToCheck);

  // remove first ">>" characters
  // tokenize using "*" as delimiter to get the name?

  for (byte rIndex = 0; rIndex <= EEPROM.readByte(ROUTER_COUNT); rIndex++) {
    if (inputHas(payloadBuffer, flashLoggerName.sensorNameList[rIndex])) return rIndex; // returns index of datalogger name
  }
  return MAX_ROUTER_COUNT+99;    
  //  return any value higher than the max router count
  //  default return value unless router name is found on sensorNameList
  
}

bool sendThruLoRaWithAck(const char* payloadToSend, uint32_t responseWaitTime, uint8_t retryCount) {
  char ackResponseBuffer[300];
  char validResponse[30];
  bool noResponse = true;
  sprintf(validResponse, "%s%s",flashLoggerName.sensorNameList[0],ackKey);
  
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