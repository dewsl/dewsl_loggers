void getdataBroadcastNew(int mode,char readCode){
	columnData[0] =0;
	char *columnPointer = columnData;
	digitalWrite(trigSW, LOW);
	digitalWrite(trigSW, HIGH);
	delay(2000);
	commandCAN = 0;
	clear_can_array(can_default_buffer);  //clear out temp buffer
	
  	switch(readCode){
	case 'T':  {//tilt only

		if (PRINT_MODE == 1) Serial.println("  --function: getdataBroadcastNew(), case T -- called");
		Serial1.print("ARQWAIT");        
		// GET_DATA(columnData,PASS_AXEL1_ADC_CALIB_MINMAX); 
		String temp= "";
		temp= "000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914000F0200541703207914";
		temp.toCharArray(columnData, 521);
		Serial.println(columnData);
		parsedData = parser(columnData,8);
		writeData(parsedData);													//write data to sdcard
		Serial.println(parsedData);
		sprintf(allData,parsedData ,strlen(parsedData));		

		Serial1.print("ARQWAIT");                
		// GET_DATA(columnData,PASS_AXEL2_ADC_CALIB_MINMAX);
		temp= "000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914000F0210541703207914";
		temp.toCharArray(columnData, 521);
		Serial.println(columnData);
		parsedData = parser(columnData,8);
		writeData(parsedData);													//write data to sdcard
		Serial.println(parsedData);
		strncat(allData,"+" ,1);
		strncat(allData,parsedData ,strlen(parsedData));
                
        if (PIEZO == 1) {		
    		Serial1.print("ARQWAIT");        
    		GET_DATA(columnData,255); // piezo
    		Serial1.print("ARQWAIT");
    		Serial.println(columnData);
    		parsedData = parser(columnData,8);
    		writeData(parsedData);													//write data to sdcard
    		Serial.println(parsedData);
    		strncat(allData,"+" ,1);
    		strncat(allData,parsedData ,strlen(parsedData));
        }

			   
		printData(allData, mode);

		digitalWrite(RELAYPIN, LOW);
	}
        break;
    case 'S': { //with soms 
				
		if (PRINT_MODE == 1) Serial.println("  --function: getdataBroadcastNew(), case S -- called");
		Serial1.print("ARQWAIT");
                Serial.println(" XXXXXXXXXX ARQWAIT XXXXXXXXXXXXX");	
		GET_DATA(columnData,PASS_AXEL1_ADC_CALIB_MINMAX); 
		Serial.println(columnData);
		parsedData = parser(columnData,8);
		writeData(parsedData);													//write data to sdcard
		Serial.println(parsedData);
		sprintf(allData,parsedData ,strlen(parsedData));		
				
		Serial1.print("ARQWAIT");  
                Serial.println(" XXXXXXXXXX ARQWAIT XXXXXXXXXXXXX");	
		GET_DATA(columnData,PASS_AXEL2_ADC_CALIB_MINMAX);
		Serial.println(columnData);
		parsedData = parser(columnData,8);
		writeData(parsedData);													//write data to sdcard
		Serial.println(parsedData);
		strncat(allData,"+" ,1);
		strncat(allData,parsedData ,strlen(parsedData));

		Serial1.print("ARQWAIT");
                Serial.println(" XXXXXXXXXX ARQWAIT XXXXXXXXXXXXX");	
		GET_DATA(columnData,POLL_SOMS_RAW_NEW);
		Serial.print("columnData: ");
		Serial.println(columnData);
		parsedData = parser(columnData,2);
		writeData(parsedData);
		Serial.print("parsedData: ");													//write data to sdcard
		Serial.println(parsedData);
		strncat(allData,"+" ,1);
		strncat(allData,parsedData ,strlen(parsedData));
		
		Serial1.print("ARQWAIT");
                Serial.println(" XXXXXXXXXX ARQWAIT XXXXXXXXXXXXX");	
		GET_DATA(columnData,POLL_SOMS_CALIB_NEW);
		Serial.print("columnData: ");
		Serial.println(columnData);
		parsedData = parser(columnData,3);
		writeData(parsedData);
		Serial.print("parsedData: ");													//write data to sdcard
		Serial.println(parsedData);
		strncat(allData,"+" ,1);
		strncat(allData,parsedData ,strlen(parsedData));		

                if (PIEZO == 1) {		
    		  Serial1.print("ARQWAIT");        
    		  GET_DATA(columnData,255); // piezo
    		  Serial1.print("ARQWAIT");
    		  Serial.println(columnData);
    		  parsedData = parser(columnData,8);
    		  writeData(parsedData);													//write data to sdcard
    		  Serial.println(parsedData);
    		  strncat(allData,"+" ,1);
    		  strncat(allData,parsedData ,strlen(parsedData));
                }
                
		printData(allData, mode);
			
		digitalWrite(RELAYPIN, LOW);
		break;

   }
}
   digitalWrite(trigSW, LOW);
}


