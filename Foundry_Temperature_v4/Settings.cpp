
#include "Common.h"

char versionString[] = "1.11";

WiFiServer telnetServer(23);
WiFiClient telnetClient;

String cmdString;
String argString;
String rawString;

bool mqttDirty = false;
bool wifiDirty = false;

typedef enum Commands_t { server, port, user, key, tempfeed, setfeed, daySet, nightSet, overrideSet, overrideLen, dayStart, dayEnd, tempList, ssid, password, defaults, settings, save, cancel, menu, unknown};

char* commandStrings[] = {
                          "server",
                          "port",
                          "user",
                          "key",
                          "tempfeed",
                          "setfeed",
                          "day",
                          "night",
                          "override",
                          "overrideLen",
                          "dayStart",
                          "dayEnd",
                          "tempList",
                          "ssid",
                          "password",
                          "defaults",
                          "settings",
                          "save",
                          "cancel",
                          "?"
                         };

typedef struct {
    long header;
    char server[256];
    int  port;
    char user[256];
    char key[256];
    char tempfeed[256];
    char setfeed[256];
    float  daySet;
    float  nightSet;
    float  overrideSet;
    int  overrideLen;
    int  dayStart;
    int  dayEnd;
    char ssid[32];
    char password[32];
} Settings_t;

Settings_t eepromSettings;  //current eeprom settings (mirror of eeprom)
Settings_t editSettings;    // editable copy of current settings

Settings_t defaultSettings = {
                                0xa5a5a5a5,         //header
                                "io.adafruit.com",  //server
                                1883,               //port
                                "",                 //user
                                "",                 //key
                                "",                 //tempFeed
                                "",                 //setFeed
                                55,                 //daySet
                                55,                 //nightSet
                                65,                 //overrideSet
                                120,                //overrideLen
                                730,                //dayStart
                                1800,               //dayEnd  
                                "BuffaloLab",       //ssid
                                "M4k3Stuff"         //password                          
                              };

void ShowMenu()
{
  telnetClient.print("Version:"); telnetClient.println(versionString); telnetClient.println();
  
  telnetClient.println("Menu:");
  telnetClient.println("  server       mqttAddress   # set MQTT server");
  telnetClient.println("  port         mqttPort      # set MQTT port");
  telnetClient.println("  user         mqttUserName  # set MQTT user name");
  telnetClient.println("  key          mqttKey       # set MQTT key");
  telnetClient.println("  tempfeed     feedName      # set MQTT temperature feed");
  telnetClient.println("  setfeed      feedName      # set MQTT setpoint feed");
  telnetClient.println("  day          setPoint      # set daytime set-point");
  telnetClient.println("  night        setPoint      # set night set-point");
  telnetClient.println("  override     setPoint      # set override set-point");
  telnetClient.println("  overrideLen  minutes       # set override duration (minutes)");
  telnetClient.println("  dayStart     time          # set day start time (hour*100 + min). Ex. 730");
  telnetClient.println("  dayEnd       time          # set day end time (hour*100 + min). Ex. 1800");
  telnetClient.println("  tempList                   # list temperature sensors");
  telnetClient.println("  ssid         wifi ssid     # set wifi access point ssid");
  telnetClient.println("  password     wifi password # set wifi access point password");
  telnetClient.println("  defaults                   # reset to default settings");
  telnetClient.println("  settings                   # print current settings (cannot be undone)");
  telnetClient.println("  save                       # save current settings");
  telnetClient.println("  cancel                     # cancel changes");
  telnetClient.println("  ?                          # print this menu");
  telnetClient.println();
}

void ShowSettings()
{
  telnetClient.println("Current Settings:");
  telnetClient.print("  server     :  "); telnetClient.println(editSettings.server);
  telnetClient.print("  port       :  "); telnetClient.println(editSettings.port);
  telnetClient.print("  user       :  "); telnetClient.println(editSettings.user);
  telnetClient.print("  key        :  "); telnetClient.println(editSettings.key);
  telnetClient.print("  tempfeed   :  "); telnetClient.println(editSettings.tempfeed);
  telnetClient.print("  setfeed    :  "); telnetClient.println(editSettings.setfeed);
  telnetClient.print("  day        :  "); telnetClient.println(editSettings.daySet);
  telnetClient.print("  night      :  "); telnetClient.println(editSettings.nightSet);
  telnetClient.print("  override   :  "); telnetClient.println(editSettings.overrideSet);
  telnetClient.print("  overrideLen:  "); telnetClient.println(editSettings.overrideLen);
  telnetClient.print("  dayStart   :  "); telnetClient.println(editSettings.dayStart);
  telnetClient.print("  dayEnd     :  "); telnetClient.println(editSettings.dayEnd);
  telnetClient.print("  ssid       :  "); telnetClient.println(editSettings.ssid);
  telnetClient.print("  password   :  "); telnetClient.println("*********");
  telnetClient.println();
}

void SaveSettings()
{
  memcpy(&eepromSettings, &editSettings, sizeof(Settings_t)); //commit editable copy to current settings
  eepromSettings.header = 0xa5a5a5a5;
  EEPROM.put(0, eepromSettings);
  EEPROM.commit();
}

void CancelSettings()
{
//   EEPROM.get(0, eepromSettings); 
   memcpy(&editSettings, &eepromSettings, sizeof(Settings_t)); //get editable copy of current settings
}

bool EEPROMInit()
{
  EEPROM.begin(sizeof(Settings_t));
  EEPROM.get(0, eepromSettings);
  return (eepromSettings.header == 0xa5a5a5a5);
}

int FindCmdIndex()
{
  int command = server;
  bool found = false;
  
  while ((!found) && (command < unknown))
  {
    found = cmdString.equalsIgnoreCase(String(commandStrings[command]));
    if (!found) 
      command++;
  }
  if (found)
    return command;
  else
    return unknown;
}

void ParseCommand()
{
  char buffer[128] = "";
  int index = 0;
  char ch = 0;

  if (telnetClient.available())
  {
    ch = telnetClient.read();
    while ((telnetClient.available()) && ((ch == '/r') || (ch == '/n')))
    {
      ch = telnetClient.read();
    }

    if (ch != 0)
    {
      while ((telnetClient.available()) && (index < 127) && ((ch != '/r') || (ch != '/n')))
      {
//        Serial.print(ch);
        buffer[index++] = ch;
        ch = telnetClient.read();
      }
    }

    buffer[index] = 0;
    rawString = String(buffer);
    rawString.trim();
    argString = "";
    cmdString = "";
    if (rawString.length() != 0)
    {
      int separator = rawString.indexOf(' ');
      if (0 != separator)
      {
        cmdString = rawString.substring(0, separator);
        argString = rawString.substring(separator);
        argString.trim();
      }
    }
//    Serial.println(rawString);
//    Serial.println(cmdString);
//    Serial.println(argString);
  }
}

bool ExecuteCommand()
{
  bool cmdExecuted = false;
  if (!cmdString.equals(String("")))
  {
    switch (FindCmdIndex())
    {
      case server:
        strncpy(editSettings.server, argString.c_str(), 256);
        mqttDirty = true;
        break;
      case port:
        editSettings.port = atoi(argString.c_str());
        mqttDirty = true;
        break;
      case user:
        strncpy(editSettings.user, argString.c_str(), 256);
        mqttDirty = true;
        break;
      case key:
        strncpy(editSettings.key, argString.c_str(), 256);
        mqttDirty = true;
        break;
      case tempfeed:
        strncpy(editSettings.tempfeed, argString.c_str(), 256);
        mqttDirty = true;
        break;
      case setfeed:
        strncpy(editSettings.setfeed, argString.c_str(), 256);
        mqttDirty = true;
        break;
      case daySet:
        editSettings.daySet = atof(argString.c_str());
        break;
      case nightSet:
        editSettings.nightSet = atof(argString.c_str());
        break;
      case overrideSet:
        editSettings.overrideSet = atof(argString.c_str());
        break;
      case overrideLen:
        editSettings.overrideLen = atoi(argString.c_str());
        break;      
      case dayStart:
        editSettings.dayStart = atoi(argString.c_str());
        break;
      case dayEnd:
        editSettings.dayEnd = atoi(argString.c_str());
        break;      
      case tempList:
        ListTempSensors();
        break;      
       case ssid:
        strncpy(editSettings.ssid, argString.c_str(), 256);
        wifiDirty = true;
        break;
      case password:
        strncpy(editSettings.password, argString.c_str(), 256);
        wifiDirty = true;
        break;
     case defaults:
//        memcpy(&eepromSettings, &defaultSettings, sizeof(Settings_t));
        memcpy(&editSettings, &defaultSettings, sizeof(Settings_t));  //get editable copy of current settings
        SaveSettings();
        wifiDirty = true;
        mqttDirty = true;
        break;
     case settings:
        ShowSettings();
        break;
      case save:
        SaveSettings();

        UpdateSchedule();
        
        UpdateOverride();

        if (wifiDirty)
        {
          telnetClient.println("Reconnecting WiFi. Please wait. May take a minute...");
          telnetClient.println("Please close and re-open your telnet connection to continue changing settings");
          delay(500);
          WiFi.disconnect();  //we will automatically retry connection. May take a minute or so...
          wifiDirty = false;
        }
        
        if (mqttDirty)
        {
          telnetClient.print("MQTT reconnect ");
          if (ReinitAdafruitMQTT())
          {
            telnetClient.println("successful");
            FeedAdafruitMQTT(currentTemp, currentSetpoint);
          }
          else
             telnetClient.println("failed");           
          mqttDirty = false;
        }
        break;
      case cancel:
        CancelSettings();
        mqttDirty = false;
        break;
      case menu:
        ShowMenu();
        break;
      case unknown:
      default:
        telnetClient.println("Unrecognized command");
        break;
    }
    cmdExecuted = true;
  }
  return cmdExecuted;
}

void SettingsInit() 
{  
  if (!EEPROMInit())
  {
    memcpy(&editSettings, &defaultSettings, sizeof(Settings_t));
    SaveSettings();
  }

  memcpy(&editSettings, &eepromSettings, sizeof(Settings_t));  //get editable copy of current settings

  rawString.reserve(256);
  cmdString.reserve(256);
  argString.reserve(256);

}

void ParseSerialCommand()
{
  char buffer[128] = "";
  int index = 0;
  char ch = 0;

  if (Serial.available())
  {
    ch = Serial.read();
    
    while ((Serial.available()) && ((ch == '/r') || (ch == '/n')))
    {
      ch = Serial.read();
    }
    
    if (ch != 0)
    {
      while ((Serial.available()) && (index < 127) && ((ch != '/r') || (ch != '/n')))
      {
        buffer[index++] = ch;
        ch = Serial.read();
      }
    }

    buffer[index] = 0;
    rawString = String(buffer);
    rawString.trim();
    argString = "";
    cmdString = "";
    if (rawString.length() != 0)
    {
      int separator = rawString.indexOf(' ');
      if (0 != separator)
      {
        cmdString = rawString.substring(0, separator);
        argString = rawString.substring(separator);
        argString.trim();
      }
    }
//    Serial.println(rawString);
//    Serial.println(cmdString);
//    Serial.println(argString);
  }
}

//Can change settings via serial port at startup. 
//  Useful in case telnet isn't working due to incorrect SSID or password.
void SerialBackdoor()
{
  Serial.println("Enter any character within 5 seconds to halt startup and change settings");
  int timer = 5;
  while ((timer) && (!Serial.available()))
  {
    Serial.print(" --> "); Serial.println(timer--);
    delay(1000);
  }
  
  if (Serial.available())
  {
    while (Serial.available())
      char c = Serial.read();
      
    Serial.println("  enter \"save\" to continue startup"); 
    Serial.print("> ");
    
    do {
      cmdString = "";
      delay(100);
      ParseSerialCommand();
      if (ExecuteCommand())
        Serial.print("> ");
    } while (!cmdString.equals(String("save")));
  }
}

void TelnetInit()
{
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  
  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");

}

void ProcessSettings() 
{
  if (telnetServer.hasClient())
  {
    telnetClient = telnetServer.available();
    ShowMenu();
    telnetClient.println();
    telnetClient.print("> ");
    telnetClient.flush();
    Serial.println("telnet connected");
    memcpy(&editSettings, &eepromSettings, sizeof(Settings_t));  //get editable copy of current settings
  }
  if (telnetClient.connected())
  {
    cmdString = "";
    ParseCommand();
    if (ExecuteCommand())
      telnetClient.print("> ");
  }
}

void ListTempSensors()
{
  int index = 0;
  uint8_t sensorAddr[8];
  TempStartConversion();
  while (GetTempSensorAddress(sensorAddr, index))
  {
    telnetClient.print(index);
    telnetClient.print(":");

    for (int i = 0; i < 8; i++) {
      telnetClient.print(" ");
      telnetClient.print(sensorAddr[i], HEX);
    }
    
    telnetClient.print(": Temp = ");
    telnetClient.print(GetSensorTemperature(index));
    telnetClient.println("F");
    index++;
  }
}

char* GetMQTTServer()
{
  return eepromSettings.server;
}

int GetMQTTPort()
{
  return eepromSettings.port;
}

char* GetMQTTUser()
{
  return eepromSettings.user;
}

char* GetMQTTKey()
{
  return eepromSettings.key;
}

char* GetMQTTTempfeed()
{
  return eepromSettings.tempfeed;
}

char* GetMQTTSetfeed()
{
  return eepromSettings.setfeed;
}

float GetDaySetpoint()
{
  return eepromSettings.daySet;
}

float GetNightSetpoint()
{
  return eepromSettings.nightSet;
}

float GetOverrideSetpoint()
{
  return eepromSettings.overrideSet;
}

long GetOverrideDuration()
{
  return eepromSettings.overrideLen * 60; //return duration in seconds
}

int GetDayStart()
{
  return eepromSettings.dayStart;
}

int GetDayEnd()
{
  return eepromSettings.dayEnd;
}

char* GetSSID()
{
  return eepromSettings.ssid;
}

char* GetPassword()
{
  return eepromSettings.password;
}

