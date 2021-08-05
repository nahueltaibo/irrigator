#include <ESP8266WiFi.h>
// #include <ESP8266mDNS.h>
// #include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>

void configureOTA()
{
  // No authentication by default
  ArduinoOTA.setPassword("0fa52050-281d-46b3-9693-244b493a7003");

  ArduinoOTA.onStart([]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                       {
                         type = "sketch";
                       }
                       else
                       { // U_FS
                         type = "filesystem";
                       }

                       // NOTE: if updating FS this would be the place to unmount FS using FS.end()
                       Serial.println("Start updating " + type);
                     });

  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });

  ArduinoOTA.onError([](ota_error_t error)
                     {
                       Serial.printf("Error[%u]: ", error);
                       if (error == OTA_AUTH_ERROR)
                       {
                         Serial.println("Auth Failed");
                       }
                       else if (error == OTA_BEGIN_ERROR)
                       {
                         Serial.println("Begin Failed");
                       }
                       else if (error == OTA_CONNECT_ERROR)
                       {
                         Serial.println("Connect Failed");
                       }
                       else if (error == OTA_RECEIVE_ERROR)
                       {
                         Serial.println("Receive Failed");
                       }
                       else if (error == OTA_END_ERROR)
                       {
                         Serial.println("End Failed");
                       }
                     });

  ArduinoOTA.begin();
}

void connectToWifi()
{
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  WiFiManager wm;

  //reset settings - wipe credentials for testing
  //wm.resetSettings();

  bool res = wm.autoConnect("ConfigureDevice");

  if (!res)
  {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  else
  {
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
  }
}

void setup()
{
  Serial.begin(115200);

  connectToWifi();

  configureOTA();
}

void loop()
{
  ArduinoOTA.handle();
}