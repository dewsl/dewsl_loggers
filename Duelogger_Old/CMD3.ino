void getGroupedData(int mode, char readCode){
  
  //char readCode= 'x';
  commandCAN = 0;
  //readCode= getMode(mode);
 
  switch(readCode){
    case 'A': {   //returns calibrated axel 1, calibrated axel 2, calibrated soms [broadcast]
       break;
    }
    
    case 'B': {   //returns calibrated axel 1, calibrated axel 2, calibrated soms [polling]
      break;
    }
    
    case 'C': { //returns calibrated axel 1, calbrated axel 2 
      break;      
    }
    
    case 'D': { //returns raw axel 1, raw axel 2  
      break;
    }
    
    case 'E': { //returns calibrated axel 1, calibrated axel 2, calibrated soms [polling], raw soms [polling]   
      break;
    }
    
    
    case 'F': { //returns raw axel 1, raw axel 2, calibrated soms [polling], raw soms [polling]   
      break;
      
    }
    
    case 'G': { //returns calibrated axel 1, calibrated axel 2, calibrated soms [broadcast], raw soms [broadcast]   
      break;
    }
    
    case 'H': { //returns raw axel 1, raw axel 2, calibrated soms [broadcast], raw soms [broadcast]   
      break;
    }
    
    case 'I': { //returns self test 2  
      break;
    }
    
    case 'J': { //returns self test 3  
      break;
    }
  }
}
