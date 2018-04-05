void initADC(uint16_t *sig){
  adc_init(ADC, 84000000, 20000000, 1000);
  adc_configure_timing(ADC, 0, ADC_SETTLING_TIME_0, 0);
  adc_configure_trigger(ADC, ADC_TRIG_SW, 1); // Disable hardware trigger.
  adc_disable_interrupt(ADC, 0xFFFFFFFF); // Disable all ADC interrupts.
  adc_disable_all_channel(ADC);
  adc_enable_channel(ADC, ADC_CHANNEL_0);
  // adc_start(ADC);
  ADC->ADC_RPR=(uint32_t)sig;   // DMA buffer
  ADC->ADC_RCR=N_BUFFER;
  ADC->ADC_PTCR=1;
}

void read_tmp(){
  //Serial1.println("wake up MCP9808.... "); // wake up MSP9808 - power consumption ~200 mikro Ampere
  tempsensor.wake();   // wake up, ready to read!

  // Read and print out the temperature, then convert to *F
  float c = tempsensor.readTempC();
  float f = c * 9.0 / 5.0 + 32;
  //Serial1.print("Temp: "); 
  Serial1.print(c); //Serial1.print("*C\t"); 
  //Serial1.print(f); Serial1.println("*F");
  
  //Serial1.println("Shutdown MCP9808.... ");
  tempsensor.shutdown(); // shutdown MSP9808 - power consumption ~0.1 mikro Ampere
    
}

