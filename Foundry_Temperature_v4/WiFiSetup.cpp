
#include "Common.h"

// Create an ESP8266 WiFiClient class.
WiFiClient client;

#define WLAN_SSID       "BuffaloLab"
#define WLAN_PASS       "M4k3Stuff"

void WiFiInit(void) {
  Serial.print(F("Connecting to "));
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();

  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());
  LCDDisplayIPAddress(WiFi.localIP().toString().c_str());
}


