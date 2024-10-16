
bool read_override() {
  byte* b2 = dueFlashStorage.readAddress(4);
  int override_value;
  bool override_flag;
  f_config read_flash;
  memcpy(&read_flash, b2, sizeof(f_config));
  override_value = read_flash.override_lib_config;
  if (override_value == 99) {
    override_flag = true;
  } else {
    override_flag = false;
  }
  return override_flag;
}

void flash_fetch() {
  bool valid_flag = false;
  byte* b2 = dueFlashStorage.readAddress(4);
  char* flash_mastername;
  int flash_check, override_check;
  f_config fetch_config;
  memcpy(&fetch_config, b2, sizeof(f_config));
  flash_mastername = fetch_config.f_mastername;
  flash_check = fetch_config.check;
  override_check = fetch_config.override_lib_config;
  if (override_check == 99) {  //check whether a override value has been set
    //insert SD override function here
    Serial.println("SD CARD CONFIG IN USE");
    if (Serial2) Serial2.println("SD CARD CONFIG IN USE");
    open_config();
  } else {
    if (flash_check != 99) {
      Serial.println("NO SENSOR NAME SET FOR CONFIG!");
      if (Serial2) Serial2.println("NO SENSOR NAME SET FOR CONFIG!");
    } else {
      // Serial.print("Stored datalogger name: ");
      strcpy(g_mastername, flash_mastername);
      g_mastername[5] = '\0';
      char* name_buffer;
      for (int i = 0; i <= (LOGGER_COUNT - 1); i++) {
        if (strcmp(config_container[i].lib_mastername, g_mastername) == 0) {
          strcpy(g_mastername, config_container[i].lib_mastername);
          g_mastername[5] = '\0';
          strcpy(column_id_holder, config_container[i].lib_column_ids);
          column_id_holder[strlen(column_id_holder) + 1] = '\0';
          parse_column_ids_from_library();
          g_num_of_nodes = config_container[i].lib_num_of_nodes;
          g_datalogger_version = config_container[i].lib_datalogger_version;
          g_sensor_version = config_container[i].lib_sensor_version;
          has_piezo = config_container[i].lib_has_piezo;
          g_turn_on_delay = config_container[i].lib_turn_on_delay;
          broad_timeout = config_container[i].lib_broad_timeout;
          g_sampling_max_retry = config_container[i].lib_sampling_max_retry;
          b64 = config_container[i].lib_b64;
          valid_flag = true;
        } else {
        }
      }
      if (valid_flag) {
        print_stored_config();
        if (Serial2) print_stored_config2();
      } else {
        Serial.println("No matching datalogger configuration found in library.");
        if (Serial2) Serial2.println("No matching datalogger configuration found in library.");
      }
    }
  }
}

/*void flash_fetch() {
  bool valid_flag = false;
  byte* b2 = dueFlashStorage.readAddress(4);
  char* flash_mastername;
  int flash_check, override_check;
  f_config fetch_config;
  memcpy(&fetch_config, b2, sizeof(f_config));
  flash_mastername = fetch_config.f_mastername;
  flash_check = fetch_config.check;
  override_check = fetch_config.override_lib_config;
  
  if (override_check == 99) {  // check whether an override value has been set
    // insert SD override function here
    Serial.println("SD CARD CONFIG IN USE");
    open_config();
  } else {
    if (flash_check != 99) {
      Serial.println("NO SENSOR NAME SET FOR CONFIG!");
    } else {
      strcpy(g_mastername, flash_mastername);
      g_mastername[5] = '\0';  // Ensuring g_mastername is limited to 5 characters

      for (int i = 0; i <= (LOGGER_COUNT - 1); i++) {
        if (strncmp(config_container[i].lib_mastername, g_mastername, 4) == 0 || strncmp(config_container[i].lib_mastername, g_mastername, 5) == 0) {
          strcpy(g_mastername, config_container[i].lib_mastername);
          g_mastername[5] = '\0';  // Ensuring g_mastername is limited to 5 characters

          strcpy(column_id_holder, config_container[i].lib_column_ids);
          column_id_holder[strlen(column_id_holder) + 1] = '\0';

          parse_column_ids_from_library();
          g_num_of_nodes = config_container[i].lib_num_of_nodes;
          g_datalogger_version = config_container[i].lib_datalogger_version;
          g_sensor_version = config_container[i].lib_sensor_version;
          has_piezo = config_container[i].lib_has_piezo;
          g_turn_on_delay = config_container[i].lib_turn_on_delay;
          broad_timeout = config_container[i].lib_broad_timeout;
          g_sampling_max_retry = config_container[i].lib_sampling_max_retry;
          b64 = config_container[i].lib_b64;
          valid_flag = true;
        }
      }

      if (valid_flag) {
        print_stored_config();
      } else {
        Serial.println("No matching datalogger configuration found in library.");
      }
    }
  }
}*/


// Checks if something is stored in flash memory
int flash_LED(){
  byte* b2 = dueFlashStorage.readAddress(4);
  int flash_check, override_check;
  f_config fetch_config;
  memcpy(&fetch_config, b2, sizeof(f_config));
  flash_check = fetch_config.check;
  return flash_check;
}



char* trim(char* str) {
    // Trim leading whitespace
    while (isspace(*str)) {
        str++;
    }

    // Trim trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        end--;
    }
    *(end + 1) = '\0'; // Null-terminate the trimmed string

    return str;
}

void name_entry() {
  byte f[sizeof(f_config)];
  char name_holder[7]; // Increased by 1 for null terminator
  char serial_input[20]; // Adjusted size based on requirements
  f_config flash_config;
  Serial.print("Input datalogger name: ");
  if (Serial2) Serial2.print("Input datalogger name: ");
  delay(1000);
  do {
    while (!Serial.available()) {
      // Wait for serial input
    }
    Serial.readBytesUntil('\n', serial_input, sizeof(serial_input)); // Read serial input
  } while (strlen(serial_input) == 0); // Check if input is empty
  serial_input[strlen(serial_input)] = '\0'; // Ensure null termination
  strupr(serial_input); // Convert input to uppercase
  strncpy(flash_config.f_mastername, serial_input, sizeof(flash_config.f_mastername) - 1); // Copy input to flash_config
  flash_config.f_mastername[sizeof(flash_config.f_mastername) - 1] = '\0'; // Ensure null termination
  strncpy(v1_check, flash_config.f_mastername, sizeof(v1_check) - 1);
  v1_check[4] = '\0';
  if(strcmp("HUMB",v1_check)==0||strcmp("LABT",v1_check)==0||strcmp("LABB",v1_check)==0){
    flash_config.f_mastername[sizeof(flash_config.f_mastername) - 2] = '\0'; // Ensure null termination
  }  
  Serial.println();
  if (Serial2) Serial2.println();
  Serial.print(flash_config.f_mastername);
  if (Serial2) Serial2.print(flash_config.f_mastername);

  flash_config.check = 99;
  memcpy(f, &flash_config, sizeof(f_config));
  dueFlashStorage.write(4, f, sizeof(f_config));
  Serial.println(F(" saved to flash"));
  if (Serial2) Serial2.println(F(" saved to flash"));
  delay(1000);
} 
/*
Function: Parse column IDs stored in the library configurations in to the column ID holder array g_gids[i][i]   
*/
void parse_column_ids_from_library() {
  // g_gids[i][0]
  int id_counter = 0;
  int n;
  char* id_token = strtok(column_id_holder, ",");
  while (id_token != NULL) {
    g_gids[id_counter][0] = atoi(id_token);
    id_counter++;
    id_token = strtok(NULL, ",");
  }

}
