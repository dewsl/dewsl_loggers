
struct data_type_params{
	int type_number;
	int data_length;
	int type_cutoff;
	char identifier[3];
} struct_dtype;

/* 
  Function: to_base64

    Convert integers to a padded base64 number system.

  Parameters:

    input - integer to be converted
    dest - pointer to char array where the converted input will be stored

  Returns:

    n/a

  See Also:

    <process_g_temp_dump>
*/
void to_base64(int input, char* dest){
	int result = 0;
	int div_res,mod_res;
	int in = input;
	char temp[2] = {};
	div_res = in / 64;
	mod_res = in % 64;
	dest[0] = '\0'; // iinitialize yung dest para strcat na lang lagi ang pagsulat
	if (input == 0) {
		sprintf(temp,"%c",base64[0]);
		strcat(dest,temp);
	}
	while ( ((mod_res > 0) && (div_res >= 0)) || ((mod_res == 0 )&& (div_res > 0))){

		sprintf(temp,"%c",base64[mod_res]);
		strcat(dest,temp);
		in = div_res;
		mod_res = in % 64;
		div_res = in / 64;
		if ((mod_res == 0) && (div_res == 0)) {
			break;
		}
	}
	reverse_char_order(dest,strlen(dest));
	// pad here
}

/* 
  Function: reverse_char_order

    Reverse the order in which a string is written. For example a 3 char
    array with "ABC" will result in "CBA". Limited to 4 characters only.
    Hardcoded restriction.

  Parameters:

    input - pointer to char array to be reversed
    length - length of input

  Returns:

    n/a

  See Also:

    <to_base_64>
*/
void reverse_char_order(char* input, uint8_t length){
	char temp[4] = {0};
	char temp1[2] = {0};
	temp[0] = '\0';
	for (int i = length-1; i >= 0; i--){
		sprintf(temp1,"%c",input[i]);
		strcat(temp,temp1);
	}
	temp[length] = '\0';
	strncpy(input,temp,strlen(temp));
}

/* 
  Function: pad_b64

	Pad the input given the length_of_output and write it on the dest.
	Limited to 5 characters only.
    Hardcoded restriction.

  Parameters:

    length_of_output - integer expected length of output
    input - pointer to char array that contains the input
    dest - pointer to char array where the padded output will be written

  Returns:

    n/a

  See Also:

    <to_base_64>
*/
void pad_b64(uint8_t length_of_output, char* input, char* dest){
	int num_of_pads = 0;
	char temp[5] = {};
	temp[0] = '\0'; // para makapag strncat ka dahil may null na si temp
	num_of_pads = length_of_output - strlen(input);

	if (num_of_pads > 0){
		for (int i = 0; i<num_of_pads; i++){
			strcat(temp,"A");
		}
		strcat(temp,input);
		strncpy(dest,temp,strlen(temp));
	} else {
		strncpy(dest,input,length_of_output);
		// temp[length_of_output] = '\0';
	}
}

/* 
  Function: b64_identify_params

	

  Parameters:

	msgid - integer message id /command

  Returns:

    data_type_params - struct with members:
    struct_dtype.indetifier - 2 char identifier - converted msgid to b64
    struct_dtype.type_number - 1 - tilt, 2 - soms, 3 - diagnostics
    struct_dytpe.data_length - total number of characters that represent the data

  See Also:

    <to_base_64>
*/
struct data_type_params b64_identify_params(int msgid){
	char temp[3],temp1[3];
	if (VERBOSE == 1) { Serial.println("b64_identify_params()"); }
	to_base64(msgid,temp);
	pad_b64(2,temp,temp1);
	temp1[2] = '\0';
	strncpy(struct_dtype.identifier,temp1,2);
	switch(msgid){
		case 255:{
			struct_dtype.type_number = 1;
			struct_dtype.data_length = 11;
			break;
		} case 11: {
			struct_dtype.type_number = 1;
			struct_dtype.data_length = 11;
			struct_dtype.type_cutoff = 135; // 9 chars per node tilt data	
			break;
		} case 12: {
			struct_dtype.type_number = 1;
			struct_dtype.data_length = 11;
			struct_dtype.type_cutoff = 135; // 9 chars per node tilt data	
			break;
		} case 32: {
			struct_dtype.type_number = 1;
			struct_dtype.data_length = 11;
			struct_dtype.type_cutoff = 135; // 9 chars per node tilt data	
			break;
		} case 33: {
			struct_dtype.type_number = 1;
			struct_dtype.data_length = 11;
			struct_dtype.type_cutoff = 135; // 9 chars per node tilt data	
			break;
		} case 22:{
			struct_dtype.type_number = 3;
			struct_dtype.data_length = 5;
			struct_dtype.type_cutoff = 135;
			break;
		} case 110:{
			struct_dtype.type_number = 2;
			struct_dtype.data_length = 5;
			struct_dtype.type_cutoff = 135; // 4 chars per node soms data	
			break;
		} case 113: {
			struct_dtype.type_number = 2;
			struct_dtype.data_length = 5;
			struct_dtype.type_cutoff = 135; // 4 chars per node soms data	
			break;
		} case 111: {
			struct_dtype.type_number = 2;
			struct_dtype.data_length = 5;
			struct_dtype.type_cutoff = 135; // 4 chars per node soms data	
			break;
		} case 112:{
			struct_dtype.type_number = 2;
			struct_dtype.data_length = 5;
			struct_dtype.type_cutoff = 135; // 4 chars per node soms data	
			break;
		} case 10:{
			struct_dtype.type_number = 2;
			struct_dtype.data_length = 6;
			struct_dtype.type_cutoff = 120; // 4 chars per node soms data	
			break;
		} case 13:{
			struct_dtype.type_number = 2;
			struct_dtype.data_length = 6;
			struct_dtype.type_cutoff = 120; // 4 chars per node soms data	
			break;
		}
	}
	return struct_dtype;
}

/* 
  Function: b64_write_frame_to_dump

    Write the frames as b64 encoded char array.
    This differs from *<write_frame_to_dump>* primarily because this function converts the uids to gids.
	
	Format:
	gids - 1 char
	msgid - 2 chars
	data - varies depending on msgid.

	*[msgid][gids][data][-][gid][data][-]. . .*

  Parameters:

    incoming- CAN_FRAME struct to be written in dump
    dump - pointer to char buffer

  Returns:

    n/a

  See Also:

    <process_g_temp_dump>
*/
void b64_write_frame_to_dump(CAN_FRAME incoming, char* dump){
	char temp[3] = {};
	// char temp1[1] = {};
	char temp2[3] = {};



	char delim[2] = "-";
	int gid,msgid,x,y,z,v,somsr,tmp;
	// kailangang iconsider dito yung -1 na gid 
	// kapag wala yung incoming.id sa columnIDs
	gid = convert_uid_to_gid(incoming.id); 
	msgid = incoming.data.byte[0];
	struct data_type_params dtype = b64_identify_params(msgid);
	switch(dtype.type_number) {
		case 1: {

			x = compute_axis(incoming.data.byte[1],incoming.data.byte[2]);
			y = compute_axis(incoming.data.byte[3],incoming.data.byte[4]);
			z = compute_axis(incoming.data.byte[5],incoming.data.byte[6]);
			v = incoming.data.byte[7];

  			sprintf(temp2,"%02X",msgid);
			strcat(dump,temp2);

			to_base64(gid,temp);
			pad_b64(1,temp,temp2);
			temp2[1] = '\0'; // append null 
			strcat(dump,temp2);

			to_base64(x,temp);
			pad_b64(2,temp,temp2);
			strcat(dump,temp2);

			to_base64(y,temp);
			pad_b64(2,temp,temp2);
			strcat(dump,temp2);

			to_base64(z,temp);
			pad_b64(2,temp,temp2);
			strcat(dump,temp2);

			to_base64(v,temp);
			pad_b64(2,temp,temp2);
			strcat(dump,temp2);

			break;
		}
		case 2: {
			// based on v3 sensor code.
			somsr = compute_axis(incoming.data.byte[1],incoming.data.byte[2]);	
  			sprintf(temp2,"%02X",msgid);
			strcat(dump,temp2);

			to_base64(gid,temp);
			pad_b64(1,temp,temp2);
			temp2[1] = '\0'; // append null
			strcat(dump,temp2);

			to_base64(somsr,temp);
			pad_b64(3,temp,temp2);
			strcat(dump,temp2);

			break;
		}
		case 3:{

			tmp = compute_axis(incoming.data.byte[4],incoming.data.byte[3]);
  			
  			sprintf(temp2,"%02X",msgid);
			strcat(dump,temp2);

			to_base64(gid,temp);
			pad_b64(1,temp,temp2);
			temp2[1] = '\0'; // append null 
			strcat(dump,temp2);

			to_base64(tmp,temp);
			pad_b64(2,temp,temp2);
			strcat(dump,temp2);			
			break;
		}
	}
	strncat(dump,delim,1); //place delimiter
	return;
}

String b64_timestamp(String timestamp){
	char temp[2] = {};
	char temp2[2] = {};
	char b64_ts[6] = {};
	int value = 0;
	for (int i=0; i<= 5; i++){ // year, month, day, hours, mins, secs

		value = timestamp.substring(i*2,(i*2)+2).toInt();
		to_base64(value,temp);
		pad_b64(1,temp,temp2);
		strcat(b64_ts,temp2);
	}
	return String(b64_ts);
}

/* 
  Function: b64_process_g_temp_dump

    Process g_string:

    - Separate the main string by the delimiter "+".

    - Convert the uid ( universal id ) to the gid ( geographic id ).

    - Successful conversion of uid to gid : delimiter "-"

    - Failed conversion of uid to gid : delimiter "="

    - Write in *<g_final_dump>*

        Format of data in *<g_string_proc>*:
    
        delimiter - 1 char
        gids - 4 chars
        data - 16 chars

        *[delimiter][gids][data]*

  Parameters:

    dump - pointer to source char array to be processed.

    final_dump - pointer to char array where to write the processed char array.

  Returns:

    n/a

  See Also:

    <get_data>
*/
void b64_process_g_temp_dump(char* dump, char* final_dump, char* no_gids_dump){
  char *token,*last_char;
  char temp_msgid[2], temp_data[11];
  int counter = 0,msgid = 0,dlength=0;

  token = strtok(dump, "-");
  while(token != NULL){
  	if (counter == 0 ) {
		strncpy(temp_msgid,token,2);
		temp_msgid[2] = '\0';
		msgid = strtol(temp_msgid,&last_char,16);
		struct data_type_params dtype = b64_identify_params(msgid);
		dlength = dtype.data_length;
		// dlength = b64_identify_params(msgid,"data_length");
		Serial.println(msgid);

		sprintf(temp_data,token);
		Serial.println(temp_data);
		strncat(final_dump,temp_data,dlength);
		counter = 1;
  	} else {
  		strncpy(temp_data,token+2,9);
  		temp_data[9] = '\0';
  		Serial.println(temp_data);
  		strncat(final_dump,temp_data,dlength-2);
  	}
    token = strtok(NULL,"-");
  } 
  strncat(final_dump,g_delim,1); // add delimiter for different data type
}

void b64_build_text_msgs(char mode[], char* source, char* destination){
	String b64_ts;
	int token_length,msgid,cutoff,num_text_per_dtype,name_len;
	int c=0,char_cnt=0,num_text_to_send=0;
	char *token1,*token2,*last_char;
	char temp_msgid[2];
	char dest[5000] = {};
	char temp[6];
	char pad[12] = "___________";
	char master_name[8] = "";
	char Ctimestamp[6] = "";
	char identifier[3] = {};

	for (int i = 0; i < 5000; i++) {
	  destination[i] = '\0';
	}

	b64_ts = b64_timestamp(g_timestamp);
	for (int i = 0; i < 6; i++) {
		Ctimestamp[i] = b64_ts[i];
	}
	Ctimestamp[12] = '\0';
	token1 = strtok(source, g_delim);
	while ( token1 != NULL){
		// get first 2 chars of token1, they are the hex msgid that will determine
		token_length = strlen(token1)-2;
		// determine identifier ( same as b16 identifiers)
		// convert the string to integer
		strncpy(temp_msgid,token1,2);
		temp_msgid[2] = '\0';
		msgid = strtol(temp_msgid,&last_char,16);

		struct data_type_params dtype = b64_identify_params(msgid);
		strncpy(identifier,dtype.identifier,2);
		// determine number of messages to be sent per data type
		cutoff = dtype.type_cutoff;
		num_text_per_dtype = ( (strlen(token1) - 2) / cutoff );
		if ((strlen(token1) % cutoff) != 0){
			num_text_per_dtype++;
		}
		// determine mastername
	    if (g_sensor_version == 1){
	        name_len = 8;
	        strncpy(master_name,g_mastername,4);
	        strncat(master_name,"DUE",4);
	    } else {
	        name_len = 6;
	        strncpy(master_name,g_mastername,6);
	    }
		// copy parts of the message
		//increment token1 twice to skip over the HEX msgid
		c=0;

		for (int i = 0; i < num_text_per_dtype; i++){		
			strncat(dest,pad,11);
			strncat(dest,master_name, name_len);
			strncat(dest,"*", 2);
			// if (idf != 'p'){ // except piezo
				strncat(dest,identifier,2);
				strncat(dest,"*", 2);
			// }
			for (int j=0; j <= (cutoff); j++ ){
				strncat(dest,token1+2,1);
				c++;
				token1++;
				if (c == (token_length)){
				  break;
				}
			}
			if (strcmp(comm_mode,"XBEE") == 0){
			// Baka dapat kapag V3 ito. 
				strncat(dest,"*",1);
				strncat(dest,Ctimestamp,6);
			}
			strncat(dest,"<<",2);
			strncat(dest,g_delim,1);
		}
		Serial.print("chars written: ");
		Serial.println(c);
		num_text_to_send = num_text_to_send + num_text_per_dtype;
		token1 = strtok(NULL, g_delim);
	}
	token2 = strtok(dest, g_delim);
	c=0;
	while( token2 != NULL ){
		c++;
		char_cnt = strlen(token2) + name_len - 24;
		sprintf(pad, "%03d", char_cnt);
		strncat(pad,">>",3);
		sprintf(temp, "%02d/", c);
		strncat(pad,temp,4);
		sprintf(temp,"%02d#",num_text_to_send);
		strncat(pad,temp,4);
		strncpy(token2,pad,11);
		// strncat(token2,"<<",3);
		Serial.println(token2);
		strncat(destination,token2, strlen(token2));
		strncat(destination, g_delim, 2);
		token2 = strtok(NULL, g_delim);
	}

}

