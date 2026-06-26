#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// OLED (SH1106)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// GPS
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// WiFi
const char *ssid = "YOUR_SSID_HERE";         // Replace with your actual SSID
const char *password = "YOUR_PASSWORD_HERE"; // Replace with your actual password

// API
const char *apiUrl = "YOUR_API_ENDPOINT_HERE"; // Replace with your actual API endpoint

// Button & State
const int BUTTON_PIN = 19;
int buttonState;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

struct FlightData
{
  String callsign;
  String type;
  int altitude;
  float speed;
  float distance;
};

const int MAX_FLIGHTS = 10;
FlightData nearbyFlights[MAX_FLIGHTS];
int totalNearby = 0;
int currentIndex = 0;

unsigned long lastApiFetch = 0;
const unsigned long apiInterval = 5000;

// Distance function
float getDistance(float lat1, float lon1, float lat2, float lon2)
{
  float R = 6371.0;
  float dLat = radians(lat2 - lat1);
  float dLon = radians(lon2 - lon1);

  float a = sin(dLat / 2) * sin(dLat / 2) + cos(radians(lat1)) * cos(radians(lat2)) * sin(dLon / 2) * sin(dLon / 2);

  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}

void setup()
{
  Serial.begin(115200);

  u8g2.begin();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Start GPS (RX=16, TX=17)
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

  // WiFi connect
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
}

void updateDisplay()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);

  if (totalNearby == 0)
  {
    u8g2.drawStr(0, 30, "No aircraft");
    u8g2.drawStr(0, 45, "nearby");
    u8g2.sendBuffer();
    return;
  }

  FlightData &f = nearbyFlights[currentIndex];

  char line1[32];
  char line2[32];
  char line3[32];
  char line4[32];
  char pageStr[16];

  sprintf(line1, "%s - %s", f.callsign.c_str(), f.type.c_str());
  sprintf(line2, "Alt: %d ft", f.altitude);

  if (f.speed > 0)
  {
    sprintf(line3, "Spd: %.0f kt", f.speed);
  }
  else
  {
    sprintf(line3, "Spd: N/A");
  }

  sprintf(line4, "Dist: %.1f km", f.distance);

  if (totalNearby > 1)
  {
    sprintf(pageStr, "%d/%d", currentIndex + 1, totalNearby);
  }
  else
  {
    sprintf(pageStr, ""); // Clear string if only 1
  }

  u8g2.drawStr(0, 12, line1);
  u8g2.drawStr(0, 28, line2);
  u8g2.drawStr(0, 44, line3);
  u8g2.drawStr(0, 60, line4);

  // Draw pagination at bottom right (x=100, y=60)
  if (totalNearby > 1)
  {
    u8g2.drawStr(100, 60, pageStr);
  }

  u8g2.sendBuffer();
}

void loop()
{
  // Read GPS continuously
  while (gpsSerial.available())
  {
    gps.encode(gpsSerial.read());
  }

  // Wait until GPS is valid
  if (!gps.location.isValid())
  {
    // Only update screen every 1s so we don't spam OLED
    if (millis() - lastApiFetch > 1000 || lastApiFetch == 0)
    {
      lastApiFetch = millis();
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(0, 30, "Waiting GPS...");
      u8g2.sendBuffer();
    }
    return;
  }

  float myLat = gps.location.lat();
  float myLon = gps.location.lng();

  // Button debouncing and handling
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState)
  {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != buttonState)
    {
      buttonState = reading;
      if (buttonState == LOW)
      { // Button pressed
        if (totalNearby > 1)
        {
          currentIndex = (currentIndex + 1) % totalNearby;
          updateDisplay();
        }
      }
    }
  }
  lastButtonState = reading;

  // Fetch API periodically (non-blocking)
  if (millis() - lastApiFetch >= apiInterval || lastApiFetch == 0)
  {
    lastApiFetch = millis();

    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;
      http.begin(apiUrl);
      int httpCode = http.GET();

      if (httpCode > 0)
      {
        String payload = http.getString();
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error)
        {
          JsonArray flights = doc["ac"];

          struct TempFlight
          {
            JsonObject obj;
            float dist;
          };
          TempFlight tempArr[MAX_FLIGHTS + 1]; // +1 for easy swapping
          int tempCount = 0;

          for (JsonObject f : flights)
          {
            if (!f["lat"] || !f["lon"] || !f["alt_baro"])
              continue;

            float lat = f["lat"];
            float lon = f["lon"];
            float altitude = f["alt_baro"];

            float dist = getDistance(myLat, myLon, lat, lon);

            // 🎯 VISIBILITY FILTER
            if (altitude > 0 && dist < 50)
            {
              if (tempCount < MAX_FLIGHTS)
              {
                tempArr[tempCount].obj = f;
                tempArr[tempCount].dist = dist;
                tempCount++;
              }
              else
              {
                // Replace furthest if this is closer
                int maxIdx = 0;
                for (int i = 1; i < MAX_FLIGHTS; i++)
                {
                  if (tempArr[i].dist > tempArr[maxIdx].dist)
                  {
                    maxIdx = i;
                  }
                }
                if (dist < tempArr[maxIdx].dist)
                {
                  tempArr[maxIdx].obj = f;
                  tempArr[maxIdx].dist = dist;
                }
              }
            }
          }

          // Sort by distance (closest first)
          for (int i = 0; i < tempCount - 1; i++)
          {
            for (int j = 0; j < tempCount - i - 1; j++)
            {
              if (tempArr[j].dist > tempArr[j + 1].dist)
              {
                TempFlight t = tempArr[j];
                tempArr[j] = tempArr[j + 1];
                tempArr[j + 1] = t;
              }
            }
          }

          // Populate final array
          totalNearby = tempCount;
          for (int i = 0; i < totalNearby; i++)
          {
            JsonObject bestFlight = tempArr[i].obj;

            String callsign = bestFlight["flight"] | "N/A";
            callsign.trim();
            nearbyFlights[i].callsign = callsign;

            const char *type = bestFlight["t"] | "UNK";
            nearbyFlights[i].type = String(type);

            nearbyFlights[i].altitude = bestFlight["alt_baro"] | 0;

            float speed = 0;
            if (bestFlight["gs"] && bestFlight["gs"].as<float>() > 0)
              speed = bestFlight["gs"];
            else if (bestFlight["tas"] && bestFlight["tas"].as<float>() > 0)
              speed = bestFlight["tas"];
            else if (bestFlight["ias"] && bestFlight["ias"].as<float>() > 0)
              speed = bestFlight["ias"];

            nearbyFlights[i].speed = speed;
            nearbyFlights[i].distance = tempArr[i].dist;
          }

          // Keep current index in bounds
          if (currentIndex >= totalNearby)
          {
            currentIndex = 0;
          }

          updateDisplay();
        }
        else
        {
          Serial.println("JSON parse error");
        }
      }
      else
      {
        Serial.println("HTTP Error");
      }
      http.end();
    }
  }
}