#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
SFE_UBLOX_GNSS myGNSS;

#define BUFLEN (5*RH_RF95_MAX_MESSAGE_LEN) //max size of data burst we can handle - (5 full RF buffers) - just arbitrarily large
#define RFWAITTIME 500 //maximum milliseconds to wait for next LoRa packet - used to be 600 - may have been too long
#define rtcm_timeout 180000 //3 minutes

char sitecode[6]; //logger name - sensor site code
int min_sat = 30;
int ave_count = 12;

bool read_flag = false;
uint8_t rx_lora_flag = 0;
unsigned long start;

void init_ublox() {
  DUESerial.begin(BAUDRATE);
  Wire.begin();
  Watchdog.reset();
  if (myGNSS.begin(Wire) == false) {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    while (1);
  }
  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.setNavigationFrequency(5); //Set output to 20 times a second
  myGNSS.setHighPrecisionMode(true);  
  myGNSS.powerSaveMode(true);
  initialize_sitecode();
  Watchdog.reset();
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
  if (debug_flag == 0) {
    Watchdog.reset();
  }

  init_ublox(); 
  read_flag = false;
  rx_lora_flag = 0;
  Watchdog.reset();

  start = millis();

  do {
    Watchdog.reset();
    getRTCM();
  } while (((checkRTKFixType() != 2) || checkSatelliteCount() < min_sat) && ((millis() - start) < rtcm_timeout)); 

  if (checkRTKFixType() == 2 && checkSatelliteCount() >= min_sat) {
    if (rx_lora_flag == 0) {
      Watchdog.reset();
      readUbloxData();
      Watchdog.reset();
      rx_lora_flag == 1;
      read_flag = true;
    }
  } else if (((checkRTKFixType() != 2) || (checkSatelliteCount() < min_sat)) && ((millis() - start) >= rtcm_timeout)) {
    Serial.println("Unable to obtain fix or no. of satellites reqd. not met");
    Watchdog.reset();
    noGNSSDataAcquired();     
    rx_lora_flag == 1;
    read_flag = true;
    Watchdog.reset();
  } 

  if (read_flag = true) {
    Watchdog.reset();
    read_flag = false;
    rx_lora_flag == 0;

    readTimeStamp();
    strncat(dataToSend, "*", 2);
    strncat(dataToSend, Ctimestamp, 13);
    Watchdog.reset();

    if ((get_logger_mode() == 7) || (get_logger_mode() == 9)) {
      //Remove 1st and 2nd character data in string. Not needed in GSM mode
      for (byte i = 0; i < strlen(dataToSend); i++) {
        dataToSend[i] = dataToSend[i + 2];
      }
    }
    Watchdog.reset();
  }
}

void readUbloxData() {
  Watchdog.reset();
  int j = 0;
  for (j = 0; j < 200; j++) {
    dataToSend[j] = (uint8_t)'0';
  }
  dataToSend[j] = (uint8_t)'\0';

  // memset(dataToSend, '\0', sizeof(dataToSend));
  // memset(voltMessage, '\0', sizeof(voltMessage));
  Watchdog.reset();

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

  char tempstr[100];
  char volt[10];
  char temp[10];

  snprintf(volt, sizeof volt, "%.2f", readBatteryVoltage(10));
  snprintf(temp, sizeof temp, "%.2f", readTemp());
  Watchdog.reset();

  for (int i = 1; i <= ave_count; i++) {
    if ((millis() - start) < rtcm_timeout) {
      Watchdog.reset();
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
      Watchdog.reset();

      if ((checkHorizontalAccuracy() == 141 && checkVerticalAccuracy() <= 141)) {
        // Accumulation
        Watchdog.reset();
        accu_lat += d_lat;
        accu_lon += d_lon;
        accu_msl += f_msl;
        accu_accuracy_hor += f_accuracy_hor;
        accu_accuracy_ver += f_accuracy_ver;
        accu_count++;
        Serial.print("accu_count: ");
        Serial.println(accu_count);
        Watchdog.reset();
      } else {
        Watchdog.reset();
        i--; //loop until hacc&vacc conditions are satisfied or until timeout reached
        Serial.print("iter_count: "); 
        Serial.println(i);
        Watchdog.reset();
        getRTCM();
      }
    } else if ((millis() - start) >= rtcm_timeout) {
      Watchdog.reset();
      Serial.println("Timeout reached!");
      break;
    }
  }

  // Averaging
  Watchdog.reset();
  d_lat = accu_lat / accu_count; 
  d_lon = accu_lon / accu_count;
  f_msl = accu_msl / accu_count; 
  f_accuracy_hor = accu_accuracy_hor / accu_count;
  f_accuracy_ver = accu_accuracy_ver / accu_count;
  Watchdog.reset();

  if ((d_lat > 0) || (d_lon > 0)) {
    Watchdog.reset();
    sprintf(tempstr, ">>%s:%d,%.9f,%.9f,%.4f,%.4f,%.4f,%d", sitecode, rtk_fixtype, d_lat, d_lon, f_accuracy_hor, f_accuracy_ver, f_msl, sat_num);
    strncpy(dataToSend, tempstr, strlen(tempstr) + 1);
    strncat(dataToSend, ",", 2);
    strncat(dataToSend, temp, sizeof(temp));
    strncat(dataToSend, ",", 2);
    strncat(dataToSend, volt, sizeof(volt)); 
    Watchdog.reset();
    Serial.print("data to send: "); 
    Serial.println(dataToSend);
  } else {
    Watchdog.reset();
    noGNSSDataAcquired();
  }
  
  Watchdog.reset();
  d_lat, d_lon, f_msl, f_accuracy_hor, f_accuracy_ver = 0.0;
  accu_lat, accu_lon, accu_msl, accu_accuracy_hor, accu_accuracy_ver = 0.0;   //reset accumulators to zero
  accu_count = 0;
  Watchdog.reset();
}

void noGNSSDataAcquired() {
  memset(dataToSend, '\0', sizeof(dataToSend));

  char ndstr[50]; 
  snprintf(ndstr, sizeof(ndstr), ">>%s:No Ublox data", sitecode);
  strncat(dataToSend, ndstr, sizeof(ndstr));

  Serial.print("data to send: "); 
  Serial.println(dataToSend);
}

void initialize_sitecode() {
  if (get_logger_mode() == 9) {
    char *logger_B_data = get_logger_B_from_flashMem();
    strncpy(sitecode, logger_B_data, 5); // Copy up to 5 characters to avoid buffer overflow
    sitecode[5] = '\0'; // Null-terminate the string
  } else {
    char *logger_A_data = get_logger_A_from_flashMem();
    strncpy(sitecode, logger_A_data, 5); // Copy up to 5 characters to avoid buffer overflow
    sitecode[5] = '\0'; // Null-terminate the string
  } 
}
