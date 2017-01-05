
#include "Common.h"

WiFiServer telnetServer(23);
WiFiClient telnetClient;

String cmdString;
String argString;
String rawString;

bool mqttDirty = false;

typedef enum Commands_t { server, port, user, key, tempfeed, setfeed, daySet, nightSet, overrideSet, overrideLen, dayStart, dayEnd, settings, save, cancel, menu, unknown};

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
    int  daySet;
    int  nightSet;
    int  overrideSet;
    int  overrideLen;
    int  dayStart;
    int  dayEnd;
} Settings_t;

Settings_t eepromSettings;

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
                                1800                //dayEnd                            
                              };

void ShowMenu()
{
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
  telnetClient.println("  dayStart     time          # set day start time (hour*100 + min)");
  telnetClient.println("  dayEnd       time          # set day end time (hour*100 + min)");
  telnetClient.println("  settings                   # print current settings");
  telnetClient.println("  save                       # save current settings");
  telnetClient.println("  cancel                     # cancel changes");
  telnetClient.println("  ?                          # print this menu");
  telnetClient.println();
}

void ShowSettings()
{
  telnetClient.println("Current Settings:");
  telnetClient.print("  server     :  "); telnetClient.println(eepromSettings.server);
  telnetClient.print("  port       :  "); telnetClient.println(eepromSettings.port);
  telnetClient.print("  user       :  "); telnetClient.println(eepromSettings.user);
  telnetClient.print("  key        :  "); telnetClient.println(eepromSettings.key);
  telnetClient.print("  tempfeed   :  "); telnetClient.println(eepromSettings.tempfeed);
  telnetClient.print("  setfeed    :  "); telnetClient.println(eepromSettings.setfeed);
  telnetClient.print("  day        :  "); telnetClient.println(eepromSettings.daySet);
  telnetClient.print("  night      :  "); telnetClient.println(eepromSettings.nightSet);
  telnetClient.print("  override   :  "); telnetClient.println(eepromSettings.overrideSet);
  telnetClient.print("  overrideLen:  "); telnetClient.println(eepromSettings.overrideLen);
  telnetClient.print("  dayStart   :  "); telnetClient.println(eepromSettings.dayStart);
  telnetClient.print("  dayEnd     :  "); telnetClient.println(eepromSettings.dayEnd);
  telnetClient.println();
}

void SaveSettings()
{
  eepromSettings.header = 0xa5a5a5a5;
  EEPROM.put(0, eepromSettings);
  EEPROM.commit();
}

void CancelSettings()
{
   EEPROM.get(0, eepromSettings); 
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
      while ((telnetClient.available()) && ((ch != '/r') || (ch != '/n')))
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

void ExecuteCommand()
{
  if (!cmdString.equals(String("")))
  {
    switch (FindCmdIndex())
    {
      case server:
        strncpy(eepromSettings.server, argString.c_str(), 256);
        mqttDirty = true;
        break;
      case port:
        eepromSettings.port = atoi(argString.c_str());
        mqttDirty = true;
        break;
      case user:
        strncpy(eepromSettings.user, argString.c_str(), 256);
        mqttDirty = true;
        break;
      case key:
        strncpy(eepromSettings.key, argString.c_str(), 256);
        break;
      case tempfeed:
        strncpy(eepromSettings.tempfeed, argString.c_str(), 256);
        mqttDirty = true;
        break;
      case setfeed:
        strncpy(eepromSettings.setfeed, argString.c_str(), 256);
        mqttDirty = true;
        break;
      case daySet:
        eepromSettings.daySet = atoi(argString.c_str());
        break;
      case nightSet:
        eepromSettings.nightSet = atoi(argString.c_str());
        break;
      case overrideSet:
        eepromSettings.overrideSet = atoi(argString.c_str());
        break;
      case overrideLen:
        eepromSettings.overrideLen = atoi(argString.c_str());
        break;      
      case dayStart:
        eepromSettings.dayStart = atoi(argString.c_str());
        break;
      case dayEnd:
        eepromSettings.dayEnd = atoi(argString.c_str());
        break;      
      case settings:
        ShowSettings();
        break;
      case save:
        SaveSettings();

        UpdateSchedule();
        
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
  telnetClient.print("> ");
  }
}

void SettingsInit() 
{
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  
  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");

  rawString.reserve(256);
  cmdString.reserve(256);
  argString.reserve(256);

  if (!EEPROMInit())
  {
    memcpy(&eepromSettings, &defaultSettings, sizeof(Settings_t));
    SaveSettings();
  }
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
  }
  if (telnetClient.connected())
  {
    cmdString = "";
    ParseCommand();
    ExecuteCommand();
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

int GetDaySetpoint()
{
  return eepromSettings.daySet;
}

int GetNightSetpoint()
{
  return eepromSettings.nightSet;
}

int GetOverrideSetpoint()
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

