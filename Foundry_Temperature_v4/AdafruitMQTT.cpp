#include "Common.h"


#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "adafruit_support_rick"
#define AIO_KEY         "feb16361d6d145149e98d31f7d0da75d"
//#define AIO_USERNAME    "kfl"
//#define AIO_KEY         "bc273dc3882941ddbc67b6b0928a8d55"

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Setup a feed called 'button' for publishing changes.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
//const char TEMP1[] PROGMEM = AIO_USERNAME "/feeds/Foundry-temp1";
Adafruit_MQTT_Publish temp1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Foundry-temp1");
//const char TEMP2[] PROGMEM = AIO_USERNAME "/feeds/Foundry-temp2";
Adafruit_MQTT_Publish temp2 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Foundry-temp2");
//
Adafruit_MQTT_Publish* tempSensorList[] = {&temp1, &temp2};

typedef struct SensorMap_t
{
  byte romAddr[8];
  int sensorNumber;
  char description[128];
  Adafruit_MQTT_Publish *feed;
};

SensorMap_t sensorMap[] = {
  {{0x28, 0x20, 0x69, 0x5E, 0x07, 0x00, 0x00, 0xFD}, 0, AIO_USERNAME "/feeds/Foundry-temp1", NULL},
  {{0x28, 0x97, 0x35, 0xF9, 0x07, 0x00, 0x00, 0xE4}, 1, AIO_USERNAME "/feeds/Foundry-temp2", NULL}
};

#define NUM_SENSORS (sizeof(sensorMap)/sizeof(SensorMap_t))

void InitAdafruitMQTT()
{
  //initialize sensor map
  for (int s = 0; s < NUM_SENSORS; s++)
  {
    sensorMap[s].feed = new Adafruit_MQTT_Publish(&mqtt, sensorMap[s].description);
  }
}

// connect to adafruit io via MQTT
void ConnectAdafruitMQTT() {

  Serial.print(F("Connecting to Adafruit IO... "));

  int8_t ret;

  while ((ret = mqtt.connect()) != 0) {

    switch (ret) {
      case 1: Serial.println(F("Wrong protocol")); break;
      case 2: Serial.println(F("ID rejected")); break;
      case 3: Serial.println(F("Server unavail")); break;
      case 4: Serial.println(F("Bad user/pass")); break;
      case 5: Serial.println(F("Not authed")); break;
      case 6: Serial.println(F("Failed to subscribe")); break;
      default: Serial.println(F("Connection failed")); break;
    }

    if(ret >= 0)
      mqtt.disconnect();

    Serial.println(F("Retrying connection..."));
    delay(5000);

  }

  Serial.println(F("Adafruit IO Connected!"));

}

void CheckAdafruitMQTT()
{
  // ping adafruit io a few times to make sure we remain connected
  if(! mqtt.ping(3)) {
    // reconnect to adafruit io
    if(! mqtt.connected())
      ConnectAdafruitMQTT();
  }
}

//void FeedAdafruitMQTT(float fahrenheit)
//{
//  int s = 0;
//  bool found = false;
//  while ((!found) && (s < NUM_SENSORS))
//  {
//    if (0 == memcmp(addr, sensorMap[s].romAddr, 8))
//    {
//      found = true;
//      if (! sensorMap[s].feed->publish(fahrenheit))
//        Serial.println(F("Failed."));
//      else
//        Serial.println(F("Success!"));
//    }
//    s++;    
//  }
//}

void FeedAdafruitMQTT(float fahrenheit)
{
  if (! sensorMap[0].feed->publish(fahrenheit))
    Serial.println(F("Failed."));
  else
    Serial.println(F("Success!"));
}
