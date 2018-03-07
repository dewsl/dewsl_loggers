
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
	// Serial.print("current dest length: ");
	// Serial.println(strlen(dest));
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

