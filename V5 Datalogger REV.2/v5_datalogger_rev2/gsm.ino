void GSMINTInit(uint8_t GSMpin) {
  pinMode(GSMpin, INPUT);
  REG_EIC_INTFLAG = EIC_INTFLAG_EXTINT2;    // clears interrupt flag
  attachInterrupt(digitalPinToInterrupt(GSMpin), GSMISR, FALLING);
}

void GSMISR(){
  if (savedGSMPowerMode.read() != 2) GSMIntFlag=true;
}

void GSMOff(){
  resetWatchdog();
  debugPrintln("Turning OFF GSM module..");
  digitalWrite(GSMPWR, LOW);
  delayMillis(500);
  resetWatchdog();
}

void GSMOn(){
  resetWatchdog();
  debugPrintln("Turning ON GSM module..");
  digitalWrite(GSMPWR, HIGH);
  delayMillis(500);
  resetWatchdog();
}

bool GSMInit() {
  resetWatchdog();
  bool initOK = false;
  char GSMLine[500];
  bool gsmSerial = false;
  bool GSMconfig = false;
  bool signalCOPS = false;
  unsigned long initStart = millis();
  int initTimeout = 25000;
  uint8_t errorCount = 0;

  debugPrintln("Connecting GSM to network...");

  detachInterrupt(digitalPinToInterrupt(GSMINT));
  GSMSerial.begin(GSMBAUDRATE);
  delayMillis(300);
  
    /* Assign pins 10 & 11 UART SERCOM functionality */
  pinPeripheral(10, PIO_SERCOM);
  pinPeripheral(11, PIO_SERCOM);

  pinMode(GSMPWR, OUTPUT);
  pinMode(GSMRST, OUTPUT);
  pinMode(GSMINT, INPUT);
  digitalWrite(GSMPWR, HIGH);
  digitalWrite(GSMRST, HIGH);
  unsigned long gsmPowerOn = millis();
  char gsmInitResponse[100];
  do {
    resetWatchdog();
    GSMAnswer(gsmInitResponse, 100);  
    if (strlen(gsmInitResponse) > 0 && gsmInitResponse != "\n") debugPrintln(gsmInitResponse);
    delayMillis(300);
    resetWatchdog();
  } while (millis() - gsmPowerOn < 5000);
   
  while (!gsmSerial || !GSMconfig || !signalCOPS ) {  //include timeout later
    resetWatchdog();
    if (!gsmSerial) GSMSerial.write("AT\r");
    if (GSMWaitResponse("OK", 1000, true) && !gsmSerial) { 
      if (Serial) Serial.println("Serial comm ready!");
      GSMSerial.write("ATE0\r");
      // GSMSerial.write("AT&W_SAVE\r");
      gsmSerial = true;
    } else errorCount++;
    if (gsmSerial) {
      GSMSerial.flush();
      delayMillis(500);
      GSMSerial.write("AT+COPS=0,1;+CMGF=1;+IPR=0");
      GSMSerial.write("\r");
    }
    if (gsmSerial && !GSMconfig && GSMWaitResponse("OK", 5000, true)) {
      resetWatchdog();
      GSMSerial.flush();
      delayMillis(500);
      GSMSerial.write("AT+COPS=0,1;+CMGF=1;+IPR=0");
      GSMSerial.write("\r");
      GSMSerial.write("AT+CNMI=0,0,0,0,0\r");
      if (Serial) Serial.println("GSM module config OK!");
      GSMconfig = true;
    }
    if (GSMconfig) {
    GSMSerial.flush();
    if (Serial) Serial.println("Checking GSM network signal..");
    GSMSerial.write("AT+COPS?\r");
    delayMillis(1000);
    }
    if (GSMconfig && !signalCOPS && GSMGetResponse(gsmInitResponse, sizeof(gsmInitResponse), "+COPS: 0,1,\"", 3000)) {
      resetWatchdog();
      char CSQval[10];
      if (readCSQ(CSQval)) {
        // if (Serial) Serial.print("Checking GSM network signal..");
        // if (Serial) Serial.print("CSQ: ");
        // if (Serial) Serial.println(CSQval);
        signalCOPS = true;
      }
    }
    if (gsmSerial && GSMconfig && signalCOPS) {
      GSMSerial.write("AT&W\r");
      debugPrintln("");
      // if (Serial) Serial.println("GSM READY");
      // debugPrintln("");
      delayMillis(500);
      REG_EIC_INTFLAG = EIC_INTFLAG_EXTINT2;    // clears interrupt flag
      attachInterrupt(digitalPinToInterrupt(GSMINT), GSMISR, FALLING);
      initOK = true;
      break;
    }
    if (errorCount == 5) {
      debugPrintln("CHECK_GSM_SERIAL_OR_POWER");
      break;
    }
    if (millis() - initStart > initTimeout) {
      debugPrint("GSM_INIT: ");
      if (!GSMconfig) {
        debugPrintln("GSM_MODULE_CONFIG_ERROR");
      } else if (!signalCOPS) {
        debugPrintln("NETWORK_OR_SIM_ERROR");
      }
      break;
    }
    resetWatchdog();
  }
  GSMSerial.flush();
  resetWatchdog();
  return initOK;
}

bool sendThruGSM(const char* messageToSend, const char* serverNumber) {
  resetWatchdog();
  bool sentFlag = false;
  char GSMresponse[100];
  char messageContainer[1000];
  char CMGSContainer[50];
  char serverBuffer[20];
  strcpy(serverBuffer, serverNumber);
  strcpy(CMGSContainer, serverNumber);
  
  debugPrint("Send to ");
  if (strlen(CMGSContainer) == 11 || strlen(CMGSContainer) == 13)  {  //crude check for a valid number
    sprintf(CMGSContainer, "AT+CMGS=\"%s\"\r", serverBuffer);
    checkSender(serverBuffer);
    debugPrint(serverBuffer);
  } else {
    sprintf(CMGSContainer, "AT+CMGS=\"%s\"\r", defaultServerNumber);  // uses default server number if current is invalid
    debugPrint(defaultServerNumber);
  }
  
  sprintf(messageContainer, "%s", messageToSend);

  debugPrint(": ");
  debugPrintln(messageContainer);

  GSMSerial.write("AT\r");                                                                     
  if (GSMWaitResponse("OK",1000, false)) {             // Checks if GSM serial is accessible
    resetWatchdog();
    GSMSerial.write(CMGSContainer);
    if (GSMWaitResponse(">",5000, true)) {
      resetWatchdog();
      GSMSerial.write(messageToSend);
      delayMillis(500);
      GSMSerial.write(26);
    } else {
      debugPrintln("GSM module unable to send");
      GSMSerial.write(27);
      GSMSerial.write(27);
    }
    if (GSMWaitResponse("+CMGS",30000, true)) {
      resetWatchdog();
      LEDSend();
      sentFlag = true;
      // debugPrintln("Message send indentifier!");
      return sentFlag;
    } else {
      resetWatchdog();
      delayMillis(5000);
      debugPrintln("Sending failed");
      GSMSerial.write(27);  //crude escape
      GSMSerial.write(27);
      GSMSerial.write(27);
      GSMSerial.flush();
      // responive but cannot send
    }
  } else {  //GSM serial is not available
    resetWatchdog();
    debugPrintln("GSM module error.");
    //insert GSM reset function here
    // GSMSerial.write(messageContainer);
  }
  // Serial.println("Send end");
  resetWatchdog();
  return sentFlag;
}

void sendSMSDump(const char* messageDelimilter, const char* dumpServer) {
  resetWatchdog();
  char tokenBuffer[1000];
  int retryCount = 0;

  char * sendToken = strtok(_globalSMSDump, messageDelimilter);
  while (sendToken != NULL) {
    resetWatchdog();  
    sprintf(tokenBuffer, "%s", sendToken);
    // debugPrintln(_globalSMSDump);
    if (loggerWithGSM(savedDataLoggerMode.read())) {  // send thru GSM
      if (sendThruGSM(tokenBuffer, dumpServer)) {
      // LEDSend();
        resetWatchdog();
        delayMillis(random(5000,10000));
        resetWatchdog();
        debugPrintln("Message segment sent");
      } else {
        retryCount++;
        GSMReset();
        debugPrintln("Retrying..");
        delayMillis(5000);
        if (sendThruGSM(tokenBuffer, dumpServer)) {
          // LEDSend();
          debugPrintln("Message segment sent");
        } else {
          debugPrintln("Retry failed");
        }
      }
    } else {  // send thru LORA
      if (sendThruLoRaWithAck(tokenBuffer,random(1000,3000),3))  {
        resetWatchdog();
        LEDSend();
        debugPrintln("Message segment acknowledged");  // send thru LORA
        resetWatchdog();
        delayMillis(random(3000,8000));
        resetWatchdog();
      }
      else {
        debugPrintln("No valid response received");
        debugPrintln("");
      }
    }
    sendToken = strtok(NULL,messageDelimilter);
    resetWatchdog();
  }
  resetWatchdog();
  // in case of 
}

void GSMAnswer(char* responseBuffer, int bufferLength) {
  resetWatchdog();
  int bufferIndex = 0;
  unsigned long waitStart = millis();

  for (int i = 0; i < bufferLength; i++){
    responseBuffer[i] = 0x00;
  }
  for (int j = 0; j < bufferLength && GSMSerial.available() > 0 ; j++) {
    resetWatchdog();
    responseBuffer[j] = GSMSerial.read();
  }
  if (strlen(responseBuffer) > 0) debugPrintln(responseBuffer);
  if (strlen(responseBuffer) >= bufferLength) {
    responseBuffer[bufferLength-1] = 0x00;
  } else { responseBuffer[strlen(responseBuffer)]=0x00; }
  resetWatchdog();
}

// Checks command response from GSM serial.
// Returns TRUE then correct response is received
bool GSMWaitResponse(const char* targetResponse, int waitDuration, bool showResponse) {
  resetWatchdog();
  bool responseValid = false;
  char toCheck[50];
  char charBuffer;
  char responseBuffer[300];
  unsigned long waitStart = millis();
  strcpy(toCheck, targetResponse);
  toCheck[strlen(toCheck)] = 0x00;

  do {
    resetWatchdog();
      for (int i = 0; i < sizeof(responseBuffer); i++){
        responseBuffer[i] = 0x00;
      }
      for (int j = 0; j < sizeof(responseBuffer) && GSMSerial.available() > 0 ; j++) {
        charBuffer = GSMSerial.read();
        if (charBuffer == '\n' || charBuffer == '\r') {
          responseBuffer[j]=0x00;
          break;
        } else {
          responseBuffer[j] = charBuffer;
        }
      }

      if (strlen(responseBuffer) > 0 && responseBuffer != "\n") {
        if (showResponse) debugPrintln(responseBuffer);
        if (strstr(responseBuffer, toCheck)) {
          // debugPrintln(responseBuffer);
          // debugPrintln(toCheck);
          responseValid = true;
          break;
        }
      }
    resetWatchdog();
    delayMillis(50);
  } while (!responseValid && millis() - waitStart < waitDuration);
  // debugPrintln("function end");
  resetWatchdog();
  return responseValid;
}

// 
bool GSMGetResponse(char* responseContainer, int containerLen, const char * responseReq, int getDuration) {
  resetWatchdog();

  char toCheck[50];
  unsigned long waitStart = millis();
  char charBuffer;
  strcpy(toCheck, responseReq);
  toCheck[strlen(toCheck)] = 0x00;

  do {
    resetWatchdog();
    delayMillis(300);
      for (int i = 0; i < containerLen; i++){
        responseContainer[i] = 0x00;
      }
      for (int j = 0; j < containerLen && GSMSerial.available() > 0 ; j++) {
        charBuffer = GSMSerial.read();
        if (responseContainer[j] == '\n' || responseContainer[j] == '\r') {
          break;
        } else {
          responseContainer[j] = charBuffer;
        }

        
      }
      if (strlen(responseContainer) == containerLen) {
        responseContainer[containerLen-1] = 0x00;
      } else { responseContainer[strlen(responseContainer)]=0x00; }

      if (strlen(responseContainer) > 0) {
        // debugPrintln(responseContainer);
        if (strstr(responseContainer, toCheck)) {
          // debugPrintln("valid response found");
          return true;
          // break;
        }
      }
    resetWatchdog();
  } while (millis() - waitStart < getDuration);
  resetWatchdog();
  return false;
}

void textMode() {   // experimental
  resetWatchdog();
  char sendLine[500];
  char receiveLine[500];
  char bufferChar;
  bool _textMode = true;
  GSMSerial.flush();
  GSMSerial.write("AT+CNMI=1,2,0,0,0\r");
  while (_textMode) {
    resetWatchdog();
    for (int j = 0; j < 500; j++){
        sendLine[j] = 0x00;
        receiveLine[j] = 0x00;
    }
    if (Serial.available() > 0 ) {
      for (int j = 0; j < 500; j++) {
        bufferChar = Serial.read();
        // sendLine[j] = Serial.read();
        if (bufferChar == '\n' || bufferChar == '\r') {
          break;
        } else {
          sendLine[j] = bufferChar;
        }
      }
      if (strlen(sendLine) >= 500) {
        sendLine[500-1] = 0x00;
      } else {
        sendLine[strlen(sendLine)]=0x00;
      }
    }
    if (GSMSerial.available() > 0 ) {
      for (int k = 0; k < 500; k++) {
        bufferChar = GSMSerial.read();
        // receiveLine[k] = GSMSerial.read();
        if (bufferChar == '\n' || bufferChar == '\r') {
          break;
        } else {
          receiveLine[k] = bufferChar;
        }
        
      }
      if (strlen(receiveLine) >= 500) {
        receiveLine[500-1] = 0x00;
      } else {
        receiveLine[strlen(receiveLine)]=0x00;
      }
    }
    if (strlen(receiveLine) > 0) {
      if (strstr(receiveLine,"+CMT: ")){
          LEDReceive();
          char senderBuf[20];
          char *cmtBuf;
          cmtBuf = strtok(receiveLine, ": ");
          cmtBuf = strtok(NULL, ",");
          debugPrint("From ");
          sprintf(senderBuf, cmtBuf);
          checkSender(senderBuf);
          debugPrint(senderBuf);
          debugPrint(" : ");
      } else debugPrintln(receiveLine);    // execute parsing here
      
    }
    if (inputIs(sendLine,"EXIT")) {
      resetWatchdog();
      GSMSerial.write("AT+CNMI=0,0,0,0,0\r");
      delayMillis(1000);
      debugPrintln("Exiting text mode");
      _textMode = false;
    } else if (strstr(sendLine,">>")) {

      char *stack_token = strtok(sendLine, ">>");
      int tokenIndex = 0;
      char serverNumber[15];
      char messageContainer[500];
      while (stack_token != NULL) {
        if (tokenIndex == 0) {
          strcpy(serverNumber, stack_token);
          checkServerNumber(serverNumber);
          serverNumber[strlen(serverNumber)] = 0x00;
        } else if (tokenIndex == 1) {
          strcpy(messageContainer, stack_token);
          messageContainer[strlen(messageContainer)] = 0x00;
        }
        tokenIndex++;
       stack_token = strtok(NULL, ">>");
      }
      if (strlen(serverNumber) == 11 || strlen(serverNumber) == 13) {     ///temporary filter
        sendThruGSM(messageContainer, serverNumber);
      }

    } else if (strlen(sendLine) > 0) {
      debugPrintln(sendLine);
      // GSMSerial.write(sendLine);
    }
    delayMillis(500);
    resetWatchdog();
  }
  resetWatchdog();
}

int parseCSQ(char *buffer) {
  resetWatchdog();
  char *tmpBuf;
  tmpBuf = strtok(buffer, ": ");
  tmpBuf = strtok(NULL, ",");
  return (atoi(tmpBuf));
  resetWatchdog();
}

bool readCSQ(char * csqContainer) {
  resetWatchdog();
  bool responseValid = false;
  char csqSerialBuffer[50];

  if (loggerWithGSM(savedDataLoggerMode.read()) == false) return false;
  GSMSerial.write("AT\r");
  delayMillis(200);
  GSMSerial.flush();
  GSMSerial.write("AT+CSQ\r");
  delayMillis(1000);
  GSMAnswer(csqSerialBuffer, sizeof(csqSerialBuffer));
  sprintf(csqContainer, csqSerialBuffer);
  if (strstr(csqSerialBuffer,"+CSQ:")) responseValid = true;
  resetWatchdog();
  return responseValid;
}

void updateServerNumber() {
  resetWatchdog();
  int changeServerTimeout = 60000;
  char serverNumBuffer[15];
  char numBuf;
  int numberIndex = 0;
  unsigned long waitStart;

  waitStart = millis();
  debugPrintln("Enter new server number: ");
  while (millis() - waitStart < changeServerTimeout) {
    resetWatchdog();
    if (Serial.available() > 0 ) {
      numBuf = Serial.read();
      if ((numBuf =='\n' || numBuf == '\r') && numBuf > 0 && numberIndex < sizeof(serverNumBuffer)) {
        serverNumBuffer[numberIndex] = 0x00;
        break;
      } else {
        serverNumBuffer[numberIndex] = numBuf;
        numberIndex++;
        // waitStart = millis();
      }
    }
    resetWatchdog();
  }
  checkServerNumber(serverNumBuffer);
  serverNumBuffer[strlen(serverNumBuffer)]=0x00;
  strncpy(flashServerNumber.dataServer, serverNumBuffer, strlen(serverNumBuffer));
  savedServerNumber.write(flashServerNumber);
  debugPrint("Server number set to: ");
  debugPrintln(flashServerNumber.dataServer);
  resetWatchdog();
}

void testSendToServer() {
  resetWatchdog();
  char sendBuf;
  unsigned long sendWait = millis();
  int sendTestTimeout = 60000;
  int testSendIndex = 0;
  char testSendBuffer[200];
  flashLoggerName = savedLoggerName.read();

  while (millis() - sendWait < sendTestTimeout) {
    resetWatchdog();
    if (Serial.available() > 0 ) {
      sendBuf = Serial.read();
      if ((sendBuf =='\n' || sendBuf == '\r') && sendBuf > 0 && testSendIndex < sizeof(testSendBuffer)) {
        testSendBuffer[testSendIndex] = 0x00;
        break;
      } else {
        testSendBuffer[testSendIndex] = sendBuf;
        testSendIndex++;
        // waitStart = millis();
      }
    }
    resetWatchdog();
  }
  if (strlen(testSendBuffer) > 0 && loggerWithGSM(savedDataLoggerMode.read())) {
    flashServerNumber = savedServerNumber.read();
    if (sendThruGSM(testSendBuffer,flashServerNumber.dataServer)) debugPrintln("Message sent");
    else {GSMReset(); if (sendThruGSM(testSendBuffer,flashServerNumber.dataServer)) debugPrintln("Message sent");}
  } else {
    char loraSendBuffer[200];
    getTimeStamp(_timestamp, sizeof(_timestamp));
    sprintf(loraSendBuffer, ">>%s*%s*%s",flashLoggerName.sensorNameList[0],testSendBuffer,_timestamp);
    sendThruLoRa(loraSendBuffer);
  }
  resetWatchdog();
}


bool GSMReset() {
  resetWatchdog();
  if (Serial) Serial.println("Resetting the GSM module...");
  digitalWrite(GSMPWR, LOW);
  digitalWrite(GSMRST, LOW);
  delayMillis(3000);
  digitalWrite(GSMRST, HIGH);
  digitalWrite(GSMPWR, HIGH);
  delayMillis(2000);
  resetWatchdog();
  return GSMInit();
}

void addToSMSStack(const char* payloadToAdd) {
  resetWatchdog();
  char stackBuffer[500];
  sprintf(stackBuffer, payloadToAdd);
  // stackBuffer[strlen(stackBuffer)] = 0x00;
  
  // if filters
  if (strlen(stackBuffer) == 0) return;                 //  rejects zero-length data
  if (strstr(_globalSMSDump, stackBuffer)) return;            //  rejects duplicates

  // if editors
  if (loggerWithGSM(savedDataLoggerMode.read()) && (strstr(stackBuffer, ">>"))) {            //  offsets/removes the identifier ">>" before adding to stack (sent thru GSM)
    for (byte i = 0; i < strlen(stackBuffer); i++)  stackBuffer[i] = stackBuffer[i + 2];                
  }
  if (!loggerWithGSM(savedDataLoggerMode.read()) && !(strstr(stackBuffer, ">>"))) {          //  for routers: adds the identifier ">>" before adding to stack (sent thru LoRa)
    sprintf(stackBuffer, ">>%s",payloadToAdd);
  }
  
  if (strlen(_globalSMSDump) == 0) {                            //  prevent the delimiters from being added at the beginning of the container
    strcpy(_globalSMSDump, stackBuffer);
    debugPrintln("Data copied to container.");
  } else {
    strcat(_globalSMSDump, dumpDelimiter);
    strcat(_globalSMSDump, stackBuffer);
    debugPrintln("Data added to container.");
  }
  _globalSMSDump[strlen(_globalSMSDump)]=0x00;
  resetWatchdog();
}

void generateInfoMessage(char* infoContainer) {             // TESXXW,40.50,0.00,15.92,11,240415081500
  resetWatchdog();
  char csqBuffer[100];
  uint8_t CSQ = 0;
  char tsBuffer[13];
  float rainMultiplier = 1;
  float battVolt = readBatteryVoltage(savedBatteryType.read());
  if (readCSQ(csqBuffer)) { // if FALSE, CSQ remains 0
    CSQ = parseCSQ(csqBuffer);
  }
  getTimeStamp(tsBuffer, sizeof(tsBuffer));
  tsBuffer[12]=0x00;
  if (savedRainSendType.read()==0) {  // sends rain tip equivalent in mm
    if(savedRainCollectorType.read()==0)rainMultiplier = 0.5;
    else if (savedRainCollectorType.read()==1)rainMultiplier = 0.2;   
  }
  delayMillis(1000);
  sprintf(infoContainer,"%sW,%0.2f,%0.2f,%0.2f,%u,%s",
    flashLoggerName.sensorNameList[0],
    readRTCTemp(),
    _rainTips*rainMultiplier,
    battVolt,
    CSQ,
    tsBuffer);
  resetWatchdog();
}

void GSMPowerModeReset() {
  resetWatchdog();
  uint8_t powerMode = savedGSMPowerMode.read();
  if (powerMode == 2) {
    debugPrintln("Turning ON GSM module");
    digitalWrite(GSMPWR, HIGH);
    digitalWrite(GSMRST, HIGH);
    delayMillis(1000);
    GSMInit();
  } else if (powerMode == 1) {
    GSMSerial.write("AT\r");
    delayMillis(99);
    GSMSerial.flush();
    GSMSerial.write("AT+CSCLK=0\r");
    // GSMSerial.write("AT+CFUN=1,1\r");
    if (GSMWaitResponse,"OK",1000,true) debugPrintln("GSM is awake");
    else debugPrintln("GSM did NOT wake");  //error code
  } else {}  // default 
  resetWatchdog();  
}

void GSMPowerModeSet() {
  resetWatchdog();
  uint8_t powerMode = savedGSMPowerMode.read();
  if (powerMode == 2) {
      debugPrintln("Turning OFF GSM module");
      digitalWrite(GSMPWR, LOW);
      digitalWrite(GSMRST, LOW);
      delayMillis(1000);
  } else  if (powerMode == 1) {
    GSMSerial.write("AT+CSCLK=2\r");
    // GSMSerial.write("AT+CFUN=0\r");
    if (GSMWaitResponse,"OK",1000,true) debugPrintln("GSM auto sleep enabled");
    else debugPrintln("Failed to set GSM autosleep"); //error code
  } else {} // no powersavings; do nothing
  resetWatchdog();
}


// pwede pa ito ma-cleanup gamit ang multi-dimensional array
bool checkServerNumber(char * serverNumber) {
  resetWatchdog();
  char numberBuffer[20];
  sprintf(numberBuffer, "%s", serverNumber);
  

  if (inputIs(numberBuffer,"DAN")) sprintf(serverNumber,"%s","09762372823");
  else if (inputIs(numberBuffer,"DON")) sprintf(serverNumber,"%s","09179995183");
  else if (inputIs(numberBuffer,"GLOBE1")) sprintf(serverNumber,"%s","09175972526");
  else if (inputIs(numberBuffer,"GLOBE2")) sprintf(serverNumber,"%s","09175388301");
  else if (inputIs(numberBuffer,"KATE")) sprintf(serverNumber,"%s","09476873967");
  else if (inputIs(numberBuffer,"JAY")) sprintf(serverNumber,"%s","09451136212");
  else if (inputIs(numberBuffer,"JJ")) sprintf(serverNumber,"%s","09287706189");
  else if (inputIs(numberBuffer,"KENNEX")) sprintf(serverNumber,"%s","09293175812");
  else if (inputIs(numberBuffer,"KIM")) sprintf(serverNumber,"%s","09458057992");
  else if (inputIs(numberBuffer,"REYN")) sprintf(serverNumber,"%s","09669622726");
  else if (inputIs(numberBuffer,"SAM")) sprintf(serverNumber,"%s","09770452845");
  else if (inputIs(numberBuffer,"SMART1")) sprintf(serverNumber,"%s","09088125642");
  else if (inputIs(numberBuffer,"SMART2")) sprintf(serverNumber,"%s","09088125639");
  else if (inputIs(numberBuffer,"WEB")) sprintf(serverNumber,"%s","09053648335");
  else if (inputIs(numberBuffer,"CHI")) sprintf(serverNumber,"%s","09179995183");

  //  expects that the length is equal to usual server number lengths 09XXXXXXXXX (11) or 639XXXXXXXXX (12)
  //  otherwise the number will be replaced by the default server number
  if (strlen(serverNumber) == 11 || strlen(serverNumber) == 13) {   
    // do nothing yet?
  } else {
    sprintf(serverNumber,"%s","09175972526");
    debugPrintln("Defaulted to GLOBE1");
    return false;
  }
  return true;
  resetWatchdog();
} 

// pwede pa ito ma-cleanup gamit ang multi-dimensional array or dictionary-like function
void checkSender(char* senderNum) {
  resetWatchdog();
  char senderNumBuffer[20];
  sprintf(senderNumBuffer, "%s", senderNum);
  senderNumBuffer[strlen(senderNumBuffer)+1]=0x00;
  if(strstr(senderNumBuffer,"9762372823")) sprintf(senderNum,"%s","DAN");
  else if(strstr(senderNumBuffer,"9179995183")) sprintf(senderNum,"%s","DON");
  else if(strstr(senderNumBuffer,"9175972526")) sprintf(senderNum,"%s","GLOBE1");
  else if(strstr(senderNumBuffer,"9175388301")) sprintf(senderNum,"%s","GLOBE2");
  else if(strstr(senderNumBuffer,"9476873967")) sprintf(senderNum,"%s","KATE");
  else if(strstr(senderNumBuffer,"9451136212")) sprintf(senderNum,"%s","JAY");
  else if(strstr(senderNumBuffer,"9287706189")) sprintf(senderNum,"%s","JJ");
  else if(strstr(senderNumBuffer,"9293175812")) sprintf(senderNum,"%s","KENNEX");
  else if(strstr(senderNumBuffer,"9458057992")) sprintf(senderNum,"%s","KIM");
  else if(strstr(senderNumBuffer,"9770452845")) sprintf(senderNum,"%s","SAM");
  else if(strstr(senderNumBuffer,"9088125642")) sprintf(senderNum,"%s","SMART1");
  else if(strstr(senderNumBuffer,"9088125639")) sprintf(senderNum,"%s","SMART2");
  else if(strstr(senderNumBuffer,"9669622726")) sprintf(senderNum,"%s","REYN");
  else if(strstr(senderNumBuffer,"9053648335")) sprintf(senderNum,"%s","WEB");
  else if(strstr(senderNumBuffer,"9179995183")) sprintf(senderNum,"%s","CHI");
  resetWatchdog();
}

/// Deletes all SMS on SIM card
void deleteMessageInbox() {
  resetWatchdog();
  GSMSerial.flush();
  GSMSerial.write("AT+CMGDA=\"DEL ALL\"\r");
  delayMillis(500);
  resetWatchdog();
  if (GSMWaitResponse("OK", 15000, false)) if (Serial) Serial.println("Deleted all SMS from SIM");
  else {if (Serial) Serial.println("Delete SIM SMS failed");}
  resetWatchdog();
}

/// Attempts to extract sender and message body from the first SMS received.
/// Under most circumstances, this should work normally.
/// @param senderNum - container for sender number to be extracted
/// @param msgBody - container for message body to be extracted
/// @param msgTS - container for message timestamp to be extracted
void extractSMSdata(char *msgBody, char * senderNum, char* msgTS) {
  resetWatchdog();
  char OTALineBuffer[3000];
  char msgDate[50], msgTime[50];
  char OTASegmentPosBuffer[100];
  char OTAchar;
  bool findCMGL = false;
  uint8_t otaPos = 0;
  
  delayMillis(1000);
  GSMSerial.write("AT+CMGL=\"ALL\"\r");
  delayMillis(500);

  // fill array to prevent junk
  for (int c = 0; c < sizeof(OTALineBuffer); c++) OTALineBuffer[c]=0x00;

  while (GSMSerial.available() > 0 ) {
    OTAchar = GSMSerial.read();
    if (OTAchar == '\n' || OTAchar == '\r') {
      // replace with delimiter " to improve chances of winning
      if (otaPos < sizeof(OTALineBuffer)-2) OTALineBuffer[otaPos] = '\"';   
      otaPos++;
    } else {
        // hopefully ay hindi ito mapuno agad bago mag-reset ng position or matapos ang loop
      if (otaPos < sizeof(OTALineBuffer)-2) OTALineBuffer[otaPos] = OTAchar; 
      otaPos++; //  increment to fill buffer
    }
  }

  OTALineBuffer[strlen(OTALineBuffer)] = 0x00;

  // process entire gsm buffer line here
  uint8_t segmentPos = 0;
  debugPrintln(OTALineBuffer);
  char * otaSegment = strtok(OTALineBuffer, ",\"");
  while (otaSegment != NULL) {

    sprintf(OTASegmentPosBuffer,"%s",otaSegment);                                        // if inbox message "+CMGL: n" indicator is found, begin tracking segment position
    debugPrintln(OTASegmentPosBuffer);
    if (strstr(OTASegmentPosBuffer, "RING")) ringCounter++;
    if (strstr(OTASegmentPosBuffer, "+CMGL:") && !findCMGL) findCMGL = true;              // this stops comparing once CMGL is found
    if (segmentPos == 2 && findCMGL) sprintf(senderNum,"%s",otaSegment);                  // 2nd index from CMGL should be the sender's number
    if (segmentPos == 3 && findCMGL) sprintf(msgDate,"%s",otaSegment);
    if (segmentPos == 4 && findCMGL) sprintf(msgTime,"%s",otaSegment);
    if (segmentPos == 5 && findCMGL) sprintf(msgBody,"%s", otaSegment);                   // 5th index from CMGL should be the message body
    // add segmentPos for ts
    otaSegment = strtok(NULL,",\"");

    //  begin tracking message segment/token position here when +CMGL is found
    //  minsan ay may naiiwan na extra characters sa gsm buffer from previous commands
    //  kaya dito pa lang magsisimula and counting sa start ng actual output mulas sa "AT+CMGL" command
    if (findCMGL) segmentPos++;      
  }
 sprintf(msgTS,"%s,%s",msgDate,msgTime);
 resetWatchdog();
}


bool fetchSenderNumber(char * OTAServerContainer, char * lineToProcess) {
  resetWatchdog();
  char OTACommandBuffer[500];
  char OTASender[20];
  bool validServer = false;

  sprintf(OTACommandBuffer,"%s", lineToProcess);
  if (strstr(OTACommandBuffer, "+CMGL")) {

    char * OTASenderDetail = strtok(OTACommandBuffer, "+,\"");
    int OTADetailIndex = 0;
    while (OTASenderDetail != NULL) {
      OTADetailIndex++;
      if (OTADetailIndex==3 && strlen(OTASenderDetail) == 12) {
        sprintf(OTAServerContainer,"+%s", OTASenderDetail);
        // OTAServerContainer[strlen(OTAServerContainer)] = 0x00;
        debugPrint("OTA server: ");
        debugPrintln(OTAServerContainer);
        validServer = true;
      }
      OTASenderDetail = strtok(NULL,"+,\"");
    }
  }
  resetWatchdog();
  return validServer;
}


/// Checks for OTA commamd in the the FIRST message of the SIM inbox
/// Other SMS received together with the first message are deleted
void checkForOTAMessages() {
  resetWatchdog();
  char senderNum[100], messageBody[500], messageTs[50]; //clear these for cleaner displays  
  for (uint16_t m = 0; m < sizeof(messageBody); m++ ) messageBody[m]=0x00;
  for (uint16_t s = 0; s < sizeof(senderNum); s++ ) senderNum[s]=0x00;
  for (uint16_t t = 0; t < sizeof(messageTs); t++ ) messageTs[t]=0x00;
  extractSMSdata(messageBody, senderNum, messageTs);          //  Only the FIRST message in the inbox is checked for OTA commands to prevent overlaps and repeating commands
  messageBody[strlen(messageBody)+1] = 0x00;
  deleteMessageInbox();                                       //  Delete ALL messages in SIM inbox so its not executed again if something goes wrong
  debugPrintln(senderNum);      
  // Serial.println(strlen(senderNum));
  debugPrintln(messageBody);
  debugPrintln(messageTs);
  //  after running through the parts of the GSM buffer
  //  perform  simple check on acquired mobile number
  if (strlen(senderNum) == 13)  {                             //  expects a format of +639XXXXXXXXX of length 13 from mobile number
    //  process for OTA message here
    debugPrintln("Check OTA command functions here >>");
    findOTACommand(messageBody, senderNum, messageTs);        //  run through all known command to check for OTA command in incoming message body
  }
  resetWatchdog();
}

/// compares the SMS message body with known OTA keywords then executes the matching OTA command
/// @param OTALineCheck - message body to check for OTA keyword
/// @param OTASender - message sender
/// @param OTATimestamp - message timestamp from network
void findOTACommand(const char* OTALineCheck, const char * OTASender, const char * OTATimestamp) {
  resetWatchdog();
  char otaSenderBuf[20]; //might be needed later
  char otaString[strlen(OTALineCheck)+1];
  char OTAReply[1000];

  flashLoggerName = savedLoggerName.read();
  sprintf(otaString, "%s", OTALineCheck);
  sprintf(otaSenderBuf, "%s", OTASender);

  //  Defunct function for registering OTA sender number
  //  Sample: REGISTER:SENSLOPE:639954645704
  if (inputHas(otaString, "REGISTER:SENSLOPE:")) {
    resetWatchdog();
    sprintf(OTAReply,"%s: Registration is no longer needed; proceed with normal OTA commands.",flashLoggerName.sensorNameList[0]);
    sendThruGSM(OTAReply, OTASender);
  }

  // Option for checking saved parameters of the datalogger
  else if (inputIs(otaString, "CONFIG:SENSLOPE:")) {
    resetWatchdog();
    buidParamSMS(OTAReply);   
    sendThruGSM(OTAReply,OTASender); 
  }

  //  Simulates the periodic operation function depending on the set datalogger mode
  //  Sensor data will be sent to the OTA sender
  //  Sample: SENSORPOLL:SENSLOPE:
  else if (inputIs(otaString, "SENSORPOLL:SENSLOPE:")) {
    resetWatchdog();
    // sending_stack[0] = '\0';
    // get_Due_Data(savedDataloggerMode.read(), ota_sender);
    Operation(OTASender);
    sendThruGSM("Data sampling finished", OTASender);
  }

  // Reads rain collector type and stored rain tip value
  // Adding the parameter "CLEAR" resets the rain tip value to ZERO
  // Sample:    READRAINTIPS:SENSLOPE:           -->> This is read only
  //            READRAINTIPS:SENSLOPE:CLEAR      -->> This resets the tip count to ZERO          
  else if (inputHas(otaString, "READRAINTIPS:SENSLOPE:")) {
    resetWatchdog();
    char *_clrCmd = strtok(otaString + 21, ":");
    if (savedRainCollectorType.read() == 0) sprintf(OTAReply, "%s Collector type: Pronamic (0.5mm/tip)\nRain tips: %0.2f tips\nEquivalent: %0.2fmm",flashLoggerName.sensorNameList[0], _rainTips,(_rainTips*0.5));
    else if (savedRainCollectorType.read() == 1)  sprintf(OTAReply, "%s Collector type: Davis (0.2mm/tip)\nRain tips: %0.2f tips\nEquivalent: %0.2fmm",flashLoggerName.sensorNameList[0], _rainTips,(_rainTips*0.2));
    if (inputIs(_clrCmd,"CLEAR")) resetRainTips();
    sendThruGSM(OTAReply, OTASender);
  }

  //  Changes the datalogger mode
  //  CAUTION: Dataloggers changed to ROUTER MODE cannot receive OTA commands [yet]
  else if (inputHas(otaString, "MODECHANGE:SENSLOPE:")) {
    resetWatchdog();
    uint8_t newMode;
    char *_newMode = strtok(otaString + 19, ":");
    newMode = atoi(_newMode);
    if (savedDataLoggerMode.read() == newMode) return; //skips processing if datalogger mode will not change
    savedDataLoggerMode.write(newMode);
    delayMillis(100);
    sprintf(OTAReply,"%s datalogger mode changed to %d.",flashLoggerName.sensorNameList[0], savedDataLoggerMode.read());
    sendThruGSM(OTAReply, OTASender);
  }

  //  Replaces current/saved timestamp with the additional parameter after the keyword.
  //  Time format is YYYY,MM,DD,HH,MM,SS,dd.
  //  values for dd: Mon = 0; Tue = 1; Wed = 2...
  //  Sample: SETDATETIME:SENSLOPE:2024,02,23,21,22,40,1
  else if (inputHas(otaString, "SETDATETIME:SENSLOPE:")) {
    resetWatchdog();
    char *_password = strtok(otaString + 11, ":");      // tokenize timestning to extract datetime values
    char *YY = strtok(NULL, ",");                       //  pwede ito gawing loop para bawas sa lines pero mas madali lang tingnan kung ganito
    char *MM = strtok(NULL, ",");
    char *DD = strtok(NULL, ",");
    char *hh = strtok(NULL, ",");
    char *mm = strtok(NULL, ",");
    char *ss = strtok(NULL, ",");
    char *dd = strtok(NULL, ",");                       // 
    int _YY = atoi(YY);
    int _MM = atoi(MM);
    int _DD = atoi(DD);
    int _hh = atoi(hh);
    int _mm = atoi(mm);
    int _ss = atoi(ss);
    int _dd = atoi(dd);

    setRTCDateTime(_YY, _MM, _DD, _hh, _mm, _ss, _dd);    
    getTimeStamp(_timestamp, sizeof(_timestamp));       // updates global timetamp variable holder _timestamp

    sprintf(OTAReply, "%s updated timestamp: %s",flashLoggerName.sensorNameList[0], _timestamp);
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
  }

  //  Attempts to update datalogger timestamp using GPRS.
  //  Success may vary depending on signal quality and SIM card data allocation
  //  Sample: FETCHGPRSTIME:SENSLOPE:
  else if (inputIs(otaString, "FETCHGPRSTIME:SENSLOPE:")) {
    resetWatchdog();
    updateTimeWithGPRS(); //success depends the network connection quality
    getTimeStamp(_timestamp, sizeof(_timestamp));
    sprintf(OTAReply, "%s current timestamp: %s",flashLoggerName.sensorNameList[0], _timestamp);
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
  }

  //  CAUTION: hindi advisable na gamitin sa mga datalogger na sobrang hina ng signal.
  //  Special OTA for updating timestamp using sms timestamp
  //  pwede nama na magupdate ito everytime makareceive ng SMS...
  //  pero, mas mabilis ang magiging degradation ng flash dahil limited lang ang write cycles
  //  sa ngayon ay manually triggered na lang muna 
  //  Sample: SMSTIMEUPDATE:SENSLOPE:
  else if (inputIs(otaString,"SMSTIMEUPDATE:SENSLOPE:") || inputIs(otaString,"GATEWAYTIMEUPDATE")) {
    resetWatchdog();    
    timestampUpdate(OTATimestamp);
    delayMillis(1000);
    getTimeStamp(_timestamp, sizeof(_timestamp));
    sprintf(OTAReply, "%s updated timestamp: %s",flashLoggerName.sensorNameList[0], _timestamp);
  

    if (inputIs(otaSenderBuf,"NANEEE")) sendThruLoRa(OTAReply);
    else sendThruGSM(OTAReply, OTASender);
  }

  // Checks the current/saved timestamp of the datalogger
  // Sample: CHECKTIMESTAMP:SENSLOPE:
  else if (inputIs(otaString, "CHECKTIMESTAMP:SENSLOPE:")) {
    resetWatchdog();
    getTimeStamp(_timestamp, sizeof(_timestamp));
    sprintf(OTAReply, "%s current timestamp: %s",flashLoggerName.sensorNameList[0], _timestamp);
    OTAReply[strlen(OTAReply)+1]=0x00;
    if (inputIs(otaSenderBuf,"NANEEE")) sendThruLoRa(OTAReply);
    else sendThruGSM(OTAReply, OTASender);
  }


  //  Replaces current datalogger name with additional "parameter" NAMECHANGE:SENSLOPE:<PARAMETER> 
  //  Sample: DATALOGGERNAME:SENSLOPE:BCLTA
  else if (inputHas(otaString, "DATALOGGERNAME:SENSLOPE:")) {
    resetWatchdog();
    char *_newName = strtok(otaString + 23, ":");
    sprintf(flashLoggerName.sensorNameList[0], "%s", _newName);
    savedLoggerName.write(flashLoggerName);
    delayMillis(100);
    sprintf(OTAReply,"Datalogger name changed to %s",flashLoggerName.sensorNameList[0]);
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
  }

  //  Replaces the current server number with the additional parameter after the keyword
  //  Sample: SERVERNUMBER:SENSLOPE:639954645704
  else if (inputHas(otaString, "SERVERNUMBER:SENSLOPE:")) {
    resetWatchdog();
    bool validServer = true;
    char newServer[50];
    char *_newServerNum = strtok(otaString + 21, ":");
    
    sprintf(newServer, "%s", _newServerNum);
    flashServerNumber = savedServerNumber.read();

    //  checks if server number is incomplete or does not match normal number lenght
    //  if number is iinvalid its replaced by the default number and false is returned
    // debugPrintln(newServer);
    validServer = checkServerNumber(newServer);

    newServer[strlen(newServer)+1]=0x00;
    strncpy(flashServerNumber.dataServer, newServer, strlen(newServer));
    savedServerNumber.write(flashServerNumber);                                 //save to flash memory
    
    // compose OTA command reply
    if (validServer) sprintf(OTAReply,"%s updated server number: %s",flashLoggerName.sensorNameList[0], flashServerNumber.dataServer);
    else sprintf(OTAReply,"%s server number set to default: %s",flashLoggerName.sensorNameList[0], flashServerNumber.dataServer);
    sendThruGSM(OTAReply, OTASender);
  }

  //  Checks current/saved server number of the device
  //  Sample: ?SERVERNUMBER:SENSLOPE:
  else if (inputIs(otaString, "?SERVERNUMBER:SENSLOPE:")) {
    resetWatchdog();
    sprintf(OTAReply, "%s current server number: %s",flashLoggerName.sensorNameList[0], flashServerNumber.dataServer);
    sendThruGSM(OTAReply, OTASender);
  }

  //  Resets the microcontroller; similar to pressing the reset button of the Feather m0 
  //  Sample: RESET:SENSLOPE:
  else if (inputIs(otaString, "RESETGSM:SENSLOPE:")) {
    resetWatchdog();
    sprintf(OTAReply, "%s GSM module will reset...", flashLoggerName.sensorNameList[0]);
    sendThruGSM(OTAReply, OTASender);         //  send reply first before resetting GSM
    delayMillis(1000);
    if (GSMReset()) sendThruGSM("GSM module reset success", OTASender);                               //  GSM reset function
  }
  
  //  Resets the microcontroller; similar to pressing the reset button of the Feather m0 
  //  Sample: RESET:SENSLOPE:
  else if (inputIs(otaString, "RESET:SENSLOPE:") || inputIs(otaString, "RESETDATALOGGER")) {
    resetWatchdog();
    sprintf(OTAReply, "Resetting datalogger %s...", flashLoggerName.sensorNameList[0]);
    if (inputIs(otaSenderBuf,"NANEEE")) sendThruLoRa(OTAReply);
    else sendThruGSM(OTAReply, OTASender);         //  send reply first before resetting
    delayMillis(1000);
    selfResetFlag = true;                        //  set reset flag trigger reset function at the end of loop
  }

  //  Sets a flag to send additional parameter XXXXXXXXXXX [command] to router(s)
  //  Sample:   ROUTER:SENSLOPE:RESETDATALOGGER         -->> this should work.. hopefully \\*.*//
  //            ROUTER:SENSLOPE:UPDATETIMESTAMP
  else if (inputHas(otaString, "ROUTER:SENSLOPE:")) {
    resetWatchdog();
    routerOTAflag = true;
    char * tempOTA = strtok(otaString +15, ":");
    sprintf(routerOTACommand, tempOTA);
    sprintf(OTAReply, "%s will attempt to send OTA to router(s) after data gathering", flashLoggerName.sensorNameList[0]);
    debugPrintln(OTAReply);
    sendThruGSM(OTAReply, OTASender);
  }

  //  Updates the current/saved rain collector  type usng the saved parameter 
  //  Sample: SETRAINCOLLECTOR:SENSLOPE:0    [0 - pronamic, 1 - davis]
  else if (inputHas(otaString, "SETRAINCOLLECTOR:SENSLOPE:")) {
    resetWatchdog();
    uint8_t newCollectorType;
    char *_newCollectorType = strtok(otaString + 25, ":");
    newCollectorType = atoi(_newCollectorType);
    if (savedRainCollectorType.read() == newCollectorType) return;  //  ends processing if value will not change
    savedRainCollectorType.write(newCollectorType);
    delayMillis(1000);

    if (savedRainCollectorType.read() == 0) sprintf(OTAReply, "%s rain collector type changed to Pronamic Rain Collector (0.5mm/tip)", flashLoggerName.sensorNameList[0]);
    else if (savedRainCollectorType.read() == 2) sprintf(OTAReply, "%s rain collector type changed to DAVIS Rain Collector (0.2mm/tip)", flashLoggerName.sensorNameList[0]);
    else if (savedRainCollectorType.read() == 3) sprintf(OTAReply, "%s rain collector type changed to Generic Rain Collector (1.0/tip)", flashLoggerName.sensorNameList[0]);
    else sprintf(OTAReply, "Invalid rain collector type value");
    sendThruGSM(OTAReply, OTASender);
  }

  //  Replaces current [saved] wake time interval with using additional parameter [0-5] similar with debug menu
  //  [0] 30 minutes (hh:00 & hh:30)")
  //  [1] 15 minutes (hh:00, hh:15, hh:30, hh:45)
  //  [2] 10 minutes (hh:00, hh:10, hh:20, ... )
  //  [3] 5 minutes (hh:00, hh:05, hh:10, ... )
  //  [4] 3 minutes (hh:00, hh:03, hh:06, ... )
  //  [5] 30 minutes with 15min offset from 00 (hh:15 & hh:45)");
  //  Anything above 5 defaults to "0" (30 minutes (hh:00 & hh:30)
  //  Sample: SETSENDINGTIME:SENSLOPE:3
  else if (inputHas(otaString, "SETSENDINGTIME:SENSLOPE:")) {
    resetWatchdog();
    char *_newInterval = strtok(otaString + 23, ":");
    uint8_t newInterval = atoi(_newInterval);

    if (newInterval > 5) return;
    if (savedAlarmInterval.read() == newInterval) return; // Exit

    savedAlarmInterval.write(newInterval);
    delayMillis(500);
    // rtc.clearINTStatus();
    setNextAlarm(savedAlarmInterval.read());  
    
    sprintf(OTAReply, "%s updated send interval setting: %d",flashLoggerName.sensorNameList[0], savedAlarmInterval.read());
    OTAReply[strlen(OTAReply)+1]=0x00;
    if (inputIs(otaSenderBuf,"NANEEE")) sendThruLoRa(OTAReply);
    else sendThruGSM(OTAReply, OTASender);
  }

  //  Replaces current [saved] battery type
  //  Sample: SETBATTERYTYPE:SENSLOPE:0     -->> set to 12V lead acid
  //  Sample: SETBATTERYTYPE:SENSLOPE:1     -->> set to 4.2V li-ion
  else if (inputHas(otaString, "SETBATTERYTYPE:SENSLOPE:")) {
    resetWatchdog();
    char *_newbatteryType = strtok(otaString + 23, ":");
    uint8_t newbatteryType = atoi(_newbatteryType);

    if (newbatteryType > 1) return;   // rejects invalid values
    if (savedBatteryType.read() == newbatteryType) return; // Exit if OTA value is same as saved value

    savedBatteryType.write(newbatteryType);
    delayMillis(500);
    
    float battVolt = readBatteryVoltage(savedBatteryType.read());
    if (savedBatteryType.read() == 0) sprintf(OTAReply, "%s updated battery type [0] : 12V Lead Acid battery\nBattery voltage: %0.2fV",flashLoggerName.sensorNameList[0],battVolt);
    else if (savedBatteryType.read() == 1) sprintf(OTAReply, "%s updated battery type [1] : 4.2V Li-Ion battery\nBattery voltage: %0.2fV",flashLoggerName.sensorNameList[0],battVolt);
    sendThruGSM(OTAReply, OTASender);
  }

  //  Replaces current [saved] GSM power mode using additional parameter [0-2]
  //  [0] - Normal operation; GSM is always ON
  //  [1] - Low-power Mode (Always ON, but GSM SLEEPS when inactive)
  //  [2] - Power-saving mode; Turned off during night time to save power
  else if (inputHas(otaString, "SETGSMPOWERMODE:SENSLOPE:")) {
    resetWatchdog();
    char *_newPowerMode = strtok(otaString + 24, ":");
    uint8_t newPowerMode = atoi(_newPowerMode);

    if (newPowerMode > 2 ) return;   // rejects invalid values
    if (savedGSMPowerMode.read() == newPowerMode) return; // Exit if OTA value is same as saved value

    savedGSMPowerMode.write(newPowerMode);
    delayMillis(500);
    
    if (savedGSMPowerMode.read() == 0) sprintf(OTAReply, "%s updated GSM power mode [0]: Always ON",flashLoggerName.sensorNameList[0]);
    else if (savedGSMPowerMode.read() == 1) sprintf(OTAReply, "%s updated GSM power mode [1]: Low-power Mode",flashLoggerName.sensorNameList[0]);
    else if (savedGSMPowerMode.read() == 2) sprintf(OTAReply, "%s updated GSM power mode [2]: Power-saving Mode",flashLoggerName.sensorNameList[0]);
    sendThruGSM(OTAReply, OTASender);
    GSMPowerModeSet();
  }

  // Check current [saved] sensor command
  // CMD?:SENSLOPE:
  else if (inputIs(otaString, "CMD?:SENSLOPE:")) {
    resetWatchdog();
    flashCommands = savedCommands.read();
    sprintf(OTAReply,"%s current sensor command: %s",flashLoggerName.sensorNameList[0], flashCommands.sensorCommand);
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
  }

  // Replaces current [saved] sensor comand with additional parameter [ARQCMD6T or ARQCMD6S]
  // Sample: CMD:SENSLOPE:ARQCM6S
  else if (inputHas(otaString, "CMD:SENSLOPE:")) {
    resetWatchdog();
    flashCommands = savedCommands.read();
    char *_password = strtok(otaString + 4, ":\",");
    char *dueCmd = strtok(NULL, ":\",");

    strcpy((flashCommands.sensorCommand), dueCmd);
    savedCommands.write(flashCommands);  //save to flash memory

    flashCommands = savedCommands.read();
    sprintf(OTAReply,"%s current sensor command: %s",flashLoggerName.sensorNameList[0], flashCommands.sensorCommand);
    sendThruGSM(OTAReply, OTASender);
  }

  // ADD OPTION FOR POWER SAVING MODE, TURN ON/OFF GSM DURING NIGHTTIME
  // ~ ALSO FETCH DUE CONFIG?
  // ~ FETCH SD CARD DATA IF PERMISSIBLE?

  // ~ TURN ON/OFF GSM DURING NIGHTTIME
  // ~ INCREASE SAMPLING INTERVAL?

  // ~ ALSO FETCH DUE CONFIG?
  // ~ FETCH SD CARD DATA IF PERMISSIBLE?
  //   return OTACommandFound;
// }
  resetWatchdog();
}

void clearGlobalSMSDump() {
  resetWatchdog();
  for (int d = 0; d < sizeof(_globalSMSDump);d++) _globalSMSDump[d]=0x00; 
}

void updateTimeWithGPRS() {
  resetWatchdog();
  detachInterrupt(digitalPinToInterrupt(GSMINT));
  char timebuffer[13];
  char gsmResponse[200];

  GSMSerial.write("AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r");  //AT+SAPBR=3,1,"Contype","GPRS"
  delayMillis(1000);
  GSMAnswer(gsmResponse, sizeof(gsmResponse));
  debugPrintln(gsmResponse);
  GSMSerial.write("AT+SAPBR=3,1,\"APN\",\"internet\"\r");  //AT+SAPBR=3,1,"APN","internet"
  delayMillis(1000);
  GSMAnswer(gsmResponse, sizeof(gsmResponse));
  debugPrintln(gsmResponse);
  GSMSerial.write("AT+SAPBR=1,1\r");  //Open bearer
  delayMillis(4000);
  GSMAnswer(gsmResponse, sizeof(gsmResponse));
  debugPrintln(gsmResponse);
  GSMSerial.write("AT+CNTPCID=1\r"); //use bearer profile 1
  delayMillis(500);
  GSMAnswer(gsmResponse, sizeof(gsmResponse));
  debugPrintln(gsmResponse);
  GSMSerial.write("AT+CNTP=\"time.upd.edu.ph\",32\r");  //AT+CNTP="time.upd.edu.ph",32
  delayMillis(200);
  GSMAnswer(gsmResponse, sizeof(gsmResponse));
  debugPrintln(gsmResponse);
  GSMSerial.write("AT+CNTP\r");
  delayMillis(3000);
  GSMAnswer(gsmResponse, sizeof(gsmResponse));
  debugPrintln(gsmResponse);
  GSMSerial.write("AT+CCLK?\r");
  delay(1000);
  GSMAnswer(gsmResponse, sizeof(gsmResponse));
  debugPrintln(gsmResponse);

  //  checks for year inidicator in CCLK string, default year is "2000" = "00" [3rd & 4th number or year YYYY]
  //  finding "2" [3rd number of the year] indicates year is between 2020 and 2029.
  //  This will stop working after 2029...
  //  kailangan na palitan ng "3" for years 2030 - 2039 or something na scalable through the years, katulad ng kanta si Kenny Rogers
  if (strstr(gsmResponse, "+CCLK: \"2"))  
  {
    gsmResponse[27] = 0x00;
    for (byte i = 0; i < strlen(gsmResponse); i++) {
      gsmResponse[i] = gsmResponse[i + 10];
    }
    
    updateTsNetworkFormat(gsmResponse);
        
    getTimeStamp(_timestamp, sizeof(_timestamp));
    debugPrint("Current timestamp: ");
    debugPrintln(_timestamp);
    debugPrintln(" ");

    GSMSerial.write("AT+SAPBR=0,1\r");
    delayMillis(1000);
    GSMAnswer(gsmResponse, sizeof(gsmResponse));
    debugPrintln(gsmResponse);

  } else {
    debugPrintln("Time sync failed!");
    GSMReset();
  }
  REG_EIC_INTFLAG = EIC_INTFLAG_EXTINT2; //clear interrupt flag before enabling
  attachInterrupt(digitalPinToInterrupt(GSMINT), GSMISR, FALLING);
  resetWatchdog();
}

void setGSMPowerMode() {
  resetWatchdog();
  int intervalBuffer = 0;
  unsigned long intervalWait = millis();
  Serial.print("Enter GSM power mode: ");
  while (millis() - intervalWait < 60000) {
    resetWatchdog();
    if (Serial.available() > 0) {
    intervalBuffer = Serial.parseInt();
    if (intervalBuffer > 2) {
      Serial.println("Invalid value, mode unchanged.");
      resetWatchdog();
      return;
    }
    savedGSMPowerMode.write(intervalBuffer);
    Serial.print("Updated GSM power mode: ");
    Serial.println(savedGSMPowerMode.read());
    break;
    }
  }
  resetWatchdog();
}
