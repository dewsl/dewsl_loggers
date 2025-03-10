#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
SFE_UBLOX_GNSS myGNSS;

#define BUFLEN (5*RH_RF95_MAX_MESSAGE_LEN) //max size of data burst we can handle - (5 full RF buffers) - just arbitrarily large
#define RFWAITTIME 500 //maximum milliseconds to wait for next LoRa packet - used to be 600 - may have been too long
#define RTCM_TIMEOUT 240000 //4 minutes

char siteCode[10]; //logger name - sensor site code
int MIN_SAT = 20;
int AVE_COUNT = 12;

bool READ_FLAG = false;
uint8_t RX_LORA_FLAG = 0;
unsigned long start;

void init_ublox() {
  DUESerial.begin(BAUDRATE);
  Wire.begin();
  resetWatchdog();

  for (int x = 0; x < 10; x++) {        //10 retries to not exceed watchdog limit(16sec)
    if (myGNSS.begin(Wire) == false) {
      Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
      delay(1000);
    } else {
      Serial.println("u-blox GNSS begin");
      break;
    }
  }

  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.setNavigationFrequency(5); //Set output to 20 times a second
  myGNSS.setHighPrecisionMode(true);  
  myGNSS.powerSaveMode(true);
  initialize_sitecode(siteCode);
  resetWatchdog();
}

byte checkRTKFixType() {
  byte RTK = myGNSS.getCarrierSolutionType();
  Serial.print("RTK: ");
  Serial.print(RTK);
  if (RTK == 0) Serial.println(F(" (No solution)"));
  else if (RTK == 1) Serial.println(F(" (High precision floating fix)"));
  else if (RTK == 2) Serial.println(F(" (High precision fix)"));
  return RTK;
}

byte checkSatelliteCount() {
  byte SIV = myGNSS.getSIV();
  Serial.print("Sat #: ");
  Serial.println(SIV);
  return SIV;
}

float checkHorizontalAccuracy() {
  float HACC = myGNSS.getHorizontalAccuracy();
  Serial.print("Horizontal Accuracy: ");
  Serial.println(HACC);
  return HACC;
}

float checkVerticalAccuracy() {
  float VACC = myGNSS.getVerticalAccuracy();
  Serial.print("Vertical Accuracy: ");
  Serial.println(VACC);
  return VACC;
}

void getRTCM() {
  rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128);   //lora config for send/receive rtcm
  uint8_t buf[BUFLEN];
  unsigned buflen;

  uint8_t rfbuflen;
  uint8_t *bufptr;
  unsigned long lastTime;

  bufptr = buf;
  if (rf95.available()) {
    digitalWrite(LED_BUILTIN, HIGH);
    rfbuflen = RH_RF95_MAX_MESSAGE_LEN;
    if (rf95.recv(bufptr, &rfbuflen)) {
      bufptr += rfbuflen;
      lastTime = millis();
      while (((millis() - lastTime) < RFWAITTIME) && ((bufptr - buf) < (BUFLEN - RH_RF95_MAX_MESSAGE_LEN))) { //Time out or buffer can't hold anymore
        if (rf95.available()) {
          rfbuflen = RH_RF95_MAX_MESSAGE_LEN;
          if (rf95.recv(bufptr, &rfbuflen)) {
            Serial.println((unsigned char) *bufptr, HEX);
            bufptr += rfbuflen;
            lastTime = millis();
          } else {
            Serial.println("Receive failed");
          }
        }
      }
    } else {
      Serial.println("Receive failed");
    }
    buflen = (bufptr - buf);     //Total bytes received in all packets
    DUESerial.write(buf, buflen); //Send data to the GPS
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void getGNSSData(char *dataToSend, unsigned int bufsize) {

  char dataBuffer[bufsize];
  READ_FLAG = false;
  RX_LORA_FLAG = 0;

  init_ublox();
  resetWatchdog();
  start = millis();

  do {
    resetWatchdog();
    getRTCM();
  } while (((checkRTKFixType() != 2) || checkSatelliteCount() < MIN_SAT) && ((millis() - start) < RTCM_TIMEOUT)); 

  if (checkRTKFixType() == 2 && checkSatelliteCount() >= MIN_SAT) {
    if (RX_LORA_FLAG == 0) {
      resetWatchdog();
      readUbloxData(dataBuffer, sizeof(dataBuffer), siteCode);
      dataBuffer[strlen(dataBuffer)]=0x00;
      RX_LORA_FLAG == 1;
      READ_FLAG = true;
      resetWatchdog();
    }
  } else if (((checkRTKFixType() != 2) || (checkSatelliteCount() < MIN_SAT)) && ((millis() - start) >= RTCM_TIMEOUT)) {
    Serial.println("Unable to obtain fix or no. of satellites reqd. not met");
    resetWatchdog();
    noGNSSDataAcquired(dataBuffer, sizeof(dataBuffer), siteCode);
    dataBuffer[strlen(dataBuffer)]=0x00; // not needed 
    RX_LORA_FLAG == 1;
    READ_FLAG = true;
    resetWatchdog();
  } 

  if (READ_FLAG = true) {
    rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128);
    resetWatchdog();
    READ_FLAG = false;
    RX_LORA_FLAG == 0;

    getTimeStamp(_timestamp, sizeof(_timestamp));
    strncat(dataBuffer, "*", 2);
    strncat(dataBuffer, _timestamp, strlen(_timestamp));
    resetWatchdog();

    if (loggerWithGSM(savedDataLoggerMode.read())) {       //GSM Modes
      //If string start '>>', remove 1st and 2nd character data in string. Not needed in GSM mode
      if (strstr(dataBuffer, ">>")) {
        for (byte i = 0; i < strlen(dataBuffer); i++) {
        dataBuffer[i] = dataBuffer[i + 2];
        }
      }
    }
    sprintf(dataToSend, dataBuffer);
    resetWatchdog();
  }
}

void readUbloxData(char * ubloxDataContainer, int containerSize, char* sitecode) {
  resetWatchdog();
  for (int i = 0; i < containerSize; i++) {
    ubloxDataContainer[i] = (uint8_t)'\0';
  }

  byte rtk_fixtype = checkRTKFixType();
  int sat_num = checkSatelliteCount();

  // Defines storage for the lat and lon as double
  double d_lat = 0.0; // latitude
  double d_lon = 0.0; // longitude

  double accu_lat = 0.0; // latitude accumulator
  double accu_lon = 0.0; // longitude accumulator
  int accu_count = 0;

  // Now define float storage for the heights and accuracy
  float f_msl = 0.0;
  float f_accuracy_hor = 0.0;
  float f_accuracy_ver = 0.0;

  float accu_msl = 0.0;           //msl accumulator
  float accu_accuracy_hor = 0.0;  //hacc acuumulator
  float accu_accuracy_ver = 0.0;  //vacc accumulator

  resetWatchdog();

  for (int i = 1; i <= AVE_COUNT; i++) {
    if ((millis() - start) < RTCM_TIMEOUT) {
      resetWatchdog();
      getRTCM();

      // First, let's collect the position data
      int32_t latitude = myGNSS.getHighResLatitude();
      int8_t latitudeHp = myGNSS.getHighResLatitudeHp();
      int32_t longitude = myGNSS.getHighResLongitude();
      int8_t longitudeHp = myGNSS.getHighResLongitudeHp();
      int32_t msl = myGNSS.getMeanSeaLevel();
      int8_t mslHp = myGNSS.getMeanSeaLevelHp();
      uint32_t hor_acc = myGNSS.getHorizontalAccuracy();
      uint32_t ver_acc = myGNSS.getVerticalAccuracy();

      // Assemble the high precision latitude and longitude
      d_lat = ((double)latitude) / 10000000.0; // Convert latitude from degrees * 10^-7 to degrees
      d_lat += ((double)latitudeHp) / 1000000000.0; // Now add the high resolution component (degrees * 10^-9 )
      d_lon = ((double)longitude) / 10000000.0; // Convert longitude from degrees * 10^-7 to degrees
      d_lon += ((double)longitudeHp) / 1000000000.0; // Now add the high resolution component (degrees * 10^-9 )

      // Calculate the height above mean sea level in mm * 10^-1
      f_msl = (msl * 10) + mslHp;  // Now convert to m
      f_msl = f_msl / 10000.0; // Convert from mm * 10^-1 to m

      // Convert the accuracy (mm * 10^-1) to a float
      f_accuracy_hor = hor_acc / 10000.0; // Convert from mm * 10^-1 to m
      f_accuracy_ver = ver_acc / 10000.0; // Convert from mm * 10^-1 to m
      resetWatchdog();

      if ((checkHorizontalAccuracy() == 141 && checkVerticalAccuracy() <= 141)) {
        // Accumulation
        resetWatchdog();
        accu_lat += d_lat;
        accu_lon += d_lon;
        accu_msl += f_msl;
        accu_accuracy_hor += f_accuracy_hor;
        accu_accuracy_ver += f_accuracy_ver;
        accu_count++;
        Serial.print("accu_count: ");
        Serial.println(accu_count);
        resetWatchdog();
      } else {
        i--; //loop until hacc&vacc conditions are satisfied or until timeout reached
        resetWatchdog();
        getRTCM();
      }
    } else if ((millis() - start) >= RTCM_TIMEOUT) {
      resetWatchdog();
      Serial.println("Timeout reached!");
      break;
    }
  }

  // Averaging
  resetWatchdog();
  d_lat = accu_lat / accu_count; 
  d_lon = accu_lon / accu_count;
  f_msl = accu_msl / accu_count; 
  f_accuracy_hor = accu_accuracy_hor / accu_count;
  f_accuracy_ver = accu_accuracy_ver / accu_count;
  resetWatchdog();

  if ((d_lat > 0) || (d_lon > 0)) {
    resetWatchdog();
    sprintf(ubloxDataContainer,
            ">>%s:%d,%.9f,%.9f,%.4f,%.4f,%.4f,%d,%.2f,%.2f",
            sitecode, rtk_fixtype, d_lat, d_lon, f_accuracy_hor, f_accuracy_ver, f_msl, sat_num, readRTCTemp(), readBatteryVoltage(1));
    // strncpy(dataToSend, tempstr, strlen(tempstr) + 1);
    // strncat(dataToSend, ",", 2);
    // strncat(dataToSend, temp, sizeof(temp));
    // strncat(dataToSend, ",", 2);
    // strncat(dataToSend, volt, sizeof(volt)); 
    resetWatchdog();
    Serial.print("Data to send: "); 
    Serial.println(ubloxDataContainer);
  } else {
    resetWatchdog();
    noGNSSDataAcquired(ubloxDataContainer, sizeof(ubloxDataContainer), siteCode);
  }
  // resetWatchdog();
  d_lat, d_lon, f_msl, f_accuracy_hor, f_accuracy_ver = 0.0;
  accu_lat, accu_lon, accu_msl, accu_accuracy_hor, accu_accuracy_ver = 0.0;   //reset accumulators to zero
  accu_count = 0;
  // resetWatchdog();
}

void noGNSSDataAcquired(char* msgContainer, int containerSize, char* sitecode) {
  for (int i = 0; i < containerSize; i++) {
    msgContainer[i] = 0x00;;
  }
  sprintf(msgContainer, ">>%s:No Ublox data.", sitecode);

  // strncpy(dataToSend, ">>", 3); 
  // strncat(dataToSend, sitecode, sizeof(sitecode));
  // strncat(dataToSend,":No Ublox data.", 16); 
  Serial.print("data to send: "); 
  Serial.println(msgContainer);
}

void initialize_sitecode(char* siteCodeContainer) {
  uint8_t mode = savedDataLoggerMode.read();
  uint8_t siteIndex = 0;  // Default index for standalone mode

  // Determine site index dynamically based on Ublox and Subsurface presence
  if (mode == GATEWAYMODE || mode == ARQMODE) {
    if (hasSubsurfaceSensorFlag.read() == 99) siteIndex++; // Shift further if Subsurface sensor is present
    if ((hasSubsurfaceSensorFlag.read() == 0) && (hasUbloxRouterFlag.read() ==99)) siteIndex++;
  }

  sprintf(siteCodeContainer, "%s", flashLoggerName.sensorNameList[siteIndex]);
  siteCodeContainer[strlen(siteCodeContainer)] = 0x00; // Null-terminate
}