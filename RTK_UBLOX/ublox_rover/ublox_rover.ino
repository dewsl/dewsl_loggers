#include <Arduino.h>   // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function
#include <SPI.h>
#include <RH_RF95.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <LowPower.h>

#include <EnableInterrupt.h>
#include <FlashStorage.h>
#include "Sodaq_DS3231.h"
#include <Adafruit_SleepyDog.h>
SFE_UBLOX_GNSS myGNSS;

#define BUFLEN (5*RH_RF95_MAX_MESSAGE_LEN) //max size of data burst we can handle - (5 full RF buffers) - just arbitrarily large
#define RFWAITTIME 500 //maximum milliseconds to wait for next LoRa packet - used to be 600 - may have been too long

char dataToSend[200];
#define DATA_TO_AVERAGE 5

char Ctimestamp[13] = "";
uint16_t store_rtc = 00; //store rtc alarm
#define DEBUG 1
#define RTCINTPIN 6

// for feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RF95_FREQ 433.0
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// We will use Serial2 - Rx on pin 11, Tx on pin 10
Uart Serial2 (&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

// LED is turned on at 1st LoRa reception and off when nothing else received. It gives an indication of how long the incoming data stream is.
#define LED 13
unsigned long start;
void setup() {
    Serial.begin(115200);
    Serial2.begin(115200);

    pinPeripheral(10, PIO_SERCOM);
    pinPeripheral(11, PIO_SERCOM);
    delay(100);
    
    rtc.begin();
    /* rtc interrupt */
    // attachInterrupt(RTCINTPIN, wake, FALLING);
    // init_Sleep(); //initialize MCU sleep state
    // // setAlarmEvery30(0); //rtc alarm settings 0 [00 & 30] 1 [05 & 35]
    // setAlarm2();


    pinMode(LED, OUTPUT);
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_RST, HIGH);

    digitalWrite(RFM95_RST, LOW);
    delay_millis(10);
    digitalWrite(RFM95_RST, HIGH);
    delay_millis(10);

  // while (!Serial) { //Waits for the Serial Monitor
  //   delay(1);
  // }
    // Serial.begin(115200);
    // Serial2.begin(115200);

    // Assign pins 10 & 11 SERCOM functionality


    Serial.println("Feather LoRa RX");
    digitalWrite(RFM95_RST, LOW);
    delay(10);
    digitalWrite(RFM95_RST, HIGH);
    delay(10);

    while (!rf95.init()) {
        Serial.println("LoRa radio init failed");
        while (1);
    }
    Serial.println("LoRa radio init OK!");
    if (!rf95.setFrequency(RF95_FREQ)) {
        Serial.println("setFrequency failed");
        while (1);
    }
    Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

    rf95.setTxPower(23, false);

    init_ublox();
    readTimeStamp();
    Serial.print("Current timestamp: ");
    Serial.println(Ctimestamp);
    start = millis(); 

}

void loop() {
    if (samplingTime() == 1){
        do {
            get_rtcm();
        } while ((RTK() != 2) && ((millis() - start) < 300000));
    
        read_ublox_data();
        arrange_data(dataToSend);
        send_thru_lora(dataToSend);

    } else {
        delay(1000);
    }
}
