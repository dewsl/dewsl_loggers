/* 
	Function: sd_init
		Assert the chip_select pin for SD use.	
	Parameters:
		n/a
	Returns:
		-1 fail, 0 success.
	See Also:
		<main.ino/setup>
*/
int init_sd(int chip_select){
	if (!SD.begin(chip_select)){
		Serial.println("SD initialization failed!");
		return -1;
	}
	return 0;
}

void init_gids(){
	if (VERBOSE == 1){Serial.println(F("init_uids()"));}
	for (int i=0; i < g_num_of_nodes; i++ ){
		g_gids[i][0] = 0;	// unique id
		g_gids[i][1] = i+1; // geographic id ( 1- 40)
	}
	return;
}
/* 
	Function: open_config
		Open the CONFIG.txt file in the SD card and process
		each line.	
	Parameters:
		n/a
	Returns:
		n/a
	See Also:
		<main.ino/getATCommand>
*/
void open_config(){
	File g_file;
	char *temp; 							// address for a character
	int max_char_per_line = 1000;
	char one_line[max_char_per_line];
	g_file = SD.open("CONFIG.txt");

	if (!g_file){
		Serial.println("CONFIG.txt not found.");
		return;
	} 

	memset(one_line,0,sizeof(one_line));
	temp = one_line; 						// temp starts at address of one_line
	while((*temp = g_file.peek())!= -1){  	// para may paglalagyan ng result ng .peek()
		temp = one_line; 					// start ulit sa address ng one_line
		while(((*temp = g_file.read()) != '\n')) {
			temp++;
			if(*temp == -1)
				break;
		}
		if (process_config_line(one_line)){
			break;
		}
		memset(one_line,0,sizeof(one_line));
	}
	Serial.println("Finished config processing");
	g_file.close();
	return;
}
unsigned int process_config_line(char *one_line){
	String str1;
	str1 = String(one_line);
	if ( (str1.startsWith("mastername")) | (str1.startsWith("MasterName")) ){
		return 0;
	} else if((str1.startsWith("endofconfig")) | (str1.startsWith("ENDOFCONFIG"))){
		return 1;
	} else if((str1.startsWith("columnids")) | (str1.startsWith("column1")) 
		| (str1.startsWith("columnIDs"))) {
		g_num_of_nodes = process_column_ids(str1);
		print_gids();
		return 0;
	} else {
		return 0;
	}
}
/* 
	Function: process_column_ids
		Find the unique ids from string and place in g_gids	
	Parameters:
		String line - String object containing ids
	Returns:
		The number of unique ids read.
	See Also:
		<process_config_line>
*/
int process_column_ids(String line){
	// Serial.println(line);
	String delim = ",";
	int i,i0,i1,i2,ilast,id;
	i = 0;
	ilast = line.lastIndexOf(delim);
	i0 = line.lastIndexOf("=");
	i1 = line.indexOf(delim);
	id =  line.substring(i0+1,i1).toInt();
	g_gids[i][0] = id;
	i++;
	while ((i1 <= ilast) & ( i < 40 )){
		i2 = line.indexOf(delim,i1+1);
		id = line.substring(i1+1,i2).toInt();
		g_gids[i][0] = id;
		i++;
		i1 = i2;
		if (i1 == -1)
			break;
	}
	return i+1;
}
/* 
	Function: print_gids
		Print in the gids and uids from the global g_gids.
	Parameters:
		n/a
	Returns:
		n/a
	See Also:
		<init_gids>
*/
void print_gids(){
	char gid[2],uid[4];
	Serial.println(F("================================="));
	Serial.println(F("Geographic ID\t\tUnique ID"));
	Serial.println(F("================================="));
	for (int i = 0; i<g_num_of_nodes-1; i++){
		sprintf(gid,"%2d",g_gids[i][1]);
		sprintf(uid,"%4d",g_gids[i][0]);
		Serial.print("\t");
		Serial.print(gid);
		Serial.print("\t\t");
		Serial.println(uid);
	}
	Serial.println(F("================================="));
}
