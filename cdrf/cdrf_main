//sample print

const byte numChars = 32;
char receivedChars[numChars];   // an array to store the received data
boolean newData = false;

void setup()
{
   Serial.begin(9600);
}

void loop()
{
   recvWithEndMarker();
   showNewData();
}

void recvWithEndMarker()
{
   static byte ndx = 0;
   char endMarker = '\n';
   char rc;

   while (Serial.available() > 0 && newData == false)
   {
      rc = Serial.read();

      if (rc != endMarker)
      {
         receivedChars[ndx] = rc;
         ndx++;
         if (ndx >= numChars)
         {
            ndx = numChars - 1;
         }
      }
      else
      {
         receivedChars[ndx] = '\0'; // terminate the string
         ndx = 0;
         newData = true;
      }
   }
}

void showNewData()
{
  char temp[3];
  int cmd = 0;
   if (newData == true)
   {
      if (strcmp(receivedChars,"send")>= 0){
        Serial.println("word found");
      }
      temp[0] = receivedChars[4];
      temp[1] = receivedChars[5];
      temp[2] = '\0';

      Serial.println(atoi(strchr(receivedChars,'d') + 1));
      /*cmd = atoi(temp);
      Serial.print("cmd = ");
      Serial.println(cmd);*/

      // int num1 = int(firstNum - '0');
      // int num2 = int(secondNum - '0');

      // Serial.println (num1);
      // Serial.println (num2);
      // Serial.println (receivedChars);

      delay(1000);
      newData = false;
   }
}
