int sd_init(int chip_select){
	if (!SD.begin(chip_select)){
		Serial.println("SD initialization failed!");
		return -1;
	}
	return 0;
}

String open_config(){
	String conf;
	File g_file;
	char *temp;
	conf = String("");
	g_file = SD.open("CONFIG.txt");

	if (!g_file){
		Serial.println("CONFIG.txt not found.");
		return String("");
	} 

	while(g_file.available()){
		*temp = g_file.read();
		conf = String( conf + String(*temp));
	}
	g_file.close();
	// SD.end();
	return conf;
}

void process_config_line(String line){

}

void process_config(String conf){
	// split into config lines
	String one_line;
	char *token;
	token = strtok(conf,'\n');
	while (token != NULL){
		token = strtok(NULL,'\n');
		process_config_line(token);
	}
	// int conf_len = 0;

	// conf_len = conf.length();
	// if ( conf.substring(0,conf_len-1) == "date" ){
	// 	Serial.println("may nahanap na date");
	// }
}