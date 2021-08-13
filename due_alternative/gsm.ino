/* 
  Function: send_thru_gsm

    Sends the text message to the server number. 
    
      - Build the AT+CMGS command [AT+CMGS = "<ServerNumber>"\r]
      - Waits for the ">" character before entering the text message
      - Sends the complete text message and checks for an "OK" response. LED indicator will also flash on succesful sending.
      - GSM is reset when the sending of the same text message fails thrice.
      
  Parameters:
    inputMessage - message to be sent
    ServerNumber - recipient number

  Returns:
    n/a

  See Also:
    n/a
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
  int send_count = 0;
  
  for (int i = 0; i < 5; i++)
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
        flashLed(LED_BUILTIN, 2, 30);
        break;
      }
    }
    else
    {
      send_count++;
      Serial.println("Sending message failed!");
      GSMSerial.write(26); //need to clear pending gsm cmd
      delay_millis(500);
    }
    if (send_count == 3)
    {
      resetGSM();
      gsm_network_connect();
      init_gsm();
      Serial.print("CSQ: ");
      Serial.println(readCSQ());
    }
  }
}

/* 
  Function: manualGSMcmd
    Accepts GSM commands based on the SIM800L AT instruction set.
    
    Commonly used AT commands:
    
      - AT+CSQ - reads current signal strength      
      - AT+COPS? - reads current network connected to      
      - AT+COPS=?  - reads available networks in the area      
      - AT+CMGF=1  - make gsm receive sms to text format (PDU is in binary - 0)       
      - AT+CMGR=1  - read 1st sms in inbox      
      - AT+CMGL="ALL"  - read all current received sms      
      - AT+CPMS?   - Gets the # of SMS messages stored in GSM modem.      
      - AT+CNMI=1,2,0,0,0 - read incoming message from buffer      
      - AT+CSCLK=2 - sleep GSM via AT command      
      - AT+CSCLK=0 - wake gsm from sleep mode in AT command

  Parameters:
    n/a

  Returns:
    n/a

  See Also:
    <processIncomingByte>
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

/* 
  Function: isPassWordCorrect

    Compares the provided password in the OTA command with the stored password.

  Parameters:
    n/a

  Returns:
    True / False

  See Also:
    <process_data>
*/
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

/* 
  Function: process_data

    Processes the over-the-air commands received by the GSM.

    Over the air commands:

     * REGISTER:SENSLOPE:639954645704         - register mobile number to control MCU *9
     * SENSORPOLL:SENSLOPE:                   - force data sampling *10
     * SERVERNUMBER:SENSLOPE:639954645704     - change server number in flash memory  *12
     * ?SERVERNUMBER:SENSLOPE:                - check current server number *13
     * RESET:SENSLOPE:                        - reset MCU   *5
     * ?PASSW:SENSLOPE:                       - check current password  *6
     * PASSWORD:SENSLOPE:[updated password]:  - change password  *8
     * SETDATETIME:SENSLOPE:[YYYY,MM,DD,HH,MM,SS,dd[0-6/m-sun],] 2021,02,23,21,22,40,1, *11
     * SETSENDINGTIME:SENSLOPE:[0-4]:         - sending time *14
     * CMD?:SENSLOPE:                         - read current DUE command
     * CMD:SENSLOPE:[SENSLOPE]:               - update DUE command

  Parameters:
    n/a

  Returns:
    n/a

  See Also:
    n/a
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
  //?SERVERNUMBER:SENSLOPE:
  else if (strncmp(data, "?SERVERNUMBER", 13) == 0)
  {
    char currenServerNumber[30];
    char messageToSend[100];
    Serial.println("Check current server number");
    char *_password = strtok(data + 13, ":");
    currenServerNumber[0] = '\0';
    messageToSend[0] = '\0';

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
    char *_password = strtok(data + 5, ":");

    if (isPassWordCorrect(_password) && registerNumber)
    {
      send_thru_gsm("Resetting datalogger, please register your number again to access OTA commands.", regServer);
      Serial.println("Resetting Watchdog in 2 seconds");
      int countDownMS = Watchdog.enable(2000); //max of 16 seconds
    }
  }
  //?PASSW:SENSLOPE:
  else if (strncmp(data, "?PASSW", 6) == 0)
  {
    char messageToSend[100];
    Serial.println("Reading current password!");
    char *_password = strtok(data + 6, ":");

    if (isPassWordCorrect(_password) && registerNumber)
    {
      Serial.println("Sending current password!");
      strncpy(messageToSend, "Current password: ", 18);
      strncat(messageToSend, get_password_from_flashMem(), 50);
      send_thru_gsm(messageToSend, regServer);
    }
  }
  //PASSWORD:SENSLOPE:[updated password]:
  else if (strncmp(data, "PASSWORD", 8) == 0)
  {
    char messageToSend[100];
    char newPassword[50];
    char *_password = strtok(data + 8, ":");
    char *_newPassword = strtok(NULL, ":");
    Serial.println(_newPassword);
    Serial.println("change server number");

    if (isPassWordCorrect(_password) && registerNumber)
    {
      //strore new password to flash memory
      strcpy((flashPassword.keyword), _newPassword);
      flashPasswordIn.write(flashPassword); //save to flash memory

      strncpy(messageToSend, "New password: ", 14);
      strncat(messageToSend, get_password_from_flashMem(), 50);
      Serial.println(messageToSend);

      send_thru_gsm(messageToSend, regServer);
    }
  }
  //SETDATETIME:SENSLOPE:[YYYY,MM,DD,HH,MM,SS,dd[0-6/m-sun],] 2021,02,23,21,22,40,1,
  else if (strncmp(data, "SETDATETIME", 11) == 0)
  {
    Serial.println("change timestamp!");
    char messageToSend[100];
    messageToSend[0] = '\0';
    Serial.println(data);
    char *_password = strtok(data + 11, ":");
    char *YY = strtok(NULL, ",");
    char *MM = strtok(NULL, ",");
    char *DD = strtok(NULL, ",");
    char *hh = strtok(NULL, ",");
    char *mm = strtok(NULL, ",");
    char *ss = strtok(NULL, ",");
    char *dd = strtok(NULL, ",");

    int _YY = atoi(YY);
    int _MM = atoi(MM);
    int _DD = atoi(DD);
    int _hh = atoi(hh);
    int _mm = atoi(mm);
    int _ss = atoi(ss);
    int _dd = atoi(dd);

    if (isPassWordCorrect(_password) && registerNumber)
    {
      //set date and time
      Serial.println(_YY);
      Serial.println(_MM);
      Serial.println(_DD);
      Serial.println(_hh);
      Serial.println(_mm);
      Serial.println(_ss);
      Serial.println(_dd);
      adjustDate(_YY, _MM, _DD, _hh, _mm, _ss, _dd);
      readTimeStamp();

      strncpy(messageToSend, "Current timestamp: ", 19);
      strncat(messageToSend, Ctimestamp, sizeof(Ctimestamp));
      Serial.println(messageToSend);
      send_thru_gsm(messageToSend, regServer);
    }
  }
  //SENDINGTIME:SENSLOPE:[0-4]:
  else if (strncmp(data, "SETSENDINGTIME", 14) == 0)
  {
    Serial.println("change sending time!");
    char messageToSend[100];
    char sendStorage[10];
    messageToSend[0] = '\0';
    sendStorage[0] = '\0';
    Serial.println(data);

    char *_password = strtok(data + 14, ":");
    char *inputSending = strtok(NULL, ":");
    int _inputSending = atoi(inputSending);

    if (isPassWordCorrect(_password) && registerNumber)
    {
      //set sending time
      Serial.println(_inputSending);
      alarmStorage.write(_inputSending);
      sprintf(sendStorage, "%d", alarmFromFlashMem());

      strncpy(messageToSend, "Updated sending time: ", 22);
      strncat(messageToSend, sendStorage, sizeof(sendStorage));
      Serial.println(messageToSend);
      send_thru_gsm(messageToSend, regServer);
    }
  }
  //CMD?:SENSLOPE:
  else if (strncmp(data, "CMD?", 4) == 0)
  {
    Serial.println("change sending time!");
    char messageToSend[100];
    ;
    messageToSend[0] = '\0';
    char *_password = strtok(data + 4, ":");

    if (isPassWordCorrect(_password) && registerNumber)
    {
      //read DUE command
      sensCommand = passCommand.read();
      strncpy(messageToSend, "Current DUE command: ", 21);
      strncat(messageToSend, sensCommand.senslopeCommand, 50);
      Serial.println(messageToSend);
      send_thru_gsm(messageToSend, regServer);
    }
  }
  //CMD:SENSLOPE[ARQCM6T/6S]
  else if (strncmp(data, "CMD?", 4) == 0)
  {
    Serial.println("change sending time!");
    char messageToSend[100];
    messageToSend[0] = '\0';

    char *_password = strtok(data + 4, ":");
    char *dueCmd = strtok(NULL, ":");

    if (isPassWordCorrect(_password) && registerNumber)
    {
      //read DUE command
      strcpy((sensCommand.senslopeCommand), dueCmd);
      passCommand.write(sensCommand); //save to flash memory

      //read command
      sensCommand = passCommand.read();

      strncpy(messageToSend, "Current DUE command: ", 21);
      strncat(messageToSend, sensCommand.senslopeCommand, 50);
      Serial.println(messageToSend);
      send_thru_gsm(messageToSend, regServer);
    }
  }
  //LISTPHONENUM:SENSLOPE:
}

/* 
  Function: processIncomingByte

    Accumulates the data received from GSM serial until a newline character is received.

  Parameters:
    inByte - incoming byte
    mode - if this is 0, the data will be fed to process_data function, otherwise, the data will just be printed in the terminal.

  Returns:
    n/a

  See Also:
    <process_data>
*/
void processIncomingByte(const byte inByte, int _mode)
{
  // const unsigned int MAX_INPUT = 256; // how much serial data we expect before a newline
  static char input_line[255];
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

/* 
  Function: readGSMResponse

    Reads the data received from the GSM serial line. Number of characters is limited by the size of global variable *response*.

  Parameters:
    n/a

  Returns:
    response

  See Also:
    n/a
*/
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
  Function: gsmReadOK

    Calls the *readGSMResponse* function and checks if "OK" is received.

  Parameters:
    n/a

  Returns:
    True/False

  See Also:
    <readGSMResponse>
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

/* 
  Function: gsmSerialFlush

    Clears the content of GSM serial rx line.

  Parameters:
    n/a

  Returns:
    n/a

  See Also:
    n/a
*/
void gsmSerialFlush()
{
  while (GSMSerial.available() > 0)
  {
    char t = GSMSerial.read();
  }
}

/* 
  Function: getCsqStrtok

    Extracts the CSQ value from the response of *readCSQ* function.

  Parameters:
    n/a

  Returns:
    integer CSQ value

  See Also:
    <readCSQ>
*/
int getCsqStrtok(char *buffer)
{
  char *tmpBuf;
  tmpBuf = strtok(buffer, ": ");
  tmpBuf = strtok(NULL, ",");
  return (atoi(tmpBuf));
}

/* 
  Function: readCSQ

    Issues the *AT+CSQ* GSM command and extract the CSQ value by passing it to the *getCsqStrtok* function.

  Parameters:
    n/a

  Returns:
    CSQ value as char array

  See Also:
    <getCsqStrtok> <readGSMResponse>
*/
char *readCSQ()
{
  char c_csq[5] = "99";
  gsmSerialFlush();
  GSMSerial.write("AT+CSQ\r");
  delay_millis(500);
  snprintf(_csq, sizeof _csq, "%d", getCsqStrtok(readGSMResponse()));
  return _csq;
}

/* 
  Function: gsmHangup

    Issues the *ATH* GSM command to hang up a received call.
    
    RING pin:
    
      - Normal voltage: 2.7v
      - When SMS received 2.7v , LOW , 2.7v. This can be used to call an ISR.
      - During the call, the pin is pulled to 0V, and returns to 2.7V at the end of call.
      
  Parameters:
    n/a

  Returns:
    n/a

  See Also:
    n/a
*/
void gsmHangup()
{
  //when ring pin triggers, call this function
  delay_millis(2000);
  GSMSerial.write("ATH\r");
}

/* 
  Function: gsmDeleteReadSmsInbox

    Issues the *AT+CMGD* GSM command to delete the contents of inbox.
      
  Parameters:
    n/a

  Returns:
    n/a

  See Also:
    n/a
*/
void gsmDeleteReadSmsInbox()
{
  Serial.println("deleting sms read from inbox . . .");
  GSMSerial.write("AT+CMGD=1,2\r");
  delay_millis(1000);
  if (gsmReadOK())
  {
    Serial.println("deleting done!");
  }
  else
  {
    Serial.println("deleting failed!");
  }
}

/* 
  Function: simNetwork

    Checks if the server number provided is a SMART or GLOBE SIM based on the SIM prefix. Current SMART SIM cards used by the GSM server begins with "0908". 
      
  Parameters:
    n/a

  Returns:
    "SMART" or "GLOBE"

  See Also:
    n/a
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

/* 
  Function: gsmManualNetworkConnect

    Issues the *AT+COPS* GSM command to manually connect to a mobile network.
      
  Parameters:
    n/a

  Returns:
    n/a

  See Also:
    n/a
*/
void gsmManualNetworkConnect()
{
  // String simNetwork = "GLOBE"; //or "SMART" , hardcoded
  String simNetCommand = "AT+COPS=1,1,\"" + simNetwork() + "\"\r";
  String response;
  char command[25];
  simNetCommand.toCharArray(command, 25);
  Serial.println("Connecting GSM to network.");
  Serial.println(command);

  for (int i = 0; i < 10; i++)
  {
    gsmSerialFlush();
    GSMSerial.write(command);
    delay_millis(1000);
    if (gsmReadOK())
    {
      Serial.print("");
      Serial.print("GSM Connected to: ");
      Serial.println(simNetwork());
      break;
    }
    else
    {
      Serial.print(". ");
    }
  }
}

/* 
  Function: sleepGSM

    Issues the *AT+CSCLK* GSM command to enter sleep mode.
      
  Parameters:
    n/a

  Returns:
    n/a

  See Also:
    n/a
*/
void sleepGSM()
{
  //the module enters sleep mode after 5 seconds of inactivity
  GSMSerial.write("AT+CSCLK=2\r");
  if (gsmReadOK())
  {
    Serial.println("GSM going to sleep!");
  }
  else
  {
    Serial.println("GSM failed to sleep!");
  }
}

/* 
  Function: wakeGSM

    Wakes up the GSM module.
      
  Parameters:
    n/a

  Returns:
    n/a

  See Also:
    n/a
*/
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
  GSMSerial.write("AT\r");
  if (gsmReadOK())
  {
    Serial.println("GSM is alive!");
  }
  else
  {
    Serial.println("GSM did NOT wake!");
  }
}

/* 
  Function: resetGSM

    Resets the GSM module using the RST pin.
      
  Parameters:
    n/a

  Returns:
    n/a

  See Also:
    n/a
*/
void resetGSM()
{
  int overflow_counter = 0;
  Serial.println("GSM resetting . . .");
  digitalWrite(GSMRST, LOW);
  delay_millis(500);
  digitalWrite(GSMRST, HIGH);
  delay_millis(1000);
  Serial.println("Sending AT cmd");
  for (int i = 0; i < 6; i++)
  {
    GSMSerial.write("AT\r"); //gsm initialization
    Serial.print(". ");
    if (gsmReadOK() == true)
    {
      Serial.println("Got reply from GSM");
      break;
    }
  }
  Serial.println(" ");
  Serial.println("Done resetting GSM");
  Serial.println(" ");
  // init_gsm();
  Serial.println(" ");
}

/* 
  Function: init_gsm

    Initializes the GSM module.
    
      * AT - basic GSM test
      * ATE0 - set GSM to no echo mode
      * AT+CMGF=1 - set GSM to text mode
      
  Parameters:
    n/a

  Returns:
    n/a

  See Also:
    n/a
*/
void init_gsm()
{
  Serial.println("initializing gsm . . .");
  GSMSerial.write("AT\r"); //gsm initialization
  gsmReadOK();
  GSMSerial.write("ATE0\r"); //turn off echo
  if (gsmReadOK())
  {
    Serial.println("Turning GSM to NO echo mode.");
  }
  for (int i = 0; i < 4; i++)
  {
    GSMSerial.write("AT+CMGF=1\r"); //switching to text mode
    if (gsmReadOK())
    {
      Serial.println(" ");
      Serial.println("GSM is set to text mode.");
      Serial.println("GSM is no now ready!");
      break;
    }
    Serial.print(". ");
  }
}

/* 
  Function: turn_ON_GSM

    Power up the GSM module.
      
  Parameters:
    gsmPowerMode - 0 if the datalogger board is configured to switch on/off the GSM. 1 if otherwise.

  Returns:
    n/a

  See Also:
    n/a
*/
void turn_ON_GSM(int _gsmPowerMode)
{
  disable_watchdog();
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

    init_gsm();
    GSMSerial.write("AT\r");
    if (gsmReadOK())
    {
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
  enable_watchdog();
}

/* 
  Function: gsm_network_connect

    Connects the GSM to the mobile network.

      - Manually connect to the mobile network. If it fails to receive an "OK" response, reissue the command until the third try.
      - Check GSM response using AT command.
      - Reads CSQ value and prints the result.
      
  Parameters:
    n/a

  Returns:
    n/a

  See Also:
    <gsmManualNetworkConnect>
*/
void gsm_network_connect()
{
  int overflow_counter = 0;
  do
  {
    gsmManualNetworkConnect();
    overflow_counter++;
  } while (readCSQ() == 0 || overflow_counter < 2);

  GSMSerial.write("AT\r");
  if (gsmReadOK())
  {
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

/* 
  Function: turn_OFF_GSM

    Power down the GSM module.
      
  Parameters:
    gsmPowerMode - 0 if the datalogger board is configured to switch on/off the GSM. 1 if otherwise.

  Returns:
    n/a

  See Also:
    n/a
*/
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

/* 
  Function: delay_millis

    Time delay.
      
  Parameters:
    delay - integer amount of time delay in milliseconds.

  Returns:
    n/a

  See Also:
    n/a
*/
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
