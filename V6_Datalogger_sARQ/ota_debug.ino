/*  USED BY LAN OTA FIRMWARE UPDATES FUNCTIONS */
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Update.h>

const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* mqttTopic = "test/topic";


void initWifiOTAConnection() {
  const char* ssid = "SARQ_ESP_WIFI";
  const char* password = "espwifipass"; 
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("WiFi init OK");
}

void setupWiFi(const char * wifiSSID, const char * wifiPass) {
  bool wifiSearch = true;
  unsigned long setupStart = millis();
  unsigned long setupTimeout = 20000; // 10 sec timout
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true); // Ensure STA is idle before config
  delayMillis(10);

  IPAddress local_IP(192,168,1,184);
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);
  WiFi.begin(wifiSSID, wifiPass);
  debugPrint("Searching for WiFi ..");
  while (WiFi.status() != WL_CONNECTED && wifiSearch) {
    delayMillis(500);
    Serial.print(".");
    if ((millis() - setupStart) >= setupTimeout) {
      setupStart = millis();
      debugPrintln("WiFI search timeout");
      wifiSearch = false; 
      return; // return for now
    }

  }
  Serial.println("WiFi Connected");
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

/*
temporary broker
Host:	broker.hivemq.com
TCP Port:	1883
Websocket Port:	8000
TLS TCP Port:	8883
TLS Websocket Port:	8884
*/

void sendThruWifi(const char * payload) {

}

// bool connectMQTT() {

//   client.setServer(mqttServer, mqttPort);

//   // Fixed client ID keyword
//   char clientId[50];
//   char dLogNameBuf[10];
//   getNameFromList(0, dLogNameBuf);
//   snprintf(clientId, sizeof(clientId), "ESPMQTT+");

//   if (client.connect(clientId)) {
//     Serial.print("Connected to MQTT broker with ID: ");
//     Serial.println(clientId);
//     return true;
//   } else {
//     Serial.println("MQTT connection failed");
//     return false;
//   }
// }