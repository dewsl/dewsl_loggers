#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

const byte LED = 13;
const byte relayPin = 12;
const byte INTP = 2;
const byte INTR = 3;
volatile boolean RELAY;
volatile unsigned int count = 0;
int y;

//uint8_t toggle1[]  = {0x00,0x20,0x10,0x20,0x10,0x10,0x20,0x10,0x20,0x00};
uint8_t toggle2[]  = {0x00,0x20,0x10,0x20,0x10,0x20,0x10,0x20,0x10,0x00};     //0x10-4, 0x20-5

void setup() {
    pinMode(relayPin, OUTPUT);
    //DDRD &= ~(1 << DDD2);   //Set pin 2 as input
    DDRD |= (1 << 4);       //Set pin 4 as output 
    DDRD |= (1 << 5);       //Set pin 5 as output  

    PORTB |= 0b00110000;    //set pin 12 and 13 as output
    PORTB &= ~0x30;       // set pin 12 and 13 LOW 
    PORTD &= ~0x00;       // set pin 4 and 5 LOW

    TCCR1B = 0;                     // set up timer with prescaler = 1 and CTC mode
   //TCCR1B |= (1 << WGM12)|(1 << CS10);   //11
    TCNT1 = 0;                    // initialize counter
    OCR1A = 0;                    // initialize compare value
    OCR1A = 175;       //200
    TIMSK1 |= (1 << OCIE1A);     //set interrupt bit

   attachInterrupt(digitalPinToInterrupt(INTP), sendPulse, FALLING);
   attachInterrupt(digitalPinToInterrupt(INTR), toggleRelayPin, FALLING);
   sei();    
}


void toggleRelayPin(){
  if(digitalRead(INTR) == 0x00){
    RELAY = true;
  }
}

uint32_t x = 0;

void sendPulse(){
  x=0;
  while(x<1000){
    x++;
  }

  if (digitalRead(INTP) == 0x00) {
    PORTB ^= 0x20;
    TCCR1B |= (1 << WGM12)|(1 << CS10);
  }
//  for(x=0; x<10000; x++){
//    __asm__("nop\n\t"); 
//  }
//  PORTB &= ~0x20;
}

ISR (TIMER1_COMPA_vect){
  TCNT1 = 0;
  PORTD &= ~0x00;   // dagdag makes pin LOW
  PORTD = toggle2[count];   //toggle pin 4 and 5
  count++;
  if (count > 10) {
    PORTD &= ~0x00;   //dagdag makes pin LOW
    TCCR1B = 0;
    count=0;
    //noInterrupts();
  }
  
}

void loop() {

  if(RELAY){
    digitalWrite(relayPin, HIGH);
    delay(5000);
    digitalWrite(relayPin, LOW);
    delay(100);
    RELAY = false;
  }

}
