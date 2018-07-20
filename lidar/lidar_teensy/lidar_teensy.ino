#include <Snooze.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include "Sodaq_DS3231.h"
#include <LIDARLite.h>
#include <Adafruit_INA219.h>
#include <string.h>
#include <XBee.h>
#include <SD.h>
#include <SPI.h>
#include <Adafruit_LSM9DS1.h>
#include <Adafruit_Sensor.h>  // not used in this demo but required!

#define DEBUG 1	//0,1-turn on debug mode 
#define BAUDRATE 115200
#define XBEEBAUD 9600
#define XBLEN 200  //paylenght+2(identifier)+3(randnum)+1(null) 83
#define PAYLEN 200
#define ISRPIN 22
#define PWRPIN 23
#define LEDPIN 6

uint8_t store_rtc = 0;   //store alarm

SnoozeDigital digital;    // Load drivers
SnoozeBlock config(digital);    // install drivers to a SnoozeBlock

int xbFlag=0;
int exc=0;
int parts=0;
int length=0;
int paylength=200;
int datalen=0;
int count_success=0;
int verify_send[24]={0};
           

char g_mastername[6] = "IMULA";
char streamBuffer[250];
char s_ave_distance[10] = "";     //store average distance reading
char l_loadvoltage[10] = "";      //store load voltage
char l_current_mA[10] = "";       //store load current reading
char l_temp[10] = "";       //store RTC temperature
char Ctimestamp[13] = "";	//store currrent timestamp
char l_ACx[7] = "";
char l_ACy[7] = "";
char l_ACz[7] = "";
char l_MGx[7] = "";
char l_MGy[7] = "";
char l_MGz[7] = "";
char l_GRx[7] = "";
char l_GRy[7] = "";
char l_GRz[7] = "";

const int numReadings = 1000;   //max reading average

bool TRIG = false;

// i2c
Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();
Adafruit_INA219 ina219;
LIDARLite myLidarLite;

// set up variables using the SD utility library functions:
File myFile;
const int chipSelect = 10;

//Initialize xbee 
uint8_t payload[XBLEN];

XBee xbee = XBee();

XBeeAddress64 addr64 = XBeeAddress64(0x00, 0x00);  //Coordinator
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse();   // create reusable response objects for responses we expect to handle 
ModemStatusResponse msr = ModemStatusResponse();

void setup() {
  
  pinMode(LEDPIN, OUTPUT);

	Serial.begin(BAUDRATE);
	Serial3.begin(9600);
	xbee.setSerial(Serial3);

  // init_9dof();
 
  init_sd();
	// ina219.begin();


	rtc.begin();
	pinMode(ISRPIN, INPUT);
	init_pwrPin();

  /******************Snooze Driver Config******************/
  // Configure pin 21 for Snooze wakeup, also used by bounce
  // but no need to reconfigure the pin after waking Snooze
  // does it for you.
  digital.pinMode(21, INPUT_PULLUP, FALLING);//pin, mode, type
  /****************End Snooze Driver Config****************/

  //for external interrupt pin
	// attachInterrupt(21, ISR0_ISR, FALLING);

  rtc.enableInterrupts(store_rtc, 00);    // interrupt at (m,s)
	// Enable Interrupt 
  // rtc.enableInterrupts(Every10Minute); //interrupt at  EverySecond, EveryMinute, EveryHour
  //Enable HH/MM/SS interrupt on /INTA pin. All interrupts works like single-shot counter
  // rtc.enableInterrupts(10,53,20);    // interrupt at (h,m,s)
}

void init_in219(){
  uint32_t currentFrequency;
  ina219.setCalibration_16V_400mA();  
}

void setAlarm(){
  DateTime now = rtc.now(); //get the current date-time
  Serial.print("Minutes: ");
  Serial.print(now.minute());
  // if (DEBUG == 1) {Serial.print("Minutes: ");}
  // if (DEBUG == 1) {Serial.println(now.minute());}  

  if((now.minute() >= 0) && (now.minute() <=9)){
    store_rtc = 10;
    // rtc.enableInterrupts(store_rtc, 00);    // interrupt at (m,s)
  }
  else if((now.minute() >= 10) && (now.minute() <=19)){
    store_rtc = 20;
    // rtc.enableInterrupts(store_rtc, 00);    // interrupt at (m,s)
  }
  else if((now.minute() >= 20) && (now.minute() <=29)){
    store_rtc = 30;
    // rtc.enableInterrupts(store_rtc, 00);    // interrupt at (m,s)
  }
  else if((now.minute() >= 30) && (now.minute() <=39)){
    store_rtc = 40;
    // rtc.enableInterrupts(store_rtc, 00);    // interrupt at (m,s)
  }
  else if((now.minute() >= 40) && (now.minute() <=49)){
    store_rtc = 50;
    // rtc.enableInterrupts(store_rtc, 00);    // interrupt at (m,s)
  }
  else if((now.minute() >= 50) && (now.minute() <=59)){
    store_rtc = 0;
  }  
  // else{
  //   store_rtc = 0;
    // rtc.enableInterrupts(store_rtc, 00);    // interrupt at (m,s)
  // }

  rtc.enableInterrupts(store_rtc, 00);    // interrupt at (m,s)
  Serial.print(" - next alarm: ");
  Serial.println(store_rtc);
  // if (DEBUG == 1) {Serial.print(" - next alarm: ");}
  // if (DEBUG == 1) {Serial.println(store_rtc);}
}


void loop() {
  sleepDataLogger();
  // getAtcommand();
}

void trig(){
/*
  if(TRIG){
    Serial.print("ISR Triggered: ");
    // readTimeStamp();
    // setAlarm();
    turnOn_pwr();
    delay(500);
    readTimeStamp();
    // readTemp();
    init_lidar();
    lidar();
    read_voltage();
    read_current();
    init_9dof();
    read_9dof();
    build_message();
    delay(1000);
      
     sendMessage();
    logData();
    delay(7000);
    turnOff_pwr();

    TRIG = false;
    rtc.clearINTStatus();   //preparing rtc to alarm trigger
    }
  */  
}

void delay_1sec(){
  int timestart = millis();
  int timenow = millis();
  timenow= millis();
  timestart= millis();

  while ( timenow - timestart < 500 ) {
    timenow = millis();

    // init_lidar();
    init_9dof();
  }

  timenow= millis();
  timestart= millis();
}

inline void sleepDataLogger() {
  /********************************************************
  feed the sleep function its wakeup parameters. Then go
  to deepSleep.
  ********************************************************/
  // delay_1sec();
  delay(100);
  Serial.println("Powering logger.");
  
  turnOn_pwr(); //power system
  delay(1000);
  init_in219();
  init_lidar();

  flashLed(LEDPIN, 5, 100);
  
  lidar();             
  readTimeStamp();
  delay(100);
  readTemp();

  read_voltage();
  read_current();
  init_9dof();
  read_9dof();

  delay(1000);
  build_message();

  delay(3000);
  setAlarm();
  send_thru_xbee(streamBuffer);
  logData();
  delay(2000);
  turnOff_pwr();
  Serial.println("Logger Will Sleep Now...");
  delay(100);  

  rtc.clearINTStatus(); // needed for the rtc to re-trigger
  // put Teensy into low power sleep mode
  Snooze.deepSleep( config );
}

void flashLed(int pin, int times, int wait) {
    
    for (int i = 0; i < times; i++) {
      digitalWrite(pin, HIGH);
      delay(wait);
      digitalWrite(pin, LOW);
      
      if (i + 1 < times) {
        delay(wait);
      }
    }
}

void init_lidar(){
  /*
    begin(int configuration, bool fasti2c, char lidarliteAddress)

    Starts the sensor and I2C.

    Parameters
    ----------------------------------------------------------------------------
    configuration: Default 0. Selects one of several preset configurations.
    fasti2c: Default 100 kHz. I2C base frequency.
      If true I2C frequency is set to 400kHz.
    lidarliteAddress: Default 0x62. Fill in new address here if changed. See
      operating manual for instructions.
  */
  myLidarLite.begin(0, true); // Set configuration to default and I2C to 400 kHz

  /*
    configure(int configuration, char lidarliteAddress)

    Selects one of several preset configurations.

    Parameters
    ----------------------------------------------------------------------------
    configuration:  Default 0.
      0: Default mode, balanced performance.
      1: Short range, high speed. Uses 0x1d maximum acquisition count.
      2: Default range, higher speed short range. Turns on quick termination
          detection for faster measurements at short range (with decreased
          accuracy)
      3: Maximum range. Uses 0xff maximum acquisition count.
      4: High sensitivity detection. Overrides default valid measurement detection
          algorithm, and uses a threshold value for high sensitivity and noise.
      5: Low sensitivity detection. Overrides default valid measurement detection
          algorithm, and uses a threshold value for low sensitivity and noise.
    lidarliteAddress: Default 0x62. Fill in new address here if changed. See
      operating manual for instructions.
  */
  myLidarLite.configure(4); // Change this number to try out alternate configurations  
}

void lidar(){
  float total;
  float ave_distance;     //distance lidar
  /*
    distance(bool biasCorrection, char lidarliteAddress)

    Take a distance measurement and read the result.

    Parameters
    ----------------------------------------------------------------------------
    biasCorrection: Default true. Take aquisition with receiver bias
      correction. If set to false measurements will be faster. Receiver bias
      correction must be performed periodically. (e.g. 1 out of every 100
      readings).
    lidarliteAddress: Default 0x62. Fill in new address here if changed. See
      operating manual for instructions.
  */

  // Take a measurement with receiver bias correction and print to serial terminal
  // Serial.println(myLidarLite.distance());

  // Take 99 measurements without receiver bias correction and print to serial terminal
  for(int i = 0; i < numReadings; i++)
  {
    // Serial.println(myLidarLite.distance());
    total = total + myLidarLite.distance(true);
  }

  ave_distance = total/numReadings;
  // Serial.println(total);
  // Serial.println(ave_distance);

  dtostrf(ave_distance, 3, 3, s_ave_distance);
  if (DEBUG == 1) {Serial.print("LIDAR Distance: ");}
  if (DEBUG == 1) {Serial.println(s_ave_distance);}
}

void init_sd(){
  if (DEBUG == 1) {Serial.println("Initializing SD card...");}
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    if (DEBUG == 1) {Serial.println("SD initialization failed!");}
    // don't do anything more:
    return;
  }
  // if (DEBUG == 1) {Serial.println("card initialized.");}
}

void init_pwrPin(){
	pinMode(PWRPIN, OUTPUT);
	digitalWrite(PWRPIN, LOW);
}

void turnOn_pwr(){
	if (DEBUG == 1) {Serial.println("Powering ON System  ....");}
	digitalWrite(PWRPIN, HIGH);
}

void turnOff_pwr(){
	if (DEBUG == 1) {Serial.println("Powering OFF System ...");}
	digitalWrite(PWRPIN, LOW);
}

void build_message(){  

  for (int i=0; i<100; i++) streamBuffer[i]=0x00;
    strncpy(streamBuffer,">>01/01#", 8);
    strncat(streamBuffer, g_mastername, sizeof(g_mastername));
    strncat(streamBuffer, "*L*LR:", 6);
    strncat(streamBuffer, s_ave_distance, sizeof(s_ave_distance));
    strncat(streamBuffer, "*BV:", 4);
    strncat(streamBuffer, l_loadvoltage, sizeof(l_loadvoltage));
    strncat(streamBuffer, "*BI:", 4);
    strncat(streamBuffer, l_current_mA, sizeof(l_current_mA));
    strncat(streamBuffer, "*AC:", 4);
    strncat(streamBuffer, l_ACx, sizeof(l_ACx));
    strncat(streamBuffer, ",", 1);
    strncat(streamBuffer, l_ACy, sizeof(l_ACy));
    strncat(streamBuffer, ",", 1);
    strncat(streamBuffer, l_ACz, sizeof(l_ACz));
    strncat(streamBuffer, "*MG:", 4);
    strncat(streamBuffer, l_MGx, sizeof(l_MGx));
    strncat(streamBuffer, ",", 1);
    strncat(streamBuffer, l_MGy, sizeof(l_MGy));
    strncat(streamBuffer, ",", 1);
    strncat(streamBuffer, l_MGz, sizeof(l_MGz));
    strncat(streamBuffer, "*GR:", 4);
    strncat(streamBuffer, l_GRx, sizeof(l_GRx));
    strncat(streamBuffer, ",", 1);
    strncat(streamBuffer, l_GRy, sizeof(l_GRy));
    strncat(streamBuffer, ",", 1);
    strncat(streamBuffer, l_GRz, sizeof(l_GRz));

    strncat(streamBuffer, "*TP:", 4);
    strncat(streamBuffer, l_temp, sizeof(l_temp)); 
    strncat(streamBuffer, "*", 1);
    strncat(streamBuffer, Ctimestamp, sizeof(Ctimestamp));
    strncat(streamBuffer, "<<", 2);
    // Serial.println(streamBuffer);
   // dataString.toCharArray(streamBuffer, sizeof(dataString));
   // Serial.println(dataString);
}


void logData(){  
  String dataString = "";   //hold string from streamBuffer

  Serial.println("Writing Data to SD card . . ");

  File dataFile = SD.open ("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(streamBuffer);
    dataFile.close();
  }  
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
}

void init_9dof(){
  // Try to initialise and warn if we couldn't detect the chip
  if (!lsm.begin())
  {
    Serial.println("Oops ... unable to initialize the LSM9DS1. Check your wiring!");
    while (1);
  }
  // Serial.println("Found LSM9DS1 9DOF");

  // 1.) Set the accelerometer range
  lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_4G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_8G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_16G);
  
  // 2.) Set the magnetometer sensitivity
  lsm.setupMag(lsm.LSM9DS1_MAGGAIN_4GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_8GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_12GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_16GAUSS);

  // 3.) Setup the gyroscope
  lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_245DPS);
  //lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_500DPS);
  //lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_2000DPS);
}

void read_9dof(){
  float ACx, ACy, ACz = 0;
  float MGx, MGy, MGz = 0;
  float GRx, GRy, GRz = 0;

  lsm.read();  /* ask it to read in the data */ 

  /* Get a new sensor event */ 
  sensors_event_t a, m, g, temp;

  lsm.getEvent(&a, &m, &g, &temp); 

  ACx = (a.acceleration.x);
  dtostrf(ACx, 3, 4, l_ACx);  
  if (DEBUG == 1) {Serial.print("Accel X: ");}
  if (DEBUG == 1) {Serial.print(l_ACx);}

  ACy = (a.acceleration.y);
  dtostrf(ACy, 5, 4, l_ACy);  
  if (DEBUG == 1) {Serial.print("\tY: ");}
  if (DEBUG == 1) {Serial.print(l_ACy);}

  ACz = (a.acceleration.z);
  dtostrf(ACz, 3, 4, l_ACz);  
  if (DEBUG == 1) {Serial.print("\tZ: ");}
  if (DEBUG == 1) {Serial.println(l_ACz);}

  MGx = (m.magnetic.x);
  dtostrf(MGx, 5, 4, l_MGx);  
  if (DEBUG == 1) {Serial.print("Mag X: ");}
  if (DEBUG == 1) {Serial.print(l_MGx);}

  MGy = (m.magnetic.y);
  dtostrf(MGy, 3, 4, l_MGy);  
  if (DEBUG == 1) {Serial.print("\tY: ");}
  if (DEBUG == 1) {Serial.print(l_MGy);}

  MGz = (m.magnetic.z);
  dtostrf(MGz, 3, 4, l_MGz);  
  if (DEBUG == 1) {Serial.print("\tZ: ");}
  if (DEBUG == 1) {Serial.println(l_MGz);}

  GRx = (g.gyro.x);
  dtostrf(GRx, 3, 4, l_GRx);  
  if (DEBUG == 1) {Serial.print("Gyro X: ");}
  if (DEBUG == 1) {Serial.print(l_GRx);}

  GRy = (g.gyro.y);
  dtostrf(GRy, 3, 4, l_GRy);  
  if (DEBUG == 1) {Serial.print("\tY: ");}
  if (DEBUG == 1) {Serial.print(l_GRy);}

  GRz = (g.gyro.z);
  dtostrf(GRz, 3, 4, l_GRz);  
  if (DEBUG == 1) {Serial.print("\tY: ");}
  if (DEBUG == 1) {Serial.println(l_GRz);}

/*
  Serial.print("Accel X: "); Serial.print(a.acceleration.x); Serial.print(" m/s^2");
  Serial.print("\tY: "); Serial.print(a.acceleration.y);     Serial.print(" m/s^2 ");
  Serial.print("\tZ: "); Serial.print(a.acceleration.z);     Serial.println(" m/s^2 ");

  Serial.print("Mag X: "); Serial.print(m.magnetic.x);   Serial.print(" gauss");
  Serial.print("\tY: "); Serial.print(m.magnetic.y);     Serial.print(" gauss");
  Serial.print("\tZ: "); Serial.print(m.magnetic.z);     Serial.println(" gauss");

  Serial.print("Gyro X: "); Serial.print(g.gyro.x);   Serial.print(" dps");
  Serial.print("\tY: "); Serial.print(g.gyro.y);      Serial.print(" dps");
  Serial.print("\tZ: "); Serial.print(g.gyro.z);      Serial.println(" dps");

  Serial.println();
  delay(200);  
*/
}

void send_thru_xbee(char* load_data) {
  bool successFlag= false;
  int count_success=0;
  int verify_send[24]={0};
  delay(500);
  int length = strlen(load_data);

  Serial.println(F("Sending data to gateway!"));
  Serial.println(load_data);
  // Serial.print(F("length = "));
  // Serial.println(length);
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
      // Serial.println(F("Got a response!"));
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
}