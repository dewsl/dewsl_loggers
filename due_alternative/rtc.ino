/* 
  Function: rtc

    <function>

  Parameters:
    n/a

  Returns:
    n/a

  See Also:
    <setup>
*/
void getAtcommand()
{
  // wakeGSM();
  String serial_line, command;
  int i_equals = 0;
  unsigned long startHere = millis();
  bool timeToExit;

  do
  {
    timeToExit = timeOutExit(startHere, DEBUGTIMEOUT);
    serial_line = Serial.readStringUntil('\r\n');
  } while (serial_line == "" && timeToExit == false);
  serial_line.toUpperCase();
  serial_line.replace("\r", "");

  if (timeToExit)
  {
    Serial.println("Exit from debug menu");
    debug_flag_exit = true;
  }

  // echo command if ate is set, default true
  if (ate)
    Serial.println(serial_line);

  // get characters before '='
  i_equals = serial_line.indexOf('=');
  if (i_equals == -1)
    command = serial_line;
  else
    command = serial_line.substring(0, i_equals);

  if (command == ATCMD)
    Serial.println(OKSTR);
  else if (command == ATECMDTRUE)
  {
    ate = true;
    Serial.println(OKSTR);
  }
  else if (command == ATECMDFALSE)
  {
    ate = false;
    Serial.println(OKSTR);
  }
  else if (command == "A")
  {
    Serial.println(F("[0]\tSending data using GSM only"));               //arQ like function only
    Serial.println(F("[1]\tVersion 5 datalogger LoRa with GSM"));        //arQ + LoRa rx
    Serial.println(F("[2]\tLoRa transmitter for version 5 datalogger")); //TX LoRa
    Serial.println(F("[3]\tGateway Mode with only ONE LoRa transmitter"));
    Serial.println(F("[4]\tGateway Mode with TWO LoRa transmitter"));
    Serial.println(F("[5]\tGateway Mode with THREE LoRa transmitter"));
    Serial.println(F("[6]\tLoRa transmitter for Raspberry Pi"));         //TX LoRa
    Serial.println(F("[7]\tSends sensor and rain gauge data via LoRa")); //TX LoRa
    Serial.println(F("[8]\tLoRa dummy transmitter"));                    //TX LoRa
    Serial.println(F("[9]\tGSM - Surficial Tilt"));
    Serial.println(F("[10]\tLoRa Tx - Surficial Tilt"));
    Serial.println(F("[11]\tRain gauge mode only"));

    if (get_logger_version() == 0)
    {
      //default arQ like sending
      turn_ON_GSM(get_gsm_power_mode());
      send_rain_data(0);
      disable_watchdog();
      if (fullblastlogger)
      {
        operation(1, get_serverNum_from_flashMem());
      }
      else
      {
        get_Due_Data(1, get_serverNum_from_flashMem());
      }
      disable_watchdog();
      turn_OFF_GSM(get_gsm_power_mode());
    }
    else if (get_logger_version() == 1)
    {
      //arQ + 1 LoRa receiver
      if (gsmPwrStat)
      {
        turn_ON_GSM(get_gsm_power_mode());
      }
      if (fullblastlogger)
      {
        operation(1, get_serverNum_from_flashMem());
      }
      else
      {
        get_Due_Data(1, get_serverNum_from_flashMem());
      }
      disable_watchdog();
      send_rain_data(0);
      disable_watchdog();
      if (getSensorDataFlag == true && OperationFlag == true)
      {
        receive_lora_data(1);
        disable_watchdog();
      }
      if (gsmPwrStat)
      {
        turn_OFF_GSM(get_gsm_power_mode());
      }
    }
    else if (get_logger_version() == 2)
    {
      //LoRa transmitter of version 5 datalogger
      if (fullblastlogger)
      {
        operation(2, get_serverNum_from_flashMem());
      }
      else
      {
        get_Due_Data(2, get_serverNum_from_flashMem());
      }
      disable_watchdog();
    }
    else if (get_logger_version() == 3)
    {
      //only one trasmitter
      turn_ON_GSM(get_gsm_power_mode());
      send_rain_data(0);
      disable_watchdog();
      receive_lora_data(3);
      disable_watchdog();
      turn_OFF_GSM(get_gsm_power_mode());
    }
    else if (get_logger_version() == 4)
    {
      //Two transmitter
      turn_ON_GSM(get_gsm_power_mode());
      send_rain_data(0);
      disable_watchdog();
      receive_lora_data(4);
      disable_watchdog();
      turn_OFF_GSM(get_gsm_power_mode());
    }
    else if (get_logger_version() == 5)
    {
      // Three transmitter
      turn_ON_GSM(get_gsm_power_mode());
      send_rain_data(0);
      disable_watchdog();
      receive_lora_data(5);
      disable_watchdog();
      turn_OFF_GSM(get_gsm_power_mode());
    }
    else if (get_logger_version() == 6)
    {
      //default arabica LoRa transmitter
      get_Due_Data(6, get_serverNum_from_flashMem());
      disable_watchdog();
    }
    else if (get_logger_version() == 7)
    {
      // Sends rain gauge data via LoRa
      get_Due_Data(0, get_serverNum_from_flashMem());
      disable_watchdog();
      delay_millis(1000);
      send_rain_data(1);
      disable_watchdog();
    }
    else if (get_logger_version() == 8)
    {
      // Sends rain gauge data via LoRa
      get_Due_Data(0, get_serverNum_from_flashMem());
      disable_watchdog();
      delay_millis(1000);
      send_thru_lora(dataToSend);
      send_rain_data(1);
      disable_watchdog();
    }
    else if (get_logger_version() == 9)
    {
      Serial.print("Datalogger verion: ");
      Serial.println(get_logger_version());
      /*
      // Sends IMU sensor data to GSM
      on_IMU();
      turn_ON_GSM(get_gsm_power_mode());
      send_rain_data(0);
      disable_watchdog();
      delay_millis(1000);
      send_thru_gsm(read_IMU_data(get_calib_param()), get_serverNum_from_flashMem());
      delay_millis(1000);
      turn_OFF_GSM(get_gsm_power_mode());
      off_IMU();*/
    }
    else if (get_logger_version() == 10)
    {
      Serial.print("Datalogger verion: ");
      Serial.println(get_logger_version());
      /*
      // Sends IMU sensor data to LoRa
      // send_thru_gsm(read_IMU_data(),get_serverNum_from_flashMem());
      on_IMU();
      send_thru_lora(read_IMU_data(get_calib_param()));
      delay_millis(1000);
      send_rain_data(1);
      disable_watchdog();
      off_IMU();*/
    }
    else if (get_logger_version() == 11)
    {
      Serial.print("Datalogger verion: ");
      Serial.println(get_logger_version());
      // Sends rain gauge data ONLY
      turn_ON_GSM(get_gsm_power_mode());
      send_rain_data(0);
      disable_watchdog();
      delay_millis(1000);
      turn_OFF_GSM(get_gsm_power_mode());
    }
    else
    {
      Serial.print("Datalogger verion: ");
      Serial.println(get_logger_version());
    }
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "B")
  {
    Serial.print("RTC temperature: ");
    Serial.println(readTemp());
    // Serial.println(readTempRTC());
    // Serial.println(BatteryVoltage(get_logger_version()));
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "C")
  {
    printMenu();
  }
  else if (command == "D")
  {
    Serial.print("Alarm stored: ");
    Serial.println(alarmFromFlashMem());
    print_rtcInterval();
    if (isChangeParam())
      setAlarmInterval();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
    Serial.readStringUntil('\r\n');
  }
  else if (command == "E")
  {
    Serial.println("Exiting debug mode!");
    //real time clock alarm settings
    setAlarmEvery30(alarmFromFlashMem());
    delay_millis(75);
    rtc.clearINTStatus(); // needed to re-trigger rtc
    turn_OFF_GSM(get_gsm_power_mode());
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
    debug_flag = 0;
  }
  else if (command == "F")
  {
    Serial.print("Server Number: ");
    Serial.println(get_serverNum_from_flashMem());
    Serial.println("Default server numbers: GLOBE1 - 639175972526 ; SMART1 - 639088125642");
    Serial.println("Default server numbers: GLOBE2 - 639175388301 ; SMART2 - 639088125639");
    if (isChangeParam())
      changeServerNumber();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
    Serial.readStringUntil('\r\n');
  }
  else if (command == "G")
  {
    //print voltage
    Serial.print("Voltage: ");
    Serial.println(readBatteryVoltage(get_logger_version()));

    if (get_logger_version() == 6)
    {
      send_thru_lora(read_batt_vol(get_logger_version()));
    }
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "H")
  { /*
    on_IMU();
    delay_millis(1000);
    read_IMU_data(get_calib_param()); //print IMU sensor Datalogger
    delay_millis(1000);
    off_IMU();*/
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "I")
  {
    Serial.println("GSM receive test!");
    flashLed(LED_BUILTIN, 3, 50);

    Serial.println("AT + CNMI");
    if (get_gsm_power_mode() == 1)
    {
      Serial.println("1st AT + CNMI");
      GSMSerial.write("AT+CNMI=1,2,0,0,0\r");
      delay_millis(100);
    }
    GSMSerial.write("AT+CNMI=1,2,0,0,0\r");
    delay_millis(300);
    Serial.println("after CNMI");
    while (GSMSerial.available() > 0)
    {
      processIncomingByte(GSMSerial.read(), 0);
    }

    // Serial.println("delete sms");
    // gsmDeleteReadSmsInbox();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "J")
  {
    Serial.print("Logger version: ");
    Serial.println(get_logger_version());
    printLoggerVersion();
    if (isChangeParam())
      setLoggerVersion();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
    Serial.readStringUntil('\r\n');
  }
  else if (command == "K")
  {
    Serial.print("Password: ");
    Serial.println(get_password_from_flashMem());
    if (isChangeParam())
      changePassword();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
    Serial.readStringUntil('\r\n');
  }
  else if (command == "L")
  {
    manualGSMcmd();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "M")
  {
    disable_watchdog();
    send_rain_data(1);
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "N")
  {
    if (get_logger_version() == 1)
    {
      Serial.print("Gateway sensor name: ");
      Serial.println(get_logger_A_from_flashMem());
      Serial.print("Remote sensor name: ");
      Serial.println(get_logger_B_from_flashMem());
    }
    else if (get_logger_version() == 3)
    {
      Serial.print("Gateway name: ");
      Serial.println(get_logger_A_from_flashMem());
      Serial.print("Sensor Name A: ");
      Serial.println(get_logger_B_from_flashMem());
    }
    else if (get_logger_version() == 4)
    {
      Serial.print("Gateway name: ");
      Serial.println(get_logger_A_from_flashMem());
      Serial.print("Sensor Name A: ");
      Serial.println(get_logger_B_from_flashMem());
      Serial.print("Sensor Name B: ");
      Serial.println(get_logger_C_from_flashMem());
    }
    else if (get_logger_version() == 5)
    {
      Serial.print("Gateway name: ");
      Serial.println(get_logger_A_from_flashMem());
      Serial.print("Sensor Name A: ");
      Serial.println(get_logger_B_from_flashMem());
      Serial.print("Sensor Name B: ");
      Serial.println(get_logger_C_from_flashMem());
      Serial.print("Sensor Name C: ");
      Serial.println(get_logger_D_from_flashMem());
    }
    else
    { // 2; 6; 7
      Serial.print("Gateway sensor name: ");
      Serial.println(get_logger_A_from_flashMem());
      // Serial.print("Remote sensor name: ");
      // Serial.println(get_logger_B_from_flashMem());
    }
    if (isChangeParam())
      inputLoggerNames();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
    Serial.readStringUntil('\r\n');
  }
  else if (command == "O")
  {
    Serial.print("CSQ: ");
    Serial.println(readCSQ());
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "P")
  {
    Serial.print("Rain tips: ");
    if (get_rainCollector_type() == 0)
    {
      Serial.println(rainTips * 2.0);
    }
    else if (get_rainCollector_type() == 1)
    {
      Serial.println(rainTips);
    }
    else
    {
      Serial.println(rainTips);
    }
    delay_millis(20);
    resetRainTips();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "Q")
  {
    Serial.print("IMU Parameter: ");
    Serial.println(get_calib_param());
    Serial.println("[0] Raw IMU data");
    Serial.println("[1] Calibrated IMU data");
    if (isChangeParam())
      setIMUdataRawCalib();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
    Serial.readStringUntil('\r\n');
  }
  else if (command == "R")
  {
    resetGSM();
    gsm_network_connect();
    init_gsm();
    Serial.print("CSQ: ");
    Serial.println(readCSQ());
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "S")
  {
    readTimeStamp();
    Serial.print("Timestamp: ");
    Serial.println(Ctimestamp);
    if (isChangeParam())
      setupTime();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
    Serial.readStringUntil('\r\n');
  }
  else if (command == "T")
  {
    Serial.print("Rain collector type: ");
    Serial.println(get_rainCollector_type());
    Serial.println("[0] Pronamic Rain Collector (0.5mm/tip)");
    Serial.println("[1] DAVIS Rain Collector (0.2mm/tip)");
    Serial.println("[2] Generic Rain Collector (1.0/tip)");
    if (isChangeParam())
      setRainCollectorType();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
    Serial.readStringUntil('\r\n');
    /*
    Serial.println("MADTB*VOLT:12.33*200214111000");
    char toParse[200];
    Serial.setTimeout(15000);
    Serial.println("Insert data to be parsed: ");
    String fromUser = Serial.readStringUntil('\n');
    fromUser.toCharArray(toParse, 200);
    // Serial.print("Parsed data: ");
    // Serial.println(parse_voltage(toParse));
    // strncpy(txVoltage, parse_voltage(toParse), sizeof(parse_voltage(toParse)));
    parse_voltage(toParse).toCharArray(txVoltage, sizeof(txVoltage));
    Serial.print("Volt: ");
    Serial.println(txVoltage);
    Serial.print("Volt: ");
    Serial.println(txVoltageB);
    Serial.print("Volt: ");
    Serial.println(txVoltageC);
    get_rssi(5);
    Serial.println("* * * * * * * * * * * * * * * * * * * *");
    */
  }
  else if (command == "U")
  {
    Serial.println("sending rain gauge data . . .");
    if (get_logger_version() == 6)
    {
      //LoRa
      send_rain_data(1);
    }
    else
    {
      //GSM
      send_rain_data(0);
    }
    disable_watchdog();
    // wakeGSM();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "V")
  {
    // turn_OFF_GSM(get_gsm_power_mode());
    turn_ON_GSM(get_gsm_power_mode());
    disable_watchdog();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "W")
  {
    Serial.print("Current gsm power mode: ");
    Serial.println(get_gsm_power_mode());
    Serial.println("[0] Hardware power ON or OFF");
    Serial.println("[1] Sleep and wake via AT commands");
    Serial.println("[2] GSM power always ON");
    if (isChangeParam())
      setGsmPowerMode();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
    Serial.readStringUntil('\r\n');
  }
  else if (command == "X")
  {
    Serial.print("Datalogger Version: ");
    Serial.println(get_logger_version());
    receive_lora_data(get_logger_version());
    disable_watchdog();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "Y")
  {
    unsigned long startHere = millis();
    char specialMsg[200];
    Serial.print("Enter message to send: ");
    while (!Serial.available())
    {
      if (timeOutExit(startHere, DEBUGTIMEOUT))
      {
        debug_flag_exit = true;
        break;
      }
    }
    if (Serial.available())
    {
      String gsmCmd = Serial.readStringUntil('\n');
      Serial.println(gsmCmd);
      gsmCmd.toCharArray(specialMsg, sizeof(specialMsg));
      GSMSerial.write("AT\r");
      delay_millis(300);
      send_thru_gsm(specialMsg, get_serverNum_from_flashMem());
    }
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else if (command == "Z")
  {
    Serial.print("Current command: ");
    sensCommand = passCommand.read();
    Serial.println(sensCommand.senslopeCommand);
    if (isChangeParam())
      changeSensCommand();
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
    Serial.readStringUntil('\r\n');
  }
  else if (command == "?")
  {
    Serial.println("**Printing Stored Parameters**");
    readTimeStamp();
    Serial.print("Server number:\t");
    Serial.println(get_serverNum_from_flashMem());
    Serial.print("Gsm power mode:\t");
    Serial.println(get_gsm_power_mode());
    Serial.print("Sending time:\t");
    Serial.println(alarmFromFlashMem());
    Serial.print("Logger version:\t");
    Serial.println(get_logger_version());
    Serial.print("Logger name:\t");
    Serial.println(get_logger_A_from_flashMem());
    Serial.print("Sensor command:\t");
    sensCommand = passCommand.read();
    Serial.println(sensCommand.senslopeCommand);
    Serial.print("MCU password:\t");
    Serial.println(get_password_from_flashMem());
    Serial.print("Rain collector type:\t");
    Serial.println(get_rainCollector_type());
    Serial.print("Real time clock:\t");
    Serial.println(Ctimestamp);
    Serial.println(F("* * * * * * * * * * * * * * * * * * * *"));
  }
  else
  {
    Serial.println(" ");
    Serial.println("Command invalid!");
    Serial.println(" ");
  }
}

void printMenu()
{
  Serial.println(F(" "));
  Serial.print(F("Firmware Version: "));
  Serial.println(F(firmwareVersion));
  Serial.println(F("****************************************"));
  Serial.println(F("[A]\tSample sensor data"));
  Serial.println(F("[B]\tRead RTC temperature."));
  Serial.println(F("[C]\tPrint this menu"));
  Serial.println(F("[D]\tChange sending time."));
  Serial.println(F("[E]\tExit Debug mode."));
  Serial.println(F("[F]\tChange SERVER NUMBER"));
  Serial.println(F("[G]\tPrint input voltage"));
  Serial.println(F("[H]\tTest send IMU sensor data"));
  Serial.println(F("[I]\tGSM receive SMS test"));
  Serial.println(F("[J]\tChange LOGGER VERSION"));
  Serial.println(F("[K]\tChange MCU PASSWORD"));
  Serial.println(F("[L]\tManual GSM commands"));
  Serial.println(F("[M]\tSpecial sending rain data via LoRa"));
  Serial.println(F("[N]\tChange DATALOGGER NAMES"));
  Serial.println(F("[O]\tRead GSM CSQ."));
  Serial.println(F("[P]\tRead rain gauge tip."));
  Serial.println(F("[Q]\tChange IMU data parameters"));
  Serial.println(F("[R]\tReset GSM"));
  Serial.println(F("[S]\tSet date and time."));
  Serial.println(F("[T]\tSet rain collector type."));
  Serial.println(F("[U]\tSend rain tips."));
  Serial.println(F("[V]\tTurn ON GSM"));
  Serial.println(F("[W]\tSet GSM POWER MODE"));
  Serial.println(F("[X]\tWait for LoRa data"));
  Serial.println(F("[Y]\tSend CUSTOM SMS"));
  Serial.println(F("[Z]\tChange SENSLOPE command."));
  Serial.println(F("[?]\tPrint stored config parameters."));
  Serial.println(F("****************************************"));
  Serial.println(F(" "));
}

void print_rtcInterval()
{
  // Serial.println("------------------------------------------------");
  Serial.println(F("[0]\tAlarm for every 0 and 30 minutes interval"));
  Serial.println(F("[1]\tAlarm for every 5 and 35 minutes interval"));
  Serial.println(F("[2]\tAlarm for every 10 and 40 minutes interval"));
  Serial.println(F("[3]\tAlarm for every 15 and 45 minutes interval"));
  Serial.println(F("[4]\tAlarm for every 10 minutes interval"));
  Serial.println(F("[5]\tAlarm for every 5,15,25. . .  minutes interval"));
  // Serial.println("------------------------------------------------");
}

void setRainCollectorType()
{
  unsigned long startHere = millis();
  int _rainCollector;
  Serial.print("Enter rain collector type: ");
  while (!Serial.available())
  {
    if (timeOutExit(startHere, DEBUGTIMEOUT))
    {
      debug_flag_exit = true;
      break;
    }
  }
  if (Serial.available())
  {
    _rainCollector = Serial.parseInt();
    Serial.print("Rain Collector type = ");
    Serial.println(_rainCollector);
    delay_millis(50);
    rainCollectorType.write(_rainCollector);
  }
}

uint8_t get_rainCollector_type()
{
  int _rainType = rainCollectorType.read();
  return _rainType;
}

void setIMUdataRawCalib()
{
  unsigned long startHere = millis();
  int raw_calib;
  Serial.print("Enter IMU data mode: ");
  while (!Serial.available())
  {
    if (timeOutExit(startHere, DEBUGTIMEOUT))
    {
      debug_flag_exit = true;
      break;
    }
  }
  if (Serial.available())
  {
    raw_calib = Serial.parseInt();
    Serial.print("IMU mode = ");
    Serial.println(raw_calib);
    delay_millis(50);
    imuRawCalib.write(raw_calib);
  }
}

uint8_t get_calib_param()
{
  int param = imuRawCalib.read();
  return param;
}

void setLoggerVersion()
{
  int version;
  unsigned long startHere = millis();
  Serial.print("Enter datalogger version: ");
  while (!Serial.available())
  {
    if (timeOutExit(startHere, DEBUGTIMEOUT))
    {
      debug_flag_exit = true;
      break;
    }
  }
  if (Serial.available())
  {
    version = Serial.parseInt();
    Serial.println(version);
    delay_millis(50);
    loggerVersion.write(version);
  }
}

void printLoggerVersion()
{
  Serial.println(F("[0]\tSending sensor data using GSM only (arQ like function)")); //arQ like function only
  Serial.println(F("[1]\tVersion 5 GSM with LoRa tx (arQ + LoRa RX)"));             //arQ + LoRa rx
  Serial.println(F("[2]\tLoRa transmitter for version 5 datalogger"));              //TX LoRa
  Serial.println(F("[3]\tGateway Mode with only ONE LoRa transmitter"));
  Serial.println(F("[4]\tGateway Mode with TWO LoRa transmitter"));
  Serial.println(F("[5]\tGateway Mode with THREE LoRa transmitter"));
  Serial.println(F("[6]\tLoRa transmitter for Raspberry Pi"));         //TX LoRa
  Serial.println(F("[7]\tSends sensor and rain gauge data via LoRa")); //TX LoRa
  Serial.println(F("[8]\tLoRa dummy transmitter (for testing only)")); //TX LoRa
  Serial.println(F("[9]\tSurficial Tilt - GSM mode"));
  Serial.println(F("[10]\tSurficial Tilt - LoRa TX"));
  Serial.println(F("[11]\tRain gauge sensor only - GSM"));
}

uint8_t get_logger_version()
{
  int lversion = loggerVersion.read();
  return lversion;
}

void setGsmPowerMode()
{
  int gsm_power;
  unsigned long startHere = millis();
  Serial.print("Enter GSM mode setting: ");
  while (!Serial.available())
  {
    if (timeOutExit(startHere, DEBUGTIMEOUT))
    {
      debug_flag_exit = true;
      break;
    }
  }
  if (Serial.available())
  {
    Serial.setTimeout(8000);
    gsm_power = Serial.parseInt();
    Serial.println(gsm_power);
    delay_millis(50);
    gsmPower.write(gsm_power);
  }
}

uint8_t get_gsm_power_mode()
{
  int gsm_mode = gsmPower.read();
  return gsm_mode;
}

void setAlarmInterval()
{
  unsigned long startHere = millis();
  int alarmSelect;
  Serial.print("Enter alarm settings: ");

  while (!Serial.available())
  {
    if (timeOutExit(startHere, DEBUGTIMEOUT))
    {
      debug_flag_exit = true;
      break;
    }
  }
  if (Serial.available())
  {
    alarmSelect = Serial.parseInt();
    Serial.println(alarmSelect);
    delay_millis(50);
    alarmStorage.write(alarmSelect);
  }
}

uint8_t alarmFromFlashMem()
{
  int fromAlarmStorage;
  fromAlarmStorage = alarmStorage.read();
  return fromAlarmStorage;
}

void changeSensCommand()
{
  unsigned long startHere = millis();
  Serial.print("Insert DUE command: ");
  while (!Serial.available())
  {
    if (timeOutExit(startHere, DEBUGTIMEOUT))
    {
      debug_flag_exit = true;
      break;
    }
  }
  if (Serial.available())
  {
    String dynaCommand = Serial.readStringUntil('\n');
    Serial.println(dynaCommand);
    dynaCommand.toCharArray(sensCommand.senslopeCommand, 50);
    passCommand.write(sensCommand); //save to flash memory
  }
}

void inputLoggerNames()
{
  unsigned long startHere = millis();
  Serial.setTimeout(20000);
  if (get_logger_version() == 1)
  {
    Serial.print("Input name of gateway SENSOR: ");
    while (!Serial.available())
    {
      if (timeOutExit(startHere, DEBUGTIMEOUT))
      {
        debug_flag_exit = true;
        break;
      }
    }
    if (Serial.available())
    {
      String inputLoggerA = Serial.readStringUntil('\n');
      Serial.println(inputLoggerA);
      Serial.print("Input name of remote SENSOR: ");
      String inputLoggerB = Serial.readStringUntil('\n');
      Serial.println(inputLoggerB);
      inputLoggerA.toCharArray(loggerName.sensorA, 10);
      inputLoggerB.toCharArray(loggerName.sensorB, 10);
      flashLoggerName.write(loggerName);
    }
  }
  else if (get_logger_version() == 3)
  {
    Serial.print("Input name of GATEWAY: ");
    while (!Serial.available())
    {
      if (timeOutExit(startHere, DEBUGTIMEOUT))
      {
        debug_flag_exit = true;
        break;
      }
    }
    if (Serial.available())
    {
      String inputLoggerA = Serial.readStringUntil('\n');
      Serial.println(inputLoggerA);
      Serial.print("Input name of remote SENSOR: ");
      String inputLoggerB = Serial.readStringUntil('\n');
      Serial.println(inputLoggerB);
      inputLoggerA.toCharArray(loggerName.sensorA, 10);
      inputLoggerB.toCharArray(loggerName.sensorB, 10);
      flashLoggerName.write(loggerName);
    }
  }
  else if (get_logger_version() == 4)
  {
    Serial.print("Input name of GATEWAY: ");
    while (!Serial.available())
    {
      if (timeOutExit(startHere, DEBUGTIMEOUT))
      {
        debug_flag_exit = true;
        break;
      }
    }
    if (Serial.available())
    {
      String inputLoggerA = Serial.readStringUntil('\n');
      Serial.println(inputLoggerA);
      Serial.print("Input name of remote SENSOR A: ");
      String inputLoggerB = Serial.readStringUntil('\n');
      Serial.println(inputLoggerB);
      Serial.print("Input name of remote SENSOR B: ");
      String inputLoggerC = Serial.readStringUntil('\n');
      Serial.println(inputLoggerC);
      inputLoggerA.toCharArray(loggerName.sensorA, 10);
      inputLoggerB.toCharArray(loggerName.sensorB, 10);
      inputLoggerC.toCharArray(loggerName.sensorC, 10);
      flashLoggerName.write(loggerName);
    }
  }
  else if (get_logger_version() == 5)
  {
    Serial.print("Input name of GATEWAY: ");
    while (!Serial.available())
    {
      if (timeOutExit(startHere, DEBUGTIMEOUT))
      {
        debug_flag_exit = true;
        break;
      }
    }
    if (Serial.available())
    {
      String inputLoggerA = Serial.readStringUntil('\n');
      Serial.println(inputLoggerA);
      Serial.print("Input name of remote SENSOR A: ");
      String inputLoggerB = Serial.readStringUntil('\n');
      Serial.println(inputLoggerB);
      Serial.print("Input name of remote SENSOR B: ");
      String inputLoggerC = Serial.readStringUntil('\n');
      Serial.println(inputLoggerC);
      Serial.print("Input name of remote SENSOR C: ");
      String inputLoggerD = Serial.readStringUntil('\n');
      Serial.println(inputLoggerD);
      inputLoggerA.toCharArray(loggerName.sensorA, 10);
      inputLoggerB.toCharArray(loggerName.sensorB, 10);
      inputLoggerC.toCharArray(loggerName.sensorC, 10);
      inputLoggerD.toCharArray(loggerName.sensorD, 10);
      flashLoggerName.write(loggerName);
    }
  }
  else
  {
    /*
    Serial.print("Input name of gateway SENSOR: ");
    String inputLoggerA = Serial.readStringUntil('\n');
    Serial.println(inputLoggerA);
    // Serial.print("Input name of SENSOR B: ");
    // String inputLoggerB = Serial.readStringUntil('\n');
    // Serial.println(inputLoggerB);
    // Serial.print("Input name of SENSOR C: ");
    // String inputLoggerC = Serial.readStringUntil('\n');
    // Serial.println(inputLoggerC);
    // Serial.print("Input name of SENSOR D: ");
    // String inputLoggerD = Serial.readStringUntil('\n');
    // Serial.println(inputLoggerD);
    inputLoggerA.toCharArray(loggerName.sensorA, 10);
    // inputLoggerB.toCharArray(loggerName.sensorB, 10);
    // inputLoggerC.toCharArray(loggerName.sensorC, 10);
    // inputLoggerD.toCharArray(loggerName.sensorD, 10);
    flashLoggerName.write(loggerName);
    */
    Serial.print("Input name of SENSOR: ");
    while (!Serial.available())
    {
      if (timeOutExit(startHere, DEBUGTIMEOUT))
      {
        debug_flag_exit = true;
        break;
      }
    }
    if (Serial.available())
    {
      String inputLoggerA = Serial.readStringUntil('\n');
      Serial.println(inputLoggerA);
      inputLoggerA.toCharArray(loggerName.sensorA, 10);
      flashLoggerName.write(loggerName);
    }
  }
}

void changeServerNumber()
{
  unsigned long startHere = millis();
  // Serial.println("Insert server number GLOBE - 639175972526 ; SMART - 639088125642");
  Serial.print("Enter new server number: ");
  while (!Serial.available())
  {
    if (timeOutExit(startHere, DEBUGTIMEOUT))
    {
      debug_flag_exit = true;
      break;
    }
  }
  if (Serial.available())
  {
    String ser_num = Serial.readStringUntil('\n');
    Serial.println(ser_num);
    ser_num.toCharArray(flashServerNumber.inputNumber, 50);
    newServerNum.write(flashServerNumber); //save to flash memory
  }
}

void changePassword()
{
  unsigned long startHere = millis();
  Serial.print("Enter MCU password: ");
  while (!Serial.available())
  {
    if (timeOutExit(startHere, DEBUGTIMEOUT))
    {
      debug_flag_exit = true;
      break;
    }
  }
  if (Serial.available())
  {
    String inPass = Serial.readStringUntil('\n');
    // inPass += ",";
    Serial.println(inPass);
    inPass.toCharArray(flashPassword.keyword, 50);
    flashPasswordIn.write(flashPassword);
  }
}

void setupTime()
{
  unsigned long startHere = millis();
  int MM = 0, DD = 0, YY = 0, hh = 0, mm = 0, ss = 0, dd = 0;
  Serial.println(F("\nSet time and date in this format: YYYY,MM,DD,hh,mm,ss,dd[0-6]Mon-Sun"));
  // delay(10);
  while (!Serial.available())
  {
    if (timeOutExit(startHere, DEBUGTIMEOUT))
    {
      debug_flag_exit = true;
      break;
    }
  }
  if (Serial.available())
  {
    YY = Serial.parseInt();
    MM = Serial.parseInt();
    DD = Serial.parseInt();
    hh = Serial.parseInt();
    mm = Serial.parseInt();
    ss = Serial.parseInt();
    dd = Serial.parseInt();
    delay_millis(10);
    adjustDate(YY, MM, DD, hh, mm, ss, dd);
    readTimeStamp();
    Serial.print("Current timestamp: ");
    Serial.println(Ctimestamp);
  }
}

void adjustDate(int year, int month, int date, int hour, int min, int sec, int weekday)
{
  DateTime dt(year, month, date, hour, min, sec, weekday);
  rtc.setDateTime(dt); // adjust date-time as defined by 'dt'
                       // Serial.println(rtc.now().getEpoch());	//debug info
  //char weekDay[][4] = {"Sun"-0, "Mon"-1, "Tue"-2, "Wed"-3, "Thu"-4, "Fri"-5, "Sat"-6 };
}

float readTemp()
{
  float temp;
  rtc.convertTemperature();
  temp = rtc.getTemperature();
  return temp;
}

char new_temp[15];
char *readTempRTC()
{
  char tmp[10];
  rtc.convertTemperature();
  float temp = rtc.getTemperature();
  snprintf(tmp, sizeof tmp, "%.2f", temp);
  strncpy(new_temp, tmp, sizeof(tmp));
  return new_temp;
}

void readTimeStamp()
{
  DateTime now = rtc.now(); //get the current date-time
  String ts = String(now.year());

  if (now.month() <= 9)
  {
    ts += "0" + String(now.month());
  }
  else
  {
    ts += String(now.month());
  }

  if (now.date() <= 9)
  {
    ts += "0" + String(now.date());
  }
  else
  {
    ts += String(now.date());
  }

  if (now.hour() <= 9)
  {
    ts += "0" + String(now.hour());
  }
  else
  {
    ts += String(now.hour());
  }

  if (now.minute() <= 9)
  {
    ts += "0" + String(now.minute());
  }
  else
  {
    ts += String(now.minute());
  }

  if (now.second() <= 9)
  {
    ts += "0" + String(now.second());
  }
  else
  {
    ts += String(now.second());
  }

  ts.remove(0, 2); //remove 1st 2 data in ts
  ts.toCharArray(Ctimestamp, 13);
}

char *readDateTime()
{
  char storeDt[50];
  storeDt[0] = '\0';

  DateTime now = rtc.now(); //get the current date-time
  String ts = String(now.year());

  if (now.month() <= 9)
  {
    ts += "0" + String(now.month());
  }
  else
  {
    ts += String(now.month());
  }

  if (now.date() <= 9)
  {
    ts += "0" + String(now.date());
  }
  else
  {
    ts += String(now.date());
  }

  if (now.hour() <= 9)
  {
    ts += "0" + String(now.hour());
  }
  else
  {
    ts += String(now.hour());
  }

  if (now.minute() <= 9)
  {
    ts += "0" + String(now.minute());
  }
  else
  {
    ts += String(now.minute());
  }

  if (now.second() <= 9)
  {
    ts += "0" + String(now.second());
  }
  else
  {
    ts += String(now.second());
  }

  ts.remove(0, 2); //remove 1st 2 data in ts
  // ts.toCharArray(Ctimestamp, 13);
  ts.toCharArray(storeDt, 13);

  return storeDt;
}

//default every 10 minutes interval
void setAlarm()
{
  DateTime now = rtc.now(); //get the current date-time

  if ((now.minute() >= 0) && (now.minute() <= 9))
  {
    store_rtc = 10;
  }
  else if ((now.minute() >= 10) && (now.minute() <= 19))
  {
    store_rtc = 20;
  }
  else if ((now.minute() >= 20) && (now.minute() <= 29))
  {
    store_rtc = 30;
  }
  else if ((now.minute() >= 30) && (now.minute() <= 39))
  {
    store_rtc = 40;
  }
  else if ((now.minute() >= 40) && (now.minute() <= 49))
  {
    store_rtc = 50;
  }
  else if ((now.minute() >= 50) && (now.minute() <= 59))
  {
    store_rtc = 0;
  }
  rtc.enableInterrupts(store_rtc, 00); // interrupt at (m,s)
  if (DEBUG == 1)
  {
    Serial.print("Next alarm: ");
  }
  if (DEBUG == 1)
  {
    Serial.println(store_rtc);
  }
}

//30 minutes interval
void setAlarmEvery30(int alarmSET)
{
  DateTime now = rtc.now(); //get the current date-time
  switch (alarmSET)
  {
  case 0:
  {
    // Serial.println("set 0/30 . . .");
    if ((now.minute() >= 0) && (now.minute() <= 29))
    {
      store_rtc = 30;
    }
    else if ((now.minute() >= 30) && (now.minute() <= 59))
    {
      store_rtc = 0;
    }
    enable_rtc_interrupt();
    break;
  }
  case 1:
  {
    // Serial.println("set 5/35 . . .");
    if ((now.minute() >= 0) && (now.minute() <= 34))
    {
      store_rtc = 35;
    }
    else if ((now.minute() >= 35) && (now.minute() <= 59))
    {
      store_rtc = 5;
    }
    enable_rtc_interrupt();
    break;
  }
  case 2:
  {
    // Serial.println("set 10/40 . . .");
    if ((now.minute() >= 0) && (now.minute() <= 39))
    {
      store_rtc = 40;
    }
    else if ((now.minute() >= 40) && (now.minute() <= 59))
    {
      store_rtc = 10;
    }
    enable_rtc_interrupt();
    break;
  }
  case 3:
  {
    // Serial.println("set 15/45 . . .");
    if ((now.minute() >= 0) && (now.minute() <= 44))
    {
      store_rtc = 45;
    }
    else if ((now.minute() >= 45) && (now.minute() <= 59))
    {
      store_rtc = 15;
    }
    enable_rtc_interrupt();
    break;
  }
  case 4:
  {
    //set every 10 minutes interval
    if ((now.minute() >= 0) && (now.minute() <= 9))
    {
      store_rtc = 10;
    }
    else if ((now.minute() >= 10) && (now.minute() <= 19))
    {
      store_rtc = 20;
    }
    else if ((now.minute() >= 20) && (now.minute() <= 29))
    {
      store_rtc = 30;
    }
    else if ((now.minute() >= 30) && (now.minute() <= 39))
    {
      store_rtc = 40;
    }
    else if ((now.minute() >= 40) && (now.minute() <= 49))
    {
      store_rtc = 50;
    }
    else if ((now.minute() >= 50) && (now.minute() <= 59))
    {
      store_rtc = 0;
    }
    enable_rtc_interrupt();
    break;
  }
  case 5:
  {
    //set every 15 minutes interval
    if ((now.minute() >= 5) && (now.minute() <= 14))
    {
      store_rtc = 15;
    }
    else if ((now.minute() >= 15) && (now.minute() <= 24))
    {
      store_rtc = 25;
    }
    else if ((now.minute() >= 25) && (now.minute() <= 34))
    {
      store_rtc = 35;
    }
    else if ((now.minute() >= 35) && (now.minute() <= 44))
    {
      store_rtc = 45;
    }
    else if ((now.minute() >= 45) && (now.minute() <= 54))
    {
      store_rtc = 55;
    }
    else if ((now.minute() >= 55) && (now.minute() <= 4))
    {
      store_rtc = 5;
    }
    enable_rtc_interrupt();
    break;
  }
  case 6:
  {
    //set every 10 minutes interval
    if ((now.minute() >= 0) && (now.minute() <= 9))
    {
      store_rtc = 10;
    }
    else if ((now.minute() >= 10) && (now.minute() <= 19))
    {
      store_rtc = 20;
    }
    else if ((now.minute() >= 20) && (now.minute() <= 29))
    {
      store_rtc = 30;
    }
    else if ((now.minute() >= 30) && (now.minute() <= 39))
    {
      store_rtc = 40;
    }
    else if ((now.minute() >= 40) && (now.minute() <= 49))
    {
      store_rtc = 50;
    }
    else if ((now.minute() >= 50) && (now.minute() <= 59))
    {
      store_rtc = 0;
    }
    enable_rtc_interrupt();
    break;
  }
  }
}

void enable_rtc_interrupt()
{
  rtc.enableInterrupts(store_rtc, 00); // interrupt at (minutes, seconds)
  if (DEBUG == 1)
  {
    Serial.print("Next alarm: ");
  }
  if (DEBUG == 1)
  {
    Serial.println(store_rtc);
  }
  readTimeStamp();
}

bool isChangeParam()
{
  String serial_line;
  unsigned long serStart = millis();
  unsigned long startHere = millis();
  bool timeToExit;
  Serial.println(" ");
  Serial.println("Enter C to change:");

  do
  {
    timeToExit = timeOutExit(startHere, DEBUGTIMEOUT);
    serial_line = Serial.readStringUntil('\r\n');
  } while (serial_line == "" && timeToExit == false);
  // } while ((serial_line == "") && ((millis() - serStart) < DEBUGTIMEOUT));
  serial_line.toUpperCase();
  serial_line.replace("\r", "");

  if (timeToExit)
  {
    Serial.println("Exiting . . .");
    debug_flag_exit = true;
  }

  if (serial_line == "C")
  {
    return true;
  }
  else
  {
    // printMenu();
    Serial.println(" ");
    Serial.println("Change cancelled. Reselect.");
    Serial.println(" ");
    return false;
  }
}

bool timeOutExit(unsigned long _startTimeOut, int _timeOut)
{
  if ((millis() - _startTimeOut) > _timeOut)
  {
    _startTimeOut = millis();
    Serial.println(" ");
    Serial.println("*****************************");
    Serial.print("Timeout reached: ");
    Serial.print(_timeOut / 1000);
    Serial.println(" seconds");
    Serial.println("*****************************");
    Serial.println(" ");
    return true;
  }
  else
  {
    return false;
  }
}
