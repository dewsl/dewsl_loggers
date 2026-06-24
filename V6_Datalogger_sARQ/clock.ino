void rtcInit(byte RTC_INT_PIN) {
  pinMode(RTC_INT_PIN, INPUT);
  attachInterrupt(RTC_INT_PIN, RTCISR, FALLING);
  if (!rtc.begin()) {                             
    debugPrintln("RTC module ERROR");
    delayMillis(1000);
  } else debugPrintln("RTC init OK");
  
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
  // rtc_gpio_pulldown_dis(GPIO_NUM_13);
  // rtc_gpio_pullup_en(GPIO_NUM_13);
}

void delayMillis(int delayDuration) {
  int maxDelayDuration = 15000;  //15sec
  bool maxDurationReached = false;
  unsigned long timeStart = millis();
  while (!maxDurationReached) {
    if ((millis() - timeStart) > maxDelayDuration) {
      debugPrintln("Max delay duration: 15000");
      return;
    }
    if ((millis() - timeStart) > delayDuration) {
      return;
    }
  }
}


void dateTimeNow() {
  DateTime now = rtc.now();
  char timeString[100];
  const char * dayPeriod = (now.hour() < 12) ? "AM" : "PM";
  const char * dayNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  const char * monthNames[] = {"","Jan","Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  uint8_t hour12f = (now.hour() % 12) == 0 ? 12 : (now.hour() % 12) ;
  debugPrint("RTC Date Time: ");
  sprintf(timeString, "  %s %d %s %d %02d:%02d:%02d %s",dayNames[now.dayOfTheWeek()],now.day(),monthNames[now.month()],now.year(),hour12f,now.minute(),now.second(),dayPeriod);
  debugPrintln(timeString);
}

void timeNow() {
  DateTime now = rtc.now();
  char timeString[100];
  debugPrint("Current time: ");
  sprintf(timeString, "%02d:%02d:%02d",now.hour(),now.minute(),now.second());
  debugPrintln(timeString);
}

// this part uses two registers instead of one
void set2Alarms(uint8_t minuteAlarm1, uint8_t minuteAlarm2) {
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

// arms the primary alarm (1)
// also clears the second alarm (2); this assumes only alarm 1 will be used
void setAlarm1(uint8_t minuteAlarm1) {

  rtc.armAlarm(1, false);                                 // disarm alarm 1
  rtc.clearAlarm(1);
  rtc.alarmInterrupt(1, false);

  rtc.armAlarm(2, false);                                 // disarm alarm 2
  rtc.clearAlarm(2);
  rtc.alarmInterrupt(2, false);
  
  rtc.writeSqwPinMode(DS3231_OFF);                        // turn off square wave output
  
  rtc.setAlarm(ALM1_MATCH_MINUTES, minuteAlarm1, 0, 0);   //write alarm setting to alarm 1 register
  rtc.alarmInterrupt(1, true);                            // arm alarm 1
}

void updateAlarmInterval() {

}

/// Syncs RTC with date & time that the code is compiled
/// Incase rtc loses power, time is reset compile time
/// This is just a fall back measure to prevent invalid timstamps
void syncRTCwithCompileTime() {
  if (rtc.lostPower()) {
    debugPrintln("RTC lost POWER! Timestamp will be set to compile time.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void setupTime() {
  unsigned long setupStart = millis();
  int YY = 0, MM = 0, DD = 0, hh = 0, mm = 0, ss = 0;
  bool inputCaptured = false;

  debugPrintln("\nSet time and date in this format: YYYY,MM,DD,hh,mm,ss");

  while (millis() - setupStart < 60000) {

    if (Serial.available() > 0) {
      YY = Serial.parseInt();
      MM = Serial.parseInt();
      DD = Serial.parseInt();
      hh = Serial.parseInt();
      mm = Serial.parseInt();
      ss = Serial.parseInt();
      inputCaptured = true;
    } 
    else if (BTSerial.available() > 0) {
      YY = BTSerial.parseInt();
      MM = BTSerial.parseInt();
      DD = BTSerial.parseInt();
      hh = BTSerial.parseInt();
      mm = BTSerial.parseInt();
      ss = BTSerial.parseInt();
      inputCaptured = true;
    }

    if (inputCaptured) {
      if (YY >= 2020 && MM >= 1 && MM <= 12 && DD >= 1 && DD <= 31 &&
          hh >= 0 && hh <= 23 && mm >= 0 && mm <= 59 && ss >= 0 && ss <= 59) {

        setRTCDateTime(YY, MM, DD, hh, mm, ss);
        getTimeStamp(_timestamp, sizeof(_timestamp));

        debugPrint("RTC updated. New timestamp: ");
        debugPrintln(_timestamp);
      } else {
        debugPrintln("Invalid date/time input. RTC not updated.");
      }

      return;
    }

    delayMillis(10);
  }

  debugPrintln("RTC update timed out. RTC not updated.");
}

/// Changes the saved date time with input paramters
/// @param year - current year; format YYYY
/// @param month - current month; format MM
/// @param date - current date; format DD
/// @param hour - current hour; format 24hr hh
/// @param mim - current minute; format mm
/// @param sec - current second; format ss (you can try..)
/// @param weekday - current day of week; format [0-6], where 0=Mon, 1=Tue, 3=Wed,...
void setRTCDateTime(int year, int month, int date, int hour, int min, int sec) {
  DateTime dt(year, month, date, hour, min, sec);
  rtc.adjust(dt);  // adjust date-time as defined by 'dt'
}


/// Fetches current timestamp of RTC and stores it the container parameter
/// Output is formatted as "YYMMDDhhmmss"
/// Output string is used as timestamp of sensor data
/// @param tsContainer - container of timetamp string
/// @param sizeOfContainer - size of timestamp container [tsContainer]
void getTimeStamp(char* tsContainer, uint8_t sizeOfContainer) {
  for (int t=0; t < sizeOfContainer; t++) tsContainer[t] = 0x00;
  DateTime now = rtc.now();  //get the current date-time
  sprintf(tsContainer, "%02d%02d%02d%02d%02d%02d", now.year()%1000,now.month(),now.day(),now.hour(),now.minute(),now.second());
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

/// Updates the device timestamps with the the format 22/09/23,18:38:19+08 from the variable networkTimeString
/// 
/// @param networkTimeString - container of timestamp to be used for device ts update 
///
void updateTsNetworkFormat(const char * networkTimeString) {
  char timeStringBuffer[50];
  sprintf(timeStringBuffer, "%s",networkTimeString); //copy it to a new variable container because we will tokenize this
  int ts_buffer[7];
  char *ts_token = strtok(timeStringBuffer, ",/:+");  

  byte ts_counter = 0;
  while (ts_token != NULL) {
    ts_buffer[ts_counter] = atoi(ts_token);
    ts_counter++;
    ts_token = strtok(NULL, ",/:+");
  }

  // debugPrintln(timebuffer);
  // ts_buffer[6] = dayOfWeek((2000+ts_buffer[0]),ts_buffer[1],ts_buffer[2]); // attempt to get correct weekday data
  setRTCDateTime(ts_buffer[0], ts_buffer[1], ts_buffer[2], ts_buffer[3], ts_buffer[4], ts_buffer[5]);

  debugPrintln("Timestamp updated!");
}


float readRTCTemp() {

  getTimeStamp(_timestamp, sizeof(_timestamp));
  float temp = 0;
  //  prevents the code from being stuck when no rtc is not connected or unusable
  //  temporay check for timestamp validity;
  //  checks for the current decade of the year in the timestamp
  //  this should work until 2029..
  //  ..change it to == '3' afterwards
  if (_timestamp[0] == '2') {
    // rtc.forceConversion();
    temp = rtc.getTemp();
  }
  return temp;
}

void getNetworkFormatTimeStamp(char* tsContainer, uint8_t sizeOfContainer) {
  // add function for generating easy to read date time 
  for (int t=0; t < sizeOfContainer; t++) tsContainer[t] = 0x00;
  DateTime now = rtc.now();  //get the current date-time
  sprintf(tsContainer, "%02d/%02d/%02d,%02d:%02d:%02d", now.year()%1000,now.month(),now.day(),now.hour(),now.minute(),now.second());
}

/// Sets next alarm of RTC depending on the the interval equivalent value of the parameter
/// Actual alrm interval is computed based on current minute and interval equivalent of parameter value
/// @param IntervalEquivalent - interval equivalent value [0-5]; not the actual alarm interval value, only an index
void setNextAlarm() {
  DateTime now = rtc.now();             //get the current date-time
  uint8_t intervalEquivalent = 30;      //  default value
  uint8_t savedInterval = fetchParam(paramStorage, ALARM_INTERVAL,(uint8_t)0);
  if (savedInterval == 0) {set2Alarms(0,30); return;} // multiple returns are not really advisable, but.. this keeps this part short, so yes... it stays 
  else if (savedInterval == 1) {set2Alarms(15,45); return;}
  else if (savedInterval == 2) intervalEquivalent = 15;
  else if (savedInterval == 3) intervalEquivalent = 10;
  else if (savedInterval == 4) intervalEquivalent = 5;
  setAlarm1(nextAlarmGen((int)(now.minute()),intervalEquivalent,0));   //  set computed next alarm to register    
}

//  identify next alarm minute depending on parameters
//  this should also work for intervals that doesn't start with 00 minute, provided that the interval series would return to the same number with the given interval
uint8_t nextAlarmGen(uint8_t currentMinute, uint8_t intervalEquivalent, uint8_t intervalStart) {
  uint8_t maxSteps = 60 / intervalEquivalent + 1;

  // If exactly on an alarm mark, move to next slot
  for (uint8_t i = 0; i < maxSteps; i++) {
    uint8_t candidate = (intervalStart + (i * intervalEquivalent)) % 60;
    uint8_t next = (intervalStart + ((i + 1) * intervalEquivalent)) % 60;

    if (candidate == currentMinute) {
      return next;
    }
  }

  // Find next future alarm
  for (uint8_t i = 0; i < maxSteps; i++) {
    uint8_t candidate = (intervalStart + (i * intervalEquivalent)) % 60;

    if (candidate > currentMinute) {
      return candidate;
    }
  }

  // Wrap to next hour
  return intervalStart;
}

void displayNextAlarm2(uint8_t alarmIndex) {
  DateTime now = rtc.now();
  uint8_t intervalStart = 0;   // default to 0
  uint8_t almInterval = 0;
  uint8_t currentHour = now.hour();
  uint8_t currentMinute = now.minute();

  switch (alarmIndex) {
    case 0: almInterval = 30; break;
    case 1: almInterval = 30; intervalStart = 15; break;
    case 2: almInterval = 15; break;
    case 3: almInterval = 10; break;
    case 4: almInterval = 5; break;
    default: almInterval = 30; break;
  }
  int offsetMinute = (currentMinute - intervalStart + 60) % 60;             //  get minute equivalent if offset is not zero
  int remainder = offsetMinute % almInterval;                               //  excess minute from last alarm
  int nextShifted = (offsetMinute - remainder + almInterval);               //  jump to next alarm if needed
  int nextMinute = (intervalStart + nextShifted) % 60;                      //  get next actual alarm minute 
  int nextHour = currentHour;                                               //  next hour if minute does not exceed next hour boundary
  if (nextShifted + intervalStart >= 60) {                                  //  next hour if hour and day boundary is exceeded 
    nextHour = (currentHour + 1) % 24;
  }

  const char* dayPeriod = (nextHour >= 12) ? "PM" : "AM";                   //  get day period so we can use 12hr format
  int displayHH = nextHour % 12;                                            //  convert 24hr format to 12
  if (displayHH == 0) displayHH = 12;                                       //  midnight is 00 not 24

  char nextBuffer[15];                                                       // container for "HH:MM AM" + null terminator
  snprintf(nextBuffer, sizeof(nextBuffer), "%02d:%02d %s", displayHH, nextMinute, dayPeriod);

  debugPrint("Next alarm:\t ");
  debugPrintln(nextBuffer);
}

void setAlarmInterval() {
  int intervalBuffer = 0;
  unsigned long intervalWait = millis();
  bool inputCapture = false;
  debugPrint("Enter alarm settings: ");
  while (millis() - intervalWait < 60000) {
    if (Serial.available() > 0) {     // usb serial instance
      intervalBuffer = Serial.parseInt();
      inputCapture = true;
    }
    if (BTSerial.available() > 0) {   // bluetooth instance.. keep separate
      intervalBuffer = BTSerial.parseInt();
      inputCapture = true;
    }
    if (inputCapture) {
      if (intervalBuffer > 4) {intervalBuffer = 0; debugPrintln("Alarm interval defaulted to 30 mins.");}  // prevent values outside index range and defaults to 30 min interval
      storeParam(paramStorage, ALARM_INTERVAL, (uint8_t)intervalBuffer);
      debugPrint("Updated alarm interval: ");
      debugPrintln(fetchParam(paramStorage, ALARM_INTERVAL,(uint8_t)0));
      inputCapture = false; // this is not necessary but keep it
      break;
    }
  }
}

void displayNextAlarm() {
  DateTime now = rtc.now();             //get the current date-time
  
  uint8_t intervalEquivalent = 30;      //  default value
  uint8_t intervalStart = 0;            //  default value
  uint8_t intervalIndex = fetchParam(paramStorage, ALARM_INTERVAL,(uint8_t)0);
  // if (fetchParam(paramStorage, ALARM_INTERVAL,(uint8_t)0) == 0) // this does nothins because it uses default values
  if (intervalIndex == 1) intervalStart = 15; 
  else if (intervalIndex == 2) intervalEquivalent = 15;
  else if (intervalIndex == 3) intervalEquivalent = 10;  
  else if (intervalIndex == 4) intervalEquivalent = 5;  // anything higher and it does nothing..
  debugPrint("Next alarm:\t hh:");
  debugPrintln(nextAlarmGen((int)(now.minute()),intervalEquivalent,intervalStart));
}

void setCompileTime() {
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("Time set to compile time!");
}

uint8_t intervalEquivalent(uint8_t alarmInterval) {
  uint8_t interval = 30;    //  default value
  switch (alarmInterval) {
    case 0: interval = 30; ; break;
    case 1: interval = 15; break;
    case 2: interval = 10; break;
    case 3: interval = 5; break;
    default: interval = 30; break;
  }
  return interval;
}
