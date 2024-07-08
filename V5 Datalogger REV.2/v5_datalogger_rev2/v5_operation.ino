void Operation(const char * operationServerNumber) {

  char smsSegmentContainer[500];
  char infoSMS[100];
  // char routerEndIdentifier[100];     // "volt" string
  uint8_t dataloggerMode = savedDataLoggerMode.read();
  // LEDOn();

  clearGlobalSMSDump();

  // reload global variables (parameters) here
  flashLoggerName = savedLoggerName.read();
  flashServerNumber = savedServerNumber.read();
  flashCommands = savedCommands.read();
  getTimeStamp(_timestamp, sizeof(_timestamp));
  
  // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
  // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
  // 1.0 / First Part
  // complete data collection tasks according to datalogger requirements
  // dump (delimited) everything to global SMS dump "_globalSMSDump" then tokenize for sending on second part
  if (dataloggerMode == 0 || dataloggerMode == 1){
    debugPrint("ARQ MODE");
    if (hasUbloxRouterFlag.read() == 99) debugPrint("+ UBLOX");
    debugPrintln("");

    if (hasUbloxRouterFlag.read() == 99) {                                //  process for UBLOX additional 
      getGNSSData(smsSegmentContainer, sizeof(smsSegmentContainer));      //  get UBLOX data..
      addToSMSStack(smsSegmentContainer);                                 //  add UBLOX data to global sms stack..
    }
    dueDataCollection(SAMPLINGTIMEOUT);                                   //  get data from sensor column and adds received data to smsStack
    generateInfoMessage(infoSMS);                                         //  similar to old send_rain_data(); holds rain info and some diagnostics    
    addToSMSStack(infoSMS);                                               //  adds info sms to global sms stack
  }
 else if (dataloggerMode == 2) {                                          // routine is almost the same as above, pwede pa ito (lahat ng modes) ma-consolidate sa mas simple na process
    debugPrintln("ROUTER MODE:");
    if (hasSubsurfaceSensorFlag.read() == 99) debugPrint("SUBSURFACE SENSOR ");
    if (hasUbloxRouterFlag.read() == 99) debugPrint("+ UBLOX");
    debugPrintln("");
    if (hasUbloxRouterFlag.read() == 99) {
      getGNSSData(smsSegmentContainer, sizeof(smsSegmentContainer));
      addToSMSStack(smsSegmentContainer);
    }
    if (hasSubsurfaceSensorFlag.read() == 99) {
      debugPrintln("Collecting sensor column data..");
      dueDataCollection(SAMPLINGTIMEOUT); 
    } 
    generateInfoMessage(infoSMS);                                         // if rain data / datalogger info string is needed
    addToSMSStack(infoSMS);
    generateVoltString(infoSMS);                                          // volt data for terminating gateway wait time and couting data sets from routers
    addToSMSStack(infoSMS);

  } else if (dataloggerMode == 3) {                                       //   GATEWAY MODE
    debugPrintln("Waiting for router data..");
    waitForLoRaRouterData(MAX_GATWAY_WAIT, savedRouterCount.read(), savedLoraReceiveMode.read());
    if (hasUbloxRouterFlag.read() == 99) {
      getGNSSData(smsSegmentContainer, sizeof(smsSegmentContainer));
      addToSMSStack(smsSegmentContainer);
    }
    if (hasSubsurfaceSensorFlag.read() == 99) {
      debugPrintln("Collecting sensor column data..");
      dueDataCollection(SAMPLINGTIMEOUT); 
    } 
  } else if (dataloggerMode == 4) {                                       //   RAIN GAUGE ONLY - GSM
    generateInfoMessage(infoSMS);
    addToSMSStack(infoSMS);

  } else if (dataloggerMode == 5) {
  //   RAIN GAUGE ONLY - LoRA
  //   Pwede din ito maacheive kung gagawa ka ng router na walang ubox and subsurface sensor
  //   pero ganito na lang setup para mas madali makita or kung may kailangan pa i-add sa process flow
      generateInfoMessage(infoSMS);
      addToSMSStack(infoSMS);
  
  } 

  // reset rain tips after adding stored rain tips to sms stack
  resetRainTips();

  // Additional
  // insert datalogger self reset indicator
  if (alarmResetFlag && !debugMode) {
    char resetNotif[100];
    sprintf(resetNotif, "Datalogger %s will reset after data collection", flashLoggerName.sensorNameList[0]);
    addToSMSStack(resetNotif);  // notif for interval based reset
  }

  // 2.0 / Part 2
  // Send everything in global dump

  if (loggerWithGSM(savedDataLoggerMode.read())) GSMPowerModeReset();                       //  disable power saving (if any) BEFORE sending data
  debugPrintln("Sending SMS dump");
  debugPrintln(_globalSMSDump);
  sendSMSDump(dumpDelimiter, operationServerNumber);                                        //  sends entire sms stack depending on communication of mode
  clearGlobalSMSDump();                                                                     //  as the name suggests...
  
  if (savedGSMPowerMode.read() == 2 && loggerWithGSM(savedDataLoggerMode.read())) {         //  Mode 2: (power saving) GSM module is inactive (turned OFF) except when sending data
    delayMillis(15000);                                                                     //  allow some wait time for some OTA SMS to arrive (if any). Not sure kung reliable yung ganitong process, may instance na hindi na dumadating yung SMS. Pero mas priority ang data avaiilability than OTA capabilities
    checkForOTAMessages();
    // checkOTACommand();                                                                   //  opens message inbox and execute any OTA COMMANDS found 
    deleteMessageInbox();                                                                   //  deletes ALL messages in SIMCARD                                                      
  }
  
  if (loggerWithGSM(savedDataLoggerMode.read()))  GSMPowerModeSet();                        //  Enable/sets power saving mode AFTER sending data
  // debugPrintln("End of operation");

  //  router use only
  //  flag is set by the sendThruLoraWithAck function
  //  kung magawa nang always listening ang mga routers, pwede na ito gamitin  directly to immediately process broadcasted commands
  if (routerProcessOTAflag) {
    routerProcessOTAflag = false;     // failsafe
    // sample: LTEG^REC'D_~ROUTER~RESET~24/07/08,10:30:00

    char * ackResponse = strtok(routerOTACommand, dumpDelimiter);
    char routerOTAbuf[50];
    char routerOTAts[50];
    uint8_t responseIndex = 0;
    while (ackResponse!= NULL) {
      if (responseIndex == 2 && strlen(ackResponse) > 0) sprintf(routerOTAbuf, "%s", ackResponse);
      if (responseIndex == 3 && strlen(ackResponse) > 0) sprintf(routerOTAts, "%s", ackResponse);
      responseIndex++;
      ackResponse = strtok(NULL, dumpDelimiter);
    }
    debugPrint("Router OTA to process: ");
    debugPrintln(routerOTAbuf);
    if (strlen(routerOTAbuf) > 0) findOTACommand(routerOTAbuf, "NANEEE", routerOTAts);               // CAUTION Experimental function only
  }
}

void initSleepCycle() {

  // SYSCTRL->VREG.bit.RUNSTDBY = 1;
  SYSCTRL->XOSC32K.reg |= (SYSCTRL_XOSC32K_RUNSTDBY | SYSCTRL_XOSC32K_ONDEMAND);  // set external 32k oscillator to run when idle or sleep mode is chosen

  REG_GCLK_CLKCTRL |= GCLK_CLKCTRL_ID(GCM_EIC) |  // generic clock multiplexer id for the external interrupt controller
                      GCLK_CLKCTRL_GEN_GCLK1 |    // generic clock 1 which is xosc32k
                      GCLK_CLKCTRL_CLKEN;         // enable it
  while (GCLK->STATUS.bit.SYNCBUSY);  // write protected, wait for sync

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