// V6 [sARQ] datalogger code
// Feb 4, 2025

#include <RTClibExtended.h>   // https://github.com/FabioCuomo/FabioCuomo-DS3231/tree/master

// #include <WiFi.h>
// #include <esp_wifi.h>
// #include "driver/adc.h"
// #include <BluetoothSerial.h>
// #include <esp_bt.h>

#include "driver/rtc_io.h"   // for sleep-wake interaction
#include <Wire.h>
#include <SPI.h>
#include <RH_RF95.h>          // Yes, this aslo works with RF98...
#include <HardwareSerial.h>
#include <EEPROM.h>

#define EEPROM_SIZE 500

#define NO_ESP32_CRYPT  // Disable all SHA, AES and RSA hardware acceleration

// ULP
#include "esp32/ulp.h"
#include "driver/rtc_io.h"
#include "soc/rtc_io_reg.h"
#define GPIO_SENSOR_PIN GPIO_NUM_36   // GPIO pin connected to the sensor
#define RTC_GPIO_INDEX 0              // attain dynamically with: rtc_io_number_get(GPIO_SENSOR_PIN)
// #define ULP_INIT_MARKER_ADDR 100 // Address in RTC_SLOW_MEM for the marker
// #define ULP_INIT_MARKER 0x1234 // Unique marker to identify initialization

// #define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
// #define TIME_TO_SLEEP  5          /* Time ESP32 will go to sleep (in seconds) */

// enum {
//   EDGE_COUNT,
//   SLOW_PROG_ADDR  // Program start address
// };

#define EDGE_COUNT 50
#define SLOW_PROG_ADDR 51

//  instruction macros for ULP rain data
const ulp_insn_t ulp_program[] = {

  // Initialize transition counter and previous state
  I_MOVI(R3, 0),      // R3 <- 0 (reset the transition counter)
  I_MOVI(R2, 1),      // R2 <- 0 (previous state, assume LOW initially)
  
  M_LABEL(1),         // Main loop
  I_RD_REG(RTC_GPIO_IN_REG, RTC_GPIO_INDEX + RTC_GPIO_IN_NEXT_S, RTC_GPIO_INDEX + RTC_GPIO_IN_NEXT_S),    // Read RTC_GPIO_INDEX with RTC offset

  I_MOVR(R1, R0),  // R1 <- R0 Save current state to temporary register (R1)

  I_SUBR(R0, R1, R2),  // R0 = current state (R1) - previous state (R2) (Compare current state (R1) with previous state (R2))
  I_BL(5, 1),          // If R0 == 0 (no state change), skip instructions
  I_ADDI(R3, R3, 1),   // Increment R3 by 1 (transition detected)
  I_MOVR(R2, R1),      // R2 <- R1 (store the current state for the next iteration)

  // Store the state transition counter
  I_MOVI(R1, EDGE_COUNT),  // Set R1 to address RTC_SLOW_MEM[1]
  I_ST(R3, R1, 0),         // Store it in RTC_SLOW_MEM

  // introduce some delay; RTC clock on the ESP32 is 17.5MHz 
  I_DELAY(0xFFFF),  // debounce delay 0xFFFF = 3.74 ms
  I_DELAY(0xFFFF),  // debounce
  I_DELAY(0xFFFF),  // debounce
  I_DELAY(0xFFFF),  // debounce
  I_DELAY(0xFFFF),  // debounce
  I_DELAY(0xFFFF),  // debounce

  M_BX(1),  // Loop back to label 1
};


//  DEBUG 
#define FIRMWAREVERSION 2503.12
#define DEBUGTIMEOUT 120000
#define SERIALBAUDRATE 115200
#define SSMBAUDRATE 9600
#define arrayCount(x) (sizeof(x) / sizeof(x[0]))    // arrayCount(arr) = number of rows  arrayCount(arr[0]) = number of columns
bool loggerNameChange = false;
bool workingMode = false;
bool debugExitSkip = false;

// for indexing & debugging
#define ARQMODE 0
#define GATEWAYMODE 1
#define ROUTERMODE 2
#define MAX_NAME_COUNT 10   // temporary limit

// GSM
#define GSM_RXD 16
#define GSM_TXD 17
#define GSM_RING_INT 35
#define GSM_RST 25
HardwareSerial GSMSerial(2);
char _globalSMSDump[3000];
const char dumpDelimiter[] = "~";

//LoRa
#define CS_NSS 5          // this must be HIGH during boot or may cause errors; inherent to ESP32
#define VSPI_RST 4        // defeault VSPI, different from HSPI
#define VSPI_CLK 18
#define VSPI_MOSI 23
#define VSPI_MISO 19

#define LORA_DIO0 27
#define LORA_DIO1 14
#define LORA_DIO5 255     //  placeholder only
#define RF98_FREQ 433     
#define MAX_GATEWAY_WAIT_TIME 300000
#define MAX_ROUTER_COUNT 10
SPIClass SPI3(VSPI); 
RH_RF95 rf98(CS_NSS, LORA_DIO0);

//  RTC
#define RTC_INT 13
char _timestamp[30];
RTC_DS3231 rtc;                                             //RTC instance
#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)
uint64_t bitmask = BUTTON_PIN_BITMASK(GPIO_NUM_13);

#define AUX_TRIG 26

#define RAIN_PIN 36
#define MAX_TEST_LIMIT 20
bool testFlag = false;


bool alarmInterruptFlag = false;

char defaultServerNumber[] = "09175388301";   //  GLOBE2
bool GSM_INIT_FLAG = false;

typedef struct {
  boolean valid;
  char sensorNameList[MAX_NAME_COUNT][20];      // currently limited to MAX_NAME_COUNT 
  // pwede pa maglagay dito ng ibang list
} SensorNameStruct;
SensorNameStruct flashLoggerName;

portMUX_TYPE intSync = portMUX_INITIALIZER_UNLOCKED;        //  used for interruppt synchronization

RTC_DATA_ATTR volatile uint8_t tipCount = 0;
RTC_DATA_ATTR volatile uint32_t alarmCount = 0;
bool routerOTAflag = false;          //  determined wether OTA command will be passed to the router(s)
bool routerProcessOTAflag = false;   //  triggers router OTA processing after data sending
char routerOTACommand[100];         //  container for OTA command to be passed to routers(s)
                                    //  routers also use this to store OTA command to be processed triggered by the routerProcessOTAflag


/// Interrupt servide routine for RTC alarms
void IRAM_ATTR alarmISR() {
  portENTER_CRITICAL(&intSync);
  alarmInterruptFlag = true;
  alarmCount++;
  portEXIT_CRITICAL(&intSync);
}

/// Interrupt service routine for LoRa peripheral
void IRAM_ATTR LoRaISR() {
  portENTER_CRITICAL(&intSync);
  // Serial.println("LoRa Interupt DIO0"); // not sure kung ito ay data interrupt or transmission interrupt
  // LoRa.rfm_done = true;
  portEXIT_CRITICAL(&intSync);
}

/// Interrup service routine for GSM
void IRAM_ATTR GSMISR() {
  // Serial.println("GSM Ring Interupt");
  // LoRa.rfm_done = true;
}

/// Interrupt service routine for rain gauge attachment 
/// May interaction ito sa sleep-wake cycle, kailangan lagyan ng mask para di matrigger
void IRAM_ATTR rainISR() { 
  portENTER_CRITICAL(&intSync);
  tipCount++;
  testFlag = true;
  // debugPrint.print("Rain tips: ");
  // Serial.println( tipCount);    // this is not advisable
  portEXIT_CRITICAL(&intSync);
}

void setup() {

  EEPROM.begin(EEPROM_SIZE);          //  initialize EEPROM
  Serial.begin(SERIALBAUDRATE);       //  initialize Serial
  initializeLORA(VSPI_RST);           //  initialize VSPI for LORA
  InitializeRainULP(RAIN_PIN);
  initializeULPProgram();             // Load the ULP program for reading rain tips during deep sleep
  // GSMSerial.begin(115200, SERIAL_8N1, GSM_RXD, GSM_TXD);
  // GSMInitInt(GSM_RING_INT);        // iffy but it works

  rtcInit(RTC_INT);
  if (!rtc.begin()) {
    Serial.println("RTC module ERROR");
    delayMillis(1000);
  }

  ulp_run(SLOW_PROG_ADDR);  // Start the ULP program with offset
  
  rtcFallback();     
  
  delayMillis(1000);
  set2Alarms(0,30);  // 
  setDefaultValues(defaultServerNumber);

  pinMode(AUX_TRIG, OUTPUT);
  // digitalWrite(AUX_TRIG, HIGH);
  powerSaving();

}

void loop() {
  // this will not run repeatedly
  if (!GSM_INIT_FLAG & gpio_get_level(GPIO_NUM_26) == 1) {
    GSMInit();
    GSM_INIT_FLAG = true;
    printMenu();
  }
  dateTimeNow();
  if (changeParameter()) {
    // digitalWrite(AUX_TRIG, HIGH);
    debugPrintln("DEBUG Start");
    debugFunction();
    debugPrintln("DEBUG END");
    // digitalWrite(AUX_TRIG, LOW);
  }
  
  if (debugExitSkip) debugExitSkip=false;
  else operation_test();

  debugPrintln("ESP32 will sleep..");
  esp_deep_sleep_start();
  rtc_gpio_deinit(GPIO_NUM_13);
  // delay(1000);
  // esp_light_sleep_start();
  // REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_DBIAS_WAK, 4);
  // REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_DBIAS_SLP, 4);
  delayMillis(1000);      // add some delay after waking
}




//overload here
void debugPrint(const char * toPrint) {
  if (Serial) Serial.print(toPrint);
}
void debugPrintln(const char * toPrintln) {
  if (Serial) Serial.println(toPrintln);
}
void debugPrint(float toPrint) {
  if (Serial) Serial.print(toPrint);
}
void debugPrintln(float toPrintln) {
  if (Serial) Serial.println(toPrintln);
}
void debugPrint(int toPrint) {
  if (Serial) Serial.print(toPrint);
}
void debugPrintln(int toPrintln) {
  if (Serial) Serial.println(toPrintln);
}
void debugPrint(unsigned long toPrint) {
  if (Serial) Serial.print(toPrint);
}
void debugPrintln(unsigned long toPrintln) {
  if (Serial) Serial.println(toPrintln);
}
void debugPrint(long toPrint) {
  if (Serial) Serial.print(toPrint);
}
void debugPrintln(long toPrintln) {
  if (Serial) Serial.println(toPrintln);
}

void wakeReason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP:      Serial.println("Wakeup caused by ULP program"); break;
    default:                        Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

void powerSaving(){
    // adc_digi_stop();           //  stop ADC conversions. restart ADC before use
    // adc_digi_deinitialize();   //  deinitialize ADC driver
    // WiFi.disconnect(true);     //  Disconnect from the network
    // WiFi.mode(WIFI_OFF);       //  Switch WiFi off
    // btStop();
    setCpuFrequencyMhz(40); //  reduce CPU frequency
}