/// Created by : MAD, TEP, 2020
/// Modified by : DRM 2024

///
#include <Wire.h>
#include <LowPower.h>
#include "Sodaq_DS3231.h"
#include <SPI.h>
#include <RH_RF95.h>
#include <avr/dtostrf.h>  // dtostrf missing in Arduino Zero/Due
#include <EnableInterrupt.h>
#include <FlashStorage.h>
#include <Arduino.h>         // required before wiring_private.h
#include "wiring_private.h"  // pinPeripheral() function
#include <string.h>
#include <Adafruit_SleepyDog.h>

#define FIRMWAREVERSION 2408.12
#define BAUDRATE 115200
#define DUEBAUD 9600
#define DEBUGTIMEOUT 60000
#define WAITDEBUGTIMEOUT 15000            // wait 15 sec
#define DUESerial Serial1
#define DUETRIG 5
#define SAMPLINGTIMEOUT 60000

// for GSM
#define GSMSerial Serial2
#define GSMBAUDRATE 115200
#define GSMRST 12
#define GSMPWR A2  // not connected in rev3 (purple) boards
#define GSMDTR A1
#define GSMINT A0  // gsm interrupt

// for LoRa radio
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RF95_FREQ 433.0 
#define INIT_WAIT 10000
#define MAX_GATWAY_WAIT 300000
#define MAX_ROUTER_COUNT 20

// rain
#define RAININT A4

// RTC
#define RTCINTPIN 6

// voltage check
#define VBATPIN A7
#define VBATEXT A5

// misc macro
// arrayCount(arr) = number of rows  arrayCount(arr[0]) = number of columns
#define arrayCount(x) (sizeof(x) / sizeof(x[0]))      

RH_RF95 rf95(RFM95_CS, RFM95_INT);
const char ackKey[] = "^REC'D_";
char _routerOTA[100];

bool debugMode = false;
char _globalSMSDump[3000];
const char dumpDelimiter[] = "~";

typedef struct
{
  boolean valid;
  char sensorCommand[9];
  char stationName[10];
} commandStruct;
commandStruct flashCommands;

typedef struct
{
  boolean valid;
  char sensorNameList[MAX_ROUTER_COUNT][20];      // currently limited to 19 routers 
  // pwede pa maglagay dito ng ibang list
} SensorNameStruct;
SensorNameStruct flashLoggerName;

const char defaultDataloggerNameA[] = "XXXXA";
const char defaultDataloggerNameB[] = "XXXXB";
const char defaultDataloggerNameC[] = "XXXXC";
const char defaultDataloggerNameD[] = "XXXXD";
// const char defaultDataloggerNameC[] = "TESTE";
// const char defaultDataloggerNameD[] = "TESTF";
const char defaultSenslopeCommand[] = "ARQCMD6T";
bool loggerNameChange = false;

// RTC related
char _timestamp[30];
bool RTCWakeFlag = false;

// GSM serial hardware port
Uart Serial2(&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler() {
  Serial2.IrqHandler();
}
bool GSMReadyFlag = false;
volatile bool GSMIntFlag = false;
char defaultServerNumber[] = "09175388301";   //  GLOBE2
int ringCounter = 0;
bool runOnceFlag = true;

typedef struct
{
  boolean valid;
  char dataServer[15];
  char OTAServer[15];
} serverNumberStruct;
serverNumberStruct flashServerNumber;

// RAIN-related global variables
unsigned long _last_interrupt_time = 0;
volatile float _rainTips = 0.00;

// OTA related global variables
bool routerOTAflag = false;          //  determined wether OTA command will be passed to the router(s)
bool routerProcessOTAflag = false;   //  triggers router OTA processing after data sending
char routerOTACommand[100];         //  container for OTA command to be passed to routers(s)
                                    //  routers also use this to store OTA command to be processed triggered by the routerProcessOTAflag


/**
 * Reserve a portions of flash memory to store parameters
 */
FlashStorage(savedAlarmInterval, uint8_t);
FlashStorage(savedDataLoggerMode, uint8_t);
FlashStorage(hasSubsurfaceSensorFlag, uint8_t);
FlashStorage(hasUbloxRouterFlag, uint8_t);
FlashStorage(savedRouterCount, uint8_t);
FlashStorage(savedGSMPowerMode, uint8_t);
FlashStorage(savedRainCollectorType, uint8_t);
FlashStorage(savedRainSendType, uint8_t);
FlashStorage(savedBatteryType, uint8_t);
FlashStorage(savedLoraReceiveMode, uint8_t);
FlashStorage(loggerParamSetFlag,uint8_t);
FlashStorage(savedLoggerResetAlarm,uint8_t);
FlashStorage(savedServerNumber, serverNumberStruct);
FlashStorage(savedCommands, commandStruct);
FlashStorage(savedLoggerName, SensorNameStruct);
FlashStorage(autoPowerSaving, uint8_t);

//Flags
bool selfResetFlag = false;

//overloooooaaaaaad
void debugPrint(const char * toPrint) {
  if (debugMode && Serial) Serial.print(toPrint);
}
void debugPrintln(const char * toPrintln) {
  if (debugMode && Serial) Serial.println(toPrintln);
}
void debugPrint(float toPrint) {
  if (debugMode && Serial) Serial.print(toPrint);
}
void debugPrintln(float toPrintln) {
  if (debugMode && Serial) Serial.println(toPrintln);
}
void debugPrint(int toPrint) {
  if (debugMode && Serial) Serial.print(toPrint);
}
void debugPrintln(int toPrintln) {
  if (debugMode && Serial) Serial.println(toPrintln);
}

void setup() {

  Serial.begin(BAUDRATE);
  DUESerial.begin(DUEBAUD);
  rtc.begin();

  // /* Assign pins 10 & 11 UART SERCOM functionality */
  // pinPeripheral(10, PIO_SERCOM);
  // pinPeripheral(11, PIO_SERCOM);
  
  Wire.begin();
  rtc.begin();
  LoRaInit(INIT_WAIT);
  LEDInit();
  initSleepCycle();

  RTCInit(RTCINTPIN);
  GSMINTInit(GSMINT);
  rainInit(RAININT);
  dueInit(DUETRIG);

  unsigned long debugWaitStart = millis();   
  bool waitDebugMode = true;

  LEDSleepWake();
  LEDOn();
  delayMillis(1000);

  enableWatchdog();

   // precursor for entering debug mode
  while (waitDebugMode) {
    if ((millis() - debugWaitStart) > WAITDEBUGTIMEOUT) {    
      debugWaitStart = millis();
      break;
    }
    if (Serial) { // Automatically enters debug mode if serial is connected within the first 15 sec after reset/boot
      LEDOff();
      char restMsg[50];
      delayMillis(2000);
      if (loggerParamSetFlag.read() == 0) introMSG();
      Serial.println(F(""));
      Serial.println(F("------------------------------------------------------"));
      Serial.println(F("-                     DEBUG MODE                     -"));
      Serial.println(F("------------------------------------------------------"));
      resetStatCheck(restMsg);
      Serial.println(restMsg);
      debugMode = true;   // sets flag for debug mode in main loop
      break;
    }
  }
  LEDOff();
}

void loop() {

  // for testing use, tanggalin sa final version
  char restMsg[30];
  resetStatCheck(restMsg);
  Serial.println(F("------------------------------------------------------"));

  //  Runs once after upload of firmware and datalogger parameters are not set
  while (loggerParamSetFlag.read() == 0) {
    printLoggerModes();     //   
    updateLoggerMode();     // Set datalogger mode, ddefaults to 0 if invalid or timed-out
    Serial.println(F("------------------------------------------------------"));
    scalableUpdateSensorNames();
    loggerParamSetFlag.write(99);
    debugPrintln("");
    Serial.println(F("------------------------------------------------------"));
  }

  // Runs once after upload/roboot/reset
  if (runOnceFlag && loggerWithGSM(savedDataLoggerMode.read())) {
    runOnceFlag = false;
    if (GSMInit()) {                                          // Initializes GSM module after reset/boot
      // If GSM reset if successful
      Serial.println("GSM READY\n");
      char bootMgs[200];
      deleteMessageInbox();                                   //  delete ALL messages in SIM inbox to prevent processing of old SMS
      flashLoggerName = savedLoggerName.read();               //  update global param
      flashServerNumber = savedServerNumber.read();     
      char restMsgBuffer[50];
      resetStatCheck(restMsgBuffer);
      debugPrintln(restMsgBuffer);
      if(!debugMode) {
        sprintf(bootMgs,"%s: LOGGER POWER UP\nLast reset cause: %s",flashLoggerName.sensorNameList[0], restMsgBuffer);  // build boot message
        delayMillis(1500);
        sendThruGSM(bootMgs,flashServerNumber.dataServer);                         // send boot msg to server
      }
      // rf95.sleep();
      GSMPowerModeSet(); 
    }
  }
  
  // main loop for debug mode
  while (debugMode) {  
    printMenu();
    debugFunction();
  }

  // all routine operation function here
  if (RTCWakeFlag) {
    RTCWakeFlag = false;
    LEDSleepWake();
    ringCounter = 0;                                    //  resets GSM couter for reset thru calls
    setSelfResetFlag(savedLoggerResetAlarm.read());     //  Sets a flag "selfResetFlag" to trigger self reset segment [below] instead of executing sleep function immediately
    Operation(flashServerNumber.dataServer);            //  main operation function for data collection and sending
    LEDSleepWake();
  }

  // processing of received SMS
  if (GSMIntFlag) {
    GSMIntFlag = false;
    LEDOn();
    if (savedGSMPowerMode.read() == 0) checkForOTAMessages();
    if (savedGSMPowerMode.read() == 1) {
      GSMPowerModeReset();
      // checkOTACommand();
      checkForOTAMessages();
      GSMPowerModeSet();
    }
    // deleteMessageInbox();
    LEDOff();
  }        

  setNextAlarm(savedAlarmInterval.read());            //  sets next alarm

  // point here all function that wants to reset the datalogger, instead of using a separate reset function
  if (selfResetFlag || ringCounter >= 3) {                           //  perform self reset function if self reset flag is TRUE  
    selfResetFlag = false;                       //  precaution only in case reset function fails (unlikely); pwede naman kahit wala ito
    LEDSelfReset();                               //  distinct dapat ito sa regular sleep/wake LED pattern
    NVIC_SystemReset();                           //  reset
  } else {            
    LEDOn();                                      //  Pangtest ito ng interrupt [RAININT], pwede tanggalin later
    delayMillis(300);                             // 
    LEDOff();                                     //
    disableWatchdog();
    sleepNow(savedDataLoggerMode.read());         // proceed with normal wake sleep cycle
  }
  // wdSleepDuration = Watchdog.sleep();  
}

void disableWatchdog() {
  Serial.println("Watchdog Disabled!");
  Watchdog.disable();
}

void enableWatchdog() {
  Serial.println("Watchdog Enabled!");
  int countDownMS = Watchdog.enable(16000);  // max of 16 seconds
}

/// Kick watchdog to reset timer
/// Nilagay sa function para madali magmodify or magdisable ng WDT
void resetWatchdog() {          
  Watchdog.reset();
}

bool loggerWithGSM(uint8_t dMode) {
  if (dMode == 2 ||
      dMode == 7 )
      return false;
  else return true;
}

void resetStatCheck(char* resetCauseMsg) {
  if (REG_PM_RCAUSE == PM_RCAUSE_SYST) sprintf(resetCauseMsg, "Reset by system");
  else if (REG_PM_RCAUSE == PM_RCAUSE_WDT) sprintf(resetCauseMsg, "Reset by Watchdog");
  else if (REG_PM_RCAUSE == PM_RCAUSE_EXT) sprintf(resetCauseMsg, "External reset requested");
  else if (REG_PM_RCAUSE == PM_RCAUSE_BOD33) sprintf(resetCauseMsg, "Reset brown out 3.3V");
  else if (REG_PM_RCAUSE == PM_RCAUSE_BOD12) sprintf(resetCauseMsg, "Reset brown out 1.2v");
  else if (REG_PM_RCAUSE == PM_RCAUSE_POR) sprintf(resetCauseMsg, "Normal power on reset");
  // resetCauseMsg[strlen(resetCauseMsg)+1]=0x00;
  // Serial.println(resetCauseMsg);
}