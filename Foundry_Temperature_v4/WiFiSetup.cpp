
#include "Common.h"

// Create an ESP8266 WiFiClient class.
WiFiClient client;

bool WiFiInit(void) {
  int retries = 40;
  bool connected = false;

  Serial.print(F("Connecting to "));
//  Serial.println(WLAN_SSID);
  Serial.println(GetSSID());

//  WiFi.begin(WLAN_SSID, WLAN_PASS);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(GetSSID(), GetPassword());
  while ((retries--) && (WiFi.status() != WL_CONNECTED)) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  connected = (WiFi.status() == WL_CONNECTED);
  if (connected)
  {
    Serial.println(F("WiFi connected"));
    Serial.println(F("IP address: "));
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println(F("WiFi NOT connected.  Will try again in 1 minute"));
  }
  LCDDisplayIPAddress(WiFi.localIP().toString().c_str());
  return connected;
}

