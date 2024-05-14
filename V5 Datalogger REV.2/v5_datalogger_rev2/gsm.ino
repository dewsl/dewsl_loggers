void GSMINTInit(uint8_t GSMpin) {
  pinMode(GSMpin, INPUT);
  REG_EIC_INTFLAG = EIC_INTFLAG_EXTINT2;    // clears interrupt flag
  attachInterrupt(digitalPinToInterrupt(GSMpin), GSMISR, FALLING);
}

void GSMISR(){
  if (savedGSMPowerMode.read() != 2) GSMIntFlag=true;
}

void GSMInit() {
  char GSMLine[500];
  bool gsmSerial = false;
  bool GSMconfig = false;
  bool signalCOPS = false;
  unsigned long initStart = millis();
  int initTimeout = 20000;
  uint8_t errorCount = 0;

  debugPrintln("Connecting GSM to network...");

  detachInterrupt(digitalPinToInterrupt(GSMINT));
  GSMSerial.begin(GSMBAUDRATE);
  delayMillis(1000);
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
    GSMAnswer(gsmInitResponse, 100);  
    if (strlen(gsmInitResponse) > 0 && gsmInitResponse != "\n") debugPrintln(gsmInitResponse);
    delayMillis(300);
  } while (millis() - gsmPowerOn < 5000);
   
  while (!gsmSerial || !GSMconfig || !signalCOPS ) {  //include timeout later
    if (!gsmSerial) GSMSerial.write("AT\r");
    if (GSMWaitResponse("OK", 1000, 1) && !gsmSerial) { 
      debugPrintln("Serial comm ready!");
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
    if (gsmSerial && !GSMconfig && GSMWaitResponse("OK", 5000, 1)) {
      GSMSerial.flush();
      delayMillis(500);
      GSMSerial.write("AT+COPS=0,1;+CMGF=1;+IPR=0");
      GSMSerial.write("\r");
      GSMSerial.write("AT+CNMI=0,0,0,0,0\r");
      debugPrintln("GSM module config OK!");
      GSMconfig = true;
    }
    if (GSMconfig) {
    GSMSerial.flush();
    GSMSerial.write("AT+CSQ\r");
    delayMillis(1000);
    }
    if (GSMconfig && !signalCOPS && GSMGetResponse(gsmInitResponse, sizeof(gsmInitResponse), "+CSQ", 4000)) {
      // int CSQval = parseCSQ(gsmInitResponse);
      // debugPrintln(gsmInitResponse);
      int CSQval = parseCSQ(gsmInitResponse);
      if (CSQval > 0) {
        debugPrintln("Checked network signal quality..");
        debugPrint("CSQ: ");
        debugPrintln(CSQval);
        signalCOPS = true;
      }
    }
    if (gsmSerial && GSMconfig && signalCOPS) {
      GSMSerial.write("AT&W\r");
      debugPrintln("");
      debugPrintln("GSM READY");
      REG_EIC_INTFLAG = EIC_INTFLAG_EXTINT2;    // clears interrupt flag
      attachInterrupt(digitalPinToInterrupt(GSMINT), GSMISR, FALLING);
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
  }
  GSMSerial.flush();
}

bool sendThruGSM(const char* messageToSend, const char* serverNumber) {
  
  bool sentFlag = false;
  char GSMresponse[100];
  char messageContainer[250];
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
  if (GSMWaitResponse("OK",1000, 1)) {                                                        // Checks if GSM serial is accessible
    GSMSerial.write(CMGSContainer);
    if (GSMWaitResponse(">",5000, 1)) {
      GSMSerial.write(messageToSend);
      delayMillis(500);
      GSMSerial.write(26);
    } else {
      debugPrintln("Unable to send");
      GSMSerial.write(27);
      GSMSerial.write(27);
    }
    if (GSMWaitResponse("+CMGS",5000, 1)) {
      LEDSend();
      // debugPrintln("Message sent!");
      sentFlag = true;
    } else {
      delayMillis(5000);
      debugPrintln("Sending failed");
      GSMSerial.write(27);  //crude escape
      GSMSerial.write(27);
      GSMSerial.write(27);
      GSMSerial.flush();
      // responive but cannot send
    }
  } else {  //GSM serial is not available
    debugPrintln("GSM module error.");
    //insert GSM reset function here
    // GSMSerial.write(messageContainer);
  }
  return sentFlag;
}

void sendSMSDump(const char* messageDelimilter, const char* dumpServer) {

  char * smsToken;
  int retryCount = 0;

  smsToken = strtok(_globalSMSDump, messageDelimilter);

  while (smsToken != NULL) {
    if (loggerWithGSM(savedDataLoggerMode.read())) {  // send thru GSM
      if (sendThruGSM(smsToken, dumpServer)) {
      LEDSend();
      debugPrintln("Message segment sent");
      } else {
        retryCount++;
        debugPrintln("Retrying..");
        delayMillis(5000);
        if (sendThruGSM(smsToken, dumpServer)) {
          LEDSend();
          debugPrintln("Message segment sent");
        } else {
          debugPrintln("Retry failed");
        }
      }
    } else {  // send thru LORA
      if (sendThruLoRaWithAck(smsToken,random(1000,3000),3)) debugPrintln("Message segment acknowledged");  // send thru LORA
      else debugPrintln("No valid response received");
    }
    smsToken = strtok(NULL,"~");
  }

  // in case of 
}

void GSMAnswer(char* responseBuffer, int bufferLength) {
  
  int bufferIndex = 0;
  unsigned long waitStart = millis();

  for (int i = 0; i < bufferLength; i++){
    responseBuffer[i] = 0x00;
  }
  for (int j = 0; j < bufferLength && GSMSerial.available() > 0 ; j++) {
    responseBuffer[j] = GSMSerial.read();
  }
  if (strlen(responseBuffer) >= bufferLength) {
    responseBuffer[bufferLength-1] = 0x00;
  } else { responseBuffer[strlen(responseBuffer)]=0x00; }
}

// checks of a response from GSM serial
// returns TRUE then correct response is received
bool GSMWaitResponse(const char* targetResponse, int waitDuration, int showResponse) {
  bool responseValid = false;
  char toCheck[50];
  char charBuffer;
  char responseBuffer[300];
  unsigned long waitStart = millis();
  strcpy(toCheck, targetResponse);
  toCheck[strlen(toCheck)] = 0x00;

  do {
    delayMillis(100);
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

      if (strlen(responseBuffer) > 0 && responseBuffer != "/n") {
        if (showResponse > 0) debugPrintln(responseBuffer);
        if (strstr(responseBuffer, toCheck)) {
          responseValid = true;
          // break;
        }
      }
  } while (!responseValid && millis() - waitStart < waitDuration);
  return responseValid;
}

// 
bool GSMGetResponse(char* responseContainer, int containerLen, const char * responseReq, int getDuration) {
  char toCheck[50];
  unsigned long waitStart = millis();
  char charBuffer;
  strcpy(toCheck, responseReq);
  toCheck[strlen(toCheck)] = 0x00;

  do {
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
  } while (millis() - waitStart < getDuration);
  return false;
}

void textMode() {   // experimental
  char sendLine[500];
  char receiveLine[500];
  char bufferChar;
  bool _textMode = true;
  GSMSerial.flush();
  GSMSerial.write("AT+CNMI=1,2,0,0,0\r");
  while (_textMode) {
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
  }
}

int parseCSQ(char *buffer) {
  char *tmpBuf;
  tmpBuf = strtok(buffer, ": ");
  tmpBuf = strtok(NULL, ",");
  return (atoi(tmpBuf));
}

bool readCSQ(char * csqContainer) {
  bool responseValid = false;
  char csqSerialBuffer[50];
  // char c_csq[5] = "99";
  // for (int c = 0; c > sizeof(csqBuffer) ; c++) {
  //   csqBuffer[c] = 0x00;
  // }
  GSMSerial.flush();
  // GSMSerial.write("AT+CSQ\r");
  // delayMillis(500);
  // GSMAnswer(csqContainer, sizeof(csqContainer));
  // sprintf(csqBuffer, csqContainer);
  

  GSMSerial.write("AT+CSQ\r");
  delayMillis(1000);
  GSMAnswer(csqSerialBuffer, sizeof(csqSerialBuffer));
  sprintf(csqContainer, csqSerialBuffer);
  if (strstr(csqSerialBuffer,"+CSQ:")) responseValid = true;

  return responseValid;
}

void updateServerNumber() {
  int changeServerTimeout = 60000;
  char serverNumBuffer[15];
  char numBuf;
  int numberIndex = 0;
  unsigned long waitStart;

  waitStart = millis();
  debugPrintln("Enter new server number: ");
  while (millis() - waitStart < changeServerTimeout) {
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
  }
  checkServerNumber(serverNumBuffer);
  serverNumBuffer[strlen(serverNumBuffer)+1]=0x00;
  strncpy(flashServerNumber.dataServer, serverNumBuffer, strlen(serverNumBuffer));
  savedServerNumber.write(flashServerNumber);
  debugPrint("Server number set to: ");
  debugPrintln(flashServerNumber.dataServer);

  // if (strlen(serverNumBuffer) == 11 || strlen(serverNumBuffer) == 13) {
  //   strncpy(flashServerNumber.dataServer, serverNumBuffer, strlen(serverNumBuffer));
  //   savedServerNumber.write(flashServerNumber);
  //   delayMillis(500);
  //   flashServerNumber = savedServerNumber.read();
  //   debugPrint("New server number: ");
  //   debugPrintln(flashServerNumber.dataServer);
  // } else {
  //   strncpy(flashServerNumber.dataServer, defaultServerNumber, strlen(defaultServerNumber));
  //   savedServerNumber.write(flashServerNumber);
  //   delayMillis(500);
  //   flashServerNumber = savedServerNumber.read();
  //   debugPrint("Defaulted to server number: ");
  //   debugPrintln(flashServerNumber.dataServer);
  // }
}

void testSendToServer() {
  char sendBuf;
  unsigned long sendWait = millis();
  int sendTestTimeout = 60000;
  int testSendIndex = 0;
  char testSendBuffer[200];

  while (millis() - sendWait < sendTestTimeout) {
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
  }
  if (strlen(testSendBuffer) > 0) {
    flashServerNumber = savedServerNumber.read();
    if (sendThruGSM(testSendBuffer,flashServerNumber.dataServer)) debugPrintln("Message sent");
  }
}

void buildLoggerInfoSMS() {

}

void GSMReset() {
  digitalWrite(GSMPWR, LOW);
  delayMillis(3000);
  digitalWrite(GSMPWR, HIGH);
  delayMillis(2000);
  GSMInit();
}

void addToSMSStack(const char* payloadToAdd) {
  char payloadBuffer[500];
  strcpy(payloadBuffer, payloadToAdd);
  payloadBuffer[strlen(payloadBuffer)] = 0x00;
  
  // if filters
  if (strlen(payloadBuffer) == 0) return;                 //  rejects zero-length data
  if (strstr(_globalSMSDump, payloadBuffer)) return;            //  rejects duplicates

  // if editors
  if (loggerWithGSM(savedDataLoggerMode.read()) && (strstr(payloadBuffer, ">>"))) {            //  offsets/removes the identifier ">>" before adding to stack (sent thru GSM)
    for (byte i = 0; i < strlen(payloadBuffer); i++)  payloadBuffer[i] = payloadBuffer[i + 2];                
  }
  if (!loggerWithGSM(savedDataLoggerMode.read()) && !(strstr(payloadBuffer, ">>"))) {          //  for routers: adds the identifier ">>" before adding to stack (sent thru LoRa)
    sprintf(payloadBuffer, ">>%s",payloadToAdd);
  }
  

  if (strlen(_globalSMSDump) == 0) {                            //  prevent the delimiters from being added at the beginning of the container
    strcpy(_globalSMSDump, payloadBuffer);
    debugPrintln("Data copied to container.");
  } else {
    strcat(_globalSMSDump, dumpDelimiter);
    strcat(_globalSMSDump, payloadBuffer);
    debugPrintln("Data added to container.");
  }
  _globalSMSDump[strlen(_globalSMSDump)+1]=0x00;
}

void generateInfoMessage(char* infoContainer) {
  // TESXXW,40.50,0.00,15.92,11,240415081500
  char csqBuffer[100];
  uint8_t CSQ = 0;
  char tsBuffer[13];
  float rainMultiplier = 1;
  if (readCSQ(csqBuffer)) { // if FALSE, CSQ remains 0
    // debugPrintln(csqBuffer);
    // debugPrintln(CSQ);
    CSQ = parseCSQ(csqBuffer);
  }
  // if (GSMGetResponse(csqBuffer, sizeof(csqBuffer), "+CSQ", 2000)) {
  //   debugPrintln(csqBuffer);
  //   debugPrintln(CSQ);
  //   CSQ = parseCSQ(csqBuffer);
  // }
  getTimeStamp(tsBuffer, sizeof(tsBuffer));
  tsBuffer[12]=0x00;
  if (savedRainSendType.read()==0) {  // sends rain tip equivalent in mm
    if(savedRainCollectorType.read()==0)rainMultiplier = 0.5;
    else if (savedRainCollectorType.read()==1)rainMultiplier = 0.2;   
  }
  sprintf(infoContainer,"%sW,%0.2f,%0.2f,%0.2f,%u,%s",
    flashLoggerName.sensorNameList[0],
    readRTCTemp(),
    _rainTips*rainMultiplier,
    readBatteryVoltage(savedBatteryType.read()),
    CSQ,
    tsBuffer);
}

void GSMPowerModeReset() {
  uint8_t powerMode = savedGSMPowerMode.read();
  if (powerMode == 2) {
    debugPrintln("Turning ON GSM module");
    digitalWrite(GSMPWR, HIGH);
    digitalWrite(GSMRST, HIGH);
    delayMillis(1000);
  } else if (powerMode == 1) {
    GSMSerial.write("AT\r");
    delayMillis(99);
    GSMSerial.flush();
    GSMSerial.write("AT+CSCLK=0\r");
    if (GSMWaitResponse,"OK",1000,1) debugPrintln("GSM is awake");
    else debugPrintln("GSM did NOT wake");  //error code
  } else {}  // default   
}

void GSMPowerModeSet() {
  uint8_t powerMode = savedGSMPowerMode.read();
  if (powerMode == 2) {
      debugPrintln("Turning OFF GSM module");
      digitalWrite(GSMPWR, LOW);
      digitalWrite(GSMRST, LOW);
      delayMillis(1000);
  } else  if (powerMode == 1) {
    GSMSerial.write("AT+CSCLK=2\r");
    if (GSMWaitResponse,"OK",1000,1) debugPrintln("GSM auto sleep enabled");
    else debugPrintln("Failed to set GSM autosleep"); //error code
  } else {} // no powersavings; do nothing
  
}

void checkServerNumber(char * serverNumber) {
  char numberBuffer[20];
  sprintf(numberBuffer, "%s", serverNumber);
  numberBuffer[strlen(numberBuffer)+1]=0x00;
  if (strlen(numberBuffer) == 11 || strlen(numberBuffer) == 12) return;
  //  ignore the rest of the function if length is equal to usual server number lengths 09XXXXXXXXX (11) or 639XXXXXXXXX (12)
  //  
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
  else {
    sprintf(serverNumber,"%s","09175972526");
    debugPrintln("Defaulted to GLOBE1");
  }

} void checkSender(char* senderNum) {
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

}

void deleteMessageInbox() {
  GSMSerial.flush();
  GSMSerial.write("AT+CMGDA=\"DEL ALL\"\r");
  if (GSMWaitResponse("OK", 15000, 1)) debugPrintln("deleted all SMS from SIM");
  else {debugPrintln("Delete SMS failed");}
}

void checkOTACommand() {

    char OTALineBuffer[2000];
    char OTAServer[20];
    char OTASerialLine[500];
    uint16_t otaPos = 0;
    char OTAchar;
    bool findCommand = false;
    
    delayMillis(1000);
    GSMSerial.write("AT+CMGL=\"ALL\"\r");
    delayMillis(500);
  
    while (GSMSerial.available() > 0 ) {
      OTAchar = GSMSerial.read();
      if (OTAchar == '\n' || OTAchar == '\r') {
        OTALineBuffer[strlen(OTALineBuffer)+1]=0x00;  // 
        if (findCommand && strlen(OTAServer)==13) {   // does not get used untila a "valid" mobile number if found
          debugPrintln("Searching for command.. ");
          if (runOTACommand(OTALineBuffer, OTAServer))  {   // process line to extract / run OTA command
            debugPrintln("OTA COMMAND FOUND");
            break;
          }
        }
        if (fetchSenderNumber(OTAServer, OTALineBuffer)) {  //returns TRUE when a string that resembles a mobile number is found
          findCommand = true;     // enables the IF function above
          debugPrint("OTA Number found: ");
          debugPrintln(OTAServer);
        }
        otaPos = 0; // reset buffer index for next line
      }
      else {
         // hopefully ay hindi ito mapuno agad bago mag-reset ng position
        if (strlen(OTALineBuffer) < sizeof(OTALineBuffer)-2) OTALineBuffer[otaPos] = OTAchar;   
        otaPos++; //  increment to fill buffer
      }
    }
}

bool fetchSenderNumber(char * OTAServerContainer, char * lineToProcess) {
  char OTACommandBuffer[500];
  char OTASender[20];
  bool validServer = false;

  sprintf(OTACommandBuffer,"%s", lineToProcess);

  OTACommandBuffer[strlen(OTACommandBuffer)+1]=0x00;

  if (strstr(OTACommandBuffer, "+CMGL")) {
    
    char * OTASenderDetail = strtok(OTACommandBuffer, "+,\"");
    int OTADetailIndex = 0;
    while (OTASenderDetail != NULL) {
      OTADetailIndex++;
      if (OTADetailIndex==3 && strlen(OTASenderDetail) == 12) {
        sprintf(OTAServerContainer,"+%s", OTASenderDetail);
        OTAServerContainer[strlen(OTAServerContainer)+1] = 0x00;
        debugPrint("OTA server: ");
        debugPrintln(OTAServerContainer);
        validServer = true;
      }
      OTASenderDetail = strtok(NULL,"+,\"");
    }
  }
  return validServer;

}

bool runOTACommand(char* OTALineCheck, char * OTASender) {
  char LoRaKey[20]; //might be needed later
  char otaString[strlen(OTALineCheck)+1];
  bool OTACommandFound = false;
  char OTAReply[200];

  flashLoggerName = savedLoggerName.read();
  sprintf(otaString, "%s", OTALineCheck);
  sprintf(LoRaKey, "%s%s", flashLoggerName.sensorNameList[0], ackKey);

  // tokenize response to extract OTA sender mobile number
  // overwrites preceeding sender mobile number with no valid OTA command
  //REGISTER:SENSLOPE:639954645704
  if (strncmp(otaString, "REGISTER:SENSLOPE:", 18) == 0) {
    sprintf(OTAReply,"%s: Registration is no longer needed; proceed with normal OTA commands.",flashLoggerName.sensorNameList[0]);
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
    OTACommandFound = true;
  }
  //SENSORPOLL:SENSLOPE:
  else if (strncmp(otaString, "SENSORPOLL:SENSLOPE:", 20) == 0) {
    // sending_stack[0] = '\0';
    // get_Due_Data(savedDataloggerMode.read(), ota_sender);
    Operation(OTASender);
    sendThruGSM("Data sampling finished", OTASender);
    OTACommandFound = true;
  }
  //SERVERNUMBER:SENSLOPE:639954645704
  else if (strncmp(otaString, "SERVERNUMBER:", 13) == 0) {
    char newServer[50];
    newServer[0] = 0x00;
    // debugPrintln("change server number");
    char *_password = strtok(otaString + 13, ":");
    char *_newServerNum = strtok(NULL, ":");
    // debugPrintln(_newServerNum);

    // store new server number to flash memory
    sprintf(newServer, "%s", _newServerNum);
    
    flashServerNumber = savedServerNumber.read();

    checkServerNumber(newServer);
    newServer[strlen(newServer)+1]=0x00;

    strncpy(flashServerNumber.dataServer, newServer, strlen(newServer));
    savedServerNumber.write(flashServerNumber);
    //save to flash memory

    // compose OTA command reply
    sprintf(OTAReply,"%s updated server number in flash: %s",flashLoggerName.sensorNameList[0], flashServerNumber.dataServer);
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
    OTACommandFound = true;
  }
  //?SERVERNUMBER:SENSLOPE:
  else if (strncmp(otaString, "?SERVERNUMBER:", 14) == 0) {

    sprintf(OTAReply, "%s current server number: %s",flashLoggerName.sensorNameList[0], flashServerNumber.dataServer);
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
    OTACommandFound = true;
  }
  //RESET:SENSLOPE:
  else if (strncmp(otaString, "RESET:SENSLOPE:", 15) == 0 ) {
    // debugPrintln("Resetting microcontroller!");
    sprintf(OTAReply, "Resetting datalogger %s...", flashLoggerName.sensorNameList[0]);
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
    deleteMessageInbox();
    NVIC_SystemReset();
    OTACommandFound = true;
  }
  //SETDATETIME:SENSLOPE:[YYYY,MM,DD,HH,MM,SS,dd[0-6/m-sun],] 2021,02,23,21,22,40,1,
  else if (strncmp(otaString, "SETDATETIME", 11) == 0) {
  
    char *_password = strtok(otaString + 11, ":");
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

    sprintf(OTAReply, "%s updated timestamp: %s",flashLoggerName.sensorNameList[0], _timestamp);
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
    OTACommandFound = true;
  }

  // FETCHGPRSTIME:SENSLOPE:
  else if (strncmp(otaString, "FETCHGPRSTIME", 13) == 0) {
    updateTimeWithGPRS(); //success depends the network connection quality
    getTimeStamp(_timestamp, sizeof(_timestamp));
    sprintf(OTAReply, "%s current timestamp: %s",flashLoggerName.sensorNameList[0], _timestamp);
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
    OTACommandFound = true;
  }

  // CHECKTIMESTAMP:SENSLOPE:
  else if (strncmp(otaString, "CHECKTIMESTAMP:", 15) == 0) {
    
    getTimeStamp(_timestamp, sizeof(_timestamp));
    sprintf(OTAReply, "%s current timestamp: %s",flashLoggerName.sensorNameList[0], _timestamp);
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
    OTACommandFound = true;
  }
  //SENDINGTIME:SENSLOPE:[0-5]:
  else if (strncmp(otaString, "SETSENDINTERVAL", 15) == 0) {
    // debugPrintln("change sending time!");
    char sendStorage[10];
    sendStorage[0] = '\0';
    // debugPrintln(data);

    char *_password = strtok(otaString + 15, ":");
    char *inputSending = strtok(NULL, ":");
    int _inputSending = atoi(inputSending);


    //set sending time
    // debugPrintln(_inputSending);
    savedAlarmInterval.write(_inputSending);
    delayMillis(100);
    
    sprintf(OTAReply, "%s updated send interval setting: %d",flashLoggerName.sensorNameList[0], savedAlarmInterval.read());
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
    OTACommandFound = true;
    
  }
  //CMD?:SENSLOPE:
  else if (strncmp(otaString, "CMD?:", 5) == 0) {
    //read DUE command
  
    flashCommands = savedCommands.read();
    sprintf(OTAReply,"%s current sensor command: %s",flashLoggerName.sensorNameList[0], flashCommands.sensorCommand);
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
    OTACommandFound = true;
  }

  //CMD:SENSLOPE:[ARQCM6T/6S]
  else if (strncmp(otaString, "CMD:SENSLOPE:", 13) == 0) {

    flashCommands = savedCommands.read();
    char *_password = strtok(otaString + 4, ":");
    char *dueCmd = strtok(NULL, ":");

    strcpy((flashCommands.sensorCommand), dueCmd);
    savedCommands.write(flashCommands);  //save to flash memory

    flashCommands = savedCommands.read();
    sprintf(OTAReply,"%s current sensor command: %s",flashLoggerName.sensorNameList[0], flashCommands.sensorCommand);
    OTAReply[strlen(OTAReply)+1]=0x00;
    sendThruGSM(OTAReply, OTASender);
    OTACommandFound = true;
  }

  // ADD OPTION FOR POWER SAVING MODE
  // ~ TURN ON/OFF GSM DURING NIGHTTIME
  // ~ INCREASE SAMPLING INTERVAL?
  // ADD OPTION CHECKING SAVED CONFIGURATION
  // ~ ALSO FETCH DUE CONFIG?
  // ~ FETCH SD CARD DATA IF PERMISSIBLE?
  return OTACommandFound;
}

void clearGlobalSMSDump() {
  for (byte d = 0; d < sizeof(_globalSMSDump);d++) _globalSMSDump[d]=0x00; 
}

void updateTimeWithGPRS() {
  detachInterrupt(digitalPinToInterrupt(GSMINT));
  char timebuffer[13];
  int ts_buffer[7];
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
  if (strstr(gsmResponse, "+CCLK: \"2"))  //2 denotes 3rd number or year YYYY
  {
    gsmResponse[27] = 0x00;
    for (byte i = 0; i < strlen(gsmResponse); i++) {
      gsmResponse[i] = gsmResponse[i + 10];
    }
    
    // debugPrintln(gsmResponse);
    char *ts_token = strtok(gsmResponse, ",/:+");  //22/09/23,18:38:19+08
    byte ts_counter = 0;
    while (ts_token != NULL) {
      ts_buffer[ts_counter] = atoi(ts_token);
      ts_counter++;
      ts_token = strtok(NULL, ",/:+");
    }
    debugPrintln("Synced with GSM network time!");

    // debugPrintln(timebuffer);
    ts_buffer[6] = dayOfWeek((2000+ts_buffer[0]),ts_buffer[1],ts_buffer[2]); // attempt to get correct weekday data
    setRTCDateTime(ts_buffer[0], ts_buffer[1], ts_buffer[2], ts_buffer[3], ts_buffer[4], ts_buffer[5], ts_buffer[6]);
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
}

void setGSMPowerMode() {
  int intervalBuffer = 0;
  unsigned long intervalWait = millis();
  Serial.print("Enter GSM power mode: ");
  while (millis() - intervalWait < 60000) {
    if (Serial.available() > 0) {
    intervalBuffer = Serial.parseInt();
    if (intervalBuffer > 2) {
      Serial.println("Invalid value, mode unchanged.");
      return;
    }
    savedAlarmInterval.write(intervalBuffer);
    Serial.print("Updated GSM power mode: ");
    Serial.println(savedAlarmInterval.read());
    break;
    }
  }
}