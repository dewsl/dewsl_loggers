#define ATCMD 		"AT"
#define ATECMDTRUE	"ATE"
#define ATECMDFALSE	"ATE0"
#define SENDIRP		"AT+IRP"	// send IR pulse
#define OKSTR 		"OK"
#define ERRORSTR	"ERROR"
#define SAMPLEULTRASONIC	"AT+UTS"
#define TURNONPOWER "AT+PWR"
#define SETGAIN "AT+GAIN"
#define SETRDAC1 "AT+RDAC1"
#define SETRDAC2 "AT+RDAC2"
#define SENDPULSE "AT+PULSE"
#define READTMP "AT+TMP"
#define Addr 44

String gainValue,gainValue1,gainValue2;
bool ate=false;

void getArguments(String at_cmd, String *arguments){
	int i_from = 0, i_to = 0, i_arg = 0;
	bool f_exit = true;
	String sub;

	// find '=' sign
	i_from = at_cmd.indexOf('=');

	do{
		i_to = at_cmd.indexOf(',',i_from+1);
		if (i_to < 0){
			sub = at_cmd.substring(i_from+1);
			f_exit = false;
		}
		else sub = at_cmd.substring(i_from+1,i_to);

		arguments[i_arg] = sub;
		i_from = i_to;
		i_arg += 1;

	} while(f_exit);

}
void getATCommand(){
	String serial_line, command;
	int i_equals = 0;
	
	do{
		serial_line = Serial1.readStringUntil('\r\n');
	}while(serial_line == "");
	serial_line.toUpperCase();
	serial_line.replace("\r","");

	// echo command if ate is set, default true
	if (ate) Serial1.println(serial_line);

	// get characters before '='
	i_equals = serial_line.indexOf('=');
	if (i_equals == -1) command = serial_line;
	else command = serial_line.substring(0,i_equals);

	// Serial.println(command);
	
	if (command == ATCMD)
		Serial1.println(OKSTR);
	else if (command == ATECMDTRUE){
		ate = true;
		Serial1.println(OKSTR);
	}
	else if (command == ATECMDFALSE){
		ate = false;
		Serial1.println(OKSTR);
	}
	else if (command == SAMPLEULTRASONIC){
		
		getUltrasonicWave(0);
	}
	else if (command == SENDIRP){
		sendIRPulse();
    Serial1.println(OKSTR);
	}
	else if (command == TURNONPOWER){
		turnOnRelay();
		Serial1.println("TURNONPOWER relay!");
	}
  else if (command == SENDPULSE){
    turnOn_IRPIN();
    Serial1.println("Turn on IR PIN!");
  } 
  else if (command == READTMP){
    read_tmp();
    //Serial.println("Read Temperature!");
  }   
  else if (command == SETGAIN){
    Serial.println("Setting gain for AD8220!");
    gainValue = serial_line.substring(i_equals+1);
    Serial.print("Command: ");
    Serial.print(gainValue);
    Serial.println("");
    //set_digiPOT(gainValue.toInt());
    //set_rdac1(gainValue.toInt());
    //delay(50);
    set_rdac2(gainValue.toInt());
    //delay(50);
  }
  else if (command == SETRDAC1){
    Serial1.println("Setting Resistance of RDAC1 - potA");
    gainValue1 = serial_line.substring(i_equals+1);
    Serial1.print("Command: ");
    Serial1.print(gainValue1);
    Serial1.println("");
    set_rdac1(gainValue1.toInt());
  }
  else if (command == SETRDAC2){
    Serial1.println("Setting Resistance of RDAC2 - potB");
    gainValue2 = serial_line.substring(i_equals+1);
    Serial1.print("Command: ");
    Serial1.print(gainValue2);
    Serial1.println("");
    set_rdac2(gainValue2.toInt());
  }  
	else{
		Serial.println(ERRORSTR);
}
}

void set_rdac1(int rdac1){            //rdac1 - priority POT
  Wire.beginTransmission(Addr);       //Start I2C connection with slave byte
  Wire.write(0);                      //Send instruction byte
  Wire.write(rdac1);                  //Input resistence value
  Wire.endTransmission();            //Stop i2c connection
  delay(30);
  unsigned int data;
  Wire.requestFrom(Addr, 1);
  if (Wire.available() == 1){
    data = Wire.read();
  }
  float resistance = (((data / 256.0 ) * 100000.0) + 60.0) / 1000.0; 
  float g_resistance = resistance;
  float gain = (1+(49.4/g_resistance));
  Serial1.print("Data: ");             Serial1.print(data);
  Serial1.print("\t Resistance: ");    Serial1.print(resistance);   Serial1.print(" K\t"); 
  Serial1.print("Gain: ");             Serial1.print(gain);         Serial1.println("\n");
}

void set_rdac2(int rdac2){            //default for rdac2 = 1 = 450 ohms-SERIES with rdac1
  Wire.beginTransmission(Addr);       //Start I2C connection with slave byte
  Wire.write(128);                      //Send instruction byte
  Wire.write(rdac2);                  //Input resistence value
  Wire.endTransmission();            //Stop i2c connection
  delay(30);
  
  unsigned int data_rdac2;
  Wire.requestFrom(Addr, 1);
  if (Wire.available() == 1){
    data_rdac2 = Wire.read();
  }
  float resistance = (((data_rdac2 / 256.0 ) * 100000.0) + 60.0) / 1000.0; 
  float g_resistance = resistance;
  float gain = (1+(49.4/g_resistance));
  Serial1.print("Data: ");             Serial1.print(data_rdac2);
  Serial1.print("\t Resistance: ");    Serial1.print(resistance);   Serial1.print(" K\t"); 
  Serial1.print("Gain: ");             Serial1.print(gain);         Serial1.println("\n");
}

void readResistance_rdac1(){
  unsigned int data;
  Wire.requestFrom(Addr, 1);
  if (Wire.available() == 1){
    data = Wire.read();
  }
  float resistance = (((data / 256.0 ) * 100000.0) + 60.0) / 1000.0; 
  float g_resistance = resistance;
  float gain = (1+(49.4/g_resistance));
  Serial.print("Data: ");             Serial.print(data);
  Serial.print("\t Resistance: ");    Serial.print(resistance);   Serial.print(" K\t"); 
  Serial.print("Gain: ");             Serial.print(gain);         Serial.println("\n");
}

void set_digiPOT(int value){
  Serial1.println("Sent command to POT!\t");
  Wire.beginTransmission(Addr);       //Start I2C connection with slave byte
  Wire.write(0);                      //Send instruction byte, Default: 0
  Wire.write(value);                  //Input resistence value
  Wire.endTransmission();            //Stop i2c connection
  // Serial.println("Setting AD5341");
  delay(300);

  unsigned int data;
    // Request 1 byte of data
    Wire.requestFrom(Addr, 1);
  
    // Read 1 byte of data
    if (Wire.available() == 1){
      data = Wire.read();
  }

    // Convert the data
    float resistance = (((data / 256.0 ) * 100000.0) + 60.0) / 1000.0;    //1000
    float g_resistance = resistance;
    float gain = (1+(49.4/g_resistance));
    
    // Output data to serial monitor
    Serial.print("Data: ");
    Serial.print(data);
    Serial.print("\t Resistance: ");
    Serial.print(resistance);
    Serial.print(" K\t"); 
    Serial.print("Gain: ");
    Serial.print(gain);
    Serial.println("\n");
}



