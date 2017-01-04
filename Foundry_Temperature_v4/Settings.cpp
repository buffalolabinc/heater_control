
#include "Common.h"

WiFiServer telnetServer(23);
WiFiClient telnetClient;

String cmdString;
String argString;
String rawString;

bool mqttDirty = false;

typedef enum Commands_t { server, port, user, key, temp, override, settings, save, cancel, menu, unknown};

char* commandStrings[] = {
                          "server",
                          "port",
                          "user",
                          "key",
                          "temp",
                          "override",
                          "settings",
                          "save",
                          "cancel",
                          "?"
                         };

typedef struct {
    char server[256];
    int  port;
    char user[256];
    char key[256];
    char temp[256];
    char override[256];
} Settings_t;

Settings_t eepromSettings;


void ShowMenu()
{
  telnetClient.println("Menu:");
  telnetClient.println("  server    mqttAddress   # set MQTT server");
  telnetClient.println("  port      mqttPort      # set MQTT port");
  telnetClient.println("  user      mqttUserName  # set MQTT user name");
  telnetClient.println("  key       mqttKey       # set MQTT key");
  telnetClient.println("  temp      feedName      # set MQTT temperature feed");
  telnetClient.println("  override  feedName      # set MQTT override feed");
  telnetClient.println("  settings                # print current settings");
  telnetClient.println("  save                    # save current settings");
  telnetClient.println("  cancel                  # cancel changes");
  telnetClient.println("  ?                       # print this menu");
  telnetClient.println();
}

void ShowSettings()
{
  telnetClient.println("Current Settings:");
  telnetClient.print("  server  :  "); telnetClient.println(eepromSettings.server);
  telnetClient.print("  port    :  "); telnetClient.println(eepromSettings.port);
  telnetClient.print("  user    :  "); telnetClient.println(eepromSettings.user);
  telnetClient.print("  key     :  "); telnetClient.println(eepromSettings.key);
  telnetClient.print("  temp    :  "); telnetClient.println(eepromSettings.temp);
  telnetClient.print("  override:  "); telnetClient.println(eepromSettings.override);
  telnetClient.println();
}

void SaveSettings()
{
  EEPROM.put(0, eepromSettings);
  EEPROM.commit();
}

void CancelSettings()
{
   EEPROM.get(0, eepromSettings); 
}

void EEPROMInit()
{
  EEPROM.begin(sizeof(Settings_t));
  EEPROM.get(0, eepromSettings);
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
      case temp:
        strncpy(eepromSettings.temp, argString.c_str(), 256);
        mqttDirty = true;
        break;
      case override:
        strncpy(eepromSettings.override, argString.c_str(), 256);
        mqttDirty = true;
        break;
      case settings:
        ShowSettings();
        break;
      case save:
        SaveSettings();
        if (mqttDirty)
        {
          telnetClient.print("MQTT reconnect ");
          if (ReinitAdafruitMQTT())
          {
            telnetClient.println("successful");
            FeedAdafruitMQTT(currentTemp, overrideEnabled);
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

  EEPROMInit();
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

char* GetMQTTTemp()
{
  return eepromSettings.temp;
}

char* GetMQTTOverride()
{
  return eepromSettings.override;
}

