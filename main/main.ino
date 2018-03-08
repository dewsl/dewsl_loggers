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
#define DATALOGGER Serial1
#define powerM Serial2


#define VERBOSE 0
#define RELAYPIN 44
#define POLL_TIMEOUT 1500
#define BAUDRATE 9600
#define ARQTIMEOUT 30000


#define ENABLE_RTC 0
#define CAN_ARRAY_BUFFER_SIZE 100

#define comm_mode 1 // 1 for ARQ, 2 XBEE

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
uint8_t b64 = 0; //boolean for b64 operations.

uint8_t payload[200];
XBeeAddress64 addr64 = XBeeAddress64(0x00, 0x00); //sets the data that it will only send to the coordinator

//XBeeAddress64 addr64 = XBeeAddress64(0x0013a200, 0x40F62F77);
//XBeeAddress64 addr64 = XBeeAddress64(0x0013a200, 0x40F62F8A);

ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();
XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse();

uint8_t xbFlag=0;
int g_chip_select = SS3;
uint8_t datalogger_flag = 0;

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
  ---
*/
uint8_t g_sensor_version = 3;

/*
  Variable: g_datalogger_version
  integer that determines the datalogger version ( 2 ARQ or 3 Regular,V3 ). This variable is overwrittern by *<process_config_line>*. 
  
  Variable declaration:
  --- Code
  uint8_t g_datalogger_version = 3;
  ---
*/
uint8_t g_datalogger_version = 3;

/* 
  Variable: TIMEOUT
  integer that determines the tumeout duration of broadcast (in milliseconds ). This variable is overwrittern by *<process_config_line>*. 

  *SD Card Config Line* usage:
  --- Code
  brodcast_timeout = 3000
  ---
*/
int TIMEOUT = 3000;



// CAN-related
char g_temp_dump[1250];
char g_final_dump[2500];
char g_no_gids_dump[2500];
char text_message[5000];
String g_string;
String g_string_proc;
int g_sampling_max_retry = 3;
CAN_FRAME g_can_buffer[CAN_ARRAY_BUFFER_SIZE];

// Text message related 
int t_num_message_type = 6; // 5 - ilang klase ng text messages ang gagawin
  // i.e. x - axel 1 , y - accel 2, b - raw soms , c - calib soms, ff - piezo, d - diagnostics

/*
  Variable: g_timestamp
  global String that holds the timestamp.This variable is overwritten by the timestamp from the string sent by the ARQ. This variable defaults to "TIMESTAMP000".
*/
String g_timestamp = "TIMESTAMP000";

//current sensor
Adafruit_INA219 ina219;



bool ate=true;
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
  powerM.begin(9600);
  if (comm_mode == 2){
    xbee.setSerial(DATALOGGER);
  }
  ina219.begin();
  pinMode(RELAYPIN, OUTPUT);
  init_can();
  init_char_arrays();
  init_gids();
  init_sd(); 
  open_config();
  print_stored_config();
}
//Function: loop
// Run the <getATcommand> in loop.
void loop(){
  int timestart = millis();
  int timenow = millis();
  while ( timenow - timestart < 20000){
    timenow = millis();
    if (Serial.available()){
      getATCommand();  
      datalogger_flag = 1;
    }
    else if (DATALOGGER.available()) {
      if (comm_mode == 1){
        operation(wait_arq_cmd(), comm_mode);
        shut_down();
        datalogger_flag = 1;
      }
      else{
        datalogger_flag =0;
      }
    }
  }
  if (datalogger_flag == 0) {
    operation(1, comm_mode);
    Serial.println("Turning off ");
    shut_down();
  }
  delay (1000);
}

void hard_code(){
  String str1 = "column1 = 2748,1105,1098,1073,1069,971,942,925,920,888,873,827,807,671";
  String str2 = "MasterName = BOLTA";

  g_num_of_nodes = process_column_ids(str1);
  get_value_from_line(str2).toCharArray(g_mastername,6);
  
}
//Function: getATCommand
// Take in-line serial input and execute AT command 
void getATCommand(){
  String serial_line, command, extra_parameters;
  char converted[5] = {};
  char padded[5] = {};
  int i_equals = 0; // index of equal sign
  if (Serial.available()) {
    do{
      serial_line = Serial.readStringUntil('\r\n');
    } while(serial_line == "");
    
    serial_line.toUpperCase();
    serial_line.replace("\r","");

    // echo command if ate is set, default true
    if (ate) Serial.println(serial_line);
      i_equals = serial_line.indexOf('=');
    if (i_equals == -1) command = serial_line;
    else command = serial_line.substring(0,i_equals);
    
    if (command == ATCMD)
      Serial.println(OKSTR);
    else if (command == ATECMDTRUE){
      ate = true;
      Serial.println(OKSTR);
    } else if (command == "AT+B64"){
      extra_parameters = serial_line.substring(i_equals+1);
      to_base64(extra_parameters.toInt(),converted);
      pad_b64(1,converted,padded);
      Serial.print("converted and padded: ");
      Serial.print(padded);

    } else if (command == "AT+RTC"){
      extra_parameters = serial_line.substring(i_equals+1);
      set_rtc_time(extra_parameters);
    } else if (command == "AT+TIMENOW"){
      // if (ENABLE_RTC){
      //   Serial.print("Due RTC:");
      //   Serial.print(rtc.getDay());
      //   Serial.print("/");
      //   Serial.print(rtc.getMonth());
      //   Serial.print("/");
      //   Serial.print(rtc.getYear());
      //   Serial.print("\t");

      //   Serial.print(rtc.getHours());
      //   Serial.print(":");
      //   Serial.print(rtc.getMinutes());
      //   Serial.print(":");
      //   Serial.println(rtc.getSeconds());
      // }
      Serial.print("ARQ Time String: ");
      Serial.println(g_timestamp);
    }
    else if (command == ATECMDFALSE){
      ate = false;
      Serial.println(OKSTR);
    }
    else if (command == "AT+SWITCHB64"){
      if(b64){
        b64 = 0;
      } else {
        b64 = 1;
      }
      Serial.println("Toggled b64 operations.");
    }
    else if (command == ATRCVCAN){
      Serial.println(OKSTR);
    }
    else if (command == ATGETSENSORDATA){
      read_data_from_column(g_final_dump, g_sensor_version,1);
      Serial.println(OKSTR);
    }
    else if (command == "AT+POLL"){
      read_data_from_column(g_final_dump, g_sensor_version,1);
      build_txt_msgs(1, g_final_dump, text_message); //di ko to sure
      Serial.println(OKSTR);
    }
    else if (command == "AT+GETDATA"){
      get_data(11,1,g_final_dump);
      get_data(12,1,g_final_dump);
    }
    else if (command == ATSNIFFCAN){
      while (true){
        Serial.println(OKSTR);
      }
    }
    else if (command == "AT+S"){
      get_data(11,1,g_final_dump);
      get_data(12,1,g_final_dump);
    }
    else if (command == ATDUMP){
      Serial.print("g_final_dump: ");
      Serial.println(g_final_dump);
      Serial.println(OKSTR);
    }
    else if (command == ATSD){
      String conf;
      init_sd();
      open_config();
      Serial.println(F(OKSTR));
    }
    else if (command == "AT+SEND"){
      Serial.println(text_message);
      char *token1 = strtok(text_message,"+");
      while (token1 != NULL){
        send_data(false, token1);
        token1 = strtok(NULL, "+");
      }
    }
    else if (command == "AT+CURRENT"){
      read_current();
    }
    else if (command == "AT+VOLTAGE"){
      read_voltage();
    }
    else if (command == "AT+TIMESTAMPPMM"){
        String timestamp = getTimestamp(2);
        Serial.println(timestamp);
    }
    else if (command == "AT+LOOPSEND"){
      while(1){
        Serial.println("sent.");
        send_command(3,3);
        delay(1000);
      }
    }
    else{
      Serial.println(ERRORSTR);
    }
  }
  else 
    return;
}

/* 
  Function: operation

    Remove specific characters unnecessary for data interpretation.
  
  Parameters:
  
    types - integer that determines the kind of data requested from the sensor. *1 - tilt* *2 - tilt + soms*

    mode - integer that determines which DATALOGGER receives the sent data. *1- ARQ* *2- Xbee* 
  
  Returns:
  
    n/a
  
  See Also:
  
    <loop>

  Global Variables:

    g_final_dump, g_sensor_version, text_message
*/
void operation(int sensor_type, int communication_mode){
  int counter= 0;
  int num_of_tokens = 0;
  read_data_from_column(g_final_dump, g_sensor_version, sensor_type);// matagal ito.
  build_txt_msgs(communication_mode, g_final_dump, text_message); 
  char *token1 = strtok(text_message,"+");
    
  while (token1 != NULL){
    Serial.println(token1);
    if (communication_mode == 1) { // ARQ
      send_data(false, token1);    
    } else if(communication_mode == 2) { // XBEE
      while (send_thru_xbee(token1) == false){
        if (counter == 10)
          break;
        counter ++;
      }
    } else { //default
      send_data(true, token1); 
    }
    token1 = strtok(NULL, "+");
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


//Group: Data Gathering Functions
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
      get_data(110,1,column_data);
      get_data(112,1,column_data);
    }
  } else if (sensor_version == 3){
    get_data(11,1,column_data);
    get_data(12,1,column_data);
    get_data(22,1,column_data);
    if (sensor_type == 2){
      get_data(111,1,column_data);
      get_data(113,1,column_data);
    }
  } else if (sensor_version == 1){
    Serial.println("Not yet supported");
  }
}

//Function: read_current()
// Reads the current draw from the onboard ina219.
void read_current(){
  float current_mA = 0;
  current_mA = ina219.getCurrent_mA();
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
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
String getTimestamp(int communication_mode){

  if (communication_mode == 0){ //internal rtc
    return g_timestamp;
  } else if (communication_mode == 1){ // ARQ
    Serial.print("g_timestamp: ");
    Serial.println(g_timestamp);
    return g_timestamp;
  } else if(communication_mode == 2){ //xbee
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
  
    - <loop,arqwait_delay>
*/
int wait_arq_cmd(){
  String serial_line, command, temp_time,purged_time;
  char c_serial_line[30];  
  char * pch;
  char cmd[] = "CMD6";
  int cmd_index,slash_index;

  while(!DATALOGGER.available());
  
  arq_start_time = millis(); // Global variable used by arqwait_delay

  do{
    serial_line = DATALOGGER.readStringUntil('\r\n');
  } while(serial_line == "");
  
  serial_line.toCharArray(c_serial_line,serial_line.length());

  Serial.println(serial_line);


  if ((pch = strstr(c_serial_line,cmd)) != NULL) {
    if (*(pch+strlen(cmd)) == 'S'){ // SOMS + TILT
      cmd_index = serial_line.indexOf('S',6); // ARQCMD has 6 characters
      temp_time = serial_line.substring(cmd_index+1); 
      slash_index = temp_time.indexOf('/');
      temp_time.remove(slash_index,1);
      g_timestamp = temp_time;
      return 2;
      // Serial.print("g_timestamp: ");
      // Serial.println(g_timestamp);
    } else if (*(pch+strlen(cmd)) == 'T'){ // TILT Only
      cmd_index = serial_line.indexOf('T',6); // ARQCMD has 6 characters
      temp_time = serial_line.substring(cmd_index+1); 
      slash_index = temp_time.indexOf('/');
      temp_time.remove(slash_index,1);
      g_timestamp = temp_time;
      return 1;
      // Serial.print("g_timestamp: ");
      // Serial.println(g_timestamp);
    } else {
      Serial.println("wait_arq_cmd returned 0");
      return 0;
    }
  }
}

/* 
  Function: arqwait_delay()

    A delay function that sends ARQWAIT every

  Parameters:

    time - int milliseconds of delay

  Returns:

    n/a

  See Also:

    <process_g_string>
*/

void arqwait_delay(int milli_secs){
  int func_start = 0;
  if (comm_mode == 1){
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
//Group: Data parsing functions

/* 
  Function: build_txt_msgs

    Build the text messages to be sent.

    * Splits the data though the delimiter "+"

    * The identifier, and cutoff are identified based on the type of data.

    * Extra characters are removed via <remove_extra_characters>.

    * Data is stored in SD card via <writeData>.

    * The following are appended to the data split as text messages to be sent:

        * delimiters
        
        * mastername

        * padding for the numerator /  denominator

        * identifier

        * timestamp

    * The delimiter is a "+"

    * The appended data is copied per character to a temporary buffer.

    * The temporary buffer is split per message.

    * The pads are overwritten with the proper text message length,
    numerator and denominator.
  
    * These are then stored to the *destination* variable.

  Parameters:
  
    source - char array that contains the aggregated data

    destination - char array that will contain the text messages to be sent separated by
    the delimiter "+"
  
  Returns:
  
    n/a
  
  See Also:
  
    - <check_cutoff>
    
    - <remove_extra_characters>
    
    - <writeData>
*/

void build_txt_msgs(int mode, char* source, char* destination){

  char *token1,*token2;
  char dest[5000] = {};
  char idf = '0';
  char identifier[2] = {};
  char temp[6];
  char pad[12] = "___________";
  char master_name[8] = "";
  int cutoff = 0, num_text_to_send = 0, num_text_per_dtype = 0;
  int name_len = 0,char_cnt = 0,c=0;
  int i,j;
  int token_length = 0;

  for (int i = 0; i < 5000; i++) {
      destination[i] = '\0';
  }

  String timestamp = getTimestamp(mode);
  char Ctimestamp[12] = "";
  for (int i = 0; i < 12; i++) {
      Ctimestamp[i] = timestamp[i];
  }
  Ctimestamp[12] = '\0';
  
  token1 = strtok(source, "+");
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
    } else {
      name_len = 6;
      strncpy(master_name,g_mastername,6);
    }
    token_length = strlen(token1); 
    for (i = 0; i < num_text_per_dtype; i++){
      strncat(dest,pad,11);
      strncat(dest,master_name, name_len);
      strncat(dest,"*", 2);
      strncat(dest,identifier,2);
      strncat(dest,"*", 2);
      for (j=0; j < (cutoff); j++ ){
        strncat(dest,token1,1);
        c++;
        token1++;
        if (c == (token_length)){

          break;
        }
      }
      // Baka dapat kapag V3 ito. 
      // strncat(dest,"*",1);
      // strncat(dest,Ctimestamp,12);
      strncat(dest,"<<",2);
      strncat(dest,"+",1);
    }
    num_text_to_send = num_text_to_send + num_text_per_dtype;
    token1 = strtok(NULL, "+");
  }
  token2 = strtok(dest, "+");
  c=0;
  while( token2 != NULL ){
    c++;
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
    strncat(destination, "+", 2);
    token2 = strtok(NULL, "+");
  }

  if (destination[0] == '\0'){
    no_data_parsed(destination);
    writeData(timestamp,String("*0*ERROR: no data parsed"));
  }
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
      case 2: { // raw soms // 10
        if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 != 12 && i % 20 < 14 ) {
          strncat(pArray, columnData, 1);
        }
        break;
      }
      case 3: { //calib soms //7
          if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 < 10 ) {
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
      else if (token[index_msgid+1] == 'A')
        idfier = 'b';
      else if (token[index_msgid+1] == 'D')
        idfier = 'c';
      break;
    } case '2': {
      Serial.println(token[index_msgid+1]);
      if (token[index_msgid+1] == '0'){
        idfier = 'x';
      } else if (token[index_msgid+1] == '1'){
        idfier = 'y';
      }
      break;
    } case '6': {
      idfier = 'b';
      break;
    } case '7': {
      idfier = 'c';
      break;
    } 
    default: {
      idfier = '0';
      break;
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
    }
    case 'x': {
      cutoff = 135;  //15 chars only for axel
      break;
    }
    case 'y': {
      cutoff = 135;  //15 chars only for axel
      break;
    }
    case 'c': {
      cutoff = 133;  //15 chars only for axel
      break;
    }
    case 'd': {
      cutoff = 144;  //15 chars only for axel
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

    - Send this message instead if there is no availabe data from the sensor
    
    - Not yet working
  
  Parameters:
  
    message - empty char array 
*/

void no_data_parsed(char* message){
  
  sprintf(message, "040>>1/1#", 3);
  strncat(message, g_mastername, 5);
  strncat(message, "*0*ERROR: no data parsed<<+", 27);
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
  arqwait_delay(g_turn_on_delay);
}

//Function: turn_off_column
// Assert GPIO ( defined by RELAYPIN ) low to turn off sensor column.
void turn_off_column(){
  digitalWrite(RELAYPIN, LOW);
  arqwait_delay(1000);
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
  
    1 - tilt

    2 - tilt and soil moisture
  
  See Also:
  
    - <operation>
*/

void send_data(bool isDebug, char* columnData){
  int timestart = millis();
  int timenow = millis();
  bool OKFlag = false;
  if (isDebug == true){
    Serial.println("debug is true");
    Serial.println(columnData);

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
  }
  else {
    do{   
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

    } while (OKFlag == false);

  }
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
  Serial.println("now in sendMessage");
  delay(500);
  Serial.println(F("Start"));
  int length = strlen(load_data);

  Serial.print(F("length="));
  Serial.println(length);
  int i=0, j=0;    

  for (j=0;j<200;j++){
      payload[j]=(uint8_t)'\0';
  }

  for (j=0;j<length;j++){
      payload[j]=(uint8_t)load_data[j];
  }
    payload[j]= (uint8_t)'\0';
    
    Serial.println(F("sending before xbee.send"));
  
    xbee.send(zbTx);

    Serial.println(F("Packet sent"));
    if (xbee.readPacket(1000)) {
      Serial.println(F("Got a response!"));
      if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
        xbee.getResponse().getZBTxStatusResponse(txStatus);
        if (txStatus.getDeliveryStatus() == SUCCESS) {
          Serial.println(F("Success!"));
          if (verify_send[i] == 0){
            count_success=count_success+1;
            verify_send[i]=1;
          } 
          successFlag= true;
        } 
        else {
          Serial.println(F("myb no pwr"));
        }
      } 
      else{
      }
    } 
    else if (xbee.getResponse().isError()) {
      Serial.println(F("Error1"));
    } 
    else {
      Serial.println(F("Error2"));
    }
  Serial.println(F("exit send"));
  delay(1000);
  return successFlag;
}

/* 
  Function: get_xbee_flag

    - Sets the xbee flag 1 if received acknowledgement from the coordinator
*/
void get_xbee_flag(){
  Serial.println(F("Wait for xb"));  
  xbee.readPacket();
    
  if (xbee.getResponse().isAvailable()) {
    // got something
    Serial.println(F("We got something!"));
      
    if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
      // got a zb rx packet
      Serial.println(F("Izz zb rx packet!"));
        
      // now fill our zb rx class
      xbee.getResponse().getZBRxResponse(rx);
      for (int i = 0; i < rx.getDataLength (); i++)
        Serial.print((char) rx.getData(i));
          
        xbFlag = 1;
        Serial.println(F("xbFlag is set"));
          
        if (rx.getOption() == ZB_PACKET_ACKNOWLEDGED) {
          Serial.println(F("And sender got an ACK"));
        } else {
          Serial.println(F("But sender did not receive ACK"));
        }
      }
  }
  return;
  
  }



