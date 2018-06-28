#define ATCMD     "AT"
#define ATECMDTRUE  "ATE"
#define ATECMDFALSE "ATE0"
#define OKSTR     "OK"
#define ERRORSTR  "ERROR"


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
    }
    else sub = at_cmd.substring(i_from+1,i_to);

    arguments[i_arg] = sub;
    i_from = i_to;
    i_arg += 1;

  } while(f_exit);

}


void getAtcommand(){

	String serial_line, command;
	int i_equals = 0;
	  
	do{
	   serial_line = Serial.readStringUntil('\r\n');
	  }while(serial_line == "");
	  serial_line.toUpperCase();
	  serial_line.replace("\r","");

	  // echo command if ate is set, default true
	  if (ate) Serial.println(serial_line);

	  // get characters before '='
	  i_equals = serial_line.indexOf('=');
	  if (i_equals == -1) command = serial_line;
	  else command = serial_line.substring(0,i_equals);

	  // Serial.println(command);
	  
	  if (command == ATCMD)
	    Serial.println(OKSTR);
	  else if (command == ATECMDTRUE){
	    ate = true;
	    Serial.println(OKSTR);
	  }
	  else if (command == ATECMDFALSE){
	    ate = false;
	    Serial.println(OKSTR);
	  }
	  else if (command == "AT+SETTS"){
	  	setupTime();
	  }    
	  else if (command == "AT+READTS"){
	  	readTimeStamp();
	  }  
	  else if (command == "AT+READTEMP"){
	  	// Serial.print("RTC Temperature: ");
	  	readTemp();
	  }    
	  else if (command == "AT+VOLTAGE"){
	  	read_voltage();
	  }  
	  else if (command == "AT+CURRENT"){
	  	read_current();
	  }    
	  else if (command == "AT+ONPWR"){
	  	turnOn_pwr();
	  	flashLed(LEDPIN, 5, 100);
	  }  
	  else if (command == "AT+OFFPWR"){
	  	turnOff_pwr();
	  	flashLed(LEDPIN, 5, 100);
	  }  
	  else if (command == "AT+LDR"){
	  	init_lidar();
	  	lidar();
	  }
	  else if (command == "AT+TEXT"){
	  	turnOn_pwr();
	  	delay(500);
	  	readTimeStamp();
	  	// readTemp();
	  	init_lidar();
	  	lidar();
	  	read_voltage();
	  	read_current();
	  	init_9dof();
	  	read_9dof();
	  	build_message();
	  	delay(1000);
	  	logData();
	  	// sendMessage();
	  	send_thru_xbee(streamBuffer);
	  	delay(2000);
	  	turnOff_pwr();
	  }       
	  else if (command == "AT+SENDTEXT"){
	  	turnOn_pwr();
	  	delay(2000);	  	
	  	// sendMessage();
	  	delay(1000);
	  	turnOff_pwr();
	  }     
	  else if (command == "AT+9DOF"){
	  	init_9dof();
	  	read_9dof();
	  }	  
	  else if (command == "AT+SD"){
	  	logData();
	  }		  
	  else{
	    Serial.println(ERRORSTR);
		}
}

String getDateTime(){
	String dateTimestr;
	int timestamp[12];
	DateTime dt(rtc.makeDateTime(rtc.now().getEpoch()));	//create object for current date-time
	dt.addToString(dateTimestr);	//convert to string
	// return dateTimestr;
	dateTimestr.remove(0,2);
	Serial.println(dateTimestr);
}

void setupTime() {
	int MM = 0, DD = 0, YY = 0, hh = 0, mm = 0, ss = 0, dd = 0;
	Serial.println(F("\nSet time and date in this format: YY,MM,DD,hh,mm,ss,dd[0-6]Mon-Sun"));
	delay (5000);
	//2018,06,27,19,54,50,2
	while (!Serial.available()) {}
	if (Serial.available()) {
		YY = Serial.parseInt();
		MM  = Serial.parseInt();
		DD = Serial.parseInt();
		hh = Serial.parseInt();
		mm = Serial.parseInt();
		ss = Serial.parseInt();
		dd = Serial.parseInt();
	}
  
	adjustDate(YY, MM, DD, hh, mm, ss, dd);
	Serial.print(F("Time now is: "));
	Serial.println(Ctimestamp);
}

void adjustDate(int year, int month, int date, int hour, int min, int sec, int weekday){
	DateTime dt(year, month, date, hour, min, sec, weekday);
	rtc.setDateTime(dt);	// adjust date-time as defined by 'dt'
	// Serial.println(rtc.now().getEpoch());	//debug info
}

void readTemp(){
	float temp;
	rtc.convertTemperature();
	temp = rtc.getTemperature();
	// Serial.print(rtc.getTemperature());
	dtostrf(temp, 5, 2, l_temp);
	Serial.print("RTC Temperature: ");
	Serial.println(l_temp);
}

void readTimeStamp(){
	// char timestamp[12];

	DateTime now = rtc.now(); //get the current date-time

	String ts = String(now.year());
	
	if (now.month() < 10){
		ts += "0" + String(now.month());
	}else{
		ts += String(now.month());
	}

	if (now.date() < 10){
		ts += "0" + String(now.date());
	}else{
		ts += String(now.date());
	}

	if (now.hour() < 10){
		ts += "0" + String(now.hour());
	}else{
		ts += String(now.hour());
	}

	if (now.minute() < 10){
		ts += "0" + String(now.minute());
	}else{
		ts += String(now.minute());
	}

	if (now.second() < 10){
		ts += "0" + String(now.second());
	}else{
		ts += String(now.second());
	}
	ts.remove(0,2);	//remove 1st 2 data in ts
	// return(ts);	
	// Serial.println(ts);
	ts.toCharArray(Ctimestamp, 13);
	// Serial.println(Ctimestamp);

	// for(int i=0; i<12; i++){
 //    	Ctimestamp[i] = timestamp[i];
 //  	}
 //  	Ctimestamp[12] = '\0';

  	if (DEBUG == 1) {Serial.print("Timestamp: ");}
  	if (DEBUG == 1) {Serial.println(Ctimestamp);}

	// return Ctimestamp;
}

//Function: read_current()
// Reads the current draw from the onboard ina219.
void read_current(){
	
  float current_mA = 0;
  current_mA = ina219.getCurrent_mA();
  // Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");

  dtostrf(current_mA, 5, 2, l_current_mA);
  if (DEBUG == 1) {Serial.print("Current Draw: ");}
  if (DEBUG == 1) {Serial.println(l_current_mA);}
}

/* 
  Function: read_voltage()

    Reads the *Bus Voltage*, *Shunt Voltage*, *Load Voltage* from the onboard ina219.
*/
void read_voltage(){

  float shuntvoltage = 0;
  float busvoltage = 0;
  float loadvoltage = 0;

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  // Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  // Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  // Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");

  dtostrf(loadvoltage, 3, 2, l_loadvoltage);
  if (DEBUG == 1) {Serial.print("Load Voltage: ");}
  if (DEBUG == 1) {Serial.println(l_loadvoltage);}  
}