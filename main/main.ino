#include "variant.h"
#include <due_can.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <avr/pgmspace.h>
#include <XBee.h>
#include <stdbool.h>

#include <RTCDue.h>


#define ATCMD     "AT" 
#define ATECMDTRUE  "ATE"
#define ATECMDFALSE "ATE0"
#define ATRCVCAN    "ATRCV"
#define ATSNDCAN    "ATSND"
#define ATGETSENSORDATA    "ATGSDT"
#define ATSNIFFCAN  "ATSNIFF"
#define ATDUMP    "ATDUMP"
#define OKSTR     "OK"
#define ERRORSTR  "ERROR"
#define ATSD      "ATSD"
#define DATALOGGER Serial1 // Change this to Serial1 if using ARQ
#define LORA Serial2
#define powerM Serial3


#define VERBOSE 0
#define RELAYPIN 44
#define LED1 48
#define POLL_TIMEOUT 5000
#define BAUDRATE 9600
#define ARQTIMEOUT 30000


#define ENABLE_RTC 0
#define CAN_ARRAY_BUFFER_SIZE 100

// #define comm_mode "ARQ" // 1 for ARQ, 2 XBEE
char comm_mode[5] = "ARQ";

const char base64[64] PROGMEM = {'A','B','C','D','E','F','G','H','I','J','K','L','M',
'N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h',
'i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2',
'3','4','5','6','7','8','9','+','/'};

XBee xbee = XBee();
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

XBeeAddress64 addr64 = XBeeAddress64(0x00, 0x00); //sets the data that it will only send to the coordinator
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();
XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse();

int g_chip_select = SS3;
uint8_t datalogger_flag = 0;


/* 
  Variable: xbee_response
  global variable char array that will hold the command frame received from the xbee

  Variable declaration:
  --- Code
  char xbee_response[200];
  --- 
*/
char xbee_response[200];

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
  integer that determines the datalogger version ( 2 ARQ or 3 Regular,V3 ). This variable is overwrittern by *<process_config_line>*. 
  
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
// CAN-related
char g_temp_dump[2500];
char g_final_dump[5000];
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
char text_message[10000];

String g_string;
String g_string_proc;

CAN_FRAME g_can_buffer[CAN_ARRAY_BUFFER_SIZE];

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

//current sensor
Adafruit_INA219 ina219(0x41);
bool ate=true;

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
void setup() {
  Serial.begin(BAUDRATE);
  DATALOGGER.begin(9600);
  LORA.begin(9600);
  // powerM.begin(9600);
  ina219.begin();
  pinMode(RELAYPIN, OUTPUT);
  pinMode(LED1, OUTPUT);
  init_can();
  init_char_arrays();
  init_gids();
  init_sd(); 
  open_config();
  print_stored_config();

  
  if (g_datalogger_version == 3){
    strncpy(comm_mode,"XBEE",4);
    xbee.setSerial(DATALOGGER);  
  } else if(g_datalogger_version == 2){
    strncpy(comm_mode,"ARQ",3);
  } else if(g_datalogger_version == 4){
    strncpy(comm_mode,"LORA",4);  
  } else {
    Serial.print("g_datalogger_version == ");
    Serial.println(g_datalogger_version);
    strncpy(comm_mode,"ARQ",3);
  }
    Serial.print("Comms: "); Serial.println(comm_mode);
    print_due_command2();
}


/* 
  Function: loop

    - Waits for any activity in the DATALOGGER Serial or xbee Serial for 20 secs.

    - Calls <operation>

    - Sets the datalogger_flag

  Parameters:
    
    n/a

  Returns:
    
    n/a
  
  See Also:

    <operation> <getATCommand> 
*/
void loop(){
  int timestart = millis();
  while ( (millis() - timestart < 20000) && (datalogger_flag == 0)){
      if (Serial.available()){
        Serial.println("Debug Mode!");
        getATCommand();  
        // datalogger_flag = 1; // Kagpag nagdebug, wag na mag ops mode.
      } else if (strcmp(comm_mode,"XBEE") == 0){// sira ito // fix by: ilabas yung pagkuha nung string dito tapos ipasa na lang yung cmd.

        char temp[1];
        xbee_response[0] = '\0';
        xbee.readPacket(); 
        if (xbee.getResponse().isAvailable()) {
          Serial.println("xbee.getResponse().isAvailable()!!!!");
            if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
                xbee.getResponse().getZBRxResponse(rx);
                for (int i = 0; i < rx.getDataLength (); i++){
                  temp[0] = (char)rx.getData(i);
                  strncat(xbee_response,temp,1 ) ;        
                }
            }
            Serial.print("response_string: ");
            Serial.println(xbee_response);
            operation(parse_cmd(xbee_response), comm_mode);
            datalogger_flag = 1;
            shut_down();
        } else if ( xbee.getResponse().isError()){
              Serial.print("Error ");
              Serial.println(xbee.getResponse().getErrorCode());
        }
      } else if ((strcmp(comm_mode,"ARQ") == 0) && (DATALOGGER.available()) ){ // sira ito       
        operation(wait_arq_cmd(), comm_mode);
        shut_down();
        datalogger_flag = 1;
     } else if ((strcmp(comm_mode,"LORA") == 0) && (LORA.available()) ){
      operation(wait_lora_cmd(), comm_mode);
      LORA.println("STOPLORA");
      datalogger_flag = 1;
     }
  }
  delay (100);
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


void hard_code(){
  //String str1 = "column1 = 1644,1710,2022,1915,749,1691,1724,1742,1906,1921,1923,1983,2021,2024,2079,2107,2137,2160,2255,2282,2307,2329,2429,2570,2553,2544,2460,2459,2455,1717,1708,2598,2581,2577,2308,2239,2409,2369,2442,2076";
  String str1 = "column1 = 1835, 2055, 1924, 1886, 2495";
  String str2 = "MasterName = PHITA";

  g_num_of_nodes = process_column_ids(str1);
  get_value_from_line(str2).toCharArray(g_mastername,6); 
}

//Function: getATCommand
// Take in-line serial input and execute AT command 
void getATCommand(){
  String serial_line, command, extra_parameters;
  char converted[5] = {};
  char padded[5] = {};
  char atSwitch;
  int i_equals = 0; // index of equal sign
  if (Serial.available()) {
    do{
      serial_line = Serial.readStringUntil('\r\n');
    } while(serial_line == "");

     serial_line.toUpperCase();
     serial_line.replace("\r","");
    

    // echo command if ate is set, default true
    if (ate) Serial.println(serial_line);{
      i_equals = serial_line.indexOf('=');
    }
    if (i_equals == -1) command = serial_line;
    else command = serial_line.substring(0,i_equals);
    
     
    atSwitch = serial_line.charAt(0);
    switch(atSwitch) {
      case '?': 
        print_due_command2();
        break;
      case 'A': {         //ATGETSENSORDATA
          read_data_from_column(g_final_dump, g_sensor_version,1);
          Serial.println(OKSTR);
        }
        break;
      case 'B': {         //AT+POLL
          read_data_from_column(g_final_dump, g_sensor_version,2);
          Serial.println(g_final_dump);
          if (b64 == 1) {
            b64_build_text_msgs(comm_mode, g_final_dump, text_message);
          } else {
          build_txt_msgs(comm_mode, g_final_dump, text_message); 
          }
          Serial.println(OKSTR);
        }
        break;
      case 'C': {          //ATSD){
          String conf;
          init_sd();
          open_config();
          Serial.println(F(OKSTR));
        }
        break;
      case 'D': {         //AT+TIMENOW
          Serial.print("ARQ Time String: ");
          Serial.println(g_timestamp);
        }
        break;
      case 'E': {         //AT+RTC
          extra_parameters = serial_line.substring(i_equals+1);
          set_rtc_time(extra_parameters);
        }
        break;
      case 'F': {         //ATECMDTRUE
          ate = true;
          Serial.println(OKSTR);
        }
        break;
      case 'G': {         //ATECMDFALSE
          ate = false;
          Serial.println(OKSTR);
        }
        break;        
      case 'H': {         //ATCMD
          Serial.println(OKSTR);
        }
        break;
      case 'I': {         //AT+S
          get_data(11,1,g_final_dump);
          get_data(12,1,g_final_dump);
        }
        break;
      case 'J': {         //AT+B64
          // g_timestamp = b64_timestamp(g_timestamp);
          // Serial.println(g_timestamp);
          extra_parameters = serial_line.substring(i_equals+1);
          to_base64(extra_parameters.toInt(),converted);
          pad_b64(5,converted,padded);
          Serial.print("converted and padded: ");
          Serial.print(padded);
        }
        break;
      case 'K': {         //AT+SWITCHB64
          if(b64){
            b64 = 0;
          } else {
            b64 = 1;
          }
          Serial.println("Toggled b64 operations.");
          get_data(11,1,g_final_dump);
          get_data(12,1,g_final_dump);
          get_data(22,1,g_final_dump);
          get_data(10,1,g_final_dump);
          get_data(13,1,g_final_dump);
          // Serial.println(g_final_dump);
    
          g_timestamp = String("180607142000");
          b64_build_text_msgs(comm_mode, g_final_dump, text_message);
          // Serial.println(text_message); 
          // Serial.println(g_final_dump);
          Serial.println("OK");
        }
        break;
      case 'L': {         //ATSNIFFCAN
          while (true){
            Serial.println(OKSTR);
          }
        } 
        break;
      case 'M':         //AT+CURRENT
        read_current();
        break;
      case 'N':         //AT+VOLTAGE
        read_voltage();
        break;
      case 'O': {         //AT+TIMESTAMPPMM
          String timestamp = getTimestamp("XBEE");
          Serial.println(timestamp);
        }
        break;
      case 'P': {          //AT+SEND
          Serial.println(text_message);
          char *token1 = strtok(text_message,g_delim);
          while (token1 != NULL){
            send_data(false, token1);
            token1 = strtok(NULL, g_delim);
          }
        }
        break;
      case 'Q': {         //AT+PIEZO
          get_data(255,1,g_final_dump);
          Serial.print("g_final_dump:::");
          Serial.println(g_final_dump);
        }
        break;
      case 'R': {         //AT+XBEE
          wait_xbee_cmd(10000,xbee_response);
          Serial.print("xbee_response :: ");
          Serial.println(xbee_response);
        }
        break;
      case 'S': {          //AT+LOOPSEND
          while(1){
            Serial.println("sent.");
            send_command(3,3);
            delay(1000);
          }
        } 
        break;                
      case 'T': {         //AT+LOOPBACK
          serial_loopback();
          Serial.println(OKSTR);
        }
        break;
      case 'U': {         //AT+LOOPBACK2
          serial_loopback2();
          Serial.println(OKSTR);
        }
        break;
      case 'V': {         //ATDUMP
          Serial.print("g_final_dump: ");
          Serial.println(g_final_dump);
          Serial.println(OKSTR);
        }
        break;
      default:
        Serial.println(ERRORSTR);
        break;
    }
  } else 
    return;
}

/* 
  Function: operation

    Remove specific characters unnecessary for data interpretation.
  
  Parameters:
  
    types - integer that determines the kind of data requested from the sensor. *1 - tilt* *2 - tilt + soms*

    communication_mode - char array that determines which DATALOGGER receives the sent data. *ARQ* or *XBEE* 
  
  Returns:
  
    n/a
  
  See Also:
  
    <loop>

  Global Variables:

    <g_final_dump>, <g_sensor_version>, <text_message>
*/
void operation(int sensor_type, char communication_mode[]){
  int counter= 0;
  int num_of_tokens = 0;
  read_data_from_column(g_final_dump, g_sensor_version, sensor_type);// matagal ito.
  Serial.print("g_final_dump: ");
  Serial.println(g_final_dump);
  if (b64 == 1) {
    b64_build_text_msgs(comm_mode, g_final_dump, text_message);
  } else {
    build_txt_msgs(comm_mode, g_final_dump, text_message); 
  }
  char *token1 = strtok(text_message,"~");
    
  while (token1 != NULL){
    Serial.print("Sending ::::");
    Serial.println(token1);
    if (strcmp(comm_mode,"ARQ") == 0) {
      send_data(false, token1);    
    }else if(strcmp(comm_mode, "LORA") == 0){
      send_thru_lora(false,token1);
    }else if(strcmp(comm_mode,"XBEE") == 0) {
      while (send_thru_xbee(token1) == false){
        if (counter == 10)
          break;
        counter ++;
      }
    } else { //default
      send_data(true, token1); 
    }
    token1 = strtok(NULL, g_delim);
    num_of_tokens = num_of_tokens + 1;
  }
}

//Function: getArguments
// Read in-line serial AT command.
void getArguments(String at_cmd, String *arguments){
  int i_from = 0, i_to = 0, i_arg = 0;
  bool f_exit = true;
  String sub;

  i_from = at_cmd.indexOf('=');

  do{
    i_to = at_cmd.indexOf(',',i_from+1);
    if (i_to < 0){
      sub = at_cmd.substring(i_from+1);
      f_exit = false;
    }
    else sub = at_cmd.substring(i_from+1,i_to);

    arguments[i_arg] = sub;
    i_from = i_to;
    i_arg += 1;

  } while(f_exit);
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
void read_data_from_column(char* column_data, int sensor_version, int sensor_type){
  if (sensor_version == 2){
    get_data(32,1,column_data);
    get_data(33,1,column_data);
    if (sensor_type == 2){
      get_data(111,1,column_data);
      get_data(112,1,column_data);
    }
  } else if (sensor_version == 3){
    get_data(11,1,column_data);
    get_data(12,1,column_data);
    get_data(22,1,column_data);
    if (sensor_type == 2){
      get_data(10,1,column_data);
      get_data(13,1,column_data);
    }   
  } else if (sensor_version == 4){
    get_data(41,1,column_data);
    get_data(42,1,column_data);
    get_data(22,1,column_data); 
    
  } else if (sensor_version == 1){
    get_data(256,1,column_data); //Added for polling of version 1
 
  }
  if (has_piezo){
    get_data(255,1,column_data);
  }
}

//Function: read_current()
// Reads the current draw from the onboard ina219.
void read_current(){
  turn_on_column();
  delay(1000);
  float current_mA = 0;
  current_mA = ina219.getCurrent_mA();
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  delay(1000);
  turn_off_column();
}

/* 
  Function: read_voltage()

    Reads the *Bus Voltage*, *Shunt Voltage*, *Load Voltage* from the onboard ina219.
*/
void read_voltage(){
  float shuntvoltage = 0;
  float busvoltage = 0;
  float loadvoltage = 0;

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
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
String getTimestamp(char communication_mode[]){

  if (communication_mode == 0){ //internal rtc
    return g_timestamp;
  } else if (strcmp(comm_mode,"ARQ") == 0){ // ARQ
    // Serial.print("g_timestamp: ");
    // Serial.println(g_timestamp);
    return g_timestamp;
  } else if(strcmp(comm_mode,"XBEE") == 0){ //xbee
    return g_timestamp;
  } else if(strcmp(comm_mode, "LORA") == 0){
    Serial.println(g_timestamp);
    return g_timestamp;
    /*
    char timestamp[20] = "";    
    timestart = millis();
    timenow = millis();
    powerM.write("PM+R");
    
    while ( (!powerM.available()) && ( timenow - timestart < 7000 )){
      timenow = millis();
    }
    if (powerM.available()){
      powerM.readBytesUntil('\n', timestamp, 20);
      Serial.println(timestamp);
    } else {
      return String("0TIMESTAMP");
    }

    if (timestamp[0] == '1'){
      return timestamp;
    } else {
      while ( (!powerM.available()) && ( timenow - timestart < 7000 )){
        timenow = millis();
      }
      if (powerM.available()){
        powerM.readBytesUntil('\n', timestamp, 20);
        Serial.println(timestamp);
        return timestamp; 
      }    
    }
    */
  }
  
  else{

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
void set_rtc_time(String time_string){
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
}

//Group: Command Gathering Functions
/* 
  Function: wait_arq_cmd

    Determine the types of data to be sampled from the sensors based on given ARQCMD.
    ARQCMD is read from the defined DATALOGGER.
  
  Parameters:
  
    n/a
  
  Returns:
  
    1 - tilt

    2 - tilt and soil moisture
  
  See Also:
  
    - <loop> <parse_cmd>
*/
int wait_arq_cmd(){
  String serial_line; 
  char c_serial_line[30];  
  while(!DATALOGGER.available());
  arq_start_time = millis(); // Global variable used by arqwait_delay
  do{
    serial_line = DATALOGGER.readStringUntil('\r\n');
  } while(serial_line == "");
  serial_line.toCharArray(c_serial_line,serial_line.length()+1);
  Serial.println(serial_line);
  return parse_cmd(c_serial_line);
}
//Group: Command Gathering Function
/* 
  Function: wait_lora_cmd

    Determine the types of data to be sampled from the sensors based on given ARQCMD.
    ARQCMD is read from the defined DATALOGGER. It differs from wait_arq_cmd as it gets 
    its command from arq, this commands from lora. 
  
  Parameters:
  
    n/a
  
  Returns:
  
    1 - tilt

    2 - tilt and soil moisture
  
  See Also:
  
    - <loop> <parse_cmd>
*/
int wait_lora_cmd(){
  String serial_line; 
  char c_serial_line[30];  
  while(!LORA.available());
  arq_start_time = millis(); // Global variable used by arqwait_delay
  do{
    serial_line = LORA.readStringUntil('\r\n');
  } while(serial_line == "");
  serial_line.toCharArray(c_serial_line,serial_line.length()+1);
  Serial.println(serial_line);
  return parse_cmd(c_serial_line);
}

/* 
  Function: wait_xbee_cmd

    Waits for a frame from the xbee and writes it to the response_string

  Parameters:

    timeout - integer in milliseconds that determines the timeout value for waitig

    response_string - char array that will contain the response from the xbee

  See Also:

    - <parse_cmd> <loop>
*/
int wait_xbee_cmd(int timeout, char* response_string){
  char temp[1];
  xbee_response[0] = '\0';
  long start = millis();
  while((millis() - start) < 180000){ // hardcode na 3 minutes of waiting time
    xbee.readPacket(); 
    if (xbee.getResponse().isAvailable()) {
        if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
            xbee.getResponse().getZBRxResponse(rx);
            for (int i = 0; i < rx.getDataLength (); i++){
              temp[0] = (char)rx.getData(i);
              strncat(response_string,temp,1 ) ;        
            }
        }
        Serial.print("response_string: ");
        Serial.println(response_string);
        return parse_cmd(response_string);
    } else if ( xbee.getResponse().isError()){
          Serial.print("Error ");
          Serial.println(xbee.getResponse().getErrorCode());
    }
  }
  Serial.print("response_string: ");
  Serial.println(response_string);
  return 0;
}

/* 
  Function: parse_cmd

    Determine the types of data to be sampled from the sensors based on input command_string.
    <wait_arq_cmd> and <wait_xbee_cmd> both return a call to <parse_cmd>.

    *From <wait_arq_cmd>:*
    --- Code
    return parse_cmd(c_serial_line);
    ---

    *From <wait_xbee_cmd>*
    --- Code
    return parse_cmd(response_string);
    ---

  Parameters:
  
    command_string - char* to char array that contains the command to be interpreted
  
  Returns:
  
    1 - tilt

    2 - tilt and soil moisture
  
  See Also:
  
    - <wait_arq_cmd> <wait_xbee_cmd>
*/
int parse_cmd(char* command_string){
  String serial_line; 
  String command, temp_time,purged_time;
  char c_serial_line[30];  
  char * pch;
  char cmd[] = "CMD6";
  int cmd_index,slash_index;

  serial_line = String(command_string);
  Serial.println(serial_line);

  if ((pch = strstr(command_string,cmd)) != NULL) {
    if (*(pch+strlen(cmd)) == 'S'){ // SOMS + TILT
      cmd_index = serial_line.indexOf('S',6); // ARQCMD has 6 characters
      temp_time = serial_line.substring(cmd_index+1); 
      slash_index = temp_time.indexOf('/');
      temp_time.remove(slash_index,1);
      g_timestamp = temp_time;
      Serial.print("g_timestamp: ");
      Serial.println(g_timestamp);
      return 2;
    } else if (*(pch+strlen(cmd)) == 'T'){ // TILT Only
      cmd_index = serial_line.indexOf('T',6); // ARQCMD has 6 characters
      temp_time = serial_line.substring(cmd_index+1); 
      slash_index = temp_time.indexOf('/');
      temp_time.remove(slash_index,1);
      g_timestamp = temp_time;
      Serial.print("g_timestamp: ");
      Serial.println(g_timestamp);
      return 1;
    } else {
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
void build_txt_msgs(char mode[], char* source, char* destination){
  char *token1,*token2;
  char dest[5000] = {};
  char idf = '0';
  char identifier[2] = {};
  char temp[6];
  char temp_id[5];
  char master_name[8] = "";
  int cutoff = 0, num_text_to_send = 0, num_text_per_dtype = 0;
  int name_len = 0,char_cnt = 0,c=0;
  int i,j;
  int token_length = 0;
  char pad[12] = "___________";
  
  Serial.println(g_final_dump); //Print final dump data
  for (int i = 0; i < 5000; i++) {
      destination[i] = '\0';
  }

  String timestamp = getTimestamp(mode);
  char Ctimestamp[13] = "";
  if (strcmp(comm_mode, "LORA") == 0){
      for (int i = 0; i < 13; i++) {
      Ctimestamp[i] = timestamp[i];
    }
  }else{
  for (int i = 0; i < 12; i++) {
      Ctimestamp[i] = timestamp[i];
    }
  }
  
  //Ctimestamp[12] = '\0';
  
  token1 = strtok(source, g_delim);
  while ( token1 != NULL){
    c=0;
    idf = check_identifier(token1,4);
    identifier[0] = idf;
    identifier[1] = '\0';
    cutoff = check_cutoff(idf);
    remove_extra_characters(token1, idf);

    writeData(timestamp,String(token1));
    num_text_per_dtype = ( strlen(token1) / cutoff );
    if ((strlen(token1) % cutoff) != 0){
      num_text_per_dtype++;
    }

    if (g_sensor_version == 1){
        name_len = 8;
        strncpy(master_name,g_mastername,4);
        strncat(master_name,"DUE",4);
    } else if ( idf == 'p'){
        name_len = 8;
        strncpy(master_name,g_mastername,6);
        strncat(master_name,"PZ",2);
    } else {
        name_len = 6;
        strncpy(master_name,g_mastername,6);
    }

    token_length = strlen(token1); 

    for (i = 0; i < num_text_per_dtype; i++){
     if (strcmp(comm_mode, "LORA") == 0){
     strncat(dest,pad,2);
      }
     else{
     strncat(dest,pad,11);
      }
      strncat(dest,master_name, name_len); 
      strncat(dest,"*", 2);
      if (g_sensor_version != 1){ // except piezo and v1 // or idf != 'p' (Tinangal ko muna ito)
          strncat(dest,identifier,2);
          strncat(dest,"*", 2);
      }
      for (j=0; j < (cutoff); j++ ){
        strncat(dest,token1,1);
        c++;
        token1++;
        if (c == (token_length)){
          break;
        }
      }
      if (strcmp(comm_mode,"XBEE") == 0){
      // Baka dapat kapag V3 ito. 
        strncat(dest,"*",1);
        strncat(dest,Ctimestamp,12);
      }
      if (strcmp(comm_mode, "LORA") == 0){
        strncat(dest,"*",1);
        strncat(dest,Ctimestamp,13);
        strncat(dest,g_delim,1);   
        } else{
      strncat(dest,"<<",2);
      strncat(dest,g_delim,1);          
          
          }

    }
    num_text_to_send = num_text_to_send + num_text_per_dtype;
    token1 = strtok(NULL, g_delim);
  }
  token2 = strtok(dest, g_delim);
  c=0;
  while( token2 != NULL ){
    c++;
    if (strcmp(comm_mode, "LORA") == 0){      
    idf = check_identifier(token1,2);
    identifier[0] = idf;
    identifier[1] = '\0';
    sprintf(pad, "%02s", ">>");
    strncpy(token2,pad,2);
    // strncat(token2,"<<",3);
    Serial.println(token2);
    strncat(destination,token2, strlen(token2));
    strncat(destination, g_delim, 2);
    token2 = strtok(NULL, g_delim);  
      
    }else{
    char_cnt = strlen(token2) + name_len - 24;
    idf = check_identifier(token1,2);
    identifier[0] = idf;
    identifier[1] = '\0';
    sprintf(pad, "%03d", char_cnt);
    strncat(pad,">>",3);
    sprintf(temp, "%02d/", c);
    strncat(pad,temp,4);
    sprintf(temp,"%02d#",num_text_to_send);
    strncat(pad,temp,4);
    strncpy(token2,pad,11);
    // strncat(token2,"<<",3);
    Serial.println(token2);
    strncat(destination,token2, strlen(token2));
    strncat(destination, g_delim, 2);
    token2 = strtok(NULL, g_delim);
  }
  }
  if (destination[0] == '\0'){
    no_data_parsed(destination);
    writeData(timestamp,String("*0*ERROR: no data parsed"));
  }
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
void remove_extra_characters(char* columnData, char idf){
  int i = 0,cmd = 1;
  char pArray[2500] = "";
  int initlen = strlen(columnData);
  char *start_pointer = columnData;

  // dagdagan kapag kailangan
  if (idf == 'd'){
    cmd = 9;
  } else if ( (idf == 'x' ) || (idf == 'y') ){
    cmd = 1;
  } else if ((idf == 'p')){
    cmd = 10;
  } else if (idf=='b'){
    cmd = 3;
  } else if (idf == 'c'){
    cmd = 3;
  }
  for (i = 0; i < initlen; i++, columnData++) {
  // for (i = 0; i < 23; i++,) {
    switch (cmd) {
      case 1: {// axel data //15
        if (i % 20 != 0 && i % 20 != 1  && i % 20 != 8 && i % 20 != 12 && i % 20 != 16 ) {
          strncat(pArray, columnData, 1);
        }
        break;
      }
      case 2: { // calib soms // 10
        if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 != 12 && i % 20 < 14 ) {
          strncat(pArray, columnData, 1);
        }
        break;
      }
      case 3: { //raw soms //7
          if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 < 13 ) { // dating 11 yung last na number
          strncat(pArray, columnData, 1);
        }
        break;
      }
      case 4: { // old format
        if (i%20 != 0 && i%20!= 1 && i%20 != 4 && i%20 != 8 && i%20 != 12){
          strncat(pArray, columnData, 1);
        }
        break;
      }
      case 8: { // old axel /for 15
        if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 != 12 && i % 20 != 16 ) {
          strncat(pArray, columnData, 1);
        }
        break;
      }
      case 9: {  // diagnostics for v3 sensors //takes temp only 
      // final format is gid(2chars)msgid(2chars)temp_hex(4chars)
        if (i % 20 != 0 && i % 20 != 1 && i % 20 != 6 && i % 20 != 7 && i % 20 != 8 && i % 20 != 9 && i % 20 != 14 && i % 20 != 15 && i % 20 != 16 && i % 20 != 17 && i % 20 != 18 && i % 20 != 19 ) {
          strncat(pArray, columnData, 1);
        }
        break;
      }
      case 10:{
        if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 != 12 && i % 20 != 16){
          strncat(pArray, columnData, 1);
        }
        break;
      }
      case 41: { // axel wrong gids
        if (i % 20 != 4 && i % 20 != 5 && i % 20 != 8 && i % 20 != 12 && i % 20 != 16 ) {
          strncat(pArray, columnData, 1);
        }
        break;
      }
    }
  }

  columnData = start_pointer;
  sprintf(columnData, pArray,strlen(pArray));
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
char check_identifier(char* token, int index_msgid){
  char idfier = '0';
  char *last_char;
  int id_int;
  char temp_id[5];

  strncpy(temp_id,token,4);
  id_int = strtol(temp_id,&last_char,16);
  if (g_sensor_version == 1) {
     idfier = '\0';
    }
  else if (id_int == 255) {
    idfier = 'p';
    return idfier;
  } else {
    switch (token[index_msgid]) {
        case '1': {
          if (token[index_msgid+1] == '5') {
            idfier = 'b';
          }
          else if (token[index_msgid+1] == 'A') {
            idfier = 'c';
          }
          else if (token[index_msgid+1] == '6') {
            idfier = 'd';
          }
          break;
        } case '0': {
          if (token[index_msgid+1] == 'B')
            idfier = 'x';
          else if (token[index_msgid+1] == 'C')
            idfier = 'y';
          else if (token[index_msgid+1] == '8')
            idfier = 'x';
          else if (token[index_msgid+1] == '9')
            idfier = 'y';
          else if (token[index_msgid+1] == 'A')
            idfier = 'b';
          else if (token[index_msgid+1] == 'D')
            idfier = 'c';
          break;
        } case '2': {
         //Serial.println(token[index_msgid+1]);
         if (token[index_msgid+1] == '0'){
           idfier = 'x';
         } else if (token[index_msgid+1] == '1'){
           idfier = 'y';
         } else if (token[index_msgid+1] == '9'){ //Version 4 idf
           idfier = 'x';
         }else if (token[index_msgid+1] == 'A'){
           idfier = 'y';
         }
         break;
        } case '6': {
          idfier = 'b';
          break;
        } case '7': {
          idfier = 'c';
          break;
        } case 'F' :{
          idfier = 'p';
        } default: {
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
int check_cutoff(char idf){
  int cutoff=0;
  switch (idf) {
    case 'b': {
      cutoff = 130;
      break;
    } case 'x': {
      cutoff = 135;  //15 chars only for axel
      break;
    } case 'y': {
      cutoff = 135;  //15 chars only for axel
      break;
    } case 'c': {
      cutoff = 133;  //15 chars only for axel
      break;
    } case 'd': {
      cutoff = 136;  //15 chars only for axel
      break;
    } case 'p' :{
      cutoff = 135; 
      break;
    }

    default: {
            cutoff = 0;
            if (g_sensor_version ==1){
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
void no_data_parsed(char* message){
  if(strcmp(comm_mode, "LORA") == 0){
  strncat(message, g_mastername, 5);
  strncat(message, "*0*ERROR: no data parsed<<+", 27);
    }else{
  sprintf(message, "040>>1/1#", 3);
  strncat(message, g_mastername, 5);
  strncat(message, "*0*ERROR: no data parsed<<+", 27);
}
}

//Group: Auxilliary Control Functions

//Function: shut_down()
// Sends a shutdown command to the 328 PowerModule.
void shut_down(){
    powerM.println("PM+D");
}

//Function: turn_on_column
// Assert GPIO ( defined by RELAYPIN ) high to turn on sensor column.
void turn_on_column(){
  digitalWrite(RELAYPIN, HIGH);
  digitalWrite(LED1,HIGH);
  arqwait_delay(g_turn_on_delay);
}

//Function: turn_off_column
// Assert GPIO ( defined by RELAYPIN ) low to turn off sensor column.
void turn_off_column(){
  digitalWrite(RELAYPIN, LOW);
  digitalWrite(LED1, LOW);
  arqwait_delay(1000);
}

/* 
  Function: arqwait_delay()

    A delay function that sends ARQWAIT every

  Parameters:

    milli_secs - int milliseconds of delay

  Returns:

    n/a

  See Also:

    <process_g_string>
*/
void arqwait_delay(int milli_secs){
  int func_start = 0;
  if (strcmp(comm_mode,"ARQ") == 0){
    while ( (millis() - func_start ) < milli_secs ){
      if ( (millis() - arq_start_time) >= ARQTIMEOUT ) {
        arq_start_time = millis();
        Serial.println("ARQWAIT");
        DATALOGGER.print("ARQWAIT");
      }
    }
  } else {
    delay(milli_secs);
  }
}

//Group: Sending Related Functions

/* 
  Function: send_data
  
    * Sends data to the defined DATALOGGER serial. 

    * Manages the retry for sending.
  
  Parameters:
  
    isDebug - boolean that states whether in debug mode or not.

    columnData - array of characters that contain the messages to be sent
  
  Returns:

    n/a
  
  See Also:
  
    - <operation>
*/
void send_data(bool isDebug, char* columnData){
  int timestart = millis();
  int timenow = millis();
  bool OKFlag = false;
  uint8_t send_retry_limit = 0; 
  if (isDebug == true){
    // Serial.print("Sending: ");
    // Serial.println(columnData);

    do{
        timestart = millis();
        timenow = millis();
        while (!Serial.available()){
            while ( timenow - timestart < 9000 ) {
                timenow = millis();
            }
            Serial.println("Time out...");
            break;
        }
        if (Serial.find("OK")){
            OKFlag = true;
            Serial.println("moving on");
        } else{
            Serial.println(columnData);       
        }
    } while (OKFlag == false);
  } else {
      do{   
        Serial.print("Sending:");
        Serial.println(columnData);
        DATALOGGER.println(columnData);
        
        timestart = millis();
        timenow = millis();
        while (!DATALOGGER.available()){
          while ( timenow - timestart < 9000 ) {
            timenow = millis();
          }
          DATALOGGER.println("Time out...");
          break;
        }

        if (DATALOGGER.find("OK")){
          OKFlag = true;
        }
        else{

           DATALOGGER.println(columnData);       
          
        }

        send_retry_limit++;
        if (send_retry_limit == 10){
          Serial.println("send_retry_limit reached.");
          OKFlag = true;
        }
      } while (OKFlag == false);
  }
  
  return;
} 

/* 
  Function: send_lora_data
  
    * Sends data to the defined LORA serial. 

    * Manages the retry for sending.
  
  Parameters:
  
    isDebug - boolean that states whether in debug mode or not.

    columnData - array of characters that contain the messages to be sent
  
  Returns:

    n/a
  
  See Also:
  
    - <operation>
*/

void send_thru_lora(bool isDebug, char* columnData){
  int timestart = millis();
  int timenow = millis();
  bool OKFlag = false;
  uint8_t send_retry_limit = 0; 
  if (isDebug == true){
    // Serial.print("Sending: ");
    // Serial.println(columnData);

    do{
        timestart = millis();
        timenow = millis();
        while (!Serial.available()){
            while ( timenow - timestart < 9000 ) {
                timenow = millis();
            }
            Serial.println("Time out...");
            break;
        }
        if (Serial.find("OK")){
            OKFlag = true;
            Serial.println("moving on");
        } else{
            Serial.println(columnData);       
        }
    } while (OKFlag == false);
  } else {
      do{   
        Serial.print("Sending: ");
        Serial.println(columnData);
        LORA.println(columnData);

        timestart = millis();
        timenow = millis();
        while (!LORA.available()){
          while ( timenow - timestart < 9000 ) {
            timenow = millis();
          }
          LORA.println("Time out...");
          break;
        }

        if (LORA.find("OK")){
          OKFlag = true;
        }
        else{
           LORA.println(columnData);     
        }

        send_retry_limit++;
        if (send_retry_limit == 10){
          Serial.println("send_retry_limit reached.");
          OKFlag = true;
        }
      } while (OKFlag == false);
  }
  return;
} 
/* 
  Function: send_thru_xbee  

     Sends data thru designated xbee Serial port 

  Parameters:
  
    load_data -  char array to be forwarded to sbee
  
  Returns:

    boolean - States whether data is successfully sent or nor  
*/
bool send_thru_xbee(char* load_data) {
  bool successFlag= false;
  int count_success=0;
  int verify_send[24]={0};
  delay(500);
  int length = strlen(load_data);

  int i=0, j=0;    

  for (j=0;j<200;j++){
      payload[j]=(uint8_t)'\0';
  }

  for (j=0;j<length;j++){
      payload[j]=(uint8_t)load_data[j];
  }
    payload[j]= (uint8_t)'\0';
    xbee.send(zbTx);

    if (xbee.readPacket(2000)) {
        if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
            xbee.getResponse().getZBTxStatusResponse(txStatus);
            if (txStatus.getDeliveryStatus() == SUCCESS) {
                Serial.println(F("Send Success!"));
                if (verify_send[i] == 0){
                    count_success=count_success+1;
                    verify_send[i]=1;
                } 
                successFlag= true;
            } else {
                Serial.println(F("Send Failed!"));
            }
        }
    } else if (xbee.getResponse().isError()) {
        Serial.print(F("Error: "));
        Serial.println(xbee.getResponse().getErrorCode());
    } else {
      Serial.println(F("Error: Others"));
    }
  delay(1000);
  return successFlag;
}
/*
  Function: serial_loopback
    Read and write loopback in Serial

  Parameters:
    inByte - integer to read/ to write

  Returns:

    Sent integer


*/
void serial_loopback(){
  Serial1.begin(9600);
  while(1){
    if (Serial.available()) {
      int inByte = Serial.read();
      Serial1.write(inByte);
    }
    if (Serial1.available()) {
      int inByte = Serial1.read();
      Serial.write(inByte);
    }
  }
}

/*
  Function: serial_loopback
    Read and write loopback in Serial2

  Parameters:
    inByte - integer to read/ to write

  Returns:

    Sent integer


*/
void serial_loopback2(){
  Serial2.begin(9600);
  while(1){
    if (Serial2.available()) {
      int inByte = Serial2.read();
      Serial.write(inByte);
    }
    if (Serial.available()) {
      int inByte = Serial.read();
      Serial2.write(inByte);
    }
  }
}
