void getRawData(int mode, char readCode){
  //mode- 1 if ARQ
  //mode 0 if debugger
  //char readCode= 'x';
  commandCAN = 0;
  //readCode= getMode(mode);
  
  switch(readCode){
    case 'A': {   //returns raw data from axel-1    
      if (SERIAL)
      Serial.println("- Axel 1");
      digitalWrite(RELAYPIN, HIGH);
      delay(4500);
      //getSingleData(axel1,columnData); 
      digitalWrite(RELAYPIN, LOW);
      parsedData = parser(columnData, 0);
      //printData(parsedData, mode, "a");
      break;
    }
    
    case 'B': {   //returns raw data from axel-2
      if (SERIAL)
      Serial.println("- Axel 2");
      digitalWrite(RELAYPIN, HIGH);
      delay(4500);
      //getSingleData(); 
      digitalWrite(RELAYPIN, LOW);
      parsedData = parser(columnData, 0);
      //printData(parsedData, mode, "a");
      break;
    }
    
    case 'C': { //returns soms data via polling
      if (SERIAL)
      Serial.println("- soms polling");
      digitalWrite(RELAYPIN, HIGH);
      delay(4500);
      //getSingleData(); 
      digitalWrite(RELAYPIN, LOW);
      parsedData = parser(columnData, 0);
//      printData(parsedData, mode, "a");
      break;      
    }
    
    case 'S': { //returns soms data via broadcast  
      if (SERIAL)
      Serial.println("- soms broadcast");
      digitalWrite(RELAYPIN, HIGH);
      delay(4500);
      //getSingleData(); 
      digitalWrite(RELAYPIN, LOW);
      parsedData = parser(columnData, 0);
//      printData(parsedData, mode, "a");
      break;    
    }
    
  }
}



