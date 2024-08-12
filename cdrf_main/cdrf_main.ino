/**
 * @file cdrf_main.ino
 * This runs the setup() and loop(). 
 * 
 */

/**
 * @mainpage 
 * 
 * This is the main page. Dito yung mga hanash niyo tungkol sa project.
 * 
 * Kapag galit kayo dito niyo ilagay.
 * 
 * Ito ay puro comments lang sa code.
 *  
*/
/**  
 * Delay (in milliseconds)
 * 
 * Used in #columnOn and #columnsOff
 */
#define MSDELAY 100
#define VDATASIZE 300

// #define LIB_LOGGER_COUNT 79;
/**
 * Char array for name of subsurface sensor.
 * Defaults to "XXXXX"
 */
char MASTERNAME[6] = "XXXXX";


/**  
 * 2 dimensional integer array that stores the links the unique id with the geographic id
 *  
 */
int GIDS[2][40];

/**  
 * Unsigned global 8 bit integer indicating sensor version ( 1 - 5 )
 *  
 */
uint8_t SENSORVERSION;

/**  
 * Unsigned global 8 bit integer indicating datalogger version ( 1 - 5 )
 *  
 */
uint8_t DATALOGGERVERSION;

/**
* Struct containing configuration for subsurface sensor
*
*/
//libConfig CONFIG;

#include "variant.h"
#include <due_can.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <avr/pgmspace.h>
// #include <XBee.h>
#include <stdbool.h>
#include <limits.h>
#include <RTCDue.h>
#include <avr/dtostrf.h>
#include "Due_config.h"
#include <DueFlashStorage.h>
#include <SPI.h>

#define ATCMD "AT"
#define ATECMDTRUE "ATE"
#define ATECMDFALSE "ATE0"
#define ATRCVCAN "ATRCV" #define ATSNDCAN "ATSND"
#define ATGETSENSORDATA "ATGSDT"
#define ATSNIFFCAN "ATSNIFF"
#define ATDUMP "ATDUMP"
#define OKSTR "OK"
#define ERRORSTR "ERROR"
#define ATSD "ATSD"
#define DATALOGGER Serial1  // Change this to Serial1 if using ARQ
#define LORA Serial2
#define powerM Serial3

#define VERBOSE 0
//#define RELAYPIN 44
#define COLUMN_SWITCH_PIN 44
#define LED1 48
#define LED2 49
#define LED3 50
#define LED4 51
#define POLL_TIMEOUT 5000
#define BAUDRATE 9600
#define LORABAUDRATE 9600
#define ARQTIMEOUT 30000
#define VDATASIZE 300

#define ENABLE_RTC 0
#define CAN_ARRAY_BUFFER_SIZE 100
File unsent_log;


// #define comm_mode "ARQ" // 1 for ARQ, 2 XBEE
char comm_mode[5] = "LORA";

const char base64[64] PROGMEM = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                                  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                                  'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2',
                                  '3', '4', '5', '6', '7', '8', '9', '+', '/' };

File newconfig, oldconfig, root;
long timestart = 0;
long timenow = 0;
long arq_start_time = 0;
// Group: Global Variables
// These are the variables declared globally.

/* 
  Variable: b64
  global variable that turns on the b64 data representation mode. 0 by default.

  An AT Command can be used to toggle this. *AT+SWITCHB64* command toggles on and off
  the b64 data representation. 

  Variable declaration:
  --- Code
  uint8_t b64 = 0;
  --- 
*/
uint8_t b64 = 0;

/* 
  Variable: payload
  global variable uint8_t array that will hold the data from xbee

  Variable declaration:
  --- Code
  uint8_t payload[200];
  --- 
*/
uint8_t payload[200];

int g_chip_select = SS3;
uint8_t datalogger_flag = 0;
int volt_last_val = 0;
// boolean vdata_flag;
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
  global char array that holds the node voltage values with respect to gids. This used by is by get_v_value.

  Variable Declaration:
  --- Code
  VDATASIZE = 40
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
uint16_t g_turn_on_delay = 10;

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
  integer that determines the datalogger version ( 2 ARQ or 3 Regular,V3 ). This variable is overwrittern by *<process_config_line>*. 
  
  Variable declaration:
  --- Code
  uint8_t g_datalogger_version = 3;
  ---
*/
uint8_t g_datalogger_version = 0;

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
// CAN-related
char g_temp_dump[5000];
char g_final_dump[5000];
char g_no_gids_dump[5000];

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
char vc_container[20];  //temporary container for voltage current data
char g_build[200];
char g_build_final[500];

char print_buffer[250];
char num_buffer[20];

char g_string[20];
char g_string_proc[20];

CAN_FRAME g_can_buffer[CAN_ARRAY_BUFFER_SIZE];

//
char rcvdChars[32] = {};

/*
  Variable: g_timestamp
  global String that holds the timestamp.This variable is overwritten by the timestamp from the string sent by the ARQ. This variable defaults to "TIMESTAMP000".

  Intial Variable declaration:
  --- Code
 String g_timestamp = "TIMESTAMP000";
  ---
*/

char g_timestamp[] = "TIMESTAMP000";

//current sensor
Adafruit_INA219 ina219(0x41);
bool ate = true;

//  for config use

char column_id_holder[500];
uint16_t LOGGER_COUNT = lib_LOGGER_COUNT;
DueFlashStorage dueFlashStorage;


struct f_config {
  char f_mastername[6];
  int override_lib_config;
  int check;
};

int g_sensor_type = 1;

int ledState = LOW; 
unsigned long ledTimer; 
int flash_check_flag; //flag if flash memory is empty

unsigned long previousMillis = 0; // will store last time LEDs was updated

const long interval = 500; // interval to blink LEDs (milliseconds)

void setup() {
  //int _EXFUN(strcmp, (const char*, const char*));
  Serial.begin(BAUDRATE);
  DATALOGGER.begin(9600);
  LORA.begin(LORABAUDRATE);
  ina219.begin();
  pinMode(COLUMN_SWITCH_PIN, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED4, OUTPUT);
  canInit();
  init_char_arrays();
  init_gids();
  init_sd();


  Serial.println(F("======================================"));
  Serial.println(F("Firmware version: 02.01.24"));
  flash_fetch();
  due_command();
}

void loop() {
  int timestart = millis();

  while ((millis() - timestart < 30000) && (datalogger_flag == 0)) {
    ledTimer = millis();
    if(flash_LED() != 99 ){  
      if (ledTimer - previousMillis >= interval) {
        // save the last time you blinked the LED
        previousMillis = ledTimer;

        // Toggle LEDs:
        if (ledState == LOW) {
          ledState = HIGH;
        } else {
          ledState = LOW;
        }    
    // set the LED with the ledState of the variable:
      digitalWrite(LED1, ledState);
      digitalWrite(LED2, ledState);
      digitalWrite(LED3, ledState);
      digitalWrite(LED4, ledState);
     }
    } else {
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, HIGH);
      digitalWrite(LED4, LOW);
    }

    if (Serial.available() > 0) {
      getATCommand();
      // Serial.println(F(comm_mode));

    } else if ((strcmp(comm_mode, "LORA") == 0) && (LORA.available())) {
      // Serial.println("LORA.available main loop");
      rcvdChars[0] = '\0';
      wait_lora_cmd(rcvdChars);
      Serial.print("rcvdChars::::::");
      Serial.println(rcvdChars);
      operation(parse_cmd(rcvdChars), comm_mode);
      //shut_down();
      delay (500);
      LORA.println("STOPLORA");
      datalogger_flag = 1;
    }
  }
  delay(100);
}

void getATCommand() {
  char serial_line[256];  // Adjust the size as needed
  char command[256], extra_parameters[256];
  char converted[5] = {};
  char padded[5] = {};
  char atSwitch;
  int i_equals = 0;

  if (Serial.available() > 0) {
    do {
      Serial.readBytesUntil('\n', serial_line, sizeof(serial_line));
    } while (strcmp(serial_line, "") == 0);

    for (int i = 0; i < sizeof(serial_line); i++) {
      serial_line[i] = toupper(serial_line[i]);
      if (serial_line[i] == '\r') {
        serial_line[i] = '\0'; // Replace '\r' with null terminator
      }
    }

    if (ate) Serial.println(serial_line);

    i_equals = -1;
    for (int i = 0; i < sizeof(serial_line); i++) {
      if (serial_line[i] == '=') {
        i_equals = i;
        break;
      }
    }

    if (i_equals == -1) {
      strcpy(command, serial_line);
    } else {
      strncpy(command, serial_line, i_equals);
      command[i_equals] = '\0';  // Null terminate the command string
    }

    for (int i = 0; i < VDATASIZE; ++i) {
      g_volt[i] = (float)0;
    }

    atSwitch = serial_line[0];
    switch (atSwitch) {
      case '?':
        {
          // flash_fetch();
          print_stored_config();
        }
        break;
      case 'A':
        {  //ATGETSENSORDATA
          if (g_mastername[3] == 'S') {
            g_sensor_type = 2;
          } else {
            g_sensor_type = 1;
          }
          char dummy[22] = "ARQCMD6T/202402151700";
          operation(parse_cmd(dummy),comm_mode);
          // read_data_from_column(g_final_dump, g_sensor_version, g_sensor_type);
          // int g_volt_size = sizeof(g_volt) / sizeof(g_volt[0]);

          Serial.println(OKSTR);
        }
        break;

      case 'B':
        {  //AT+POLL
          if (g_mastername[3] == 'S') {
              g_sensor_type = 2;
            } else {
              g_sensor_type = 1;
            }
          read_data_from_column(g_final_dump, g_sensor_version, g_sensor_type);
          Serial.println(F(g_final_dump));
          b64_build_text_msgs(comm_mode, g_final_dump, text_message);
          Serial.println(OKSTR);
        }
        break;

      case 'C':
        due_command();
        break;

      case 'D':
        name_entry();
        flash_fetch();
        Serial.println(OKSTR);
        break;

      case 'E':
        read_current();
        read_voltage();
        break;

      case 'F':
        {  //ATDUMP
          Serial.print("g_final_dump: ");
          Serial.println(g_final_dump);
          Serial.println(OKSTR);
        }
        break;
      case 'G':
        {  //ATECMDFALSE
          if (SD.begin(g_chip_select)) {
            File sdDir = SD.open("/");
            printDirectory(sdDir, 0);
            Serial.println(OKSTR);
          } else {
            Serial.println("ERR: Unable to access SD card");
          }
        }
        break;

      case 'H':
        {
          dumpSDtoPC();
          Serial.print("OK");
        }
        break;


      default:
          Serial.println(ERRORSTR);
          break;
    }
  } else
    return;
}

void operation(int sensor_type, char communication_mode[]) {
  File create_unsent;
  if (sensor_type == 3) {
    // if((minute > 5 && minute < 25) || (minute > 35 && minute < 55)){
    open_sdata();
    //  rename_sd();
  } else if (sensor_type == 4) {
    //  for arQ rain gauge only
    //  sends voltage data thru arQ
    send_current_voltage_thru_arq();
  } else if (sensor_type == 5) { // for sending config to v5 datalogger serial
    Serial2.println("OK");
    print_stored_config2();
    Serial2.println("END OF CONFIG");
    return;
  } else if (sensor_type == 6) {
    flash_fetch();
    if (Serial2) Serial2.println("END OF UPDATE");
    return;
    // add something here later
  } else {
    int counter = 0;
    int num_of_tokens = 0;
    if (!SD.begin(g_chip_select)) {  //create unsent.txt
      Serial.println(F("SD card not detected"));
      Serial2.println(F("### CAUTION: SD CARD NOT DETECTED  ###"));
      // return;
    } else {
      create_unsent = SD.open("unsent.txt", FILE_WRITE);
      create_unsent.close();
    }

    if (g_mastername[3] == 'S') {
      g_sensor_type = 2;
    } else {
      g_sensor_type = 1;
    }
    read_data_from_column(g_final_dump, g_sensor_version, g_sensor_type);  // matagal ito.
    Serial.print(F("g_final_dump: "));
    Serial.println(F(g_final_dump));
    b64_build_text_msgs(comm_mode, g_final_dump, text_message);

    Serial.println(text_message);
    char* token1 = strtok(text_message, "~");

    while (token1 != NULL) {
      Serial.print("Sending ::::");
      Serial.println(token1);
      if (strcmp(comm_mode, "LORA") == 0) {
        send_thru_lora(false, token1);
      } else {  //default
        send_data(true, token1);
      }
      token1 = strtok(NULL, g_delim);
      num_of_tokens = num_of_tokens + 1;
    }

    if (strcmp(comm_mode, "LORA") == 0) {
      char ts_container[13];
      char unsent_value[10];
      char b64_ts[13] = {};
      itoa(unsent_count(),unsent_value, 10);
      unsent_value[(strlen(unsent_value))+1];
      strcpy(g_build_final,">>");
      strncat(g_build_final, g_build, strlen(g_build));
      strcat(g_build_final,">");
      strncat(g_build_final, unsent_value,strlen(unsent_value));
      strcat(g_build_final,"*");
      strncat(g_build_final, g_timestamp,strlen(g_timestamp));
      // strncat(g_build_final, b64_ts, strlen(b64_ts));
      
      send_thru_lora(false, g_build_final);

    } else {  //default
      send_data(true, g_build_final);
      g_build_final[0] = 0X00;
      g_build[0] = 0X00;
    }
  }
}


void read_data_from_column(char* column_data, int sensor_version, int sensor_type) {

  //start building current-voltage sms;
  strncpy(g_build, g_mastername, strlen(g_mastername));
  strcat(g_build, "*m*");
  dtostrf(ina219.getCurrent_mA(), 0, 4, vc_container);
  strncat(g_build, vc_container, strlen(vc_container));
  strcat(g_build, "*");
  dtostrf(ina219.getBusVoltage_V(), 0, 4, vc_container);
  strncat(g_build, vc_container, strlen(vc_container));

  // vdata_flag = true;
  columnOn();
  delay(g_turn_on_delay);

  //continue building current-voltage sms after column has been turned on
  strcat(g_build, "*");
  dtostrf(ina219.getCurrent_mA(), 0, 4, vc_container);
  strncat(g_build, vc_container, strlen(vc_container));
  strcat(g_build, "*");
  dtostrf(ina219.getBusVoltage_V(), 0, 4, vc_container);
  strncat(g_build, vc_container, strlen(vc_container));


  if (sensor_version == 2) {
    Serial.println("Accel data");
    get_data(32, 1, column_data);
    get_data(33, 1, column_data);
    if (sensor_type == 2) {
    }
  } else if (sensor_version == 3) {
    Serial.println("Accel data");
    get_data(11, 1, column_data);
    get_data(12, 1, column_data);
    Serial.println("Temperature data");  //temp daw ito [22,23,24] sabi ni kate
    get_data(22, 1, column_data);
    if (sensor_type == 2) {
      Serial.println("Soil Moisture sensor data");
      get_data(10, 1, column_data);
      get_data(13, 1, column_data);
    }
  } else if (sensor_version == 4) {
    Serial.println("Accel data");
    get_data(41, 1, column_data);
    get_data(42, 1, column_data);
    Serial.println("Temperature data");
    get_data(22, 1, column_data);

  } else if (sensor_version == 5) {
    Serial.println("Accel data");
    get_data(51, 1, column_data);
    get_data(52, 1, column_data);
    Serial.println("Temperature data");
    get_data(22, 1, column_data);
    get_data(23, 1, column_data);
    get_data(24, 1, column_data);

  } else if (sensor_version == 1) {
    get_data(256, 1, column_data);  //Added for polling of version 1
  }

  // vdata_flag = false;
  columnOff();
  delay(1000);

  // last phase of building current-voltage sms after column has been turned off
  strcat(g_build, "*");
  dtostrf(ina219.getCurrent_mA(), 0, 4, vc_container);
  strncat(g_build, vc_container, strlen(vc_container));
  strcat(g_build, "*");
  dtostrf(ina219.getBusVoltage_V(), 0, 4, vc_container);
  strncat(g_build, vc_container, strlen(vc_container));
  // strcat(g_build, "*");
  Serial.println(g_build);
  Serial2.println(g_build);
}

void read_current() {
  columnOn();
  delay(1000);
  float current_mA = 0;
  current_mA = ina219.getCurrent_mA();
  Serial.print("Current:       ");
  Serial.print(current_mA);
  Serial.println(" mA");
  delay(1000);
  columnOff();
}

void read_voltage() {
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


char* getTimestamp(char communication_mode[]) {
 // char timestamp[20]; // Static to ensure the array is not deallocated when the function exits

  if (communication_mode == 0) {  //internal rtc
    return g_timestamp;
  } else if (strcmp(comm_mode, "ARQ") == 0) {  // ARQ
    return g_timestamp;
  } else if (strcmp(comm_mode, "LORA") == 0) {
    Serial.println(g_timestamp);
    return g_timestamp;
  } 
  else {
    return "0TIMESTAMP";
  }
}


int wait_arq_cmd() {
  char c_serial_line[30];
  while (!DATALOGGER.available())
    ;
  arq_start_time = millis();  // Global variable used by arqwait_delay
  do {
    DATALOGGER.readBytesUntil('\n', c_serial_line, 30);
  } while (c_serial_line[0] == '\0');
  Serial.println(c_serial_line);
  return parse_cmd(c_serial_line);
}


void wait_lora_cmd(char* result){
  char c_serial_line[30];
  while (!LORA.available())
    ;
  // arq_start_time = millis();  // Global variable used by arqwait_delay
  do {
    LORA.readBytesUntil('\n', c_serial_line, 30);
  } while (c_serial_line[0] == '\0');
  Serial.println(c_serial_line);
  strncpy(result,c_serial_line,21);
  Serial.println(result);
  // return parse_cmd(c_serial_line);
}

// gawing switch case yung if else dito para readable
int parse_cmd(char* command_string) {
  char command[30];
  char temp_time[30];
  char serial_line[30]; // Assuming a maximum length of 30 characters

  char* pch;
  char cmd[] = "CMD6";
  int cmd_index;
  
  strcpy(command, command_string);
  Serial.println(command);

  if ((pch = strstr(command_string, cmd)) != NULL) {
    char* slash_ptr;
    int slash_index;

    if (*(pch + strlen(cmd)) == 'S') {          // SOMS + TILT
      cmd_index = strchr(command, 'S') - command;  // ARQCMD has 6 characters
      strcpy(temp_time, command + cmd_index + 1);
      slash_ptr = strchr(temp_time, '/');
      if (slash_ptr != NULL) {
        slash_index = slash_ptr - temp_time;
        memmove(&temp_time[slash_index], &temp_time[slash_index + 1], strlen(&temp_time[slash_index + 1]) + 1);
      }
      strncpy(g_timestamp, temp_time,12);
      // trim(g_timestamp);
      Serial.print("g_timestamp: ");
      Serial.println(g_timestamp);
      return 2;
    } else if (*(pch + strlen(cmd)) == 'T') {   // TILT Only
      cmd_index = strchr(command, 'T') - command;  // ARQCMD has 6 characters
      strcpy(temp_time, command + cmd_index + 1);
      slash_ptr = strchr(temp_time, '/');
      if (slash_ptr != NULL) {
        slash_index = slash_ptr - temp_time;
        memmove(&temp_time[slash_index], &temp_time[slash_index + 1], strlen(&temp_time[slash_index + 1]) + 1);
      }
      // strcpy(g_timestamp, temp_time);
      strncpy(g_timestamp, temp_time,12);
      // trim(g_timestamp);
      Serial.print("g_timestamp: ");
      Serial.println(g_timestamp);
      return 1;
    } else if (*(pch + strlen(cmd)) == 'G') {   // GSM
      cmd_index = strchr(command, 'G') - command;  // ARQCMD6G ARQCMD has 6 characters
      strcpy(temp_time, command + cmd_index + 1);
      slash_ptr = strchr(temp_time, '/');
      if (slash_ptr != NULL) {
        slash_index = slash_ptr - temp_time;
        memmove(&temp_time[slash_index], &temp_time[slash_index + 1], strlen(&temp_time[slash_index + 1]) + 1);
      }
      // strcpy(g_timestamp, temp_time);
      strncpy(g_timestamp, temp_time,12);
      // trim(g_timestamp);
      Serial.print("g_timestamp: ");
      Serial.println(g_timestamp);
      return 3;
    } else if (*(pch + strlen(cmd)) == 'V') {  // ARQCMD6V: Current and Voltage ONLY
      cmd_index = strchr(command, 'V') - command;
      strcpy(temp_time, command + cmd_index + 1);
      slash_ptr = strchr(temp_time, '/');
      if (slash_ptr != NULL) {
        slash_index = slash_ptr - temp_time;
        memmove(&temp_time[slash_index], &temp_time[slash_index + 1], strlen(&temp_time[slash_index + 1]) + 1);
      }
      // strcpy(g_timestamp, temp_time);
      strncpy(g_timestamp, temp_time,12);
      // trim(g_timestamp);
      Serial.print("g_timestamp: ");
      Serial.println(g_timestamp);
      return 4;
    } else if (*(pch + strlen(cmd)) == 'C') {  //ARQCMD6C: SD card config
      return 5;
    } else if (*(pch + strlen(cmd)) == 'D') {  //ARQCMD6D: SD card config
      Serial2.println("Updating due config...");
      char * newName = strtok(command + 8, "/");
      byte d[sizeof(f_config)];
      f_config flash_config;
      strncpy(flash_config.f_mastername, newName, sizeof(flash_config.f_mastername) - 1); // Copy input to flash_config
      flash_config.f_mastername[sizeof(flash_config.f_mastername) - 1] = '\0'; // Ensure null termination

      flash_config.check = 99;
      memcpy(d, &flash_config, sizeof(f_config));
      dueFlashStorage.write(4, d, sizeof(f_config));
      delay(1000);
      Serial2.println(flash_config.f_mastername);
      Serial2.println(" saved to flash");
      return 6;

    } else {
      Serial.println("wait_arq_cmd returned 0");
      return 0;
    }
  }
}


//Group: Auxilliary Control Functions

//Function: shut_down()
// Sends a shutdown command to the 328 PowerModule.
void shut_down() {
  powerM.println("PM+D");
}

void columnOn() {
  digitalWrite(COLUMN_SWITCH_PIN, HIGH);
  digitalWrite(LED1, HIGH);
  delay(g_turn_on_delay);
  // arqwait_delay(g_turn_on_delay);
}

void columnOff() {
  digitalWrite(COLUMN_SWITCH_PIN, LOW);
  digitalWrite(LED1, LOW);
  delay(g_turn_on_delay);
  // arqwait_delay(1000);
}


void send_data(bool isDebug, char* columnData) {
  String new_unsent;
  int timestart = millis();
  int timenow = millis();
  bool OKFlag = false;
  uint8_t send_retry_limit = 0;
  if (isDebug == true) {
    do {
      timestart = millis();
      timenow = millis();
      while (!Serial.available() > 0) {
        while (timenow - timestart < 9000) {
          timenow = millis();
        }
        Serial.println("Time out...");
        break;
      }
      if (Serial.find((char*)"OK")) {
        OKFlag = true;
        Serial.println("moving on");
      } else {
        Serial.println(columnData);
      }
    } while (OKFlag == false);
  } else {
    do {
      Serial.print("Sending:");
      Serial.println(columnData);
      DATALOGGER.println(columnData);
      timestart = millis();
      timenow = millis();

      while (!DATALOGGER.available()) {
        while (timenow - timestart < 9000) {
          timenow = millis();
        }
        DATALOGGER.println("Time out...");
        break;
      }

      if (DATALOGGER.find((char*)"OK")) {
        OKFlag = true;
      } else {
        DATALOGGER.println(columnData);
      }

      send_retry_limit++;
      if (send_retry_limit == 3) {
        Serial.println(F("send_retry_limit reached."));
        unsent_log = SD.open("unsent.txt", FILE_WRITE);
        new_unsent = columnData;
        new_unsent.remove(0, 10);
        if (unsent_log) {
          unsent_log.println(new_unsent);
          unsent_log.close();
          Serial.println("data logged!");
          break;
        }
        OKFlag = true;
      }
    } while (OKFlag == false);
  }

  return;
}

void send_thru_lora(bool isDebug, char* columnData) {
  int timestart = millis();
  int timenow = millis();
  bool OKFlag = false;
  uint8_t send_retry_limit = 0;

  timestart = millis();

  do {
    Serial.print("Sending: ");
    Serial.println(columnData);
    LORA.println(columnData);

    if (LORA.find((char*)"OK")) {
      Serial.println("Received by LoRa..");
      OKFlag = true;
    } else {
      delay(1000);
      LORA.println(columnData);
    }

    send_retry_limit++;
    if (send_retry_limit == 3) {
      Serial.println("send_retry_limit reached.");
      OKFlag = true;
    }
    timenow = millis();
  } while (OKFlag == false && (timenow - timestart < 9000));
  return;
}

int unsent_count() {
  File unsent_file;
  int unsent_counter = 0;
  char usent_char;
  if (SD.exists("unsent.txt")) {    unsent_file = SD.open("unsent.txt");
    if (unsent_file) {
      while (unsent_file.available()) {
        usent_char = (unsent_file.read());
        if (usent_char == '\n') {
          unsent_counter++;
          //Serial.print("count");
        }
      }
      unsent_file.close();
    }
  } else {
    unsent_counter = -1;
  }
  return (unsent_counter);
}


void send_current_voltage_thru_arq()  //for arQ only
{
  static char v_test[50] = { '\0' };
  static char v_final[50] = { '\0' };

  Serial.println(F("Sending current and voltage for arQ with RAIN GUAGE only"));

  sprintf(v_test, ">>1/1#%s*m*%2.4f*%2.4f*%2.4f*%2.4f<<", g_mastername, ina219.getCurrent_mA(), ina219.getBusVoltage_V(), ina219.getCurrent_mA(), ina219.getBusVoltage_V());

  int v_data_length = String(v_test).length() + 3;  //lenth of string to be sent plus 3 char data length

  sprintf(v_final, "%03d", v_data_length);
  strcat(v_final, v_test);
  Serial.print(v_final);

  char* token1 = strtok(v_final, "~");

  while (token1 != NULL) {
    Serial.print("Sending ::::");
    Serial.println(token1);
    if (strcmp(comm_mode, "ARQ") == 0) {
      send_data(false, token1);
    } else {  //default
      send_data(true, token1);
    }
    token1 = strtok(NULL, g_delim);
  }
}
