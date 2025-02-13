void resetRainTips() {
  tipCount = 0.00;
  delayMillis(75);
}

void rainInit(byte RAIN_SIG) {
  pinMode(RAIN_SIG, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RAIN_SIG), rainISR, FALLING);
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