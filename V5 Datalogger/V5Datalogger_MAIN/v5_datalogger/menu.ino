#define ATCMD "AT"
#define ATECMDTRUE "ATE"
#define ATECMDFALSE "ATE0"
#define OKSTR "OK"
#define ERRORSTR "ERROR"

bool ate = false;

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
    else if (command == "?")  // Print stored config parameters
    {
        Serial.println("**Printing Stored Parameters**");
        readTimeStamp();
        Serial.print("Real time clock: ");
        Serial.println(Ctimestamp);
        Serial.print("Server number:   ");
        Serial.println(get_serverNum_from_flashMem());
        Serial.print("Gsm power mode:  ");
        Serial.println(get_gsm_power_mode());
        Serial.print("Sending time:    ");
        Serial.println(alarmFromFlashMem());
        Serial.print("Logger version:  ");
        Serial.println(get_logger_mode());
        Serial.print("Logger name:     ");
        Serial.println(get_logger_A_from_flashMem());
        Serial.print("Sensor command:  ");
        sensCommand = passCommand.read();
        Serial.println(sensCommand.senslopeCommand);
        Serial.print("MCU password:    ");
        Serial.println(get_password_from_flashMem());
        Serial.print("Rain collector type: ");
        Serial.println(get_rainGauge_type());
        Serial.print("Input Voltage: ");
        Serial.println(readBatteryVoltage(get_calib_param()));
        Serial.print("Battery voltage: ");
        Serial.println(get_calib_param());
        Serial.print("RTC temperature: ");
        Serial.println(readTemp());
        if ((get_logger_mode() == 0 ) || (get_logger_mode() == 1 ) || (get_logger_mode() == 3 ) || (get_logger_mode() == 4 ) ||
            (get_logger_mode() == 5 ) || (get_logger_mode() == 9 ) || (get_logger_mode() == 11 ) || (get_logger_mode() == 12 ))
            {
              Serial.print("CSQ value: ");
              Serial.println(readCSQ());
            }
        Serial.print("");
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
    }
    else if (command == "A")  // Get data from customDue
    {
        /*
        Serial.println("[0] Sending data using GSM only");                     //arQ like function only
        Serial.println("[1] Version 5 datalogger LoRa with GSM (arQ + LoRa)"); //arQ + LoRa rx
        Serial.println("[2] LoRa transmitter for version 5 datalogger");       //TX LoRa
        Serial.println("[3] Gateway Mode with only ONE LoRa transmitter");
        Serial.println("[4] Gateway Mode with TWO LoRa transmitter");
        Serial.println("[5] Gateway Mode with THREE LoRa transmitter");
        Serial.println("[6] LoRa transmitter for Raspberry Pi");         //TX LoRa
        Serial.println("[7] Sends sensor and rain gauge data via LoRa"); //TX LoRa
        Serial.println("[8] LoRa dummy transmitter");                    //TX LoRa
        Serial.println("[9] GSM - Surficial Tilt");
        Serial.println("[10] LoRa Tx - Surficial Tilt");
        Serial.println("[11] Rain gauge mode only");
        Serial.println("[12] Surficial Tilt Gateway - LoRa Rx");
        */
        if (get_logger_mode() == 0)
        {
            // default arQ like sending
            turn_ON_GSM(get_gsm_power_mode());
            send_rain_data(0);
            disable_watchdog();
            get_Due_Data(1, get_serverNum_from_flashMem());
            disable_watchdog();
            turn_OFF_GSM(get_gsm_power_mode());
        }
        else if (get_logger_mode() == 1)
        {
            // arQ + 1 LoRa receiver
            if (gsmPwrStat)
            {
                turn_ON_GSM(get_gsm_power_mode());
            }
            get_Due_Data(1, get_serverNum_from_flashMem());
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
        else if (get_logger_mode() == 2)
        {
            // LoRa transmitter of version 5 datalogger
            get_Due_Data(2, get_serverNum_from_flashMem());
            disable_watchdog();
        }
        else if (get_logger_mode() == 3)
        {
            // only one trasmitter
            turn_ON_GSM(get_gsm_power_mode());
            send_rain_data(0);
            disable_watchdog();
            receive_lora_data(3);
            disable_watchdog();
            turn_OFF_GSM(get_gsm_power_mode());
        }
        else if (get_logger_mode() == 4)
        {
            // Two transmitter
            turn_ON_GSM(get_gsm_power_mode());
            send_rain_data(0);
            disable_watchdog();
            receive_lora_data(4);
            disable_watchdog();
            turn_OFF_GSM(get_gsm_power_mode());
        }
        else if (get_logger_mode() == 5)
        {
            // Three transmitter
            turn_ON_GSM(get_gsm_power_mode());
            send_rain_data(0);
            disable_watchdog();
            receive_lora_data(5);
            disable_watchdog();
            turn_OFF_GSM(get_gsm_power_mode());
        }
        // else if (get_logger_mode() == 6)  
        // {
        //     // default arabica LoRa transmitter
        //     get_Due_Data(6, get_serverNum_from_flashMem());
        //     disable_watchdog();
        // }
        else if (get_logger_mode() == 7)
        {
            // Sends rain gauge data via LoRa
            get_Due_Data(0, get_serverNum_from_flashMem());
            disable_watchdog();
            delay_millis(1000);
            send_rain_data(1);
            disable_watchdog();
        }
        // else if (get_logger_mode() == 8)
        // {
        //     // Sends rain gauge data via LoRa
        //     get_Due_Data(0, get_serverNum_from_flashMem());
        //     disable_watchdog();
        //     delay_millis(1000);
        //     send_thru_lora(dataToSend);
        //     send_rain_data(1);
        //     disable_watchdog();
        // }
      //   else if (get_logger_mode() == 9)
      //   {
      //       Serial.print("Datalogger mode: ");
      //       Serial.println(get_logger_mode());
      //       // Sends IMU sensor data to GSM
      //       /*
      // on_IMU();
      // turn_ON_GSM(get_gsm_power_mode());
      // send_rain_data(0);
      // disable_watchdog();
      // delay_millis(1000);
      // send_thru_gsm(read_IMU_data(get_calib_param()), get_serverNum_from_flashMem());
      // delay_millis(1000);
      // turn_OFF_GSM(get_gsm_power_mode());
      // off_IMU();
      // */
      //   }
        // else if (get_logger_mode() == 10)
        // {
        //     Serial.print("Datalogger verion: ");
        //     Serial.println(get_logger_mode());
        //     // Sends IMU sensor data to LoRa
        //     // send_thru_gsm(read_IMU_data(),get_serverNum_from_flashMem());
        //     //   on_IMU();
        //     //   print_data();
        //     //   read_IMU_data(0);
        //     // send_thru_lora(read_IMU_data(get_calib_param()));
        //     // delay_millis(1000);
        //     // send_rain_data(1);
        //     // LoRa transmitter of version 5 datalogger
        //     disable_watchdog();
        //     get_Due_Data(2, get_serverNum_from_flashMem());
        //     disable_watchdog();
        //     attachInterrupt(RTCINTPIN, wake, FALLING);
        //     disable_watchdog();
        // }
        else if (get_logger_mode() == 11)
        {
            Serial.print("Datalogger verion: ");
            Serial.println(get_logger_mode());
            // Sends rain gauge data ONLY
            turn_ON_GSM(get_gsm_power_mode());
            send_rain_data(0);
            disable_watchdog();
            delay_millis(1000);
            turn_OFF_GSM(get_gsm_power_mode());
        }
        // else if (get_logger_mode() == 12)
        // {
        //     Serial.print("Datalogger verion: ");
        //     Serial.println(get_logger_mode());
        //     // Sends rain gauge data ONLY
        //     disable_watchdog();
        //     // turn_ON_GSM(get_gsm_power_mode());
        //     send_rain_data(0);
        //     delay_millis(2000);
        //     Serial.println("Sending rain data to second number: ");
        //     send_thru_gsm(dataToSend, "639161640761");
        //     // send_thru_gsm(dataToSend, "639175388301");
        //     disable_watchdog();
        //     delay_millis(1000);
        //     // turn_OFF_GSM(get_gsm_power_mode());
        // }
        else if (get_logger_mode() == 13)
        {
            // only one trasmitter
            //  turn_ON_GSM(get_gsm_power_mode());
            //  send_rain_data(0);
            disable_watchdog();
            receive_lora_data_UBLOX(12);
            disable_watchdog();
            // turn_OFF_GSM(get_gsm_power_mode());
        }
        else
        {
            Serial.print("Datalogger verion: ");
            Serial.println(get_logger_mode());
        }
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
    }
    else if (command == "B")  // Read rain gauge tips
    {
        Serial.print("Rain collecor TIPS: ");
        if (get_rainGauge_type() == 0)
        {
            Serial.println(rainTips * 2.0);
        }
        else if (get_rainGauge_type() == 1)
        {
            Serial.println(rainTips);
        }
        else
        {
            Serial.println(rainTips);
        }
        delay_millis(20);
        resetRainTips();
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
    }
    else if (command == "C")  // Print menu
    {
        printMenu();
    }
    else if (command == "D")  // Change logger MODE
    {
        Serial.print("Logger version: ");
        Serial.println(get_logger_mode());
        printLoggerVersion();
        if (isChangeParam())
            setLoggerVersion();
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
        Serial.readStringUntil('\r\n');
    }
    else if (command == "E")  // Set RTC date and time
    {
        readTimeStamp();
        Serial.print("Timestamp: ");
        Serial.println(Ctimestamp);
        if (isChangeParam())
            setupTime();
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
        Serial.readStringUntil('\r\n');
    }
    else if (command == "F")  // Change datalogger NAMES
    {
        if (get_logger_mode() == 1)
        {
            Serial.print("Gateway sensor name: ");
            Serial.println(get_logger_A_from_flashMem());
            Serial.print("Remote sensor name: ");
            Serial.println(get_logger_B_from_flashMem());
        }
        else if (get_logger_mode() == 3)
        {
            Serial.print("Gateway name: ");
            Serial.println(get_logger_A_from_flashMem());
            Serial.print("Sensor Name A: ");
            Serial.println(get_logger_B_from_flashMem());
        }
        else if (get_logger_mode() == 4)
        {
            Serial.print("Gateway name: ");
            Serial.println(get_logger_A_from_flashMem());
            Serial.print("Sensor Name A: ");
            Serial.println(get_logger_B_from_flashMem());
            Serial.print("Sensor Name B: ");
            Serial.println(get_logger_C_from_flashMem());
        }
        else if (get_logger_mode() == 5)
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
        else if (get_logger_mode() == 13)
        {
            Serial.print("Gateway name: ");
            Serial.println(get_logger_A_from_flashMem());
            Serial.print("Sensor Name A: ");
            Serial.println(get_logger_B_from_flashMem());
            Serial.print("Sensor Name B: ");
            Serial.println(get_logger_C_from_flashMem());
            Serial.print("Sensor Name C: ");
            Serial.println(get_logger_D_from_flashMem());

            Serial.print("Sensor Name D: ");
            Serial.println(get_logger_E_from_flashMem());
            Serial.print("Sensor Name E: ");
            Serial.println(get_logger_F_from_flashMem());
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
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
        Serial.readStringUntil('\r\n');
    }
    else if (command == "G")  // Change server number
    {
        Serial.print("Server Number: ");
        Serial.println(get_serverNum_from_flashMem());
        Serial.println("Default server numbers: GLOBE1 - 639175972526 ; SMART1 - 639088125642");
        Serial.println("Default server numbers: GLOBE2 - 639175388301 ; SMART2 - 639088125639");
        if (isChangeParam())
            changeServerNumber();
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
        Serial.readStringUntil('\r\n');
    }
    else if (command == "H")  // Set Network Carrier (Globe / Smart)
    {
        //insert function here        
    }
    else if (command == "I")  // Reset GSM
    {
        resetGSM();
        gsm_network_connect();
        init_gsm();
        Serial.print("CSQ: ");
        Serial.println(readCSQ());
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
    }
    else if (command == "J")  // Set Rain Gauge Type ( Pronamic 0.5mm or Davis 0.2mm )
    {
        Serial.print("Rain gauge type: ");
        Serial.println(get_rainGauge_type());
        Serial.println("[0] Pronamic Rain Gauge (0.5mm/tip)");
        Serial.println("[1] DAVIS (AeroCone) Rain Gauge (0.2mm/tip)");
        Serial.println("[2] Generic Rain Collector (1.0/tip)");
        Serial.println("[3] No Rain Gauge");        
        if (isChangeParam())
            setRainCollectorType();
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
        Serial.readStringUntil('\r\n');
    }
    else if (command == "K")  // Change sending time
    {
        Serial.print("Alarm stored: ");
        Serial.println(alarmFromFlashMem());
        print_rtcInterval();
        if (isChangeParam())
            setAlarmInterval();
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
        Serial.readStringUntil('\r\n');
    }
    else if (command == "L")  // Set battery type  (4.2V Li-ion / 12V lead acid)
    {
        // converted to battery voltage input either 12v or 4.2v
        Serial.print("Battery voltage reference: ");
        Serial.println(get_calib_param());
        Serial.println("[0] 12 volts battery");
        Serial.println("[1] 4.2 volts battery");
        if (isChangeParam())
            setIMUdataRawCalib();
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
        Serial.readStringUntil('\r\n');
    }
    else if (command == "M")  // Send custom SMS
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
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
    }
    else if (command == "N")  // set GSM POWER MODE
    {
        Serial.print("Current gsm power mode: ");
        Serial.println(get_gsm_power_mode());
        Serial.println("[0] Hardware power ON or OFF");
        Serial.println("[1] Sleep and wake via AT commands");
        Serial.println("[2] GSM power always ON");
        if (isChangeParam())
            setGsmPowerMode();
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
        Serial.readStringUntil('\r\n');
    }
    else if (command == "O")  // Manual GSM commands
    {
        manualGSMcmd();
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
    }
    else if (command == "P")  // Set all config to default values
    {
        //insert default setting function here
    }
    else if (command == "EXIT")  // Exit debug mode
    {
        Serial.println("Exiting debug mode!");
        // real time clock alarm settings
        setAlarmEvery30(alarmFromFlashMem());
        delay_millis(75);
        rtc.clearINTStatus(); // needed to re-trigger rtc
        turn_OFF_GSM(get_gsm_power_mode());
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
        debug_flag = 0;
    }
    else if (command == "TEST_WAIT_LORA")
    {
        Serial.print("Datalogger Mode: ");
        Serial.println(get_logger_mode());
        receive_lora_data(get_logger_mode());
        disable_watchdog();
        Serial.println("* * * * * * * * * * * * * * * * * * * *");
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
    Serial.println(F("[?] Print stored config parameters."));
    Serial.println(F("[A] Get data from customDue"));
    Serial.println(F("[B] Read rain gauge tips."));
    Serial.println(F("[C] Print this menu"));
    Serial.println(F("[D] Change LOGGER MODE"));
    Serial.println(F("[E] Set date and time."));
    Serial.println(F("[F] Change DATALOGGER NAMES"));
    Serial.println(F("[G] Change SERVER NUMBER"));    //unfinished
    Serial.println(F("[H] Set Network Carrier ( Globe / Smart )"));
    Serial.println(F("[I] Reset GSM"));
    Serial.println(F("[J] Set rain collector type."));
    Serial.println(F("[K] Change sending time."));
    Serial.println(F("[L] Set battery type (4.2V Li-ion / 12V Lead Acid)"));    // 12V Lead acid is set by default
    Serial.println(F("[M] Send CUSTOM SMS"));
    Serial.println(F("[N] Set GSM POWER MODE"));
    Serial.println(F("[O] Manual GSM commands"));
    Serial.println(F("[P] Set all setting to default values"));     //unfinished
    Serial.println(F("[EXIT] Exit Debug mode."));
    
    // Serial.println(F("[B] Read RTC temperature."));
    // Serial.println(F("[H] Test send IMU sensor data"));
    // Serial.println(F("[I] GSM receive SMS test"));
    // Serial.println(F("[K] Change MCU PASSWORD"));
    // Serial.println(F("[M] Special sending rain data via LoRa"));
    // Serial.println(F("[O] Read GSM CSQ."));
    // Serial.println(F("[U] Send rain tips."));
    // Serial.println(F("[V] Turn ON GSM"));
    // Serial.println(F("[X] Wait for LoRa data"));
    // Serial.println(F("[Z] Change SENSLOPE command."));
    
    Serial.println(F("****************************************"));
    Serial.println(F(" "));
}
// void printMenu()
// {
//     Serial.println(F(" "));
//     Serial.print(F("Firmware Version: "));
//     Serial.println(F(firmwareVersion));
//     Serial.println(F("****************************************"));
//     Serial.println(F("[?] Print stored config parameters."));
//     Serial.println(F("[A] Sample sensor data"));
//     Serial.println(F("[B] Read RTC temperature."));
//     Serial.println(F("[C] Print this menu"));
//     Serial.println(F("[D] Change sending time."));
//     Serial.println(F("[E] Exit Debug mode."));
//     Serial.println(F("[F] Change SERVER NUMBER"));
//     Serial.println(F("[G] Print input voltage"));
//     Serial.println(F("[H] Test send IMU sensor data"));
//     Serial.println(F("[I] GSM receive SMS test"));
//     Serial.println(F("[J] Change LOGGER VERSION"));
//     Serial.println(F("[K] Change MCU PASSWORD"));
//     Serial.println(F("[L] Manual GSM commands"));
//     Serial.println(F("[M] Special sending rain data via LoRa"));
//     Serial.println(F("[N] Change DATALOGGER NAMES"));
//     Serial.println(F("[O] Read GSM CSQ."));
//     Serial.println(F("[P] Read rain gauge tip."));
//     Serial.println(F("[Q] Change battery voltage parameter"));
//     Serial.println(F("[R] Reset GSM"));
//     Serial.println(F("[S] Set date and time."));
//     Serial.println(F("[T] Set rain collector type."));
//     Serial.println(F("[U] Send rain tips."));
//     Serial.println(F("[V] Turn ON GSM"));
//     Serial.println(F("[W] Set GSM POWER MODE"));
//     Serial.println(F("[X] Wait for LoRa data"));
//     Serial.println(F("[Y] Send CUSTOM SMS"));
//     Serial.println(F("[Z] Change SENSLOPE command."));
//     Serial.println(F("****************************************"));
//     Serial.println(F(" "));
// }

void print_rtcInterval()
{
    // Serial.println("------------------------------------------------");
    Serial.println("[0] Alarm for every 0 and 30 minutes interval");
    Serial.println("[1] Alarm for every 5 and 35 minutes interval");
    Serial.println("[2] Alarm for every 10 and 40 minutes interval");
    Serial.println("[3] Alarm for every 15 and 45 minutes interval");
    Serial.println("[4] Alarm for every 10 minutes interval");
    Serial.println("[5] Alarm for every 5,15,25. . .  minutes interval");
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

uint8_t get_rainGauge_type()
{
    int _rainType = rainCollectorType.read();
    return _rainType;
}

void setIMUdataRawCalib()
{
    unsigned long startHere = millis();
    int raw_calib;
    Serial.print("Enter battery mode: ");
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
        Serial.print("Battery mode = ");
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
    Serial.println("[0] arQ-like functions"); // arQ like function only
    Serial.println("[1] Gateway with subsurface sensor and 1 Router");             // arQ + LoRa rx
    Serial.println("[2] Router");              // TX LoRa
    Serial.println("[3] Gateway with 1 Router");
    Serial.println("[4] Gateway with 2 Routers");
    Serial.println("[5] Gateway with 3 Routers");
    Serial.println("[6] Rain gauge only");         // TX LoRa

    
    // Serial.println("[0] Sending sensor data using GSM only (arQ like function)"); // arQ like function only
    // Serial.println("[1] Version 5 GSM with LoRa tx (arQ + LoRa RX)");             // arQ + LoRa rx
    // Serial.println("[2] LoRa transmitter for version 5 datalogger");              // TX LoRa
    // Serial.println("[3] Gateway Mode with only ONE LoRa transmitter");
    // Serial.println("[4] Gateway Mode with TWO LoRa transmitter");
    // Serial.println("[5] Gateway Mode with THREE LoRa transmitter");
    // Serial.println("[6] LoRa transmitter for Raspberry Pi");         // TX LoRa
    // Serial.println("[7] Sends sensor and rain gauge data via LoRa"); // TX LoRa
    // Serial.println("[8] LoRa dummy transmitter (for testing only)"); // TX LoRa
    // Serial.println("[9] Surficial Tilt - GSM mode");
    // Serial.println("[10] LoRa Transmitter with handshake");
    // Serial.println("[11] Rain gauge sensor only - GSM");
    // Serial.println("[12] Rain gauge sensor only: Sending to 2 numbers");
    // Serial.println("[13] Surficial Tilt Gateway - LoRa TX");
    // Serial.println("[14] LoRa Gateway Only");
    // Serial.println("[15] LoRa Gateway with reply (Only name saved in mem is valid)");
}

uint8_t get_logger_mode()
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
        passCommand.write(sensCommand); // save to flash memory
    }
}

void inputLoggerNames()
{
    unsigned long startHere = millis();
    Serial.setTimeout(20000);
    if (get_logger_mode() == 1)
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
    else if (get_logger_mode() == 3)
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
    else if (get_logger_mode() == 4)
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
    else if (get_logger_mode() == 5)
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
    else if (get_logger_mode() == 13)
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

            Serial.print("Input name of remote SENSOR D: ");
            String inputLoggerE = Serial.readStringUntil('\n');
            Serial.println(inputLoggerE);
            Serial.print("Input name of remote SENSOR E: ");
            String inputLoggerF = Serial.readStringUntil('\n');
            Serial.println(inputLoggerF);

            inputLoggerA.toCharArray(loggerName.sensorA, 10);
            inputLoggerB.toCharArray(loggerName.sensorB, 10);
            inputLoggerC.toCharArray(loggerName.sensorC, 10);
            inputLoggerD.toCharArray(loggerName.sensorD, 10);

            inputLoggerE.toCharArray(loggerName.sensorE, 10);
            inputLoggerF.toCharArray(loggerName.sensorF, 10);
            flashLoggerName.write(loggerName);
        }
    }
    else
    {
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
        newServerNum.write(flashServerNumber); // save to flash memory
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

void set_defaults()
{

    Serial.println("Defaults values set");
    Serial.println("* * * * * * * * * * * * * * * * * * * *");
    Serial.readStringUntil('\r\n');
}
// else if (command == "Z")  // Change SENSLOPE Command
//     {
//         Serial.print("Current command: ");
//         sensCommand = passCommand.read();
//         Serial.println(sensCommand.senslopeCommand);
//         if (isChangeParam())
//             changeSensCommand();
//         Serial.println("* * * * * * * * * * * * * * * * * * * *");
//         Serial.readStringUntil('\r\n');
//     }
// void changeSensCommand()
// {
//     unsigned long startHere = millis();
//     Serial.print("Insert DUE command: ");
//     while (!Serial.available())
//     {
//         if (timeOutExit(startHere, DEBUGTIMEOUT))
//         {
//             debug_flag_exit = true;
//             break;
//         }
//     }
//     if (Serial.available())
//     {
//         String dynaCommand = Serial.readStringUntil('\n');
//         Serial.println(dynaCommand);
//         dynaCommand.toCharArray(sensCommand.senslopeCommand, 50);
//         passCommand.write(sensCommand); // save to flash memory
//     }
// }