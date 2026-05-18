# 🛰️ SkyTracker

A hardware + cloud IoT project for real-time sky/satellite tracking. SkyTracker combines embedded firmware (C++) running on a microcontroller with an AWS cloud backend (JavaScript) to collect, transmit, and visualize tracking data.

---

## 📁 Project Structure

```
SkyTracker/
├── firmware/       # Embedded C++ code for the tracking hardware
├── aws/            # AWS cloud infrastructure and backend (JavaScript)
├── diagram/        # Architecture and system diagrams
└── images/         # Project images and media
```

---

## 🔧 Hardware (Firmware)

The `firmware/` directory contains the C++ source code for the microcontroller that handles:

- Sensor data acquisition (e.g., GPS, IMU, or pointing sensors)
- Real-time sky/object tracking logic
- Communication with the AWS cloud backend (MQTT / HTTP)

### Prerequisites

- A compatible microcontroller (e.g., ESP32, Arduino, or similar)
- [PlatformIO](https://platformio.org/) or Arduino IDE for building and flashing
- Required libraries listed in the firmware's configuration file

### Flashing the Firmware

```bash
# Using PlatformIO
cd firmware
pio run --target upload

# Or using Arduino IDE
# Open firmware/main.cpp (or .ino) and upload via the IDE
```

---

## ☁️ AWS Backend

The `aws/` directory contains the JavaScript-based cloud backend, which handles:

- Receiving data from the tracking device (via AWS IoT Core / API Gateway)
- Processing and storing telemetry data (e.g., DynamoDB, S3)
- Serving data to a dashboard or frontend

### Prerequisites

- [Node.js](https://nodejs.org/) (v16 or later recommended)
- [AWS CLI](https://aws.amazon.com/cli/) configured with appropriate credentials
- [AWS CDK](https://aws.amazon.com/cdk/) or Serverless Framework (if applicable)

### Setup

```bash
cd aws
npm install
```

### Deploy to AWS

```bash
# If using AWS CDK
cdk deploy

# If using Serverless Framework
serverless deploy
```

> **Note:** Make sure your AWS credentials are configured via `aws configure` before deploying.

---

## 🏗️ Architecture

See the `diagram/` folder for the full system architecture diagram.

The general flow is:

```
[Tracking Hardware]
       │
       │ MQTT / HTTPS
       ▼
[AWS IoT Core / API Gateway]
       │
       ▼
[Lambda / Processing]
       │
       ▼
[DynamoDB / S3]
       │
       ▼
[Dashboard / Visualization]
```

---

## 🖼️ Images

Project photos, hardware setups, and demo screenshots are available in the `images/` directory.

---

## 🚀 Getting Started

1. **Set up the hardware** — Wire up your microcontroller and sensors as per the hardware guide in `diagram/`.
2. **Flash the firmware** — Follow the [firmware instructions](#-hardware-firmware) above.
3. **Deploy the cloud backend** — Follow the [AWS setup instructions](#-aws-backend) above.
4. **Configure the device** — Update the firmware with your AWS IoT endpoint, credentials, and device settings.
5. **Monitor** — Watch your device send live tracking data to the cloud!

---

## 🛠️ Tech Stack

| Layer     | Technology          |
|-----------|---------------------|
| Firmware  | C++ (Embedded)      |
| Cloud     | JavaScript / AWS    |
| IoT Broker | AWS IoT Core       |
| Storage   | AWS DynamoDB / S3   |

---





## 👤 Author

**mirzaatalhaa**  
GitHub: [@mirzaatalhaa](https://github.com/mirzaatalhaa)
