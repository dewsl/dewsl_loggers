/* 
  Function: init_can

    - Set the chip select and interrupt pins, clock frequency, and CAN baud rate.

    - Disable chip select for LoRa and SD card modules

  Parameters:
    
    n/a

  Returns:
    
    n/a
  
  See Also:

    <setup>
*/
void init_can()
{
  digitalWrite(RFM95_CS, HIGH);
  digitalWrite(SD_CS_PIN, HIGH);
  if (VERBOSE == 1)
  {
    Serial.println("init_can()");
  }
  CAN.setPins(CAN_CS_PIN, CAN_IRQ_PIN);
  CAN.setClockFrequency(8E6); //8MHz clock
  if (CAN.begin(40E3))
  {
    if (VERBOSE == 1)
    {
      Serial.println("CAN initialization completed.");
    }
  }
  else
  {
    Serial.println("CAN initialization failed!");
  }
}

/* 
  Function: init_strings

    Clear the global strings used for storage 
  
  Parameters:
  
    n/a
  
  Returns:
  
    n/a
  
  See Also:
  
    <setup>
*/
void init_strings()
{
  g_string = "";
  g_string_proc = "";
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
void init_char_arrays()
{
  for (int i = 0; i < 2500; i++)
  {
    g_temp_dump[i] = '\0';
  }
  for (int a = 0; a < 5000; a++)
  {
    g_final_dump[a] = '\0';
  }
  for (int b = 0; b < 1250; b++)
  {
    g_no_gids_dump[b] = '\0';
  }
}

/* 
  Function: clear_can_buffer

    Clear the can buffer array.

  Parameters:
  
    can_buffer - CAN_FRAME structure 
    
  Returns:
  
    n/a
  
  See Also:
  
    <get_data>
  
*/
void clear_can_buffer(CAN_FRAME can_buffer[])
{
  for (int i = 0; i < CAN_ARRAY_BUFFER_SIZE; i++)
  {
    can_buffer[i] = {};
  }
}

/* 
  Function: can_read

    Reads the data from the CAN bus and package as CAN_FRAME.

  Parameters:
  
    buffer - temporary storage of incoming CAN_FRAME data
    
  Returns:
  
    n/a
  
  See Also:
  
    n/a
  
*/
void can_read(CAN_FRAME &buffer)
{
  buffer.id = CAN.packetId();
  buffer.extended = CAN.packetExtended();
  buffer.length = CAN.packetDlc();
  for (int i = 0; i < 8; i++)
  {
    buffer.data.byte[i] = CAN.read();
  }
}

/* 
  Function: send_command

    Populates an outgoing frame with a command and transmit id then broadcasts the frame.

  Parameters:

    cmd_code - integer message id ( msgid )

    mcu_tx_id - integer id of frame to be broadcast. ( useful for Version 1 Sensors )

  Returns:

    n/a

  See Also:

    <get_all_frames>
*/
void send_command(int cmd_code, int mcu_tx_id)
{
  if (VERBOSE == 1)
  {
    Serial.println("send_frame()");
  }
  CAN.beginExtendedPacket(mcu_tx_id);
  CAN.write(cmd_code);
  CAN.endPacket();
}

/* 
  Function: get_data

    Wrapper function that moderates the sending and gathering of CAN Frames.
    This function is responsible for managing the retries, turning off and on of the column. 

    *cmd < 100 (Broadcast Mode)*
    
    Broadcast mode sends a CAN frame through <send_command>, for all the sensor nodes to receive and interpret. 
    <get_all_frames> waits for CAN Frames until <broad_timeout> expires. 
    The function internally counts the number of valid frames received.
    Upon reaching <g_num_of_nodes>, or the set timeout, the column is turned off.
    The arbitration is handled by the CAN protocol.

    *cmd >= 100 (Polling Mode)*

    A specifically structured CAN Frame is broacast to all sensors on the CAN Bus.
    The CAN Frame contains the low byte and high byte of the specific sensor uid.
    That specific sensor is expected to be the only valid responder. 
    <get_one_frame> waits for the valid frame or for timeout to expire.

    The collected CAN Frames are written to <g_temp_dump> before being processed by 
    <process_g_temp_dump>.

    Finally, <g_can_buffer> is emptied.

  Parameters:

    cmd - msgid
    transmit_id - id of the transmitted frame

  Returns:

    n/a

  See Also:

    <getATcommand>
*/
void get_data(int cmd, int transmit_id, char *final_dump)
{
  int retry_count = 0, respondents = 0;
  float g_current = ina219.getCurrent_mA();
  float g_voltage = ina219.getBusVoltage_V();
  dtostrf(g_current, 0, 2, g_test);
  sprintf(g_build, g_test, strlen(g_test));
  dtostrf(g_voltage, 0, 2, g_test);
  strncat(g_build, "*", 1);
  strncat(g_build, g_test, strlen(g_test));
  strncat(g_build, "*", 1);

  int count, uid, ic;
  if (cmd < 100)
  {
    for (retry_count = 0; retry_count < g_sampling_max_retry; retry_count++)
    {

      turn_on_column();
      delay(1000);
      g_current = 0;
      g_current = ina219.getCurrent_mA();
      g_voltage = ina219.getBusVoltage_V();
      dtostrf(g_current, 0, 2, g_test);
      strncat(g_build, g_test, strlen(g_test));
      dtostrf(g_voltage, 0, 2, g_test);
      strncat(g_build, "*", 1);
      strncat(g_build, g_test, strlen(g_test));
      strncat(g_build, "*", 1);

      send_command(cmd, transmit_id);

      if ((respondents = get_all_frames(broad_timeout, g_can_buffer, g_num_of_nodes)) == g_num_of_nodes)
      {
        Serial.println("Complete frames! :) ");
        turn_off_column();
        break;
      }
      else
      {
        Serial.print(respondents);
        Serial.print(" / ");
        Serial.print(g_num_of_nodes);
        Serial.println(" received / expected frames.");
      }
      turn_off_column();
    }
  }
  else if (cmd == 255)
  {
    turn_on_column();
    //poll_piezo();
    for (retry_count = 0; retry_count < g_sampling_max_retry + 2; retry_count++)
    {
      Serial.print(" .");
      if (get_one_frame(POLL_TIMEOUT, g_can_buffer, 255))
      {
        Serial.println(" OK");
        break;
      }
    }
  }
  else if (cmd == 256)
  {
    turn_on_column();
    for (int i = 0; i < g_num_of_nodes; i++)
    {
      uid = g_gids[i][0];
      //version_1(uid);
      get_all_frames(broad_timeout, g_can_buffer, g_num_of_nodes);
      count = count_frames(g_can_buffer);
      // write frames to String or char array
      for (int i = 0; i < count; i++)
      {
        if (g_can_buffer[i].id != 0)
        {
          if (b64 == 1)
          {
            b64_write_frame_to_dump(g_can_buffer[i], g_temp_dump);
          }
          else
          {
            write_frame_to_dump(g_can_buffer[i], g_temp_dump);
          }
        }
      }
    }
    turn_off_column();
  }
  else if ((cmd >= 100) && (cmd < 255))
  {
    turn_on_column();
    for (int i = 0; i < g_num_of_nodes; i++)
    {
      uid = g_gids[i][0];
      //poll_command(cmd,uid);
      Serial.print(F("Polling UID: "));
      Serial.print(uid);
      for (retry_count = 0; retry_count < g_sampling_max_retry + 2; retry_count++)
      {
        Serial.print(" .");
        // contains 1 after first run.
        if (get_one_frame(POLL_TIMEOUT, g_can_buffer, uid) == uid)
        {
          Serial.println(" OK");
          break;
        }
      }
    }
  }
  count = count_frames(g_can_buffer);
  // write frames to String or char array
  for (int i = 0; i < count; i++)
  {
    if (g_can_buffer[i].id != 0)
    {
      if (b64 == 1)
      {
        b64_write_frame_to_dump(g_can_buffer[i], g_temp_dump);
      }
      else
      {
        write_frame_to_dump(g_can_buffer[i], g_temp_dump);
      }
    }
  }
  strncat(g_temp_dump, g_delim, 1);
  //strcat(g_temp_dump, '\0');
  Serial.print("g_temp_dump: ");
  Serial.println(g_temp_dump);
  if (b64 == 1)
  {
    b64_process_g_temp_dump(g_temp_dump, final_dump, g_no_gids_dump);
  }
  else
  {
    process_g_temp_dump(g_temp_dump, final_dump, g_no_gids_dump);
  }

  for (int i = 0; i < 2500; i++)
  {
    g_temp_dump[i] = '\0';
  }
  // for (int a = 0; a < 5000; a++){
  //   g_final_dump[a] = '\0';
  // }

  clear_can_buffer(g_can_buffer);
  Serial.println(F("================================="));
  g_current = ina219.getCurrent_mA();
  g_voltage = ina219.getBusVoltage_V();
  dtostrf(g_current, 0, 2, g_test);
  strncat(g_build, g_test, strlen(g_test));
  dtostrf(g_voltage, 0, 2, g_test);
  strncat(g_build, "*", 1);
  strncat(g_build, g_test, strlen(g_test));
  Serial.println(g_build);
  vc_flag = true;
}

/* 
  Function: get_all_frames

    - Receive all the incoming frames.
    
    - Place the frames in a CAN_FRAME array.

    - If comm_mode = 1, checks the current time and sends ARQWAIT if time elapsed
    between st
    
    - Process the frames in the buffer <process_all_frames>

  Parameters:
  
    timeout_ms - int timeout in milliseconds

    can_buffer[] - CAN_FRAME struct array

    expected_frames - int expected number of unique frames from the sensors

  Returns:

    integer count of frames in the buffer

  See Also:

    <get_data>
*/
int get_all_frames(int timeout_ms, CAN_FRAME can_buffer[], int expected_frames)
{
  int timestart = millis();
  int a = 0, i = 0;
  CAN_FRAME incoming;
  if (VERBOSE == 1)
  {
    Serial.println("get_all_frames()");
  }

  do
  {
    //check_can_status();
    //if (CAN.available()){
    can_flag = true;
    while ((!CAN.parsePacket()) && (millis() - timestart <= timeout_ms))
      ;
    can_read(incoming);
    if (incoming.id == 0xFFFFFFFF)
      continue;
    if (g_sensor_version == 1)
    {
      can_buffer[i].id = incoming.id / 8;
    }
    else
    {
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
    //      }
  } while ((millis() - timestart <= timeout_ms));
  process_all_frames(g_can_buffer);
  return count_frames(g_can_buffer);
}

/* 
  Function: get_one_frame

    - Receive all the incoming frames.
    
    - Place the frames in a CAN_FRAME array.

    - If comm_mode = 1, checks the current time and sends ARQWAIT if time elapsed
    between st
    
    - Process the frames in the buffer <process_all_frames>

  Parameters:
  
    timeout_ms - int timeout in milliseconds

    can_buffer[] - CAN_FRAME struct array

    expected_uid - int expected uid of the sensor.

  Returns:

    integer uid of the sensor responding

    0 - the expected uid was not encountered by the process

  See Also:

    <get_data>
*/
int get_one_frame(int timeout_ms, CAN_FRAME can_buffer[], int expected_uid)
{
  int timestart = millis();
  int a = 0, i = 0;
  i = count_frames(can_buffer);
  CAN_FRAME incoming;
  if (VERBOSE == 1)
  {
    Serial.println("get_one_frame()");
  }
  do
  {
    //check_can_status();
    //if (CAN.available()){
    while ((!CAN.parsePacket()) && (millis() - timestart <= timeout_ms))
      ;
    can_read(incoming);
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
    if (incoming.id == expected_uid)
    {
      process_all_frames(g_can_buffer);
      return incoming.id;
    }
    if (comm_mode == "ARQ")
    {
      if ((millis() - arq_start_time) >= ARQTIMEOUT)
      {
        arq_start_time = millis();
        Serial.println("ARQWAIT");
      }
    }
  } while ((millis() - timestart <= timeout_ms));
  process_all_frames(g_can_buffer);
  return 0;
}

/* 
  Function: process_all_frames

    - Facilitates the processing of CAN Frames stored in the can_buffer[] array.
    
    - Processes include:
      
        - Deletion of repeating frames.
        
        - Deletion of out of magnitude range frames??

  Parameters:

    can_buffer - array of CAN_FRAME struct

  Returns:

    n/a

  See Also:

    <get_all_frames>
*/
void process_all_frames(CAN_FRAME can_buffer[])
{
  int count, i;
  //count = count_frames(can_buffer);
  // repeating frames filter
  delete_repeating_frames(can_buffer);
  // magnitude filter? i.e. kuha ulit data kapag bagsak sa magnitude?
}

/* 
  Function: count_frames

    Count the non-zero ids in the can_buffer[]
  
  Parameters:
  
    can_buffer - CAN_FRAME structure array
  
  Returns:
  
    count - integer number of non-zero ids in the can_bufer[]
  
  See Also:
  
    - <get_data>

    - <delete_repeating_frames>
*/
int count_frames(CAN_FRAME can_buffer[])
{
  int i = 0, count = 0;
  for (i = 0; i < CAN_ARRAY_BUFFER_SIZE; i++)
  {
    if (can_buffer[i].id != 0)
    {
      count++;
    }
  }
  return count;
}

/* 
  Function: delete_repeating_frames

    Replaces the values inside the CAN_FRAME struct with 0s
    if an id is not unique within the CAN_FRAME array. Compares ids only.
    0s the 2nd instance. The first is retained. 

  Parameters:

    can_buffer - array of the CAN_FRAME struct

  Returns:

    n/a

  See Also:

    <process_all_frames>
*/
void delete_repeating_frames(CAN_FRAME can_buffer[])
{
  int i0 = 0, i1 = 0;
  int frame_count, i;
  frame_count = count_frames(can_buffer);
  for (i0 = 0; i0 < frame_count; i0++)
  {
    for (i1 = 0; i1 < frame_count; i1++)
    {
      if ((can_buffer[i0].id == can_buffer[i1].id) && (i0 != i1))
      {
        can_buffer[i1] = {}; // clears the CAN_FRAME struct
      }
    }
  }
}

// Group: Char array based operations
/* 
  Function: write_frame_to_dump

    Write the frames as hex coded char array.
    Uids are written as 4 character hex.

    Format:
    gids - 4 char
    msgid - 2 chars
    data - 14 chars

    *[gids][msgid][data]*

    The format for data varies depending on msgid.

  Parameters:

    incoming- CAN_FRAME struct to be written in dump
    dump - pointer to char buffer

  Returns:

    n/a

  See Also:

    <process_g_temp_dump>
*/
void write_frame_to_dump(CAN_FRAME incoming, char *dump)
{
  char temp[5];
  sprintf(temp, "%04X", incoming.id);
  strcat(dump, temp);

  if (g_sensor_version == 1)
  {
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

  else
  {
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
}

/* 
  Function: convert_uid_to_gid

    Converts the uid (universal id) to the gid (geographic id)

  Parameters:

    uid - integer universal id

  Returns:

    gid - integer geographic id of given universal id according sd card config
    0 -  non existent uids
    -1 - for erroneous uids

  See Also:

    <process_g_string,>
*/
int convert_uid_to_gid(int uid)
{
  int gid = 0;
  if ((uid == 0) | (uid == -1))
  {
    return -1;
  }
  for (int i = 0; i < g_num_of_nodes; i++)
  {
    if (g_gids[i][0] == uid)
    {
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
void process_g_temp_dump(char *dump, char *final_dump, char *no_gids_dump)
{
  char *token, *last_char;
  char temp_id[5], temp_gid[5], temp_data[17];
  int id_int, gid;
  token = strtok(dump, "-");
  while (token != NULL)
  {
    // get gid
    strncpy(temp_id, token, 4);
    id_int = strtol(temp_id, &last_char, 16);
    gid = convert_uid_to_gid(id_int);
    if (id_int == 255)
    { // account for piezometer
      gid = 255;
    }
    if ((gid != 0) & (gid != -1))
    {
      sprintf(temp_gid, "%04X", gid);
      strncpy(temp_data, token + 4, 16);
      // strncat(final_dump,"-",1);
      strncat(final_dump, temp_gid, 4);
      strncat(final_dump, temp_data, 16);
    }
    else if (gid == 0)
    {
      strncat(no_gids_dump, "=", 1);
      strncat(no_gids_dump, token, 20);
    }
    token = strtok(NULL, "-");
  }

  strncat(final_dump, g_delim, 1); // add delimiterfor different data type
}

//Group: Serial Display Functions

/* 
  Function: interpret_frame

    Displays the following given a CAN frame:

      id - HEX extended id 

      id - integer extended id

      gid - integer geographic id

      x - integer x value 

      y -  integer y value

      z - integer z value

      v - float voltage value

  Parameters:

    incoming - CAN_FRAME struct

  Returns:

    n/a

  See Also:

    <write_frame_to_dump>
*/
void interpret_frame(CAN_FRAME incoming)
{
  int id, d1, d2, d3, d4, d5, d6, d7, d8, x, y, z, somsr, temper;
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

  if (VERBOSE == 1)
  {
    Serial.println("process_frame()");
  }
  if ((d1 == 11) | (d1 == 12) | (d1 == 32) | (d1 == 33))
  {

    x = compute_axis(d2, d3);
    y = compute_axis(d4, d5);
    z = compute_axis(d6, d7);
    v = ((d8 + 200.0) / 100.0);

    Serial.print("\t");
    Serial.print(id, HEX);
    Serial.print("\t");
    Serial.print(id);
    Serial.print('\t');
    Serial.print(convert_uid_to_gid(id));
    Serial.print('\t');
    sprintf(temp, "%5d", x);
    Serial.print(temp);
    temp[0] = '\0';
    Serial.print(" ");
    sprintf(temp, "%5d", y);
    Serial.print(temp);
    temp[0] = '\0';
    sprintf(temp, "%5d", z);
    Serial.print(" ");
    Serial.print(temp);
    Serial.print(" ");
    Serial.print(v);
    Serial.println("");
    if (vdata_flag)
    {
      int newIndex = 0;
      for (int i = 0; i < VDATASIZE; i++)
      {
        if (g_volt[i] > 0)
        {
          newIndex = newIndex + 1;
        }
      }
      g_volt[newIndex] = v;
    }
  }
  else if ((d1 == 10) | (d1 == 13) | (d1 == 110) | (d1 == 113))
  {
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

/* 
  Function: compute_axis

    Computes the X, Y or Z axis given it's high and low value.

  Parameters:

    d1 - integer low byte

    d2 - integer high byte

  Returns:

    value - integer converted value

  See Also:

    <interpret_frame>
*/
int compute_axis(int low, int high)
{
  int value = 5000;
  if (!b64)
  {
    if (high >= 240)
    {
      high = high - 240;
      value = (low + (high * 256)) - 4095;
    }
    else
    {
      value = (low + (high * 256));
    }
    return value;
  }
  else if (b64)
  {
    if (high >= 240)
    {
      high = high - 240;
    }
    value = (low + (high * 256));
    return value;
  }
}
