#include <SPI.h>
#include <Wire.h>
#include "WiFi.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <Udp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// wifi parameters 
#define WIFI_NETWORK "NETWORK_NAME"
#define WIFI_PASSWORD "NETWORK_PASSWORD"
#define WIFI_TIMEOUT_MS 20000

// current Time client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedDate;
String dayStamp;
String timeStamp;

// screen Parameters
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

DynamicJsonDocument doc(24576);

void setup() 
{
  Serial.begin(9600);
  // Wait for display
  delay(500);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  ConnectToWifi();
  
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600);
}

void loop() 
{
  // Get Current Time
  timeClient.update();
  int splitT = formattedDate.indexOf("T");
  formattedDate = timeClient.getFormattedTime();
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1); // time stamp is hh:mm:s
  timeStamp = timeStamp.substring(3,5);

  // 10 mins of departures
  HTTPClient http;
  http.begin("https://v6.bvg.transport.rest/stops/900055102/departures?duration=10&regional=false&suberban=false&linesOfStops=true&bus=false&remarks=false"); //Specify request destination
  int httpCode = http.GET(); //Send the request

  DeserializationError error = deserializeJson(doc, http.getString()); //Get the request response payload
  if (error) 
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  JsonArray departures = doc["departures"];

  // prep display
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int depIndex = 0;
  while ( !departures[depIndex].isNull() )
  {
    const char* dest = departures[depIndex]["direction"];
    const char* timeData  = departures[depIndex]["plannedWhen"];
    String min = String(timeData).substring(14, 16);
    int leaveTime = 0;
    int currentTime = 0;
    int timeRemaining = 0;

    // convert minute string to int
    for (int strIndex = 0; strIndex < 2; strIndex++)
    {
      currentTime += (int)(timeStamp[strIndex]) - 30;   // ascii numbers start at byte 30
      leaveTime += (int)(min[strIndex]) - 30;

      if (strIndex == 0)
      {
        leaveTime *= 10;
        currentTime *= 10;
      } 
    }

    // Get the difference between times 
    if (currentTime > leaveTime) // check if the depart time is in the next hour
    {
      leaveTime += 60;
    }

    timeRemaining = leaveTime - currentTime;

    display.printf("%.15s: %d\n", dest, timeRemaining);

    depIndex++;
  }

  display.display();

  http.end(); //Close connection

  delay(3000);
}

void ConnectToWifi()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.setTextColor(SSD1306_WHITE);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);

  display.println("Connecting to wifi...");
  display.display();
  delay(100);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS)
  {
    display.clearDisplay();
    display.setCursor(0,0);  
    display.println("Connecting to wifi.");
    display.display();
    delay(500);

    display.clearDisplay();
    display.setCursor(0,0);  
    display.println("Connecting to wifi..");
    display.display();
    delay(500);

    display.clearDisplay();
    display.setCursor(0,0);  
    display.println("Connecting to wifi...");
    display.display();
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    display.clearDisplay();
    display.setCursor(0,0);  
    display.println("Error Connecting");
    display.display();
    delay(1000);
  }
  else
  {
    display.clearDisplay();
    display.setCursor(0,0);  
    display.println("CONNECTED");
    display.println(WiFi.localIP());
    display.display();
    delay(1000);
  }
}



