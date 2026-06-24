float parseVoltage(char* stringToParse, int stringContainerSize);

const char ackKey[] = "^REC'D_";
const char LBTKey[] = "START:";
char _routerOTA[100];
char listenKey[50];

void updateListenKey() {
  char dlogName[10];
  getNameFromList(0, dlogName);
  sprintf(listenKey, "%s%s",LBTKey, dlogName);
  // rf95.setPreambleLength();
}

void initializeLORA(byte RST_PIN) {                           
  SPI.begin();
  delayMillis(1000);                                          //  wait must be greater than 10ms before init
  pinMode(VSPI_RST, OUTPUT);                                  //  set gpio mode for LoRa RST pin
  digitalWrite(VSPI_RST, HIGH);       
  delayMillis(1000);
  digitalWrite(VSPI_RST, LOW);                                //  pull RST low for >100us for manual reset; this may be useful after brownouts or re-initialization
  delayMillis(500);                 
  digitalWrite(VSPI_RST, HIGH);
  delayMillis(100);                                           // wait for >5ms before any operations

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

  debugPrint("Sending: ");
  debugPrintln(radioPacket);

  uint8_t packetLen = strlen(radioPacket);

  if (!rf98.send((uint8_t *)radioPacket, packetLen))
    debugPrint("Radio packet sending failed");
  else
    debugPrint("Radio packet sent");

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

    if (strlen(receiveContainer)) {
      waitDataFlag = false;
      receiveStatus = true;
    }
  }

  rf98.setModeIdle();
  return receiveStatus;
}

void waitForLoRaRouterData(unsigned long receiverWaitDuration, int routerCount, uint8_t receiveMode) {
  char gatewayDataDump[200];
  char loRaBuffer[1000];
  uint8_t routerNameIndex = 100;
  uint8_t nameIndexLimit = 100;
  char sendAck[200];
  int RSSIbuffer = 0;
  int endCounter[routerCount+1];
  int RSSIContainer[routerCount+1];
  float voltContainer[routerCount+1];
  int endCount = 0;
  char nameBuffer[10];

  for (int e = 0; e <= routerCount;e++) endCounter[e]=0;
  for (int r = 0; r <= routerCount;r++) RSSIContainer[r]=0;
  for (int v = 0; v <= routerCount;v++) voltContainer[v]=0;

  bool voltFlag = false;

  debugPrint("Router count: ");
  debugPrintln(fetchParam(paramStorage, ROUTER_COUNT, (uint8_t)0));

  debugPrintln("Listed router(s): ");
  for (int routerPos=1; routerPos <= fetchParam(paramStorage, ROUTER_COUNT, (uint8_t)0); routerPos++) {
    getNameFromList(routerPos, nameBuffer);
    debugPrintln(nameBuffer);
  }

  debugPrintln("");
  debugPrint("Wait time limit: ");
  debugPrint(receiverWaitDuration/1000/60);
  debugPrintln(" minute(s)");
  debugPrintln("Waiting for transmission... ");

  unsigned long routerWaitStart = millis();

  loadNameArrayFromStorage(false);

  while (millis() - routerWaitStart < receiverWaitDuration) {
    for (int lb = 0; lb < sizeof(loRaBuffer); lb++) loRaBuffer[lb]=0x00;

    debugPrintln("~");
    receiveLoRaData(loRaBuffer, sizeof(loRaBuffer), 30000);
    debugPrintln("~");

    if (receiveMode == 0 || receiveMode == 1)
      routerNameIndex = loRaFilterPass(loRaBuffer, sizeof(loRaBuffer));

    if (routerNameIndex > 0 && routerNameIndex < nameIndexLimit ) {

      debugPrint("Received data from ");
      debugPrintln(dataloggerNameList[routerNameIndex]);

      debugPrint("Router index: ");
      debugPrintln(routerNameIndex);

      debugPrintln(loRaBuffer);

      RSSIbuffer = rf98.lastRssi();

      if (RSSIbuffer > -100)
        RSSIContainer[routerNameIndex] = RSSIbuffer;

      debugPrint("Signal Loss: "); 
      debugPrintln(RSSIbuffer);

      RSSIbuffer = 0;

      if (inputHas(loRaBuffer, "*VOLT")) {
        voltFlag = true;

        debugPrintln("Router info:");

        voltContainer[routerNameIndex] = parseVoltage(loRaBuffer, sizeof(loRaBuffer));

        debugPrint("Supply voltage: ");
        debugPrintln(voltContainer[routerNameIndex]);

      }
      else {
        if (receiveMode == 0)
          addToSMSStack(loRaBuffer);
      }

      // mark complete ONLY when final packet arrives
      // char endTag[32];
      // sprintf(endTag, "%sW,", dataloggerNameList[routerNameIndex]);

      // if (inputHas(loRaBuffer, endTag)) {
      //   endCounter[routerNameIndex] = 1;
      // }

      if (voltFlag && routerOTAflag) {
        voltFlag = false;

        char tsNetBuffer[50];
        getNetworkFormatTimeStamp(tsNetBuffer, sizeof(tsNetBuffer));

        sprintf(sendAck, "%s%s~ROUTER~%s~%s",
          dataloggerNameList[routerNameIndex],
          ackKey,
          routerOTACommand,
          tsNetBuffer);

      } else {
        sprintf(sendAck, "%s%s", dataloggerNameList[routerNameIndex], ackKey);
      }

      sendThruLoRa(sendAck);
    }

    endCount = 0;

    for (int endCheck = 0; endCheck <= routerCount; endCheck++) {
      endCount += endCounter[endCheck];
    }

    if (routerCount == endCount) {
      debugPrintln("Router check count complete..");

      for (int eIndex = 0; eIndex <= routerCount;eIndex++)
        endCounter[eIndex]=0;

      debugPrint("Volt count: ");
      debugPrintln(endCount);

      debugPrint("end string count: ");
      debugPrintln(endCount);

      break;
    }
  }

  routerOTAflag = false;

  for (int rO=0;rO<sizeof(routerOTACommand);rO++)
    routerOTACommand[rO]=0x00;

  if (receiveMode == 0) {
    char numBuffer[10];

    strcpy(gatewayDataDump, "GATEWAY*RSSI,");
    strncat(gatewayDataDump, dataloggerNameList[0], 3);
    strcat(gatewayDataDump, ",");

    for (byte rCount = 1; rCount <= routerCount; rCount++) {
      strcat(gatewayDataDump, dataloggerNameList[rCount]);
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

int loRaFilterPass(char* payloadToCheck, int sizeOfPayload) {
  char payloadBuffer[sizeOfPayload+1];

  loadNameArrayFromStorage(true);
  sprintf(payloadBuffer, payloadToCheck);

  for (byte rIndex = 0; rIndex <= fetchParam(paramStorage, ROUTER_COUNT, (uint8_t)0); rIndex++) {
    if (inputHas(payloadBuffer, dataloggerNameList[rIndex]))
      return rIndex;
  }

  return MAX_ROUTER_COUNT+99;
}

bool sendThruLoRaWithAck(const char* payloadToSend, uint32_t responseWaitTime, uint8_t retryCount) {
  char ackResponseBuffer[300];
  char validResponse[30];
  bool noResponse = true;
  
  loadNameArrayFromStorage(true);

  sprintf(validResponse, "%s%s",dataloggerNameList[0],ackKey);
  
  for (int retryIndex = 0; retryIndex <= retryCount; retryIndex++) {
    sendThruLoRa(payloadToSend);

    unsigned long ackWaitStart = millis();

    while (millis() - ackWaitStart < responseWaitTime && noResponse) {
      debugPrintln("Checking response..");

      receiveLoRaData(ackResponseBuffer, sizeof(ackResponseBuffer), responseWaitTime);

      debugPrint(ackResponseBuffer);

      if (inputHas(ackResponseBuffer, validResponse)) {
        debugPrintln(" << acknowledged by gateway");

        if (inputHas(ackResponseBuffer, "~ROUTER~")) {
          routerProcessOTAflag = true;
          sprintf(routerOTACommand, ackResponseBuffer);
        }

        noResponse = false;
        break;
      } else {
        debugPrintln("");
      }
    }

    if (!noResponse) break;
  }

  return !noResponse;
}

void disableModems() {
  // Disable Wi-Fi
  WiFi.disconnect(true);   // disconnect and erase config
  WiFi.mode(WIFI_OFF);     // turn off Wi-Fi hardware

  // Disable Bluetooth
  btStop();                // stops Classic BT
  esp_bt_controller_disable(); // disables BT controller
}