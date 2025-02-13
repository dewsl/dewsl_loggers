void rtcInit(byte RTC_INT_PIN) {
  pinMode(RTC_INT_PIN, INPUT_PULLUP);
  attachInterrupt(RTC_INT_PIN, alarmISR, FALLING);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);


}

void delayMillis(int delayDuration) {
  int maxDelayDuration = 15000;  //15sec
  bool maxDurationReached = false;
  unsigned long timeStart = millis();
  while (!maxDurationReached) {
    if ((millis() - timeStart) > maxDelayDuration) {
      Serial.println("Max delay duration: 15000");
      return;
    }
    if ((millis() - timeStart) > delayDuration) {
      return;
    }
  }
}

void dateTimeNow() {
  DateTime now = rtc.now();
  Serial.print("RTC Date Time: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(now.dayOfTheWeek());
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);
}

void set2Alarms(byte minuteAlarm1, byte minuteAlarm2) {
  rtc.armAlarm(1, false);
  rtc.clearAlarm(1);
  rtc.alarmInterrupt(1, false);

  rtc.armAlarm(2, false);
  rtc.clearAlarm(2);
  rtc.alarmInterrupt(2, false);
  
  rtc.writeSqwPinMode(DS3231_OFF);

  // rtc.setAlarm1(DateTime(0, 0, 0, 0, 1, 0), DS3231_A1_Minute);
  // rtc.setAlarm2(DateTime(0, 0, 0, 0, 2, 0), DS3231_A2_Minute);
  
  rtc.setAlarm(ALM1_MATCH_MINUTES, minuteAlarm1, 0, 0);
  rtc.setAlarm(ALM2_MATCH_MINUTES, minuteAlarm2, 0, 0);

  rtc.alarmInterrupt(1, true);
  rtc.alarmInterrupt(2, true);
}

// void printDateTime() {
//   char timestring[100] = "INVALID";
//   // getTimeStamp(_timestamp, sizeof(_timestamp));
//   const char * monthsEq[12] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec"};
//   const char * daysEq[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
//   const char * timeOfDayEq[2] = {"AM", "PM"};
//   uint8_t timeOfDayIndex = 0; // defaults to AM ddaytime indicator
//   DateTime now = rtc.now();
//   uint8_t hourBuffer = now.hour();
  
//   if (hourBuffer > 11 && hourBuffer < 24) timeOfDayIndex = 1;                   //  sets PM as daytime indicator
//   if (hourBuffer > 12 && hourBuffer < 24) hourBuffer = hourBuffer - 12;         //  subtract 12 from 24hr format to get 12hr format
//   if (hourBuffer == 0) hourBuffer = 12;                                         //  replace midnight time 00 to 12
//   if (now.month()-1 <= 12) sprintf(timestring, "%s %s %d, %d", daysEq[now.dayOfTheWeek()-1],monthsEq[now.month()-1],now.day(),now.year());  //  generate day and data string
//   debugPrint("Current date:\t ");
//   debugPrintln(timestring);
//   if (inputIs(timestring, "INVALID")) sprintf(timestring, "%d:%02d:%02d %s [RTC_ERR]" ,hourBuffer,now.minute(),now.second(),timeOfDayEq[timeOfDayIndex]);
//   else sprintf(timestring, "%d:%02d:%02d %s" ,hourBuffer,now.minute(),now.second(),timeOfDayEq[timeOfDayIndex]);                             // generte timestring & daytime indicator
//   debugPrint("Current time:\t ");
//   debugPrintln(timestring);
// }