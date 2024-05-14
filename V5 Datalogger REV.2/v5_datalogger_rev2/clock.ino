#define LEAP_YEAR(Y)     ( (Y>0) && !(Y%4) && ( (Y%100) || !(Y%400) ))

void RTCInit(uint8_t RTCPin) {
  pinMode(RTCPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(RTCPin), RTCISR, FALLING);
}

void RTCISR() {
  RTCWakeFlag = true;
  // Serial.println("RTC interrupt");
}

void setupTime() {
  unsigned long setupStart = millis();
  int MM = 0, DD = 0, YY = 0, hh = 0, mm = 0, ss = 0, dd = 0;
  Serial.println(F("\nSet time and date in this format: YYYY,MM,DD,hh,mm,ss,dd[0-6]Mon-Sun"));
  
  while (millis() - setupStart < 60000) {
    if (Serial.available() > 0) {
    YY = Serial.parseInt();
    MM = Serial.parseInt();
    DD = Serial.parseInt();
    hh = Serial.parseInt();
    mm = Serial.parseInt();
    ss = Serial.parseInt();
    dd = Serial.parseInt();
    
    delayMillis(10);
    setRTCDateTime(YY, MM, DD, hh, mm, ss, dd);
    // Serial.print("Current timestamp: ");
    getTimeStamp(_timestamp, sizeof(_timestamp));
    // Serial.println(_timestamp);
    Serial.println();
    return;
    }
  }
}

void setRTCDateTime(int year, int month, int date, int hour, int min, int sec, int weekday) {
  DateTime dt(year, month, date, hour, min, sec, weekday);
  rtc.setDateTime(dt);  // adjust date-time as defined by 'dt'
}

void getTimeStamp(char* tsContainer, uint8_t sizeOfContainer) {
  // char tsBuffer[sizeOfContainer];
  // tsBuffer[0] = 0x00;
  // add function for generating easy to read date time 
  for (int t=0; t < sizeOfContainer; t++) tsContainer[t] = 0x00;
  DateTime now = rtc.now();  //get the current date-time
  sprintf(tsContainer, "%02d%02d%02d%02d%02d%02d", now.year()%1000,now.month(),now.date(),now.hour(),now.minute(),now.second());
  // tsBuffer[strlen(tsBuffer)+1]=0x00;
  // strncpy(tsContainer, tsBuffer, strlen(tsBuffer));

}

void setNextAlarm(int intervalSET) {
  char tsBuf[30];
  uint16_t store_rtc = 00;
  DateTime now = rtc.now();  //get the current date-time
  store_rtc = nextAlarm((int)(now.minute()),intervalSET);
  // enable rtc interrupt
  rtc.enableInterrupts(store_rtc, 00);  // interrupt at (minutes, seconds)
  rtc.clearINTStatus();
  if (Serial) {
    // getTimeStamp(tsBuf, sizeof(tsBuf));
    // Serial.print(tsBuf);
    Serial.print("Next alarm - hh:");
    Serial.println(store_rtc);  
  }
}

uint16_t nextAlarm(int currentMinute, int intervalSaved) {
  int intervalEquivalent = 0;
  uint16_t nextAlarm = 0;
  
  if (intervalSaved == 1) {
    intervalEquivalent = 15;
  } else if (intervalSaved == 2) {
    intervalEquivalent = 10;
  } else if (intervalSaved == 3) {
    intervalEquivalent = 5;
  } else if (intervalSaved == 4) {
    intervalEquivalent = 3;
  } else if (intervalSaved == 5) {      // special case intervals for 15min offset
    if (currentMinute < 15 || currentMinute >= 45) nextAlarm = 15;
    else nextAlarm = 45;
    return nextAlarm;
  } else {
    intervalEquivalent = 30;
  }
  // computed alarms for normal intervals 1-4
  int computedAlarm = (((currentMinute/intervalEquivalent)+1)*intervalEquivalent);
  if (computedAlarm >= 60) {
    nextAlarm = 0;
  } else {
    nextAlarm = computedAlarm;
  }
  return nextAlarm;
}

void setAlarmInterval() {
  int intervalBuffer = 0;
  unsigned long intervalWait = millis();
  Serial.print("Enter alarm settings: ");
  while (millis() - intervalWait < 60000) {
    if (Serial.available() > 0) {
    intervalBuffer = Serial.parseInt();
    savedAlarmInterval.write(intervalBuffer);
    Serial.print("Updated alarm interval: ");
    Serial.println(savedAlarmInterval.read());
    break;
    }
  }
  
}

float readRTCTemp() {
  float temp;
  rtc.convertTemperature();
  temp = rtc.getTemperature();
  return temp;
}

int dayOfWeek(uint16_t YYYY, uint8_t MM, uint8_t DD)
{
  //  for context, this computation assumes that 'start' of the gregorian calendar; January 1st, year '0000', is a Saturday.
  uint16_t months[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};   
  uint32_t days = YYYY * 365;                                           //  days until year 
  for (uint16_t i = 4; i < YYYY; i += 4) if (LEAP_YEAR(i) ) days++;     //  adjust leap years, test only multiple of 4 of course
  days += months[MM-1] + DD;                                            //  computes for the day in the year
  if ((MM > 2) && LEAP_YEAR(YYYY)) days++;                              //  adjust 1 if this year is a leap year, but only after febr
  //  removes all multiples of 7 and compute for day offset so count (zero index) begins on a SUNDAY instead of Saturday
  //  why? why not?
  return ((days + 6) % 7);   

}

void setResetFlag(uint8_t hourAlarm, uint8_t minuteAlarm) {
  DateTime checkTime = rtc.now();
  char sendNotif[100];
  if (!hhAlarm.read() == 0 && !mmAlarm.read() == 0) if ((checkTime.hour() == hhAlarm.read()) && (checkTime.minute() == mmAlarm.read())) alarmResetFlag = true;
  else if ((checkTime.hour() == hourAlarm) && (checkTime.minute() == minuteAlarm)) alarmResetFlag = true;
}

void printDateTime() {
  char timestring[100] = "INVALID";
  // getTimeStamp(_timestamp, sizeof(_timestamp));
  const char * monthsEq[12] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec"};
  DateTime now = rtc.now();
  if (now.month()-1 <= 12) sprintf(timestring, "%s %d,%d %02d:%02d:%02d",monthsEq[now.month()-1],now.date(),now.year(),now.hour(),now.minute(),now.second());
  debugPrintln(timestring);
}

