/**
 * @file can.ino
 * contains functions for the controller area network ( CAN )
 * 
 */



/** 
 * CAN Bus Baudrate.
 */
#define CAN_BAUD_RATE 40000

/**  
 * Enable Pin for Due / SAM3X board 
 */
#define CAN0_EN 50

/**  
 * GPIO for controlling the column switch
 */
#//define COLUMN_SWITCH_PIN 44

/**  
 * GPIO for controlling the LED indicator for the column switch
 */
#define LED1 48

/** 
 * @brief Set the CAN baudrate, CAN Enable Pin. 
 * 
 * Set the CAN baudrate for Can0 to #CAN_BAUD_RATE. 
 * 
 * Set mailbox0 as receiver.
 * 
 * Set mailbox0 to receive the extended ID format.
 * 
 * Set mask to recieve all. 
 * 
 * Set mailbox1 transfer ID to extended ID format. 
 * 
 * Set GPIO #COLUMN_SWITCH_PIN to output mode.
 * 
 * Set GPIO #LED1 to output mode.
 * 
 * Write LOW to #COLUMN_SWITCH_PIN. 
 * 
 * \note CAN Enable Pin for Can0 -> 50
 * \note CAN Enable Pin for Can1 -> 48
 * \source
 * \return Return false on failure of setting CAN_BAUD_RATE.
*/
void canInit(){
  if (VERBOSE == 1) { Serial.println("init_can()"); }
  pinMode(50, OUTPUT);
  if (Can0.begin(40000, 50)) {
    if (VERBOSE == 1) { Serial.println("Can0 - initialization completed."); }
  }

    Can0.watchFor();
    Can0.mailbox_set_mode(0, CAN_MB_RX_MODE);  // Set mailbox0 as receiver
    Can0.mailbox_set_id(0, 0, true);           // Set mailbox0 receive extendedID formats
    Can0.mailbox_set_accept_mask(0, 0, true);  // receive everything. // aralin yung mask
    Can0.mailbox_set_mode(1, CAN_MB_TX_MODE);  // Set mailbox1 as transmitter
    Can0.mailbox_set_id(1, 1, true);           // Set mailbox1 transfer ID to 1 extended id
}

/* 
  Function: init_char_arrays

    Clear the global char a
  
  Parameters:
  
    n/a
  
  Returns:
  
    n/a
  
  See Also:
  
    <setup>
*/
void init_char_arrays() {
  for (int i = 0; i < 2500; i++) {
    g_temp_dump[i] = '\0';
  }
  for (int a = 0; a < 5000; a++) {
    g_final_dump[a] = '\0';
  }
  for (int b = 0; b < 1250; b++) {
    g_no_gids_dump[b] = '\0';
  }
}


void clear_can_buffer(CAN_FRAME can_buffer[]) {
  for (int i = 0; i < CAN_ARRAY_BUFFER_SIZE; i++) {
    can_buffer[i] = {};
  }
}

void get_data(int cmd, int transmit_id, char* final_dump) {
  int retry_count = 0, respondents = 0;

  int count, uid, ic;
  if (cmd < 100) {
    for (retry_count = 0; retry_count < g_sampling_max_retry; retry_count++) {

      canSend(cmd, transmit_id);

      if ((respondents = get_all_frames(broad_timeout, g_can_buffer, g_num_of_nodes)) == g_num_of_nodes) {
        Serial.println("Complete frames! :) ");
        Serial2.println("Complete frames! :) ");
        break;
      } else {

        print_buffer[0] = '\0';
        itoa(respondents, num_buffer, 10);
        strcpy(print_buffer, num_buffer);
        strcat(print_buffer, " / ");
        itoa(g_num_of_nodes, num_buffer, 10);
        strcat(print_buffer, num_buffer);
        strcat(print_buffer, " received / expected frames.");
        strcat(print_buffer, '\0');
        Serial.println(print_buffer);
        if (strlen(print_buffer) != 0) {
          Serial2.println(print_buffer);
        }
        // Serial.print(respondents); Serial.print(" / ");
        // Serial.print(g_num_of_nodes); Serial.println(" received / expected frames.");
        // Serial2.print(g_num_of_nodes); Serial2.println(" received / expected frames.");
      }
    }

  } else if (cmd == 256) {
    for (int i = 0; i < g_num_of_nodes; i++) {
      uid = g_gids[i][0];
      version_1(uid);
      get_all_frames(broad_timeout, g_can_buffer, g_num_of_nodes);
      count = count_frames(g_can_buffer);
      // write frames to String or char array
      for (int i = 0; i < count; i++) {
        if (g_can_buffer[i].id != 0) {
          b64_write_frame_to_dump(g_can_buffer[i], g_temp_dump);
        }
      }
    }  //
    
 } else if ((cmd >= 100) && (cmd < 255)) {
    for (int i = 0; i < g_num_of_nodes; i++) {
      uid = g_gids[i][0];
      poll_command(cmd, uid);  //hindi ko ba kailangang ireset yung column?
      Serial.print(F("Polling UID: "));
      Serial.print(uid);
      for (retry_count = 0; retry_count < g_sampling_max_retry + 2; retry_count++) {
        Serial.print(" .");
        // contains 1 after first run.
        if (get_one_frame(POLL_TIMEOUT, g_can_buffer, uid) == uid) {
          Serial.println(" OK");
          break;
        }
      }
    }
  } 
  count = count_frames(g_can_buffer);
  // write frames to String or char array
  for (int i = 0; i < count; i++) {
    if (g_can_buffer[i].id != 0) {
      b64_write_frame_to_dump(g_can_buffer[i], g_temp_dump);

    }
  }
  strcat(g_temp_dump, g_delim);
  strcat(g_temp_dump, '\0');
  Serial.print("g_temp_dump: ");
  Serial.println(g_temp_dump);
  Serial2.println(g_temp_dump);
  b64_process_g_temp_dump(g_temp_dump, final_dump, g_no_gids_dump);
  for (int i = 0; i < 2500; i++) {
    g_temp_dump[i] = '\0';
  }
  clear_can_buffer(g_can_buffer);
  Serial.println(F("================================="));
}

int get_all_frames(int timeout_ms, CAN_FRAME can_buffer[], int expected_frames) {
  int timestart = millis();
  int a = 0, i = 0;
  CAN_FRAME incoming;
  if (VERBOSE == 1) { Serial.println("get_all_frames()"); }

  do {
    check_can_status();
    if (Can0.available()) {
      can_flag = true;
      Can0.read(incoming);
      if (g_sensor_version == 1) {
        can_buffer[i].id = incoming.id / 8;
      } else {
        can_buffer[i].id = incoming.id;
      }

      can_buffer[i].data.byte[0] = incoming.data.byte[0];
      can_buffer[i].data.byte[1] = incoming.data.byte[1];
      can_buffer[i].data.byte[2] = incoming.data.byte[2];
      can_buffer[i].data.byte[3] = incoming.data.byte[3];
      can_buffer[i].data.byte[4] = incoming.data.byte[4];
      can_buffer[i].data.byte[5] = incoming.data.byte[5];
      can_buffer[i].data.byte[6] = incoming.data.byte[6];
      can_buffer[i].data.byte[7] = incoming.data.byte[7];
      i++;
      interpret_frame(incoming);
      //        if (i == expected_frames){
      process_all_frames(g_can_buffer);
      i = count_frames(g_can_buffer);
      //          if (i == expected_frames){
      //            return i;
      //            break;
      //          }
      //        }
    }
    if (comm_mode == "LORA") {
      if ((millis() - arq_start_time) >= ARQTIMEOUT) {
        arq_start_time = millis();
        Serial.println("ARQWAIT");
        LORA.print("ARQWAIT");
      }
    }
  } while ((millis() - timestart <= timeout_ms));
  process_all_frames(g_can_buffer);
  return count_frames(g_can_buffer);
}

int get_one_frame(int timeout_ms, CAN_FRAME can_buffer[], int expected_uid) {
  int timestart = millis();
  int a = 0, i = 0;
  i = count_frames(can_buffer);
  CAN_FRAME incoming;
  if (VERBOSE == 1) { Serial.println("get_one_frame()"); }
  do {
    check_can_status();
    if (Can0.available()) {
      Can0.read(incoming);
      can_buffer[i].id = incoming.id;
      can_buffer[i].data.byte[0] = incoming.data.byte[0];
      can_buffer[i].data.byte[1] = incoming.data.byte[1];
      can_buffer[i].data.byte[2] = incoming.data.byte[2];
      can_buffer[i].data.byte[3] = incoming.data.byte[3];
      can_buffer[i].data.byte[4] = incoming.data.byte[4];
      can_buffer[i].data.byte[5] = incoming.data.byte[5];
      can_buffer[i].data.byte[6] = incoming.data.byte[6];
      can_buffer[i].data.byte[7] = incoming.data.byte[7];
      i++;
      if (incoming.id == expected_uid) {
        process_all_frames(g_can_buffer);
        return incoming.id;
      }
    }
    if (comm_mode == "ARQ") {
      if ((millis() - arq_start_time) >= ARQTIMEOUT) {
        arq_start_time = millis();
        Serial.println("ARQWAIT");
        DATALOGGER.print("ARQWAIT");
      }
    }
  } while ((millis() - timestart <= timeout_ms));
  process_all_frames(g_can_buffer);
  return 0;
}

void process_all_frames(CAN_FRAME can_buffer[]) {
  int count, i;
  //count = count_frames(can_buffer);
  // repeating frames filter
  delete_repeating_frames(can_buffer);
  // magnitude filter? i.e. kuha ulit data kapag bagsak sa magnitude?
}

int count_frames(CAN_FRAME can_buffer[]) {
  int i = 0, count = 0;
  for (i = 0; i < CAN_ARRAY_BUFFER_SIZE; i++) {
    if (can_buffer[i].id != 0) {
      count++;
    }
  }
  return count;
}

void delete_repeating_frames(CAN_FRAME can_buffer[]) {
  int i0 = 0, i1 = 0;
  int frame_count, i;
  frame_count = count_frames(can_buffer);
  for (i0 = 0; i0 < frame_count; i0++) {
    for (i1 = 0; i1 < frame_count; i1++) {
      if ((can_buffer[i0].id == can_buffer[i1].id) && (i0 != i1)) {
        can_buffer[i1] = {};  // clears the CAN_FRAME struct
      }
    }
  }
}

/*void write_frame_to_dump(CAN_FRAME incoming, char* dump) {
  char temp[5];
  sprintf(temp, "%04X", incoming.id);
  strcat(dump, temp);

  if (g_sensor_version == 1) {
    sprintf(temp, "%02X", incoming.data.byte[1]);
    strcat(dump, temp);

    sprintf(temp, "%02X", incoming.data.byte[0]);
    strcat(dump, temp);

    sprintf(temp, "%02X", incoming.data.byte[3]);
    strcat(dump, temp);

    sprintf(temp, "%02X", incoming.data.byte[2]);
    strcat(dump, temp);

    sprintf(temp, "%02X", incoming.data.byte[5]);
    strcat(dump, temp);

    sprintf(temp, "%02X", incoming.data.byte[4]);
    strcat(dump, temp);

  }

  else {
    sprintf(temp, "%02X", incoming.data.byte[0]);
    strcat(dump, temp);

    sprintf(temp, "%02X", incoming.data.byte[1]);
    strcat(dump, temp);

    sprintf(temp, "%02X", incoming.data.byte[2]);
    strcat(dump, temp);

    sprintf(temp, "%02X", incoming.data.byte[3]);
    strcat(dump, temp);

    sprintf(temp, "%02X", incoming.data.byte[4]);
    strcat(dump, temp);

    sprintf(temp, "%02X", incoming.data.byte[5]);
    strcat(dump, temp);
  }

  sprintf(temp, "%02X", incoming.data.byte[6]);
  strcat(dump, temp);

  sprintf(temp, "%02X-", incoming.data.byte[7]);
  strcat(dump, temp);

  // interpret_frame(incoming);
  return;
}*/

int convert_uid_to_gid(int uid) {
  int gid = 0;
  if ((uid == 0) | (uid == -1)) {
    return -1;
  }
  for (int i = 0; i < g_num_of_nodes; i++) {
    if (g_gids[i][0] == uid) {
      return g_gids[i][1];
    }
  }
  return 0;
}

/* 
  Function: process_g_temp_dump

    Process g_string:

    - Separate the main string by the delimiter "+".

    - Convert the uid ( universal id ) to the gid ( geographic id ).

    - Successful conversion of uid to gid : delimiter "-"

    - Failed conversion of uid to gid : delimiter "="

    - Write in *<g_final_dump>*

        Format of data in *<g_string_proc>*:
    
        delimiter - 1 char
        gids - 4 chars
        data - 16 chars

        *[delimiter][gids][data]*

  Parameters:

    dump - pointer to source char array to be processed.

    final_dump - pointer to char array where to write the processed char array.

  Returns:

    n/a

  See Also:

    <get_data>
*/
/*void process_g_temp_dump(char* dump, char* final_dump, char* no_gids_dump) {
  char *token, *last_char;
  char temp_id[5], temp_gid[5], temp_data[17];
  int id_int, gid;
  token = strtok(dump, "-");
  while (token != NULL) {
    // get gid
    strncpy(temp_id, token, 4);
    id_int = strtol(temp_id, &last_char, 16);
    gid = convert_uid_to_gid(id_int);
    if (id_int == 255) {  // account for piezometer
      gid = 255;
    }
    if ((gid != 0) & (gid != -1)) {
      sprintf(temp_gid, "%04X", gid);
      strncpy(temp_data, token + 4, 16);
      // strncat(final_dump,"-",1);
      strncat(final_dump, temp_gid, 4);
      strncat(final_dump, temp_data, 16);

    } else if (gid == 0) {
      strncat(no_gids_dump, "=", 1);
      strncat(no_gids_dump, token, 20);
    }
    token = strtok(NULL, "-");
  }

  strncat(final_dump, g_delim, 1);  // add delimiterfor different data type
}*/

void interpret_frame(CAN_FRAME incoming) {

  print_buffer[0] = '\0';
  int id, d1, d2, d3, d4, d5, d6, d7, d8, somsr, temper;
  int16_t x, y, z;
  int tilt = 1;
  int soms = 0;
  char temp[6];
  float v;

  id = incoming.id;
  d1 = incoming.data.byte[0];
  d2 = incoming.data.byte[1];
  d3 = incoming.data.byte[2];
  d4 = incoming.data.byte[3];
  d5 = incoming.data.byte[4];
  d6 = incoming.data.byte[5];
  d7 = incoming.data.byte[6];
  d8 = incoming.data.byte[7];

  if (VERBOSE == 1) { Serial.println("process_frame()"); }
  if ((d1 == 11) | (d1 == 12) | (d1 == 32) | (d1 == 33) | (d1 == 41) | (d1 == 42) | (d1 == 51) | (d1 == 52)) {

    x = compute_axis(d2, d3);
    y = compute_axis(d4, d5);
    z = compute_axis(d6, d7);
    v = ((d8 + 200.0) / 100.0);

    strcpy(print_buffer, "\t");
    itoa(id, num_buffer, 16);
    strcat(print_buffer, num_buffer);
    strcat(print_buffer, "\t");
    itoa(id, num_buffer, 10);
    strcat(print_buffer, num_buffer);
    strcat(print_buffer, "\t");
    itoa(convert_uid_to_gid(id), num_buffer, 10);
    strcat(print_buffer, num_buffer);
    strcat(print_buffer, "\t\t");
    itoa(x, num_buffer, 10);
    strcat(print_buffer, num_buffer);
    strcat(print_buffer, "\t");
    itoa(y, num_buffer, 10);
    strcat(print_buffer, num_buffer);
    strcat(print_buffer, "\t");
    itoa(z, num_buffer, 10);
    strcat(print_buffer, num_buffer);
    strcat(print_buffer, "\t");
    dtostrf(v, 0, 2, num_buffer);
    strcat(print_buffer, num_buffer);

    Serial.println(print_buffer);
    Serial2.println(print_buffer);

  } else if ((d1 == 10) | (d1 == 13) | (d1 == 110) | (d1 == 113)) {
    somsr = compute_axis(d2, d3);
    Serial.print("\t");
    Serial.print(id, HEX);
    Serial.print("\t");
    Serial.print(id);
    Serial.print('\t');
    Serial.print(convert_uid_to_gid(id));
    Serial.print('\t');
    Serial.print("somsr: ");
    Serial.println(somsr);
  }
}

  int compute_axis(int low, int high) {
  if (g_sensor_version < 5) {
    int value = 5000;
    if (!b64) {
      if (high >= 240) {
        high = high - 240;
        value = (low + (high * 256)) - 4095;
      } else {
        value = (low + (high * 256));
      }
      return value;

    } else if (b64) {
      if (high >= 240) {
        high = high - 240;
      }
      value = (low + (high * 256));
      return value;
    }
  }

  else if (g_sensor_version >= 5) {
    if (!b64) {
      int16_t value;
      value = (high << 8) | low;
      return value;
    } else if (b64) {
      int value;
      value = (high << 8) | low;
      return value;
    }
  }
}

/* 
  Function: check_can_status

    Displays the current rx and tx error count given their sum is *not* zero.

  Parameters:

    n/a

  Returns:

    n/a

  See Also:

    <get_all_frames>
*/
void check_can_status() {
  int rx_error_cnt = 0, tx_error_cnt = 0;
  rx_error_cnt = Can0.get_rx_error_cnt();
  tx_error_cnt = Can0.get_tx_error_cnt();
  if (rx_error_cnt + tx_error_cnt != 0) {
    if (VERBOSE) {
      Serial.print("rx_error : ");
      Serial.print(rx_error_cnt);
      Serial.print("\t tx_error :");
      Serial.println(tx_error_cnt);
    }
  }
  return;
}

void canSend(int command, int transmit_id){
    CAN_FRAME outgoing;
    outgoing.extended = true;
    outgoing.id = transmit_id;
    outgoing.length = 1;
    outgoing.data.byte[0] = command;
    Can0.sendFrame(outgoing);
}

void poll_command(int command, int uid) {
  if (VERBOSE == 1) { Serial.println("send_frame()"); }
  CAN_FRAME outgoing;
  outgoing.extended = true;
  outgoing.id = 1;
  outgoing.length = 3;
  outgoing.data.byte[0] = command;
  outgoing.data.byte[1] = uid >> 8;
  outgoing.data.byte[2] = uid & 0xFF;
  Can0.sendFrame(outgoing);
}

void version_1(int uid) {
  CAN_FRAME outgoing;
  Can0.mailbox_set_mode(0, CAN_MB_RX_MODE);  // Set MB0 as receiver
  Can0.mailbox_set_id(0, 0, true);           // Set MB0 receive ID extended id
  Can0.mailbox_set_accept_mask(0, 0, true);  //make it receive everything seen in bus

  Can0.mailbox_set_mode(1, CAN_MB_TX_MODE);  // Set MB1 as transmitter
  //CAN.mailbox_set_id(1,MASTERMSGID, true);              // Set MB1 transfer ID to 1 extended id
  Can0.enable();

  Can0.enable_interrupt(CAN_IER_MB0);
  Can0.enable_interrupt(CAN_IER_MB1);
  Can0.mailbox_set_id(1, uid * 8, false);  //set MB1 transfer ID
  Can0.mailbox_set_id(0, uid * 8, false);  //MB0 receive ID
  Can0.mailbox_set_databyte(1, 0, 0x00);
  Can0.mailbox_set_databyte(1, 1, 0x00);
  Can0.global_send_transfer_cmd(CAN_TCR_MB1);  // Broadcast command


  delay(2000);
  Can0.global_send_transfer_cmd(CAN_TCR_MB1);
}



