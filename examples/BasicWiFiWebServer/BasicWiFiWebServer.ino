
// Includes for ATWINC1500 WiFi
#include <WiFi101.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

#include <EasyWebServer.h> // Must be included AFTER the ethernet libraries. See comment in EasyWebServer.h.

#include "wifiinfo.h"

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
WiFiServer server(80);

int digitalPins[] = {3, 5, 6};
//int digitalPints[] = {2, 3, 4, 5, 6, 7, 8};
int countDigitalPins = sizeof(digitalPins) / sizeof(int);

void setup() {
   //Configure pins for Adafruit ATWINC1500 Feather
   // uncomment the next line if using the Adafruit board.
   WiFi.setPins(8,7,4,2);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) { ; } // wait for serial port to connect. Needed for native USB port only

  if (WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }
  // Start the Ethernet connection and the server:
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  server.begin();
  Serial.print("Server is at ");
 
  while (WiFi.localIP() == INADDR_NONE) {
    Serial.print(".");
    delay(300);
  }

  IPAddress clientip = WiFi.localIP();
  Serial.println(clientip);

  for (int digitalPinIndex = 0; digitalPinIndex < countDigitalPins; digitalPinIndex++)
    pinMode(digitalPins[digitalPinIndex],INPUT);
}

void loop() {
  // Listen for incoming clients
  if (WiFi.status() == WL_CONNECTED)
  {
    // listen for incoming clients
    WiFiClient client = server.available();
    if (client)
    {
      Serial.println("new client");
      {
        EasyWebServer w(client);                    // Read and parse the HTTP Request
        w.serveUrl("/",rootPage);                   // Root page
        w.serveUrl("/analog",analogSensorPage);     // Analog sensor page
        w.serveUrl("/digital",digitalSensorPage);   // Digital sensor page
      }
    }
  }
}

void rootPage(EasyWebServer &w){
  w.client.println(F("<!DOCTYPE HTML>"));
  w.client.println(F("<html><head><title>EasyWebServer</title></head><body>"));
  w.client.println(F("<p>Welcome to my little web server.</p>"));
  w.client.println(F("<p><a href='/analog'>Click here to see the analog sensors</a></p>"));
  w.client.println(F("<p><a href='/digital'>Click here to see the digital sensors</a></p>"));
  w.client.println(F("</body></html>"));
}

void analogSensorPage(EasyWebServer &w){
  w.client.println(F("<!DOCTYPE HTML>"));
  w.client.println(F("<html><head><title>Analog Sensors</title></head><body>"));
  for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
    int sensorReading = analogRead(analogChannel); // Note that analogReaad uses CHANNELS insead of pin.
    w.client.print(F("analog input "));
    w.client.print(analogChannel);
    w.client.print(F(" is "));
    w.client.print(sensorReading);
    w.client.println(F("<br />"));
  }
  w.client.println(F("<p><a href='/'>Home</a></body></html>"));
}

void digitalSensorPage(EasyWebServer &w){
  w.client.println(F("<!DOCTYPE HTML>"));
  w.client.println(F("<html><head><title>Digital Sensors</title></head><body>"));
  for (int digitalPinIndex = 0; digitalPinIndex < countDigitalPins; digitalPinIndex++) {
    int sensorReading = digitalRead(digitalPins[digitalPinIndex]);
    w.client.print(F("digital pin "));
    w.client.print(digitalPins[digitalPinIndex]);
    w.client.print(F(" is "));
    w.client.print(sensorReading);
    w.client.println(F("<br />"));
  }
  w.client.println(F("<p><a href='/'>Home</a></body></html>"));
}
