void init_can(){
  if (VERBOSE == 1) { Serial.println("init_can()"); }
  Can0.begin(40000);
  Can1.begin(40000);

  // int filter;
  // //extended
  // for (filter = 0; filter < 3; filter++) {
  //   Can0.setRXFilter(filter, 0, 0, true);
  //   Can1.setRXFilter(filter, 0, 0, true);
  // }  
  // //standard
  // for (int filter = 3; filter < 7; filter++) {
  //   Can0.setRXFilter(filter, 0, 0, false);
  //   Can1.setRXFilter(filter, 0, 0, false);
  // }  

  Can0.watchFor();     // allow all can traffic through sabi sa quick guide.
}

void process_frame(CAN_FRAME incoming){
  int id,d1,d2,d3,d4,d5,d6,d7,d8,priority;
  if (VERBOSE == 1) { Serial.println("process_frame()"); }
//  Can0.read(incoming);
  id = incoming.id;
  d1 = incoming.data.byte[0];
  d2 = incoming.data.byte[1];
  d3 = incoming.data.byte[2];
  d4 = incoming.data.byte[3];
  d5 = incoming.data.byte[4];
  d6 = incoming.data.byte[5];
  d7 = incoming.data.byte[6];
  d8 = incoming.data.byte[7];
  Serial.print("id:");
  Serial.print(id, HEX); Serial.print(" ");
  Serial.print(priority, HEX); Serial.print("\t");
  Serial.print(d1, HEX); Serial.print("_");
  Serial.print(d2, HEX); Serial.print("_");
  Serial.print(d3, HEX); Serial.print("_");
  Serial.print(d4, HEX); Serial.print("_");
  Serial.print(d5, HEX); Serial.print("_");
  Serial.print(d6, HEX); Serial.print("_");
  Serial.print(d7, HEX); Serial.print("_");
  Serial.println(d8, HEX);

}

bool get_all_frames(int timeout_ms) {
  // bool timeout_status = false;
  int timestart = millis();
  CAN_FRAME incoming;
  if (VERBOSE == 1) { Serial.println("check_timeout()"); }

  do {
    if (Can0.rx_avail()){
      Can0.read(incoming);
      process_frame(incoming);
    }
  } while (millis() - timestart <= timeout_ms) ; 
  // timeout_status = true;
  return 1;                              
} 


void send_frame(){
  if (VERBOSE == 1) { Serial.println("send_frame()"); }
    // Prepare transmit ID, data and data length in CAN0 mailbox 0
  CAN_FRAME outgoing;
  outgoing.id = 1;
  outgoing.extended = true;
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

