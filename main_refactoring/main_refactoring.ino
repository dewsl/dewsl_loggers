/**
 * @file main_refactoring.ino
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


/**  
 * Delay (in milliseconds)
 * 
 * Used in #columnOn and #columnsOff
 */
#define MSDELAY 100

/// demo docu for #setup()
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  canInit();
  columnOn();
  canSend(51);
}

/// banana
void loop() {
  columnOn();
  delay(MSDELAY);
  canSend(51);
  canReceive(2000);
  columnOff();
  delay(MSDELAY);
}

/** 
 * demo docu for "function1"
 * @param[in] var1 this is var1
 * @param[in] var2 this is var2
 * @param[out] none
*/
void function1(int var1, char var2){
  //
}
