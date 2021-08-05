#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>

WiFiServer server(80);
// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String relayState = "off";
String output4State = "off";

// Assign output variables to GPIO pins
const int PIN_RELAY = 0;

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000; // Define timeout time in milliseconds (example: 2000ms = 2s)

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

void processClientRequest(WiFiClient client)
{
  // If a new client connects,
  Serial.println("New Client."); // print a message out in the serial port
  String currentLine = "";       // make a String to hold incoming data from the client
  currentTime = millis();
  previousTime = currentTime;

  while (client.connected() && currentTime - previousTime <= timeoutTime)
  {
    // loop while the client's connected
    currentTime = millis();
    if (client.available()) // if there's bytes to read from the client
    {
      char c = client.read(); // read a byte
      Serial.write(c);        // then print it out the serial monitor
      header += c;

      if (c == '\n')
      {
        // if the byte is a newline character
        // if the current line is blank, you got two newline characters in a row.
        // that's the end of the client HTTP request, so send a response:
        if (currentLine.length() == 0)
        {
          // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
          // and a content-type so the client knows what's coming, then a blank line:
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println("Connection: close");
          client.println();

          // turns the GPIOs on and off
          if (header.indexOf("GET /relay/on") >= 0)
          {
            Serial.println("relay on");
            relayState = "on";
            digitalWrite(PIN_RELAY, HIGH);
          }
          else if (header.indexOf("GET /relay/off") >= 0)
          {
            Serial.println("relay off");
            relayState = "off";
            digitalWrite(PIN_RELAY, LOW);
          }

          // Display the HTML web page
          client.println("<!DOCTYPE html><html>");
          client.println("<head>");
          client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
          client.println("<link rel='icon' href='data:,'>");
          client.println("<link href='https://cdn.jsdelivr.net/gh/nahueltaibo/irrigator@esp-01s/src/data/styles.css' rel='stylesheet'></link>");
          client.println("</head>");

          // Web Page Heading
          client.println("<body><h1>ESP8266 Web Server</h1>");

          // Display current state, and ON/OFF buttons for GPIO 5
          client.println("<p>Relay - State " + relayState + "</p>");
          // If the output5State is off, it displays the ON button
          if (relayState == "off")
          {
            client.println("<p><a href='/relay/on'><button class='button'>ON</button></a></p>");
          }
          else
          {
            client.println("<p><a href='/relay/off'><button class='button button2'>OFF</button></a></p>");
          }

          client.println("<script src='https://cdn.jsdelivr.net/gh/nahueltaibo/irrigator@esp-01s/src/data/main.js'></script>");
          client.println("</body></html>");

          // The HTTP response ends with another blank line
          client.println();
          // Break out of the while loop
          break;
        }
        else
        { // if you got a newline, then clear currentLine
          currentLine = "";
        }
      }
      else if (c != '\r')
      {                   // if you got anything else but a carriage return character,
        currentLine += c; // add it to the end of the currentLine
      }
    }
  }
  // Clear the header variable
  header = "";
  // Close the connection
  client.stop();
  Serial.println("Client disconnected.");
  Serial.println("");
}

void setup()
{
  Serial.begin(115200);

  connectToWifi();

  configureOTA();

  // Initialize the output variables as outputs
  pinMode(PIN_RELAY, OUTPUT);
  // Set outputs to LOW
  digitalWrite(PIN_RELAY, LOW);

  server.begin();
}

void loop()
{
  ArduinoOTA.handle();

  WiFiClient client = server.available(); // Listen for incoming clients

  if (client)
  {
    processClientRequest(client);
  }
}