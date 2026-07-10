/*
V6 [sARQ] datalogger
Feb 4, 2025

Important parameters to set:

Board: "ESP32 dev module"
Erase All Flash Before Sketch Upload: "Enabled"
Events run on: "Core 1"
Flash Frequency: "40Mhz"
Flash Mode: "DIO"
Arduino Runs on: "Core 1"
Partition Scheme: "Minimal SPIFFS (1.9MB APP with OTA/128LB SPIFFS)"

*/


extern "C" {
  
  #include "driver/rtc_io.h"        // for sleep-wake interaction
  // #include "driver/pulse_cnt.h"

  #include <driver/gpio.h>

}
#include <RTClibExtended.h>         // https://github.com/FabioCuomo/FabioCuomo-DS3231/tree/master
#include <Wire.h>
#include <SPI.h>
#include <RH_RF95.h>                // Yes, this aslo works with RF98...
#include <HardwareSerial.h>         // This should be replaced with BLE library..  some day...
#include <BluetoothSerial.h>        // Initially from Evandro Luis Copercini
#include <Preferences.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <INA219.h>
#include "esp_task_wdt.h"
#include "driver/pcnt.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp32/rom/rtc.h"
  // #include "esp_sleep.h"

#define NO_ESP32_CRYPT              // Disable all SHA, AES and RSA hardware acceleration
#define FREQ_DEFAULT   80
#define FREQ_IDLE      40

/* COMMENT THIS OUT TO REMOVE PRINTS FOR DEBUGGING*/
// #define DEBUGPRINT

#ifdef DEBUGPRINT
  #define debugSys(z)    Serial.print(z)
  #define debugSysln(z)  Serial.println(z)
  #pragma message ("DEBUGGING PRINT is ENABLED; This should only be used for testing")
#else  // ignored by compiler if DEBUGPRINT is removed
  #define debugSys(z)
  #define debugSysln(z)
#endif

#define GSMPWR GPIO_NUM_2

/* USED BY ULP AND FOR RAIN TIP COUNTING */
#include "esp32/ulp.h"
#include "driver/rtc_io.h"
#include "soc/rtc_io_reg.h"
// #define GPIO_SENSOR_PIN GPIO_NUM_36   // GPIO pin connected to the sensor
#define MAX_TEST_LIMIT          20
#define RTC_GPIO_INDEX          0     // attain dynamically with: rtc_io_number_get(GPIO_SENSOR_PIN)
// #define SLOW_PROG_ADDR          51    // di ko maalala kung bakit 51 & 50 ang address na nilagay ko dito, dapat yata may offset ito?
#define EDGE_COUNT              10    // RTC RAM address start; different from flash address

#define RAIN_PIN                36    // this is different from above
#define PCNT_UNIT               PCNT_UNIT_0                                     // use unit 0 of 8
#define PCNT_CHANNEL            PCNT_CHANNEL_0
// #define RAIN   PCNT_PIN_NOT_USED
RTC_DATA_ATTR volatile uint16_t rainCountFlag = 0;               //  test counter for rain count event
// pcnt_unit_handle_t pcnt_unit = NULL;

/*USED FOR OTA FIRMWARE UPDATE*/
#define OTA_HANDLER_DURATION          120000
#define WIFI_SEARCH_DURATION          30000
// const char* ssid = "ESP32OTA";
// const char* password = "senslopeDay"; 


//  NVS handler here and key IDS
Preferences storageSpace;
//  why is this defined instead of just using simple strings?
//  To prevent typographical errors (spelling mistakes)
//  If a key ID is misspelled, the compiler does not know it is an error, unless you catch it..
//  So we try to prevent with the compiler so we dont need to catch
//  Everything is arbitrary; you can make it as fancy if you'd like...
//  Also, defined keys can be made longer(or shorter if you want to make life harder for other people) so its easiliy readable
//  The equivalent key ID should only be 15 characters or fewer
#define SERVER_NUMBER           "SERVER_NUMBER"
#define NVS_USE_FLAG            "NVS_USE_FLAG"
#define ALARM_INTERVAL          "ALARM_INTERVAL"
#define DATALOGGER_MODE         "DLOGGER_MODE"
#define SUBSURFACE_SENSOR_FLAG  "SSENSOR_FLAG"
#define UBLOX_FLAG              "UBLOX_FLAG"
#define ROUTER_COUNT            "ROUTER_COUNT"
#define POWER_SAVING_MODE       "PWR_SAVE_MODE" 
#define RAIN_COLLECTOR_TYPE     "RAIN_COLLECTOR" 
#define RAIN_DATA_TYPE          "RAIN_DATA_TYPE" 
#define BATTERY_TYPE            "BATTERY_TYPE" 
#define LORA_RECEIVE_MODE       "LORA_RCV_MODE" 
#define SELF_RESET_HOUR         "SELF_RESET_HH" 
#define SELF_RESET_MINUTE       "SELF_RESET_MM" 
#define LISTEN_MODE             "LISTEN_MODE" 
#define OP_AMP_SLOPE            "OP_AMP_SLOPE" 
#define OP_AMP_OFFSET           "OP_AMP_OFFSET" 
#define SERVER_NUMBER           "SERVER_NUMBER" 
#define SENSOR_COMMAND          "SENSOR_COMMAND" 

// Bluetooth
BluetoothSerial BTSerial;     // Change partition to Minimal spiffs with OTA or else the large overhead will eat up the program storage space
bool BtSerialFlag = false;

//  DEBUG
#define FIRMWAREVERSION               2607.10     //YYMM.DD
#define DEBUGTIMEOUT                  300000
#define MAX_DATALOGGER_NAME_LENGTH    10
#define SERIALBAUDRATE                115200
#define STANDALONE                    0
#define GATEWAYMODE                   1
#define ROUTERMODE                    2
#define arrayCount(x) (sizeof(x) / sizeof(x[0]))    // arrayCount(arr) = number of rows  arrayCount(arr[0]) = number of columns
bool runOnceFlag                      = true; // safety  
bool loggerNameChange                 = false;
bool workingMode                      = false;
bool debugExitSkip                    = false;
bool forDeployment                    = false;
volatile bool operationFlag           = false;  // wake reason flag for RTC
bool gsmFlag                          = false;  // wake reason flag for GSM; use this later
bool loraFlag                         = false;  // wake reason flag for LoRa; use this later
bool _debugReq                         = false;

// GSM
#define COMM_SW                       2             // Test switch pin. MUST BE LOW DURING BOOT
#define GSM_RXD 16
#define GSM_TXD 17
#define GSM_RING_INT 35
#define GSM_RST 25
HardwareSerial                        GSMSerial(2);
const char dumpDelimiter[]            = "~";
char _globalSMSDump[3000];
char _resetCauseContainer[500];
float _mValue[6];                                   //  this counts 3 the pairs of data for voltage and current monitoring (pre-peri-post) 
volatile bool ringFlag = true;

//LoRa
#define CS_NSS                        5             // this must be HIGH during boot or may cause errors; inherent to ESP32
#define VSPI_RST                      4             // defeault VSPI, different from HSPI
#define VSPI_CLK                      18
#define VSPI_MOSI                     23
#define VSPI_MISO                     19

#define LORA_DIO0                     27
#define LORA_DIO1                     14
#define LORA_DIO5                     255           //  placeholder only
#define RF98_FREQ                     433     
#define MAX_GATEWAY_WAIT_TIME         60000              //  300000
#define MAX_ROUTER_COUNT              10
SPIClass SPI3(VSPI); 
RH_RF95 rf98(CS_NSS, LORA_DIO0);

//  RTC
#define RTC_INT                       13
#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)
uint64_t bitmask = BUTTON_PIN_BITMASK(GPIO_NUM_13);
char _timestamp[30];
RTC_DS3231 rtc;                                             //RTC instance

//  SSM
#define SSMBAUDRATE                   9600
#define SSM_RX                        32
#define SSM_TX                        33
#define AUX_TRIG                      26
HardwareSerial                        SSMSerial(1);

#define RSHUNT                        100                     //  dont forget to check the breakoutboard for the resistor value
#define IMAX                          3                       //  max current our module expects.. this is number is arbitrary

//  NVS storage use
const char*  defaultDataloggerName =  "DYNXX";
const char*  defaultSensorCommand =   "ARQCMD6T";     
const char* defaultServerNumber =     "09175388301";   //  GLOBE2
const char* paramStorage = "storedParam";       //  used for setting defaults
char dataloggerNameList[MAX_ROUTER_COUNT][MAX_DATALOGGER_NAME_LENGTH];         //  let's try removing the struct container...
                                                            //  We need to reload this variable with the stored values every time the system resets

portMUX_TYPE intSync = portMUX_INITIALIZER_UNLOCKED;        //  used for interruppt synchronization

RTC_DATA_ATTR volatile uint8_t tipCount = 0;

bool routerOTAflag = false;          //  determined wether OTA command will be passed to the router(s)
bool routerProcessOTAflag = false;   //  triggers router OTA processing after data sending
char routerOTACommand[100];          //  container for OTA command to be passed to routers(s)
                                     //  routers also use this to store OTA command to be processed triggered by the routerProcessOTAflag

/// Interrupt service routine for LoRa peripheral
void IRAM_ATTR LoRaISR() {
  portENTER_CRITICAL(&intSync);
  // debugPrintln("LoRa Interupt DIO0"); // not sure kung ito ay data interrupt or transmission interrupt
  // LoRa.rfm_done = true;
  portEXIT_CRITICAL(&intSync);
}

/// Interrup service routine for GSM
/// Currently: All OTA messages thru GSM will only be processed after sensor data collection
///
void IRAM_ATTR GSMISR() {
  ringFlag = true;
}

void IRAM_ATTR RTCISR() {
  operationFlag = true;
}

INA219 INA219Module(0x40);                                                //  create object and set address of INA219

void setup() {

  Serial.begin(SERIALBAUDRATE);                                           //  initialize Serial
  Wire.begin();
  resetReason(0); resetReason(1);                                         // check reset reasons

  rainCounterInit(RAIN_PIN, GPIO_NUM_36, PCNT_UNIT);
  resetRainCounter(PCNT_UNIT);

  rtcInit(RTC_INT);
  syncRTCwithCompileTime();                                               //  Failsafe to prevent invalid timestamps when RTC power is removed

  loadDefaultParams(paramStorage);                                        //  by default this should not write to the NVS
  uint8_t dMode = fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0);
  if (dMode == 1 || dMode == 2) initializeLORA(VSPI_RST);                 //  only initialize if it will be used

  delayMillis(1000);
  
  pinMode(COMM_SW, OUTPUT);                                               //  This is detached from GSM because other functions also use it
  GSMConfig();                                                            //  GSM pin congifurations
  if (dMode != 2) GSMOn(); delayMillis(1000);                             //  set initial state as HIGH if GSM will be used
  pinMode(AUX_TRIG, OUTPUT);                                              //  This is detached from SSM init because other functions also use it         
  digitalWrite(AUX_TRIG, LOW);                                            //  set initial state

  
  SSMInit();                                                              //  SSM config
  seTPowerMode(FREQ_DEFAULT);                                             //  test power mode here; revise this later..
  watchdogConfig();                                                       //  generate watchdog congif and run it
  requestDebug();                                                         //  debug mode
}

void loop() {                                                             // try to keep this clean so its easy to read
  
  esp_task_wdt_reset();                                                   //  reset watchdog counter
  if (_debugReq) {debugFunction(); cpuFrequency(FREQ_IDLE);}              //  debug subprocess
  if (runOnceFlag) runOnce();                                             
  if (ringFlag) ringFunction();                                           //  do something when ring is detected
  if (rainCountFlag) rainCheck(PCNT_UNIT);                                //  temporary rain check

  if (operationFlag) {                                                    // Only run if RTC/scheduler sets this
    delayMillis(1000);
    operationFlag = false;
    char operationServer[15];

    cpuFrequency(FREQ_DEFAULT);                                           //  return CPU frequency to stable state
    delayMillis(1000);                                                    //  wait a bit...
    
    fetchParam(paramStorage, SERVER_NUMBER, operationServer, sizeof(operationServer));    //  fetch saved server number
    uint8_t dMode = fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0);                //  fetch saved datalogger mode
    if (dMode != ROUTERMODE) GSMInit();                                   //  if datalogger has GSM module, check it first
                                                                          //  if connection is unstable it has a chance to connect before sending
    Operation(operationServer, dMode);                                    //  run operation subprocess using saved parameters
    delayMillis(1000);                                                      

    setNextAlarm();                                                       //  set next alarm 
    // displayNextAlarm();                                                //  as it says..
    displayNextAlarm2(fetchParam(paramStorage, ALARM_INTERVAL,(uint8_t)0));
    delayMillis(500);                                                     //  wait a bit so you get a chance to see whats happening

    cpuFrequency(FREQ_IDLE);                                               //  throttle down CPU frequency to save power; this should keep current draw around 25mA
    delayMillis(500);                                                     //  wait a bit so you get a chance to see whats happening
  }
  delayMillis(20000);                                                        //  to keep thread busy 
  debugPrint("Free heap: ");
  debugPrintln(ESP.getFreeHeap());
}

void wakeReason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:     
      debugPrintln("Wakeup caused by external signal using RTC_IO"); 
      operationFlag = true;
      break;
    case ESP_SLEEP_WAKEUP_EXT1:     debugPrintln("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER:    debugPrintln("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: debugPrintln("Wakeup caused by touch"); break;
    case ESP_SLEEP_WAKEUP_ULP:      debugPrintln("Wakeup caused by ULP program"); break;
    default:                        Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

void resetReason(uint8_t coreIndex) {
  if (coreIndex == 0)  debugPrint("CPU0 reset reason: ");
  if (coreIndex == 1)  debugPrint("CPU1 reset reason: ");
  if (coreIndex > 1)  {debugPrintln("Index ???"); return;}
  switch (rtc_get_reset_reason(coreIndex)) {
    case 1 : debugPrintln ("POWERON_RESET");break;          /**<1,  Vbat power on reset*/
    case 3 : debugPrintln ("SW_RESET");break;               /**<3,  Software reset digital core*/
    case 4 : debugPrintln ("OWDT_RESET");break;             /**<4,  Legacy watch dog reset digital core*/
    case 5 : debugPrintln ("DEEPSLEEP_RESET");break;        /**<5,  Deep Sleep reset digital core*/
    case 6 : debugPrintln ("SDIO_RESET");break;             /**<6,  Reset by SLC module, reset digital core*/
    case 7 : debugPrintln ("TG0WDT_SYS_RESET");break;       /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8 : debugPrintln ("TG1WDT_SYS_RESET");break;       /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9 : debugPrintln ("RTCWDT_SYS_RESET");break;       /**<9,  RTC Watch dog Reset digital core*/
    case 10 : debugPrintln ("INTRUSION_RESET");break;       /**<10, Instrusion tested to reset CPU*/
    case 11 : debugPrintln ("TGWDT_CPU_RESET");break;       /**<11, Time Group reset CPU*/
    case 12 : debugPrintln ("SW_CPU_RESET");break;          /**<12, Software reset CPU*/
    case 13 : debugPrintln ("RTCWDT_CPU_RESET");break;      /**<13, RTC Watch dog Reset CPU*/
    case 14 : debugPrintln ("EXT_CPU_RESET");break;         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : debugPrintln ("RTCWDT_BROWN_OUT_RESET");break;/**<15, Reset when the vdd voltage is not stable*/
    case 16 : debugPrintln ("RTCWDT_RTC_RESET");break;      /**<16, RTC Watch dog reset digital core and rtc module*/
    default : debugPrintln ("NO_MEAN");
  }
}

//overload here
  void debugPrint(const char *toPrint) {
    if (Serial) Serial.print(toPrint);
    if (BTSerial.hasClient()) BTSerial.print(toPrint);
  }
  void debugPrintln(const char *toPrintln) {
    if (Serial) Serial.println(toPrintln);
    if (BTSerial.hasClient()) BTSerial.println(toPrintln);
  }
  void debugPrint(float toPrint) {
    if (Serial) Serial.print(toPrint);
    if (BTSerial.hasClient()) BTSerial.print(toPrint);
  }
  void debugPrintln(float toPrintln) {
    if (Serial) Serial.println(toPrintln);
    if (BTSerial.hasClient()) BTSerial.println(toPrintln);
  }
  void debugPrint(double toPrint) {
    if (Serial) Serial.print(toPrint);
    if (BTSerial.hasClient()) BTSerial.print(toPrint);
  }
  void debugPrintln(double toPrintln) {
    if (Serial) Serial.println(toPrintln);
    if (BTSerial.hasClient()) BTSerial.println(toPrintln);
  }
  void debugPrint(int toPrint) {
    if (Serial) Serial.println(toPrint);
    if (BTSerial.hasClient()) BTSerial.println(toPrint);
  }
  void debugPrintln(int toPrintln) {
    if (Serial) Serial.println(toPrintln);
    if (BTSerial.hasClient()) BTSerial.println(toPrintln);
  }
  void debugPrint(unsigned long toPrint) {
    if (Serial) Serial.print(toPrint);
    if (BTSerial.hasClient()) BTSerial.print(toPrint);
  }
  void debugPrintln(unsigned long toPrintln) {
    if (Serial) Serial.println(toPrintln);
    if (BTSerial.hasClient()) BTSerial.println(toPrintln);
  }
  void debugPrint(long toPrint) {
    if (Serial) Serial.print(toPrint);
    if (BTSerial.hasClient()) BTSerial.print(toPrint);
  }
  void debugPrintln(long toPrintln) {
    if (Serial) Serial.println(toPrintln);
    if (BTSerial.hasClient()) BTSerial.println(toPrintln);
  }


// void powerSaving(){
    // adc_digi_stop();           //  stop ADC conversions. restart ADC before use
    // adc_digi_deinitialize();   //  deinitialize ADC driver
    // WiFi.disconnect(true);     //  Disconnect from the network
    // WiFi.mode(WIFI_OFF);       //  Switch WiFi off
    // btStop();
//     setCpuFrequencyMhz(40);       //  set
// }

void watchdogConfig() {
  esp_task_wdt_deinit();
  esp_task_wdt_config_t wdtConfig = {
    .timeout_ms = 600000,   // 10 minutes
    // .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,      // monitor only core 0 Idle task
    .idle_core_mask = 0,      // monitor only core 1 task and not Idle0
    .trigger_panic = true,    // trigger panic before reset
  };
  esp_task_wdt_init(&wdtConfig);
  esp_task_wdt_add(NULL);
}
void runOnce() {                  //  as the name suggests, this should only be executed once in a runtime 
    runOnceFlag = false;          //  reset flag so it doesn't run again
    rainCountFlag = false;        //  reset RAIN flag; if gate should be initially false
    ringFlag = false;             //  reset RING flag; if gate should be initially false
    setNextAlarm();               //  prime the alarm for first operation to trigger subsequent alarms       
    displayNextAlarm2(fetchParam(paramStorage, ALARM_INTERVAL,(uint8_t)0));         //  name
    debugPrintln("");   
    disableModems();              // disable uncessary peripherals

    //  check if datalogger has GSM
    //  send boot message if it does                 
    //  do something for the radio
}

//  this was wrapped in a function because it might get used a lot
void auxPowerOn() {
  if (gpio_get_level(GPIO_NUM_26) == 0) {digitalWrite(AUX_TRIG, HIGH); debugPrintln("AUX POWER is now ON");} 
  else debugPrintln("No action taken: AUX POWER is ON");
  delayMillis(200);
}

void auxPowerOff() {
  if (gpio_get_level(GPIO_NUM_26) == 1) {digitalWrite(AUX_TRIG, LOW); debugPrintln("AUX POWER is now OFF");} 
  else debugPrintln("No action taken: AUX POWER is OFF");
  delayMillis(200);
}

