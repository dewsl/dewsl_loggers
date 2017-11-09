
#include "variant.h"
#include <due_can.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
// #include <avr/pgmspace.h>
#include <XBee.h>


#define VERBOSE 0

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

#define RELAYPIN 44
#define TIMEOUT 2500
#define POLL_TIMEOUT 1500
#define BAUDRATE 9600

#define DATALOGGER Serial1
#define powerM Serial2

#define CAN_ARRAY_BUFFER_SIZE 100

#define XBLEN 83 //paylenght+2(identifier)+3(randnum)+1(null)
#define PAYLEN 80

#define comm_mode 2 // 1 for ARQ, 2 XBEE
XBee xbee = XBee();
long timestart = 0;
long timenow = 0;
uint8_t payload[XBLEN];
XBeeAddress64 addr64 = XBeeAddress64(0x0013a200, 0x40F62F8A);
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();
XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse();

int xbFlag=0;

// SD-related / CONFIG-related
int g_gids[40][2];
int g_num_of_nodes = 40;
char g_mastername[6] = "XXXXX";
char g_timestamp[19] = "171009";
int g_chip_select = SS3;
int g_turn_on_delay = 10; // in centi seconds ( ie. 100 centiseconds = 1 sec) 
int g_sensor_version = 3;
int g_datalogger_version = 2;
int g_cd_counter = 0;


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
    }
    else if (DATALOGGER.available()) {
      Serial.println("DATALOGGER is available.");
      operation(wait_arq_cmd(), comm_mode);
    }
  }
  operation(1, comm_mode);
  shut_down();
  delay (1000);
}

//Function: getATCommand
// Take in-line serial input and execute AT command 
void getATCommand(){
  String serial_line, command;
  int i_equals = 0;
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
    }
    else if (command == ATECMDFALSE){
      ate = false;
      Serial.println(OKSTR);
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
      build_txt_msgs(g_final_dump, text_message);
      Serial.println(OKSTR);
    }
    else if (command == ATSNIFFCAN){
      while (true){
        Serial.println(OKSTR);
      }
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
    else{
      Serial.println(ERRORSTR);
    }
  }
  else 
    return;
}



void operation(int types, int mode){
  read_data_from_column(g_final_dump, g_sensor_version, types);
  build_txt_msgs(g_final_dump, text_message); 
  // Serial.println("txt_msgs built");
  if (mode == 1){
    Serial.println("mode 1");
    send_data(false, g_final_dump);
  }else if(mode == 2)
    send_thru_xbee(g_final_dump);
  else //default
    send_data(true, g_final_dump);
  //ioff na everything
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

//Function: turn_on_column
// Assert GPIO ( defined by RELAYPIN ) high to turn on sensor column.
void turn_on_column(){
  digitalWrite(RELAYPIN, HIGH);
  delay(g_turn_on_delay);
}

//Function: turn_off_column
// Assert GPIO ( defined by RELAYPIN ) low to turn off sensor column.
void turn_off_column(){
  digitalWrite(RELAYPIN, LOW);
  delay(1000);
}

//Function: read_data_from_column
// Collect data from sensors.
void read_data_from_column(char* column_data, int sensor_version, int types){
  if (sensor_version == 2){
    get_data(32,1,column_data);
    get_data(33,1,column_data);
    if (types == 2){
      get_data(110,1,column_data);
      get_data(112,1,column_data);
    }
  } else if (sensor_version == 3){
    get_data(11,1,column_data);
    get_data(12,1,column_data);
    get_data(22,1,column_data);
    if (types == 2){
      get_data(111,1,column_data);
      get_data(113,1,column_data);
    }
  } else if (sensor_version == 1){
    Serial.println("Not yet supported");
  }


    // get_data(110,1,column_data);
    // get_data(113,1,column_data);

    // get_data(113,1,column_data);
}

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

void build_txt_msgs(char* source, char* destination){

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

  // Serial.println("before getting timestamp");
  String timestamp = getTimestamp();
  Serial.println(timestamp);
  //I added this kuya kennex
  char Ctimestamp[12] = "";
    for (int i = 0; i < 12; i++) {
        Ctimestamp[i] = timestamp[i];
    }
    Ctimestamp[12] = '\0';
  // Use Ctimestamp
  ///////
  
  token1 = strtok(source, "+");
  while ( token1 != NULL){
    c=0;
    idf = check_identifier(token1,4);
    identifier[0] = idf;
    identifier[1] = '\0';
    cutoff = check_cutoff(idf);
    remove_extra_characters(token1, idf);
    writeData(String(token1));
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
      strncat(dest,"*",1);
      // strncat(dest,"__timestamp_",12);
      strncat(dest,Ctimestamp,12);
      strncat(dest,"+",1);
    }
    num_text_to_send = num_text_to_send + num_text_per_dtype;
    token1 = strtok(NULL, "+");
  }
  // Serial.println(dest);
  token2 = strtok(dest, "+");
  c=0;
  while( token2 != NULL ){
    c++;
    char_cnt = strlen(token2) + name_len - 24;
    idf = check_identifier(token1,2);
    identifier[0] = idf;
    identifier[1] = '\0';
    sprintf(pad, "%02d", char_cnt);
    strncat(pad,">>",3);
    sprintf(temp, "%02d/", c);
    strncat(pad,temp,4);
    sprintf(temp,"%02d#",num_text_to_send);
    strncat(pad,temp,4);
    Serial.println(token2);
    strncpy(token2,pad,11);
    Serial.println(token2);
    strncat(destination,token2, strlen(token2));
    strncat(destination, "+", 2);
    token2 = strtok(NULL, "+");
  }
  // Serial.println(destination);
  // Serial.println("build_txt_msgs finished.!!!");
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
      case 9: {  // diagnostics for v3 sensors
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
  Function: tokenize_data_by_data_type

    Remove specific characters unnecessary for data interpretation.
  
  Parameters:
  
    tokens - two dimensional char array that will contain the data split by data type.

    source - char array that contains the data that will be split.

    isDebug - boolean
  
  Returns:
  
    n/a
  
  See Also:
  
    <poll_data>
*/
void tokenize_data_by_data_type(char** tokens, char* source, bool isDebug){
  char *token;
  const char delimiter[2] = "+";
  int j=0;
  token = strtok(source, delimiter);
  strncat(token, "\0", 2);
  while (token != NULL) {
    sprintf(tokens[j], token, strlen(token));
    tokens[j][strlen(token)] = '\0';
    token = strtok(NULL, delimiter);
    delay(200);
    j++;
  }

  for (j; j < t_num_message_type; j++) {
    tokens[j]= '\0';
  }
}

/* 
  Function: tokenize_data_by_message

    Arrange the split data into a two dimensional array for convenient access.
  
  Parameters:
  
    message_array - two dimensional char array that will contain the data split by 
    cutoff_length. This will be the data in each text message.

    source - char array that contains the data that will be split.

    message_count - integer count of messages per data type.
  
  Returns:
  
    n/a
  
  See Also:
  
    <poll_data>
*/
void tokenize_data_by_message(char** message_array,char* source, int cutoff_length, int message_count){

  for (int i = 0; i < message_count; i++) {
    strncpy(message_array[i], source, cutoff_length);
    message_array[i][cutoff_length] = '\0';
    source = source + cutoff_length;
  } 
}

/* 
  Function: message_content_parameters

    Determine the following:

    * number of messages to be sent for each data type

    * identifier for each message to be sent

    * number of characters for each message to be sent 
    depending on the data type
  
  Parameters:
  
    parameters - integer array that contains the number of messages to be sent,
    ascii symbol number corresponding to the identifier, and the number of
    characters for a given message

    source_by_dtype - char array of data to be sent split by data type.
  
  Returns:
  
    n/a
  
  See Also:
  
    - <poll_data>

    - <check_identifier>

    - <check_cutoff>
*/
void message_content_parameters(int* parameters, char* source_by_dtype){
  char identifier = 'c';
  int cutoff_length = 0;
  int remove_extra_characters_cmd = 1; // by default

  identifier= check_identifier(source_by_dtype,4);
  cutoff_length = check_cutoff(identifier);
  if (identifier == 'd'){
    remove_extra_characters_cmd = 9;
  }
  parameters[1] = identifier;
  parameters[2] = cutoff_length;
  parameters[3] = remove_extra_characters_cmd;
}


void message_counter(int* parameters, char* source_by_dtype){
  int tokens_count = 0;
  int token_length = strlen(source_by_dtype);
  if (token_length == 0) {
    tokens_count = 0;
  } else {
    tokens_count = (token_length / parameters[2]);
    if (token_length % parameters[2] != 0) {
      tokens_count ++;
    }
  } 
  parameters[0] = tokens_count;
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
  Function: check_cutoff

    Determine the type of data expected from the sensors through the command
  
  Parameters:
  
    n/a
  
  Returns:
  
    1 - tilt

    2 - tilt and soil moisture

  
  See Also:
  
    - <loop>
*/
int wait_arq_cmd(){
  String serial_line, command;  
  Serial.println("poll naaaa~");

  do{
    serial_line = DATALOGGER.readStringUntil('\r\n');
  } while(serial_line == "");
    
  serial_line.toUpperCase();
  serial_line.replace("\r","");

  command = serial_line;
  if (command == "ARQCMD6T"){   // kailangang ayusin ito na CMD6T or CMD6S lang tinitingnan
    Serial.println(OKSTR);
    return 1; 
  }
  else if ( command == "ARQCMD6S"){
    Serial.println(OKSTR);
    return 2;
  }
  else
    return 0;
}

void no_data_parsed(char* message){
  
  sprintf(message, "040>>1/1#", 3);
  strncat(message, g_mastername, 5);
  strncat(message, "*0*ERROR: no data parsed<<", 26);
}


void send_data(bool isDebug, char* columnData){
  int timestart = millis();
  int timenow = millis();
  bool OKFlag = false;
  Serial.println("nasa send");
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

void read_current(){
  float current_mA = 0;
  current_mA = ina219.getCurrent_mA();
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
}


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

void send_thru_xbee(char* load_data) {
  int count_success=0;
  int verify_send[24]={0};
  Serial.println("now in sendMessage");
  delay(500);
  Serial.println(F("Start"));
  int length=strlen(load_data);
  
  int exc=length%PAYLEN;
  int parts=length/PAYLEN;
  Serial.print(F("length="));
  Serial.println(length);
  Serial.print(F("parts="));
  Serial.println(parts);
  Serial.print(F("excess="));
  Serial.println(exc);
  int datalen = 0;
  int i=0, j=0;    
  for (i=0;i<parts+1;i++){
    for (j=0;j<XBLEN+1;j++) payload[j]=0x00;

    delay(500);
       
    for (j=0;j<PAYLEN;j++){
      payload[j]=(uint8_t)load_data[datalen];
      datalen++;
    }

    Serial.println(datalen);  
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
            if (count_success==parts+1){
            }
          } 
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
  }
  Serial.println(F("exit send"));
  delay(1000);
  return;
}

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

String getTimestamp(){
    // Serial.println("before flush");
    // powerM.flush();
    // Serial.println("after flush");
    char timestamp[20] = "";    
    powerM.println("PM+R");
    timestart = millis();
    timenow = millis();
    while ( (!powerM.available()) && ( timenow - timestart < 7000 ) ) {
      // while ( timenow - timestart < 7000 ) {
          timenow = millis();
      // }
    }

    if (powerM.available()){
      powerM.readBytesUntil('\n', timestamp, 20);
      return timestamp; 
    } else {
      return String("1001011000");
    }

}

void shut_down(){
    powerM.println("PM+D");
}

