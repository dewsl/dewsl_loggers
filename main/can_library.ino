void init_can(){
  if (VERBOSE == 1) { Serial.println("init_can()"); }
  pinMode(50,OUTPUT); //Due Specific
  if (Can0.begin(40000,50)){ // also resets numBusError
    if(VERBOSE == 1) {Serial.println("Can0 - initialization completed.");}
  }
  Can0.watchFor();     // allow all can traffic through sabi sa quick guide.
  Can0.mailbox_set_mode(0,CAN_MB_RX_MODE); // Set mailbox0 as receiver
  Can0.mailbox_set_id(0, 0, true); //Set mailbox0 receive extendedID formats
  Can0.mailbox_set_accept_mask(0,0,true); //receive everything. // aralin yung mask

  Can0.mailbox_set_mode(1, CAN_MB_TX_MODE); // Set mailbox1 as transmitter
  Can0.mailbox_set_id(1,1, true); // Set mailbox1 transfer ID to 1 extended id
}

void process_frame(CAN_FRAME incoming){
  interpret_frame(incoming);
  // write_frame_to_dump(incoming, g_temp_dump);
  write_frame_to_string(incoming);
}

void process_g_string(){
  g_string = String("");
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
  Serial.print(x); //Serial.print("\t");
  Serial.print("_"); Serial.print(y); //Serial.print("\t");
  Serial.print("_"); Serial.println(z); //Serial.print("\t");
}

// compute X Y or Z values given d1 as lowbyte and d2 as highbyte
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

  sprintf(temp,"%02X\n",incoming.data.byte[7]);
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

