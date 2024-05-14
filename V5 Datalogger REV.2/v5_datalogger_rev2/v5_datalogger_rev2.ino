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

#define FIRMWAREVERSION 2405.14
#define BAUDRATE 115200
#define DUEBAUD 9600
#define DEBUGTIMEOUT 60000
#define WAITDEBUGTIMEOUT 10000            // wait 10 sec
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
#define MAX_GATWAY_WAIT 300000

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
  char sensorA[6];
  char sensorB[6];
  char sensorC[6];
  char sensorD[6];
  char sensorNameList[20][20];      // currently limited to 10 
  // char sensorE[6];
  // char sensorF[6];
} SensorNameStruct;
SensorNameStruct flashLoggerName;

const char defaultDataloggerNameA[] = "TESTA";
const char defaultDataloggerNameB[] = "TESTB";
const char defaultDataloggerNameC[] = "TESTC";
const char defaultDataloggerNameD[] = "TESTD";
// const char defaultDataloggerNameC[] = "TESTE";
// const char defaultDataloggerNameD[] = "TESTF";
const char defaultSenslopeCommand[] = "ARQCMD6T";

// RTC related
char _timestamp[30];
bool RTCWakeFlag = false;

// GSM-related global variables
Uart Serial2(&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler() {
  Serial2.IrqHandler();
}
bool GSMReadyFlag = false;
bool runGSMInit = true;
volatile bool GSMIntFlag = false;
char defaultServerNumber[] = "09175388301";   //  GLOBE2

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

/**
 * Reserve a portions of flash memory to store parameters
 */
FlashStorage(hhAlarm,int);
FlashStorage(mmAlarm,int);
FlashStorage(savedAlarmInterval, int);
FlashStorage(savedDataLoggerMode, int);
FlashStorage(hasSubsurfaceSensorFlag, int);
FlashStorage(hasUbloxRouterFlag, int);
FlashStorage(savedRouterCount, int);
FlashStorage(savedGSMPowerMode, int);
FlashStorage(savedRainCollectorType, int);
FlashStorage(savedRainSendType, int);
FlashStorage(savedBatteryType, int);
FlashStorage(savedLoraReceiveMode, int);
FlashStorage(loggerParamSetFlag,int);
FlashStorage(savedServerNumber, serverNumberStruct);
FlashStorage(savedCommands, commandStruct);
FlashStorage(savedLoggerName, SensorNameStruct);

//Flags
bool alarmResetFlag = false;

/// Initial setup of datalogger.
///
/// Details of setup.

// Uart Serial2(&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
// void SERCOM1_Handler() {
//   Serial2.IrqHandler();
// }

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

  /* Assign pins 10 & 11 UART SERCOM functionality */
  pinPeripheral(10, PIO_SERCOM);
  pinPeripheral(11, PIO_SERCOM);
  
  Wire.begin();
  rtc.begin();
  LoRaInit();
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
  while (waitDebugMode) { // Automatically enters debug mode if serial is connected within 10 sec
    if ((millis() - debugWaitStart) > WAITDEBUGTIMEOUT) {    
      debugWaitStart = millis();
      break;
    }
    if (Serial) {
      LEDOff();
      delayMillis(2000);
      if (loggerParamSetFlag.read() == 0) introMSG();
      Serial.println(F(""));
      Serial.println(F("------------------------------------------------------"));
      Serial.println(F("-                     DEBUG MODE                     -"));
      Serial.println(F("------------------------------------------------------"));
      debugMode = true;
      break;
    }
  }
  LEDOff();
}

void loop() {
  
  // Run once datalogger parameters are set
  while (loggerParamSetFlag.read() == 0) {  
    updateLoggerMode();   // Set datalogger mode, ddefaults to 0 if invalid or timed-out
    Serial.println(F("------------------------------------------------------"));
    scalableUpdateSensorNames();
    loggerParamSetFlag.write(99);
    debugPrintln("");
    Serial.println(F("------------------------------------------------------"));
  }

  if (runGSMInit && loggerWithGSM(savedDataLoggerMode.read())) {
    runGSMInit = false;
    GSMInit();
  }
  
  while (debugMode) {
    printMenu();
    debugFunction();
  }

  // Triggers time interval-based reset flag 
  setResetFlag(23, 30);

  if (RTCWakeFlag) {
    RTCWakeFlag = false;
    LEDSleepWake();
    Operation(flashServerNumber.dataServer); //place operation loop here
    LEDSleepWake();
  }

  if (GSMIntFlag) {
    GSMIntFlag = false;
    LEDOn();
    if (savedGSMPowerMode.read() == 0) checkOTACommand();
    else if (savedGSMPowerMode.read() == 1) {
      GSMPowerModeReset();
      checkOTACommand();
      GSMPowerModeSet();
    }
    deleteMessageInbox();
    LEDOff();
  }

  setNextAlarm(savedAlarmInterval.read());
  disableWatchdog();
  if (alarmResetFlag) {
    alarmResetFlag = false; // precaution only; pwede naman kahit wala ito
    NVIC_SystemReset();
  } else sleepNow(savedDataLoggerMode.read());
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

bool loggerWithGSM(uint8_t dMode) {
  if (dMode == 2 ||
      dMode == 7 )
      return false;
  else return true;
}
