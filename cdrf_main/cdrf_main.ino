/**
 * @file cdrf_main.ino
 * This runs the setup() and loop(). 
 * 
 */

/**
 * @mainpage 
 * 
 * This is the main page. Dito yung mga hanash niyo tungkol sa project.
 * 
 * Kapag galit kayo dito niyo ilagay.
 * 
 * Ito ay puro comments lang sa code.
 *  
*/

#include "Due_config.h"

/**  
 * Delay (in milliseconds)
 * 
 * Used in #columnOn and #columnsOff
 */
#define MSDELAY 100

/**
 * Char array for name of subsurface sensor.
 * Defaults to "XXXXX"
 */
char MASTERNAME[6] = "XXXXX"; 

const byte numChars = 32;
char receivedChars[numChars];   // an array to store the received data
boolean newData = false;

lib_config cfg;

void setup()
{
   Serial.begin(115200);
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
        if (strstr(receivedChars,"name:"))
        {
            Serial.println(strlen(receivedChars));
            // v1 - 4 chars tapos may \n na kinukuha ng recvWithEndMarker tapos 5 sa "name:"
            if (strlen(receivedChars) == 10)  
            {
                strncpy(MASTERNAME,receivedChars+5,5);
                Serial.println(MASTERNAME);
            } else if (strlen(receivedChars) == 11)
            {
                strncpy(MASTERNAME,receivedChars+5,6);
                Serial.println(MASTERNAME);
            } else 
            {
                Serial.println("Invalid sensor name!");
            }
        } else if (strstr(receivedChars,"send:"))
        {
            Serial.print("send:");
        }
        // if (strcmp(receivedChars,"send")>= 0)
        // {
        //     Serial.print("CMD:");
        //     temp[0] = receivedChars[4];
        //     temp[1] = receivedChars[5];
        //     temp[2] = '\0';

        //     Serial.println(atoi(strchr(receivedChars,'d') + 1));
        // } 
        // else if(strcmp(receivedChars,""))
        // {

        // } 

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


/**
 * Load lib_config for given subsurface sensor
 */
void loadConfig(){

}
