#include <SPI.h>
#include <RH_RF95.h>
#include <Wire.h> //Needed for I2C to GNSS
#include <FlashStorage.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;

//for m0
#define RFM95_CS    8
#define RFM95_RST   4
#define RFM95_INT   3

// for teensy
//#define RFM95_CS 10
//#define RFM95_RST 9
//#define RFM95_INT 2

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 433.0
//#define RF95_FREQ 915.0 // 900 MHz LoRa

//max retry for getting rtcm data
#define MAX_RETRY 30

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Blinky on receipt
#define LED 13

// number of data to average
# define DATA_TO_AVERAGE 100

//initialize global variables
uint8_t *buf_total = (uint8_t *)malloc(sizeof(uint8_t) * 1000);
uint8_t *received = (uint8_t *)malloc(sizeof(uint8_t) * 251);

int tx_RSSI = 0; //tx rssi
char dataToSend[200];

typedef struct {
  boolean valid;
  int len;
  uint8_t rtcm[502];
} base_data;

/**
 * Reserve a portion of flash memory to store an "int" variable
 * and call it "alarmStorage".
*/
FlashStorage(flash_base_data, base_data);

void setup()
{
  //pinMode(LED, OUTPUT);
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }
  delay(100);
  Serial1.begin(115200);
    while (!Serial1) {
    delay(1);
  }
  Serial.println("Feather LoRa RX Test!");

  init_lora();
  init_ublox();
  Serial.println("UBLOX Rover");
  delay (120000); //kailangan to para maginitialize si ublox

}

void loop()
{
  int len1, len2;
  int len = 0, len_total = 0;
  long lastTime = millis();

  long timer2 = millis();
  do{
    len_total = 0;
    lastTime = millis();
    do{
      len = read_lora_data(len_total);
      if (len <= -1) break;
      len_total = len + len_total;
    }while (millis() - lastTime < 5000);  //5 secs
    
    if (len == -2) break;   
//  print_buf_total(len_total);
  
    if (len_total >2){
      //possible daw masira yung code pag laging nagstore sa flash, kaya every 4hrs lang sana
      //pero wag na to haha
      //rtcm_store_to_flash(len_total); 
  
      Serial.println("Serial write");
  
      for (int i = 0; i<len_total;i++){
        Serial1.write(buf_total[i]);
      }

    }

  } while(millis() - timer2 < 180000); //3 mins
  
  read_ublox_data();
  send_thru_lora(dataToSend);

}


void rtcm_store_to_flash(int len)
{
  //==========================================
  //store to flash
    base_data ublox_base;
    ublox_base = flash_base_data.read();
    ublox_base.len = len;
  
    for (int i =0; i<len; i++){
      ublox_base.rtcm[i] = buf_total[i];
    }
    ublox_base.valid = true;
    flash_base_data.write(ublox_base);
  //============================================  
}

void print_flash_memory(){
    base_data ublox_base;
    ublox_base = flash_base_data.read();
    int c=0;
    
    Serial.println("Flash memory: ");
    for (int i = 0; i < ublox_base.len; i++) {
      if (ublox_base.rtcm[i]==211 && i!=0){
              Serial.println();
              c=0;
            }
    Serial.print(ublox_base.rtcm[i]);
    Serial.print(" ");
    c++;
    if (c % 16 == 0) {
      Serial.println();
      c = 0;
    }
  }
}

void print_buf_total(int len){
  int c = 0;
  Serial.println("buf total:");
 
  for (int i = 0; i < len; i++) {
    if (buf_total[i]==211 && i!=0){
              Serial.println();
              c=0;
            }
    Serial.print(buf_total[i]);
    Serial.print(" ");
    c++;
    if (c % 16 == 0) {
      Serial.println();
      c = 0;
    }
  }
}
