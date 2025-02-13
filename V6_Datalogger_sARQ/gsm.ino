void GSMInitInt(byte INT_PIN) {
  pinMode(INT_PIN, INPUT_PULLUP);
  attachInterrupt(INT_PIN, GSMISR, FALLING);
}

void GSMOff(){

  debugPrintln("Turning OFF GSM module..");
  // digitalWrite(GSMPWR, LOW);
  delayMillis(500);
}

bool GSMReset() {
  if (Serial) Serial.println("Resetting the GSM module...");
  // digitalWrite(GSMPWR, LOW);
  digitalWrite(GSM_RST, LOW);
  delayMillis(3000);
  digitalWrite(GSM_RST, HIGH);
  // digitalWrite(GSMPWR, HIGH);
  delayMillis(2000);
  return GSMInit();
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
  if (strlen(responseBuffer) > 0) debugPrintln(responseBuffer);
  if (strlen(responseBuffer) >= bufferLength) {
    responseBuffer[bufferLength-1] = 0x00;
  } else { responseBuffer[strlen(responseBuffer)]=0x00; }
}

void testSendToServer() {
  char sendBuf;
  unsigned long sendWait = millis();
  int sendTestTimeout = 60000;
  int testSendIndex = 0;
  char recepientNumber[15];
  char testSendBuffer[200];
  EEPROM.get(DATALOGGER_NAME, flashLoggerName);

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
  if (strlen(testSendBuffer) > 0 && loggerWithGSM(EEPROM.readByte(DATALOGGER_MODE))) {
    EEPROM.get(SERVER_NUMBER, recepientNumber);
    if (sendThruGSM(testSendBuffer,recepientNumber)) debugPrintln("Message sent");
    else {GSMReset(); if (sendThruGSM(testSendBuffer,recepientNumber)) debugPrintln("Message sent");}
  } 
  // else {
  //   char loraSendBuffer[200];
  //   getTimeStamp(_timestamp, sizeof(_timestamp));
  //   sprintf(loraSendBuffer, ">>%s*%s*%s",flashLoggerName.sensorNameList[0],testSendBuffer,_timestamp);
  //   sendThruLoRa(loraSendBuffer);
  // }
}

bool sendThruGSM(const char* messageToSend, const char* serverNumber) {
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
    GSMSerial.write(CMGSContainer);
    if (GSMWaitResponse(">",5000, true)) {
      GSMSerial.write(messageToSend);
      delayMillis(500);
      GSMSerial.write(26);
    } else {
      debugPrintln("GSM module unable to send");
      GSMSerial.write(27);
      GSMSerial.write(27);
    }
    if (GSMWaitResponse("+CMGS",30000, true)) {
      sentFlag = true;
      // debugPrintln("Message send indentifier!");
      return sentFlag;
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
  // Serial.println("Send end");
  return sentFlag;
}

bool GSMInit() {
  bool initOK = false;
  char GSMLine[500];
  bool gsmSerial = false;
  bool GSMconfig = false;
  bool signalCOPS = false;
  unsigned long initStart = millis();
  int initTimeout = 25000;
  uint8_t errorCount = 0;

  debugPrintln("Connecting GSM to network...");

  detachInterrupt(digitalPinToInterrupt(GSM_RING_INT));
  
  // GSMSerial.begin(GSMBAUDRATE);
  GSMSerial.begin(115200, SERIAL_8N1, GSM_RXD, GSM_TXD);
  
  delayMillis(300);

  // pinMode(GSMPWR, OUTPUT);
  pinMode(GSM_RST, OUTPUT);
  pinMode(GSM_RING_INT, INPUT_PULLUP);
  // digitalWrite(GSMPWR, HIGH);
  digitalWrite(GSM_RST, HIGH);

  unsigned long gsmPowerOn = millis();
  
  char gsmInitResponse[100];
  do {
    GSMAnswer(gsmInitResponse, 100);  
    if (strlen(gsmInitResponse) > 0 && gsmInitResponse != "\n") debugPrintln(gsmInitResponse);
    delayMillis(300);
  } while (millis() - gsmPowerOn < 5000);
   
  while (!gsmSerial || !GSMconfig || !signalCOPS ) {  //include timeout later
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
      // attachInterrupt(digitalPinToInterrupt(GSMINT), GSMISR, FALLING);
      attachInterrupt(digitalPinToInterrupt(GSM_RING_INT), GSMISR, FALLING);
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
  }
  GSMSerial.flush();
  return initOK;
}

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

bool GSMWaitResponse(const char* targetResponse, int waitDuration, bool showResponse) {
  bool responseValid = false;
  char toCheck[50];
  char charBuffer;
  char responseBuffer[300];
  unsigned long waitStart = millis();
  strcpy(toCheck, targetResponse);
  toCheck[strlen(toCheck)] = 0x00;

  do {
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
    delayMillis(50);
  } while (!responseValid && millis() - waitStart < waitDuration);
  // debugPrintln("function end");
  return responseValid;
}
bool readCSQ(char * csqContainer) {
  bool responseValid = false;
  char csqSerialBuffer[50];

  if (loggerWithGSM(EEPROM.readByte(DATALOGGER_MODE)) == false) return false;
  GSMSerial.write("AT\r");
  delayMillis(200);
  GSMSerial.flush();
  GSMSerial.write("AT+CSQ\r");
  delayMillis(1000);
  GSMAnswer(csqSerialBuffer, sizeof(csqSerialBuffer));
  sprintf(csqContainer, csqSerialBuffer);
  if (strstr(csqSerialBuffer,"+CSQ:")) responseValid = true;
  return responseValid;
}

bool checkServerNumber(char * serverNumber) {
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
  else if (inputIs(numberBuffer,"CHI")) sprintf(serverNumber,"%s","09954127577");

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
  serverNumBuffer[strlen(serverNumBuffer)]=0x00;
  EEPROM.put(SERVER_NUMBER, serverNumBuffer);
  EEPROM.commit();
  debugPrint("Server number set to: ");
  debugPrintln(serverNumBuffer);
}