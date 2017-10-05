#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

const byte LED = 13;
const byte SWR = 12;
const byte BUTTON = 2;
volatile boolean RELAY;
volatile unsigned int count = 0;
int y;

uint8_t toggle4[]  = {0x00,0x20,0x10,0x20,0x10,0x10,0x20,0x10,0x20,0x00};

void setup() {
    pinMode(SWR, OUTPUT);
    DDRD &= ~(1 << DDD2);   //Set pin 2 as input
    DDRD |= (1 << 4);       //Set pin 4 as output 
    DDRD |= (1 << 5);       //Set pin 5 as output  
    PORTB |= 0b00110000;    //set pin 12 and 13 as output
    PORTB &= ~0x30;       // set pin 12 and 13 LOW
        
   TCCR1B = 0;                     // set up timer with prescaler = 1 and CTC mode
   //TCCR1B |= (1 << WGM12)|(1 << CS10);   //11
   TCNT1 = 0;                    // initialize counter
   OCR1A = 0;                    // initialize compare value
   OCR1A = 175;       //200
   TIMSK1 |= (1 << OCIE1A);     //set interrupt bit
   attachInterrupt(digitalPinToInterrupt(BUTTON), PING, FALLING);
   sei();    
}

uint32_t x = 0;

void PING(){
  x=0;
  while(x<1000){
    x++;
  }

  if (digitalRead(BUTTON) == 0x00) {
    RELAY = true;    // relay pin
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
  //PORTD ^= 0x30;  
  PORTD = toggle4[count];   //toggle pin 4 and 5
  count++;
    if (count > 10) {
    TCCR1B = 0;
    count=0;
    //noInterrupts();
    }
  
}

void loop() {
  if(RELAY){
    digitalWrite(SWR, HIGH);
    delay(5000);
    digitalWrite(SWR, LOW);
    delay(100);
    // PORTB |= 0X10;    //set relay pin high
    // y=0;
    // while(y<5000){
    //   y++;
    // }
    // PORTB &= ~0X10;    //set relay pin low
    RELAY = false;
  }
}
