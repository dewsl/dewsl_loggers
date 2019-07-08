void settings(int mode, char readCode){
  //char readCode= 'x';
  int c=0;
  
  //readCode= getMode(mode);
  switch(readCode){
    case 'D':{ 
    //store default name
      for (c=0; c<5; c++){
       dueFlashStorage.write(c, MASTERNAME[c]);
       updateMasterName();
      }
      break;
    }
    
    case 'W':{   
    //write new name
      for (c=0; c<5; c++){
       if (mode==0){ 
         dueFlashStorage.write(c,(char)Serial3.read());
         updateMasterName();
       }
       else if (mode==1){
         dueFlashStorage.write(c,(char)Serial.read());       
       updateMasterName();
       }
     }
     break;
    }
    
    case 'K':{   
    //readtime
         Serial.println(ReadTimeDate(&lastSec, &lastMin, &lastHr));

     break;
    }
 
    
    case 'R':{
    //read current name
    if (SERIAL)
    Serial.print("MASTERNAME: ");
    
    for (c=0; c<5; c++){
      if (SERIAL)
      Serial.print((char)dueFlashStorage.read(c));
     }
     if (SERIAL)
     Serial.println();
     
      break;
    }
    
   }
}


void updateMasterName() {
  int c=0;
  for (c=0; c<5; c++){
     MASTERNAME[c]= (char)dueFlashStorage.read(c);
  }
    
}
