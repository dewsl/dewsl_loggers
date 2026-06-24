// extern "C" {
//   #include "esp_adc/adc_oneshot.h"
//   #include "esp_adc/adc_cali.h"
// #include "esp_adc/adc_cali_scheme.h"
// }

            

// adc_oneshot_unit_handle_t adc_handle = NULL;           // ADC unit handle
// adc_cali_handle_t cali_handle = NULL;           // Calibration handle
// bool cali_enabled = false;                      // Flag to track calibration status

// void ADCInit() {
//   adc_oneshot_unit_init_cfg_t unit_cfg = {      //  Initialize ADC unit (ADC1)
//     .unit_id = ADC_UNIT_1,
//     .clk_src = ADC_RTC_CLK_SRC_DEFAULT,         // use default clock, 0 doesn't work
//     .ulp_mode = ADC_ULP_MODE_RISCV              //  Use this to retain ULP functions
//   };
//   ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

//   adc_oneshot_chan_cfg_t chan_cfg = {           // Configure ADC channel (GPIO39 = ADC1_CHANNEL_3)
//     .atten = ADC_ATTEN_DB_12,
//     .bitwidth = ADC_BITWIDTH_DEFAULT
//   };                                            
//   ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_3, &chan_cfg));

//   adc_cali_line_fitting_config_t cali_cfg = {   // Try to initialize line fitting calibration
//     .unit_id = ADC_UNIT_1,
//     .atten = ADC_ATTEN_DB_12,
//     .bitwidth = ADC_BITWIDTH_DEFAULT,
//     .default_vref = 1100                        // Use default vRef if eFuse Vref is not available
//   };                                            

//   if (adc_cali_create_scheme_line_fitting(&cali_cfg, &cali_handle) == ESP_OK) {
//     cali_enabled = true;
//     debugPrintln("ADC calibration enabled (line fitting)");
//   } else {
//     debugPrintln("ADC calibration not available");
//   }


// }

// void readADC() {
//   int ADCraw = 0;
//   int voltage = 0;

//   esp_err_t result = adc_oneshot_read(adc_handle, ADC_CHANNEL_3, &ADCraw); // Read raw ADC value
//   if (result == ESP_OK) {
//     debugPrint("Raw ADC: ");
//     debugPrintln(ADCraw);
//     if (cali_enabled && adc_cali_raw_to_voltage(cali_handle, ADCraw, &voltage) == ESP_OK) {
//       debugPrint("\t\t Calibrated raw: ");
//       debugPrint(voltage);    //  Convert to calibrated voltage if calibration is available
//       debugPrint("\t\t Computed voltage: ");
//       debugPrint(inputVoltageEst(voltage/1000));
//       debugPrint("V");
//     }

//     debugPrintln("");
//   } else {
//     debugPrintln("ADC read failed");
//   }


// }

// #define ADC_CHANNEL     ADC1_CHANNEL_3   // GPIO39
// #define ADC_ATTEN       ADC_ATTEN_DB_11  // ~0–3.3V range; pero ang max usable\reliable nito ay 2450mv sabi sa docs ng espressif
// #define ADC_WIDTH       ADC_WIDTH_BIT_12 // 12-bit resolution
// #define DEFAULT_VREF    1100             // mV (used if eFuse not burned)
// #define VMON 39

// float opAmpSlope = 0.21;
// float opAmpOffset = -0.7;

// esp_adc_cal_characteristics_t adc_chars;

// float getOpAmpOffset(float vInput1, float vInput2, float vOut1, float vOut2) {
//   opAmpSlope = (vOut2 - vOut1)/(vInput2 - vInput1);
//   // vOut1 = opAmpSlope * vInput1 + b;
//   return vOut1 - (opAmpSlope * vInput1);  
// }

// float inputVoltageEst(float ADCvoltage) {
//   const int ADC_RES = 4095; // 12-bit resolution
//   const float Vref = 3.3; // Reference voltage
//   float computedVoltage = (ADCvoltage * Vref) / ADC_RES;
//   // float computedVoltage = (ADCvoltage - opAmpOffset)/opAmpSlope;
//   return computedVoltage;
// }

// void opAmpCalib () {
//   // set Voltage supply to 14v
//   // check output of opAmp at 14v
//   // set Voltage supply to 11v
//   // check output of opAmp at 14v
// }

// void ADCInit() {
//   adc1_config_width(ADC_WIDTH);
//   adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
//   esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, DEFAULT_VREF, &adc_chars); // use unit 1 for 32 and 39

// }
// void readVMonADC() {
//   uint32_t raw = adc1_get_raw(ADC_CHANNEL);                         // Read raw ADC value
//   uint32_t voltage = esp_adc_cal_raw_to_voltage(raw, &adc_chars);   // Convert raw value to calibrated voltage (in mV)

//   debugPrint("Raw ADC: ");
//   debugPrint(raw);
//   debugPrint(" | Calibrated Voltage: ");
//   debugPrint(voltage);
//   debugPrintln(" mV");

  // float vMonDigital = analogRead(VMON);
  // float vMonAnalog = (vMonDigital*3.3)/4095;
  // return vMonAnalog;
// }

// void refineVoltageReading(float opAmpOutput) {
//   //  R1 default value: 10MΩ - Top resistor of the voltage divider
//   //  R2 default value 100KΩ - Bottom resistor of the voltage divider
//   //  Rf default value 200KΩ - Feedback resistor of the op-amp
//   //  Rin default value 10KΩ - Resistor from op-amp inverting input to ground
//   float R1, R2, Rf, Rin;
//   float computedVoltage = opAmpOutput*((R1+R2)/(R2*(1+(Rf/Rin))));
//   debugPrint("Input R1 value:");
//   debugPrint("Input R2 value:");
//   debugPrint("Input Rf value:");
//   debugPrint("Input Rin value:");

//   debugPrint("Check OpAmp output ");
//   
// }

void initINA219() {
  // Wire.begin();
  // delayMillis(200);
  if (!INA219Module.begin()) Serial.println("could not connect to INA219 module");
  else {
    Serial.println("INA219 module ready");
    INA219Module.setMaxCurrentShunt(IMAX, RSHUNT);
  }
}


void readINA219() {     
  
  Serial.print("CALI:\t");
  Serial.println(INA219Module.isCalibrated());
  Serial.print("CLSB:\t");
  Serial.println(INA219Module.getCurrentLSB());
  Serial.print("SHUNT:\t");
  Serial.println(INA219Module.getShunt(), 4);
  Serial.print("MAXC:\t");
  Serial.println(INA219Module.getMaxCurrent(), 4);

  INA219Module.setMaxCurrentShunt(IMAX, RSHUNT);    
  delayMillis(500);

  Serial.print("CALI:\t");
  Serial.println(INA219Module.isCalibrated());
  Serial.print("CLSB:\t");
  Serial.println(INA219Module.getCurrentLSB());
  Serial.print("SHUNT:\t");
  Serial.println(INA219Module.getShunt(), 4);
  Serial.print("MAXC:\t");
  Serial.println(INA219Module.getMaxCurrent(), 4);      

  float Vsupply = INA219Module.getBusVoltage();
  float Vshunt = INA219Module.getShuntVoltage_mV();
  float Isupply = (Vshunt*1000)/RSHUNT;                           //  INA219Module.getCurrent_mA() output is a bit *different*.. or maybe I'm not using it right? ..so, Ohm's Law instead
                                                                  //  computed value is typically around ±5% of manual measurement
                                                                  //  not the Russel and Graham duo...

  Serial.print(Vsupply);
  Serial.print("V \t");
  Serial.print(Vshunt);
  Serial.print("mV \t");
  Serial.print(Isupply);
  Serial.print("mA \t");
  // For now, the getCurrent() function doesn't return any value... Not sure why maybe I don't know how to read
  // But since we can get the shunt voltage, lets just divide it by the shunt resistor 
  Serial.print(INA219Module.getCurrent_mA());  
  Serial.println();
  // delayMillis(10);
}


float readINA219VoltageCurrent (byte paramIndex) {                  //  Voltage is separated from current because these are times these are needed separately
  bool startTrigState = false;                                      //  assumes trigger is initially disabled (false)
  float paramHolder = 0.00;                                         //  This is used for both voltage and current
  //  INA219 is now continously powered with current drawe <1mA
  // if (gpio_get_level(GPIO_NUM_26) == 0) {                           //  continues here is trigger is disabled
  //   digitalWrite(AUX_TRIG, HIGH);                                   //  flip state to enable power for INA219 board; replace this is pin will be reconfigured
  //   delayMillis(1000);                                              //  introduce some delay for a stable reading
  //   debugSysln("AUX POWER ENABLED");          
  // } else {       
  //   debugSysln("AUX POWER ACTIVE");                                 //  if trigger is currently active
  //   startTrigState = true;                                          //  update trigger state
  // }
  // INA219Module.setMaxCurrentShunt(IMAX, RSHUNT);                    //  Not sure if this configuration is saved when module is powered down... so lets re-run this anyway
  if (paramIndex == 0) paramHolder = INA219Module.getBusVoltage();  //  index 0 returns voltage; 1 is current
  else paramHolder = ((INA219Module.getShuntVoltage_mV())*1000)/RSHUNT; // any index above 0 returns current value   
  
  if (startTrigState == false) digitalWrite(AUX_TRIG, LOW);;        //  return switch to initial state
  // else                                                           //  initial state was ON.. this does't really do anything        
  return paramHolder;                                               //  return
} 

void cpuFrequency(uint8_t freQ) {
    uint8_t cpuFreq = getCpuFrequencyMhz();
    if (cpuFreq != freQ) {
      setCpuFrequencyMhz(freQ);
    }
    Serial.print("CPU frequency is set to: ");
    Serial.print(cpuFreq);
    Serial.print("Mhz");
    Serial.println("\n");
}

//  This was used as a function because its cleaner, and you'll only edit instance item instead of multiple
void diagnosticCheck(uint8_t mIndex) {
  switch (mIndex) {
    case 1: _mValue[0] = readINA219VoltageCurrent(1); _mValue[1] = readINA219VoltageCurrent(0); break;
    case 2: _mValue[2] = readINA219VoltageCurrent(1); _mValue[3] = readINA219VoltageCurrent(0); break;
    case 3: _mValue[4] = readINA219VoltageCurrent(1); _mValue[5] = readINA219VoltageCurrent(0); break;
    default: debugPrintln("diagnosticCheck index overflow");
  }
}