


// const char* defaultServerNumber = "09175972526";   //  GLOBE1

// char routerList[MAX_DATALOGGER_NAME_LENGTH][MAX_ROUTER_COUNT];      // currently limited to MAX_DATALOGGER_NAME_LENGTH 
char routerDefaultList[MAX_DATALOGGER_NAME_LENGTH][MAX_ROUTER_COUNT] = {"sARQA", "sARQB", "sARQC", "sARQD", "sARQE"};

//  Loads default values to NVS for first time use or when NVS is erased
void loadDefaultParams(const char * namespaceName) {
  
  
  if (storageSpace.begin(namespaceName, false)) {             // initialize storage space; create one if it doesn't exist
    debugPrint("Initialized namespace ");
    debugPrintln(namespaceName);
    storageSpace.end();
  }
  uint8_t nvsUseFlag = fetchParam(namespaceName,NVS_USE_FLAG, (uint8_t)0); 
  if (nvsUseFlag == 29 ) {  // if gate to prevent mulitple overwrites
    debugPrintln("NVS flag found");
    return;
  } else {
    char nvsDefaultServer[15];
    char nvsDefaultCmd[15];
    sprintf(nvsDefaultServer, defaultServerNumber);
    sprintf(nvsDefaultCmd, defaultSensorCommand);
    debugPrintln("NVS flag not found");
    debugPrintln("Loading default parameters to NVS...");
    //  All of these can also ve stored in one (global) struct and saved all at once
    //  Fetch can be done by one function to update the struct and values will be read from the struct 
    //  However, when updating a single variable, the struct gets re-writtten to the nvs as a whole, even the values that are not updated  
    //  Alhough, I'm not entirely sure if it will contribute as added writes cycles to the unmodified variables.  
    storageSpace.begin(namespaceName, false);
    storageSpace.putInt(NVS_USE_FLAG,29);             //  number is arbitrary
    storageSpace.putInt(ALARM_INTERVAL,0);
    storageSpace.putInt(DATALOGGER_MODE,0);
    storageSpace.putBool(SUBSURFACE_SENSOR_FLAG,   false);
    storageSpace.putBool(UBLOX_FLAG,false);
    storageSpace.putInt(ROUTER_COUNT,0);
    storageSpace.putInt(POWER_SAVING_MODE,0);
    storageSpace.putInt(RAIN_COLLECTOR_TYPE,0);
    storageSpace.putInt(RAIN_DATA_TYPE,0);
    storageSpace.putInt(BATTERY_TYPE,0);
    storageSpace.putInt(LORA_RECEIVE_MODE,0);
    storageSpace.putInt(SELF_RESET_HOUR,0);
    storageSpace.putInt(SELF_RESET_MINUTE,0);
    storageSpace.putBool(LISTEN_MODE,false);
    // storageSpace.putFloat(OP_AMP_SLOPE,opAmpSlopeDefault);
    // storageSpace.putBool(OP_AMP_OFFSET,opAmpOffsetDefault);
    storageSpace.putBytes(SERVER_NUMBER, nvsDefaultServer, sizeof(nvsDefaultServer));
    storageSpace.putBytes(SENSOR_COMMAND, nvsDefaultCmd, sizeof(nvsDefaultCmd));
    storageSpace.end();
    // storeParam(namespaceName, SERVER_NUMBER, defaultServerNumber);
    // storeParam(namespaceName, SENSOR_COMMAND, defaultSensorCommand);
    sprintf(dataloggerNameList[0], defaultDataloggerName);
    saveNameArrayToStorage();

  }
}

//  overload functions for multiple variable types so we only call one function

//  Value to be stores must be cast as int or uint8_t to prevent ambiguitity with other overload functions
//  ...let's deal with ambiguity later
void storeParam(const char * nameSpaceID, const char* keyID, uint8_t intValue) {
  storageSpace.begin(nameSpaceID, false);
  storageSpace.putInt(keyID, intValue);
  storageSpace.end();
}

// Avoid using integer values to represent bool to prevent ambiguitiy with other overload functions
void storeParam(const char * nameSpaceID, const char* keyID, bool boolValue) {
  storageSpace.begin(nameSpaceID, false);
  storageSpace.putBool(keyID, boolValue);
  storageSpace.end();
}

void storeParam(const char * nameSpaceID, const char* keyID, const char* paramValue) {
  char paramBuffer[15];
  sprintf(paramBuffer, paramValue);
  storageSpace.begin(nameSpaceID, false);
  storageSpace.putBytes(keyID, paramBuffer, sizeof(paramBuffer));
  storageSpace.end();
}


//  Stores the global variables of corresponding key to nvs
//  Info:
//  Strangely, global variables behave properly for storing bytes.
//  However, passed char array variables differently..
//  This function is not yet optimized; you might lose track of the global variables being used because it is not delared within the function
//  We can try other approach some other time, for now this works...
//  
// void storeParam(const char * nameSpaceID, const char* keyID, const char *paramValue) {
//   storageSpace.begin(nameSpaceID, false);
//   if (strcmp(keyID,SERVER_NUMBER)==0) {
//     strcpy(serverNumber, paramValue);
//     storageSpace.putBytes(keyID, serverNumber, sizeof(serverNumber));
//     debugSys("Input to 'storeParam'(char): ");
//     debugSysln(serverNumber);
//     }
//   if (strcmp(keyID,SENSOR_COMMAND)==0) {
//     strcpy(sensorCommand, paramValue);
//     storageSpace.putBytes(keyID, sensorCommand, sizeof(sensorCommand));
//     debugSys("Input to 'storeParam'(char): ");
//     debugSysln(sensorCommand);
//     }
//   storageSpace.end();
// }


//  Value to be stores must be type cast or explicit as int or uint8_t to prevent ambiguitity with other overload functions
int fetchParam(const char* nameSpaceID, const char* keyID, uint8_t defaultIntValue) {
  int intBuffer = 0;
  storageSpace.begin(nameSpaceID, true);
  intBuffer = storageSpace.getInt(keyID, defaultIntValue);
  storageSpace.end();
  return intBuffer;
}
// Avoid using integer values to represent bool to prevent ambiguitiy with other overload functions
bool fetchParam(const char* nameSpaceID, const char* keyID, bool defaultBoolValue) {
  bool boolBuffer = false;
  storageSpace.begin(nameSpaceID, true);
  boolBuffer = storageSpace.getBool(keyID, defaultBoolValue);
  storageSpace.end();
  return boolBuffer;
}
float fetchParam(const char* nameSpaceID, const char* keyID, float defaultFloatValue) {
  float floatBuffer = 0.00;
  storageSpace.begin(nameSpaceID, true);
  floatBuffer = storageSpace.getFloat(keyID, defaultFloatValue);
  storageSpace.end();
  return floatBuffer;
}

void fetchParam(const char* nameSpaceID, const char* keyID, char arrContainer[], size_t arrSize) {     
  storageSpace.begin(nameSpaceID, true);
  storageSpace.getBytes(keyID, arrContainer, arrSize);
  storageSpace.end();
  debugSys("Output of 'fetchParam'(char): ");
  debugSysln(arrContainer);
}

// //  Using saved parameters in NVS this updates the global variables of the corresponding key
// //  This is not ideal...
// void fetchParam(const char * nameSpaceID, const char* keyID) {
//   storageSpace.begin(nameSpaceID, true);
//   if (strcmp(keyID,SERVER_NUMBER)==0) storageSpace.getBytes(keyID, serverNumber, sizeof(serverNumber));
//   if (strcmp(keyID,SENSOR_COMMAND)==0) storageSpace.getBytes(keyID, sensorCommand, sizeof(sensorCommand));
//   storageSpace.end();
// }

// Erase the entire NVS partition
void clearNVS() {
  debugPrint("This clears EVERYTHING in the non-volatile storage.");
  if (changeParameter()) {
    esp_err_t err = nvs_flash_erase();                // check if something goes wrong
    if (err == ESP_OK) {
      debugPrintln("NVS flash erased successfully."); //  if all goes well
    } else {
      debugPrint("Failed to erase NVS flash: ");      //  not good
      debugPrintln(esp_err_to_name(err));             // Convert error code to corresponding name 
    }

    // Reinitialize NVS after erasing
    err = nvs_flash_init();
    if (err == ESP_OK) {
      debugPrintln("NVS reinitialized.");
    } else {
      debugPrint("Failed to initialize NVS: ");
      debugPrintln(esp_err_to_name(err));             // Convert error code to corresponding name
    }
  }
}

//  Preferences library does not explicitly support struct type variable or two-dimensional arrays
//  A workaround can be to serialize the struct and store it as a byte array 
//  Function to save the array dataloggerNameList
void saveNameArrayToStorage() {
    storageSpace.begin(paramStorage, false); // namespace "storage", RW mode
    storageSpace.putBytes("NAME_LIST", dataloggerNameList, sizeof(dataloggerNameList));
    // if (showResult); Serial.printf("Saved %u bytes to NVS\n", bytesWritten);
    storageSpace.end();
}

// Function to load the saved array to dataloggerNameList
void loadNameArrayFromStorage(bool showList) {
    storageSpace.begin(paramStorage, true); // read-only mode
    // char loadedArray[10][MAX_DATALOGGER_NAME_LENGTH] = {0}; // buffer to hold retrieved data
    storageSpace.getBytes("NAME_LIST", dataloggerNameList, sizeof(dataloggerNameList));
    storageSpace.end();

    if (showList) {
      for (int i = 0; i < MAX_DATALOGGER_NAME_LENGTH; i++) {
        Serial.printf("Row %d: %s\n", i, dataloggerNameList[i]);
      }
    }

}

//  This might look short but this scenario happens a lot, so quite a lot of lines can be saved
void getNameFromList(uint8_t nameIndex, char * nameContainer) {
  debugSysln("#   start getNameFromList");
    loadNameArrayFromStorage(false);                               //  reload global name list variable from nvs
    if (strcpy(nameContainer, dataloggerNameList[nameIndex])) {  //  copy target paramter from global list to container
      debugSysln("#   if condition");
      debugSysln(dataloggerNameList[nameIndex]);
    }     
    else {                       //  use something else if it doesn't work
      debugSysln("#   else condition");
    }
  debugSysln("#   end getNameFromList");
}

//  This function only updates the global variable
//  It doesn't save the parameters on the nvs
void putNameOnGlobalList(uint8_t nameIndex, const char* nameValue) {
      if (strcpy(dataloggerNameList[nameIndex], nameValue));
      else (strcpy(dataloggerNameList[nameIndex], "DFLTX"));
}