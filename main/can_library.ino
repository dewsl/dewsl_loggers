/* 
  Function: init_can
    Assert the enable pin for can transceiver (pin 50).
    Set Can0 to allow all can trafic
    Set mailbox0 as receiver
    Set mailbox0 to receive extendedID formats
    Set mailbox1 as transmitter
    Set mailbox1 transferID to 1 (extendedID)
  Parameters:
    n/a
  Returns:
    n/a
  See Also:
    <main.ino/setup>
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
    <main.ino>
*/
void init_strings(){
  g_string = "";
  g_string_proc = ""; 
}

void get_data(int cmd, int transmit_id){
  int retry_count = 0,respondents = 0;
  for (retry_count = 0; retry_count < g_sampling_max_retry; retry_count++){
    turn_on_column();
    send_command(cmd,transmit_id);
    if( (respondents = get_all_frames(TIMEOUT)) == g_num_of_nodes){
      // indicate na kumpleto na yung frames
      turn_off_column();
      break;
    } else {
      Serial.print(respondents); Serial.print(" / ");
      Serial.print(g_num_of_nodes); Serial.println(" received / expected frames.");
    }
    turn_off_column();
  }
  Serial.println(F("================================="));
}

int get_all_frames(int timeout_ms) {
  int timestart = millis();
  int i = 0, a = 0;
  CAN_FRAME incoming;
  if (VERBOSE == 1) { Serial.println("get_all_frames()"); }
  do {
      check_can_status();
      if (Can0.available()){
        Can0.read(incoming);
        g_can_buffer[i].id = incoming.id;
        g_can_buffer[i].data.byte[0] = incoming.data.byte[0];
        g_can_buffer[i].data.byte[1] = incoming.data.byte[1];
        g_can_buffer[i].data.byte[2] = incoming.data.byte[2];
        g_can_buffer[i].data.byte[3] = incoming.data.byte[3];
        g_can_buffer[i].data.byte[4] = incoming.data.byte[4];
        g_can_buffer[i].data.byte[5] = incoming.data.byte[5];
        g_can_buffer[i].data.byte[6] = incoming.data.byte[6];
        g_can_buffer[i].data.byte[7] = incoming.data.byte[7];
        i++;
      }
  } while (millis() - timestart <= timeout_ms) ; 

  // dapat sa process_each_frame ito pero testing lang
  delete_repeating_frames(g_can_buffer);

  for (a=0 ;a < i;a++){
    process_each_frame(g_can_buffer[a]); 
  }
  
  return count_frames(g_can_buffer);                              
} 

void process_each_frame(CAN_FRAME incoming){ // dapat magreturn ito ng number of processed frames
  // lagyan ng filter sa pagidentify ng unique frames
  write_frame_to_string(incoming);
}

int count_frames(CAN_FRAME can_buffer[]){
  int i = 0,count = 0;
  for (i=0; i< CAN_ARRAY_BUFFER_SIZE; i++){
    if (can_buffer[i].id != 0){
        count++;
    }
  }
  return count;
}

void delete_repeating_frames(CAN_FRAME can_buffer[]){
  int i0 = 0, i1 = 0;
  int frame_count,i;
  frame_count = count_frames(can_buffer);

  for (i0 = 0; i0 < frame_count; i0++){
    for (i1 = 0; i1 < frame_count; i1++) {
      if ( (can_buffer[i0].id == can_buffer[i1].id) && (i0 != i1) ){
        can_buffer[i1].id = 0;
        for (i = 0; i < 8; i++){
          can_buffer[i1].data.byte[i] = 0;
        }
      }
    }
  }
}

/* 
  Function: string_to_int
    Converts a 4 character String(arduino class) to int
  Parameters:
    String id_string - 4 character String
  Returns:
    int id_int - integer equivalent of id_string when read as hex
  See Also:
    <process_g_string>
*/
int string_to_int(String id_string){
  char *id_c_add,*last_char;
  char idchar[id_string.length()+1];
  int id_int;
  id_c_add = idchar;                                     // point id_c_add to idchar
  id_string.toCharArray(idchar,id_string.length()+1);    // hardcoded 4 char conversion
  id_int = strtol(id_c_add,&last_char,16);
  // Serial.println(id_int);
  return id_int;
}

/* 
  Function: convert_uid_to_gid
    Converts the uid (universal id) to the gid (geographic id)
  Parameters:
    int uid - universal id
  Returns:
    int gid - geographic id of given universal id according sd card config
  See Also:
    <process_g_string/init_gids>
*/
int convert_uid_to_gid(int uid){
  int gid = 0;
  if (uid == 0){
    return 0;
  }
  for (int i=0; i< g_num_of_nodes;i++){
    if (g_gids[i][0] == uid){
      return g_gids[i][1];
    }
  }
  return -1;
}

int process_g_string(){
  char temp[5];
  int num_of_proc_data = 0;
  int i1=0, i2=0;
  int id_int,gid,ip;
  long int uid;
  String id,data;
  String delim = "-";
  String type_delim = "+";
  
  ip = g_string.indexOf(type_delim);

  i1 = g_string.indexOf(delim);

  id = g_string.substring(0,4);
  data = g_string.substring(4,i1);
  uid = string_to_int(id);
  gid = convert_uid_to_gid(uid);
  //Serial.print(id); Serial.print("\t"); Serial.println(gid);

  if (gid == -1){
    g_string_proc = String(g_string_proc + delim + id + data); 
  } else {
    temp[0] = '\0';
    sprintf(temp,"%04X",gid); 
    g_string_proc = String(String(temp) + data + delim + g_string_proc); 
  }
  
  while (i2 != -1){
    i2 = g_string.indexOf(delim,i1+1);
    id = g_string.substring(i1+1,i1+5); // 4 chars for id
    data = g_string.substring(i1+5,i2);
    uid = string_to_int(id);
    gid = convert_uid_to_gid(uid);
    //Serial.print(id); Serial.print("\t"); Serial.println(gid);
    if (gid == -1){
      g_string_proc = String(g_string_proc + delim + id + data); 
    } else if (gid == 0){
      break;
    } else {
      temp[0] = '\0';
      sprintf(temp,"%04X",gid); 
      g_string_proc = String(String(temp) + data + delim + g_string_proc); 
    }
    i1 = i2;
  }
  return 1;
}

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

void write_frame_to_dump(CAN_FRAME incoming, char *dump){
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

  sprintf(temp,"%02X\n",incoming.data.byte[7]);
  strcat(dump,temp);


  return;
}

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

void send_command(int command,int transmit_id){
  if (VERBOSE == 1) { Serial.println("send_frame()"); }
  CAN_FRAME outgoing;
  outgoing.extended = true;
  outgoing.id = transmit_id;
  outgoing.length = 1;
  outgoing.data.byte[0] = command;
  Can0.sendFrame(outgoing);
}

void send_frame(){
  if (VERBOSE == 1) { Serial.println("send_frame()"); }
    // Prepare transmit ID, data and data length in CAN0 mailbox 0
  CAN_FRAME outgoing;
  outgoing.extended = true;
  outgoing.id = 1;
  outgoing.length = 1; //MAX_CAN_FRAME_DATA_LEN;
  outgoing.data.byte[0] = 0x0B;
  Can0.sendFrame(outgoing);
}

void turn_on_column(){
  digitalWrite(RELAYPIN, HIGH);
  delay(g_turn_on_delay);
}

void turn_off_column(){
  digitalWrite(RELAYPIN, LOW);
  delay(1000);
}

