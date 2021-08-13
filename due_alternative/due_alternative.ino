/**
 * Champagne Datalogger + Alternative Custom Due
 * Surficial Tilt IMU sensor added

Features:
* Sends sensor data via LoRa or GSM
* Built-in rtc with configurable wake interrupt
* Low power mode ~10uA
* ~0.5mA with IMU sensor

The circuit:
* Champagne Board
* Adafruit Feather M0
* RFM95 433MHz
* Real time clock
* GSM

Created: January 2020
By : MAD, TEP
Modified: June 7, 2021
*/

#include <Wire.h>
#include <LowPower.h>
#include "Sodaq_DS3231.h"
#include <SPI.h>
#include <RH_RF95.h>
#include <avr/dtostrf.h> // dtostrf missing in Arduino Zero/Due
#include <EnableInterrupt.h>
#include <FlashStorage.h>
#include <Arduino.h>        // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function
#include <string.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SleepyDog.h>

#include <CAN.h>
#include "variant.h"
#include <SD.h>
#include <Adafruit_INA219.h>
#include <avr/pgmspace.h>
#include <stdbool.h>

#define BAUDRATE 115200
#define DUEBAUD 9600
#define DUESerial Serial1
#define RTCINTPIN 6
#define COLSW 5
#define DEBUG 1
#define VBATPIN A7
#define VBATEXT A5
#define GSMRST 12
#define GSMPWR A2
#define GSMDTR A1
#define GSMINT A0    //gsm ring interrupt
#define IMU_POWER A3 //A3-17

//gsm related
#define GSMBAUDRATE 9600
#define GSMSerial Serial2
#define MAXSMS 168

//LoRa related
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

#define RF95_FREQ 433.0          // Change to 434.0 or other frequency, must match RX's freq!
#define DATALEN 200              //max size of dummy length
#define LORATIMEOUT 500000       // 8.33 minutes delay
#define LORATIMEOUTMODE2 900000  // 15.0 mnutes
#define LORATIMEOUTMODE3 1200000 // 20.0 mnutes
#define DUETIMEOUT 210000        // 3.50 minutes timeout
#define DUEDELAY 60000           // 1.0 minute delay
#define RAININT A4               //rainfall interrupt pin A4
#define DEBUGTIMEOUT 300000      //debug timeout in case no data recieved; 60K~1minute

#define ATCMD "AT"
#define ATECMDTRUE "ATE"
#define ATECMDFALSE "ATE0"
#define OKSTR "OK"
#define ERRORSTR "ERROR"
#define VERBOSE 1

#define CAN_CS_PIN 17
#define CAN_IRQ_PIN 15
#define SD_CS_PIN 9
#define POLL_TIMEOUT 5000
#define ARQTIMEOUT 30000
#define VDATASIZE 300
#define ENABLE_RTC 0
#define CAN_ARRAY_BUFFER_SIZE 100

char comm_mode[5] = "ARQ";
const char base64[64] PROGMEM = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                                 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                                 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2',
                                 '3', '4', '5', '6', '7', '8', '9', '+', '/'};

File newconfig, oldconfig, root;
long timenow = 0;
long arq_start_time = 0;

// Group: Global Variables
// These are the variables declared globally.

/* 
  Variable: b64
  global variable that turns on the b64 data representation mode. 1 by default.

  Variable declaration:
  --- Code
  uint8_t b64 = 1;
  --- 
*/
uint8_t b64 = 1;

/* 
  Variable: datalogger_flag
  XXXXX. 0 by default.

  Variable declaration:
  --- Code
  uint8_t datalogger_flag = 0;
  --- 
*/
uint8_t datalogger_flag = 0;

/* 
  Variable: volt_last_val
  XXXXX. 0 by default.

  Variable declaration:
  --- Code
  int volt_last_val = 0;
  --- 
*/
int volt_last_val = 0;

/* 
  Variable: vdata_flag
  XXXXX

  Variable declaration:
  --- Code
  boolean vdata_flag;
  --- 
*/
boolean vdata_flag;

/* 
  Variable: can_flag
  XXXXX

  Variable declaration:
  --- Code
  bool can_flag;
  --- 
*/
bool can_flag;

/*
  Variable: g_gids
  2 dimensional integer array that stores the links the unique id with the geographic id
  
  Variable declaration:
  --- Code
  int g_gids[40][2];
  ---
*/
int g_gids[40][2];

/*
  Variable: g_num_of_nodes
  integer that stores the number of nodes. This variable is overwritten by *<process_config_line>*. This variable is also used by *<init_char_arrays>*.

  Variable declaration:
  --- Code
  uint8_t g_num_of_nodes = 40;
  --- 
*/
uint8_t g_num_of_nodes = 40;

/*
  Variable: g_mastername
  global char array that holds the 5 char mastername. This variable is overwrittern by *<process_config_line>*. This variable defaults to "XXXXX".

  *SD Card Config Line* usage:
  --- Code
  mastername = INATA
  ---
*/
char g_mastername[6] = "XXXXX";

/*
  Variable: g_volt
  global char array that holds the node voltage values with respect to gids.

  Variable Declaration:
  --- Code
  float g_volt[VDATASIZE];
  ---
*/
float g_volt[VDATASIZE];

/*
  Variable: g_turn_on_delay
  integer that determines the delay in centi seconds ( ie. 100 centiseconds = 1 sec ) introduced by the firmware after the column switch turns on. 
  This ensures the voltage stability for the column.

  *SD Card Config Line* usage:
  --- Code
  turn_on_delay = 100
  ---
*/
uint8_t g_turn_on_delay = 10;

/*
  Variable: g_sensor_version
  integer that determines the sensor version ( 1, 2, or 3 ). This variable is overwrittern by *<process_config_line>*. 

  *SD Card Config Line* usage:
  --- Code
  sensorVersion = 3
  ---*/
uint8_t g_sensor_version = 3; 

/*
  Variable: g_datalogger_version
  integer that determines the datalogger version. This variable is overwrittern by *<process_config_line>*. 
  
  Variable declaration:
  --- Code
  uint8_t g_datalogger_version = 3;
  ---
*/
uint8_t g_datalogger_version = 2;

/* 
  Variable: broad_timeout
  integer that determines the timeout duration of broadcast (in milliseconds ). This variable is overwrittern by *<process_config_line>*. 

  *SD Card Config Line* usage:
  --- Code
  brodcast_timeout = 3000  ---
*/
int broad_timeout = 3000;

/* 
  Variable: has_piezo
  boolean variable that decides the sampling of the piezometer readout board

  *SD Card Config Line* usage:
  --- Code
  PIEZO = 1
  ---
*/
bool has_piezo = false;

/* 
  Variable: g_sampling_max_retry
  integer that determines the number of column sampling retry
  
  *SD Card Config Line* usage:
  --- Code
  sampling_max_retry = 3
  ---
*/
int g_sampling_max_retry = 3;

/* 
  Variable: g_delim
  global char array that holds the delimiter used for separating data from different commands
  
  Variable declaration:
  --- Code
  char g_delim[2] = "~";
  ---
*/
char g_delim[2] = "~";

/* 
  Variable: g_temp_dump
  
  
  Variable declaration:
  --- Code
  char g_temp_dump[2500];
  ---
*/
char g_temp_dump[2500];

/* 
  Variable: g_final_dump
  
  
  Variable declaration:
  --- Code
  char g_final_dump[5000];
  ---
*/
char g_final_dump[5000];

/* 
  Variable: g_no_gids_dump
  
  
  Variable declaration:
  --- Code
  char g_no_gids_dump[2500];
  ---
*/
char g_no_gids_dump[2500];

/* 
  Variable: text_message
  char array that holds the formatted messages to be sent

  Intial Variable declaration:
  --- Code
  char text_message[5000];
  ---
  
  See Also:

  <build_txt_msgs>
*/
char text_message[5000];
static char g_test[100] = "\0";
char g_build[100] = "";
char g_build_final[500] = "";
/*
 * Variable: vc_flag
 * Global variable that triggers an additional sms with voltage and current reading (before, during, after polling)
 */
bool vc_flag = false;
char vc_text[100] = "\0";

String g_string;
String g_string_proc;

/*
  Variable: g_timestamp
  global String that holds the timestamp.This variable is overwritten by the timestamp from the string sent by the ARQ. This variable defaults to "TIMESTAMP000".

  Intial Variable declaration:
  --- Code
 String g_timestamp = "TIMESTAMP000";
  ---
*/
// String g_timestamp = "180607142000";
String g_timestamp = "TIMESTAMP000";

Adafruit_INA219 ina219;
bool ate = true;
bool fullblastlogger = true;

typedef union
{
  uint64_t value;
  struct
  {
    uint32_t low;
    uint32_t high;
  };
  struct
  {
    uint16_t s0;
    uint16_t s1;
    uint16_t s2;
    uint16_t s3;
  };
  uint8_t bytes[8];
  uint8_t byte[8]; //alternate name so you can omit the s if you feel it makes more sense
} BytesUnion;

typedef struct
{
  uint32_t id;      // EID if ide set, SID otherwise
  uint32_t fid;     // family ID
  uint8_t rtr;      // Remote Transmission Request
  uint8_t priority; // Priority but only important for TX frames and then only for special uses.
  uint8_t extended; // Extended ID flag
  uint8_t length;   // Number of data bytes
  BytesUnion data;  // 64 bits - lots of ways to access it.
} CAN_FRAME;

CAN_FRAME g_can_buffer[CAN_ARRAY_BUFFER_SIZE];

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

//initialize LoRa global variables
char dataToSend[DATALEN]; //lora
uint8_t payload[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len = sizeof(payload);

//LoRa received
uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len2 = sizeof(buf);
char received[250];

char streamBuffer[250]; //store message
char rssiString[250];
char voltMessage[200];
int customDueFlag = 0; //for data gathering
int sendToLoRa = 0;
uint8_t tx_RSSI = 0;   //tx rssi of sensor A
uint8_t tx_RSSI_B = 0; //tx rssi of sensor B
uint8_t tx_RSSI_C = 0; //tx rssi of sensor C

//rain gauge
static unsigned long last_interrupt_time = 0;
const unsigned int DEBOUNCE_TIME = 40; //40
volatile float rainTips = 0.00;
char sendRainTip[7] = "0.00";

volatile bool gsmRingFlag = false;  //gsm interrupt
volatile bool rainFallFlag = false; //rain tips
volatile bool OperationFlag = false;
bool getSensorDataFlag = false;
bool debug_flag_exit = false;

char firmwareVersion[8] = "21.3.23"; //year . month . date
char station_name[6] = "MADTA";
char Ctimestamp[13] = "";
char command[30];
char txVoltage[100] = "0";
char txVoltageB[100] = "0";
char txVoltageC[100] = "0";

unsigned long timestart = 0;
uint8_t serial_flag = 0;
uint8_t debug_flag = 0;
uint8_t rcv_LoRa_flag = 0;
uint16_t store_rtc = 00; //store rtc alarm
// uint8_t gsm_power = 0; //gsm power (sleep or hardware ON/OFF)

//GSM
String serverNumber = ("639175972526");
bool gsmPwrStat = true;
String tempServer, regServer;
char _csq[10];
char response[150];
bool registerNumber = false;

/* Pin 11-Rx ; 10-Tx (GSM comms) */
Uart Serial2(&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

typedef struct
{
  boolean valid;
  char senslopeCommand[50];
  char stationName[10];
} Senslope;
Senslope sensCommand;

typedef struct
{
  boolean valid;
  char sensorA[20];
  char sensorB[20];
  char sensorC[20];
  char sensorD[20];
} SensorName;
SensorName loggerName;

typedef struct
{
  boolean valid;
  char inputNumber[50];
} serNumber;
serNumber flashServerNumber;

typedef struct
{
  boolean valid;
  char keyword[50];
} smsPassword;
smsPassword flashPassword;

typedef struct
{
  boolean valid;
  char accel_param[100];
  char magneto_param[100];
  char gyro_param[15];
} imu_calib;

/**
 * Reserve a portion of flash memory to store an "int" variable
 * and call it "alarmStorage".
*/
FlashStorage(alarmStorage, int);
FlashStorage(loggerVersion, int);
FlashStorage(imuRawCalib, int);
FlashStorage(gsmPower, int);
FlashStorage(rainCollectorType, int);
FlashStorage(passCommand, Senslope);
FlashStorage(newServerNum, serNumber);
FlashStorage(flashLoggerName, SensorName);
FlashStorage(flashPasswordIn, smsPassword);
FlashStorage(flash_imu_calib, imu_calib);

//Group: Main Loop Functions
/* 
  Function: setup

    
    - Sets the baudrate for the Serial communications.

    - Initializes the CAN-related functions.

    - Initializes the global Strings objects used to compile data.

    - Initialize the gids ( geographic ids ) array before use. 

    - Initializes the SD-related functions.

    - Open the CONFIG.txt file, read and load to ram.

    - Display the settings read from CONFIG.txt.

    - Set the GPIO RELAYPIN to output.

  Parameters:
    
    n/a

  Returns:
    
    n/a
  
  See Also:

    <init_can> <init_strings> <init_gids> <init_sd> <open_config>
*/
void setup()
{
  Serial.begin(BAUDRATE);
  DUESerial.begin(DUEBAUD);
  GSMSerial.begin(115200);

  /* Assign pins 10 & 11 UART SERCOM functionality */
  pinPeripheral(10, PIO_SERCOM);
  pinPeripheral(11, PIO_SERCOM);

  Wire.begin();
  rtc.begin();
  init_lora();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(COLSW, OUTPUT);
  pinMode(GSMPWR, OUTPUT);
  pinMode(GSMRST, OUTPUT);
  pinMode(IMU_POWER, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(COLSW, LOW);
  digitalWrite(GSMPWR, LOW);
  digitalWrite(GSMRST, HIGH);
  digitalWrite(IMU_POWER, LOW);

  /* rain gauge interrupt */
  attachInterrupt(RAININT, rainISR, FALLING);
  /* rtc interrupt */
  attachInterrupt(RTCINTPIN, wake, FALLING);
  /* ring interrupt */
  attachInterrupt(GSMINT, ringISR, FALLING);

  init_Sleep(); //initialize MCU sleep state

  setAlarmEvery30(alarmFromFlashMem()); //rtc alarm settings
  rf95.sleep();

  delay_millis(3000);
  if ((get_logger_version() == 2) || (get_logger_version() == 6) || (get_logger_version() == 10) || (get_logger_version() == 8))
  {
    Serial.println(F("- - - - - - - - - - - - - - - -"));
    Serial.print("Logger Version: ");
    Serial.println(get_logger_version());
    Serial.println(F("Default to LoRa communication."));
    Serial.println(F("- - - - - - - - - - - - - - - -"));
  }
  else
  {
    //GSM power related
    Serial.println(F("- - - - - - - - - -"));
    Serial.print("Logger Version: ");
    Serial.println(get_logger_version());
    Serial.println(F("Default to GSM."));
    Serial.println(F("- - - - - - - - - -"));
    if (get_gsm_power_mode() == 1)
    {
      resetGSM();
      gsm_network_connect();
      init_gsm();
      turn_OFF_GSM(get_gsm_power_mode());
    }
    else if (get_gsm_power_mode() == 2)
    {
      resetGSM();
      gsm_network_connect();
      init_gsm();
    }
    else
    {
      resetGSM();
    }
  }

  //while (!Serial);
  ina219.begin();

  //temporary initialization for feather m0 lora, pull-up lora CS
  pinMode(RFM95_CS, OUTPUT);
  digitalWrite(RFM95_CS, HIGH);

  init_char_arrays();
  init_gids();
  
  init_sd();
  open_config();
  print_stored_config();
  digitalWrite(SD_CS_PIN, HIGH); //disable SD card CS pin to allow CAN initialization
  init_can();

  if ((g_datalogger_version == 3) || (g_datalogger_version == 4) || (g_datalogger_version == 5))
  {
    strncpy(comm_mode, "LORA", 4);
  }
  else
  {
    Serial.print("g_datalogger_version == ");
    Serial.println(g_datalogger_version);
    strncpy(comm_mode, "ARQ", 3);
  }
  Serial.print("Comms: ");
  Serial.println(comm_mode);
  //print_due_command2();

  
  /*Enter DEBUG mode within 10 seconds*/
  Serial.println(F("Press 'C' to go DEBUG mode!"));
  unsigned long serStart = millis();
  while (serial_flag == 0)
  {
    if (Serial.available())
    {
      debug_flag = 1;
      Serial.println(F("Debug Mode!"));
      // turn_ON_GSM(get_gsm_power_mode());
      serial_flag = 1;
    }
    // timeOut in case walang serial na makuha in ~10 seconds
    if ((millis() - serStart) > 10000)
    {
      serStart = millis();
      serial_flag = 1;
    }
  }
  flashLed(LED_BUILTIN, 5, 60);
}

/* 
  Function: loop

    - Waits for any activity in the DATALOGGER Serial.

    - Calls <operation>

    - Sets the datalogger_flag

  Parameters:
    
    n/a

  Returns:
    
    n/a
  
  See Also:

    <operation> <getATCommand> 
*/
void loop()
{
  while (debug_flag == 1)
  {
    getAtcommand();
    if (debug_flag_exit)
    {
      Serial.println(F("* * * * * * * * * * * * *"));
      Serial.println(F("Exiting from DEBUG MENU."));
      Serial.println(F("* * * * * * * * * * * * *"));
      if ((get_logger_version() == 2) || (get_logger_version() == 6) || (get_logger_version() == 10) || (get_logger_version() == 8))
      {
        //do nothing
      }
      else
      {
        turn_OFF_GSM(get_gsm_power_mode());
      }
      debug_flag = 0;
    }
  }

  if (OperationFlag)
  {
    flashLed(LED_BUILTIN, 2, 50);
    enable_watchdog();
    if (get_logger_version() == 1)
    {
      if (gsmPwrStat)
      {
        turn_ON_GSM(get_gsm_power_mode());
        Watchdog.reset();
      }
      if (fullblastlogger)
      {
        operation(1, get_serverNum_from_flashMem());
      }
      else
      {
        get_Due_Data(1, get_serverNum_from_flashMem());
      }
      Watchdog.reset();
      send_rain_data(0);
      Watchdog.reset();
      if (getSensorDataFlag == true && OperationFlag == true)
      {
        receive_lora_data(1);
        Watchdog.reset();
      }
      attachInterrupt(RTCINTPIN, wake, FALLING);
      Watchdog.reset();
      if (gsmPwrStat)
      {
        turn_OFF_GSM(get_gsm_power_mode());
        Watchdog.reset();
      }
    }
    else if (get_logger_version() == 2)
    {
      //LoRa transmitter of version 5 datalogger
      if (fullblastlogger)
      {
        operation(2, get_serverNum_from_flashMem());
      }
      else
      {
        get_Due_Data(2, get_serverNum_from_flashMem());
      }
      Watchdog.reset();
      attachInterrupt(RTCINTPIN, wake, FALLING);
      Watchdog.reset();
    }
    else if (get_logger_version() == 3)
    {
      //only one trasmitter
      turn_ON_GSM(get_gsm_power_mode());
      Watchdog.reset();
      send_rain_data(0);
      Watchdog.reset();
      receive_lora_data(3);
      Watchdog.reset();
      attachInterrupt(RTCINTPIN, wake, FALLING);
      Watchdog.reset();
      turn_OFF_GSM(get_gsm_power_mode());
      Watchdog.reset();
    }
    else if (get_logger_version() == 4)
    {
      //Two transmitter
      turn_ON_GSM(get_gsm_power_mode());
      Watchdog.reset();
      send_rain_data(0);
      Watchdog.reset();
      receive_lora_data(4);
      Watchdog.reset();
      attachInterrupt(RTCINTPIN, wake, FALLING);
      Watchdog.reset();
      turn_OFF_GSM(get_gsm_power_mode());
      Watchdog.reset();
    }
    else if (get_logger_version() == 5)
    {
      // Three transmitter
      turn_ON_GSM(get_gsm_power_mode());
      Watchdog.reset();
      send_rain_data(0);
      Watchdog.reset();
      receive_lora_data(5);
      Watchdog.reset();
      attachInterrupt(RTCINTPIN, wake, FALLING);
      Watchdog.reset();
      turn_OFF_GSM(get_gsm_power_mode());
      Watchdog.reset();
    }
    else if (get_logger_version() == 6)
    {
      //default arabica LoRa transmitter
      get_Due_Data(6, get_serverNum_from_flashMem());
      Watchdog.reset();
      attachInterrupt(RTCINTPIN, wake, FALLING);
      Watchdog.reset();
    }
    else if (get_logger_version() == 7)
    {
      // Sends rain gauge data via LoRa
      get_Due_Data(0, get_serverNum_from_flashMem());
      Watchdog.reset();
      delay_millis(1000);
      Watchdog.reset();
      send_rain_data(1);
      Watchdog.reset();
      attachInterrupt(RTCINTPIN, wake, FALLING);
      Watchdog.reset();
    }
    else if (get_logger_version() == 8)
    {
      // Sends rain gauge data via LoRa
      get_Due_Data(0, get_serverNum_from_flashMem());
      Watchdog.reset();
      delay_millis(1000);
      Watchdog.reset();
      send_rain_data(1);
      Watchdog.reset();
      send_thru_lora(dataToSend);
      Watchdog.reset();
      attachInterrupt(RTCINTPIN, wake, FALLING);
      Watchdog.reset();
    }
    else if (get_logger_version() == 9)
    {
      // Sends IMU sensor data to GSM
      /*
      on_IMU();
      Watchdog.reset();
      turn_ON_GSM(get_gsm_power_mode());
      Watchdog.reset();
      send_rain_data(0);
      Watchdog.reset();
      delay_millis(1000);
      Watchdog.reset();
      send_thru_gsm(read_IMU_data(get_calib_param()), get_serverNum_from_flashMem());
      Watchdog.reset();
      delay_millis(1000);
      Watchdog.reset();
      turn_OFF_GSM(get_gsm_power_mode());
      Watchdog.reset();
      off_IMU();
      Watchdog.reset();
      attachInterrupt(RTCINTPIN, wake, FALLING);
      Watchdog.reset();*/
    }
    else if (get_logger_version() == 10)
    {
      // Sends IMU sensor data to LoRa
      // send_thru_gsm(read_IMU_data(),get_serverNum_from_flashMem());
      /*
      on_IMU();
      Watchdog.reset();
      send_thru_lora(read_IMU_data(get_calib_param()));
      Watchdog.reset();
      delay_millis(1000);
      Watchdog.reset();
      send_rain_data(1);
      Watchdog.reset();
      off_IMU();
      Watchdog.reset();
      attachInterrupt(RTCINTPIN, wake, FALLING);
      Watchdog.reset();*/
    }
    else if (get_logger_version() == 11)
    {
      // Sends rain gauge data ONLY
      turn_ON_GSM(get_gsm_power_mode());
      Watchdog.reset();
      send_rain_data(0);
      Watchdog.reset();
      delay_millis(1000);
      Watchdog.reset();
      turn_OFF_GSM(get_gsm_power_mode());
      Watchdog.reset();
      attachInterrupt(RTCINTPIN, wake, FALLING);
      Watchdog.reset();
    }
    else
    {
      //default arQ like sending
      turn_ON_GSM(get_gsm_power_mode());
      Watchdog.reset();
      send_rain_data(0);
      Watchdog.reset();
      if (fullblastlogger)
      {
        operation(1, get_serverNum_from_flashMem());
      }
      else
      {
        get_Due_Data(1, get_serverNum_from_flashMem());
      }
      Watchdog.reset();
      attachInterrupt(RTCINTPIN, wake, FALLING);
      Watchdog.reset();
      turn_OFF_GSM(get_gsm_power_mode());
      Watchdog.reset();
    }

    rf95.sleep();
    getSensorDataFlag = false;
    OperationFlag = false;
  }

  if (rainFallFlag)
  {
    if (get_logger_version() == 2)
    {
      // wakeGSM();
      flashLed(LED_BUILTIN, 2, 50);
      //LoRa transmitter of version 5 datalogger
      get_Due_Data(2, get_serverNum_from_flashMem());
      // sleepGSM();
    }
    else
    {
      flashLed(LED_BUILTIN, 1, 50);
      attachInterrupt(RAININT, rainISR, FALLING);
      rainFallFlag = false;
    }
  }

  if (gsmRingFlag)
  {
    flashLed(LED_BUILTIN, 3, 40);
    if (get_gsm_power_mode() == 1)
    {
      Serial.println("1st AT + CNMI");
      GSMSerial.write("AT+CNMI=1,2,0,0,0\r");
      delay_millis(100);
    }
    GSMSerial.write("AT+CNMI=1,2,0,0,0\r");
    delay_millis(300);
    while (GSMSerial.available() > 0)
    {
      processIncomingByte(GSMSerial.read(), 0);
    }
    turn_OFF_GSM(get_gsm_power_mode());
    attachInterrupt(GSMINT, ringISR, FALLING);
    gsmRingFlag = false;
  }

  setAlarmEvery30(alarmFromFlashMem());
  delay_millis(75);
  rtc.clearINTStatus();

  attachInterrupt(GSMINT, ringISR, FALLING);
  attachInterrupt(RAININT, rainISR, FALLING);
  attachInterrupt(RTCINTPIN, wake, FALLING);
  sleepNow();
}

void enable_watchdog()
{
  Serial.println(F("Watchdog Enabled!"));
  int countDownMS = Watchdog.enable(16000); //max of 16 seconds
}

void disable_watchdog()
{
  Serial.println(F("Watchdog Disabled!"));
  Watchdog.disable();
}

void wakeAndSleep(uint8_t verSion)
{
  if (OperationFlag)
  {
    flashLed(LED_BUILTIN, 5, 100);

    if (verSion == 1)
    {
      get_Due_Data(2, get_serverNum_from_flashMem()); //tx of v5 logger
    }
    else
    {
      get_Due_Data(0, get_serverNum_from_flashMem()); //default arabica
    }

    setAlarmEvery30(alarmFromFlashMem());
    rtc.clearINTStatus(); // needed to re-trigger rtc
    rf95.sleep();
    OperationFlag = false;
  }
  // working as of May 28, 2019
  sleepNow();
  /*
  attachInterrupt(digitalPinToInterrupt(RTCINTPIN), wake, FALLING);
  SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk; //disable systick interrupt
  LowPower.standby();                         //enters sleep mode
  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;  //Enabale systick interrupt
  */
}

/*Enable sleep-standby*/
void sleepNow()
{
  Serial.println(F("MCU is going to sleep . . ."));
  SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk; //disable systick interrupt
  LowPower.standby();                         //enters sleep mode
  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;  //Enabale systick interrupt
}

void init_lora()
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay_millis(10);
  digitalWrite(RFM95_RST, HIGH);
  delay_millis(10);

  while (!rf95.init())
  {
    Serial.println(F("LoRa radio init failed"));
    while (1)
      ;
  }
  Serial.println(F("LoRa radio init OK!"));

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ))
  {
    Serial.println(F("setFrequency failed"));
    while (1)
      ;
  }
  Serial.print("Set Freq to: ");
  Serial.println(RF95_FREQ);
  /**
   *  Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
   * The default transmitter power is 13dBm, using PA_BOOST.
   * If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
   * you can set transmitter powers from 5 to 23 dBm:
  */
  rf95.setTxPower(23, false);
}

void send_thru_lora(char *radiopacket)
{
  if (digitalRead(SD_CS_PIN) == LOW)
  {
      digitalWrite(SD_CS_PIN, HIGH);
  }
  if (digitalRead(CAN_CS_PIN) == LOW)
  {
    digitalWrite(CAN_CS_PIN, HIGH);
  }
  digitalWrite(RFM95_CS, LOW);
  
  int length = sizeof(payload);
  int i = 0, j = 0;
  //do not stack
  for (i = 0; i < 200; i++)
  {
    payload[i] = (uint8_t)'0';
  }
  for (i = 0; i < length; i++)
  {
    payload[i] = (uint8_t)radiopacket[i];
  }
  payload[i] = (uint8_t)'\0';
  Serial.print("Sending to LoRa: ");
  Serial.println((char *)payload);
  // Serial.println("sending payload!");
  rf95.send(payload, length); //sending data to LoRa
  delay_millis(100);
  digitalWrite(RFM95_CS, HIGH);
}

void receive_lora_data(uint8_t mode)
{
  if (digitalRead(SD_CS_PIN) == LOW)
  {
      digitalWrite(SD_CS_PIN, HIGH);
  }
  if (digitalRead(CAN_CS_PIN) == LOW)
  {
    digitalWrite(CAN_CS_PIN, HIGH);
  }
  digitalWrite(RFM95_CS, LOW);
  
  disable_watchdog();
  int count = 0;
  int count2 = 0;
  unsigned long start = millis();
  Serial.println(F("waiting for LoRa data . . ."));
  while (rcv_LoRa_flag == 0)
  {
    if (mode == 4)
    {
      // timeOut in case walang makuhang data LoRa transmitter ~15 minutes
      if ((millis() - start) > LORATIMEOUTMODE2)
      {
        start = millis();
        // send gateway rssi values if no received from transmitter
        get_rssi(get_logger_version());
        rcv_LoRa_flag = 1;
      }
    }
    else if (mode == 5)
    {
      // timeOut in case walang makuhang data LoRa transmitter ~15 minutes
      if ((millis() - start) > LORATIMEOUTMODE3)
      {
        start = millis();
        // send gateway rssi values if no received from transmitter
        get_rssi(get_logger_version());
        rcv_LoRa_flag = 1;
      }
    }
    else
    {
      // timeOut in case walang makuhang data LoRa transmitter ~4 minutes 260 000
      if ((millis() - start) > LORATIMEOUT)
      {
        Serial.println(F("Time out reached."));
        start = millis();
        get_rssi(get_logger_version());
        rcv_LoRa_flag = 1;
      }
    }

    if (rf95.available())
    {
      // Should be a message for us now
      if (rf95.recv(buf, &len2))
      {
        int i = 0;
        for (i = 0; i < len2; ++i)
        {
          received[i] = (uint8_t)buf[i];
        }
        received[i] = (uint8_t)'\0';

        if (strstr(received, ">>"))
        { /*NOT LoRa: 0, 2, 6, 7*/
          flashLed(LED_BUILTIN, 3, 60);
          if (mode == 1 || mode == 3 || mode == 4 || mode == 5 || mode == 10)
          {
            /*remove 1st & 2nd character*/
            for (byte i = 0; i < strlen(received); i++)
            {
              received[i] = received[i + 2];
            }
            send_thru_gsm(received, get_serverNum_from_flashMem());

            //print RSSI values
            tx_RSSI = (rf95.lastRssi(), DEC);
            Serial.print("RSSI: ");
            Serial.println(tx_RSSI);
          }
          else
          {
            Serial.print("Received Data: ");
            Serial.println(received);
            //print RSSI values
            tx_RSSI = (rf95.lastRssi(), DEC);
            Serial.print("RSSI: ");
            Serial.println(tx_RSSI);
          }
        }
        /*
        else if (received, "STOPLORA")
        {
          // function is working as of 03-12-2020
          count++;
          if( mode == 4)
          {
            Serial.println("Version 4: STOPLORA");
            if (count >= 2)
            {
              Serial.print("count: ");
              Serial.println(count);
              Serial.println("Recieved STOP LoRa.");
              count = 0;
            }
          }
          else
          {
            Serial.print("count: ");
            Serial.println(count);
            Serial.println("Recieved STOP LoRa.");
            count = 0;
            // rcv_LoRa_flag = 1;
          }
        }
        */
        else if (received, "*VOLT:")
        {
          if (mode == 4) // 2 LoRa transmitter
          {
            count2++;
            Serial.print("recieved counter: ");
            Serial.println(count2);

            if (count2 == 1)
            {
              //SENSOR A
              tx_RSSI = (rf95.lastRssi(), DEC);
              Serial.print("RSSI: ");
              Serial.println(tx_RSSI);
              //  parse voltage, MADTB*VOLT:12.33*200214111000
              parse_voltage(received).toCharArray(txVoltage, sizeof(txVoltage));
              Serial.print("TX Voltage A: ");
              Serial.println(txVoltage);
            }
            else if (count2 == 2)
            {
              // SENSOR B
              tx_RSSI_B = (rf95.lastRssi(), DEC);
              Serial.print("RSSI: ");
              Serial.println(tx_RSSI_B);
              parse_voltage(received).toCharArray(txVoltageB, sizeof(txVoltageB));
              Serial.print("TX Voltage B: ");
              Serial.println(txVoltageB);
              delay_millis(500);
              get_rssi(get_logger_version());
              count2 = 0;
              rcv_LoRa_flag = 1;
            }
          }
          else if (mode == 5) // 3 LoRa transmitter
          {
            count2++;
            Serial.print("counter: ");
            Serial.println(count2);

            if (count2 == 1)
            {
              //SENSOR A
              tx_RSSI = (rf95.lastRssi(), DEC);
              Serial.print("RSSI: ");
              Serial.println(tx_RSSI);
              // parse voltage, MADTB*VOLT:12.33*200214111000
              parse_voltage(received).toCharArray(txVoltage, sizeof(txVoltage));
              Serial.print("TX Voltage A: ");
              Serial.println(txVoltage);
            }
            else if (count2 == 2)
            {
              //SENSOR B
              tx_RSSI_B = (rf95.lastRssi(), DEC);
              Serial.print("RSSI: ");
              Serial.println(tx_RSSI);
              //  parse voltage, MADTB*VOLT:12.33*200214111000
              parse_voltage(received).toCharArray(txVoltageB, sizeof(txVoltageB));
              Serial.print("TX Voltage B: ");
              Serial.println(txVoltageB);
            }
            else if (count2 == 3)
            {
              // SENSOR C
              tx_RSSI_C = (rf95.lastRssi(), DEC);
              Serial.print("RSSI: ");
              Serial.println(tx_RSSI_B);
              parse_voltage(received).toCharArray(txVoltageC, sizeof(txVoltageC));
              Serial.print("TX Voltage C: ");
              Serial.println(txVoltageC);
              get_rssi(get_logger_version());
              count2 = 0;
              rcv_LoRa_flag = 1;
            }
          }
          else
          {
            /*only 1 transmitter*/
            tx_RSSI = (rf95.lastRssi(), DEC);
            Serial.print("RSSI: ");
            Serial.println(tx_RSSI);
            //  parse voltage, MADTB*VOLT:12.33*200214111000
            parse_voltage(received).toCharArray(txVoltage, sizeof(txVoltage));
            Serial.print("Received Voltage: ");
            Serial.println(parse_voltage(received));
            get_rssi(get_logger_version());
            rcv_LoRa_flag = 1;
          }
        }
      }
      else
      {
        Serial.println(F("Receive failed"));
        rcv_LoRa_flag = 1;
      }
    }
  } //while (rcv_LoRa_flag == 0); //if NOT same with condition Loop will exit

  count = 0;
  count2 = 0;
  tx_RSSI = 0;
  tx_RSSI_B = 0;
  rcv_LoRa_flag = 0;
  getSensorDataFlag = false;
  txVoltage[0] = '\0';
  txVoltageB[0] = '\0';
  txVoltageC[0] = '\0';
  flashLed(LED_BUILTIN, 3, 80);
  enable_watchdog();
  digitalWrite(RFM95_CS, HIGH);
}

/**RTC Pin interrupt
 * hardware interrupt from RTC
 * microcontroller will wake from sleep
 * execute the process
*/
void wake()
{
  OperationFlag = true;
  //detach the interrupt in the ISR so that multiple ISRs are not called
  detachInterrupt(RTCINTPIN);
}

void ringISR()
{
  gsmRingFlag = true;
  detachInterrupt(GSMINT);
}

/**GATEWAY*RSSI,MAD,MADTA,rssi,voltage,MADTB,,,*200212141406 
  *main logger name, MADTA, MADTB, . . .
  *send data to gsm
*/
void get_rssi(uint8_t mode)
{
  char convertRssi[100];
  char convertRssiB[100];
  char convertRssiC[100];
  char logger_name[200];
  String old_rssi = String(tx_RSSI);
  String old_rssi_B = String(tx_RSSI_B);
  String old_rssi_C = String(tx_RSSI_C);
  old_rssi.toCharArray(convertRssi, 100);
  old_rssi_B.toCharArray(convertRssiB, 100);
  old_rssi_C.toCharArray(convertRssiC, 100);
  readTimeStamp();

  String loggerName = String(get_logger_A_from_flashMem());
  loggerName.replace("\r", "");
  loggerName.remove(3);
  loggerName.toCharArray(logger_name, 200);

  for (int i = 0; i < 250; i++)
    rssiString[i] = 0;
  strncpy(rssiString, "GATEWAY*RSSI,", 13);
  strncat(rssiString, logger_name, sizeof(logger_name));
  strncat(rssiString, ",", 1);
  strncat(rssiString, get_logger_B_from_flashMem(), 20);
  strncat(rssiString, ",", 1);
  strncat(rssiString, convertRssi, 100);
  strncat(rssiString, ",", 1);
  strncat(rssiString, txVoltage, sizeof(txVoltage)); //voltage working 02-17-2020
  // strncat(dataToSend, parse_voltage(received), sizeof(parse_voltage(received)));
  if (mode == 4)
  {
    strncat(rssiString, ",", 1);
    strncat(rssiString, get_logger_C_from_flashMem(), 20);
    strncat(rssiString, ",", 1);
    strncat(rssiString, convertRssiB, 100);
    strncat(rssiString, ",", 1);
    strncat(rssiString, txVoltageB, sizeof(txVoltageB)); //voltage working 02-17-2020
    // strncat(dataToSend, parse_voltage_B(received), sizeof(parse_voltage_B(received)));
  }
  else if (mode == 5)
  {
    strncat(rssiString, ",", 1);
    strncat(rssiString, get_logger_C_from_flashMem(), 20);
    strncat(rssiString, ",", 1);
    strncat(rssiString, convertRssiB, 100);
    strncat(rssiString, ",", 1);
    strncat(rssiString, txVoltageB, sizeof(txVoltageB)); //voltage working 02-17-2020
    strncat(rssiString, ",", 1);
    strncat(rssiString, get_logger_D_from_flashMem(), 20); //sensorD
    strncat(rssiString, ",", 1);
    strncat(rssiString, convertRssiC, 100);
    strncat(rssiString, ",", 1);
    strncat(rssiString, txVoltageC, sizeof(txVoltageC));
  }
  strncat(rssiString, ",*", 2);
  strncat(rssiString, Ctimestamp, sizeof(Ctimestamp));
  delay_millis(500);
  send_thru_gsm(rssiString, get_serverNum_from_flashMem());
}

/**
 * Get data allocated from flash memory
*/
char *stationName_from_flashMem()
{
  String get_cmd;
  char new_cmd[10];
  sensCommand = passCommand.read();
  get_cmd = sensCommand.stationName;
  get_cmd.replace("\r", "");
  get_cmd.toCharArray(new_cmd, 10);
  return new_cmd;
}

char *get_logger_A_from_flashMem()
{
  loggerName = flashLoggerName.read();
  return loggerName.sensorA;
}

char *get_logger_B_from_flashMem()
{
  loggerName = flashLoggerName.read();
  return loggerName.sensorB;
}

char *get_logger_C_from_flashMem()
{
  loggerName = flashLoggerName.read();
  return loggerName.sensorC;
}

char *get_logger_D_from_flashMem()
{
  loggerName = flashLoggerName.read();
  return loggerName.sensorD;
}

String get_serverNum_from_flashMem()
{
  String flashNum;
  flashServerNumber = newServerNum.read();
  flashNum = flashServerNumber.inputNumber;
  flashNum.replace("\r", "");
  return flashNum;
}

char *get_password_from_flashMem()
{
  flashPassword = flashPasswordIn.read();
  return flashPassword.keyword;
}

/**
 * 0 - default for GSM sending
 * 1 - send to LoRa with >> and << added to data
*/
void send_rain_data(uint8_t sendTo)
{
  disable_watchdog();
  char temp[10];
  char volt[10];
  readTimeStamp();

  for (int i = 0; i < DATALEN; i++)
    dataToSend[i] = 0;
  /* MADTAW,32.00,0.00,0.82,99,200415171303 
      station name, temperature, raintips, input voltage, gsm csq */
  if (sendTo == 1)
  {
    strncpy(dataToSend, ">>", 2);
    strncat((dataToSend), (get_logger_A_from_flashMem()), (20));
  }
  else
  {
    strncpy((dataToSend), (get_logger_A_from_flashMem()), (20));
  }
  // strncpy((dataToSend), (get_logger_A_from_flashMem()), (20));
  strncat(dataToSend, "W", 1);
  strncat(dataToSend, ",", 1);

  snprintf(temp, sizeof temp, "%.2f", readTemp());
  strncat(dataToSend, temp, sizeof(temp));
  strncat(dataToSend, ",", 1);

  snprintf(sendRainTip, sizeof sendRainTip, "%.2f", rainTips);
  strncat(dataToSend, sendRainTip, sizeof(sendRainTip));
  strncat(dataToSend, ",", 1);

  snprintf(volt, sizeof volt, "%.2f", readBatteryVoltage(get_logger_version()));
  strncat(dataToSend, volt, sizeof(volt));

  strncat(dataToSend, ",", 1);
  strncat(dataToSend, readCSQ(), sizeof(readCSQ()));
  // strncat(dataToSend, _csq, sizeof(_csq));
  strncat(dataToSend, ",", 1);
  strncat(dataToSend, Ctimestamp, sizeof(Ctimestamp));
  if (get_logger_version() == 6)
  {
    strncat(dataToSend, "<<", 2);
  }
  delay_millis(500);
  if (sendTo == 1)
  {
    send_thru_lora(dataToSend);
  }
  else
  {
    send_thru_gsm(dataToSend, get_serverNum_from_flashMem());
    delay_millis(500);
    resetRainTips();
  }
  enable_watchdog();
}

void flashLed(int pin, int times, int wait)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(pin, HIGH);
    delay_millis(wait);
    digitalWrite(pin, LOW);
    if (i + 1 < times)
    {
      delay_millis(wait);
    }
  }
}

char *read_batt_vol(uint8_t ver)
{
  char volt[10];
  for (int i = 0; i < 200; i++)
    voltMessage[i] = 0;
  // dtostrf((readBatteryVoltage(get_logger_version())), 4, 2, volt);
  snprintf(volt, sizeof volt, "%.2f", readBatteryVoltage(ver));
  readTimeStamp();

  if (ver == 6)
  {
    strncpy(voltMessage, ">>", 2);
  }
  else
  {
    strncpy(voltMessage, get_logger_A_from_flashMem(), 20);
  }
  strncat(voltMessage, "*VOLT:", 7);
  strncat(voltMessage, volt, sizeof(volt));
  strncat(voltMessage, "*", 1);
  if (ver == 6)
  {
    strncat(voltMessage, Ctimestamp, sizeof(Ctimestamp));
    strncat(voltMessage, "<<", 2);
  }
  else
  {
    strncat(voltMessage, Ctimestamp, sizeof(Ctimestamp));
  }
  return voltMessage;
}

// Measure battery voltage using divider on Feather M0
float readBatteryVoltage(uint8_t ver)
{
  float measuredvbat;
  if ((ver == 3) || (ver == 9) || (ver == 10) || (ver == 11))
  {
    measuredvbat = analogRead(VBATPIN); //Measure the battery voltage at pin A7
    measuredvbat *= 2;                  // we divided by 2, so multiply back
    measuredvbat *= 3.3;                // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024;               // convert to voltage
    measuredvbat += 0.28;               // add 0.7V drop in schottky diode
  }
  else
  {
    /* Voltage Divider 1M and  100k */
    measuredvbat = analogRead(VBATEXT);
    measuredvbat *= 3.3;    // reference voltage
    measuredvbat /= 1024.0; // adc max count
    measuredvbat *= 11.0;   // (100k+1M)/100k
  }
  return measuredvbat;
}

/*
  ** interrupts EIC
  EXTERNAL_INT_2: A0, A5, 10
  EXTERNAL_INT_4: A3, 6
  EXTERNAL_INT_5: A4, 7
  EXTERNAL_INT_6: 8, SDA
  EXTERNAL_INT_7: 9, SCL
  EXTERNAL_INT_9: A2, 3
  EXTERNAL_INT_10: TX, MOSI
  EXTERNAL_INT_11: RX, SCK
*/
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

void getPwrdFromMemory()
{
  sensCommand = passCommand.read();
}

/**
 * Get custom due sensor data.
 * ~5 minutes timeout if no data read.  
 * mode in sending data: 1-gsm ; 0 - LoRa(defualt) ; 2 - V5 logger
*/
void get_Due_Data(uint8_t mode, String serverNum)
{
  DUESerial.begin(DUEBAUD);
  disable_watchdog();
  unsigned long start = millis();

  /*Serial.println("starting delay please wait . . .");
  // delay 1 minute before getting due data
  delay(DUEDELAY);
  Serial.println("DUE delay reached!");*/

  readTimeStamp();
  turn_ON_due(get_logger_version());
  delay_millis(500);

  sensCommand = passCommand.read();
  command[0] = '\0';
  strncpy((command), (sensCommand.senslopeCommand), (10));
  strncat(command, "/", 1);
  strncat(command, Ctimestamp, sizeof(Ctimestamp));
  Serial.println(command);
  DUESerial.write(command);
  Serial.println(F("Waiting for sensor data. . ."));

  while (customDueFlag == 0)
  {
    // timeOut in case walang makuhang data sa due
    if ((millis() - start) > DUETIMEOUT)
    {
      start = millis();
      no_data_from_senslope(mode);
      customDueFlag = 1;
    }

    for (int i = 0; i < 250; ++i)
      streamBuffer[i] = 0x00;
    DUESerial.readBytesUntil('\n', streamBuffer, 250);
    delay_millis(500);

    if (strstr(streamBuffer, ">>"))
    {
      if (strstr(streamBuffer, "*"))
      {
        Serial.println(F("Getting sensor data. . ."));
        if (mode == 0 || mode == 1)
        {
          /**
            * Remove 1st and 2nd character data in string
            * Not needed in GSM mode 
          */
          for (byte i = 0; i < strlen(streamBuffer); i++)
          {
            streamBuffer[i] = streamBuffer[i + 2];
          }
          // send_thru_gsm(streamBuffer, get_serverNum_from_flashMem());
          send_thru_gsm(streamBuffer, serverNum);
          flashLed(LED_BUILTIN, 2, 100);
          DUESerial.write("OK");
        }
        else if (mode == 6 || mode == 7)
        {
          strncat(streamBuffer, "<<", 2);
          delay(10);
          send_thru_lora(streamBuffer);
          flashLed(LED_BUILTIN, 2, 100);
          DUESerial.write("OK");
          // send_thru_lora(streamBuffer);
          // flashLed(LED_BUILTIN, 2, 100);
          // DUESerial.write("OK");
        }
        else
        {
          send_thru_lora(streamBuffer);
          flashLed(LED_BUILTIN, 2, 100);
          DUESerial.write("OK");
        }
      }
      else
      {
        //maglagay ng counter max 5 then exit
        Serial.println(F("Message incomplete"));
        DUESerial.write("NO");
      }
    }
    else if (strstr(streamBuffer, "STOPLORA"))
    {
      /*if (mode == 0 || mode == 2)
      {
        delay(1000);
        send_thru_lora(read_batt_vol(mode));
        delay(1500); //needed for the gsm to wait until sending
        send_thru_lora("STOPLORA");
      }*/
      Serial.println(F("Done getting DUE data!"));
      streamBuffer[0] = '\0';
      customDueFlag = 1;
    }
  }
  if (mode == 2 || mode == 6 || mode == 7 || mode == 8)
  {
    delay_millis(2000);
    send_thru_lora(read_batt_vol(mode));
  }
  turn_OFF_due(get_logger_version());
  DUESerial.end();
  flashLed(LED_BUILTIN, 4, 90);
  customDueFlag = 0;
  getSensorDataFlag = true;
  enable_watchdog();
}

/**
 * Sends no data from senslope if no data available
 * mode :     1 - gsm
 * default:   0 - LoRa
 *Serial.println("[0] Sendng data using GSM only");
  Serial.println("[1] Version 5 datalogger LoRa with GSM");
  Serial.println("[2] LoRa transmitter for version 5 datalogger");
  Serial.println("[3] Gateway Mode with only ONE LoRa transmitter");
  Serial.println("[4] Gateway Mode with TWO LoRa transmitter");
  Serial.println("[5] Gateway Mode with THREE LoRa transmitter");
  Serial.println("[6] LoRa transmitter for Raspberry Pi");
  Serial.println("[7] Sends rain gauge data via LoRa");
*/
void no_data_from_senslope(uint8_t mode)
{
  readTimeStamp();
  sensCommand = passCommand.read(); //read from flash memory
  Serial.println(F("No data from senslope"));
  streamBuffer[0] = '\0';

  if (mode == 1 || mode == 0)
  {
    strncpy((streamBuffer), (get_logger_A_from_flashMem()), (20));
  }
  else
  {
    strncpy(streamBuffer, ">>", 2);
    strncat((streamBuffer), (get_logger_A_from_flashMem()), (20));
  }

  strncat(streamBuffer, "*NODATAFROMSENSLOPE*", 22);
  strncat(streamBuffer, Ctimestamp, sizeof(Ctimestamp));

  if (mode == 1 || mode == 0)
  {
    send_thru_gsm(streamBuffer, get_serverNum_from_flashMem());
  }
  else if (mode == 2)
  {
    send_thru_lora(streamBuffer);
  }
  else
  {
    strncat(streamBuffer, "<<", 2);
    send_thru_lora(streamBuffer);
  }
  customDueFlag = 1;
}

void turn_ON_due(uint8_t mode)
{
  Serial.println(F("Turning ON Custom Due. . ."));
  digitalWrite(COLSW, HIGH);
  delay_millis(100);
}

void turn_OFF_due(uint8_t mode)
{
  Serial.println(F("Turning OFF Custom Due. . ."));
  digitalWrite(COLSW, LOW);
  delay_millis(100);
}

void rainISR()
{
  detachInterrupt(digitalPinToInterrupt(RAININT));
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > DEBOUNCE_TIME)
  {
    if (get_rainCollector_type() == 0)
    {
      rainTips += 0.5;
    }
    else if (get_rainCollector_type() == 1)
    {
      rainTips += 0.2;
    }
    else
    {
      rainTips += 1;
    }
  }
  last_interrupt_time = interrupt_time;
  if (OperationFlag == true || debug_flag == 1)
  {
    attachInterrupt(RAININT, rainISR, FALLING);
  }
  else
  {
    rainFallFlag = true;
  }
}

void resetRainTips()
{
  rainTips = 0.00;
  delay_millis(75);
  dtostrf(rainTips, 3, 2, sendRainTip); //convert rainTip to char
  Serial.print("Rain tips: ");
  Serial.println(rainTips);
}

void send_rain_tips()
{
  send_rain_data(0);
}

String parse_voltage(char *toParse)
{
  int i = 0;
  String parse_volt;

  //MADTB*VOLT:12.33*200214111000
  char *buff = strtok(toParse, ":");
  while (buff != 0)
  {
    char *separator = strchr(buff, '*');
    if (separator != 0)
    {
      *separator = 0;
      if (i == 1) //2nd appearance
      {
        parse_volt = buff;
      }
      i++;
    }
    buff = strtok(0, ":");
  }
  return parse_volt;
}

/* Function: hard_code
   Hard code node ids for specific logger

   Parameters:

      str1 - Node Ids
      str2 - Logger name ex. AGBSB
      g_num_of_nodes - Counts number of nodes

   Returns:

      String of node ids plus the logger name

   See Also:

      <process_column_ids>  <get_value_from_line>
 
 */
void hard_code()
{
  //String str1 = "column1 = 2267,1649,1663,1798,1904,1977,1993,1998,2056,2057,2061,2150,2199,2201,2232,2249,2256,2273,2323,2346,1748,2285,2208,2242,2311,2378,2384,2389,2398,2418,2427,2461,2462,2476,2386";
  String str1 = "column1 = 2416,2458";
  //String str1 = "column1 = 1835,1886,2495,1924,2055";
  String str2 = "MasterName = PHITA";

  g_num_of_nodes = process_column_ids(str1);
  get_value_from_line(str2).toCharArray(g_mastername, 6);
}

//Function: getATCommand
// Take in-line serial input and execute AT command
void getATCommand()
{
  String serial_line, command, extra_parameters;
  char converted[5] = {};
  char padded[5] = {};
  char atSwitch;
  int i_equals = 0; // index of equal sign
  if (Serial.available())
  {
    do
    {
      serial_line = Serial.readStringUntil('\r\n');
    } while (serial_line == "");

    serial_line.toUpperCase();
    serial_line.replace("\r", "");

    // echo command if ate is set, default true
    if (ate)
      Serial.println(serial_line);
    {
      i_equals = serial_line.indexOf('=');
    }
    if (i_equals == -1)
      command = serial_line;
    else
      command = serial_line.substring(0, i_equals);

    for (int i = 0; i < VDATASIZE; ++i)
    {
      g_volt[i] = (float)0;
    }

    atSwitch = serial_line.charAt(0);
    switch (atSwitch)
    {
    case '?':
      print_due_command2();
      break;
    case 'A':
    { //ATGETSENSORDATA
      read_data_from_column(g_final_dump, g_sensor_version, 1);
      int g_volt_size = sizeof(g_volt) / sizeof(g_volt[0]);
      //sample_send();
      /*
          for (int x=0; x < g_volt_size; x++) {
            Serial.println(g_volt[x]);
          }
          */
      Serial.println(OKSTR);
    }
    break;
    case 'B':
    { //AT+POLL
      read_data_from_column(g_final_dump, g_sensor_version, 2);
      Serial.println(F(g_final_dump));
      if (b64 == 1)
      {
        b64_build_text_msgs(comm_mode, g_final_dump, text_message);
      }
      else
      {
        build_txt_msgs(comm_mode, g_final_dump, text_message);
      }
      Serial.println(OKSTR);
    }
    break;
    case 'C':
    { //ATSD){
      String conf;
      init_sd();
      open_config();
      Serial.println(F(OKSTR));
    }
    break;
    case 'D':
    { //AT+TIMENOW
      Serial.print("ARQ Time String: ");
      Serial.println(g_timestamp);
    }
    break;
    case 'E':
    { //AT+RTC
      extra_parameters = serial_line.substring(i_equals + 1);
      //set_rtc_time(extra_parameters);
      Serial.print("");
    }
    break;
    case 'F':
    { //ATECMDTRUE
      ate = true;
      Serial.println(OKSTR);
    }
    break;
    case 'G':
    { //ATECMDFALSE
      ate = false;
      Serial.println(OKSTR);
    }
    break;
    case 'H':
    { //ATCMD
      Serial.println(OKSTR);
    }
    break;
    case 'I':
    { //AT+S
      get_data(11, 1, g_final_dump);
      get_data(12, 1, g_final_dump);
    }
    break;
    case 'J':
    { //AT+B64
      // g_timestamp = b64_timestamp(g_timestamp);
      // Serial.println(g_timestamp);
      extra_parameters = serial_line.substring(i_equals + 1);
      to_base64(extra_parameters.toInt(), converted);
      pad_b64(5, converted, padded);
      Serial.print("converted and padded: ");
      Serial.print(padded);
    }
    break;
    case 'K':
    { //AT+SWITCHB64
      if (b64)
      {
        b64 = 0;
      }
      else
      {
        b64 = 1;
      }
      Serial.println(F("Toggled b64 operations."));
      get_data(11, 1, g_final_dump);
      get_data(12, 1, g_final_dump);
      get_data(22, 1, g_final_dump);
      get_data(10, 1, g_final_dump);
      get_data(13, 1, g_final_dump);
      // Serial.println(g_final_dump);

      g_timestamp = String("180607142000");
      b64_build_text_msgs(comm_mode, g_final_dump, text_message);
      // Serial.println(text_message);
      // Serial.println(g_final_dump);
      Serial.println("OK");
    }
    break;
    case 'L':
    { //ATSNIFFCAN
      while (true)
      {
        Serial.println(OKSTR);
      }
    }
    break;
    case 'M': //AT+CURRENT
      read_current();
      break;
    case 'N': //AT+VOLTAGE
      read_voltage();
      break;
    case 'O':
    { //AT+TIMESTAMPPMM
    }
    break;
    case 'P':
    { //AT+SEND
      Serial.print(text_message);
      char *token1 = strtok(text_message, g_delim);
      while (token1 != NULL)
      {
        Serial.println(token1);
        //send_data(false, token1);
        token1 = strtok(NULL, g_delim);
      }
    }
    break;
    case 'Q':
    { //AT+PIEZO
      get_data(255, 1, g_final_dump);
      Serial.print("g_final_dump:::");
      Serial.println(g_final_dump);
    }
    break;
    case 'R':
    { //AT+XBEE
    }
    break;
    case 'S':
    { //AT+LOOPSEND
      while (1)
      {
        Serial.println("sent.");
        send_command(3, 3);
        delay(1000);
      }
    }
    break;
    case 'T':
    { //AT+LOOPBACK
      //serial_loopback();
      Serial.println(OKSTR);
    }
    break;
    case 'U':
    { //AT+LOOPBACK2
      //serial_loopback2();
      Serial.println(OKSTR);
    }
    break;
    case 'V':
    { //ATDUMP
      Serial.print("g_final_dump: ");
      Serial.println(g_final_dump);
      Serial.println(OKSTR);
    }
    break;
    case 'W':
    { //Change sensor version format W=new_version
      pinMode(SD_CS_PIN, OUTPUT);
      extra_parameters = serial_line.substring(i_equals + 1);
      extra_parameters.trim();
      change_sensor_version(extra_parameters.toInt());
    }
    break;
    case 'X':
    {
      root = SD.open("config.txt");
      if (root)
      {
        int data;
        while ((data = root.read()) >= 0)
        {
          Serial.write(data);
        }
        Serial.println(" CONFIG.TXT end of file");
      }
      else
      {
        Serial.println("error opening config.txt");
      }
      root.close();
    }
    break;
    case 'Z':
    {
      dumpSDtoPC();
      //Serial.print("OK");
    }
    break;
    default:
      Serial.println(ERRORSTR);
      break;
    }
  }
  else
    return;
}

/* 
  Function: operation

    Processes the data to be sent from the sensors to the radio comms.
  
  Parameters:
  
    mode - integer that determines the v5 logger mode of operation (lora or gsm).

    serverNum - string that holds the server number for gsm operations. 
  
  Returns:
  
    n/a
  
  See Also:
  
    <loop>

  Global Variables:

    <g_final_dump>, <g_sensor_version>, <text_message>
*/
void operation(uint8_t mode, String serverNum)
{
  disable_watchdog();
  readTimeStamp();
  //turn_ON_due(get_logger_version());
  delay_millis(500);

  sensCommand = passCommand.read();
  command[0] = '\0';
  strncpy((command), (sensCommand.senslopeCommand), (10));
  strncat(command, "/", 1);
  strncat(command, Ctimestamp, sizeof(Ctimestamp));
  Serial.println(command);
  //Serial.println("Waiting for sensor data. . .");
  int sensor_type = parse_cmd(command);

  int counter = 0;
  int num_of_tokens = 0;
  read_data_from_column(g_final_dump, g_sensor_version, sensor_type); // matagal ito.
  Serial.print(F("g_final_dump: "));
  Serial.println(F(g_final_dump));
  if (b64 == 1)
  {
    b64_build_text_msgs(comm_mode, g_final_dump, text_message);
  }
  else
  {
    build_txt_msgs(comm_mode, g_final_dump, text_message);
  }

  Serial.println(text_message);
  char *token1 = strtok(text_message, "~");

  while (token1 != NULL)
  {
    Serial.print("Sending ::::");
    Serial.println(token1);
    if (strstr(token1, ">>"))
    {
      if (strstr(token1, "*"))
      {
        Serial.println(F("Getting sensor data. . ."));
        if (mode == 0 || mode == 1)
        {
          for (byte i = 0; i < strlen(token1); i++)
          {
            token1[i] = token1[i + 2];
          }
          send_thru_gsm(token1, serverNum);
          flashLed(LED_BUILTIN, 2, 100);
        }
        else if (mode == 6 || mode == 7)
        {
          strncat(token1, "<<", 2);
          delay(10);
          send_thru_lora(token1);
          flashLed(LED_BUILTIN, 2, 100);
        }
        else
        {
          send_thru_lora(token1);
          flashLed(LED_BUILTIN, 2, 100);
        }
      }
    }
    token1 = strtok(NULL, g_delim);
    num_of_tokens = num_of_tokens + 1;
  }
  enable_watchdog();  
}

//Function: getArguments
// Read in-line serial AT command.
void getArguments(String at_cmd, String *arguments)
{
  int i_from = 0, i_to = 0, i_arg = 0;
  bool f_exit = true;
  String sub;

  i_from = at_cmd.indexOf('=');

  do
  {
    i_to = at_cmd.indexOf(',', i_from + 1);
    if (i_to < 0)
    {
      sub = at_cmd.substring(i_from + 1);
      f_exit = false;
    }
    else
      sub = at_cmd.substring(i_from + 1, i_to);

    arguments[i_arg] = sub;
    i_from = i_to;
    i_arg += 1;

  } while (f_exit);
}

//Group: Sensor Data Gathering Functions
/* 
  Function: read_data_from_column

    Sequentially samples data from the sensor column.
  
  Parameters:
  
    column_data - char array that will store the raw data received from the sensors

    sensor_version - integer that states the sensor version

    types - integer that determines the kind of data requested from the sensor
  
  Returns:
  
    n/a
  
  See Also:
  
    <operation>
*/
void read_data_from_column(char *column_data, int sensor_version, int sensor_type)
{
  vdata_flag = true;
  if (sensor_version == 2)
  {
    get_data(32, 1, column_data);
    get_data(33, 1, column_data);
    if (sensor_type == 2)
    {
      get_data(111, 1, column_data);
      get_data(112, 1, column_data);
    }
  }
  else if (sensor_version == 3)
  {
    get_data(11, 1, column_data);
    get_data(12, 1, column_data);
    get_data(22, 1, column_data);
    if (sensor_type == 2)
    {
      get_data(10, 1, column_data);
      get_data(13, 1, column_data);
    }
  }
  else if (sensor_version == 4)
  {
    get_data(41, 1, column_data);
    get_data(42, 1, column_data);
    get_data(22, 1, column_data);
  }
  else if (sensor_version == 1)
  {
    get_data(256, 1, column_data); //Added for polling of version 1
  }
  if (has_piezo)
  {
    get_data(255, 1, column_data);
  }

  sensor_voltage_ver();
  vdata_flag = false;
  //sample_send();
}

//Function: read_current()
// Reads the current draw from the onboard ina219.
void read_current()
{
  turn_on_column();
  delay(1000);
  float current_mA = 0;
  current_mA = ina219.getCurrent_mA();
  Serial.print("Current:       ");
  Serial.print(current_mA);
  Serial.println(" mA");
  delay(1000);
  turn_off_column();
}
/* 
  Function: read_voltage()

    Reads the *Bus Voltage*, *Shunt Voltage*, *Load Voltage* from the onboard ina219.
*/
void read_voltage()
{
  float shuntvoltage = 0;
  float busvoltage = 0;
  float loadvoltage = 0;

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  loadvoltage = busvoltage + (shuntvoltage / 1000);

  Serial.print("Bus Voltage:   ");
  Serial.print(busvoltage);
  Serial.println(" V");
  Serial.print("Shunt Voltage: ");
  Serial.print(shuntvoltage);
  Serial.println(" mV");
  Serial.print("Load Voltage:  ");
  Serial.print(loadvoltage);
  Serial.println(" V");
}

/* 
  Function: getTimestamp()

    Extract the timestamp from either the ARQCMD or power module.
  
  Parameters:
  
    mode - integer, 1 from arq, 2 if from xbee 
  
  Returns:
  
    timestamp or String "0TIMESTAMP"
  
  See Also:
  
    <poll_data>
*/
String getTimestamp(char communication_mode[])
{

  if (communication_mode == 0)
  { //internal rtc
    return g_timestamp;
  }
  else if (strcmp(comm_mode, "ARQ") == 0)
  { // ARQ
    // Serial.print("g_timestamp: ");
    // Serial.println(g_timestamp);
    return g_timestamp;
  }
  else
  {
    return String("0TIMESTAMP");
  }
}

/* 
  Function: set_rtc_time()

    Set the time of the customDue internal rtc.
  
  Parameters:
  
    time_str - String  
  
  Returns:
  
    timestamp or String "0TIMESTAMP"
  
  See Also:
  
    <poll_data>
*/
void set_rtc_time(String time_string)
{
  /*
  int hours,minutes,seconds,day,month,year;
  
  year = time_string.substring(0,2).toInt();
  month = time_string.substring(2,4).toInt();
  day = time_string.substring(4,6).toInt();
  hours = time_string.substring(6,8).toInt();
  minutes = time_string.substring(8,10).toInt();
  seconds = time_string.substring(10,12).toInt();
  if (ENABLE_RTC){
    RTCDue rtc(XTAL);
    rtc.setTime(hours, minutes, seconds);
    rtc.setDate(day, month, year);
  }
  */
}
//Group: Command Gathering Function


/* 
  Function: parse_cmd

    Determine the types of data to be sampled from the sensors based on input command_string.

  Parameters:
  
    command_string - char* to char array that contains the command to be interpreted
  
  Returns:
  
    1 - tilt

    2 - tilt and soil moisture
  
  See Also:
  
    - <wait_arq_cmd> <wait_xbee_cmd>
*/
int parse_cmd(char *command_string)
{
  String serial_line;
  String command, temp_time;
  char c_serial_line[30];
  char *pch;
  char cmd[] = "CMD6";
  int cmd_index, slash_index;

  serial_line = String(command_string);
  Serial.println(serial_line);

  if ((pch = strstr(command_string, cmd)) != NULL)
  {
    if (*(pch + strlen(cmd)) == 'S')
    {                                          // SOMS + TILT
      cmd_index = serial_line.indexOf('S', 6); // ARQCMD has 6 characters
      temp_time = serial_line.substring(cmd_index + 1);
      slash_index = temp_time.indexOf('/');
      temp_time.remove(slash_index, 1);
      g_timestamp = temp_time;
      Serial.print("g_timestamp: ");
      Serial.println(g_timestamp);
      return 2;
    }
    else if (*(pch + strlen(cmd)) == 'T')
    {                                          // TILT Only
      cmd_index = serial_line.indexOf('T', 6); // ARQCMD has 6 characters
      temp_time = serial_line.substring(cmd_index + 1);
      slash_index = temp_time.indexOf('/');
      temp_time.remove(slash_index, 1);
      g_timestamp = temp_time;
      Serial.print("g_timestamp: ");
      Serial.println(g_timestamp);
      return 1;
    }
    else
    {
      Serial.println("wait_arq_cmd returned 0");
      return 0;
    }
  }
}

//Group: Data parsing functions
/* 
  Function: build_txt_msgs

    Build the text messages to be sent.

    * Splits the data though the delimiter defined by *<g_delim>* 

    * The identifier, and cutoff are identified based on the type of data.

    * A special case for the piezometer data is also considered.

    * Extra characters are removed via <remove_extra_characters>.

    * Data is stored in SD card via <writeData>.

    * The following are appended to the data split as text messages to be sent:

        * delimiters
        
        * mastername

        * padding for the numerator /  denominator

        * identifier

        * timestamp


    * The appended data is copied per character to a temporary buffer.

    * The temporary buffer is split per message.

    * The pads are overwritten with the proper text message length,
    numerator and denominator.
  
    * These are then stored to the *destination* parameter.

  Parameters:
  
    source - char array that contains the aggregated data

    destination - char array that will contain the text messages to be sent separated by
    the delimiter *<g_delim>*
  
  Returns:
  
    n/a
  
  See Also:
  
    - <check_cutoff>
    
    - <remove_extra_characters>
    
    - <writeData>
*/
void build_txt_msgs(char mode[], char *source, char *destination)
{
  char *token1, *token2;
  char dest[5000] = {};
  char idf = '0';
  char identifier[2] = {};
  char temp[6];
  char temp_id[5];
  char master_name[8] = "";
  int cutoff = 0, num_text_to_send = 0, num_text_per_dtype = 0;
  int name_len = 0, char_cnt = 0, c = 0;
  int i, j;
  int token_length = 0;
  char pad[12] = "___________";
  char pad2[12] = "___________";

  //Serial.println(F(g_final_dump)); //Print final dump data
  for (int i = 0; i < 5000; i++)
  {
    destination[i] = '\0';
  }

  String timestamp = getTimestamp(mode);
  char Ctimestamp[13] = "";

  for (int i = 0; i < 12; i++)
  {
    Ctimestamp[i] = timestamp[i];
  }

  //Ctimestamp[12] = '\0';
  //Serial.print(g_delim);
  token1 = strtok(source, g_delim);
  if (vc_flag == true)
  {
    num_text_to_send = 1;
  }
  while (token1 != NULL)
  {
    c = 0;
    idf = check_identifier(token1, 4);
    identifier[0] = idf;
    identifier[1] = '\0';
    cutoff = check_cutoff(idf);
    remove_extra_characters(token1, idf);

    writeData(timestamp, String(token1));
    num_text_per_dtype = (strlen(token1) / cutoff);
    if ((strlen(token1) % cutoff) != 0)
    {
      num_text_per_dtype++;
    }

    if (g_sensor_version == 1)
    {
      name_len = 8;
      strncpy(master_name, g_mastername, 4);
      strncat(master_name, "DUE", 4);
    }
    else if (idf == 'p')
    {
      name_len = 8;
      strncpy(master_name, g_mastername, 6);
      strncat(master_name, "PZ", 2);
    }
    else
    {
      name_len = 6;
      strncpy(master_name, g_mastername, 6);
    }

    token_length = strlen(token1);

    for (i = 0; i < num_text_per_dtype; i++)
    {
      strncat(dest, pad, 11);
      strncat(dest, master_name, name_len);
      strncat(dest, "*", 2);
      if (g_sensor_version != 1)
      { // except piezo and v1 // or idf != 'p' (Tinangal ko muna ito)
        strncat(dest, identifier, 2);
        strncat(dest, "*", 2);
      }
      for (j = 0; j < (cutoff); j++)
      {
        strncat(dest, token1, 1);
        c++;
        token1++;
        if (c == (token_length))
        {
          break;
        }
      }
      strncat(dest, "<<", 2);
      strncat(dest, g_delim, 1);
    }

    num_text_to_send = num_text_to_send + num_text_per_dtype;
    token1 = strtok(NULL, g_delim);
  }
  token2 = strtok(dest, g_delim);
  c = 0;
  while (token2 != NULL)
  {
    c++;
    char_cnt = strlen(token2) + name_len - 24;
    idf = check_identifier(token1, 2);
    identifier[0] = idf;
    identifier[1] = '\0';
    sprintf(pad, "%03d", char_cnt);
    strncat(pad, ">>", 3);
    sprintf(temp, "%02d/", c);
    strncat(pad, temp, 4);
    sprintf(temp, "%02d#", num_text_to_send);
    strncat(pad, temp, 4);
    strncpy(token2, pad, 11);
    // strncat(token2,"<<",3);
    Serial.println(token2);
    strncat(destination, token2, strlen(token2));
    strncat(destination, g_delim, 2);
    token2 = strtok(NULL, g_delim);
  }

  if (vc_flag == true)
  { //for testing
    char temp3[100] = "";

    strncat(temp3, master_name, strlen(master_name));
    strncat(temp3, "*m*", 4);
    strncat(temp3, g_build, strlen(g_build));
    writeData(timestamp, g_build);
    if (strcmp(comm_mode, "ARQ") == 0)
    {
      sprintf(pad2, "%03d", strlen(g_build)); //message count
      strncat(pad2, ">>", 3);
      sprintf(temp, "%02d/%02d#", num_text_to_send, num_text_to_send);
      strncat(pad2, temp, 6); //000>>00/00#
      strncat(g_build_final, pad2, strlen(pad2));
      strncat(g_build_final, temp3, strlen(temp3));
      strncat(g_build_final, "<<", 3);
    }
    Serial.println(g_build_final);
    //strncpy(vc_text, pad, strlen(pad));
    strncat(destination, g_build_final, strlen(g_build_final));
    strncat(destination, g_delim, 2);

    Serial.println(F(destination));
    vc_flag = false;
    for (int i = 0; i < 500; i++)
    {
      g_build_final[i] = '\0';
    }
  }
  if (destination[0] == '\0')
  {
    no_data_parsed(destination);
    writeData(timestamp, String("*0*ERROR: no data parsed"));
  }
  //Serial.println(destination);
  Serial.println(F("================================="));
}

/* 
  Function: remove_extra_characters

    Remove specific characters unnecessary for data interpretation.
  
  Parameters:
  
    columnData - char array that contains data split by datatype.
    cmd - integer that determines which format is executed
  
  Returns:
  
    n/a
  
  See Also:
  
    <poll_data>
*/
void remove_extra_characters(char *columnData, char idf)
{
  int i = 0, cmd = 1;
  char pArray[2500] = "";
  int initlen = strlen(columnData);
  char *start_pointer = columnData;

  // dagdagan kapag kailangan
  if (idf == 'd')
  {
    cmd = 9;
  }
  else if ((idf == 'x') || (idf == 'y'))
  {
    cmd = 1;
  }
  else if ((idf == 'p'))
  {
    cmd = 10;
  }
  else if (idf == 'b')
  {
    cmd = 3;
  }
  else if (idf == 'c')
  {
    cmd = 3;
  }
  else if (g_sensor_version == 1)
  {
    cmd = 4;
  }

  for (i = 0; i < initlen; i++, columnData++)
  {
    // for (i = 0; i < 23; i++,) {
    switch (cmd)
    {
    case 1:
    { // axel data //15
      if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 != 12 && i % 20 != 16)
      {
        strncat(pArray, columnData, 1);
      }
      break;
    }
    case 2:
    { // calib soms // 10
      if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 != 12 && i % 20 < 14)
      {
        strncat(pArray, columnData, 1);
      }
      break;
    }
    case 3:
    { //raw soms //7
      if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 < 13)
      { // dating 11 yung last na number
        strncat(pArray, columnData, 1);
      }
      break;
    }
    case 4:
    { // old format
      if (i % 20 != 0 && i % 20 != 1 && i % 20 != 4 && i % 20 != 8 && i % 20 != 12)
      {
        strncat(pArray, columnData, 1);
      }
      break;
    }
    case 8:
    { // old axel /for 15
      if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 != 12 && i % 20 != 16)
      {
        strncat(pArray, columnData, 1);
      }
      break;
    }
    case 9:
    { // diagnostics for v3 sensors //takes temp only
      // final format is gid(2chars)msgid(2chars)temp_hex(4chars)
      if (i % 20 != 0 && i % 20 != 1 && i % 20 != 6 && i % 20 != 7 && i % 20 != 8 && i % 20 != 9 && i % 20 != 14 && i % 20 != 15 && i % 20 != 16 && i % 20 != 17 && i % 20 != 18 && i % 20 != 19)
      {
        strncat(pArray, columnData, 1);
      }
      break;
    }
    case 10:
    {
      if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 != 12 && i % 20 != 16)
      {
        strncat(pArray, columnData, 1);
      }
      break;
    }
    case 41:
    { // axel wrong gids
      if (i % 20 != 4 && i % 20 != 5 && i % 20 != 8 && i % 20 != 12 && i % 20 != 16)
      {
        strncat(pArray, columnData, 1);
      }
      break;
    }
    }
  }

  columnData = start_pointer;
  sprintf(columnData, pArray, strlen(pArray));
}

/* 
  Function: check_identifier

    Determine the identifier based on the message id of the message. 
    The starting position of the message id is determined by the index_msgid.
    A check is performed on the initial 4 characters ( contains the id ).
    This checks for data with id ( first 4 chars correspond to id ) 255 (piezo).
  
  Parameters:
  
    token - char array containing the data split by data type.

    index_msgid - integer index of the start of the message id.
  
  Returns:
  
    idfier - char identifier
  
  See Also:
  
    - <message_content_parameters>
*/
char check_identifier(char *token, int index_msgid)
{
  char idfier = '0';
  char *last_char;
  int id_int;
  char temp_id[5];

  strncpy(temp_id, token, 4);
  id_int = strtol(temp_id, &last_char, 16);
  if (g_sensor_version == 1)
  {
    idfier = '\0';
  }
  else if (id_int == 255)
  {
    idfier = 'p';
    return idfier;
  }
  else
  {
    switch (token[index_msgid])
    {
    case '1':
    {
      if (token[index_msgid + 1] == '5')
      {
        idfier = 'b';
      }
      else if (token[index_msgid + 1] == 'A')
      {
        idfier = 'c';
      }
      else if (token[index_msgid + 1] == '6')
      {
        idfier = 'd';
      }
      break;
    }
    case '0':
    {
      if (token[index_msgid + 1] == 'B')
        idfier = 'x';
      else if (token[index_msgid + 1] == 'C')
        idfier = 'y';
      else if (token[index_msgid + 1] == '8')
        idfier = 'x';
      else if (token[index_msgid + 1] == '9')
        idfier = 'y';
      else if (token[index_msgid + 1] == 'A')
        idfier = 'b';
      else if (token[index_msgid + 1] == 'D')
        idfier = 'c';
      break;
    }
    case '2':
    {
      //Serial.println(token[index_msgid+1]);
      if (token[index_msgid + 1] == '0')
      {
        idfier = 'x';
      }
      else if (token[index_msgid + 1] == '1')
      {
        idfier = 'y';
      }
      else if (token[index_msgid + 1] == '9')
      { //Version 4 idf
        idfier = 'x';
      }
      else if (token[index_msgid + 1] == 'A')
      {
        idfier = 'y';
      }
      break;
    }
    case '6':
    {
      idfier = 'b';
      break;
    }
    case '7':
    {
      idfier = 'c';
      break;
    }
    case 'F':
    {
      idfier = 'p';
    }
    default:
    {
      idfier = '0';
      break;
    }
    }
  }
  return idfier;
}

/* 
  Function: check_cutoff

    Determine the number of characters depending on the identifier
  
  Parameters:
  
    idf - char identifier
  
  Returns:
  
    cutoff - integer number of characters per message
  
  See Also:
  
    - <message_content_parameters>
*/
int check_cutoff(char idf)
{
  int cutoff = 0;
  switch (idf)
  {
  case 'b':
  {
    cutoff = 130;
    break;
  }
  case 'x':
  {
    cutoff = 135; //15 chars only for axel
    break;
  }
  case 'y':
  {
    cutoff = 135; //15 chars only for axel
    break;
  }
  case 'c':
  {
    cutoff = 133; //15 chars only for axel
    break;
  }
  case 'd':
  {
    cutoff = 136; //15 chars only for axel
    break;
  }
  case 'p':
  {
    cutoff = 135;
    break;
  }

  default:
  {
    cutoff = 0;
    if (g_sensor_version == 1)
    {
      cutoff = 135;
    }
    break;
  }
  }
  return cutoff;
}

/* 
  Function: no_data_parsed

    - Writes "*0*ERROR: no data parsed<<+" to the input char array if there is no availabe data from the sensor
  
  Parameters:
  
    message - empty char array 
*/
void no_data_parsed(char *message)
{
  sprintf(message, "040>>1/1#", 3);
  strncat(message, g_mastername, 5);
  strncat(message, "*0*ERROR: no data parsed<<+", 27);
}

//Group: Auxilliary Control Functions

//Function: shut_down()
// Sends a shutdown command to the 328 PowerModule.
void shut_down()
{
  //powerM.println("PM+D");
}

//Function: turn_on_column
// Assert GPIO ( defined by RELAYPIN ) high to turn on sensor column.
void turn_on_column()
{
  if (VERBOSE == 1)
  {
    Serial.println(F("Turning ON column"));
  }
  digitalWrite(COLSW, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
}

//Function: turn_off_column
// Assert GPIO ( defined by RELAYPIN ) low to turn off sensor column.
void turn_off_column()
{
  if (VERBOSE == 1)
  {
    Serial.println(F("Turning OFF column"));
  }
  digitalWrite(COLSW, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}

/*
  Function: sensor_voltage_ver
    Read voltage values from can frames to g_volt

  Parameters:
    g_volt - array for voltage reading

  Returns:
*/
//medyo magulo paitong part na ito..
void sensor_voltage_ver()
{ //kailangan pang ayusin and triggers for v2 testing, v3 lang available
  int normal = 0;
  int nonZero = 0; //limited to v2 & v3
  int zeroVal = 0;
  int counter = 0;
  int trig = 0;
  int repeat = 1;

  for (int i = 0; i < VDATASIZE; i++)
  {
    if (g_volt[i] == 2.0)
    { //Values for testing
      trig++;
      nonZero++;
    }
    else if ((g_volt[i] > 2.5) && (g_volt[i] < 3.5))
    {
      normal++;
      nonZero++;
    }
    else if (g_volt[i] == 0)
    {
      zeroVal++;
    }
  }
  for (int j; j < counter; j++)
  {
    if (g_volt[0] == g_volt[j])
    {
      repeat++;
    }
  }
  if (((g_sensor_version == 3) && (trig >= 1)) || ((g_sensor_version == 3) && (can_flag) && (zeroVal == (int)VDATASIZE)))
  { //trigger for single instance for now
    if (Serial.available())
    {
      Serial.println(F("sensor version mismatch"));
    }
    //change_sensor_version(2);
  }
  else if (((g_sensor_version == 2) && (trig >= 1)) || ((g_sensor_version == 2) && (can_flag) && (zeroVal == (int)VDATASIZE)))
  { //babaguhin pa
    if (Serial.available())
    {
      Serial.println(F("sensor version mismatch"));
    }
    //change_sensor_version(3);
  }

  /*  
  Serial.print("normal ");
  Serial.println(normal);
  Serial.print("nonZero ");
  Serial.println(nonZero);
  Serial.print("zeroVal ");
  Serial.println(zeroVal);
  Serial.print("trig ");
  Serial.println(trig);
  Serial.print("g_sensor_version ");
  Serial.println(g_sensor_version);
*/

  //reboot();                                              //reset watchdogtimer
  normal = nonZero = zeroVal = trig = counter = 0;
  can_flag = false;
}

void change_sensor_version(int new_version)
{
  if (g_sensor_version == new_version)
  {
    Serial.println("OK");
  }
  else if (copy_config_lines(String(new_version)))
  {
    replace_old_config();
    g_sensor_version = new_version;
  }
}

void(* resetFunc) (void) = 0;
