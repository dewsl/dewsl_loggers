#define LEAP_YEAR(Y)     ( (Y>0) && !(Y%4) && ( (Y%100) || !(Y%400) ))

/// RTC initialization
void RTCInit(uint8_t RTCPin) {
  pinMode(RTCPin, INPUT);
  REG_EIC_INTFLAG = EIC_INTFLAG_EXTINT4;    // clears interrupt flag
  attachInterrupt(digitalPinToInterrupt(RTCPin), RTCISR, FALLING);
}

/// RTC iterrupt service routine function
void RTCISR() {
  debugPrintln("RTC interrupt");
  int countDownMS = Watchdog.enable(16000);  // max of 16 seconds
  if (listenMode.read() && savedDataLoggerMode.read() == ROUTERMODE) return;  //gate
  operationFlag = true;  
}

/// Accepts sting input of date time;
/// Accepted format: YYYY,MM,DD,hh,mm,ss,dd[0-6]Mon-Sun
void setupTime() {
  resetWatchdog();
  unsigned long setupStart = millis();
  int MM = 0, DD = 0, YY = 0, hh = 0, mm = 0, ss = 0, dd = 0;
  Serial.println(F("\nSet time and date in this format: YYYY,MM,DD,hh,mm,ss,dd[0-6]Sun-Sat"));
  
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
    resetWatchdog();
    return;
    }
  }
}

/// Changes the saved date time with input paramters
/// @param year - current year; format YYYY
/// @param month - current month; format MM
/// @param date - current date; format DD
/// @param hour - current hour; format 24hr hh
/// @param mim - current minute; format mm
/// @param sec - current second; format ss (you can try..)
/// @param weekday - current day of week; format [0-6], where 0=Mon, 1=Tue, 3=Wed,...
void setRTCDateTime(int year, int month, int date, int hour, int min, int sec, int weekday) {
  DateTime dt(year, month, date, hour, min, sec, weekday);
  rtc.setDateTime(dt);  // adjust date-time as defined by 'dt'
}

/// Fetches current timestamp of RTC and stores it the container parameter
/// Output is formatted as "YYMMDDhhmmss"
/// Output string is used as timestamp of sensor data
/// @param tsContainer - container of timetamp string
/// @param sizeOfContainer - size of timestamp container [tsContainer]
void getTimeStamp(char* tsContainer, uint8_t sizeOfContainer) {
  for (int t=0; t < sizeOfContainer; t++) tsContainer[t] = 0x00;
  DateTime now = rtc.now();  //get the current date-time
  sprintf(tsContainer, "%02d%02d%02d%02d%02d%02d", now.year()%1000,now.month(),now.date(),now.hour(),now.minute(),now.second());
}

/// generates a timestring similar to cellular network time format 22/09/23,18:38:19
/// Output string is used for updating timestamp of routers
/// @param tsContainer - container of network formatted timetamp
/// @param sizeOf
void getNetworkFormatTimeStamp(char* tsContainer, uint8_t sizeOfContainer) {
  // add function for generating easy to read date time 
  for (int t=0; t < sizeOfContainer; t++) tsContainer[t] = 0x00;
  DateTime now = rtc.now();  //get the current date-time
  sprintf(tsContainer, "%02d/%02d/%02d,%02d:%02d:%02d", now.year()%1000,now.month(),now.date(),now.hour(),now.minute(),now.second());

}


/// Sets next alarm of RTC depending on the the interval equivalent value of the parameter
/// Actual alrm interval is computed based on current minute and interval equivalent of parameter value
/// @param IntervalEquivalent - interval equivalent value [0-5]; not the actual alarm interval value
void setNextAlarm(int intervalEquivalent) {
  char tsBuf[30];
  uint16_t store_rtc = 00;
  DateTime now = rtc.now();  //get the current date-time
  store_rtc = nextAlarm((int)(now.minute()),intervalEquivalent);
  // enable rtc interrupt
  if (debugMode) Serial.print("Next alarm\t hh:");
  if (debugMode) Serial.println(store_rtc);  
  if (!debugMode) {
    rtc.clearINTStatus();
    delayMillis(100);
    rtc.enableInterrupts(store_rtc, 00);  // interrupt/wake at (minutes, seconds)
  }
}

/// Computes actual alarm interval depending on 
uint8_t nextAlarm(int currentMinute, int intervalEquivalent) {
  resetWatchdog();
  int actualInterval = 0;
  uint16_t nextMinuteAlarm = 0;
     
  if (intervalEquivalent == 1) {
    actualInterval = 15;
  } else if (intervalEquivalent == 2) {
    actualInterval = 10;
  } else if (intervalEquivalent == 3) {
    actualInterval = 5;
  } else if (intervalEquivalent == 4) {
    actualInterval = 3;
  } else if (intervalEquivalent == 5) {      // special case intervals for 15min offset
    if (currentMinute < 15 || currentMinute >= 45) nextMinuteAlarm = 15;
    else nextMinuteAlarm = 45;
    resetWatchdog();
    return nextMinuteAlarm;                         // skips the rest of the function 
  } else {
    actualInterval = 30;
  }

  int computedAlarm = (((currentMinute/actualInterval)+1)*actualInterval);  // next minute alarm computation for regular intervals starting and ending in minute 0 [intervals 1-4]
  if (computedAlarm >= 60) {
    nextMinuteAlarm = 0;
  } else {
    nextMinuteAlarm = computedAlarm;
  }
  resetWatchdog();
  return nextMinuteAlarm;
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

void setShortSleepInterval() {
  int sleepIntervalBuffer = 0;
  unsigned long inputWait = millis();
  Serial.print("Enter alarm settings: ");
  while (millis() - inputWait < 60000) {
    if (Serial.available() > 0) {
      sleepIntervalBuffer = Serial.parseInt();
      savedShortSleepInterval.write(sleepIntervalBuffer);
      Serial.print("Updated short sleep interval: ");
      Serial.println(savedShortSleepInterval.read());
      break;
    }
  }
  
}

float readRTCTemp() {
  resetWatchdog();
  getTimeStamp(_timestamp, sizeof(_timestamp));
  float temp = 0;
  
  //  prevents the code from being stuck when no rtc is not connected or unusable
  //  temporay check for timestamp validity;
  //  checks for the current decade of the year in the timestamp
  //  this should work until 2029..
  //  ..change it to == '3' afterwards
  if (_timestamp[0] == '2') {
    rtc.convertTemperature();
    temp = rtc.getTemperature();
  }
  return temp;
}

int dayOfWeek(uint16_t YYYY, uint8_t MM, uint8_t DD)  {
  //  for context, this computation assumes that 'start' of the gregorian calendar; January 1st, year '0000', is a Saturday.
  resetWatchdog();
  uint16_t months[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};   
  uint32_t days = YYYY * 365;                                           //  days until year 
  for (uint16_t i = 4; i < YYYY; i += 4) if (LEAP_YEAR(i) ) days++;     //  adjust leap years, test only multiple of 4 of course
  days += months[MM-1] + DD;                                            //  computes for the day in the year
  if ((MM > 2) && LEAP_YEAR(YYYY)) days++;                              //  adjust 1 if this year is a leap year, but only after febr
  //  removes all multiples of 7 and compute for day offset so count (zero index) begins on a SUNDAY instead of Saturday
  //  why? why not?
  resetWatchdog();
  return ((days + 6) % 7);   
}

void setResetAlarmTime() {
  resetWatchdog();
  int alarmBuffer = 0;
  unsigned long intervalWait = millis();
  Serial.println("***To ensure datalogger will RESET itself..");
  Serial.println("***Make sure that the RESET ALARM to be set will converge with WAKE INTERVAL ALARM time.");
  Serial.println("Invalid values will default to \"0000\"");
  Serial.print("Enter new alarm time (e.g. 2300 for 11PM): ");
  
  while (millis() - intervalWait < 60000) {
    if (Serial.available() > 0) {
    alarmBuffer = Serial.parseInt();
    if (alarmBuffer > 2400) {
      Serial.println("Invalid value");
      alarmBuffer = 0;
      return;
    }
    savedLoggerResetAlarm.write(alarmBuffer);
    Serial.println(savedLoggerResetAlarm.read());
    // Serial.print("Updated Reset alarm time: ");
    
    break;
    }
  resetWatchdog();
  }
}

void setSelfResetFlag(int alarm24hrFormat) {
  resetWatchdog();
  int savedAlarm = alarm24hrFormat;
  if (savedAlarm > 2400) savedAlarm == 0; 
  // with input example "2330" (or 11:30PM)
  // first part should get first 2 digits (hour) from the left of 24h format input.
  // extracting "23" from "2330"
  uint8_t hourAlarm = (savedAlarm/100);      
  // second part should get last 2 digits (minute) from the left of 24h format input.
  // extracting "30" from "2330"
  uint8_t minuteAlarm = (savedAlarm%100);    
  DateTime checkTime = rtc.now();
  if ((checkTime.hour() == hourAlarm) && (checkTime.minute() == minuteAlarm)) selfResetFlag = true;
  resetWatchdog();
}

void printDateTime() {
  resetWatchdog();
  char timestring[100] = "INVALID";
  // getTimeStamp(_timestamp, sizeof(_timestamp));
  const char * monthsEq[12] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec"};
  const char * daysEq[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  const char * timeOfDayEq[2] = {"AM", "PM"};
  uint8_t timeOfDayIndex = 0; // defaults to AM ddaytime indicator
  DateTime now = rtc.now();
  uint8_t hourBuffer = now.hour();
  
  if (hourBuffer > 11 && hourBuffer < 24) timeOfDayIndex = 1;                   //  sets PM as daytime indicator
  if (hourBuffer > 12 && hourBuffer < 24) hourBuffer = hourBuffer - 12;         //  subtract 12 from 24hr format to get 12hr format
  if (hourBuffer == 0) hourBuffer = 12;                                         //  replace midnight time 00 to 12
  if (now.month()-1 <= 12) sprintf(timestring, "%s %s %d, %d", daysEq[now.dayOfWeek()-1],monthsEq[now.month()-1],now.date(),now.year());  //  generate day and data string
  debugPrint("Current date:\t ");
  debugPrintln(timestring);
  if (inputIs(timestring, "INVALID")) sprintf(timestring, "%d:%02d:%02d %s [RTC_ERR]" ,hourBuffer,now.minute(),now.second(),timeOfDayEq[timeOfDayIndex]);
  else sprintf(timestring, "%d:%02d:%02d %s" ,hourBuffer,now.minute(),now.second(),timeOfDayEq[timeOfDayIndex]);                             // generte timestring & daytime indicator
  debugPrint("Current time:\t ");
  debugPrintln(timestring);
  resetWatchdog();
}


// Compares current [stored] timestamp with newTiemstamp for datetime updating
// Used when updateing timestamp using GPRS time
void timestampUpdate(const char* newTimestamp) {
  resetWatchdog();
  bool updateTS = false;
  char timeBuffer[50];
  uint8_t tsIndex = 0;
  sprintf(timeBuffer, "%s",newTimestamp); // expected format == 24/06/07,19:20:05+32
  DateTime now = rtc.now();
  
  char * subSegment = strtok(timeBuffer,"/,:+");
  while (subSegment != NULL) {

    //if filters
    if (tsIndex == 0 && now.year()%1000 != atoi(subSegment)) updateTS = true;     // update TS if the year does not match
    if (tsIndex == 1 && now.month() != atoi(subSegment)) updateTS = true;         // update TS if the month does not match
    if (tsIndex == 2 && now.date() != atoi(subSegment)) updateTS = true;          // update TS if the date does not match
    if (tsIndex == 3 && now.hour() != atoi(subSegment)) updateTS = true;          // update TS if the hour does not match
    if (tsIndex == 4 && now.minute() != atoi(subSegment)) updateTS = true;        // update TS if the minute does not match

    subSegment = strtok(NULL,"/,:+");
    tsIndex++;
  }  

  // 
  if (updateTS) {
    updateTsNetworkFormat(newTimestamp);
  }
  resetWatchdog();
  // now.year()%1000,now.month(),now.date(),now.hour(),now.minute(),now.second()
}

/// Return true if current [stored] timestamp is within +-2 min (approx) of the parameter newTiemstampReference
/// Otherwise, returns false
/// @param newTimestampReference refetence timestamp for comparison
/// 
bool checkTimeSync(const char* newTimestampReference) {
  resetWatchdog();
  bool timeOk = false;
  debugPrint("Checking time sync..");
  getTimeStamp(_timestamp, sizeof(_timestamp));
  long referenceTime = strtol(newTimestampReference,0,10);
  long currentTime = strtol(_timestamp,0,10);
  long timeDiffRep = referenceTime - currentTime;
  // debugPrint("reference: ");
  // debugPrintln(referenceTime);
  // debugPrint("current: ");
  // debugPrintln(currentTime);
  // debugPrint("time diff: ");
  // debugPrintln(timeDiffRep);
  if (timeDiffRep < 200 && timeDiffRep > -200) timeOk = true;    // This is not the actual time diff only representative
  else timeOk = false;
  resetWatchdog();
  return timeOk;
  // now.year()%1000,now.month(),now.date(),now.hour(),now.minute(),now.second()
}

/// Updates the device timestamps with the the format 22/09/23,18:38:19+08 from the variable networkTimeString
/// 
/// @param networkTimeString - container of timestamp to be used for device ts update 
///
void updateTsNetworkFormat(const char * networkTimeString) {
  resetWatchdog();
  char timeStringBuffer[50];
  sprintf(timeStringBuffer, "%s",networkTimeString); //copy it to a new variable container because we will tokenizze this
  int ts_buffer[7];
  char *ts_token = strtok(timeStringBuffer, ",/:+");  

  byte ts_counter = 0;
  while (ts_token != NULL) {
    ts_buffer[ts_counter] = atoi(ts_token);
    ts_counter++;
    ts_token = strtok(NULL, ",/:+");
  }

  // debugPrintln(timebuffer);
  ts_buffer[6] = dayOfWeek((2000+ts_buffer[0]),ts_buffer[1],ts_buffer[2]); // attempt to get correct weekday data
  setRTCDateTime(ts_buffer[0], ts_buffer[1], ts_buffer[2], ts_buffer[3], ts_buffer[4], ts_buffer[5], ts_buffer[6]);

  debugPrintln("Timestamp updated!");
  resetWatchdog();
}


/// Updates saved timestamp using parameter.
/// @param tsDataString - reference timestamp
void updateTsDataFormat(const char * tsDataString) {
  char tsDataBuffer[20];
  char tempHolder[3];
  byte YY = 0, MM = 0, DD = 0, hh = 0, mm = 0, ss = 0, dd = 0;
  sprintf(tsDataBuffer, "%s", tsDataString);
  debugPrint("Updating saved timestamp..");
  //  convert by brute force
  tempHolder[0] = tsDataBuffer[0]; tempHolder[1] = tsDataBuffer[1]; tempHolder[2] = 0x00; YY = atoi(tempHolder);
  tempHolder[0] = tsDataBuffer[2]; tempHolder[1] = tsDataBuffer[3]; tempHolder[2] = 0x00; MM = atoi(tempHolder);
  tempHolder[0] = tsDataBuffer[4]; tempHolder[1] = tsDataBuffer[5]; tempHolder[2] = 0x00; DD = atoi(tempHolder);
  tempHolder[0] = tsDataBuffer[6]; tempHolder[1] = tsDataBuffer[7]; tempHolder[2] = 0x00; hh = atoi(tempHolder);
  tempHolder[0] = tsDataBuffer[8]; tempHolder[1] = tsDataBuffer[9]; tempHolder[2] = 0x00; mm = atoi(tempHolder);
  tempHolder[0] = tsDataBuffer[10]; tempHolder[1] = tsDataBuffer[11]; tempHolder[2] = 0x00; ss = atoi(tempHolder);

  dd = dayOfWeek((2000+YY),MM,DD);
  setRTCDateTime(YY, MM, DD, hh, mm, ss, dd);
  getTimeStamp(_timestamp, sizeof(_timestamp));
  debugPrint("Timestamp updated: ");
  debugPrintln(_timestamp);
}