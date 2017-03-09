#include "Common.h"

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library


OneWire  ds(DS_PIN);  // (a 4.7K termination resistor is necessary)
DallasTemperature sensors(&ds);// Pass our oneWire reference to Dallas Temperature. 

float currentTemp;

void InitTempSensors()
{
  sensors.begin();

    // locate devices on the bus
  Serial.print("Locating temperature sensors...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" sensors.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  sensors.setResolution(9);
}

float ReadTempSensors()
{
  int index = 0;
  uint8_t sensorAddr[8];
  float sum = 0;
  float avgTemp = 0;
  int numSensors = 0;
  
  uint32_t startTime = millis();
  sensors.requestTemperatures();
  
  while (sensors.getAddress(sensorAddr, index))
  {
    sum += sensors.getTempF(sensorAddr);
    index++;
  }
  numSensors = index;
  
  Serial.print("sensor sampling time: "); Serial.print(millis() - startTime); Serial.println("ms");
  
  if (0 != numSensors)
    avgTemp = (sum / numSensors);
  else
    avgTemp = 100.0;  //if no sensors, make sure heat doesn't come on.

  Serial.print("  Temperature = ");
  Serial.print(avgTemp);
  Serial.println(" Fahrenheit");
        
  return avgTemp;
}

//float otherReadTempSensors()
//{
//  float sum = 0;
//  float avgTemp = 0;
//  uint32_t startTime = millis();
//  sensors.requestTemperatures();
//  int numSensors = sensors.getDeviceCount();
//  for (int i = 0; i < numSensors; i++)
//  {
//    sum += sensors.getTempFByIndex(i);
//  }
//
//  Serial.print("sensor sampling time: "); Serial.print(millis() - startTime); Serial.println("ms");
//  
//  if (0 != numSensors)
//    avgTemp = (sum / numSensors);
//  else
//    avgTemp = 100.0;  //if no sensors, make sure heat doesn't come on.
//
//  Serial.print("  Temperature = ");
//  Serial.print(avgTemp);
//  Serial.println(" Fahrenheit");
//        
//  return avgTemp;
//}

//float oldReadTempSensors()
//{
//  byte i;
//  byte present = 0;
//  byte type_s;
//  byte data[12];
//  byte addr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
//
//  float celsius, fahrenheit;
//  uint8_t sensorNumber = 0;
//  int sensorCount = 0;
//  int sum = 0;
//
//  while (ds.search(addr))
//  {
//    Serial.print("ROM =");
//    for ( i = 0; i < 8; i++) {
//      Serial.print(" ");
//      Serial.print(addr[i], HEX);
//    }
//
//    if (OneWire::crc8(addr, 7) != addr[7]) {
//      Serial.println("CRC is not valid!");
//    }
//    else
//    {
//      //Serial.println();
//      bool skipSensor = false;
//      // the first ROM byte indicates which chip
//      switch (addr[0]) {
//        case 0x10:
//          //Serial.println("  Chip = DS18S20");  // or old DS1820
//          type_s = 1;
//          break;
//        case 0x28:
//          //Serial.println("  Chip = DS18B20");
//          type_s = 0;
//          break;
//        case 0x22:
//          //Serial.println("  Chip = DS1822");
//          type_s = 0;
//          break;
//        default:
//          //Serial.println("Device is not a DS18x20 family device.");
//          skipSensor = true;
//      }
//      if (!skipSensor)
//      {
//        ds.reset();
//        ds.select(addr);
//        ds.write(0x44, 1);        // start conversion, with parasite power on at the end
//
//        delay(1000);     // maybe 750ms is enough, maybe not
//        // we might do a ds.depower() here, but the reset will take care of it.
//
//        present = ds.reset();
//        ds.select(addr);
//        ds.write(0xBE);         // Read Scratchpad
//
//        //Serial.print("  Data = ");
//        //Serial.print(present, HEX);
//        //Serial.print(" ");
//        for ( i = 0; i < 9; i++) {           // we need 9 bytes
//          data[i] = ds.read();
//          //Serial.print(data[i], HEX);
//          //Serial.print(" ");
//        }
//        //Serial.print(" CRC=");
//        //Serial.print(OneWire::crc8(data, 8), HEX);
//        //Serial.println();
//
//        // Convert the data to actual temperature
//        // because the result is a 16 bit signed integer, it should
//        // be stored to an "int16_t" type, which is always 16 bits
//        // even when compiled on a 32 bit processor.
//        int16_t raw = (data[1] << 8) | data[0];
//        if (type_s) {
//          raw = raw << 3; // 9 bit resolution default
//          if (data[7] == 0x10) {
//            // "count remain" gives full 12 bit resolution
//            raw = (raw & 0xFFF0) + 12 - data[6];
//          }
//        } else {
//          byte cfg = (data[4] & 0x60);
//          // at lower res, the low bits are undefined, so let's zero them
//          if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
//          else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
//          else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
//          //// default is 12 bit resolution, 750 ms conversion time
//        }
//        sum += raw;
//        sensorCount++;
//
//        celsius = (float)raw / 16.0;
//        fahrenheit = celsius * 1.8 + 32.0;
//
//        Serial.print("  Temperature = ");
//        //Serial.print(celsius);
//        //Serial.print(" Celsius, ");
//        Serial.print(fahrenheit);
//        Serial.println(" Fahrenheit");
//      }
//    }
////    LCDUpdateDisplay();  //kludge.  ReadTempSensors blocks for about 1 second per sensor.  This call keeps the colon blinking in the time display.
//  }
//  
//  //Serial.println("No more addresses.");
//  //Serial.println();
//  ds.reset_search();
//  if (0 != sensorCount)
//  {
//    float avgCelsius = (sum / sensorCount) / 16.0;
//    float avgFahrenheit = avgCelsius * 1.8 + 32.0;
//    return avgFahrenheit;
//  }
//  else
//    return 100.0;  //if no sensors, make sure heat doesn't come on.
//}


// the following calls are used by the Settings module for the tempList command
void TempStartConversion()
{
  sensors.requestTemperatures();
}

bool GetTempSensorAddress(uint8_t* buffer, int index)
{
  return sensors.getAddress(buffer, index);
}

float GetSensorTemperature(int index)
{
  return sensors.getTempFByIndex(index);
}

