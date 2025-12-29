# **DryGuard: An IoT Rain and Temperature Based Laundry Saver**

## 🌟 Introduction

DryGuard is a smart, automated solution designed to solve the common problem of outdoor laundry getting wet from unexpected rain. This Internet of Things (IoT) project utilizes an ESP32 microcontroller with sensors and actuators to monitor weather conditions and physically protect your laundry when rain is detected.

The system intelligently makes decisions based on real-time environmental data, offering both automatic protection and manual control through a beautifully designed web interface. Whether you're a homeowner who dries clothes outside, a student learning about IoT systems, or a maker looking for practical automation projects, DryGuard provides a complete, functional system that bridges the physical and digital worlds.

### Key Features:
- **🌧️ Rain Detection**: Uses an analog rain sensor to detect precipitation
- **🌡️ Environmental Monitoring**: Tracks temperature and humidity via DHT11 sensor
- **🤖 Smart Automation**: Automatically covers laundry when rain is detected
- **🧺 Drying Progress Tracking**: Calculates estimated drying time based on conditions
- **📱 Web Dashboard**: Modern, responsive interface accessible from any device
- **🔧 Manual Override**: User control when automatic decisions aren't desired

## 🔧 How to Use DryGuard

### Step 1: Hardware Setup
1. **Connect Components**:
   ```
   Rain Sensor → GPIO 35
   DHT11 → GPIO 4
   Servo Motor → GPIO 13
   ```
2. **Power** the ESP32 via USB or external 5V power supply
3. Ensure the servo is mechanically connected to your laundry cover mechanism

### Step 2: Software Configuration
1. **Update WiFi Credentials** in the code:
   ```cpp
   const char* ssid = "YOUR_WIFI_NAME";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
2. **Upload the Code** to your ESP32 using Arduino IDE or PlatformIO
3. Open Serial Monitor (115200 baud) to view initialization process

### Step 3: System Calibration
1. **Rain Sensor Calibration** (adjust in code if needed):
   - `DRY_VALUE`: Maximum reading when completely dry
   - `WET_THRESHOLD`: Lower for more sensitivity to moisture
2. **Test Servo Range** (0-100 degrees):
   - 0° = Fully covered/protected
   - 100° = Fully exposed for drying

### Step 4: Accessing the Web Interface
1. Note the IP address shown in Serial Monitor after ESP32 connects to WiFi
2. Open a web browser on any device connected to the same network
3. Navigate to: `http://[ESP32_IP_ADDRESS]`

### Step 5: Using the Dashboard
The dashboard provides real-time information and controls:

| Dashboard Section | Function |
|------------------|----------|
| **Weather Status** | Shows current conditions (Sunny, Raining, Humid, etc.) |
| **Cover Status** | Indicates if laundry is protected or exposed |
| **Drying Progress** | Visual bar showing drying completion percentage |
| **Manual Controls** | Buttons to expose or protect laundry manually |
| **Sensor Readings** | Live temperature, humidity, and rain sensor values |

### Step 6: Operating Modes
- **🌤️ Automatic Mode**: System decides when to expose/protect based on sensors
- **🔄 Manual Mode**: Override auto-decisions using web interface buttons
  - Click "EXPOSE AND DRY" to manually expose laundry
  - Click "PROTECT LAUNDRY" to manually cover laundry

### Step 7: Monitoring and Alerts
The system provides several automatic features:
- **Rain Protection**: Auto-covers laundry when rain detected
- **Drying Completion**: Auto-protects when laundry reaches 100% dry
- **Condition-Based Exposure**: Auto-exposes when conditions are favorable (sunny, low humidity)

## ⚠️ Important Notes
- The system prevents manual operation during active rain for safety
- Manual controls activate "manual override" mode, disabling auto-decisions
- Regular calibration may be needed for accurate rain detection
- Ensure mechanical components are securely mounted and weather-protected

## 🔍 Troubleshooting
- **No WiFi Connection**: Check credentials and network availability
- **Sensor Errors**: Verify wiring connections and pin assignments
- **Servo Not Moving**: Check power supply and PWM signal
- **Web Interface Unavailable**: Confirm ESP32 IP address and network connectivity

This system transforms the chore of managing outdoor laundry into a fully automated process, giving you peace of mind and dry clothes regardless of changing weather conditions. The open-source nature of the project also allows for customization and expansion based on your specific needs.
