void init_lora() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay_millis(10);
  digitalWrite(RFM95_RST, HIGH);
  delay_millis(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1)
      ;
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1)
      ;
  }
  Serial.print("Set Freq to: ");
  Serial.println(RF95_FREQ);
  /*
  * Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  * The default transmitter power is 13dBm, using PA_BOOST.
  * If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  * you can set transmitter powers from 5 to 23 dBm:
  */
  rf95.setTxPower(23, false);
}

void send_thru_lora(char *radiopacket) {
  rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128);
  char ack_from_gateway[13];
  ack_from_gateway[0]=0x00;
  if (debug_flag == 0) { //reset watchdog before resuming
    Watchdog.reset();
  }

  int length = sizeof(payload);
  int i = 0, j = 0;
  bool ack_wait = true;
  bool exit_retry = false;
  int LORA_SEND_RETRY_LIMIT = 3;


  for (i = 0; i < 200; i++) {
    payload[i] = (uint8_t)'0';
  }
  for (i = 0; i < length; i++) {
    payload[i] = (uint8_t)radiopacket[i];
  }
  payload[i] = (uint8_t)'\0';

  if (logger_ack_filter_enabled()) {
    for (j = 0; j <= LORA_SEND_RETRY_LIMIT; j++) {
      if (strstr(radiopacket, "*VOLT")) {
        LORA_SEND_RETRY_LIMIT = 0;      //temporary: remove retry for *VOLT messages to prevent cutoffs
      } else {
        LORA_SEND_RETRY_LIMIT = 3;
      }
      if (debug_flag == 0) { //reset watchdog before resuming
        Watchdog.reset();
      }
      Serial.print("Sending to LoRa: ");
      Serial.println((char *)payload);
      // Serial.println("sending payload!");
      rf95.send(payload, length);  // sending data to LoRa
      rf95.waitPacketSent();       // Waiting for packet to complete...

      unsigned long ack_wait_start = millis();
      ack_payload[0] = '\0';
      do {
        if (rf95.waitAvailableTimeout(ACKWAIT)) {
          if (rf95.recv(ack_payload, &len2)) {
            // Serial.print("ack payload: ");
            // Serial.println((char *)ack_payload);
            strncat(ack_from_gateway, (char *)ack_payload, 12);
            ack_from_gateway[13]=0x00;
            if (strstr(ack_from_gateway, ack_key)) {
              key_gen(get_logger_A_from_flashMem());  //generate own key [passed to the variable ack_msg] to be matched with gateway ack_msg
              // Serial.println(strcmp(ack_from_gateway,ack_msg));
              if (strcmp(ack_from_gateway, ack_msg) == 0) {
                flashLed(LED_BUILTIN, 2, 30);
                Serial.println("Received ack_key from gateway");
                // Serial.println(ack_from_gateway);
                Serial.print("RSSI: ");
                Serial.println(rf95.lastRssi(), DEC);
                exit_retry = true;
                delay_millis(random(1000, 2000));
                break;
              }
            } else {
              // Serial.print("Received reply: ");
              // Serial.println((char *)ack_payload);
            }
          }
        }
        // else {
        //   Serial.println("No valid response...");
        // }
      } while ((millis() - ack_wait_start) < ACKWAIT);
      if (exit_retry) {
        exit_retry = false;
        break;
      }
      Serial.println("Retrying...");
    }
  } else {
    // do not stack
    if (debug_flag == 0) { //reset watchdog before resuming
      Watchdog.reset();
    }
    Serial.print("Sending to LoRa [no ack]: ");
    flashLed(LED_BUILTIN, 2, 30);
    Serial.println((char *)payload);
    // Serial.println("sending payload!");
    rf95.send(payload, length);  // sending data to LoRa
    delay_millis(100);
  }
  delay_millis(1000);
  if (debug_flag == 0) { //reset watchdog before resuming
    Watchdog.reset();
  }
}

// bool ack_LoRa_reply(char *LoRa_reply)
// {
//     char temp_reply[50];
//     char _ack[8] = "^REC'D_";

//     strncpy(temp_reply, _ack, sizeof(_ack));
//     strncat(temp_reply, get_logger_B_from_flashMem(), sizeof(get_logger_B_from_flashMem()));
//     Serial.println(F(temp_reply));

//     if (strstr(LoRa_reply, temp_reply))
//     {
//         Serial.println(F("Valid ACK Received!"));
//         return true;
//     }
//     else
//     {
//         Serial.println(F("Invalid ACK!"));
//         return false;
//     }
// }

void key_gen(char *_reference_string) {

  ack_msg[0] = '\0';
  // if (get_logger_mode() == 2) {
  //   strncat(ack_msg, get_logger_A_from_flashMem(), 5);   //gets 5 charaters from saved logger name
  //   ack_msg[strlen(ack_msg)+1] = 0x00;
  //   strcat(ack_msg, ack_key);
  //   ack_msg[13] = '\0';
  // } else {
  //   strncat(ack_msg, _reference_string, 5);             //gets first 5 charaters from payload to compare
  //   ack_msg[strlen(ack_msg)+1] = 0x00;
  //   strcat(ack_msg, ack_key);
  //   ack_msg[13] = '\0';
  // }
  if ((get_logger_mode() == 2) || (get_logger_mode() == 8)) {
    //  gets first charaters (depending on router char length) from reference to generate acknowledgement key
    strncat(ack_msg, _reference_string, (strlen(loggerName.sensorA)));             
  } else {
    //  gets first charaters (depending on router char length) from reference to generate acknowledgement key
    strncat(ack_msg, _reference_string, (strlen(loggerName.sensorB)));             
  }    
  ack_msg[strlen(ack_msg)+1] = 0x00;
  strcat(ack_msg, ack_key);
  ack_msg[13] = '\0';
  // Serial.print("generated key: ");
  // Serial.println(ack_msg);                    
}

// bool check_loRa_ack(char *_received) {
//   bool lora_tx_flag = false;  // will retry sending by default unless valid ack_msg is received
//   if (strstr(_received, ack_msg)) {
//     Serial.print("Ack valid ");
//     // Serial.println((char *)_received);
//     lora_tx_flag = true;
//   }
//   return lora_tx_flag;
// }

void receive_lora_data(uint8_t mode) {
  rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128);
  lora_TX_end = 0;
  rcv_LoRa_flag = 0;

  int count = 0;
  int count2 = 0;
  // sending_stack[0] = '\0';
  unsigned long start = millis();
  Serial.println("waiting for LoRa data . . ");
  while (rcv_LoRa_flag == 0) {
    if (debug_flag == 0) {
      Watchdog.reset();
    }
    if (rf95.available()) {
      // Should be a message for us now
      if (rf95.recv(buf, &len2)) {
        int i = 0;
        for (i = 0; i < len2; ++i) {
          received[i] = (uint8_t)buf[i];
        }
        received[i] = (uint8_t)'\0';

        if (strstr(received, ">>")) { /*NOT LoRa: 0, 2, 7*/
          flashLed(LED_BUILTIN, 3, 60);
          if (mode == 1 || mode == 3 || mode == 4 || mode == 5) {  // LoRa mode only
            /*remove 1st & 2nd character*/
            for (byte i = 0; i < strlen(received); i++) {
              received[i] = received[i + 2];
            }
            if (logger_ack_filter_enabled()) {
              if (if_receive_valid(received)) {
                if (check_duplicates_in_stack((char *)received)) {
                  aggregate_received_data(received);
                  // tx_RSSI = String(rf95.lastRssi(), DEC);
                  // Serial.print("RSSI: ");
                  // Serial.println(tx_RSSI);
                }
              } else {
                if (allow_unlisted()) {
                  if (check_duplicates_in_stack((char *)received)) {
                    Serial.print("Adding data from unlisted transmitter to sending stack: ");
                    aggregate_received_data(received);
                    // tx_RSSI = String(rf95.lastRssi(), DEC);
                    // Serial.print("RSSI: ");
                    // Serial.println(tx_RSSI);
                  }
                } else {
                  Serial.print("Ignoring data: ");
                  Serial.println(received);
                }
              }
            } else {
              // No filtering required here, add everything to sending stack
              Serial.print("Received data: ");
              Serial.println(received);
              if (check_duplicates_in_stack((char *)received)) {
                aggregate_received_data(received);
                // tx_RSSI = String(rf95.lastRssi(), DEC);
                // Serial.print("RSSI: ");
                // Serial.println(tx_RSSI);
              }
            }
          } else {
            // for modes not listed yet...
            Serial.print("Received Data: ");
            Serial.println(received);
            aggregate_received_data(received);
            // print RSSI values
            tx_RSSI = String(rf95.lastRssi(), DEC);
            Serial.print("RSSI: ");
            Serial.println(tx_RSSI);
          }
        } else if (strstr(received, "*VOLT:")) {
          // Serial.print("received raw: ");
          // Serial.println(received);
          if (if_receive_valid(received)) {
            lora_TX_end++;
            if (mode == 4) {  // 2 LoRa transmitter
              count2++;
              Serial.print("recieved counter: ");
              Serial.println(count2);
              if (count2 == 1) {
                // SENSOR A
                tx_RSSI = String(rf95.lastRssi(), DEC);
                Serial.print("RSSI: ");
                Serial.println(tx_RSSI);
                //  parse voltage, MADTB*VOLT:12.33*200214111000
                parse_voltage(received).toCharArray(txVoltage, sizeof(txVoltage));
                Serial.print("TX Voltage A: ");
                Serial.println(txVoltage);
              } else if (count2 == 2) {
                // SENSOR B
                tx_RSSI_B = String(rf95.lastRssi(), DEC);
                Serial.print("RSSI: ");
                Serial.println(tx_RSSI_B);
                parse_voltage(received).toCharArray(txVoltageB, sizeof(txVoltageB));
                Serial.print("TX Voltage B: ");
                Serial.println(txVoltageB);
                delay_millis(500);
                get_rssi(get_logger_mode());
                count2 = 0;
                // rcv_LoRa_flag = 1;
              }
            } else if (mode == 5) {  // 3 LoRa transmitter
              count2++;
              Serial.print("counter: ");
              Serial.println(count2);
              if (count2 == 1) {
                // SENSOR A
                tx_RSSI = String(rf95.lastRssi(), DEC);
                Serial.print("RSSI: ");
                Serial.println(tx_RSSI);
                // parse voltage, MADTB*VOLT:12.33*200214111000
                parse_voltage(received).toCharArray(txVoltage, sizeof(txVoltage));
                Serial.print("TX Voltage A: ");
                Serial.println(txVoltage);
              } else if (count2 == 2) {
                // SENSOR B
                tx_RSSI_B = String(rf95.lastRssi(), DEC);
                Serial.print("RSSI: ");
                Serial.println(tx_RSSI);
                //  parse voltage, MADTB*VOLT:12.33*200214111000
                parse_voltage(received).toCharArray(txVoltageB, sizeof(txVoltageB));
                Serial.print("TX Voltage B: ");
                Serial.println(txVoltageB);
              } else if (count2 == 3) {
                // SENSOR C
                tx_RSSI_C = String(rf95.lastRssi(), DEC);
                Serial.print("RSSI: ");
                Serial.println(tx_RSSI_B);
                parse_voltage(received).toCharArray(txVoltageC, sizeof(txVoltageC));
                Serial.print("TX Voltage C: ");
                Serial.println(txVoltageC);
                get_rssi(get_logger_mode());
                count2 = 0;
                // rcv_LoRa_flag = 1;
              }
            } else {
              /*only 1 transmitter*/
              tx_RSSI = String(rf95.lastRssi(), DEC);
              Serial.print("RSSI: ");
              Serial.println(tx_RSSI);
              //  parse voltage, MADTB*VOLT:12.33*200214111000
              parse_voltage(received).toCharArray(txVoltage, sizeof(txVoltage));
              Serial.print("Received Voltage: ");
              Serial.println(txVoltage);
              get_rssi(get_logger_mode());
              // rcv_LoRa_flag = 1;
            }
          } else {
            // If transmitter is unlisted with voltage data
            Serial.print("Unlisted transmitter voltage data: ");
            Serial.println(received);
            if (allow_unlisted()) {
              if (check_duplicates_in_stack((char *)received)) {
                lora_TX_end++;
                if (mode == 4) {  // 2 LoRa transmitter
                  count2++;
                  Serial.print("recieved counter: ");
                  Serial.println(count2);
                  if (count2 == 1) {
                    // SENSOR A
                    tx_RSSI = String(rf95.lastRssi(), DEC);
                    Serial.print("RSSI: ");
                    Serial.println(tx_RSSI);  
                    //  parse voltage, MADTB*VOLT:12.33*200214111000
                    parse_voltage(received).toCharArray(txVoltage, sizeof(txVoltage));
                    Serial.print("TX Voltage A: ");
                    Serial.println(txVoltage);
                  } else if (count2 == 2) {
                    // SENSOR B
                    tx_RSSI_B = String(rf95.lastRssi(), DEC);
                    Serial.print("RSSI: ");
                    Serial.println(tx_RSSI_B);
                    parse_voltage(received).toCharArray(txVoltageB, sizeof(txVoltageB));
                    Serial.print("TX Voltage B: ");
                    Serial.println(txVoltageB);
                    delay_millis(500);
                    get_rssi(get_logger_mode());
                    count2 = 0;
                    // rcv_LoRa_flag = 1;
                  }
                } else if (mode == 5) {  // 3 LoRa transmitter
                  count2++;
                  Serial.print("counter: ");
                  Serial.println(count2);
                  if (count2 == 1) {
                    // SENSOR A
                    tx_RSSI = String(rf95.lastRssi(), DEC);
                    Serial.print("RSSI: ");
                    Serial.println(tx_RSSI);
                    // parse voltage, MADTB*VOLT:12.33*200214111000
                    parse_voltage(received).toCharArray(txVoltage, sizeof(txVoltage));
                    Serial.print("TX Voltage A: ");
                    Serial.println(txVoltage);
                  } else if (count2 == 2) {
                    // SENSOR B
                    tx_RSSI_B = String(rf95.lastRssi(), DEC);
                    Serial.print("RSSI: ");
                    Serial.println(tx_RSSI);
                    //  parse voltage, MADTB*VOLT:12.33*200214111000
                    parse_voltage(received).toCharArray(txVoltageB, sizeof(txVoltageB));
                    Serial.print("TX Voltage B: ");
                    Serial.println(txVoltageB);
                  } else if (count2 == 3) {
                    // SENSOR C
                    tx_RSSI_C = String(rf95.lastRssi(), DEC);
                    Serial.print("RSSI: ");
                    Serial.println(tx_RSSI_B);
                    parse_voltage(received).toCharArray(txVoltageC, sizeof(txVoltageC));
                    Serial.print("TX Voltage C: ");
                    Serial.println(txVoltageC);
                    get_rssi(get_logger_mode());
                    count2 = 0;
                    // rcv_LoRa_flag = 1;
                  }
                } else {
                  /*only 1 transmitter*/
                  tx_RSSI = String(rf95.lastRssi(), DEC);
                  Serial.print("RSSI: ");
                  Serial.println(tx_RSSI);
                  //  parse voltage, MADTB*VOLT:12.33*200214111000
                  parse_voltage(received).toCharArray(txVoltage, sizeof(txVoltage));
                  Serial.print("Received Voltage: ");
                  Serial.println(txVoltage);
                  get_rssi(get_logger_mode());
                  // rcv_LoRa_flag = 1;
                }
              }
            }
          }


        } else {
          // valid transmitter with unknown data format
          if (if_receive_valid(received)) {
            if (!strstr(received, ack_key)) {
              Serial.print("Unrecognized data format :");
              Serial.println(received);
              aggregate_received_data(received);
              Serial.print("RSSI: ");
              Serial.println(tx_RSSI);
            }
          } else {
            if (!strstr(received, ack_key)) {
              Serial.print("Unrecognized data format from unlisted transmitter: ");
              Serial.println(received);
              if (allow_unlisted()) {
                if (check_duplicates_in_stack((char *)received)) {
                  aggregate_received_data(received);
                  Serial.print("RSSI: ");
                  Serial.println(tx_RSSI);
                }
              }
            }
          }
        }
      }
    }

    if ((millis() - start) > LORATIMEOUTWITHACK) {
      if (debug_flag == 0) {
        Watchdog.reset();
      }
      start = millis();
      // send gateway rssi values if nothing received from transmitter
      get_rssi(get_logger_mode());
      rcv_LoRa_flag = 1;
    }
    if (mode == 4) {
      if (lora_TX_end == 2) {
        rcv_LoRa_flag = 1;
      }
    } else if (mode == 5) {
      if (lora_TX_end == 3) {
        rcv_LoRa_flag = 1;
      }
    } else {
      if (lora_TX_end == 1) {
        rcv_LoRa_flag = 1;
      }
    }
    if (debug_flag == 0) {
      Watchdog.reset();
    }
  }  // while (rcv_LoRa_flag == 0); //if NOT same with condition Loop will exit

  //Split and send aggregated data [sending_stack] here

  if (mode != 1) {
    send_message_segments(sending_stack);
  }
  // display_freeram();

  count = 0;
  count2 = 0;
  rcv_LoRa_flag = 0;
  getSensorDataFlag = false;
  txVoltage[0] = '\0';
  txVoltageB[0] = '\0';
  txVoltageC[0] = '\0';
  flashLed(LED_BUILTIN, 3, 80);
  if (debug_flag == 0) {
    Watchdog.reset();
  }
}

bool if_receive_valid(char *_received) {

  // Check if received payload is from valid transmitter
  // if transmission is valid, an acknowledgement is broadcasted
  bool valid_LoRa_tx = false;
  uint8_t _tx_name = 0;
  char _Blog[6];
  char _Clog[6];
  char _Dlog[6];

  // _Blog[0] = 0x00;
  // _Clog[0] = 0x00;
  // _Dlog[0] = 0x00;

  // strncat(ack_msg, get_logger_B_from_flashMem(), strlen(get_logger_B_from_flashMem()));
  // strncat(ack_msg, get_logger_C_from_flashMem(), strlen(get_logger_C_from_flashMem()));
  // strncat(ack_msg, get_logger_D_from_flashMem(), strlen(get_logger_D_from_flashMem()));
  
  // _Blog[strlen(_Blog)+1] = 0x00;
  // _Clog[strlen(_Clog)+1] = 0x00;
  // _Dlog[strlen(_Dlog)+1] = 0x00;

  char print_buffer[250];
  print_buffer[0] = '\0';

  key_gen(_received);

  if (strstr(ack_msg, get_logger_B_from_flashMem())) {

    valid_LoRa_tx = true;
    // sprintf(print_buffer, "Received from %s: %s", _Blog, _received);
    Serial.print("Received from ");
    Serial.print(get_logger_B_from_flashMem());
    Serial.print(" : ");
    Serial.println(_received);
    // Serial.println(print_buffer);
    // print_buffer[0] = '\0';
    if (logger_ack_filter_enabled()) {
      rf95.send((uint8_t *)ack_msg, strlen(ack_msg));
      rf95.waitPacketSent();
      Serial.println("acknowledgement sent!");
    }

  } else if (strstr(ack_msg, get_logger_C_from_flashMem())) {

    valid_LoRa_tx = true;
    // sprintf(print_buffer, "Received from %s: %s", _Blog, _received);
    Serial.print("Received from ");
    Serial.print(get_logger_C_from_flashMem());
    Serial.print(" : ");
    Serial.println(_received);
    // Serial.println(print_buffer);
    // print_buffer[0] = '\0';
    if (logger_ack_filter_enabled()) {
      rf95.send((uint8_t *)ack_msg, strlen(ack_msg));
      rf95.waitPacketSent();
    }

  } else if (strstr(ack_msg, get_logger_D_from_flashMem())) {

    valid_LoRa_tx = true;
    // sprintf(print_buffer, "Received from %s: %s", _Clog, _received);
    Serial.print("Received from ");
    Serial.print(get_logger_D_from_flashMem());
    Serial.print(" : ");
    Serial.println(_received);
    // Serial.println(print_buffer);
    // print_buffer[0] = '\0';
    if (logger_ack_filter_enabled()) {
      rf95.send((uint8_t *)ack_msg, strlen(ack_msg));
      rf95.waitPacketSent();
    }

    // } else if (strstr(ack_msg, _Elog) && get_logger_mode() == 7) {

    //   valid_LoRa_tx = true;
    //   // sprintf(print_buffer, "Received from %s: %s", _Elog, _received);
    //   Serial.print("Received from router 4");
    //   Serial.print(_Elog);
    //   Serial.print(" : ");
    //   Serial.println(_received);
    //   // Serial.println(print_buffer);
    //   // print_buffer[0] = '\0';
    //   if (logger_ack_filter_enabled()) {
    //     rf95.send((uint8_t *)ack_msg, strlen(ack_msg));
    //     rf95.waitPacketSent();
    //   }

    // } else if (strstr(ack_msg, _Flog) && get_logger_mode() == 8) {
    //   valid_LoRa_tx = true;
    //   // sprintf(print_buffer, "Received from %s: %s", _Flog, _received);
    //   Serial.print("Received from router 5");
    //   Serial.print(_Flog);
    //   Serial.print(" : ");
    //   Serial.println(_received);
    //   // Serial.println(print_buffer);
    //   // print_buffer[0] = '\0';
    //   if (logger_ack_filter_enabled()) {
    //     rf95.send((uint8_t *)ack_msg, strlen(ack_msg));
    //     rf95.waitPacketSent();
    //   }

  } else {
    valid_LoRa_tx = false;
    Serial.println("Unlisted transmitter");
  }

  return valid_LoRa_tx;
}


void receive_lora_data_UBLOX(uint8_t mode) {

  if (debug_flag == 0) {
    Watchdog.reset();
  }
  
  int count = 0;
  int count2 = 0;
  unsigned long start = millis();
  Serial.println("waiting for LoRa data . . .");
  while (rcv_LoRa_flag == 0) {
    uint8_t buf2[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len3 = sizeof(buf2);
    if (mode == 4) {
      // timeOut in case walang makuhang data LoRa transmitter ~15 minutes
      if ((millis() - start) > LORATIMEOUTMODE2) {
        start = millis();
        // send gateway rssi values if no received from transmitter
        get_rssi(get_logger_mode());
        rcv_LoRa_flag = 1;
      }
    } else if (mode == 5) {
      // timeOut in case walang makuhang data LoRa transmitter ~15 minutes
      if ((millis() - start) > LORATIMEOUTMODE3) {
        start = millis();
        // send gateway rssi values if no received from transmitter
        get_rssi(get_logger_mode());
        rcv_LoRa_flag = 1;
      }
    } else if (mode == 13) {
      // timeOut in case walang makuhang data LoRa transmitter ~4 minutes 260 000
      if ((millis() - start) > LORATIMEOUTMODE2) {
        Serial.println("Time out reached for Surficial Tilt Gateway");
        start = millis();
        send_thru_gsm("Time out . . .", get_serverNum_from_flashMem());
        rcv_LoRa_flag = 1;
      }
    } else {
      // timeOut in case walang makuhang data LoRa transmitter ~4 minutes 260 000
      if ((millis() - start) > LORATIMEOUT) {
        Serial.println("Time out reached.");
        start = millis();
        get_rssi(get_logger_mode());
        rcv_LoRa_flag = 1;
      }
    }

    if (rf95.available()) {
      // Should be a message for us now
      if (rf95.recv(buf2, &len3)) {
        int i = 0;
        for (i = 0; i < len3; ++i) {
          received[i] = (uint8_t)buf2[i];
        }
        received[i] = (uint8_t)'\0';

        if (strstr(received, ">>")) { /*NOT LoRa: 0, 2, 6, 7*/
          flashLed(LED_BUILTIN, 3, 60);
          if (mode == 1 || mode == 3 || mode == 4 || mode == 5 || mode == 13 || mode == 15)  // LoRa mode only
          {
            /*remove 1st & 2nd character*/
            for (byte i = 0; i < strlen(received); i++) {
              received[i] = received[i + 2];
            }
            // surficial tilt logger
            if (mode == 15) {
              Serial.print("Surficial Tilt Received: ");
              Serial.println(received);


              if (if_receive_valid(received)) {
                // send_LoRa_reply(if_receive_valid(received));

                send_thru_gsm(received, get_serverNum_from_flashMem());
              } else {
                Serial.println("InValid transmitter!");
              }
            } else {
              send_thru_gsm(received, get_serverNum_from_flashMem());

              // print RSSI values
              tx_RSSI = String(rf95.lastRssi(), DEC);
              Serial.print("RSSI: ");
              Serial.println(tx_RSSI);
            }
          } else {
            Serial.print("Received Data: ");
            Serial.println(received);
            // print RSSI values
            tx_RSSI = String(rf95.lastRssi(), DEC);
            Serial.print("RSSI: ");
            Serial.println(tx_RSSI);
          }
        } else {
          Serial.println("Invalid LoRa message!");
          // rcv_LoRa_flag = 1;
        }
      }
    }
  }

  count = 0;
  count2 = 0;
  rcv_LoRa_flag = 0;
  getSensorDataFlag = false;
  txVoltage[0] = '\0';
  txVoltageB[0] = '\0';
  txVoltageC[0] = '\0';
  flashLed(LED_BUILTIN, 3, 80);
  if (debug_flag == 0) {
    Watchdog.reset();
  }
}

/**
 * @brief LoRa recieve only
 *
 * @param mode
 */

void receive_lora_data_ONLY(uint8_t mode) {
  if (debug_flag == 0) {
    Watchdog.reset();
  }
  int count = 0;
  int count2 = 0;
  unsigned long start = millis();
  Serial.println("waiting for LoRa data . . ..");
  while (rcv_LoRa_flag == 0) {
    uint8_t buf2[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len3 = sizeof(buf2);
    if (mode == 14) {
      // timeOut in case walang makuhang data LoRa transmitter ~15 minutes
      if ((millis() - start) > LORATIMEOUTMODE3) {
        start = millis();
        // send gateway rssi values if no received from transmitter
        get_rssi(get_logger_mode());
        rcv_LoRa_flag = 1;
      }
    } else {
      // timeOut in case walang makuhang data LoRa transmitter ~4 minutes 260 000
      if ((millis() - start) > LORATIMEOUT) {
        Serial.println("Time out reached.");
        start = millis();
        get_rssi(get_logger_mode());
        rcv_LoRa_flag = 1;
      }
    }

    if (rf95.available()) {
      // Should be a message for us now
      if (rf95.recv(buf2, &len3)) {
        int i = 0;
        for (i = 0; i < len3; ++i) {
          received[i] = (uint8_t)buf2[i];
        }
        received[i] = (uint8_t)'\0';

        if (strstr(received, ">>")) { /*NOT LoRa: 0, 2, 6, 7*/
          flashLed(LED_BUILTIN, 3, 60);
          if (mode == 1 || mode == 3 || mode == 4 || mode == 5 || mode == 14)  // LoRa mode only
          {
            /*remove 1st & 2nd character*/
            for (byte i = 0; i < strlen(received); i++) {
              received[i] = received[i + 2];
            }

            send_thru_gsm(received, get_serverNum_from_flashMem());
            // print RSSI values
            tx_RSSI = String(rf95.lastRssi(), DEC);
            Serial.print("RSSI: ");
            Serial.println(tx_RSSI);
          } else {
            Serial.print("Received Data: ");
            Serial.println(received);
            // print RSSI values
            tx_RSSI = String(rf95.lastRssi(), DEC);
            Serial.print("RSSI: ");
            Serial.println(tx_RSSI);
          }
        } else if (received, "*VOLT:") {

          send_thru_gsm(received, get_serverNum_from_flashMem());

          tx_RSSI = String(rf95.lastRssi(), DEC);
          Serial.print("RSSI: ");
          Serial.println(tx_RSSI);
        } else {
          Serial.println("Invalid LoRa message!");
          // rcv_LoRa_flag = 1;
        }
      }
    }
  }
  rcv_LoRa_flag = 0;
  flashLed(LED_BUILTIN, 3, 80);
  if (debug_flag == 0) {
    Watchdog.reset();
  }
}

bool check_duplicates_in_stack(char *payload_to_check) {

  stack_temp[0] = '\0';
  strcpy(stack_temp, sending_stack);
  stack_temp[strlen(sending_stack) + 1] = '\0';
  bool continue_flag = true;
  // Serial.println("checking payload duplicates..");
  // Serial.println(payload_to_check);

  char *stack_token = strtok(stack_temp, "~");
  while (stack_token != NULL) {
    // Serial.println(stack_token);
    if (strstr(stack_token, payload_to_check)) {
      // Serial.println("duplicate found!");
      continue_flag = false;
      break;
    }
    stack_token = strtok(NULL, "~");
  }
  return continue_flag;
}