#include "variant.h"
#include <due_can.h>
#include <SD.h>
// #include <avr/pgmspace.h>

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
#define TIMEOUT 5000
#define BAUDRATE 9600

#define CAN_ARRAY_BUFFER_SIZE 100

// SD-related / CONFIG-related
int g_gids[40][2];
int g_num_of_nodes = 40;
char g_mastername[6] = "XXXXX";
char g_timestamp[19];
int g_chip_select = SS3;
int g_turn_on_delay = 10; // in centi seconds ( ie. 100 centiseconds = 1 sec) 
int g_sensor_version = 3;
int g_datalogger_version = 2;
int g_cd_counter = 0;


// CAN-related
// char *g_temp_dump = {};
// char *g_final_dump = {};
char g_temp_dump[1250];
char g_final_dump[2500];
char g_no_gids_dump[2500];
String g_string;
String g_string_proc;
int g_sampling_max_retry = 3;
CAN_FRAME g_can_buffer[CAN_ARRAY_BUFFER_SIZE];

// Text message related 
int t_num_message_type = 5; // 5 - ilang klase ng text messages ang gagawin
  // i.e. x - axel 1 , y - accel 2, b - raw soms , c - calib soms, ff - piezo



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
  delay(5000);
  Serial.begin(BAUDRATE);
  Serial.println("setup");
  pinMode(RELAYPIN, OUTPUT);
  init_can();
  init_char_arrays();
  init_gids();
  init_sd(); 
  open_config();
  print_stored_config();
  Serial.println("Receiving AT Command. . .");
}
//Function: loop
// Run the <getATcommand in loop.
void loop(){

  getATCommand();
}
//Function: getATCommand
// Take in-line serial input and execute AT command 
void getATCommand(){
  String serial_line, command;
  int i_equals = 0;
  
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
    // get_all_frames(TIMEOUT);
    Serial.println(OKSTR);
  }
  else if (command == ATGETSENSORDATA){
    read_data_from_column(g_final_dump, 1);
    Serial.println(OKSTR);
  }
  else if (command == "AT+POLL"){
    poll_data();
    Serial.println(OKSTR);
  }
  else if (command == ATSNIFFCAN){
    while (true){
      // get_all_frames(TIMEOUT);
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
  else{
    Serial.println(ERRORSTR);
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

void read_data_from_column(char* column_data, int sensor_version){
    get_data(11,1,column_data);
    get_data(12,1,column_data);
}
/* 
  Function: poll_data

    * Gets data from the sensor column via 
    <read_data_from_column>.

    * Splits the data by data type via 
    <tokenize_data_by_data_type>.

    * Generate the text message related parameters via 
    <message_content_parameters>. _Note: message_params[1]
    is an integer representing the ascii equivalent character.
    Hence the typecasting (char)._

    * Remove unnecessary characters from each message via 
    <remove_extra_characters>.

    * Arrange the messages into an array for easier access via 
    <tokenize_data_by_message>.

  Parameters:
  
    n/a
  
  Returns:
  
    n/a
  
  See Also:
  
    - <read_data_from_column>
    
    - <tokenize_data_by_data_type>
    
    - <message_content_parameters>
    
    - <remove_extra_characters>
    
    - <tokenize_data_by_message>
*/
void poll_data(){
  int message_params[4];
  read_data_from_column(g_final_dump, 1);
  g_cd_counter = strlen(g_final_dump);

  char** tokens_by_dtype = {};
  tokens_by_dtype = (char**)malloc(t_num_message_type*sizeof(char*));

  for(int i=0; i<t_num_message_type; i++){
    (tokens_by_dtype)[i] = (char*)malloc(1250*sizeof(char));
  }

  tokenize_data_by_data_type(tokens_by_dtype, g_final_dump, true);

  for (int k = 0; k < t_num_message_type; k++){
    message_content_parameters(message_params, tokens_by_dtype[k]);
    remove_extra_characters(tokens_by_dtype[k],1);
    Serial.println(tokens_by_dtype[k]);
    // Serial.print("tokens_count:"); Serial.println(message_params[0]);
    // Serial.print("identifier:"); Serial.println((char)message_params[1]);
    // Serial.print("cutoff_length:"); Serial.println(message_params[2]);

    char** message = {};
    message = (char**)malloc(message_params[0]*sizeof(char*));
    for(int i=0; i<message_params[0]; i++){
        (message)[i] = (char*)malloc(message_params[2]*sizeof(char));
    }
    tokenize_data_by_message(message, tokens_by_dtype[k], message_params[2], message_params[0]);
    // for (int i = 0; i < message_params[0]; i++) {
    //   Serial.print("message[");
    //   Serial.print(i);
    //   Serial.print("]:");
    //   Serial.println(message[i]);
    // }
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
void remove_extra_characters(char* columnData, int cmd){
  int i = 0;
  char pArray[2500] = "";
  int initlen = strlen(columnData);
  char *start_pointer = columnData;

  for (i = 0; i < initlen; i++, columnData++) {
  // for (i = 0; i < 23; i++,) {
    switch (cmd) {
      case 1: {// axel data //13
        if (i % 20 != 0 && i % 20 != 1 && i % 20 != 4 && i % 20 != 5 && i % 20 != 8 && i % 20 != 12 && i % 20 != 16 ) {
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
  int token_length = strlen(token);
  char identifier = 'c';
  int cutoff_length = 0;
  int tokens_count = 0;

  identifier= check_identifier(source_by_dtype,4);
  cutoff_length= check_cutoff(identifier);
  
  if (token_length == 0) {
    tokens_count = 0;
  } else {
    tokens_count = (token_length / cutoff_length);
    if (token_length % cutoff_length!= 0) {
      tokens_count ++;
    }
  } 

  parameters[0] = tokens_count;
  parameters[1] = identifier;
  parameters[2] = cutoff_length;
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

