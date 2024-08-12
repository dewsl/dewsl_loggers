/* 
	Function: sd_init

		Assert the chip_select pin for SD use.

	Parameters:
   n/

  Returns:


		-1 fail, 0 success.

	See Also:

		<setup>
*/
int init_sd() {
  if (!SD.begin(g_chip_select)) {
    Serial.println("###  SD card initialization failed!  ###");
    return -1;
  }
  return 0;
}

/* 
	Function: init_gids
		
		*g_gids[i][0]* - *uids*
		*g_gids[i][1]* - *gids*

		- Set the uids to 0s.
		- Set the gids to the numbers: 1 - 40.

	Parameters:

    n/a

  Returns:

    n/a

	See Also:
	
		<setup>
*/
void init_gids() {
  if (VERBOSE == 1) { Serial.println(F("init_uids()")); }
  for (int i = 0; i < g_num_of_nodes; i++) {
    g_gids[i][0] = 0;      // unique id
    g_gids[i][1] = i + 1;  // geographic id ( 1- 40)
  }
  return;
}

int unsent_row() {
  File unsent_log;
  int totalrow = 0;
  unsent_log = SD.open("unsent.txt");
  while (unsent_log.available()) {
    if (totalrow <= 50) {
      unsent_log.readStringUntil('\n');
      totalrow = totalrow + 1;
      Serial.println(totalrow);
    } else {
      break;
    }
  }
  unsent_log.close();
  return (totalrow);
}

void open_sdata() {
  File unsent_log;
  File temp_log;
  int row = 0;
  char values[390];
  int state;
  int i;

  int totalrow = unsent_row();


  unsent_log = SD.open("unsent.txt");
  while (unsent_log.available()) {
    if (totalrow <= 50) {
      unsent_log.readStringUntil('\n');
      totalrow = totalrow + 1;
      Serial.println(totalrow);
    } else {
      break;
    }
  }
  unsent_log.close();

  //if (DATALOGGER.available()){
  int dummytotalrow = totalrow + 1;
  unsent_log = SD.open("unsent.txt");
  // read from the file until there's nothing else in it:
  while (unsent_log.available() > 0) {
    //Serial.write(sd_files.read());
    i = unsent_log.readBytesUntil('\n', values, sizeof(values) - 1);
    row = row + 1;
    values[i] = '\0';
    char *token1 = values;
    if (row <= totalrow) {
      Serial.println(token1);
      delay(200);
      char token2[142];  // "040>>1/1#cartc*asda<<";//Bilangin yung rows para malagay dito. Tapos tska isend
      sprintf(token2, "%s%d%s%d%s%s%s\n", "040>>", row, "/", dummytotalrow, "#", token1, "<<");
      //strcat(token2, token1);
      Serial.println(token2);
      send_data(false, token2);
    } else {
      // close the file:
      unsent_log.close();
    }
  }
  unsent_log.close();
  //   }



  unsent_log = SD.open("unsent.txt");
  temp_log = SD.open("temp.txt", FILE_WRITE);
  int rowN = 0;

  while (unsent_log.available() > 0) {
    i = unsent_log.readBytesUntil('\n', values, sizeof(values) - 1);
    rowN = rowN + 1;
    values[i] = '\n';
    values[i + 1] = '\0';
    char *token1 = values;
    if (rowN > totalrow) {
      Serial.print(token1);
      temp_log.print(token1);
    } else if (rowN == totalrow && rowN < 50) {
      temp_log.print("");
    }
  }
  temp_log.close();
  unsent_log.close();
  rename_sd();
}

void rename_sd() {
  int n;
  File temp_log;
  File unsent_log;

  SD.remove("unsent.txt");
  temp_log = SD.open("temp.txt");
  unsent_log = SD.open("unsent.txt", FILE_WRITE);

  while ((n = temp_log.read()) >= 0) {
    unsent_log.write(n);
  }
  temp_log.close();
  unsent_log.close();
  SD.remove("temp.txt");
}

/* 
	Function: print_stored_config

		Print in the gids and uids from the global g_gids and,
		other variables read from CONFIG.txt

	Parameters:

		n/a

	Returns:

		n/a

	See Also:

		<init_gids>
*/
void print_stored_config() {
  char gid[2], uid[4], desc[25];
  if (g_datalogger_version == 2) {
    strncpy(comm_mode, "ARQ", 3);
  }
  if (g_datalogger_version == 4) {
    strncpy(comm_mode, "LORA", 4);
  } else if (g_datalogger_version == 1 || g_datalogger_version == 3 || g_datalogger_version == 5) {
    Serial.print("g_datalogger_version == ");
    Serial.print(g_datalogger_version);
    Serial.println(" (default to LORA)");
    strncpy(comm_mode, "LORA", 4);
  }
  Serial.print("Comms: ");
  Serial.println(comm_mode);
  Serial.println(F("======================================"));
  sprintf(desc, "%-24s", "Sensor Name:");
  Serial.print(desc);
  Serial.println(g_mastername);
  sprintf(desc, "%-24s", "Datalogger Version: ");
  Serial.print(desc);
  Serial.println(g_datalogger_version);
  sprintf(desc, "%-24s", "Sensor Version: ");
  Serial.print(desc);
  Serial.println(g_sensor_version);
  sprintf(desc, "%-24s", "Broadcast Timeout: ");
  Serial.print(desc);
  Serial.println(broad_timeout);
  sprintf(desc, "%-24s", "Sampling Max Retry: ");
  Serial.print(desc);
  Serial.println(g_sampling_max_retry);
  sprintf(desc, "%-24s", "Turn On Delay: ");
  Serial.print(desc);
  Serial.println(g_turn_on_delay);
  sprintf(desc, "%-24s", "Number of Nodes");
  Serial.print(desc);
  Serial.println(g_num_of_nodes);
  Serial.println(F("======================================"));
  Serial.println(F("Geographic ID\t\tUnique ID"));
  Serial.println(F("======================================"));
  for (int i = 0; i < g_num_of_nodes; i++) {
    sprintf(gid, "%2d", g_gids[i][1]);
    sprintf(uid, "%4d", g_gids[i][0]);
    Serial.print("\t");
    Serial.print(gid);
    Serial.print("\t\t");
    Serial.println(uid);
  }
  Serial.println(F("======================================"));
}

/* 
	Function: print_stored_config2

		Print in the gids and uids from the global g_gids and,
		other variables read from CONFIG.txt. Cloned of print_stored_config()      

	Parameters:

		n/a

	Returns:

		n/a

	See Also:

		<init_gids>
*/
void print_stored_config2() {
  char gid[2], uid[4], desc[25];
  Serial2.println(F("======================================"));
  sprintf(desc, "%-24s", "Sensor Name:");
  Serial2.print(desc);
  Serial2.println(g_mastername);
  sprintf(desc, "%-24s", "Datalogger Version: ");
  Serial2.print(desc);
  Serial2.println(g_datalogger_version);
  sprintf(desc, "%-24s", "Sensor Version: ");
  Serial2.print(desc);
  Serial2.println(g_sensor_version);
  sprintf(desc, "%-24s", "Broadcast Timeout: ");
  Serial2.print(desc);
  Serial2.println(broad_timeout);
  sprintf(desc, "%-24s", "Sampling Max Retry: ");
  Serial2.print(desc);
  Serial2.println(g_sampling_max_retry);
  sprintf(desc, "%-24s", "Turn On Delay: ");
  Serial2.print(desc);
  Serial2.println(g_turn_on_delay);
  sprintf(desc, "%-24s", "Number of Nodes");
  Serial2.print(desc);
  Serial2.println(g_num_of_nodes);
  Serial2.println(F("======================================"));
  Serial2.println(F("Geographic ID\t\tUnique ID"));
  Serial2.println(F("======================================"));
  for (int i = 0; i < g_num_of_nodes; i++) {
    sprintf(gid, "%2d", g_gids[i][1]);
    sprintf(uid, "%4d", g_gids[i][0]);
    Serial2.print("\t");
    Serial2.print(gid);
    Serial2.print("\t\t");
    Serial2.println(uid);
  }
  Serial2.println(F("======================================"));
}

/* 
	Function: get_value_from_line

		Extract the value from current line of the config.

	Parameters:

		line - String of one line read from config 

	Returns:

		String after "=" stripped of spaces

	See Also:

		<process_config_line>
*/
/*String get_value_from_line(String line) {
  String delim = "=";
  String other_chars = " ";
  int index, i;
  while ((i = line.indexOf(other_chars)) != -1) {
    line.remove(i, 1);
  }
  index = line.lastIndexOf(delim);
  return line.substring(index + 1);
}*/

char* get_value_from_line(char* line) {
  char delim = '=';
  char other_chars[] = " ";
  int index = 0;
  int i = 0;
  int line_length = strlen(line);

  // Remove other characters
  for (i = 0; i < line_length; i++) {
    if (line[i] == *other_chars) {
      memmove(&line[i], &line[i + 1], line_length - i);
      line_length--; // Update line length after removal
    }
  }

  // Find the index of the last occurrence of the delimiter
  char* ptr = strrchr(line, delim);
  if (ptr != NULL) {
    index = ptr - line;
    // Allocate memory for the substring
    char* result = (char*)malloc((line_length - index) * sizeof(char));
    if (result != NULL) {
      // Copy substring after the delimiter to the result
      strcpy(result, &line[index + 1]);
      return result;
    }
  }
  // Return NULL if the delimiter is not found or memory allocation fails
  return NULL;
}

int8_t writeData(const char* fname, const char* data) {
  File sdFile;
  char filename[100] = {};
  char logger_file_name[7] = {};

  // Initialize SD card
  if (!SD.begin(g_chip_select)) { // Assuming g_chip_select is the chip select pin
    Serial.println(F("SD.begin() Failed!"));
    return -1;
  }
  delay(20);

  // Copy first 6 characters of fname to logger_file_name
  for (int i = 0; i < 6; i++) {
    logger_file_name[i] = fname[i];
  }
  logger_file_name[6] = '\0'; // Null terminate the string

  // Copy logger_file_name to filename and append ".TXT"
  strcpy(filename, logger_file_name);
  strcat(filename, ".TXT");

  Serial.println(filename);

  // Open the file
  sdFile = SD.open(filename, FILE_WRITE);
  if (!sdFile) {
    Serial.println(F("Can't write to file"));
    return -1;
  }

  // Write data to file
  sdFile.print(g_mastername); // Assuming g_mastername is a global char array
  sdFile.print(",");
  sdFile.print(data);
  sdFile.print(",");
  sdFile.print(g_timestamp); // Assuming g_timestamp is a global variable
  sdFile.println();
  sdFile.close();  // Close the file
}


void due_command() {

  Serial.println(F("======================================"));
  Serial.println(F("KEY\tCOMMAND"));
  Serial.println(F("======================================"));
  Serial.println(F("[?]\tPrint stored config"));
  Serial.println(F("[A]\tRead Sensor Data "));
  Serial.println(F("[B]\tRead Sensor Data and build SMS"));
  Serial.println(F("[C]\tPrint this menu"));
  Serial.println(F("[D]\tSet SENSOR NAME"));
  Serial.println(F("[E]\tRead current and voltage"));
  Serial.println(F("[F]\tFinal dump"));
  Serial.println(F("[G]\tPrint SD directory"));
  Serial.println(F("[H]\tDump SD to PC"));
  //   Serial.println(F("Y.\tSD Data Dump to PC (REALTERM)"));
  //   Serial.println(F("Z.\tSend SD card data backup to Server"));


  Serial.println(F("======================================"));
  Serial.print(F("Enter Choice: "));
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry = dir.openNextFile();
    if (!entry) {
      dir.rewindDirectory();
      break;  // no more files
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.println(entry.name());

    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }

    entry.close();
    dir.close();
  }
}

void dumpSDtoPC() {
  root = SD.open("/");
  File dir = root;
  int numTabs = 0;
  Serial.println(F("Open REALTERM application to dump data to file"));
  String dirarray[20];
  int idx = 0;
  int i;
  while (true) {
    File entry = dir.openNextFile();
    Serial.println(entry.name());

    if (!entry) {
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    dirarray[idx] = entry.name();

    if (dirarray[idx][0] == '1' || dirarray[idx][0] == '2') {
      File dataFile = SD.open(dirarray[idx]);
      Serial.print(idx);
      if (dataFile) {
        while (dataFile.available()) {

          Serial.println(dataFile.readStringUntil('\n'));
        }
        dataFile.close();
      }
      // if the file isn't open, pop up an error:
      else {
        Serial.println("error opening check sd card");
      }
    }
  }
  root.close();
  dir.close();
}



unsigned int process_config_line(char *one_line) {
  char *str1 = one_line;
  int temp_int = 0;
  
  if ((startsWithIgnoreCase(str1, "mastername")) | (startsWithIgnoreCase(str1, "MasterName"))) {
    char* value = get_value_from_line(str1); // Assuming get_value_from_line returns char*
    strncpy(g_mastername, value, 5); // Copy at most 5 characters to g_mastername
    g_mastername[5] = '\0'; // Ensure null termination
    return 0;

  } else if (startsWithIgnoreCase(str1, "turn_on_delay")) {
    g_turn_on_delay = atoi(get_value_from_line(str1)); // Convert to integer using atoi
    return 0;
    
  } else if (startsWithIgnoreCase(str1, "PIEZO") || startsWithIgnoreCase(str1, "Piezo")) {
    temp_int = atoi(get_value_from_line(str1));
    if (temp_int == 1) {
      has_piezo = true;
    } else if (temp_int == 0) {
      has_piezo = false;
    } else {
      has_piezo = false;
    }
    return 0;

  } else if (startsWithIgnoreCase(str1, "dataloggerVersion")) {
    g_datalogger_version = atoi(get_value_from_line(str1)); // Convert to integer using atoi
    return 0;

  } else if (startsWithIgnoreCase(str1, "sensorVersion")) {
    g_sensor_version = atoi(get_value_from_line(str1)); // Convert to integer using atoi
    return 0;

  } else if (startsWithIgnoreCase(str1, "broadcastTimeout") || startsWithIgnoreCase(str1, "broadcast_timeout")
              || startsWithIgnoreCase(str1, "broadcastTimeout") || startsWithIgnoreCase(str1, "broadcastTimeout")) {
    broad_timeout = atoi(get_value_from_line(str1)); // Convert to integer using atoi
    return 0;

  } else if ((startsWithIgnoreCase(str1, "sampling_max_retry")) || (startsWithIgnoreCase(str1, "sampling_max_num_of_retry"))) {
    g_sampling_max_retry = atoi(get_value_from_line(str1)); // Convert to integer using atoi
    return 0;

  } else if (startsWithIgnoreCase(str1, "columnids") || startsWithIgnoreCase(str1, "column1")
  | startsWithIgnoreCase(str1, "columnIDs") || startsWithIgnoreCase(str1, "columnIDs")) {
    g_num_of_nodes = process_column_ids(str1);
    return 0;

  } else if (startsWithIgnoreCase(str1, "b64")) {
    b64 = atoi(get_value_from_line(str1)); // Convert to integer using atoi
    return 0;

  } else if (startsWithIgnoreCase(str1, "endofconfig") | startsWithIgnoreCase(str1, "ENDOFCONFIG")) {
    return 1;

  } else {
    return 0;
  }
}


bool startsWithIgnoreCase(const char* str, const char* prefix) {
    while (*prefix) {
        if (tolower(*str) != tolower(*prefix)) {
            return false;
        }
        str++;
        prefix++;
    }
    return true;
}


int process_column_ids(char* line) { // changed string type to char type
  char delim = ',';
  int i, i0, i1, i2, ilast, id;
  i = 0;
  ilast = strrchr(line, delim) - line;
  i0 = strrchr(line, '=') - line;
  i1 = strchr(line, delim) - line;
  line[i0] = '\0'; // Null-terminate the substring
  id = atoi(line + i0 + 1);
  g_gids[i][0] = id;
  i++;
  while ((i1 <= ilast) && (i < 40)) {
    i2 = strchr(line + i1 + 1, delim) - line;
    line[i2] = '\0'; // Null-terminate the substring
    id = atoi(line + i1 + 1);
    g_gids[i][0] = id;
    i++;
    i1 = i2;
    if (i1 == -1)
      break;
  }
  return i;  // dating i+1
}

void open_config() {
  File g_file;
  char *temp;
  int max_char_per_line = 1000;
  char one_line[max_char_per_line];
  g_file = SD.open("CONFIG.txt");

  if (!g_file) {
    Serial.println("CONFIG.txt not found.");
    return;
  }
  memset(one_line, 0, sizeof(one_line));
  // temp starts at address of one_line
  temp = one_line;
  // para may paglalagyan ng result ng .peek()
  while ((*temp = g_file.peek()) != -1) {
    // start ulit sa address ng one_line
    temp = one_line;
    while (((*temp = g_file.read()) != '\n')) {
      temp++;
      if (*temp == -1)
        break;
    }
    if (process_config_line(one_line)) {
      break;
    }
    memset(one_line, 0, sizeof(one_line));
  }
  Serial.println("Finished config processing");
  print_stored_config();
  g_file.close();
  return;
}

