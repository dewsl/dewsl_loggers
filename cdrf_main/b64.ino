/**
 * @file b64.ino
 * contains functions for data compression and packing
 * 
 */

/**
 * int array containing list of all valid subsurface message ids.
 * 
 * \note 11,12,32,33,41,42,51,52 covers version 2 to 5 subruface sensors
 * 
 */
const int validMsgIds[8] = {11,12,32,33,41,42,51,52};

/**
 * char array containing list of all characters used for b64 compression.
 * 
 */

const char PROGMEM base64[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                                  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                                  'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2',
                                  '3', '4', '5', '6', '7', '8', '9', '+', '/' };

struct ssData{
  uint8_t gid;
  unsigned int uid;
  int16_t x;
  int16_t y;
  int16_t z;
  float vBefore;
  float vAfter;
  float vNode;
} axelData[40];


/** 
 * @brief Check for valid message ids from tilt sensors
 * 
 * @param[in] msgId integer to be checked for validity
 * 
 * Checks the size of #validMsgIds. 
 * 
 * Runs a loop to compare msgIds with each element in #valdiMsgIds.
 *  
 * 
 * \return Return false if msgId is not found in #validMsgIds and return true as soon a valid comparison is made. 
 * 
*/
bool checkValidMsgId(int msgId){
        int size = sizeof(validMsgIds);
        for (int i = 0; i<size; i++){
            if (msgId == validMsgIds[i]){
                return true;
            }
        }
        return false;
}

void interpretFrame(CAN_FRAME incoming, int sensorType){
        int id, d1, d2, d3, d4, d5, d6, d7, d8, somsr, temper;
        int16_t x,y,z;
        float v;
        char temp[150];

        id = incoming.id;
        d1 = incoming.data.byte[0];
        d2 = incoming.data.byte[1];
        d3 = incoming.data.byte[2];
        d4 = incoming.data.byte[3];
        d5 = incoming.data.byte[4];
        d6 = incoming.data.byte[5];
        d7 = incoming.data.byte[6];
        d8 = incoming.data.byte[7];
        
        if (sensorType == 1) // tilt
        {
                if (checkValidMsgId(d1))
                {
                    x = (d3 << 8) | d2;
                    y = (d5 << 8) | d4;
                    z = (d7 << 8) | d6;
                    v = ((d8 + 200.0) / 100.0);
                } else 
                {
                    Serial.print("msgId (d1) : ");
                    Serial.println(d1);
                }
        }
        // imbis na print ay isulat sa buffer na naka b64 format
        Serial.print(id);
        Serial.print("\t");
        Serial.print("x:");
        Serial.print(x);
        Serial.print("\ty:");
        Serial.print(y);
        Serial.print("\tz:");
        Serial.println(z);
}

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
void to_base64(int input, char* dest) {
  int result = 0;
  int div_res, mod_res;
  int in = input;
  char temp[2] = {};
  div_res = in / 64;
  mod_res = in % 64;
  dest[0] = '\0';  // iinitialize yung dest para strcat na lang lagi ang pagsulat
  if (input == 0) {
    sprintf(temp, "%c", base64[0]);
    strcat(dest, temp);
  }
  while (((mod_res > 0) && (div_res >= 0)) || ((mod_res == 0) && (div_res > 0))) {

    sprintf(temp, "%c", base64[mod_res]);
    strcat(dest, temp);
    in = div_res;
    mod_res = in % 64;
    div_res = in / 64;
    if ((mod_res == 0) && (div_res == 0)) {
      break;
    }
  }
  reverse_char_order(dest, strlen(dest));
  // pad here
}

/*
  Function: b64_timestamp

  Converts a 12 character String timestamp with format `yymmddhhmmss` to
  b64 encoded 6 character timestamp with format `ymdhms`

  Parameters:

    timestamp - String object with format yymmddhhmmss

  Returns:

    Timestamp as a String object with format ymdhms.

  See Also:

    <get_data>
*/

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

    <to_base64>
*/
void pad_b64(uint8_t length_of_output, char* input, char* dest) {
  int num_of_pads = 0;
  char temp[5] = {};
  temp[0] = '\0';  // para makapag strncat ka dahil may null na si temp
  num_of_pads = length_of_output - strlen(input);

  if (num_of_pads > 0) {
    for (int i = 0; i < num_of_pads; i++) {
      strcat(temp, "A");
    }
    strcat(temp, input);
    strncpy(dest, temp, strlen(temp));
  } else {
    strncpy(dest, input, length_of_output);
    // temp[length_of_output] = '\0';
  }
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

    <to_base64>
*/
void reverse_char_order(char* input, uint8_t length) {
  char temp[4] = { 0 };
  char temp1[2] = { 0 };
  temp[0] = '\0';
  for (int i = length - 1; i >= 0; i--) {
    sprintf(temp1, "%c", input[i]);
    strcat(temp, temp1);
  }
  temp[length] = '\0';
  strncpy(input, temp, strlen(temp));
}

char* b64_timestamp(char * timestamp) {
  char temp[3] = {};
  // char temp2[3] = {};
  char b64_ts[6] = {};
  int value = 0;
  for (int i = 0; i <= 5; i++) {  // year, month, day, hours, mins, secs
    strncpy(temp,timestamp+(i*2),2);
    temp[2] = '\0'; 
    value = atoi(temp);
    to_base64(value, temp);
    pad_b64(1, temp, temp2);
    strcat(b64_ts, temp2);
    // 
  }
  Serial.println(b64_ts);
  return b64_ts;
}