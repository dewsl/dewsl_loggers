//#include <avr/io.h>
//#include <avr/interrupt.h>
//#include <stdint.h>
//#include "LowPower.h"
//#include <avr/sleep.h>

#define IRpin_PIN      PIND
#define IRpin          2
#define RESOLUTION 20 

const byte LED = 13;
volatile unsigned int trig;
volatile boolean triggered;
volatile unsigned long iRticks;
volatile unsigned int count = 0;      //counter

volatile bool counting;
unsigned long startTime;
const unsigned long INTERVAL = 2000;  // 2 second

uint8_t toggle1[]  = {0x00,0x20,0x10,0x20,0x10,0x10,0x20,0x10,0x20,0x00};   //toggle high low pin 4 and 5
//uint8_t toggle2[]  = {0x00,0x20,0x10,0x20,0x10,0x20,0x10,0x20,0x10,0x00};

ISR (TIMER1_COMPA_vect){
  TCNT1 = 0;                //reset timer 
  PORTD = toggle1[count];   //toggle pin 4 and 5
  count++;                  //increment count
  if (count > 10) {         //if counter counts >10 stops the output
    TCCR1B = 0;             //stop pulse and reset timer compare value to zero
    count=0;                //reset count to zero
  }
}

void setup(void) {
  DDRD = DDRD | B00110000;               //set pin 4 and 5 as output
  PORTD = B00000000;                     //set pin 4 and 5 LOW
  PIND = B00001100;                      //set pin 2 and 3 input
        
  TCCR1B = 0;                             //set up timer 1 with prescaler = 1 and CTC mode
  //TCCR1B |= (1 << WGM12)|(1 << CS10);   //set BIT to start pulses
  TCNT1 = 0;                              //initialize counter
  OCR1A = 0;                              //initialize compare value
  OCR1A = 175;                            //set 40kHz frequency out DEFAULT: 175 for 40kHz
  TIMSK1 |= (1 << OCIE1A);                //set interrupt bit
  attachInterrupt(digitalPinToInterrupt(2), eventISR, FALLING);  //iterrupt from High to Low
  sei();                                  //enable interrupt
}

void eventISR(){
  if(! (IRpin_PIN & _BV(IRpin))){
     PORTB ^= 0x20;                        //toggle pin 13 high/low
     TCCR1B |= (1 << WGM12)|(1 << CS10);   //set BIT to start pulses 
     noInterrupts();
     EIFR = bit (INTF0);                   //clear flag interrupt 0
     interrupts();
  }
}

/*
void wakeUp()
{
    // Just a handler for the pin interrupt.
}
*/

void loop(){
  
/*
  PORTB ^= 0x20;                        //toggle pin 13 high/low
  TCCR1B |= (1 << WGM12)|(1 << CS10);   //set BIT to start pulses
  delay(100);
  
  // disable ADC
  ADCSRA = 0;  
  
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();

  // Do not interrupt before we go to sleep, or the
  // ISR will detach interrupts and we won't wake.
  noInterrupts ();
  
  // will be called when pin D2 goes low  
  attachInterrupt (0, wake, FALLING);
  EIFR = bit (INTF0);  // clear flag for interrupt 0
 
  // turn off brown-out enable in software
  // BODS must be set to one and BODSE must be set to zero within four clock cycles
  MCUCR = bit (BODS) | bit (BODSE);
  // The BODS bit is automatically cleared after three clock cycles
  MCUCR = bit (BODS); 
  
  // We are guaranteed that the sleep_cpu call will be done
  // as the processor executes the next instruction after
  // interrupts are turned on.
  interrupts ();  // one cycle
  sleep_cpu ();   // one cycle
  */
}
