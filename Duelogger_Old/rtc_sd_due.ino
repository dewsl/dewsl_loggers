//MASTERNAME IN WRITEDATA



#include <SD.h>
#include <SPI.h>
#include <string.h>

/*from due working writing software SPI */
//#define SCKPIN			52
//#define MOSIPIN			51
//#define MISOPIN			50

File sdFile;			   //!< file for interfacing SdCard files
//char configFileName[] = "senslopeConfig001.txt";

#define WAIT_TO_START    1 // Wait for serial input in setup()
#define SYNC_INTERVAL 1000 // mills between calls to sync()
//#define SITE_NAME "TESTA00.TXT"

const int  cs=18; //chip select RTC

/*rap modification due logger implementation*/
char loggerFileName[100];

int RTC_init(){ 
	
	/*added code for changing stuff */
	
	 
    pinMode(cs,OUTPUT); // chip select
    SPI.begin();
    SPI.setBitOrder(MSBFIRST); 
    SPI.setDataMode(SPI_MODE1); // both mode 1 & 3 should work 
    digitalWrite(cs, LOW);  
    SPI.transfer(0x8E);
	
	
    Serial.println("7");
    SPI.transfer(0x60); //60= disable Osciallator and Battery SQ wave @1hz, temp compensation, Alarms disabled
	
	
    Serial.println("8");
    digitalWrite(cs, HIGH);
    delay(10);

}
/************************************************************************/
/* @brief      reads the sdcard and gets the configuration from the sdcard
 /************************************************************************/
 unsigned int loadVariablesFromSdcard(void){
	 char maxline  = 18;
	 char linecount = 0;
	 unsigned char endoffile;
	 unsigned int lineLength = 1000;
	 unsigned int x;
	 char oneLine[lineLength];
	 char *byteptr;

	initGIDTable();
	initiSOMSNodeSpecificTable();
		
	SPI.setBitOrder(LSBFIRST); 
		
	if (!SD.begin(6,chipSelect)) {
		   Serial.println(" SDCARD initialization failed!");
		   return -1;
	}
	  
	 //return if configuration file not found	 
	 sdFile = SD.open("CONFIG.txt", FILE_READ);
	 
        //read the part on the nodes and place them in the global variable
	 if(!sdFile){
		 Serial.println("failed to open file");
		 return -2;
	 }
	 Serial.println("Reading contents");	
	 byteptr = oneLine;
	 while((*byteptr = sdFile.peek())!= -1){
		 byteptr = oneLine;
		 while(((*byteptr = sdFile.read())!= '\n') ){
		 
			byteptr++; 
			
			if(*byteptr == -1)
				break;	
		 }
		 
		 linecount++;
		 if(linecount>maxline) break;
		
		 endoffile =processConfigLine(oneLine); //check if line has a command
		 if(endoffile) break;
			
		 //clearout oneLine
		 for(x=0;x<lineLength;x++){
			 oneLine[x] = 0;
		 }
		 
	 }
	 //read number of nodes
	 
	//read number of columns
	Serial.println("Finished Reading Configuration file");
	
        sdFile.close();
	SD.end();
	Serial.println("after ending");
	
        //make sure to disable the chip select pin
	digitalWrite(chipSelect, HIGH);
	return 0;
	 
 }
 
   unsigned int processConfigLine(char *ptr)
   {
	   String str,str2;
	   char *sptr;
	   char buff[64];
	   int indexOfValue,value;
	   
           //check if matches any of the commands
	   str = String(ptr);
	   
	   if(str.startsWith("numOfNodes")){
		   indexOfValue =str.indexOf("=");
		   indexOfValue++; // get index of the number
		   sptr = &str[indexOfValue];
		   str2 = String(sptr);
		   value = str2.toInt();
		   numOfNodes = value;
		   sprintf(buff,"numOfNodes Found. New value = %d",numOfNodes);
		   Serial.println(buff);
		   return 0;

           }else if(str.startsWith("numOfNodesColumn1")){
		   indexOfValue =str.indexOf("=");
		   indexOfValue++; // get index of the number
		   sptr = &str[indexOfValue];
		   str2 = String(sptr);
		   value = str2.toInt();
		   numOfNodes = value;
		   sprintf(buff,"numOfNodes Found. New value = %d",numOfNodes);
		   Serial.println(buff);
		   return 0;

           }else if(str.startsWith("TURN_ON_DELAY")){
		   indexOfValue =str.indexOf("=");
		   indexOfValue++; // get index of the number
		   sptr = &str[indexOfValue];
		   str2 = String(sptr);
		   value = str2.toInt();
		   TURN_ON_DELAY = value;
		   sprintf(buff,"TURN_ON_DELAY Found. New value = %d",TURN_ON_DELAY);
		   Serial.println(buff);
		   return 0;

	    }else if(str.startsWith("column1")){
		   indexOfValue =str.indexOf("=");
		   indexOfValue++; // get index of the number
		   sptr = &str[indexOfValue];
		   //this part gets the id per member in the column
		   char *colptr;
		   colptr = strtok(sptr,",");
		   int x;
		   x = 0;
		   while((colptr != NULL)){
			   GIDTable[x][1] = atoi(colptr);
			   sprintf(buff,"column1 Found. New value = %d",GIDTable[x][1]);
			   Serial.println(buff);
			   colptr = strtok(NULL,",");
			   x++;
		   }
		   numOfNodes = x; // set number of nodes of column1
		   str2 = String(sptr);
		   value = str2.toInt();
		   column1 = value;
		   sprintf(buff,"column1 Found. New value = %d",column1);
		   Serial.println(buff);
		   return 0;
		   
	  }else if(str.startsWith("ColumnCommand")){ //checks if the command is for column
		 indexOfValue =str.indexOf("=");
		 indexOfValue++; // get index of the number
		 while(str[indexOfValue] == ' ')
		 indexOfValue++;
		 sptr = &str[indexOfValue];
		 ColumnCommand = *sptr;
		 sprintf(buff,"ColumnCommand Found = %c",ColumnCommand);
		 Serial.println(buff);
		 return 0;
		 
	}else if(str.startsWith("MasterName")){ //checks if the command is for column
		
		indexOfValue =str.indexOf("=");
		indexOfValue++; // get index of the number
		while(str[indexOfValue] == ' ')
	        indexOfValue++;
		sptr = &str[indexOfValue];
		strncpy(MASTERNAME,sptr,5);
		sprintf(buff,"New MasterName = ");
		strncat(buff,MASTERNAME,5);
		strcat(buff,"\n");
		Serial.println(buff);
		return 0;
		
	}else if(str.startsWith("SamplingMaxNumOfRetry")){
			indexOfValue =str.indexOf("=");
			indexOfValue++; // get index of the number
			sptr = &str[indexOfValue];
			str2 = String(sptr);
			value = str2.toInt();
			NO_COLUMN_LIMIT = value;
			sprintf(buff,"NO_COLUMN_LIMIT = %d",NO_COLUMN_LIMIT);
			Serial.println(buff);
			return 0;

	}else if(str.startsWith("REPEATING_FRAMES_LIMIT")){
			indexOfValue =str.indexOf("=");
			indexOfValue++; // get index of the number
			sptr = &str[indexOfValue];
			str2 = String(sptr);
			value = str2.toInt();
			REPEATING_FRAMES_LIMIT = value;
			sprintf(buff,"REPEATING_FRAMES_LIMIT = %d",REPEATING_FRAMES_LIMIT);
			Serial.println(buff);
			return 0;

	}else if(str.startsWith("enable_find_node_ids")){
			indexOfValue =str.indexOf("=");
			indexOfValue++; // get index of the number
			sptr = &str[indexOfValue];
			str2 = String(sptr);
			value = str2.toInt();
			enable_find_node_ids= value;
			sprintf(buff,"enable_find_node_ids = %d",enable_find_node_ids);
			Serial.println(buff);
			return 0;


	}else if(str.startsWith("RepeatingRetryLimit")){
			indexOfValue =str.indexOf("=");
			indexOfValue++; // get index of the number
			sptr = &str[indexOfValue];
			str2 = String(sptr);
			value = str2.toInt();
			REPEATING_FRAMES_RETRY_LIMIT  = value;
			sprintf(buff,"REPEATING_FRAMES_RETRY_LIMIT = %d",REPEATING_FRAMES_RETRY_LIMIT );
			Serial.println(buff);
			return 0;

	}else if(str.startsWith("ALLOWED_MISSING_NODES")){
			indexOfValue =str.indexOf("=");
			indexOfValue++; // get index of the number
			sptr = &str[indexOfValue];
			str2 = String(sptr);
			value = str2.toInt();
			ALLOWED_MISSING_NODES  = value;
			sprintf(buff,"ALLOWED_MISSING_NODES = %d",ALLOWED_MISSING_NODES );
			Serial.println(buff);
			return 0;
			
	}else if(str.startsWith("PRINT_MODE")){
			indexOfValue =str.indexOf("=");
			indexOfValue++; // get index of the number
			sptr = &str[indexOfValue];
			str2 = String(sptr);
			value = str2.toInt();
			PRINT_MODE = value;
			sprintf(buff,"PRINT_MODE = %d",PRINT_MODE);
			Serial.println(buff);
			return 0;
	}else if(str.startsWith("PIEZO")){
			indexOfValue =str.indexOf("=");
			indexOfValue++; // get index of the number
			sptr = &str[indexOfValue];
			str2 = String(sptr);
			value = str2.toInt();
			PRINT_MODE = value;
			sprintf(buff,"PIEZO = %d",PIEZO);
			Serial.println(buff);
			return 0;
	
	}else if(str.startsWith("ENDOFCONFIG")){ //checks if the command is for column
			
		Serial.println("End of file found");
		return 1;
		
	}else{
		Serial.println("UnknownLine");
		return 0;
	     }
  }
   
 
 void initGIDTable(void){
	  unsigned int count;
	  count = 0;
	  for(count = 0;count < CANARRAYMAXSIZE;count++){
		  GIDTable[count][1] = 0;
		  GIDTable[count][0] = count+1;
	  }
 }
 
 void initiSOMSNodeSpecificTable(void){
	 unsigned int count;
	 count = 0;
	 for(count = 0;count < CANARRAYMAXSIZE;count++){
		 SOMSNodeSpecificTable[count][1] = 0;
		 SOMSNodeSpecificTable[count][0] = count+1;
	 }
 }


//=====================================
String ReadTimeDate(int *secp, int *minp, int *hrp){
  String temp;
  int TimeDate [7]; //second,minute,hour,null,day,month,year   
  
   SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE1); // both mode 1 & 3 should work   
  for(int i=0; i<=6;i++){
    if(i==3)
      i++;
    digitalWrite(cs, LOW);
    SPI.transfer(i+0x00);
	
	 
    unsigned int n = SPI.transfer(0x00);
	
	   
    digitalWrite(cs, HIGH);
    int a=n & B00001111;    
    if(i==2){ 
      int b=(n & B00110000)>>4; //24 hour mode
      if(b==B00000010)
        b=20;        
      else if(b==B00000001)
        b=10;
      TimeDate[i]=a+b;
    }
    else if(i==4){
      int b=(n & B00110000)>>4;
      TimeDate[i]=a+b*10;
    }
    else if(i==5){
      int b=(n & B00010000)>>4;
      TimeDate[i]=a+b*10;
    }
    else if(i==6){
      int b=(n & B11110000)>>4;
      TimeDate[i]=a+b*10;
    }
    else{ 
      int b=(n & B01110000)>>4;
      TimeDate[i]=a+b*10; 
      }
  }

//2020-12-31 23:55:00
  temp.concat("20");
  temp.concat(TimeDate[6]); // YEAR
  temp.concat("-") ;
  if (TimeDate[5] <10)
    temp.concat("0");
  temp.concat(TimeDate[5]); //MONTH 
  temp.concat("-") ;
  if (TimeDate[4] <10)
    temp.concat("0");
  temp.concat(TimeDate[4]); //DAY
  temp.concat(" ") ;
  if (TimeDate[2] <10)
    temp.concat("0");
  temp.concat(TimeDate[2]); // HH
  temp.concat(":") ;
  if (TimeDate[1] <10)
    temp.concat("0");
  temp.concat(TimeDate[1]); //MIN
  temp.concat(":") ;
  if (TimeDate[0] <10)
    temp.concat("0");
  temp.concat(TimeDate[0]);  // SEC

        *secp= TimeDate[0];
        *minp= TimeDate[1];
        *hrp= TimeDate[2];
  Serial.println(temp);      
  return(temp);
}


/*this part are reimplementation of the SD functions 
from MEGA to be written in the DUE */


/**

@brief    this function initializes the SD card

*/
int8_t initSD(void){
	String timeString;
        int sec1, min1, hr1, sec2, min2, hr2;
  
        File sdFile;
	unsigned int i,sentinel;
	SPI.setDataMode(SPI_MODE0); // 

	if (!SD.begin(6,chipSelect)) {
		Serial.println(" SDCARD initialization failed!");
		return -1;
	}
	Serial.println("SDCARD initialization Success");

	char name[10] = {};
        strcpy(name,MASTERNAME);
	strncat(name,".csv", 4);
	
	sdFile = SD.open(name,FILE_WRITE);
	if(!sdFile){
		Serial.println("Failed to create logger file");
		return -2;
	}
	
	Serial.println("created the name file");			
        //write the current date and stuff
  	
	sdFile.close();//close the file
	SD.end();
	
	return 0;

}

int8_t writeData(String data){
	File sdFile;
	char filename[100]= {};
	String timeString;
	int sec1, min1, hr1, sec2, min2, hr2;
	SPI.setDataMode(SPI_MODE0); // switch mode to SD
	SPI.setBitOrder(LSBFIRST); 
	Serial.println(TIMESTAMP);


	if (!SD.begin(6,chipSelect)) {
		Serial.println(" SDCARD: walang timestamp!");
		return -1;
	}

	delay(20);
	loggerFileName= {};
	for(int i=0; i<6 ; i++){
		loggerFileName[i]= TIMESTAMP[i];
	}

	strcpy(filename,loggerFileName);

	strcat(filename,".TXT");
	Serial.println(filename);

	sdFile = SD.open(filename,FILE_WRITE);
	if(!sdFile){
		Serial.println("Can't Write to file");
		return -1;
	}
	sdFile.print(MASTERNAME);
	sdFile.print("*");
	sdFile.print(data);
	sdFile.print("*");

	//get timestamp from what was given by the MEGA
	sdFile.print(TIMESTAMP);
	sdFile.println();
	sdFile.close();//close the file
	SD.end();
	SPI.setDataMode(SPI_MODE3); // switch mode to clock
	Serial.println("writing to SD"); 
}
/*
  unsigned int changeGlobalWaitTime(unsigned int time){
	  GlobalWaitTime = time;
	  return 0;
  }
  
  unsigned int setUseGlobalWaitTime(unsigned char on){
	  
	  if(on)
	  useGlobalWaitTime = 1;
	  else
	  useGlobalWaitTime = 0;
	  
	  return useGlobalWaitTime;
  }


  unsigned int setFilterForRepeatingAndWrong(unsigned char on){
	  if(on)
	  CanUseFilter= 1;
	  else
	  CanUseFilter= 0;
	  
	  return CanUseFilter;
  }
*/
