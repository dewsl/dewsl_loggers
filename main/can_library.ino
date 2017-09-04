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

void process_frame(CAN_FRAME incoming){
  interpret_frame(incoming);
  // write_frame_to_dump(incoming, g_temp_dump);
  write_frame_to_string(incoming);
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
  int id_int,gid;
  long int uid;
  String id,data;
  String delim = "-";
   
  i1 = g_string.indexOf(delim);

  id = g_string.substring(0,4);
  data = g_string.substring(4,i1);
  uid = string_to_int(id);
  gid = convert_uid_to_gid(uid);
  // Serial.print(id); Serial.print("\t"); Serial.println(gid);

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
    Serial.print(id); Serial.print("\t"); Serial.println(gid);
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
  // int x,y,z;
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
  Serial.print(x); //Serial.print("\t");
  Serial.print("_"); Serial.print(y); //Serial.print("\t");
  Serial.print("_"); Serial.println(z); //Serial.print("\t");
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

void get_all_frames(int timeout_ms) {
  int timestart = millis();
  CAN_FRAME incoming;
  if (VERBOSE == 1) { Serial.println("check_timeout()"); }
  do {
      check_can_status();
      if (Can0.available()){
        Can0.read(incoming);
        process_frame(incoming);
      }
  } while (millis() - timestart <= timeout_ms) ; 
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

void send_frame(){
  if (VERBOSE == 1) { Serial.println("send_frame()"); }
    // Prepare transmit ID, data and data length in CAN0 mailbox 0
  CAN_FRAME outgoing;
  outgoing.extended = true;
  outgoing.id = 1;
  outgoing.length = 8; //MAX_CAN_FRAME_DATA_LEN;
  outgoing.data.byte[0] = 0x0B;
  outgoing.data.byte[1] = 0x00;
  outgoing.data.byte[2] = 0x00;
  outgoing.data.byte[3] = 0x00;
  outgoing.data.byte[4] = 0x00;
  outgoing.data.byte[5] = 0x00;
  outgoing.data.byte[6] = 0x00;
  outgoing.data.byte[7] = 0x00;
  outgoing.data.byte[8] = 0x00;
  Can0.sendFrame(outgoing);
}

void turn_on_column(){
  digitalWrite(RELAYPIN, HIGH);
  delay(1000);
}

void turn_off_column(){
  digitalWrite(RELAYPIN, LOW);
  delay(1000);
}

