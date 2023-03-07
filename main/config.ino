
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
  if (override_check == 99) {         //check whether a override value has been set
    //insert SD override function here
    Serial.println("SD CARD CONFIG IN USE");
    open_config();
  } else {
    if (flash_check != 99) {
      Serial.println("NO SENSOR NAME SET FOR CONFIG!");
    } else {
      // Serial.print("Stored datalogger name: ");
      strcpy(g_mastername, flash_mastername);
      g_mastername[5] = '\0';
      // Serial.println(g_mastername);
      // Serial.println(sizeof(g_mastername));
      char* name_buffer;
      for (int i = 0; i<=(LOGGER_COUNT-1); i++) {
        if (strcmp(config_container[i].lib_mastername,g_mastername) == 0) {
          strcpy(g_mastername, config_container[i].lib_mastername);
          g_mastername[5] = '\0';
          // Serial.print("Saved configuration for sensor ");
          // Serial.println(g_mastername);
          // Serial.print("Column IDs: ");
          strcpy(column_id_holder, config_container[i].lib_column_ids);
          column_id_holder[strlen(column_id_holder)+1] = '\0';
          // Serial.println(column_id_holder);
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
      } else {
        Serial.println("No matching datalogger configuration found in library.");
      }
    }

  }

}
/*
Function: Accepts input from serial as SENSOR NAME to store in flash.
*/
void name_entry() {
  byte f[sizeof(f_config)];
  char name_holder[6];
  String serial_input;
  f_config flash_config;
  Serial.print("Input datalogger name: ");
  delay(1000);
  do {
    serial_input = Serial.readStringUntil('\n');
  } while(serial_input == "");
  serial_input.trim();
  serial_input.toUpperCase();
  serial_input.toCharArray(flash_config.f_mastername, 6);
  flash_config.f_mastername[6] = '\0';
  Serial.println();
  Serial.print(flash_config.f_mastername);
  
  flash_config.check = 99;
  memcpy (f, &flash_config, sizeof(f_config));
  dueFlashStorage.write(4, f, sizeof(f_config));
  Serial.println(F(" saved to flash"));
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

  // Serial.print("Stored node count: ");
  // Serial.println(id_counter);  
  // Serial.println("Saved IDs");
  // for (n=0;n<40;n++){
  //   if (g_gids[n][0] != 0) {
  //     Serial.print("Node ");
  //     Serial.print(n);
  //     Serial.print(" ");
  //     Serial.println(g_gids[n][0]);
  //   }
  // }
}
