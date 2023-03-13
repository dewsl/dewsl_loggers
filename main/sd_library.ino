/* 
	Function: sd_init

		Assert the chip_select pin for SD use.

	Parameters:
   n/a

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
  } else if (g_datalogger_version == 4) {
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
	Function: get_value_from_line

		Extract the value from current line of the config.

	Parameters:

		line - String of one line read from config 

	Returns:

		String after "=" stripped of spaces

	See Also:

		<process_config_line>
*/
String get_value_from_line(String line) {
  String delim = "=";
  String other_chars = " ";
  int index, i;
  while ((i = line.indexOf(other_chars)) != -1) {
    line.remove(i, 1);
  }
  index = line.lastIndexOf(delim);
  return line.substring(index + 1);
}

/* 
	Function: writeData

		Write data to sd card. Copied from rtc_sd_due ( Duelogger )

	Parameters:

		data -  String data to be written

	Returns:

		-1 - for failure

	See Also:
		<>
*/
int8_t writeData(String fname, String data) {

  File sdFile;
  char filename[100] = {};
  char logger_file_name[8] = {};
  String timeString;
  //	 Serial.println(g_timestamp);
  //
  //	 if (!SD.begin(6,g_chip_select)) {
  //	 	Serial.println(" SD.begin() Failed!");
  //	 	return -1;
  //	 }
  delay(20);

  fname.trim();
  for (int i = 0; i < 6; i++) {
    logger_file_name[i] = fname.charAt(i);
  }
  //	fname.substring(0,6).toCharArray(filename,6);

  //	Serial.println(logger_file_name);
  strcpy(filename, logger_file_name);
  strcat(filename, ".TXT");
  Serial.println(filename);
  sdFile = SD.open(filename, FILE_WRITE);
  if (!sdFile) {
    Serial.println(F("Can't Write to file"));
    return -1;
  }
  sdFile.print(g_mastername);
  sdFile.print(",");
  sdFile.print(data);
  sdFile.print(",");
  sdFile.print(g_timestamp);
  sdFile.print('\n');
  sdFile.close();  //close the file
                   // Serial.println("writing to SD");
}
/* 
  Function: print_due_command2

    Print in the AT command options with corresponding keys
    for debug mode

  Parameters:

    n/a

  Returns:

    n/a

  See Also:

    n/a
*/
void print_due_command2() {

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
  Serial.println(F("[I]\tOverride library config with SD"));
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

/* 
	Function: process_config_line
		Reads one line config and interprets accordingly.
	Parameters:
		*one_line - pointer to a char array that contains the current line of Config being processed.
	Returns:
		0 - successful read of Config line
		1 - reached the end of Config
	See Also:
	
		<open_config>
*/
unsigned int process_config_line(char *one_line) {
  String str1;
  int temp_int = 0;
  str1 = String(one_line);
  if ((str1.startsWith("mastername")) | (str1.startsWith("MasterName"))) {
    get_value_from_line(str1).toCharArray(g_mastername, 6);
    return 0;

  } else if (str1.startsWith("turn_on_delay")) {
    g_turn_on_delay = get_value_from_line(str1).toInt();
    return 0;

  } else if (str1.startsWith("PIEZO") | str1.startsWith("Piezo")) {
    temp_int = get_value_from_line(str1).toInt();
    if (temp_int == 1) {
      has_piezo = true;
    } else if (temp_int == 0) {
      has_piezo = false;
    } else {
      has_piezo = false;
    }
    return 0;

  } else if (str1.startsWith("dataloggerVersion")) {
    g_datalogger_version = get_value_from_line(str1).toInt();
    return 0;

  } else if (str1.startsWith("sensorVersion")) {
    g_sensor_version = get_value_from_line(str1).toInt();
    return 0;

  } else if ((str1.startsWith("broadcastTimeout")) | (str1.startsWith("broadcast_timeout"))
             | (str1.startsWith("broadcasttimeout")) | (str1.startsWith("BroadcastTimeout"))) {
    broad_timeout = get_value_from_line(str1).toInt();
    return 0;

  } else if ((str1.startsWith("sampling_max_retry")) | (str1.startsWith("sampling_max_num_of_retry"))) {
    g_sampling_max_retry = get_value_from_line(str1).toInt();
    return 0;

  } else if ((str1.startsWith("columnids")) | (str1.startsWith("column1"))
             | (str1.startsWith("columnIDs")) | (str1.startsWith("columnIDs"))) {
    g_num_of_nodes = process_column_ids(str1);
    return 0;

  } else if (str1.startsWith("b64")) {
    b64 = get_value_from_line(str1).toInt();
    return 0;

  } else if ((str1.startsWith("endofconfig")) | (str1.startsWith("ENDOFCONFIG"))) {
    return 1;

  } else {
    return 0;
  }
}
/*
	Function: process_column_ids
		Find the unique ids from string and place in *<g_gids>*
	Parameters:
		line - String object containing ids
	Returns:
		integer number of unique ids read.
	See Also:
		<process_config_line>
*/
int process_column_ids(String line) {
  String delim = ",";
  int i, i0, i1, i2, ilast, id;
  i = 0;
  ilast = line.lastIndexOf(delim);
  i0 = line.lastIndexOf("=");
  i1 = line.indexOf(delim);
  id = line.substring(i0 + 1, i1).toInt();
  g_gids[i][0] = id;
  i++;
  while ((i1 <= ilast) & (i < 40)) {
    i2 = line.indexOf(delim, i1 + 1);
    id = line.substring(i1 + 1, i2).toInt();
    g_gids[i][0] = id;
    i++;
    i1 = i2;
    if (i1 == -1)
      break;
  }
  return i;  // dating i+1
}

/* 
	Function: open_config
		Open the CONFIG.txt file in the SD card and process
		each line.	
	Parameters:
		n/a
	Returns:
		n/a
	See Also:
	
		<getATCommand>
*/
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