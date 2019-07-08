void getOldData(int mode, char readCode){
  //mode- 1 if ARQ
  //mode 0 if debugger
  //char readCode= 'x';
  commandCAN = 0;
  //readCode= getMode(mode);
  
  switch(readCode){
    case 'A': {   //read all data from old column
      if (SERIAL)  Serial.println("- All");
      digitalWrite(RELAYPIN, HIGH);
      delay(4500);
      getSingleDataOld();
      digitalWrite(RELAYPIN, LOW);
      parsedData = parser(columnData, 6);
//      printData(parsedData, mode, "a");
      break;
    }
    
    case 'B': {   //read axel only
      if (SERIAL)  Serial.println("- Axel only");
      digitalWrite(RELAYPIN, HIGH);
      delay(4500);
      getSingleDataOld();
      digitalWrite(RELAYPIN, LOW);
      parsedData = parser(columnData, 0); //di pa okay!!
//      printData(parsedData, mode, "b");
      break;
    }
    
    case 'C': { //read soms only  
      if (SERIAL)  Serial.println("- Soms only");
      digitalWrite(RELAYPIN, HIGH);
      delay(4500);
      getSingleDataOld();
      digitalWrite(RELAYPIN, LOW);
      parsedData = parser(columnData, 0); //di pa okay!!
//      printData(parsedData, mode, "c");
      break;     
    }
 }
}


void getSingleDataOld(void) {    // For commands that need specific data (either Axel1, Axel2 or SoMS)
  
  int ctrid, k=0;
  char *columnPointer = columnData;
  can_initialize();
  int z=0;
  
  for (ctrid=0,k=0; ctrid<NUMBERNODES; ctrid++)  {               // Set node numbers

    CAN.mailbox_set_id(1, nodearray[ctrid] * 8, false);          // Set MB1 transfer ID
    CAN.mailbox_set_id(0, nodearray[ctrid] * 8, false);          // Set MB0 receive ID
    
    CAN.mailbox_set_datah(1, 0);
    CAN.mailbox_set_datal(1, commandCAN);                        // Set command values
  
    CAN.enable_interrupt(CAN_IER_MB1);
    CAN.enable_interrupt(CAN_IER_MB0);
    CAN.global_send_transfer_cmd(CAN_TCR_MB1);

    check_timeout();                                             // Wait for data
    if (timeout_status == true) {                                // If no data, print timeout for node id
      if (SERIAL)  Serial.print("Timeout Node ");
      if (SERIAL)  Serial.println(ctrid);
      continue;                                                  // Proceed with next node id
    }

    CAN.get_rx_buff(&incoming);                                  // Store receive mailbox data

    sprintf(&columnData[k], "%02X", incoming.id/8);              // Data stored in array
    sprintf(&columnData[k+2], "%02X", incoming.data[0]);         // Assumed received data is 
    sprintf(&columnData[k+4], "%02X", incoming.data[1]);         // NNLLHHLLHHLLHHLLHHLLHHLLHHLLHHHHLL
    sprintf(&columnData[k+6], "%02X", incoming.data[2]);
    sprintf(&columnData[k+8], "%02X", incoming.data[3]);
    sprintf(&columnData[k+10], "%02X", incoming.data[4]);
    sprintf(&columnData[k+12], "%02X", incoming.data[5]);        // Already reversed in columnData array
    sprintf(&columnData[k+14], "%02X", incoming.data[6]);
    sprintf(&columnData[k+16], "%02X", incoming.data[7]);
    for (int m=0; m<18; m++) { 
    if (SERIAL)  Serial.print(columnData[k+m]); 
    }  // Printing for monitoring purposes
    
    k = k + 18;
  } columnData[k] = '\0';                                        // columnData as string

  CAN.reset_all_mailbox();
  CAN.disable();

}
