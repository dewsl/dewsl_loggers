#define SSM_DEFAULT_TIMEOUT 180000UL
#define SSM_LINE_BUFFER_SIZE 500

void flushSSMSerial() {
  while (SSMSerial.available()) {
    SSMSerial.read();
  }
}

void SSMInit() {
  // pinMode(AUX_TRIG, OUTPUT);
  // // Force SSM ON
  // digitalWrite(AUX_TRIG, HIGH);
  SSMSerial.begin(SSMBAUDRATE, SERIAL_8N1, SSM_RX, SSM_TX);
  // delayMillis(500);
  // flushSSMSerial();
  // debugPrintln("SSM forced ON.");
}

void getSSMData() {

  bool readSSMData = true;
  char ssmLineBuffer[SSM_LINE_BUFFER_SIZE];
  char commandContainer[80];
  char sensorCommand[15];
  uint8_t dataSegmentCount = 0;
  unsigned long samplingStart;

  fetchParam(paramStorage,SENSOR_COMMAND,sensorCommand,sizeof(sensorCommand));
  if (strlen(sensorCommand) == 0) {
    sprintf(sensorCommand, defaultSensorCommand);
  }

  debugPrintln("Starting SSM data collection...");
  /*
   * IMPORTANT:
   * Flush UART before powering STM32.
   */
  flushSSMSerial();
  debugPrintln("Turning ON SSM...");
  digitalWrite(AUX_TRIG, HIGH);
  debugPrintln("SSM should now be ON");

  /*
   * Allow STM32 boot time.
   */
  delayMillis(5000);
  diagnosticCheck(2);
  /*
   * Flush startup garbage bytes from STM32 UART.
   */
  flushSSMSerial();
  delayMillis(200);
  getTimeStamp(_timestamp, sizeof(_timestamp));
  sprintf(commandContainer, "%s/%s/%s", sensorCommand, dataloggerNameList[0],_timestamp);
  debugPrint("Sending to SSM: ");
  debugPrintln(commandContainer);

  /*
   * Extra flush before sending command.
   */
  flushSSMSerial();
  delayMillis(200);
  SSMSerial.println(commandContainer);
  SSMSerial.flush();
  samplingStart = millis();

  while (readSSMData) {
    if (millis() - samplingStart > SSM_DEFAULT_TIMEOUT) {
      debugPrintln("SSM sampling timed out!");
      readSSMData = false;
      break;
    }
    memset(ssmLineBuffer, 0, sizeof(ssmLineBuffer));
    if (SSMSerial.available() > 0) {
      SSMSerial.readBytesUntil(
        '\n',
        ssmLineBuffer,
        sizeof(ssmLineBuffer) - 1
      );
      for (uint16_t i = 0; i < sizeof(ssmLineBuffer); i++) {
        if (
          ssmLineBuffer[i] == '\n' ||
          ssmLineBuffer[i] == '\r'
        ) {
          ssmLineBuffer[i] = 0x00;
        }
      }
      if (strlen(ssmLineBuffer) > 0) {
        debugPrint("SSM RX: ");
        debugPrintln(ssmLineBuffer);
        samplingStart = millis();
      }

      /*
       * STM32 done.
       */
      if (strstr(ssmLineBuffer, "STOPLORA")) {
        debugPrintln(
          "SSM data collection stop marker received."
        );
        readSSMData = false;
        break;
      }

      /*
       * Valid segmented payload.
       */
      if (strstr(ssmLineBuffer, ">>")) {
        dataSegmentCount++;
        addToSMSStack(ssmLineBuffer);
        debugPrintln("Sending ACK to STM32...");
        //SSMSerial.println("OK");
        //SSMSerial.flush();
        debugPrintln("ACK sent.");
      }
    }
  }


  delayMillis(100);
  SSMSerial.end();

  if (dataSegmentCount == 0) {
    char noDataBuffer[200];
    getTimeStamp(_timestamp, sizeof(_timestamp));
    sprintf(noDataBuffer,"%s*NODATAFROMSENSLOPE*%s",dataloggerNameList[0],_timestamp);
    addToSMSStack(noDataBuffer);
  }

  debugPrint("SSM data segment count: ");
  debugPrintln(dataSegmentCount);
  debugPrintln("SSM data collection finished!");

  /*
   * Turn OFF STM32 after sampling.
   */
  diagnosticCheck(3);
  digitalWrite(AUX_TRIG, LOW);
}