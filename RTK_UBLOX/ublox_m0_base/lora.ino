void init_lora(){
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.println("LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);  
}

void send_thru_lora(uint8_t* radiopacket, int len){
  
  int i=0, j=0,command = 0;
  // char temp[DATALEN];   //lora
  // uint8_t payload[RH_RF95_MAX_MESSAGE_LEN]; //store the data

  Serial.println("Sending to rf95_server");
  // Send a message to rf95_server


  for(i=0; i<len; i++){
    payload[i] = (uint8_t)radiopacket[i];
    Serial.print(payload[i]);    
  }


  Serial.println();
  Serial.println("sending payload!");
  Serial.print("packet size: ");
  Serial.println(len);
  rf95.send(payload, len);
  rf95.waitPacketSent();
  delay(100);  
}


int read_lora_data()
{  
  //uint8_t len=400;
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  
  if (rf95.waitAvailableTimeout(20000))
  {
    // Should be a message for us now

    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      Serial.print("packet size: ");
      Serial.println(len);
      RH_RF95::printBuffer("Received: ", buf, len);
      int i = 0,c=0;
      for (i = 0; i < len; ++i){
        received[i] = buf[i];
      }
      //received from tx
      received[i] = (uint8_t)'\0';
      
      //Serial.println(received);

      //print RSSI values
      tx_RSSI = (rf95.lastRssi(), DEC);
      Serial.print("RSSI: ");
      Serial.println(tx_RSSI);
      
    }
    else
    {
      Serial.println("Receive failed");
      return 0;
    }
    return len;
  }
  else return 0;
}
