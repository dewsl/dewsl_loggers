#define TWAITPIN 25
#define IRLEDPIN 26
#define IRPULSEMAX 10
#define FREQ 13          //13~38.46kHz

bool data_filled = 0;

static uint16_t sig[N_BUFFER];
uint8_t edge = 0;

uint32_t t_wait = 100;  // t_wait value in millis
uint8_t ir_pulse_counter = 0;
bool verbose_out = true;

//Function: Pin 33 output.
//Set Pin 33 as output.
void init_IRPIN(){
  pinMode(33,OUTPUT);
  REG_PIOC_OWER = 0x00000002;
  REG_PIOC_OWDR = 0xfffffffd;
}

//Function: Pin 33 output.
//Turns on pin 33 ~38kHz frequency
void turnOn_IRPIN(){
  for(int i=0; i<10; i++){     //default:10 pulses
    REG_PIOC_ODSR = 0x00000002;
    delayMicroseconds(FREQ);
    REG_PIOC_ODSR = 0x00000000;
    delayMicroseconds(FREQ);
  }

}
void stopTimer3(){
  REG_PIOB_ODSR ^= (1<<TWAITPIN);
  edge += 1;
  if (edge>1){
    edge = 0;
    Timer3.stop();
  }
}

void isrTimer2(){
  REG_PIOB_ODSR ^= (1<<IRLEDPIN);
  ir_pulse_counter += 1;
  if (ir_pulse_counter == IRPULSEMAX){
    ir_pulse_counter = 0;
    Timer2.stop();
  }
}

void startWaitTimer(){  
	// delay(1000);      
	Timer3.start(100); // Calls every 50ms
}

void sampleSignal(uint16_t ch){
  initADC(sig);
  adc_start(ADC);
  delay(10);  
  adc_stop(ADC);
}

void sendIRPulse(){
  // pinMode(22, OUTPUT);
  REG_PIOB_PER = (1<<IRLEDPIN);     //gpio
  REG_PIOB_OER = (1<<IRLEDPIN);     //output
  REG_PIOB_ODSR &= ~(1<<IRLEDPIN);   //SODR lights the led
  ir_pulse_counter = 0;
  Timer2.attachInterrupt(isrTimer2);
  Timer2.setFrequency(76000);
  Timer2.start();
  // while(1);
  Serial.println("IR Pulse");
}

void turnOnRelay(){
  digitalWrite(3, HIGH);
  delay(100);
  digitalWrite(3, LOW);  
}

void getUltrasonicWave(uint16_t channel){
  //Sends IR pulses for trigger.
  turnOn_IRPIN();
  
  // setup gpios and registers
  Timer3.attachInterrupt(stopTimer3);  
                                                           
  /*
  send IR pulse here
  sendIRPulse();
  while (ir_pulse_counter<IRPULSEMAX);
  */

  // start t_wait timer
  Timer3.start(t_wait);
  
  //Serial.write("before sample");
  sampleSignal(channel);
  //Serial.write("helloTin");
  //Serial.write(channel);

  int sizesignal = sizeof(sig);
  int i;
  //Serial.write(sizesignal);
  i=0;
  while(i<N_BUFFER){
//    Serial.println(sig[i]);       //Serial1 dati
    //Serial.print(",");

    Serial1.write((sig[i]>>8)&0xFF); //High Byte
    Serial1.write(sig[i] & 0xFF);    //Low byte
    i=i+1;
  }
//  int done=0;
//  Serial.write(done & 0xFF);    //
}

