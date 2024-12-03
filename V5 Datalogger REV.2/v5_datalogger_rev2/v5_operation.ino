/// 
void Operation(const char * operationServerNumber) {

  resetWatchdog();
  char smsSegmentContainer[500];
  char infoSMS[100];
  
  
  clearGlobalSMSDump();

  // SAVED PARAMETERS & VARIABLES
  // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
  // reload global variables (parameters) here
  uint8_t dataloggerMode = savedDataLoggerMode.read();
  flashLoggerName = savedLoggerName.read();
  flashServerNumber = savedServerNumber.read();
  flashCommands = savedCommands.read();
  getTimeStamp(_timestamp, sizeof(_timestamp));
  resetWatchdog();


  // SENSOR DATA COLLECTION
  // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
  // complete data collection tasks according to datalogger requirements
  // dump everything [delimited] to global SMS dump "_globalSMSDump"
  if (dataloggerMode == ARQMODE){
    // ARQ MODE
    getLoggerModeAndName();
    if (hasUbloxRouterFlag.read() == 99) {                                //  process for UBLOX additional 
      debugPrintln("Collecting GNSS data..");
      getGNSSData(smsSegmentContainer, sizeof(smsSegmentContainer));      //  get UBLOX data..
      addToSMSStack(smsSegmentContainer);                                 //  add UBLOX data to global sms stack..
    }
    if (hasSubsurfaceSensorFlag.read() == 99) {                           //  get data from sensor column
      debugPrintln("Collecting sensor column data..");
      dueDataCollection(SAMPLINGTIMEOUT);                                 // adding data to sms stack is done inside the function..
    }                                   
    generateInfoMessage(infoSMS);                                         //  similar to old send_rain_data(); holds rain info and some diagnostics    
    addToSMSStack(infoSMS);                                               //  adds info sms to global sms stack
  
 } else if (dataloggerMode == ROUTERMODE) {                               // this ROUTER routine is almost same as above except the added terminations segments for LoRa
    // ROUTER MODE
    getLoggerModeAndName();
    if (hasUbloxRouterFlag.read() == 99) {
      debugPrintln("Collecting GNSS data..");
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

  } else if (dataloggerMode == GATEWAYMODE) {                             //  GATEWAY MODE
    // GATEWAY MODE
    if (listenMode.read()) broadcastLoRaKey(random(100, 500), LBT_BROADCAST);     
    debugPrintln("Waiting for router data..");                            //  This is not conditional: We'd assume that if its a gateway, there should be a router
    waitForLoRaRouterData(MAX_GATEWAY_WAIT, savedRouterCount.read(), savedLoraReceiveMode.read());
    
    if (hasUbloxRouterFlag.read() == 99) {
      debugPrintln("Collecting GNSS data..");
      getGNSSData(smsSegmentContainer, sizeof(smsSegmentContainer));
      addToSMSStack(smsSegmentContainer);
    }
    if (hasSubsurfaceSensorFlag.read() == 99) {
      debugPrintln("Collecting sensor column data..");
      dueDataCollection(SAMPLINGTIMEOUT); 
    }
    generateInfoMessage(infoSMS);                                         // if rain data / datalogger info string is needed
    addToSMSStack(infoSMS);
  }
  // END OF SENSOR DATA COLLECTION
  

  // SOME EXTRA STEPS
  // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
  resetRainTips();  // reset rain tips after accumulated rain tips value was added to sms stack

  if (selfResetFlag && !debugMode) {  // Add datalogger self reset indicator to sending stack
    char resetNotif[100];
    sprintf(resetNotif, "Datalogger %s will reset after data collection", flashLoggerName.sensorNameList[0]);
    addToSMSStack(resetNotif);  // notif for interval based reset
  }

  // DATA SENDING
  // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
  // At this point [assuming everything goes well], all needed data would hava been added to the sending stack (_globalSMSDump)
  // Next step is to send everything to the appropriate channel (thru GSM or LoRa)

  if (loggerWithGSM(savedDataLoggerMode.read())) GSMPowerModeReset();                       //  disable power saving modes (if any is enabled) BEFORE sending data; 
  debugPrintln("Sending SMS dump");                                                         //  inidcator
  debugPrintln(_globalSMSDump);                                                             //  In case you want to check manually.. 
  sendSMSDump(dumpDelimiter, operationServerNumber);                                        //  sends entire sending stack depending on communication mode
  clearGlobalSMSDump();                                                                     //  as the name suggests...
  
  if (savedGSMPowerMode.read() == 2 && loggerWithGSM(savedDataLoggerMode.read())) {         //  Only used when GSM Power Mode 2 (power saving) is set: When GSM module is inactive (turned OFF) except when sending data
    resetWatchdog();
    delayMillis(15000);                                                                     //  allow for some wait time for some OTA SMS to arrive (if any). Not sure kung reliable yung ganitong approach, may instance na hindi na dumadating yung SMS. Pero mas priority ang data availability than OTA capabilities
    resetWatchdog();
    checkForOTAMessages();                                                                  //  check message inbox and executes ONLY THE FIRST OTA COMMAND found 
    deleteMessageInbox();                                                                   //  deletes ALL messages in SIMCARD                                                      
  }
  // END OF DATA SENDING

  
  // PREPATIONS BEORE SLEEPING
  // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
  // checks supply voltage after data sending [this part is still experimental]
  if (savedBatteryType.read() ==  0 && autoPowerSaving.read() == 99) {
    // swtiches to power saving mode (if not already) if battery dips below threshold
    if (readBatteryVoltage(savedBatteryType.read()) < 12.5 && savedGSMPowerMode.read() == 0) if (savedGSMPowerMode.read() != 2) savedGSMPowerMode.write(2);  
    // swtiches to back to always On (if not already) when battery is above threshold
    if (readBatteryVoltage(savedBatteryType.read()) > 12.8 && savedGSMPowerMode.read() == 2) if (savedGSMPowerMode.read() != 0) savedGSMPowerMode.write(0);  
  }
  if (savedBatteryType.read() == 1 && autoPowerSaving.read() == 99) {
    // swtiches to power saving mode (if not already) if battery dips below threshold
    if (readBatteryVoltage(savedBatteryType.read()) < 3.8 && savedGSMPowerMode.read() == 0) if (savedGSMPowerMode.read() != 2) savedGSMPowerMode.write(2);
    // swtiches to back to always On (if not already) when battery is above threshold
    if (readBatteryVoltage(savedBatteryType.read()) > 4.0 && savedGSMPowerMode.read() == 2) if (savedGSMPowerMode.read() != 0) savedGSMPowerMode.write(0);
  }

  if (loggerWithGSM(savedDataLoggerMode.read()))  GSMPowerModeSet();                        //  Re-enable power saving mode (if any) AFTER sending data
  // debugPrintln("End of operation");

  //  router use only
  //  flag is set by the sendThruLoraWithAck function
  //  For LISTEN MODE routes, pwede na ito gamitin  directly to immediately process broadcasted commands
  if (routerProcessOTAflag) {
    routerProcessOTAflag = false;     // failsafe. meh
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

  if (listenMode.read() && savedDataLoggerMode.read() == ROUTERMODE) LoRaInit(INIT_WAIT);

  // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
  // END OF OPERATION
  // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

}
void initSleepCycle() {

  // SYSCTRL->VREG.bit.RUNSTDBY = 1;
  
  // do not remove this segment; baka makalimutan ko..
  // used to allow usb clock source to run aduring sleep
  // SYSCTRL->DFLLCTRL.bit.RUNSTDBY = 1;     
  // while(!SYSCTRL->PCLKSR.bit.DFLLRDY);    

  SYSCTRL->XOSC32K.reg |= (SYSCTRL_XOSC32K_RUNSTDBY | SYSCTRL_XOSC32K_ONDEMAND);  // set external 32k oscillator to run when idle or sleep mode is chosen

  REG_GCLK_CLKCTRL |= GCLK_CLKCTRL_ID(GCM_EIC) |  // generic clock multiplexer id for the external interrupt controller
                      GCLK_CLKCTRL_GEN_GCLK1 |    // generic clock 1 which is xosc32k
                      GCLK_CLKCTRL_CLKEN;         // enable it
  while (GCLK->STATUS.bit.SYNCBUSY);  // write protected, wait for sync

  // NVIC_DisableIRQ(EIC_IRQn);
  // NVIC_ClearPendingIRQ(EIC_IRQn);

  // NVIC_SetPriority(EIC_IRQn, 0);
  // NVIC_EnableIRQ(EIC_IRQn);
  // EIC->INTENSET.reg |= EIC_INTENSET_EXTINT5;

  EIC->WAKEUP.reg |= EIC_WAKEUP_WAKEUPEN4;  // Set External Interrupt Controller to use channel 4 (pin 6)
  EIC->WAKEUP.reg |= EIC_WAKEUP_WAKEUPEN5;  // Set External Interrupt Controller to use channel 5 (pin A4)
  EIC->WAKEUP.reg |= EIC_WAKEUP_WAKEUPEN2;  // Set External Interrupt Controller to use channel 2 (pin A0)

  // REG_EIC_CONFIG1 |= 0xA0000000;
  // REG_EIC_CTRL |= EIC_CTRL_ENABLE;

  // NVIC_EnableIRQ(EIC_IRQn);

  PM->SLEEP.reg |= PM_SLEEP_IDLE_CPU;  // Enable Idle0 mode - sleep CPU clock only
  // PM->SLEEP.reg |= PM_SLEEP_IDLE_AHB; // Idle1 - sleep CPU and AHB clocks
  // PM->SLEEP.reg |= PM_SLEEP_IDLE_APB; // Idle2 - sleep CPU, AHB, and APB clocks

  // It is either Idle mode or Standby mode, not both.
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;  // Enable Standby or "deep sleep" mode
}

// void initSleepCycle2() {
//   SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk; // Enable Sleep-On-Exit feature
//   SCB->SCR |= SCB_SCR_SEVONPEND_Msk;  // Enable Send-Event-on-Pend
// }

void sleepNow(uint8_t savedLoggerMode) {

  rf95.sleep();
  Serial.println(F("MCU is going to sleep . . ."));
  Serial.println(F(""));

  // Enable later for testing power draw
  // NVMCTRL->CTRLB.bit.SLEEPPRM = NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val;
  // SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

  SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;   // Disable systick interrupt
  __DSB();                                      // Use of memory barrier is recommended for portability
  __WFI();                                      // Execute WFI and enter sleep
                                                // code resumes here after any interrupt
  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;    // Enable SysTick interrupts

  resetWatchdog();
}

void resetGlobals() {
  // do something here
}