int ublox_get_rtcm(){
  int i = 0, c = 0;
    if (Serial1.available() > 0) {
      delay(50); //allows all serial sent to be received together
      while(Serial1.available()) {
        rtcm[i] = Serial1.read();
        
//          if (rtcm[i]==211 && i!=0){
//            Serial.println();
//            c=0;
//          }
//          Serial.print(rtcm[i]);
//          Serial.print(" ");
//          c++;
//          if (c%16==0){
//            Serial.println();
//            c=0;
//          }
          i++;
        }      
        Serial.println();
        Serial.print("number of packets: ");
        Serial.println(i);
        return i;
    }
  else return 0;
}
