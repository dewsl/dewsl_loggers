void init_lora()
{
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.println("Arduino LoRa RX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init())
  {
    Serial.println("LoRa radio init failed");
    while (1)
      ;
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ))
  {
    Serial.println("setFrequency failed");
    while (1)
      ;
  }
  Serial.print("Set Freq to: ");
  Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}

int read_lora_data(int start)
{
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  
  if (rf95.waitAvailableTimeout(3000))
  {
    // Should be a message for us now

    if (rf95.recv(buf, &len))
    {
      //digitalWrite(LED, HIGH);
      Serial.print("packet size: ");
      Serial.println(len);
      //RH_RF95::printBuffer("Received: ", buf, len);
      //Serial.print("Got: ");
      if (String((char*)buf).indexOf("end") >= 0){
        return -1;
      }

      else if (String((char*)buf).indexOf("final") >= 0){
        return -2;
      }

      
      int i = 0, c = 0;
      for (i = 0; i < len; ++i)
      {
        received[i] = buf[i];
        buf_total[i + start] = buf[i];

        if (received[i] == 211 && i != 0) {
          //Serial.println();
          c = 0;
        }
        //Serial.print(received[i]);
        //Serial.print(" ");

        c++;
        if (c % 16 == 0) {
          //Serial.println();
          c = 0;
        }
      }
      //received from tx
      //received[i] = (uint8_t)'\0';

      //Serial.println(received);

      //print RSSI values
      tx_RSSI = (rf95.lastRssi(), DEC);
      //Serial.print("RSSI: ");
      //Serial.println(tx_RSSI);

    }
    else
    {
      //Serial.println("Receive failed");
      return 0;
    }
  return len;
  }
  
  else return 0;
}

void send_thru_lora(char* radiopacket){
  uint8_t payload[RH_RF95_MAX_MESSAGE_LEN];
  //int len = sizeof(payload);
  int len = String(radiopacket).length();
  int i=0, j=0;

  Serial.println("Sending to rf95_server");
  // Send a message to rf95_server

  //do not stack
  for(i=0; i<200; i++){
    payload[i] = (uint8_t)'0';
  }

  for(i=0; i<len; i++){
    payload[i] = (uint8_t)radiopacket[i];
  }

  payload[i] = (uint8_t)'\0';
  
  Serial.println((char*)payload);
  //Serial.println(len);

  Serial.println("sending payload!");

  rf95.send(payload, len);
  rf95.waitPacketSent();
  delay(100);  
}
