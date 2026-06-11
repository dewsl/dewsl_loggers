/*  USED BY LAN OTA FIRMWARE UPDATES FUNCTIONS */
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Update.h>


void initWifiOTAConnection() {
  const char* ssid = "Selah";
  const char* password = "ilovehubby"; 
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("WiFi init OK");
}

bool wifiConnectedStatus() {
  bool connectedStat = false;
  if (WiFi.status() == WL_CONNECTED) connectedStat = true;
  return connectedStat;
}

void disableWifi() {
  WiFi.mode(WIFI_OFF);
}

void OTAProc(unsigned long handlerDuration) {

  ArduinoOTA.setPassword("dynaslope");
  ArduinoOTA.onStart([]() {Serial.println("Starting OTA update process... ");})
            .onEnd([]() {Serial.println("\nUpdate Finished");})
            .onProgress([](unsigned int progress, unsigned int total) {Serial.printf("Progress: %u%%\n", (progress / (total / 100)));})
            .onError([](ota_error_t error) {
              Serial.printf("Error[%u]: ", error);
              if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
              else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
              else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
              else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
              else if (error == OTA_END_ERROR) Serial.println("End Failed");
            });
  ArduinoOTA.begin();
 
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  unsigned long handlerStart = millis();
  while ((millis() - handlerStart) < handlerDuration) {
      ArduinoOTA.handle();
  }
}