#include<Wire.h>
#include <DueTimer.h>
#include <stdlib.h>
#define N_BUFFER 12000    //12000 orig
#undef HID_ENABLED
#include "Adafruit_MCP9808.h"

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

String args[5];

// Arduino Due ADC->DMA->USB 1MSPS
// by stimmer
// from http://forum.arduino.cc/index.php?topic=137635.msg1136315#msg1136315
// Input: Analog in A0
// Output: Raw stream of uint16_t in range 0-4095 on Native USB Serial/ACM

// on linux, to stop the OS cooking your data: 
// stty -F /dev/ttyACM0 raw -iexten -echo -echoe -echok -echoctl -echoke -onlcr

void setup(){
  Wire.begin();
  // SerialUSB.begin(0);
  //while(!SerialUSB);
  Serial1.begin(115200);
  //Serial.setTimeout(100);                                                                 
  //Serial.print("due\r\n");
  init_IRPIN();
  //MCP9808
  if (!tempsensor.begin()) {
    Serial1.println("Couldn't find MCP9808!");
    while (1);
  }  
}

void loop(){
  //sendIRPulse();
  
  uint16_t ch = 0;
    
  getATCommand();
       
   //getArguments("AT+UTS=0,15,re,67y",args);

/*
   for (uint8_t i=0; (i<5)&args[i]!=""; i++){
     Serial.print(i);
     Serial.print(": ");
     Serial.println(args[i]);
   }
   
   char cmd[20];

   Serial.readBytesUntil('a',cmd,20);

   //ch = getCommand();

   Serial.println(cmd);
  // sampleSignal(ch);
  // REG_PIOB_ODSR ^= (1<<25);
   // digitalWrite(2, HIGH);
   delayMicroseconds(50);

   digitalWrite(2, LOW);
   delay(100);

   //printValues();

   while (1);
   */
}
