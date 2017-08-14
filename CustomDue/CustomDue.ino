// error kapag walang name

#define ATCMD 		"AT"
#define ATECMDTRUE	"ATE"
#define ATECMDFALSE	"ATE0"
#define OKSTR 		"OK"
#define ERRORSTR	"ERROR"

#define datalogger Serial1


#include <SD.h>
#include <SPI.h>

char *columnDataVar = {};

bool ate = true;
int sensorVersion = 1;
int cdCounter = 0;
 
int timestart = 0;
int timenow = 0;

char master_name[6] = "ZEYTA";

void setup(){
	Serial.begin(9600);
	columnDataVar = (char *)malloc(2500*sizeof(char));
	for (int i = 0; i < 2500; i++) {
		columnDataVar[i] = '\0';
	}
}

void loop(){

	getATCommand();
}

void getATCommand(){
	String serial_line, command;
	int i_equals = 0;

	timestart= millis();
    timenow = millis();
	
    do{
		serial_line = Serial.readStringUntil('\r\n');
         timenow = millis();

	} while((serial_line == "") && (timenow - timestart < 120000 ));

    timenow = millis();
    
    if (timenow- timestart >= 120000  ){
        Serial.println ("timeout!");
        // operation();
    }  

	serial_line.toUpperCase();
	serial_line.replace("\r","");

	// echo command if ate is set, default true
	if (ate) Serial.println(serial_line);

	// get characters before '='
	i_equals = serial_line.indexOf('=');
	if (i_equals == -1) command = serial_line;
	else command = serial_line.substring(0,i_equals);

	
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
	
	else if (command == "AT+GETDATA"){
		readDataFromColumn(columnDataVar, 1);
		cdCounter = strlen(columnDataVar);
		Serial.println(columnDataVar);
		removeExtraCharacters(columnDataVar,1 );
		columnDataVar =	columnDataVar + cdCounter;
		Serial.println(OKSTR);
	}

	else if (command == "AT+SENDDATA"){
		sendData(true, "try lang");
	}

	else if (command == "AT+ADDINFO"){
		readDataFromColumn(columnDataVar, 1);
		appendInfo(columnDataVar, 1, 1, 1, "x");

		Serial.println(columnDataVar);

		// appendInfo(char* columnData, int sensorVersion, int num, int den)
	}

	else if (command == "AT+TOKENSBYDATA"){
		readDataFromColumn(columnDataVar, 1);
		char** tokens= {};

		tokens = (char**)malloc(5*sizeof(char*));
  		for(int i=0; i<5; i++)
    		(tokens)[i] = (char*)malloc(200*sizeof(char));


		// = (String**)malloc(3000*sizeof(char));

		tokenizeDatabyDataType(tokens, columnDataVar, true);
		// tokenizeDatabyDataType( columnDataVar, true);
		Serial.println("sa labas");
		for (int i = 0; i < 5; i++) {

			Serial.println(tokens[i]);
		}
	}

	else if (command == "AT+TOKENSBYMSG"){
		readDataFromColumn(columnDataVar, 1);
		
		int x= strlen(data_token);
		int token_count= x/cutoff_length;
		for(int i=0; i<token_count; i++)
    		(tokens)[i] = (char*)malloc(cutoff_length*sizeof(char));

		tokenizeDatabyMessage(char** message,columnDataVar, int cutoff_length)
		
	}


	else if (command == "AT+PRINTDATA"){
		// sendData(true, "try lang");
	}



	else{
		Serial.println(ERRORSTR);
	}
}
// void init(){


// }

// void initCAN(){


// }

// void initEEPROM(){

// }

// void loadVariablesFromSDcard(){


// }

// void readEEPROM(){


// }

// void getSensorMessageID(){


// }


void readDataFromColumn(char* columnData, int sensorVersion){
	// merong char array tapo*s lalagyan ng laman dito, dapat ay void lang 
	char* column = "";
	column = "abcdefghijklmnopqrst+abcdefghijklmnopqrstabcdefghijklmnopqrstabcdefghijklmnopqrst+abcdefghijklmnopqrstabcdefghijklmnopqrstabcdefghijklmnopqrst";
	strncpy(columnData, column, strlen(column));
}

void removeExtraCharacters(char* columnData, int cmd){
	int i = 0;
	char pArray[2500] = "";
	int initlen = strlen(columnData);
	for (i = 0; i < initlen; i++, columnData++) {
		switch (cmd) {
			case 1: {// axel data //13
				if (i % 20 != 0 && i % 20 != 1 && i % 20 != 4 && i % 20 != 5 && i % 20 != 8 && i % 20 != 12 && i % 20 != 16 ) {
					strncat(pArray, columnData, 1);
				}
				break;
			}
			case 2: { // raw soms // 10
				if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 != 12 && i % 20 < 14 ) {
					strncat(pArray, columnData, 1);
				}
				break;
			}
			case 3: { //calib soms //7
			  	if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 < 10 ) {
					strncat(pArray, columnData, 1);
				}
				break;
			}
			case 4: { // old format
			  if (i%20 != 0 && i%20!= 1 && i%20 != 4 && i%20 != 8 && i%20 != 12){
					strncat(pArray, columnData, 1);
				}
				break;
			}
			case 8: { // old axel /for 15
				if (i % 20 != 0 && i % 20 != 1 && i % 20 != 8 && i % 20 != 12 && i % 20 != 16 ) {
					strncat(pArray, columnData, 1);
				}
				break;
			}
		}
	}

	pArray[i] = '\0';
	i = 0;
	sprintf(columnData, pArray,strlen(pArray));
}

// void ewanKopa(){
// 	readDataFromColumn(); --> outputs char array
// 	removeExtraCharacters();--> outputs parsed data
// 	String* dasda= tokenizeDatabyDataType(columnData, isDebug); --> outputs stringssss depende sa data_type
// 	for loop:
// 		messageContentParameters(dasda [i]) --> totalToken, identifier, cutoff_length
					

// }


void tokenizeDatabyDataType(char** tokens, char* columnData, bool isDebug){

	char *token;
	const char delimiter[2] = "+";

	String token_array[5] = {}; // String array
	token = strtok(columnData, delimiter);
	strncat(token, "\0", 2);
	int j=0; 

	while (token != NULL) {
		sprintf(tokens[j], token, strlen(token));
		tokens[j][strlen(token)] = '\0';
		token = strtok(NULL, delimiter);
		delay(200);
		j++;
	}

	for (j; j < 5; j++) {
		tokens[j]= '\0';
	}
	for (int i = 0; i < 5; i++) {
		Serial.println(tokens[i]);
	}	
}


void tokenizeDatabyMessage(char** message,char* data_token, int cutoff_length){

	// char** message= {};

	message = (char**)malloc(token_count*sizeof(char*));
	

	for (int i = 0; i < tokens_count; i++) {
		sprintf(message[i], data_token, cutoff_length);
	}	

}

void messageContentParameters(int* parameters, String token){
	int token_length = token.length();
	char identifier = 'c';
	int cutoff_length = 0;
	int tokens_count = 0;
	char* token_array = {};
	token.toCharArray(token_array, 999);
	
	identifier= checkIdentifier(token_array);
	cutoff_length= checkCutoff(identifier);

	
	if (token_length == 0) {
		tokens_count = 0;
	} 
	
	else {
		tokens_count = (token_length / cutoff_length);
		if (token_length % cutoff_length!= 0) {
			tokens_count ++;
		}
	} 

	int retValues[3]= {0,0,0};
	retValues= {tokens_count, identifier, cutoff_length};
	return retValues;
}



char checkIdentifier(char* token){
	char idfier = '0';
	switch (token[3]) {
		case '1': {
			if (token[4] == '5') {
				idfier = 'b';
			}
			else if (token[4] == 'A') {
				idfier = 'c';
			}
			break;
		} case '0': {
			if (token[4] == 'B')
				idfier = 'x';
			else if (token[3] == 'C')
				idfier = 'y';
			break;
		} case '2': {
			Serial.println(token[4]);
			if (token[4] == '0'){
				idfier = 'x';
			} else if (token[4] == '1'){
				idfier = 'y';
			}
			break;
		} case '6': {
			idfier = 'b';
			break;
		} case '7': {
			idfier = 'c';
			break;
		}
		default: {
			idfier = '0';
			break;
		}
	}
	return idfier;
}

int checkCutoff(char idf){
	int cutoff=0;
	Serial.println("idf");
	Serial.println(idf);
	
	switch (idf) {
		case 'b': {
			cutoff = 130;
			break;
		}
		case 'x': {
			cutoff = 135;  //15 chars only for axel
			break;
		}
		case 'y': {
			cutoff = 135;  //15 chars only for axel
			break;
		}
		case 'c': {
			cutoff = 133;  //15 chars only for axel
			break;
		}
		default: {
            cutoff = 0;
            if (sensorVersion ==1){
                cutoff = 135;
          	}
			break;
		}
	}
	return cutoff;
}


void noDataParsed(char* columnData){
	
	sprintf(columnData, "040>>1/1#", 3);
	strncat(columnData, master_name, 5);
	strncat(columnData, "*0*ERROR: no data parsed<<", 26);
}

//okay na
void appendInfo(char* columnData, int sensorVersion, int numerator, int denominator, char* data_type){
	// len>>10/10#ZEYTADUE*x*data<<
	char* appendedArray = "";
	char int_to_charArr[3] ={};


	appendedArray = (char *)malloc(900*sizeof(char));
	for (int i = 0; i < 900; i++) {
		appendedArray[i] = '\0';
	}
	sprintf(int_to_charArr, "%d", strlen(columnData));

	
	if (strlen(columnData)<100){
		sprintf(appendedArray, "0", 1);
		strncat(appendedArray, int_to_charArr, strlen(int_to_charArr));	
	}
	else{
		sprintf(appendedArray, int_to_charArr, strlen(int_to_charArr));		
	}

	strncat(appendedArray, ">>", 2);


	if (numerator < 10){
		strncat(appendedArray, "0", 1);		
	}
	sprintf(int_to_charArr, "%d", numerator);
	strncat(appendedArray, int_to_charArr, strlen(int_to_charArr));
	strncat(appendedArray, "/", 1);




	if (denominator < 10){
		strncat(appendedArray, "0", 1);
	}
	sprintf(int_to_charArr, "%d", denominator);	
	strncat(appendedArray, int_to_charArr, strlen(int_to_charArr));
	strncat(appendedArray, "#", 1);

	strncat(appendedArray, master_name, strlen(master_name));

	if (sensorVersion == 1){
		//append ng DUE na word
	strncat(appendedArray, "DUE", 3);

	}

	strncat(appendedArray, "*", 1);

	//para sa data type	
	strncat(appendedArray, data_type, 1);


	strncat(appendedArray, "*", 1);

	strncat(appendedArray, columnData, strlen(columnData));

	strncat(appendedArray, "<<", 2);

	Serial.println("appendedArray");

	Serial.println(appendedArray);

	strncpy(columnData, appendedArray, strlen(appendedArray));

//	free(appendedArray);

	Serial.println("columnData");

	Serial.println(columnData);
}

void sendData(bool isDebug, char* columnData){
	timestart = millis();
	timenow = millis();
	bool OKFlag = false;
	Serial.println("nasa send");
	if (isDebug == true){
		Serial.println("debug is true");
		Serial.println(columnData);

		do{
			timestart = millis();
			timenow = millis();
			while (!Serial.available()){
				while ( timenow - timestart < 9000 ) {
					timenow = millis();
				}
				
				Serial.println("Time out...");
				break;
			}

			if (Serial.find("OK")){
				OKFlag = true;
				Serial.println("moving on");
			}
			else{
				Serial.println(columnData);				
			}
		} while (OKFlag == false);
	}
	else {
		do{		
			datalogger.println(columnData);
			timestart = millis();
			timenow = millis();

			while (!datalogger.available()){
				while ( timenow - timestart < 9000 ) {
					timenow = millis();
				}
				datalogger.println("Time out...");
				break;
			}

			if (datalogger.find("OK")){
				OKFlag = true;
			}
			else{
				datalogger.println(columnData);				
			}

		} while (OKFlag == false);

	}

} 

// void writeDataToSD(){


// }

// void saveAll(){

// }

// void operation(){}