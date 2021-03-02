/**
 * @brief Send message to mobile number
 * 
 * @return success: sent, failed: not sent
 */
void send_thru_gsm(char *inputMessage, String serverNumber)
{
  String smsCMD = ("AT+CMGS=");
  String quote = ("\"");
  String CR = ("\r");
  char msgToSend[250];
  char atCmgsNo[250];
  String incomingData = String(inputMessage);
  incomingData.replace("\r", "");
  incomingData.toCharArray(msgToSend, 250);
  String rawMsg = smsCMD + quote + serverNumber + quote + CR;
  rawMsg.toCharArray(atCmgsNo, 250);

  Serial.print("Sending to '");
  Serial.print(serverNumber);
  Serial.print("': ");
  Serial.println(msgToSend);

  GSMSerial.write("AT+CMGF=1\r");
  gsmReadOK();

  for (int i = 0; i < 3; i++)
  {
    gsmSerialFlush();
    GSMSerial.write(atCmgsNo); //AT+CMGS="639XXXXXXXXX"\r
    delay_millis(500);
    if (strstr(readGSMResponse(), ">"))
    {
      GSMSerial.write(msgToSend);
      delay_millis(500);
      GSMSerial.write(26);
      if (readGSMResponse(), "OK")
      {
        Serial.println("Message sent!");
        break;
      }
    }
    else
    {
      Serial.println("Sending message failed!");
      GSMSerial.write(26); //need to clear pending gsm cmd
      delay_millis(500);
      Serial.print("CSQ: ");
      Serial.println(readCSQ());
    }
  }
}

/**Commonly used AT commands
 * AT+CSQ - read current signal strenght
 * AT+COPS? - reads current network connected to
 * AT+COPS=?  - reads available networks in the area
 * AT+CMGF=1  - make gsm receive sms to text format (PDU is in binary - 0) 
 * AT+CMGR=1  - read 1st sms in inbox
 * AT+CMGL="ALL"  - read all current received sms
*/
void manualGSMcmd()
{
  char cmdAllchar[80];
  String quote = ("\"");
  String CR = ("\r");
  cmdAllchar[0] = '\0';

  Serial.setTimeout(15000);
  Serial.print("Insert GSM command: ");
  String manualCMD = Serial.readStringUntil('\n');
  Serial.println(manualCMD);
  String cmdAll = quote + manualCMD + quote + CR;
  cmdAll.toCharArray(cmdAllchar, sizeof(cmdAllchar));

  GSMSerial.write(cmdAllchar);
  delay_millis(500);
  while (GSMSerial.available() > 0)
  {
    processIncomingByte(GSMSerial.read(), 1);
  }
}

bool isPassWordCorrect(char *_passW)
{
  if (strstr(get_password_from_flashMem(), _passW))
  {
    Serial.println("Valid password received!");
    return true;
  }
  else
  {
    Serial.println("Invalid password!");
    return false;
  }
}

/** Over the air commands
 * Parse sms if valid and execute command
 * REGISTER:SENSLOPE:639954645704     - register mobile number to control MCU
 * SENSORPOLL:SENSLOPE:               - force data sampling
 * SERVERNUMBER:SENSLOPE:639954645704 - change server number in flash memory
 * ?SERVERNUM:SENSLOPE:               - check current server number
 * RESET:SENSLOPE:                    - reset MCU
 * SENSLOPE,REGISTERNUM, - register mobile number to control MCU, if not it will ignore SMS
 * SENSLOPE,SENSORPOLL, - force data sampling
 * SENSLOPE,SERVERNUMBER,639954645704 - change server number in flash memory
 * SENSLOPE,?SERVERNUM, - check current server number
 * SENSLOPE,?PASSW, - check Password
*/
void process_data(char *data)
{
  //REGISTER:SENSLOPE:639954645704
  if (strncmp(data, "REGISTER:", 9) == 0)
  {
    Serial.println("REGISTER is read");
    char *_password = strtok(data + 9, ":");
    char *_regThisNum = strtok(NULL, ":");

    // Serial.println(_password);
    tempServer = String(_regThisNum);
    tempServer.replace(" ", "");
    regServer = tempServer;
    Serial.println(regServer);

    if (isPassWordCorrect(_password))
    {
      registerNumber = true;
      send_thru_gsm("Number Registered!", regServer);
    }
  }
  //SENSORPOLL:SENSLOPE:
  else if (strncmp(data, "SENSORPOLL", 10) == 0)
  {
    Serial.println("SENSORPOLL is read");
    char *_password = strtok(data + 10, ":");
    // Serial.println(_password);

    if (isPassWordCorrect(_password) && registerNumber)
    {
      get_Due_Data(get_logger_version(), regServer);
    }
  }
  //SERVERNUMBER:SENSLOPE:639954645704
  else if (strncmp(data, "SERVERNUMBER", 12) == 0)
  {
    char messageToSend[100];
    char newServer[50];
    Serial.println("change server number");
    char *_password = strtok(data + 12, ":");
    char *_newServerNum = strtok(NULL, ":");
    // Serial.println(_password);
    Serial.println(_newServerNum);

    if (isPassWordCorrect(_password) && registerNumber)
    {
      //strore new server number to flash memory
      strcpy(flashServerNumber.inputNumber, _newServerNum);
      newServerNum.write(flashServerNumber); //save to flash memory

      get_serverNum_from_flashMem().toCharArray(newServer, sizeof(newServer));
      strncpy(messageToSend, "New server number: ", 19);
      strncat(messageToSend, newServer, sizeof(newServer));
      Serial.println(messageToSend);

      send_thru_gsm(messageToSend, regServer);
    }
  }
  //?SERVERNUM:SENSLOPE:
  else if (strncmp(data, "?SERVERNUM", 10) == 0)
  {
    char currenServerNumber[50];
    char messageToSend[100];
    Serial.println("Check current server number");
    char *_password = strtok(data + 10, ":");
    // Serial.println(_password);

    if (isPassWordCorrect(_password) && registerNumber)
    {
      get_serverNum_from_flashMem().toCharArray(currenServerNumber, sizeof(currenServerNumber));

      strncpy(messageToSend, "Current server number: ", 23);
      strncat(messageToSend, currenServerNumber, sizeof(currenServerNumber));
      Serial.println(messageToSend);

      send_thru_gsm(messageToSend, regServer);
    }
  }
  //RESET:SENSLOPE:
  else if (strncmp(data, "RESET", 5) == 0)
  {
    Serial.println("Resetting microcontroller!");
    char *_password = strtok(data + 10, ":");

    if (isPassWordCorrect(_password) && registerNumber)
    {
      send_thru_gsm("Resetting datalogger, please register your number again to access OTA commands.", regServer);
      Serial.println("Resetting Watchdog in 2 seconds");
      int countDownMS = Watchdog.enable(2000); //max of 16 seconds
    }
  }
  /*
  if (strncmp(data, "+CMT: \"+", 8) == 0)
  {
    char *mobNum = strtok(data + 8, "\",\""); //\"
    char *name = strtok(NULL, "\",\"");
    char *timeStamp = strtok(NULL, "+");
    char *afterPlus = strtok(NULL, "\"\r");
    char *cmd_message = strtok(NULL, ",");

    String tempServer = String(mobNum);
    tempServer.replace(" ", "");

    Serial.print("Sender Number:");
    Serial.println(tempServer);

    // Serial.println(mobNum);
    // Serial.println(name);
    // Serial.println(timeStamp);
    // Serial.println(afterPlus);
    // Serial.println(cmd_message);

    Serial.println(_data1);
    if (_data1.indexOf("SENSLOPE,") > -1)
    {
      Serial.println("Valid Password received!!!");
    }
    if (valid_password)
    {
      Serial.println("valid password si TRUE");
      valid_password = false;
    }
  }
  else if (strncmp(data, get_password_from_flashMem(), 9) == 0)
  {
    Serial.println("Valid PASSWORD received!");
    char *smsCommand = strtok(data + 9, ",");
    char *serverSMS = strtok(NULL, ",");
    Serial.print("Keyword: ");
    Serial.println(smsCommand);
    flashLed(LED_BUILTIN, 2, 60);
    //input sms command here
    if (strstr(smsCommand, "SENSORPOLL"))
    {
      if (regServer == tempServer)
      {
        flashLed(LED_BUILTIN, 3, 50);
        get_Due_Data(get_logger_version(), regServer);
      }
    }
    else if (strstr(smsCommand, "REGISTERNUM"))
    {
      flashLed(LED_BUILTIN, 2, 50);
      regServer = "";
      regServer += tempServer;
      Serial.print("Saving this number: ");
      Serial.println(regServer);
      send_thru_gsm("Number Registered!", regServer);
    }
    else if (strstr(smsCommand, "?REGNUM"))
    {
      if (regServer == tempServer)
      {
        char serNum[50];
        flashLed(LED_BUILTIN, 3, 50);
        regServer.toCharArray(serNum, sizeof(serNum));
        Serial.print("Registered Number: ");
        Serial.println(serNum);
      }
    }
    else if (strstr(smsCommand, "?SERVERNUM"))
    {
      if (regServer == tempServer)
      {
        char tempSerNum[50];
        flashLed(LED_BUILTIN, 3, 50);
        Serial.print("Server Number: ");
        Serial.println(get_serverNum_from_flashMem());
        get_serverNum_from_flashMem().toCharArray(tempSerNum, sizeof(tempSerNum));
        send_thru_gsm(tempSerNum, regServer);
      }
    }
    else if (strstr(smsCommand, "SERVERNUMBER"))
    {
      char messageNum[100];
      char newServer[50];
      if (regServer == tempServer)
      {
        flashLed(LED_BUILTIN, 3, 50);
        //save new server number from remote
        strcpy(flashServerNumber.inputNumber, serverSMS);
        newServerNum.write(flashServerNumber);

        get_serverNum_from_flashMem().toCharArray(newServer, sizeof(newServer));

        strncpy(messageNum, "New server number: ", 19);
        strncat(messageNum, newServer, sizeof(newServer));
        Serial.println(messageNum);
        send_thru_gsm(messageNum, tempServer);
      }
    }
    else if (strstr(smsCommand, "?PASSW"))
    {
      if (regServer == tempServer)
      {
        flashLed(LED_BUILTIN, 3, 50);
        Serial.print("Password: ");
        Serial.println(get_password_from_flashMem());
        send_thru_gsm(get_password_from_flashMem(), regServer);
      }
    }
    else if (strstr(smsCommand, "RESET_NOW"))
    {
      if (regServer == tempServer)
      {
        flashLed(LED_BUILTIN, 3, 50);
        send_thru_gsm(get_password_from_flashMem(), "Manual Reset! - watchdog activated");
        enable_watchdog();
      }
    }
  }
  */
}

/* Read GSM reply; non read blocking process*/
void processIncomingByte(const byte inByte, int _mode)
{
  // const unsigned int MAX_INPUT = 256; // how much serial data we expect before a newline
  static char input_line[256];
  static unsigned int input_pos = 0;

  switch (inByte)
  {
  case '\n':                   // end of text
    input_line[input_pos] = 0; // terminating null byte
    if (_mode == 0)
    {
      process_data(input_line);
    }
    else
    {
      Serial.println(input_line);
    }
    // reset buffer for next time
    input_pos = 0;
    break;

  case '\r': // discard carriage return
    break;

  default:
    // keep adding if not full ... allow for terminating null byte
    if (input_pos < (MAX_INPUT - 1))
      input_line[input_pos++] = inByte;
    break;
  } // end of switch
} // end of processIncomingByte

char *readGSMResponse()
{
  // char response[100]; //200
  int length = sizeof(response);
  response[0] = '\0';

  for (int i = 0; GSMSerial.available() > 0 && i < length; i++)
  {
    response[i] = GSMSerial.read();
  }
  return response;
}

/*
char *readGSMResponse_test()
{
  while (GSMSerial.available() > 0)
  {
    // processIncomingByte(GSMSerial.read(), 0);
  }
}

void process_gsm_data(char *_data)
{
  if (strncmp(_data, "OK", 2) == 0)
  {
    Serial.println("read 'OK' from GSM");
  }
}
*/

bool gsmReadOK()
{
  for (int i = 0; i < 100; i++) // 50 - 500ms
  {
    if (strstr(readGSMResponse(), "OK"))
    {
      // Serial.println("Read OK from GSM");
      return true;
      break;
    }
    // Serial.print(" .");
    delay_millis(10);
  }
  return false;
}

void gsmSerialFlush()
{
  while (GSMSerial.available() > 0)
  {
    char t = GSMSerial.read();
  }
}

int getCsqStrtok(char *buffer)
{
  char *tmpBuf;
  tmpBuf = strtok(buffer, ": ");
  tmpBuf = strtok(NULL, ",");
  return (atoi(tmpBuf));
}

char *readCSQ()
{
  char c_csq[5] = "99";
  gsmSerialFlush();
  GSMSerial.write("AT+CSQ\r");
  delay_millis(500);
  snprintf(_csq, sizeof _csq, "%d", getCsqStrtok(readGSMResponse()));
  return _csq;
}

String getCSQ()
{
  String cCSQval = ("99");
  gsmSerialFlush();
  GSMSerial.write("AT\r");
  delay(500);
  if (gsmReadOK())
  {
    GSMSerial.write("AT+CSQ\r");
    delay(500);
    String nCSQ = String(getCsqStrtok(readGSMResponse()));
    return nCSQ;
  }
  else
  {
    return cCSQval;
  }
}

/*  RING pin
  - normal voltage: 2.7v
  - when SMS received 2.7v , LOW , 2.7v
  - when call pin is 0v, then back after to 2.7 after end call
  *i-connect sa intrrupt pin, mabubuhay if may received na sms
*/
void gsmHangup()
{
  //when ring pin triggers, call this function
  delay_millis(2000);
  GSMSerial.write("ATH\r");
}

void gsmDeleteReadSmsInbox()
{
  GSMSerial.write("AT+CMGD=1,2\r");
  // Serial.println(readGSMResponse());
  delay_millis(1000);
  if (gsmReadOK())
  {
    Serial.println("Command success!");
  }
  else
  {
    Serial.println("Command failed.");
  }
}

/**
 * @brief Check if mobile number network is GLOBE or SMART
 * 
 * @return : SMART: 63908, failed: not sent
 * @default : GLOBE
 */
String simNetwork()
{
  char tNum[50];
  get_serverNum_from_flashMem().toCharArray(tNum, sizeof(tNum));
  if (strstr(tNum, "63908"))
  {
    return "SMART";
  }
  else
  {
    return "GLOBE";
  }
}

void gsmManualNetworkConnect()
{
  // String simNetwork = "GLOBE"; //or "SMART" , hardcoded
  String simNetCommand = "AT+COPS=1,1,\"" + simNetwork() + "\"\r";
  String response;
  char command[25];
  simNetCommand.toCharArray(command, 25);
  Serial.println(command);

  for (int i = 0; i < 10; i++)
  {
    gsmSerialFlush();
    GSMSerial.write(command);
    delay_millis(1000);
    if (gsmReadOK())
    {
      Serial.print("GSM Connected to: ");
      Serial.println(simNetwork());
      break;
    }
    else
    {
      Serial.println("retrying connection");
    }
  }
}

void sleepGSM()
{
  //the module enters sleep mode after 5 seconds of inactivity
  GSMSerial.write("AT+CSCLK=2\r");
  if (gsmReadOK())
  {
    Serial.println("GSM going to sleep!");
  }
}

void wakeGSM()
{
  /* to wake it up, you need to send any AT command, which will be ignored by 
  the module (so no response), followed (within 5 seconds) by "AT+CSCLK=0\r" */

  // To save more power +CFUN=0 before SLEEP and +CFUN=1 after WAKE UP.
  GSMSerial.write("AT\r");
  delay_millis(50);
  GSMSerial.write("AT\r");
  gsmReadOK();
  GSMSerial.write("AT+CSCLK=0\r");
  gsmReadOK();
  GSMSerial.write("AT+CMGF=1\r");
  if (gsmReadOK())
  {
    Serial.println("GSM is alive!");
  }
  else
  {
    Serial.println("GSM did NOT wake!");
  }
}

void resetGSM()
{
  int overflow_counter = 0;
  Serial.println("GSM resetting . . .");
  digitalWrite(GSMRST, LOW);
  delay_millis(500);
  digitalWrite(GSMRST, HIGH);
  delay_millis(1000);
  for (int i = 0; i < 6; i++)
  {
    GSMSerial.write("AT\r"); //gsm initialization
    Serial.println("Sending AT cmd");
    if (gsmReadOK() == true)
    {
      Serial.println("Got reply from GSM");
      break;
    }
    Serial.println("No reply from GSM");
  }
  // gsmManualNetworkConnect();
  Serial.println("Done resetting GSM");
}

void turn_ON_GSM(int _gsmPowerMode)
{
  if (_gsmPowerMode == 0)
  {
    digitalWrite(GSMPWR, HIGH);
    Serial.println("Turning ON GSM ");
    delay_millis(20000);
    int overflow_counter = 0;
    do
    {
      gsmManualNetworkConnect();
      overflow_counter++;
    } while (readCSQ() == 0 || overflow_counter < 2);

    GSMSerial.write("ATE0\r"); //turn off echo
    if (gsmReadOK())
    {
      Serial.println("Turning GSM to NO echo mode");
      Serial.println("GSM is no now ready!");
    }
    else
    {
      Serial.println("");
      Serial.println("Check GSM if connected or powered ON!");
    }
    Serial.print("CSQ: ");
    Serial.println(readCSQ());
  }
  else if (_gsmPowerMode == 1)
  {
    wakeGSM();
  }
  else
  {
  }
}

void gsm_network_connect()
{
  int overflow_counter = 0;
  do
  {
    gsmManualNetworkConnect();
    overflow_counter++;
  } while (readCSQ() == 0 || overflow_counter < 2);

  GSMSerial.write("ATE0\r"); //turn off echo
  if (gsmReadOK())
  {
    Serial.println("Turning GSM to NO echo mode");
    Serial.println("GSM is no now ready!");
  }
  else
  {
    Serial.println("");
    Serial.println("Check GSM if connected or powered ON!");
  }
  Serial.print("CSQ: ");
  Serial.println(readCSQ());
}

void turn_OFF_GSM(int _gsmPowerMode)
{
  gsmDeleteReadSmsInbox();
  delay_millis(1000);
  if (_gsmPowerMode == 0)
  {
    delay_millis(5000);
    digitalWrite(GSMPWR, LOW);
    Serial.println("Turning OFF GSM . . .");
  }
  else if (_gsmPowerMode == 1)
  {
    sleepGSM();
  }
  else
  {
  }
}

void delay_millis(int _delay)
{
  uint8_t delay_turn_on_flag = 0;
  unsigned long _delayStart = millis();
  // Serial.println("starting delay . . .");
  do
  {
    if ((millis() - _delayStart) > _delay)
    {
      _delayStart = millis();
      delay_turn_on_flag = 1;
      // Serial.println("delay timeout!");
    }
  } while (delay_turn_on_flag == 0);
}