void rtcInit(byte RTC_INT_PIN) {
  // pinMode(RTC_INT_PIN, INPUT_PULLUP);
  // attachInterrupt(RTC_INT_PIN, alarmISR, FALLING);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
  // REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_DBIAS_WAK, 4);
  // REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_DBIAS_SLP, 4);
  // esp_sleep_enable_ext1_wakeup_io(BUTTON_PIN_BITMASK(GPIO_NUM_13), ESP_EXT1_WAKEUP_ANY_HIGH);
  rtc_gpio_pulldown_dis(GPIO_NUM_13);
  rtc_gpio_pullup_en(GPIO_NUM_13);
  
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


/// Incase rtc loses power, time is set to the datetime that the code is compiled
/// This is just a fall back measure to prevent invalid timstamps
void rtcFallback() {
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  
  }
}

void setupTime() {
  unsigned long setupStart = millis();
  int MM = 0, DD = 0, YY = 0, hh = 0, mm = 0, ss = 0, dd = 0;
  Serial.println(F("\nSet time and date in this format: YYYY,MM,DD,hh,mm,ss"));
  
  while (millis() - setupStart < 60000) {
    if (Serial.available() > 0) {
    YY = Serial.parseInt();
    MM = Serial.parseInt();
    DD = Serial.parseInt();
    hh = Serial.parseInt();
    mm = Serial.parseInt();
    ss = Serial.parseInt();
    // dd = Serial.parseInt();
    
    delayMillis(10);
    setRTCDateTime(YY, MM, DD, hh, mm, ss);
    // Serial.print("Current timestamp: ");
    getTimeStamp(_timestamp, sizeof(_timestamp));
    // Serial.println(_timestamp);
    Serial.println();
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