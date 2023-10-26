/**
 * @file can.ino
 * contains functions for the controller area network ( CAN )
 * 
 */

#include <due_can.h>
#include "variant.h"

/** 
 * CAN Bus Baudrate.
 */
#define CAN_BAUD_RATE 40000

/**  
 * Enable Pin for Due / SAM3X board 
 */
#define CAN0_EN 50

/**  
 * GPIO for controlling the column switch
 */
#define COLUMN_SWITCH_PIN 44

/**  
 * GPIO for controlling the LED indicator for the column switch
 */
#define LED1 48

/** 
 * @brief Set the CAN baudrate, CAN Enable Pin. 
 * 
 * Set the CAN baudrate for Can0 to #CAN_BAUD_RATE. 
 * 
 * Set mailbox0 as receiver.
 * 
 * Set mailbox0 to receive the extended ID format.
 * 
 * Set mask to recieve all. 
 * 
 * Set mailbox1 transfer ID to extended ID format. 
 * 
 * Set GPIO #COLUMN_SWITCH_PIN to output mode.
 * 
 * Set GPIO #LED1 to output mode.
 * 
 * Write LOW to #COLUMN_SWITCH_PIN. 
 * 
 * \note CAN Enable Pin for Can0 -> 50
 * \note CAN Enable Pin for Can1 -> 48
 * \source
 * \return Return false on failure of setting CAN_BAUD_RATE.
*/
bool canInit(){
  pinMode(CAN0_EN, OUTPUT);
  if (!Can0.begin(CAN_BAUD_RATE, CAN0_EN)) {
    return false;
  } 
  Can0.watchFor();
  Can0.mailbox_set_mode(0, CAN_MB_RX_MODE);  // Set mailbox0 as receiver
  Can0.mailbox_set_id(0, 0, true);           // Set mailbox0 receive extendedID formats
  Can0.mailbox_set_accept_mask(0, 0, true);  // receive everything. // aralin yung mask
  Can0.mailbox_set_mode(1, CAN_MB_TX_MODE);  // Set mailbox1 as transmitter
  Can0.mailbox_set_id(1, 1, true);           // Set mailbox1 transfer ID to 1 extended id

  pinMode(COLUMN_SWITCH_PIN,OUTPUT);
  pinMode(LED1,OUTPUT);
  digitalWrite(COLUMN_SWITCH_PIN, LOW);
  return true;
}

/** 
 * @brief Send a CAN message via Can0. 
 * 
 * @param[in] command integer sent to sensor column
 * 
 * Create a struct CAN_FRAME outgoing.
 * 
 * Set the the extended flag for the CAN_FRAME to true.
 * 
 * Set the outgoing id to 1.
 * 
 * Set lenth of the frame to 1 and the first data byte to command.
 * (subsurface sensors only take the first byte as command)
 * 
 * \return N/A
*/
void canSend(int command){
  CAN_FRAME outgoing;
  outgoing.extended = true;
  outgoing.id = 1;
  outgoing.length = 1;
  outgoing.data.byte[0] = command;
  Can0.sendFrame(outgoing);
}

/** 
 * @brief Power on the sensor column
 * 
 * Set #COLUMN_SWITCH_PIN and #LED1 to HIGH. Also implement a delay defined by #MSDELAY.
 *  
 * @param[in] none
 * @return none
*/
void columnOn(){
  digitalWrite(COLUMN_SWITCH_PIN, HIGH);
  digitalWrite(LED1, HIGH);
  delay(MSDELAY);
}

/** 
 * @brief Power off the sensor column
 * 
 * Set #COLUMN_SWITCH_PIN and #LED1 to LOW. Also implement a delay defined by #MSDELAY.
 * @param[in] none
 * @return none
*/
void columnOff(){
  digitalWrite(COLUMN_SWITCH_PIN,LOW);
  digitalWrite(LED1, LOW);
  delay(MSDELAY);
}

/** 
 * @brief Receive CAN messages via Can0 until the timeout (in milliseconds) expires.
 * 
 * @param[in] timeout unsigned 16 bit integer that specifies the wait time (in milliseconds) for incoming CAN_FRAMES
 * 
 * Initialize an unsigned long as start to mark the runtime start of the function. 
 * 
 * Create a struct CAN_FRAME incoming.
 * 
 * Create an integer read_frames and set to 0;
 * 
 * Receive frames as long as the specified timeout is not reached and increment read_frames. 
 * 
 * \return unsigned 8 bit integer read_frames equal to the number of frames read
 * within the elapsed timeout 
*/
uint8_t canReceive(uint16_t timeout){
  unsigned long start = millis();
  CAN_FRAME incoming;
  uint8_t read_frames = 0;
  while ( millis() - start < timeout){
    if (Can0.available()){
      Can0.read(incoming);
      Serial.print("ID: ");
      Serial.print(incoming.id);
      for (int i = 0; i<8; i++){
        Serial.print(" ");
        Serial.print(incoming.data.byte[i],HEX);
      }
      Serial.println();
      read_frames++;
    } 
  }
  return read_frames;
}
