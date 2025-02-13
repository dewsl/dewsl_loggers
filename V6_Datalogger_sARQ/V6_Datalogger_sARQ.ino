// V6 [sARQ] datalogger code
// Feb 4, 2025

#include <RTClibExtended.h>   // https://github.com/FabioCuomo/FabioCuomo-DS3231/tree/master
#include <Wire.h>
#include <SPI.h>
#include <RH_RF95.h>          // Yes, this aslo works with RF98...
#include <HardwareSerial.h>
#include <EEPROM.h>

#define EEPROM_SIZE 500

//  DEBUG 
#define FIRMWAREVERSION 2502.13
#define DEBUGTIMEOUT 120000
#define SERIALBAUDRATE 115200
#define SSMBAUDRATE 9600
bool loggerNameChange = false;
#define arrayCount(x) (sizeof(x) / sizeof(x[0]))    // arrayCount(arr) = number of rows  arrayCount(arr[0]) = number of columns

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
RH_RF95 rf98(CS_NSS, LORA_DIO0);

//  RTC
#define RTC_INT 13
RTC_DS3231 rtc;                                             //RTC instance

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

/// Interrupt servide routine for RTC alarms
void IRAM_ATTR alarmISR() {
  portENTER_CRITICAL(&intSync);
  alarmInterruptFlag = true;
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
  EEPROM.begin(EEPROM_SIZE);
  Serial.begin(SERIALBAUDRATE);
  // GSMSerial.begin(115200, SERIAL_8N1, GSM_RXD, GSM_TXD);
  // GSMInitInt(GSM_RING_INT);     // iffy but it works
  rtcInit(RTC_INT);
  if (!rtc.begin()) {
    Serial.println("RTC module ERROR");
    delayMillis(1000);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));     
  rainInit(RAIN_PIN);
  delayMillis(1000);
  set2Alarms(0,30);

  setDefaultValues(defaultServerNumber);

  pinMode(AUX_TRIG, OUTPUT);
  digitalWrite(AUX_TRIG, HIGH);

}

void loop() {
  // put your main code here, to run repeatedly:
  debugPrintln("Start");
  if (!GSM_INIT_FLAG) {
    GSMInit();
    GSM_INIT_FLAG = true;
    printMenu();
  }
  dateTimeNow();
  debugFunction();
  debugPrintln("debugMode end");
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