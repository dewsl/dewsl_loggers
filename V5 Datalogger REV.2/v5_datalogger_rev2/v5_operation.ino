void Operation(char * operationServerNumber) {
  char smsSegmentContainer[2000];
  char infoSMS[100];
  uint8_t dataloggerMode = savedDataLoggerMode.read();
  // LEDOn();
  
  for (int s=0;s<sizeof(_globalSMSDump);s++) _globalSMSDump[s] = 0x00;

  getTimeStamp(_timestamp, sizeof(_timestamp));
  
  if (dataloggerMode == 1) {
      Watchdog.reset();
      debugPrintln("GATEWAY with subsurface sensor + [1] ROUTER");
      debugPrintln("Waiting for router data..");
      waitForLoRaRouterData(MAX_GATWAY_WAIT, 1, savedLoraReceiveMode.read()); //  waits for router to send sensor info and adds it so sms sending stack
      debugPrintln("Collecting sensor column data..");                        //  collects sensor after router terminates with volt string or time-out
      dueDataCollection(SAMPLINGTIMEOUT);                                     //  adds collected sensors data to SMS stack
      generateInfoMessage(infoSMS);                                           //  generates datalogger info & rain sms(e.g.,TESTAW,32.00,0.00,0.82,99,200415171303)
      addToSMSStack(infoSMS);                                                 //  adds datalogger info & rain to SMS stack

  } else if (dataloggerMode == 2) {
    debugPrintln("ROUTER MODE");
    // LoRa transmitter of version 5 datalogger
    // get_Due_Data(2, get_serverNum_from_flashMem());
    Watchdog.reset();
  } else if (dataloggerMode == 3) {
  //   // Gateway only with 1 LoRa transmitter
  //   turn_ON_GSM(get_gsm_power_mode());
  //   Watchdog.reset();
  //   send_rain_data(0);
  //   Watchdog.reset();
  //   receive_lora_data(3);
  //   Watchdog.reset();
  //   turn_OFF_GSM(get_gsm_power_mode());
  //   Watchdog.reset();
  } else if (dataloggerMode == 4) {
  //   // Gateway only with 2 LoRa transmitter
  //   turn_ON_GSM(get_gsm_power_mode());
  //   Watchdog.reset();
  //   send_rain_data(0);
  //   Watchdog.reset();
  //   receive_lora_data(4);
  //   Watchdog.reset();
  //   turn_OFF_GSM(get_gsm_power_mode());
  //   Watchdog.reset();
  } else if (dataloggerMode == 5) {
  //   // Gateway only with 3 LoRa transmitter
  //   turn_ON_GSM(get_gsm_power_mode());
  //   Watchdog.reset();
  //   send_rain_data(0);
  //   Watchdog.reset();
  //   receive_lora_data(5);
  //   Watchdog.reset();
  //   turn_OFF_GSM(get_gsm_power_mode());
  //   Watchdog.reset();
  } else if (dataloggerMode == 6) {
  //   // Rain gauge ONLY datalogger - GSM
  //   debug_println("Begin: logger mode 6");
  //   turn_ON_GSM(get_gsm_power_mode());
  //   Watchdog.reset();
  //   send_rain_data(0);
  //   Watchdog.reset();
  //   turn_OFF_GSM(get_gsm_power_mode());
  //   Watchdog.reset();

  } else if (dataloggerMode == 7) {

  } else if (dataloggerMode == 8) {
  
  } else if (dataloggerMode == 9) {
    // debug_println("Begin: logger mode 9");
    // turn_ON_GSM(get_gsm_power_mode());
    // Watchdog.reset();
    // send_rain_data(0); //send rain
    // Watchdog.reset();
    // getGNSSData(dataToSend, sizeof(dataToSend));  //read gnss data
    // Watchdog.reset();
    // get_Due_Data(1, get_serverNum_from_flashMem());
    // Watchdog.reset();
    // send_thru_gsm(dataToSend, get_serverNum_from_flashMem()); 
    // Watchdog.reset();
    // turn_OFF_GSM(get_gsm_power_mode());
    // Watchdog.reset();
  
  } else {
    dueDataCollection(SAMPLINGTIMEOUT); // adds received data to smsStack
    generateInfoMessage(infoSMS);   // send_rain_data()    
    addToSMSStack(infoSMS);
  }

  if (alarmResetFlag) addToSMSStack("Datalogger will reset after data collection.");  // notif for interval based reset

  if (loggerWithGSM(savedDataLoggerMode.read())) GSMPowerModeReset();                                                    //  disable power saving (if any) BEFORE sending data
  debugPrintln("Sending SMS dump");
  debugPrintln(_globalSMSDump);
  sendSMSDump(dumpDelimiter, operationServerNumber);                            //  sends entire sms stack
  clearGlobalSMSDump();                                                                     //  as the name suggests...
  if (savedGSMPowerMode.read() == 2 && loggerWithGSM(savedDataLoggerMode.read())) {   //  Mode 2: GSM module is inactive except when sending data
    delayMillis(10000);                                                                     //  allow some wait time for OTA SMS to arrive (if any)
    checkOTACommand();                                                                      //  opens message inbox and execute any OTA COMMANDS found 
    deleteMessageInbox();                                                                   //  deletes ALL messages in SIM                                                              
  }
  if (loggerWithGSM(savedDataLoggerMode.read()))  GSMPowerModeSet();                                                      //  Enable/sets power saving mode AFTER sending data
}

void initSleepCycle() {

  // SYSCTRL->VREG.bit.RUNSTDBY = 1;
  SYSCTRL->XOSC32K.reg |= (SYSCTRL_XOSC32K_RUNSTDBY | SYSCTRL_XOSC32K_ONDEMAND);  // set external 32k oscillator to run when idle or sleep mode is chosen

  REG_GCLK_CLKCTRL |= GCLK_CLKCTRL_ID(GCM_EIC) |  // generic clock multiplexer id for the external interrupt controller
                      GCLK_CLKCTRL_GEN_GCLK1 |    // generic clock 1 which is xosc32k
                      GCLK_CLKCTRL_CLKEN;         // enable it
  while (GCLK->STATUS.bit.SYNCBUSY)
    ;  // write protected, wait for sync

  EIC->WAKEUP.reg |= EIC_WAKEUP_WAKEUPEN4;  // Set External Interrupt Controller to use channel 4 (pin 6)
  EIC->WAKEUP.reg |= EIC_WAKEUP_WAKEUPEN5;  // Set External Interrupt Controller to use channel 5 (pin A4)
  EIC->WAKEUP.reg |= EIC_WAKEUP_WAKEUPEN2;  // Set External Interrupt Controller to use channel 2 (pin A0)

  PM->SLEEP.reg |= PM_SLEEP_IDLE_CPU;  // Enable Idle0 mode - sleep CPU clock only
  // PM->SLEEP.reg |= PM_SLEEP_IDLE_AHB; // Idle1 - sleep CPU and AHB clocks
  // PM->SLEEP.reg |= PM_SLEEP_IDLE_APB; // Idle2 - sleep CPU, AHB, and APB clocks

  // It is either Idle mode or Standby mode, not both.
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;  // Enable Standby or "deep sleep" mode
}

void sleepNow(uint8_t savedLoggerMode) {
  Serial.println(F("MCU is going to sleep . . ."));
  Serial.println(F(""));

  SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;  // disable systick interrupt
  // LowPower.standby();                          // enters sleep mode
  __DSB();
  __WFI();
  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;   // Enable systick interrupt
}