void resetRainTips() {
  tipCount = 0.00;
  delayMillis(75);
}

void InitializeRainULP(byte RAIN_SIG) {
  pinMode(RAIN_SIG, INPUT_PULLUP);                                                  //  set pin as INPUT
  Serial.printf("Initialize rain count %u\n", RTC_SLOW_MEM[EDGE_COUNT] & 0xFFFF);   //  this should throw some random values


  // attachInterrupt(digitalPinToInterrupt(RAIN_SIG), rainISR, FALLING);
}

void rainTest() {
  bool runTest = true;
  Serial.printf("Max test limit: %u tips \n", MAX_TEST_LIMIT);
  while (runTest) {
    if (testFlag) {
      Serial.printf("Rain tips %u \n", tipCount);
      testFlag = false;
    }
    if (tipCount == MAX_TEST_LIMIT) {
      Serial.println("Rain test end");
      runTest = false;
    }
  } 
}
