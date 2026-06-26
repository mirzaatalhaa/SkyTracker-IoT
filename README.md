# 🛰️ SkyTracker IoT

A real-time aircraft tracker built with an ESP32 and a 128×64 OLED display. SkyTracker automatically detects aircraft flying within 50 km of your location and displays live details — callsign, airline name, altitude, speed, direction, and distance. Press a button to cycle through all detected aircraft.

No pointing needed. The device passively receives flight data via an AWS cloud backend that queries live ADS-B data and resolves airline names from a 1000+ airline database stored in S3.

The device talks to an AWS cloud backend that fetches live flight data and resolves airline names from a 1000+ airline database stored in S3.

---

## 📁 Project Structure

```
SkyTracker/
├── firmware/                    # ESP32 Arduino sketch (C++)
├── aws/                         # Lambda function (JavaScript)
│   └── index.mjs                # Flight fetch + airline enrichment
├── scripts/
│   └── convert_airlines.py      # Generates airlines.json for S3
├── diagram/                     # Architecture and system diagrams
└── images/                      # Project images and media
```

---

## 🔧 Hardware

| Component | Details |
|-----------|---------|
| Microcontroller | ESP32 |
| Display | SH1106 128×64 OLED (I2C) |
| Button | Momentary push button on GPIO 19 |

**Wiring:**

```
OLED SDA  →  ESP32 GPIO 21
OLED SCL  →  ESP32 GPIO 22
OLED VCC  →  3.3V
OLED GND  →  GND

Button    →  GPIO 19 (INPUT_PULLUP, other leg to GND)
```

### Prerequisites

- Arduino IDE or [PlatformIO](https://platformio.org/)
- Install these libraries via Arduino Library Manager:
  - `U8g2` — OLED display driver
  - `ArduinoJson` — JSON parsing
  - `HTTPClient` — built into ESP32 Arduino core

### Configuration

In `firmware/skytracker.ino`, update these values before flashing:

```cpp
const char *ssid     = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
const char *apiUrl   = "YOUR_API_GATEWAY_URL/flights";

const float myLat = your latitude  // Eg. 5.34545
const float myLon = your longitude  // Eg. 7.34654
```

### Flashing

```bash
# Using PlatformIO
cd firmware
pio run --target upload

# Or open firmware/skytracker.ino in Arduino IDE and click Upload
```

---

## ☁️ AWS Backend

### Architecture

```
ESP32
  └── HTTPS GET → API Gateway
                      └── Lambda (index.mjs)
                            ├── Fetches flights from airplanes.live
                            ├── Loads airline map from S3 (cached in memory)
                            └── Returns enriched JSON with airline names
```

### Prerequisites

- AWS account with CLI configured (`aws configure`)
- Node.js v16+

### Step 1 — S3 Bucket (Airline Database)

Generate the airline lookup file:

```bash
cd scripts
python convert_airlines.py
# outputs airlines.json (~1085 active airlines)
```

Then manually add newer Indian carriers missing from the database:

```json
"AIX": "Air India Express",
"AKJ": "Akasa Air",
"GOA": "Fly91"
```

Create an S3 bucket (e.g. `skytracker-airlines`, region `ap-south-1`) and upload:

```bash
aws s3 cp airlines.json s3://skytracker-airlines/airlines.json
```

### Step 2 — Lambda Function

- Runtime: Node.js (ES Modules)
- Region: `ap-south-1`
- Handler: `index.handler`
- Deploy the code from `aws/index.mjs`
- Set the `BUCKET` constant to your bucket name

### Step 3 — IAM Permission

Attach `AmazonS3ReadOnlyAccess` to your Lambda's execution role so it can read `airlines.json` from S3.

### Step 4 — API Gateway

- Create an HTTP API
- Route: `GET /flights`
- Integration: your Lambda function
- Deploy and paste the endpoint URL into the firmware as `apiUrl`

---

## 🖥️ OLED Display Layout

```
ETD335 - B789              1/2
Etihad Airways
NW|5850ft|261kt
dist: 12.3 km
```

| Line | Content |
|------|---------|
| 1 | Callsign · Aircraft type · Page indicator |
| 2 | Airline name |
| 3 | Direction · Altitude · Speed |
| 4 | Distance from your location |

---

## 🛠️ Tech Stack

| Layer | Technology |
|-------|-----------|
| Firmware | C++ (ESP32 Arduino) |
| Cloud | JavaScript / AWS Lambda |
| Flight Data | [airplanes.live](https://airplanes.live) API |
| Airline DB | OpenFlights dataset, hosted on S3 |
| API | AWS API Gateway (HTTP) |

---

## 🚀 Getting Started

1. **Wire the hardware** — Connect the OLED and button as per the wiring table above
2. **Generate airlines.json** — Run `convert_airlines.py` and upload to S3
3. **Deploy Lambda** — Deploy `aws/index.mjs` and set up API Gateway
4. **Flash the firmware** — Update credentials and flash to your ESP32
5. **Power it up** — Watch live aircraft appear on the display automatically!

---

## 🖼️ Images

Project photos, hardware setups, and demo screenshots are in the `images/` directory.

---

## 👤 Author

**mirzaatalhaa**  
GitHub: [@mirzaatalhaa](https://github.com/mirzaatalhaa)
