#include "variant.h"
#include <due_can.h>
// #include <SPI.h>
#include <SD.h>

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
char g_temp_dump[1000];
String g_string;

// File g_file;

bool ate=true;

void setup() {
  Serial.begin(BAUDRATE);
  Serial.println("Receiving AT Command. . .");
  init_can();
  pinMode(RELAYPIN, OUTPUT);

}

void loop(){
  getATCommand();

}

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
    get_all_frames(TIMEOUT);
    Serial.println(OKSTR);
  }
  else if (command == ATSNDCAN){
    send_frame();
    Serial.println(OKSTR);
  }
  else if (command == ATGETSENSORDATA){
    turn_on_column();
    send_frame();
    get_all_frames(TIMEOUT);
    turn_off_column();
    Serial.println(OKSTR);
  }
  else if (command == ATSNIFFCAN){
    while (true){
      get_all_frames(TIMEOUT);
      Serial.println(OKSTR);
    }
  }
  else if (command == ATDUMP){
      Serial.print(g_string);
      process_g_string();
      Serial.println(OKSTR);
  }
  else if (command == ATSD){
      String conf;
      sd_init(SS3);
      conf = open_config();
      // Serial.println(conf);
      process_config(conf);
      Serial.println(OKSTR);
  }
  else{
    Serial.println(ERRORSTR);
  }
}

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