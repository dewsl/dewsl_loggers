/* 
  Function: init_can

    - Assert the enable pin for can transceiver (pin 50).

    - Set Can0 to allow all can trafic.

    - Set mailbox0 as receiver.

    - Set mailbox0 to receive extendedID formats.

    - Set mailbox1 as transmitter.

    - Set mailbox1 transferID to 1 (extendedID).

  Parameters:
    
    n/a

  Returns:
    
    n/a
  
  See Also:

    <setup>
*/
void init_can(){
  if (VERBOSE == 1) { Serial.println("init_can()"); }
  pinMode(50,OUTPUT);                       //Due Specific
  if (Can0.begin(40000,50)){                // also resets numBusError
    if(VERBOSE == 1) {Serial.println("Can0 - initialization completed.");}
  }
  Can0.watchFor();                          // allow all can traffic through sabi sa quick guide.
  Can0.mailbox_set_mode(0,CAN_MB_RX_MODE);  // Set mailbox0 as receiver
  Can0.mailbox_set_id(0, 0, true);          // Set mailbox0 receive extendedID formats
  Can0.mailbox_set_accept_mask(0,0,true);   // receive everything. // aralin yung mask

  Can0.mailbox_set_mode(1, CAN_MB_TX_MODE); // Set mailbox1 as transmitter
  Can0.mailbox_set_id(1,1, true);           // Set mailbox1 transfer ID to 1 extended id
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
void init_strings(){
  g_string = "";
  g_string_proc = ""; 
}

/* 
  Function: init_char_arrays

    Clear the global char arrays used for storage 
  
  Parameters:
  
    n/a
  
  Returns:
  
    n/a
  
  See Also:
  
    <setup>
*/
void init_char_arrays(){
  for (int i = 0; i < 1250; i++){
    g_temp_dump[i] = '\0';
  }
  for (int a = 0; a < 2500; a++){
    g_final_dump[a] = '\0';
  }
  for (int b = 0; b < 1250; b++){
    g_no_gids_dump[b] = '\0';
  }
  // g_temp_dump = (char *)malloc(1250*sizeof(char));
  // for (int i = 0; i < 1250; i++) {
  //   g_temp_dump[i] = '\0';
  // }

  // g_final_dump = (char *)malloc(2500*sizeof(char));
  // for (int i = 0; i < 2500; i++) {
  //   g_final_dump[i] = '\0';
  // }
}
/* 
  Function: clear_can_buffer

    Clear the can buffer array.

  Returns:
  
    n/a
  
  See Also:
  
    <get_data>
  
  Parameters:
  
    can_buffer - CAN_FRAME structure
  
*/
void clear_can_buffer(CAN_FRAME can_buffer[]){
  for(int i = 0; i< CAN_ARRAY_BUFFER_SIZE; i++){
    can_buffer[i] = {}; // clear all elements in buffer?
  }
}

/* 
  Function: get_data

    Clear the global strings used for storage.
    *Modifies the global char array g_no_gids_dump.*

  Parameters:

    cmd - msgid
    transmit_id - id of the transmitted frame

  Returns:

    n/a

  See Also:

    <getATcommand>
*/
void get_data(int cmd, int transmit_id, char* final_dump){
  int retry_count = 0,respondents = 0;
  int count,uid,ic;
  if (cmd < 100){
    for (retry_count = 0; retry_count < g_sampling_max_retry; retry_count++){
      turn_on_column();
      send_command(cmd,transmit_id);
      if( (respondents = get_all_frames(TIMEOUT,g_can_buffer,g_num_of_nodes)) == g_num_of_nodes){
        Serial.println("Complete frames! :) ");
        turn_off_column();
        break;
      } else {
        Serial.print(respondents); Serial.print(" / ");
        Serial.print(g_num_of_nodes); Serial.println(" received / expected frames.");
      }
      turn_off_column();
    }
  } else {
    for (int i = 0; i < g_num_of_nodes; i++){
      turn_on_column();
      uid = g_gids[i][0];
      poll_command(cmd,uid);
      Serial.print(F("Polling UID: "));
      Serial.print(uid);
      for (retry_count=0; retry_count < g_sampling_max_retry; retry_count++){
        Serial.print(" .");
        ic = count_frames(g_can_buffer);
        if (get_all_frames(POLL_TIMEOUT, g_can_buffer, 1) > ic ){
          Serial.println(" OK");
          break;
        }
      }
      Serial.println(" ");
    }
  }
  count = count_frames(g_can_buffer);
  // write frames to String or char array
  for (int i = 0;i<count;i++){
    if (g_can_buffer[i].id != 0){
      write_frame_to_dump(g_can_buffer[i],g_temp_dump);
    }
  }
  // add delimiter "+"
  strcat(g_temp_dump,"+");
  strcat(g_temp_dump, '\0');
  Serial.print("g_temp_dump: ");
  Serial.println(g_temp_dump);
  process_g_temp_dump(g_temp_dump,final_dump,g_no_gids_dump);
  g_temp_dump[0] = '\0';

  clear_can_buffer(g_can_buffer);
  Serial.println(F("================================="));
}

/* 
  Function: get_all_frames

    - Receive all the incoming frames.
    
    - Place the frames in a CAN_FRAME array.
    
    - Process the frames in the buffer <process_all_frames>

  Parameters:
  
    timeout_ms - int timeout in milliseconds

    can_buffer[] - CAN_FRAME struct array

  Returns:

    integer count of frames in the buffer

  See Also:

    <get_data>
*/
int get_all_frames(int timeout_ms, CAN_FRAME can_buffer[], int expected_frames) {
  int timestart = millis();
  int a = 0, i = 0;
  CAN_FRAME incoming;
  if (VERBOSE == 1) { Serial.println("get_all_frames()"); }
  do {
      check_can_status();
      if (Can0.available()){
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
        if (i == expected_frames){
          process_all_frames(g_can_buffer);
          i = count_frames(g_can_buffer);
          if (i == expected_frames){
            return i;
            break;
          }
        }
      }
  } while ((millis() - timestart <= timeout_ms)); 
  process_all_frames(g_can_buffer);
  return count_frames(g_can_buffer);                              
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
void process_all_frames(CAN_FRAME can_buffer[]){
  int count,i;
  count = count_frames(can_buffer);
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
int count_frames(CAN_FRAME can_buffer[]){
  int i = 0,count = 0;
  for (i=0; i< CAN_ARRAY_BUFFER_SIZE; i++){
    if (can_buffer[i].id != 0){
        count++;
    }
  }
  return count;
}

/* 
  Function: delete_repeating_frames

    Replaces the values inside the CAN_FRAME struct with 0s
    if an id is not unique within the CAN_FRAME array.
    0s the 2nd instance. The first is retained. 

  Parameters:

    can_buffer - array of the CAN_FRAME struct

  Returns:

    n/a

  See Also:

    <process_all_frames>
*/
void delete_repeating_frames(CAN_FRAME can_buffer[]){
  int i0 = 0, i1 = 0;
  int frame_count,i;
  frame_count = count_frames(can_buffer);
  for (i0 = 0; i0 < frame_count; i0++){
    for (i1 = 0; i1 < frame_count; i1++) {
      if ( (can_buffer[i0].id == can_buffer[i1].id) && (i0 != i1) ){
        can_buffer[i1] = {}; // clears the CAN_FRAME struct
      }
    }
  }
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
int convert_uid_to_gid(int uid){
  int gid = 0;
  if ((uid == 0) | (uid == -1)){
    return -1;
  }
  for (int i=0; i< g_num_of_nodes;i++){
    if (g_gids[i][0] == uid){
      return g_gids[i][1];
    }
  }
  return 0;
}


// Group: Char array based operations
/* 
  Function: write_frame_to_dump

    Convert the frames into one char array.
    id 

  Parameters:

    incoming- CAN_FRAME struct to be written in dump
    dump - pointer to char buffer

  Returns:

    n/a

  See Also:

    <process_g_temp_dump>
*/
void write_frame_to_dump(CAN_FRAME incoming, char* dump){
  char temp[5];
  sprintf(temp,"%04X",incoming.id);
  strcat(dump,temp);

  sprintf(temp,"%02X",incoming.data.byte[0]);
  strcat(dump,temp);

  sprintf(temp,"%02X",incoming.data.byte[1]);
  strcat(dump,temp);

  sprintf(temp,"%02X",incoming.data.byte[2]);
  strcat(dump,temp);

  sprintf(temp,"%02X",incoming.data.byte[3]);
  strcat(dump,temp);

  sprintf(temp,"%02X",incoming.data.byte[4]);
  strcat(dump,temp);

  sprintf(temp,"%02X",incoming.data.byte[5]);
  strcat(dump,temp);

  sprintf(temp,"%02X",incoming.data.byte[6]);
  strcat(dump,temp);

  sprintf(temp,"%02X-",incoming.data.byte[7]);
  strcat(dump,temp);

  return;
}

/* 
  Function: process_g_temp_dump

    Process g_string:

    - Separate the main string by the delimiter "+".

    - Convert the uid ( universal id ) to the gid ( geographic id ).

    - Successful conversion of uid to gid : delimiter "-"

    - Failed conversion of uid to gid : delimiter "="

    - Write in g_final_dump

        Format of data in g_string_proc:
    
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

void process_g_temp_dump(char* dump, char* final_dump, char* no_gids_dump){
  char *token,*last_char;
  char temp_id[5],temp_gid[5],temp_data[17];
  int id_int,gid;
  token = strtok(dump, "-");

  while(token != NULL){
    // get gid
    strncpy(temp_id,token,4);
    id_int = strtol(temp_id,&last_char,16);
    gid = convert_uid_to_gid(id_int);

    if ( (gid != 0) & (gid != -1) ){
      sprintf(temp_gid,"%04X",gid);
      strncpy(temp_data,token+4,16);
      // strncat(final_dump,"-",1);
      strncat(final_dump,temp_gid,4);
      strncat(final_dump,temp_data,16);
      
    } else if (gid == 0) {
      strncat(no_gids_dump,"=",1);
      strncat(no_gids_dump,token,20);
    }
    token = strtok(NULL,"-");
  } 
  strncat(final_dump,"+",1); // add delimiterfor different data type
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

  Parameters:

    incoming - CAN_FRAME struct

  Returns:

    n/a

  See Also:

    <write_frame_to_dump>
*/

void interpret_frame(CAN_FRAME incoming){
  int id,d1,d2,d3,d4,d5,d6,d7,d8,x,y,z;
  char temp[6];
  if (VERBOSE == 1) { Serial.println("process_frame()"); }
  id = incoming.id;
  d1 = incoming.data.byte[0];
  d2 = incoming.data.byte[1];
  d3 = incoming.data.byte[2];
  d4 = incoming.data.byte[3];
  d5 = incoming.data.byte[4];
  d6 = incoming.data.byte[5];
  d7 = incoming.data.byte[6];
  d8 = incoming.data.byte[7]; 
  x = compute_axis(d2,d3);
  y = compute_axis(d4,d5);
  z = compute_axis(d6,d7);
  
  Serial.print("\t");
  Serial.print(id,HEX); Serial.print("\t"); 
  Serial.print(id); Serial.print('\t');
  Serial.print(convert_uid_to_gid(id)); Serial.print('\t');
  sprintf(temp, "%5d", x); Serial.print(temp);
  temp[0] = '\0';
  Serial.print(" "); 
  sprintf(temp, "%5d", y); Serial.print(temp);
  temp[0] = '\0';
  sprintf(temp, "%5d", z);
  Serial.print(" "); Serial.println(temp);
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
int compute_axis(int d1, int d2){
  int value = 5000;
  if (d2 >= 240) {
    d2 = d2 - 240;
    value = (d1 + (d2*256)) - 4095;
  } else {
    value = (d1 + (d2*256));
  }
  return value;
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
void check_can_status(){
  int rx_error_cnt=0,tx_error_cnt =0;
  rx_error_cnt = Can0.get_rx_error_cnt();
  tx_error_cnt = Can0.get_tx_error_cnt();
  if (rx_error_cnt + tx_error_cnt != 0){ 
    Serial.print("rx_error : ");
    Serial.print(rx_error_cnt);
    Serial.print("\t tx_error :");
    Serial.println(tx_error_cnt);
  }
  return;
}

/* 
  Function: send_command

    Populates an outgoing frame with a command and transmit id then broadcasts the frame.

  Parameters:

    command - integer message id ( msgid )

    transmit_id - integer id of frame to be broadcast. ( useful for Version 1 Sensors )

  Returns:

    n/a

  See Also:

    <get_all_frames>
*/
void send_command(int command,int transmit_id){
  if (VERBOSE == 1) { Serial.println("send_frame()"); }
  CAN_FRAME outgoing;
  outgoing.extended = true;
  outgoing.id = transmit_id;
  outgoing.length = 1;
  outgoing.data.byte[0] = command;
  Can0.sendFrame(outgoing);
}

void poll_command(int command,int uid){
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




//Group: String-based functions.
/* 
  Function: write_frame_to_string

    Convert the frames into one String.

  Parameters:

    incoming- CAN_FRAME struct to be written in g_string.

  Returns:

    n/a

  See Also:

    <process_g_string>
*/
void write_frame_to_string(CAN_FRAME incoming){
  char temp[5];

  interpret_frame(incoming);

  sprintf(temp,"%04X",incoming.id);
  g_string = String(g_string +  String(temp));

  sprintf(temp,"%02X",incoming.data.byte[0]);
  g_string = String(g_string +  String(temp));

  sprintf(temp,"%02X",incoming.data.byte[1]);
  g_string = String(g_string +  String(temp));

  sprintf(temp,"%02X",incoming.data.byte[2]);
  g_string = String(g_string +  String(temp));

  sprintf(temp,"%02X",incoming.data.byte[3]);
  g_string = String(g_string +  String(temp));

  sprintf(temp,"%02X",incoming.data.byte[4]);
  g_string = String(g_string +  String(temp));

  sprintf(temp,"%02X",incoming.data.byte[5]);
  g_string = String(g_string +  String(temp));

  sprintf(temp,"%02X",incoming.data.byte[6]);
  g_string = String(g_string +  String(temp));

  sprintf(temp,"%02X-",incoming.data.byte[7]);
  g_string = String(g_string +  String(temp));

  return;
}

/* 
  Function: string_to_int

    Converts a 4 character String(arduino class) to int

  Parameters:

    id_string - 4 character String

  Returns:

    id_int - integer equivalent of id_string when read as hex
    -1 - when id_string length is less than 4 chars

  See Also:

    <process_g_string>
*/
int string_to_int(String id_string){
  char *id_c_add,*last_char;
  char idchar[id_string.length()+1];
  int id_int;
  if ( id_string.length() < 4 ){
    return -1;
  }
  // point id_c_add to idchar
  id_c_add = idchar;       
  // hardcoded 4 char conversion                              
  id_string.toCharArray(idchar,id_string.length()+1);    
  id_int = strtol(id_c_add,&last_char,16);
  return id_int;
}

/* 
  Function: process_g_string

    Process g_string:

    1. Separate the main string by the delimiter "+".
    2. Convert the uid ( universal id ) to the gid ( geographic id ).
    3. Retain uids of nodes with no corresponding gids.
    4. Add a delimiter at the beginning of the String "-".
    5. Write in g_string_proc

    Format of data in g_string_proc:
    
    delimiter - 1 char
    gids - 4 chars
    data - 16 chars

    [delimiter][gids][data]

    or

    [uids][data][delimiter]

  Parameters:

    n/a

  Returns:

    1 - successful operation

  See Also:

    <separate_gids_and_uids>
*/
int process_g_string(){
  int num_of_proc_data = 0;
  int i1=0, i2=0;
  int id_int,gid,ip0,ip1;
  long int uid;
  String id,data,big_string;

  String type_delim = "+";
  
  ip0 = g_string.indexOf(type_delim);
  big_string = g_string.substring(0,ip0);
  // Serial.println(big_string);
  g_string_proc = String(g_string_proc + separate_gids_and_uids(big_string));
  //dito magrearrange ng string na gagamitin

  while (ip0 != -1){
    ip1 = g_string.indexOf(type_delim,ip0+1);
    big_string = g_string.substring(ip0,ip1);
    separate_gids_and_uids(big_string);
    g_string_proc = String(g_string_proc + separate_gids_and_uids(big_string));
    ip0 = ip1;
  }
  return 1;
}

String separate_gids_and_uids(String source_string){
  char temp[5];
  int i1,i2,gid;
  long int uid;
  String delim = "-";
  String id,data,processed_string;

  i1 = source_string.indexOf(delim);
  id = source_string.substring(0,4);
  data = source_string.substring(4,i1);
  uid = string_to_int(id);
  gid = convert_uid_to_gid(uid);
  //Serial.print(id); Serial.print("\t"); Serial.println(gid);

  if (gid == 0){
    processed_string = String(delim + id + data + processed_string); 
  } else if ( (gid != -1) | (gid != 0) ){
    temp[0] = '\0';
    sprintf(temp,"%04X",gid); 
    processed_string = String(processed_string + String(temp) + data + delim); 
  }

  while (i2 != -1){
    i2 = source_string.indexOf(delim,i1+1);
    id = source_string.substring(i1+1,i1+5); // 4 chars for id
    data = source_string.substring(i1+5,i2);
    uid = string_to_int(id);
    gid = convert_uid_to_gid(uid);
    //Serial.print(id); Serial.print("\t"); Serial.println(gid);
    if (gid == 0){
      processed_string = String(delim + id + data + processed_string);  
    } else if (gid == -1){
      break;
    } else {
      temp[0] = '\0';
      sprintf(temp,"%04X",gid); 
      processed_string = String(processed_string + String(temp) + data + delim);  
    }
    i1 = i2;
  }
  return processed_string;
}
