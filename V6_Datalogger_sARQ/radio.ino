const char ackKey[] = "^REC'D_";
const char LBTKey[] = "START:";
char _routerOTA[100];
char listenKey[50];

void updateListenKey() {
  EEPROM.get(DATALOGGER_NAME, flashLoggerName);
  sprintf(listenKey, "%s%s",LBTKey, flashLoggerName.sensorNameList[0]);
  // rf95.setPreambleLength();
}