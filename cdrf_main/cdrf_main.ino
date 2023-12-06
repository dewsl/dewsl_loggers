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
#include <due_can.h>
#include "variant.h"
/**  
 * Delay (in milliseconds)
 * 
 * Used in #columnOn and #columnsOff
 */
#define MSDELAY 100

// #define LIB_LOGGER_COUNT 79;
/**
 * Char array for name of subsurface sensor.
 * Defaults to "XXXXX"
 */
char MASTERNAME[6] = "XXXXX"; 


/**  
 * 2 dimensional integer array that stores the links the unique id with the geographic id
 *  
 */
int GIDS[2][40];

/**  
 * Unsigned global 8 bit integer indicating sensor version ( 1 - 5 )
 *  
 */
uint8_t SENSORVERSION;

/**  
 * Unsigned global 8 bit integer indicating datalogger version ( 1 - 5 )
 *  
 */
uint8_t DATALOGGERVERSION;

/**
* Struct containing configuration for subsurface sensor
*
*/
libConfig CONFIG;

const byte numChars = 32;
char receivedChars[numChars];   // an array to store the received data
boolean newData = false;

void setup()
{
   Serial.begin(115200);
   canInit();
}

void loop()
{
    // recvWithEndMarker();
    // showNewData();
    // loadConfig("MNGSA");
    // printConfig();
    delay(4000);
    b64_timestamp("231206102100");
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
                // Serial.println(MASTERNAME);
            } else if (strlen(receivedChars) == 11)
            {
                strncpy(MASTERNAME,receivedChars+5,6);
                // Serial.println(MASTERNAME);
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
bool loadConfig(char* mastername){
    for (int i=0; i < LIB_LOGGER_COUNT; i++)
    {
        if ( strstr(config_container[i].lib_mastername, mastername) )
        {
            CONFIG = config_container[i];
            // Serial.print(config_container[i].lib_mastername);
            // Serial.println(" loaded to CONFIG!");
            return true; 
        }
    }
    Serial.print("No CONFIG for:");
    Serial.println(mastername);
    return false;
}

void printConfig(){
    SENSORVERSION = CONFIG.lib_sensor_version;
    if (SENSORVERSION >= 1)
    {
        strncpy(MASTERNAME,CONFIG.lib_mastername,5);
    } 
    else if (CONFIG.lib_sensor_version == 1) 
    {
        strncpy(MASTERNAME,CONFIG.lib_mastername,4);
    }
    // check MASTERNAME
    if (MASTERNAME[3] == 'S' || MASTERNAME[3] == 'T' || MASTERNAME[3] == 'B')
    {
        Serial.println(MASTERNAME);
    }  
    parseGids();
    displayGIDS();
}

void parseGids(){
    char *p;
    char tmp[250];
    int a = 0, b=0;
    strncpy(tmp,CONFIG.lib_column_ids,strlen(CONFIG.lib_column_ids) + 1);
    p = strtok(tmp,",");
    for (b=0; b<40; b++)
    {
        if (p==NULL)
        {
            break;
        } 
        GIDS[0][a] = atoi(p);
        GIDS[1][a] = b+1;
        if(strlen(p) > 0)
        {
            a++;
        }
        p = strtok(NULL, ",");
    }
    if ( b == CONFIG.lib_num_of_nodes) {
       Serial.println("Same number of nodes listed on CONFIG and count of nodes.");
    }
}

void displayGIDS(){
    char display[40],temp[5], temp2[3];
    display[0] = '\0';
    Serial.println("===============================");
    for (int a = 0; a < CONFIG.lib_num_of_nodes; a++){
        strncat(display,"\t", 3);
        snprintf(temp2,3,"%02d",GIDS[1][a]);
        snprintf(temp,5,"%04d",GIDS[0][a]);
        strncat(display,temp2,3);
        strncat(display,"\t",2);
        strncat(display,temp,5);
        Serial.println(display);
        display[0] = '\0';
    }
}