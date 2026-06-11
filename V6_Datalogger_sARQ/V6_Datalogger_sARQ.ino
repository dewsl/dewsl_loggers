// V6 [sARQ] datalogger code
// Feb 4, 2025
extern "C" {
  
  #include "driver/rtc_io.h"        // for sleep-wake interaction
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



#define NO_ESP32_CRYPT              // Disable all SHA, AES and RSA hardware acceleration

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
#define GPIO_SENSOR_PIN GPIO_NUM_36   // GPIO pin connected to the sensor
#define RAIN_PIN                36    // this is different from above
#define MAX_TEST_LIMIT          20
#define RTC_GPIO_INDEX          0     // attain dynamically with: rtc_io_number_get(GPIO_SENSOR_PIN)
#define SLOW_PROG_ADDR          51    // di ko maalala kung bakit 51 & 50 ang address na nilagay ko dito, dapat yata may offset ito?
#define EDGE_COUNT              10    // RTC RAM address start; different from flash address

//  macros for ULP rain tip counter
//  should always be placed at the beginning
//  do not relocate.. unless necessary..

const ulp_insn_t ulp_program[] = {
  // Initialize transition counter and previous state
  I_MOVI(R3, 0),                // R3 <- 0 (reset the transition counter)
  I_MOVI(R2, 1),                // R2 <- 0 (previous state, assume LOW initially)
  M_LABEL(1),                   // Main loop
    I_RD_REG(RTC_GPIO_IN_REG, RTC_GPIO_INDEX + RTC_GPIO_IN_NEXT_S, RTC_GPIO_INDEX + RTC_GPIO_IN_NEXT_S),    // Read RTC_GPIO_INDEX with RTC offset
    I_MOVR(R1, R0),             // R1 <- R0 Save current state to temporary register (R1)
    I_SUBR(R0, R1, R2),         // R0 = current state (R1) - previous state (R2) (Compare current state (R1) with previous state (R2))
    I_BL(5, 1),                 // If R0 == 0 (no state change), skip instructions
    I_ADDI(R3, R3, 1),          // Increment R3 by 1 (transition detected)
    I_MOVR(R2, R1),             // R2 <- R1 (store the current state for the next iteration)
    // Store the state transition counter
    I_MOVI(R1, EDGE_COUNT),     // Set R1 to address RTC_SLOW_MEM[1]
    I_ST(R3, R1, 0),            // Store it in RTC_SLOW_MEM
    // introduce some delay; RTC clock on the ESP32 is 17.5MHz 
    // this might needs 5-10ms of sofware debounce..  
    I_DELAY(0xFFFF),            // delay 0xFFFF = 3.74 ms
    I_DELAY(0xFFFF),            // more..
    I_DELAY(0xFFFF),            // 
    // I_DELAY(0xFFFF),         // too much?
  M_BX(1),                      // Loop back to label 1
};

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
#define FIRMWAREVERSION               2506.11     //YYMM.DD
#define DEBUGTIMEOUT                  300000
#define MAX_DATALOGGER_NAME_LENGTH    10
#define SERIALBAUDRATE                115200
#define STANDALONE                    0
#define GATEWAYMODE                   1
#define ROUTERMODE                    2
#define arrayCount(x) (sizeof(x) / sizeof(x[0]))    // arrayCount(arr) = number of rows  arrayCount(arr[0]) = number of columns
RTC_DATA_ATTR volatile bool runOnce   = true; // safety  
bool loggerNameChange                 = false;
bool workingMode                      = false;
bool debugExitSkip                    = false;
bool forDeployment                    = false;
bool operationFlag                    = false;  // wake reason flag for RTC
bool gsmFlag                          = false;  // wake reason flag for GSM; use this later
bool loraFlag                         = false;  // wake reason flag for LoRa; use this later
bool debugReq                         = false;

// GSM
#define COMM_SW                       2             // Test switch pin. MUST BE LOW DURING BOOT
HardwareSerial                        GSMSerial(2);
const char dumpDelimiter[]            = "~";
char _globalSMSDump[3000];
float _mValue[6];                                   //  this counts 3 the pairs of data for voltage and current monitoring (pre-peri-post) 
bool GSM_INIT_FLAG = true;

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

typedef struct {
  boolean valid;
  char sensorNameList[MAX_DATALOGGER_NAME_LENGTH][20];      // currently limited to MAX_DATALOGGER_NAME_LENGTH 
  // pwede pa maglagay dito ng ibang list
} SensorNameStruct;
SensorNameStruct flashLoggerName;



// float opAmpSlopeDefault = 0.21;                   //  based on readings from sAQR 0013 op-amp 
// float opAmpOffsetDefault = -0.07;



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
  // debugPrintln("GSM Ring Interupt");
  // LoRa.rfm_done = true;
}

INA219 INA219Module(0x40);                                                          //  create object and set address of INA219

void setup() {  
  Serial.begin(SERIALBAUDRATE);                                                     //  initialize Serial
  Wire.begin();
  wakeReason();
  // initializeLORA(VSPI_RST);           //  initialize VSPI for LORA
  InitializeRainULP(RAIN_PIN);                                                      //
  initializeULPProgram();                                                           // Load the ULP program for reading rain tips during deep sleep
  ADCInit();
  // GSMSerial.begin(115200, SERIAL_8N1, GSM_RXD, GSM_TXD);
  // GSMInitInt(GSM_RING_INT);        // iffy but it works

  rtcInit(RTC_INT);
  if (!rtc.begin()) {
    debugPrintln("RTC module ERROR");
    delayMillis(1000);
  }

  //RTC_SLOW_MEM[EDGE_COUNT] = 0;
  //ulp_run(SLOW_PROG_ADDR);      // Start the ULP program with offset

  esp_err_t err = ulp_run(SLOW_PROG_ADDR);

  debugPrint("ulp_run result = ");
  debugPrintln(err);

  // delay(1000);

  // debugPrint("RTC[10] after 1 sec = ");
  // debugPrintln(RTC_SLOW_MEM[10]);

  // pinMode(GPIO_NUM_2, OUTPUT);
  // gpio_hold_dis(GPIO_NUM_2);      // release hold after wake
  // digitalWrite(GPIO_NUM_2, HIGH); // turn GSM ON
  // delayMillis(3000);              // wait for GSM boot

  GSMConfig();
  
  GSM_INIT_FLAG = true;

  SSMInit();
  loadDefaultParams(paramStorage);              // by default this should not write to the NVS
  syncRTCwithCompileTime();                     //  Failsafe to prevent invalid timestamps when RTC power is removed
  uint8_t dMode = fetchParam(paramStorage, DATALOGGER_MODE, (uint8_t)0);
  if (dMode == 1 || dMode == 2) initializeLORA(VSPI_RST);         // only initialize if it will be used

  delayMillis(1000);
  // set2Alarms(0,30);  // 
  // loadDefaultParams(defaultServerNumber);

  pinMode(AUX_TRIG, OUTPUT);
  digitalWrite(AUX_TRIG, LOW);

  // pinMode(COMM_SW, OUTPUT);
  // pinMode()
  // digitalWrite(COMM_SW, LOW);
  
  // digitalWrite(COMM_SW, HIGH);
  
  if (runOnce) {
    //setCpuFrequencyMhz(80);
    setCpuFrequencyMhz(160);    // Wifi & bluetooth requires frequencies above 80Mhz
                                // 160Mhz setting causes sAQR to consume around 150mA.
                                // Higher frequency means code is loaded and executed faster, but will require MORE power..
                                // This can be adjusted up to 240Mhz.
    
    
    // initWifiOTAConnection();  // wifi init function, do this later...
    initBluetooth();          // bluetooth init
    debugPrintln("Bluetooth debugging enabled");

    unsigned long startConnectionWaitTime = millis();
    bool connectedStat = false;
    debugPrintln("WiFi network connection");
    debugPrintln("[Input anything to cancel search]");
    while (millis() - startConnectionWaitTime < WIFI_SEARCH_DURATION && !connectedStat) {
      debugPrint("."); delayMillis(1000);
      if (wifiConnectedStatus()) { connectedStat = true; OTAProc(OTA_HANDLER_DURATION);}
      if (BTSerial.hasClient() == true) {connectedStat = true; BtSerialFlag = true; break;}
      if (Serial.available() > 0) break;                        // for the impatient ones..
    }
    debugPrintln("");
    if (!connectedStat) {
      debugPrintln("No WiFi or Bluetooth connected");
      disableWifi();                // turn these of since nothing is connected
      BTSerial.end();               // turn these of since nothing is connected
      // setCpuFrequencyMhz(80);       // drop cpu frequency to save power; otherwise retain higher freuqncy to maintain wifi & bluetooth connectivity
      Serial.println("now operating at 80Mhz"); 
    }

    // debugPrintln("GSM ONLY MODE");
    debugPrintln("[Input anything to enter debug]");

    unsigned long startWait = millis();

    while (millis() - startWait < 10000) {
      debugPrint(".");
      delayMillis(1000);

      if (Serial.available() > 0) {
        debugPrintln("");
        debugPrintln("Debug requested");
        BtSerialFlag = true;
        debugReq = true;
        break;
      }
    }        
    debugPrintln("");
    runOnce=false;                  // prevents this part from executing again, unless MCU is restarted
  }

  seTPowerMode();

}

void loop() {

  debugPrintln("");
  debugPrintln("");

  // Debug mode
  if (debugReq) {
  // if (BtSerialFlag || Serial.available() > 0) {
    debugPrintln("DEBUG MODE START");
    debugFunction();
    debugPrintln("DEBUG MODE END");
    // return;
  }

  if (runOnce) {

  }

  // Only run if RTC/scheduler sets this
  if (operationFlag) {
    operationFlag = false;

    Serial.println("Initializing GSM...");

    GSMInit();

    char operationServer[15];
    fetchParam(paramStorage, SERVER_NUMBER, operationServer, sizeof(operationServer));
    Operation(operationServer);
    delayMillis(3000);
    resetRainULP();
  }

  debugPrintln("");
  setNextAlarm();
  displayNextAlarm();
  delayMillis(500);

  seTPowerMode();
  delayMillis(500);

  debugPrintln("ESP32 will enter deep sleep..");
  delayMillis(500);
 
                                                  //  hold GPIO states so its retained during sleep
  rtc_gpio_hold_en(GPIO_NUM_26);                  //  AUX POWER
  rtc_gpio_hold_en(GPIO_NUM_25);                  //  GSM RST (If HIGH; change causes module reset)
  rtc_gpio_hold_en(GPIO_NUM_2);                   //  COMMS POWER
  rtc_gpio_hold_en(GPIO_NUM_4);                   //  SPI RST (If HIGH; change causes module reset)

  // esp_deep_sleep_start();
  esp_light_sleep_start();
  Serial.println("ESP32 resumed from light sleep..");
  
                                                  //  disable pad holds so it can be changed if needed 
  rtc_gpio_hold_en(GPIO_NUM_26);                  //  AUX POWER
  rtc_gpio_hold_en(GPIO_NUM_25);                  //  GSM RST (to allow reset)
  rtc_gpio_hold_en(GPIO_NUM_2);                   //  COMMS POWER
  rtc_gpio_hold_en(GPIO_NUM_4);                   //  SPI RST (to allow reset)

  // resetPowerMode();

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

void powerSaving(){
    // adc_digi_stop();           //  stop ADC conversions. restart ADC before use
    // adc_digi_deinitialize();   //  deinitialize ADC driver
    // WiFi.disconnect(true);     //  Disconnect from the network
    // WiFi.mode(WIFI_OFF);       //  Switch WiFi off
    // btStop();
    setCpuFrequencyMhz(40);       //  reduce CPU frequency. Hanggang 80Mhz lang "daw" stable pero Ok pa din naman sa 40Mhz, Doable din ang 20Mhz pero wala pang long term testing
}