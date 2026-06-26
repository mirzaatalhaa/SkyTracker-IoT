#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <U8g2lib.h>

// OLED (SH1106)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// WiFi
const char *ssid = "YouWiFiSSID";
const char *password = "YouWiFiPassword";

// API
const char *apiUrl = "YouAPIEndpoint"; // e.g., "https://opensky-network.org/api/states/all?lamin=12.0&lomin=76.0&lamax=13.0&lomax=77.0"

// Fixed observer location
const float myLat = YouLatitude;  // e.g., 12.9716
const float myLon = YouLongitude; // e.g., 76.398733

// Button & State
const int BUTTON_PIN = 19;
int buttonState;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;



// ─────────────────────────────────────────────────────────────
//  Cardinal direction from track (degrees)
// ─────────────────────────────────────────────────────────────
String getCardinal(float track) {
  if (track < 0) track += 360;
  track = fmod(track, 360.0);
  // 8-point compass
  if (track < 22.5 || track >= 337.5) return "N";
  if (track < 67.5) return "NE";
  if (track < 112.5) return "E";
  if (track < 157.5) return "SE";
  if (track < 202.5) return "S";
  if (track < 247.5) return "SW";
  if (track < 292.5) return "W";
  return "NW";
}

// ─────────────────────────────────────────────────────────────
//  Flight data struct
// ─────────────────────────────────────────────────────────────
struct FlightData {
  String callsign;
  String airline;    // resolved airline name (may be "")
  String type;       // aircraft type e.g. B789
  int altitude;      // feet
  float speed;       // knots
  float distance;    // km
  String direction;  // cardinal heading string e.g. "NW"
};

const int MAX_FLIGHTS = 10;
FlightData nearbyFlights[MAX_FLIGHTS];
int totalNearby = 0;
int currentIndex = 0;

unsigned long lastApiFetch = 0;
const unsigned long apiInterval = 5000;

// ─────────────────────────────────────────────────────────────
//  Haversine distance (km)
// ─────────────────────────────────────────────────────────────
float getDistance(float lat1, float lon1, float lat2, float lon2) {
  float R = 6371.0;
  float dLat = radians(lat2 - lat1);
  float dLon = radians(lon2 - lon1);
  float a = sin(dLat / 2) * sin(dLat / 2)
            + cos(radians(lat1)) * cos(radians(lat2))
                * sin(dLon / 2) * sin(dLon / 2);
  return R * 2 * atan2(sqrt(a), sqrt(1 - a));
}

// ─────────────────────────────────────────────────────────────
//  Setup
// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  u8g2.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
}

// ─────────────────────────────────────────────────────────────
//  Display
//
//  Line 1  (y=12):  "ETD335 - B789          1/2"
//  Line 2  (y=26):  " Etihad Airways"           (or blank if unknown)
//  Line 3  (y=40):  "NW | 5850 ft | 261 kt"
//  Line 4  (y=54):  "dist: xx.x km"
// ─────────────────────────────────────────────────────────────
void updateDisplay() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);  // 6×10 px, fits 21 chars across 128 px

  if (totalNearby == 0) {
    u8g2.drawStr(20, 30, "No aircraft(s)");
    u8g2.drawStr(30, 45, "nearby");
    u8g2.sendBuffer();
    return;
  }

  FlightData &f = nearbyFlights[currentIndex];

  // ── Line 1: callsign - type  ···  page ──────────────────────
  // Build page string first so we can right-align it
  char pageStr[8] = "";
  if (totalNearby > 1) {
    sprintf(pageStr, "%d/%d", currentIndex + 1, totalNearby);
  }

  // "ETD335 - B789"
  char leftPart[22];
  snprintf(leftPart, sizeof(leftPart), "%s - %s",
           f.callsign.c_str(), f.type.c_str());

  u8g2.drawStr(0, 12, leftPart);

  if (totalNearby > 1) {
    // Right-align page string (each char is 6 px wide)
    int pageX = 128 - (strlen(pageStr) * 6);
    u8g2.drawStr(pageX, 12, pageStr);
  }

  // ── Line 2: airline name ─────────────────────────────────────
  char line2[24];
  snprintf(line2, sizeof(line2), "%s", f.airline.c_str());
  u8g2.drawStr(0, 26, line2);

  // ── Line 3: DIR | alt ft | spd kt ────────────────────────────
  char line3[32];
  if (f.speed > 0) {
    snprintf(line3, sizeof(line3), "%s | %d ft | %.0f kt",
             f.direction.c_str(), f.altitude, f.speed);
  } else {
    snprintf(line3, sizeof(line3), "%s | %d ft | N/A kt",
             f.direction.c_str(), f.altitude);
  }
  u8g2.drawStr(0, 40, line3);

  // ── Line 4: dist ─────────────────────────────────────────────
  char line4[24];
  snprintf(line4, sizeof(line4), "Dist: %.1f km", f.distance);
  u8g2.drawStr(0, 54, line4);

  u8g2.sendBuffer();
}

// ─────────────────────────────────────────────────────────────
//  Main loop
// ─────────────────────────────────────────────────────────────
void loop() {
  // ── Button debounce ─────────────────────────────────────────
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) lastDebounceTime = millis();

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW && totalNearby > 1) {
        currentIndex = (currentIndex + 1) % totalNearby;
        updateDisplay();
      }
    }
  }
  lastButtonState = reading;

  // ── Periodic API fetch ──────────────────────────────────────
  if (millis() - lastApiFetch >= apiInterval || lastApiFetch == 0) {
    lastApiFetch = millis();

    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    http.begin(apiUrl);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      DynamicJsonDocument doc(8192);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        JsonArray flights = doc["ac"];

        struct TempFlight {
          JsonObject obj;
          float dist;
        };
        TempFlight tempArr[MAX_FLIGHTS + 1];
        int tempCount = 0;

        for (JsonObject f : flights) {
          if (!f["lat"] || !f["lon"] || !f["alt_baro"]) continue;

          float lat = f["lat"];
          float lon = f["lon"];
          float altitude = f["alt_baro"];
          float dist = getDistance(myLat, myLon, lat, lon);

          if (altitude > 0 && dist < 50) {
            if (tempCount < MAX_FLIGHTS) {
              tempArr[tempCount++] = { f, dist };
            } else {
              // Replace furthest if this one is closer
              int maxIdx = 0;
              for (int i = 1; i < MAX_FLIGHTS; i++)
                if (tempArr[i].dist > tempArr[maxIdx].dist) maxIdx = i;
              if (dist < tempArr[maxIdx].dist)
                tempArr[maxIdx] = { f, dist };
            }
          }
        }

        // Sort by distance (closest first) — bubble sort
        for (int i = 0; i < tempCount - 1; i++)
          for (int j = 0; j < tempCount - i - 1; j++)
            if (tempArr[j].dist > tempArr[j + 1].dist) {
              TempFlight t = tempArr[j];
              tempArr[j] = tempArr[j + 1];
              tempArr[j + 1] = t;
            }

        // Populate final array
        totalNearby = tempCount;
        for (int i = 0; i < totalNearby; i++) {
          JsonObject &bf = tempArr[i].obj;

          String callsign = bf["flight"] | "N/A";
          callsign.trim();
          nearbyFlights[i].callsign = callsign;

          nearbyFlights[i].airline = String(bf["airline"] | ""); 

          const char *type = bf["t"] | "UNK";
          nearbyFlights[i].type = String(type);

          nearbyFlights[i].altitude = bf["alt_baro"] | 0;

          // Speed: prefer ground speed, fall back to TAS / IAS
          float spd = 0;
          if (bf["gs"] && bf["gs"].as<float>() > 0) spd = bf["gs"];
          else if (bf["tas"] && bf["tas"].as<float>() > 0) spd = bf["tas"];
          else if (bf["ias"] && bf["ias"].as<float>() > 0) spd = bf["ias"];
          nearbyFlights[i].speed = spd;

          nearbyFlights[i].distance = tempArr[i].dist;

          // Track → cardinal direction
          float track = bf["track"] | -1.0f;
          nearbyFlights[i].direction = (track >= 0) ? getCardinal(track) : "?";
        }

        if (currentIndex >= totalNearby) currentIndex = 0;
        updateDisplay();

      } else {
        Serial.println("JSON parse error");
      }
    } else {
      Serial.println("HTTP Error");
    }
    http.end();
  }
}
