
/* 
  Function: to_base64

    Convert integers to a padded base64 number system.

  Parameters:

    input - integer to be converted
    dest - pointer to char array where the converted input will be stored
    length - number of resulting characters from the conversion. If
    length > character count of result, the result will be padded with 'A'
    (which represent 0 in base64).

  Returns:

    n/a

  See Also:

    <process_g_temp_dump>
*/
void to_base64(int input, char* dest){
	int result = 0;
	// int index1 = 0;
	// int index2 = 0;
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
		strncpy(dest,input,strlen(input));
	}
}


/* 
  Function: b64_write_frame_to_dump

    Write the frames as b64 encoded char array.
    This differs from *<write_frame_to_dump>* primarily because this function converts the uids to gids.
	
	Format:
	gids - 1 char
	msgid - 2 chars
	data - 14 chars

	*[gids][msgid][data]*

	The format for data varies depending on msgid.

  Parameters:

    incoming- CAN_FRAME struct to be written in dump
    dump - pointer to char buffer

  Returns:

    n/a

  See Also:

    <process_g_temp_dump>
*/
void b64_write_frame_to_dump(CAN_FRAME incoming, char* dump){
  char temp[5] = {};
  char temp2[5] = {};
  int gid,msgid,x,y,z,v;
  gid = convert_uid_to_gid(incoming.id);
  msgid = incoming.data.byte[0];

  // x = compute_axis(incoming.data.byte[1],incoming.data.byte[2]);
  // y = compute_axis(incoming.data.byte[3],incoming.data.byte[4]);
  // z = compute_axis(incoming.data.byte[5],incoming.data.byte[6]);
  // v = incoming.data.byte[7];

  to_base64(gid,temp);
  pad_b64(1,temp,temp2);
  strcat(dump,temp2);

  to_base64(msgid,temp);
  pad_b64(2,temp,temp2);
  strcat(dump,temp2);

  to_base64(incoming.data.byte[1],temp);
  pad_b64(2,temp,temp2);
  strcat(dump,temp2);

  to_base64(incoming.data.byte[2],temp);
  pad_b64(2,temp,temp2);
  strcat(dump,temp2);

  to_base64(incoming.data.byte[3],temp);
  pad_b64(2,temp,temp2);
  strcat(dump,temp2);

  to_base64(incoming.data.byte[4],temp);
  pad_b64(2,temp,temp2);
  strcat(dump,temp2);

  to_base64(incoming.data.byte[5],temp);
  pad_b64(2,temp,temp2);
  strcat(dump,temp2);

  to_base64(incoming.data.byte[6],temp);
  pad_b64(2,temp,temp2);
  strcat(dump,temp2);

  to_base64(incoming.data.byte[7],temp);
  pad_b64(2,temp,temp2);
  strcat(dump,temp2);

  interpret_frame(incoming);
  return;
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
  char temp_id[5],temp_gid[5],temp_data[17];
  int id_int,gid;
  token = strtok(dump, "-");

  while(token != NULL){
    // get gid
    strncpy(temp_id,token,4);
    id_int = strtol(temp_id,&last_char,16);
    gid = convert_uid_to_gid(id_int);

    if ( (gid != 0) & (gid != -1) ){
      sprintf(temp_gid,"%04X",gid);
      strncpy(temp_data,token+4,16);
      // strncat(final_dump,"-",1);
      strncat(final_dump,temp_gid,4);
      strncat(final_dump,temp_data,16);
      
    } else if (gid == 0) {
      strncat(no_gids_dump,"=",1);
      strncat(no_gids_dump,token,20);
    }
    token = strtok(NULL,"-");
  } 
  strncat(final_dump,g_delim,1); // add delimiterfor different data type
}

