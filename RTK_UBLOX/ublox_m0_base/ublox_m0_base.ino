#include <SPI.h>
#include <RH_RF95.h>

//for m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3


// Change to 434.0 or other frequency, must match RX's freq!
 #define RF95_FREQ 433.0
//#define RF95_FREQ 915.0   // 900 MHz LoRa


//dat length
#define DATALEN 400       //max size of dummy length

//max retry for getting rtcm data
#define MAX_RETRY 300
// Blinky on receipt
#define LED 14

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

//initialize global variables
uint8_t payload[RH_RF95_MAX_MESSAGE_LEN];
uint8_t *rtcm = (uint8_t *)malloc(sizeof(uint8_t) * 1000);
uint8_t *received = (uint8_t *)malloc(sizeof(uint8_t) * 400);

int tx_RSSI = 0; //tx rssi

int max_delay = 3000;

void setup() {
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
	init_lora();
  Serial.println("UBLOX Base");
  delay(120000); //kailangan to para maginitialize si ublox

}

void loop() {
  int rtcm_len;
  char read_char;
  uint8_t final_string[] = "final"; 
  
    //kukunin yung rtcm data ng ilang uulit
   for (int i=0 ; i< MAX_RETRY; i++){
      rtcm_len = ublox_get_rtcm();
      Serial.print("rtcm length: ");
      Serial.println(rtcm_len);
    
      send_rtcm_to_rover(rtcm_len);
      delay(100);
   }

  //send final string
  rf95.send(final_string, 5);
  rf95.waitPacketSent();
  
  //get final data
  read_lora_data();
  Serial.println((char*)received);

	delay(1000);
  
}


void send_rtcm_to_rover(int rtcm_len){
  int c = 0;
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t end_string[] = "end"; 
  
  for (int j=0; j<rtcm_len; j++){
    if (c==RH_RF95_MAX_MESSAGE_LEN){
      rf95.send(buf, c);
      rf95.waitPacketSent();
      //delay(50);
      c=0;
    }
    buf[c] = rtcm[j];
    c++;
    
  }
  rf95.send(buf, c);
  rf95.waitPacketSent();
  //delay(50);
  rf95.send(end_string, 3);
  rf95.waitPacketSent();
}
