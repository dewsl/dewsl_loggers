void setupTime()
{
    unsigned long startHere = millis();
    int done = 0;
    int MM = 0, DD = 0, YY = 0, hh = 0, mm = 0, ss = 0, dd = 0;
    Serial.println(F("\nSet time and date in this format: YYYY,MM,DD,hh,mm,ss,dd[0-6]Mon-Sun"));
    Serial.println(F("15 secs only"));
    // delay(10);
    // while (!Serial.available())
    // {
    //     if (timeOutExit(startHere, DEBUGTIMEOUT))
    //     {
    //         debug_flag_exit = true;
    //         break;
    //     }
    // }
    do
    {
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
	        done = 1;
	    }else{
	    	delay(10);
	    }
	} while (((millis() - startHere) < 120000) && done == 0); //2 minutes
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

int samplingTime(){
	DateTime now = rtc.now();
	if ((now.minute() == 0) || (now.minute() == 30)){
		return 1;
	// } else if ((now.minute() == 5) || (now.minute() == 35)){
	// 	return 1;
	} else if ((now.minute() == 10) || (now.minute() == 40)){
		return 1;
	// } else if ((now.minute() == 15) || (now.minute() == 45)){
	// 	return 1;
	} else if ((now.minute() == 20) || (now.minute() == 50)){
		return 1;
	// } else if ((now.minute() == 25) || (now.minute() == 55)){
	// 	return 1;

	} else {
		return 0;
	}
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

void setAlarm2()
{
    DateTime now = rtc.now(); //get the current date-time
    int a,b,c = 0;


  //   (now.minute() >= a ) && now.minute() <= b)
		// store_rtc = c;

    if ((now.minute() >= 0) && (now.minute() <= 4))
    {
        store_rtc = 5;
    }
	else if ((now.minute() >= 5) && (now.minute() <= 9))
    {
        store_rtc = 10;
    }
    else if ((now.minute() >= 10) && (now.minute() <= 14))
    {
        store_rtc = 15;
    }
    else if ((now.minute() >= 15) && (now.minute() <= 19))
    {
        store_rtc = 20;
    }
    else if ((now.minute() >= 20) && (now.minute() <= 24))
    {
        store_rtc = 25;
    }
    else if ((now.minute() >= 25) && (now.minute() <= 29))
    {
        store_rtc = 30;
    }
    else if ((now.minute() >= 30) && (now.minute() <= 34))
    {
        store_rtc = 35;
    }
    else if ((now.minute() >= 35) && (now.minute() <= 39))
    {
        store_rtc = 40;
    }
    else if ((now.minute() >= 40) && (now.minute() <= 44))
    {
        store_rtc = 45;
    }
    else if ((now.minute() >= 45) && (now.minute() <= 49))
    {
        store_rtc = 50;
    }
    else if ((now.minute() >= 50) && (now.minute() <= 54))
    {
        store_rtc = 55;
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


void wake()
{
  // OperationFlag = true;
  //detach the interrupt in the ISR so that multiple ISRs are not called
  detachInterrupt(RTCINTPIN);
}

void init_Sleep()
{
  //working to as of 05-17-2019
  SYSCTRL->XOSC32K.reg |= (SYSCTRL_XOSC32K_RUNSTDBY | SYSCTRL_XOSC32K_ONDEMAND); // set external 32k oscillator to run when idle or sleep mode is chosen
  REG_GCLK_CLKCTRL |= GCLK_CLKCTRL_ID(GCM_EIC) |                                 // generic clock multiplexer id for the external interrupt controller
                      GCLK_CLKCTRL_GEN_GCLK1 |                                   // generic clock 1 which is xosc32k
                      GCLK_CLKCTRL_CLKEN;                                        // enable it
  while (GCLK->STATUS.bit.SYNCBUSY)
    ; // write protected, wait for sync

  EIC->WAKEUP.reg |= EIC_WAKEUP_WAKEUPEN4; // Set External Interrupt Controller to use channel 4 (pin 6)
  EIC->WAKEUP.reg |= EIC_WAKEUP_WAKEUPEN5; // Set External Interrupt Controller to use channel 2 (pin A4)
  // EIC->WAKEUP.reg |= EIC_WAKEUP_WAKEUPEN2; // channel 2 (pin A0)

  PM->SLEEP.reg |= PM_SLEEP_IDLE_CPU; // Enable Idle0 mode - sleep CPU clock only
  //PM->SLEEP.reg |= PM_SLEEP_IDLE_AHB; // Idle1 - sleep CPU and AHB clocks
  //PM->SLEEP.reg |= PM_SLEEP_IDLE_APB; // Idle2 - sleep CPU, AHB, and APB clocks

  // It is either Idle mode or Standby mode, not both.
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; // Enable Standby or "deep sleep" mode
}

void sleepNow()
{
  Serial.println("MCU is going to sleep . . .");
  SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk; //disable systick interrupt
  LowPower.standby();                         //enters sleep mode
  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;  //Enabale systick interrupt
}


