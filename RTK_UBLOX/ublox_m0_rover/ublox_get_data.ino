void init_ublox(){
  Wire.begin();

  //myGNSS.enableDebugging(Serial);

  if (myGNSS.begin(Wire) == false) //Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    while (1);
  }

  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.setNavigationFrequency(5); //Set output to 20 times a second

  byte rate = myGNSS.getNavigationFrequency(); //Get the update rate of this module
}

void read_ublox_data(){
    for (int i = 0; i < 200; i++)
    dataToSend[i] = 0x00;

    float lat = 0.0, lon = 0.0;
    // Now define float storage for the heights and accuracy
    float f_ellipsoid;
    float f_msl;
    float f_accuracy =0.0;
    
    char tempstr[100];

    for (int i= 0; i < DATA_TO_AVERAGE; i++){
      // getHighResLatitude: returns the latitude from HPPOSLLH as an int32_t in degrees * 10^-7
      // getHighResLatitudeHp: returns the high resolution component of latitude from HPPOSLLH as an int8_t in degrees * 10^-9
      // getHighResLongitude: returns the longitude from HPPOSLLH as an int32_t in degrees * 10^-7
      // getHighResLongitudeHp: returns the high resolution component of longitude from HPPOSLLH as an int8_t in degrees * 10^-9
      // getElipsoid: returns the height above ellipsoid as an int32_t in mm
      // getElipsoidHp: returns the high resolution component of the height above ellipsoid as an int8_t in mm * 10^-1
      // getMeanSeaLevel: returns the height above mean sea level as an int32_t in mm
      // getMeanSeaLevelHp: returns the high resolution component of the height above mean sea level as an int8_t in mm * 10^-1
      // getHorizontalAccuracy: returns the horizontal accuracy estimate from HPPOSLLH as an uint32_t in mm * 10^-1
  
      // If you want to use the high precision latitude and longitude with the full 9 decimal places
      // you will need to use a 64-bit double - which is not supported on all platforms
  
      // To allow this example to run on standard platforms, we cheat by converting lat and lon to integer and fractional degrees
  
      // The high resolution altitudes can be converted into standard 32-bit float
  
      // First, let's collect the position data
      int32_t latitude = myGNSS.getHighResLatitude();
      int8_t latitudeHp = myGNSS.getHighResLatitudeHp();
      int32_t longitude = myGNSS.getHighResLongitude();
      int8_t longitudeHp = myGNSS.getHighResLongitudeHp();
      int32_t ellipsoid = myGNSS.getElipsoid();
      int8_t ellipsoidHp = myGNSS.getElipsoidHp();
      int32_t msl = myGNSS.getMeanSeaLevel();
      int8_t mslHp = myGNSS.getMeanSeaLevelHp();
      uint32_t accuracy = myGNSS.getHorizontalAccuracy();
  
      // Defines storage for the lat and lon units integer and fractional parts
      int32_t lat_int; // Integer part of the latitude in degrees
      int32_t lat_frac; // Fractional part of the latitude
      int32_t lon_int; // Integer part of the longitude in degrees
      int32_t lon_frac; // Fractional part of the longitude


      // Calculate the latitude and longitude integer and fractional parts
      lat_int = latitude / 10000000; // Convert latitude from degrees * 10^-7 to Degrees
      lat_frac = latitude - (lat_int * 10000000); // Calculate the fractional part of the latitude
      lat_frac = (lat_frac * 100) + latitudeHp; // Now add the high resolution component
      if (lat_frac < 0) // If the fractional part is negative, remove the minus sign
      {
        lat_frac = 0 - lat_frac;
      }


      lon_int = longitude / 10000000; // Convert latitude from degrees * 10^-7 to Degrees
      lon_frac = longitude - (lon_int * 10000000); // Calculate the fractional part of the longitude
      lon_frac = (lon_frac * 100) + longitudeHp; // Now add the high resolution component
      if (lon_frac < 0) // If the fractional part is negative, remove the minus sign
      {
        lon_frac = 0 - lon_frac;
      }

      // Calculate lat-long in float
      lat = lat + (float)lat_int + (float)lat_frac/pow(10, 9);
      lon = lon +(float)lon_int + (float)lon_frac/pow(10, 9);
  
      // Calculate the height above ellipsoid in mm * 10^-1
      f_ellipsoid = (ellipsoid * 10) + ellipsoidHp;
      // Now convert to m
      f_ellipsoid = f_ellipsoid / 10000.0; // Convert from mm * 10^-1 to m
  
      // Calculate the height above mean sea level in mm * 10^-1
      f_msl = (msl * 10) + mslHp;
      // Now convert to m
      f_msl = f_msl / 10000.0; // Convert from mm * 10^-1 to m
  
      // Now convert to m
      f_accuracy = f_accuracy + ((float)accuracy/ 10000.0); // Convert from mm * 10^-1 to m
      //delay(50);
    }
    
    lat = lat / DATA_TO_AVERAGE;
    lon = lon / DATA_TO_AVERAGE;
    f_accuracy = f_accuracy / DATA_TO_AVERAGE;
    
    sprintf(tempstr, "%.9f,%.9f,%.4f", lat,lon,f_accuracy);
    strncpy(dataToSend, tempstr, String(tempstr).length() + 1);
    
      
    Serial.print("data to send: ");
    Serial.println(dataToSend);
}

// Pretty-print the fractional part with leading zeros - without using printf
// (Only works with positive numbers)
void printFractional(int32_t fractional, uint8_t places)
{

  char tempstr[64];
  if (places > 1)
  {
    for (uint8_t place = places - 1; place > 0; place--)
    {
      if (fractional < pow(10, place))
      {
        //Serial.print("0");
        strncat(dataToSend, "0", 1);

      }
    }
  }
  //Serial.print(fractional);
  sprintf(tempstr, "%d", fractional);
  strncat(dataToSend, tempstr, String(tempstr).length() + 1);
}
