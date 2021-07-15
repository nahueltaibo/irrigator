#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>

// Pins definition
#define PIN_RELAY_1 36
#define PIN_RELAY_2 39
#define PIN_RELAY_3 34
#define PIN_RELAY_4 35
#define PIN_RGBLED_R 32
#define PIN_RGBLED_G 33
#define PIN_RGBLED_B 25

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";
const char *PARAM_INPUT_3 = "ip";

//Variables to save values from HTML form
String ssid;
String pass;
String ip;

// File paths to save input values permanently
const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";
const char *ipPath = "/ip.txt";

IPAddress localIP;

// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// Timer variables (check wifi)
unsigned long previousMillis = 0;
const long interval = 10000; // interval to wait for Wi-Fi connection (milliseconds)

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables (get sensor readings)
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

//-----------------FUNCTIONS TO HANDLE SENSOR READINGS-----------------//

// Return JSON String from sensor Readings
String getJSONReadings()
{
  readings["temperature"] = String(22);
  readings["humidity"] = String(13);
  readings["pressure"] = String(54);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

// Initialize SPIFFS
void initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else
  {
    Serial.println("SPIFFS mounted successfully");
  }
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available())
  {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("- file written");
  }
  else
  {
    Serial.println("- frite failed");
  }
}

// Initialize WiFi
bool initWiFi()
{
  if (ssid == "" || ip == "")
  {
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());

  if (!WiFi.config(localIP, gateway, subnet))
  {
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while (WiFi.status() != WL_CONNECTED)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

void configurePins()
{
  // Integrated LED
  pinMode(LED_BUILTIN, OUTPUT);

  // Relays pins
  pinMode(PIN_RELAY_1, OUTPUT);
  pinMode(PIN_RELAY_2, OUTPUT);
  pinMode(PIN_RELAY_3, OUTPUT);
  pinMode(PIN_RELAY_4, OUTPUT);

  //RGB LED pins
  pinMode(PIN_RGBLED_R, OUTPUT);
  pinMode(PIN_RGBLED_G, OUTPUT);
  pinMode(PIN_RGBLED_B, OUTPUT);
}

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);

  configurePins();

  // Init SPIFFS
  initSPIFFS();

  // Load values saved in SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  Serial.printf("ssid: %s\r\n", ssid.c_str());
  // Serial.printf("pass: %s",pass);
  Serial.printf("ip: %s\r\n", ip.c_str());

  if (initWiFi())
  {
    // LED turns green when connected to wifi
    digitalWrite(PIN_RGBLED_R, LOW);
    digitalWrite(PIN_RGBLED_G, HIGH);
    digitalWrite(PIN_RGBLED_B, LOW);

    // Handle the Web Server in Station Mode
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                Serial.println("GET /");
                request->send(SPIFFS, "/index.html", "text/html");
              });
    server.serveStatic("/", SPIFFS, "/");

    // Request for the latest sensor readings
    server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                Serial.println("GET /readings");

                String json = getJSONReadings();
                request->send(200, "application/json", json);
                json = String();
              });

    events.onConnect([](AsyncEventSourceClient *client)
                     {
                       if (client->lastId())
                       {
                         Serial.printf("Client connected. Last message ID: %u\n", client->lastId());
                       }
                     });
    server.addHandler(&events);

    server.begin();
  }
  else
  {
    // LED turns red when connected to wifi
    digitalWrite(PIN_RGBLED_R, HIGH);
    digitalWrite(PIN_RGBLED_G, LOW);
    digitalWrite(PIN_RGBLED_B, LOW);

    // Set Access Point
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Web Server Root URL For WiFi Manager Web Page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                Serial.println("WiFiMan: /");
                request->send(SPIFFS, "/wifimanager.html", "text/html");
              });

    server.serveStatic("/", SPIFFS, "/");

    // Get the parameters submited on the form
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                Serial.println("POST: /");

                int params = request->params();
                for (int i = 0; i < params; i++)
                {
                  AsyncWebParameter *p = request->getParam(i);
                  if (p->isPost())
                  {
                    // HTTP POST ssid value
                    if (p->name() == PARAM_INPUT_1)
                    {
                      ssid = p->value().c_str();
                      Serial.print("SSID set to: ");
                      Serial.println(ssid);
                      // Write file to save value
                      writeFile(SPIFFS, ssidPath, ssid.c_str());
                    }
                    // HTTP POST pass value
                    if (p->name() == PARAM_INPUT_2)
                    {
                      pass = p->value().c_str();
                      Serial.print("Password set to: ");
                      Serial.println(pass);
                      // Write file to save value
                      writeFile(SPIFFS, passPath, pass.c_str());
                    }
                    // HTTP POST ip value
                    if (p->name() == PARAM_INPUT_3)
                    {
                      ip = p->value().c_str();
                      Serial.print("IP Address set to: ");
                      Serial.println(ip);
                      // Write file to save value
                      writeFile(SPIFFS, ipPath, ip.c_str());
                    }
                    //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
                  }
                }
                request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
                delay(3000);
                // After saving the parameters, restart the ESP32
                ESP.restart();
              });
    server.begin();
  }
}

void loop()
{
  // If the ESP32 is set successfully in station mode...
  if (WiFi.status() == WL_CONNECTED)
  {
    //...Send Events to the client with sensor readins and update colors every 30 seconds
    if (millis() - lastTime > timerDelay)
    {
      String message = getJSONReadings();
      events.send(message.c_str(), "new_readings", millis());
      lastTime = millis();
    }
  }
}
