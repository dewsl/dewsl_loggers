// #include <BLEDevice.h>
// #include <BLEUtils.h>
// #include <BLEScan.h>
// #include <BLEAdvertisedDevice.h>

void initBluetooth() {

  char btNameContainer[30];
  char dlogName[10];
  getNameFromList(0, dlogName);

  sprintf(btNameContainer, "sARQ_%s_BT", dlogName);

  if (BTSerial.begin(btNameContainer)) Serial.println("Bluetooth ENABLED"); // Bt init  // append datalogger name here
  else Serial.println("Bluetooth initialization ERROR");
}

// what else can we do here?