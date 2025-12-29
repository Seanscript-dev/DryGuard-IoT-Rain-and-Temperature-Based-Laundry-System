/*
 * DryGuard: IoT Rain and Temperature Based Laundry Saver
 * ESP32 Implementation with Web Server
 * 
 * Simplified Version - Removed RGB, Button, and IR Sensor
 * 
 * Components:
 * - Rain Sensor (GPIO 35) - Detect rain
 * - DHT11 (GPIO 4) - Temperature & Humidity
 * - Servo Motor (GPIO 13) - Cover control
 */

#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <ESP32Servo.h>

// WiFi Credentials - UPDATE THESE!
const char* ssid = "sean1234";
const char* password = "sean1234";

// Pin Definitions
#define RAIN_SENSOR_PIN 35
#define DHT_PIN 4
#define SERVO_PIN 13

// Rain Sensor Calibration
#define DRY_VALUE 4095    // Maximum reading when completely dry
#define WET_THRESHOLD 1500    // Lower this value for more sensitivity
#define RAIN_DEBOUNCE_TIME 3000  // 3-second debounce to avoid false triggers
#define DRY_THRESHOLD 2500     // Value above which we consider it dry

// DHT11 Configuration
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

// Servo Configuration
Servo coverServo;

// Web Server
WebServer server(80);

// Global Variables
float temperature = 0.0;
float humidity = 0.0;
bool isRaining = false;
int servoPosition = 0; // 0 = protected, 100 = exposed
bool isExposed = false;
bool manualOverride = false; // To prevent auto-control when manually set
float dryingProgress = 0.0;
unsigned long dryingStartTime = 0;
bool wasDrying = false;
bool autoReturnWhenDry = false; // Flag for auto-return when dry

// Rain sensor variables
unsigned long lastRainChange = 0;
bool rainLastState = false;
int rainSensorValue = 0;

// Timing
const unsigned long SENSOR_UPDATE_INTERVAL = 2000;
unsigned long lastSensorUpdate = 0;

// HTML Page (stored in PROGMEM to save RAM)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DryGuard - IoT Laundry Saver</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 900px;
            width: 100%;
            padding: 30px;
        }
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        .header h1 {
            color: #667eea;
            font-size: 2.5em;
            margin-bottom: 5px;
        }
        .grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 20px;
            margin-bottom: 30px;
        }
        .card {
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
            border-radius: 15px;
            padding: 25px;
            box-shadow: 0 5px 15px rgba(0,0,0,0.1);
        }
        .card-title {
            font-size: 0.9em;
            color: #666;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 10px;
        }
        .card-value {
            font-size: 2.5em;
            font-weight: bold;
            color: #333;
        }
        .status-card {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        .status-card .card-title { color: rgba(255,255,255,0.9); }
        .status-card .card-value { color: white; font-size: 2em; }
        .status-icon { font-size: 3em; margin-bottom: 10px; }
        .drying-status {
            text-align: center;
            padding: 30px;
            background: linear-gradient(135deg, #ffecd2 0%, #fcb69f 100%);
            border-radius: 15px;
            margin-bottom: 30px;
        }
        .drying-progress {
            background: white;
            height: 30px;
            border-radius: 15px;
            overflow: hidden;
            margin: 20px 0;
        }
        .progress-bar {
            height: 100%;
            background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
            transition: width 0.5s ease;
            display: flex;
            align-items: center;
            justify-content: flex-end;
            padding-right: 10px;
            color: white;
            font-weight: bold;
        }
        .expose-button {
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
            border: none;
            color: white;
            font-size: 1.3em;
            font-weight: bold;
            padding: 20px 40px;
            border-radius: 50px;
            cursor: pointer;
            box-shadow: 0 10px 25px rgba(245, 87, 108, 0.4);
            transition: transform 0.2s, opacity 0.2s;
            margin: 10px;
        }
        .expose-button:hover:not(:disabled) {
            transform: scale(1.05);
        }
        .expose-button:active:not(:disabled) {
            transform: scale(0.95);
        }
        .expose-button:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }
        .servo-status {
            margin-top: 20px;
            padding: 15px;
            background: #f0f0f0;
            border-radius: 10px;
            font-size: 1.1em;
        }
        .rain-sensor-value {
            margin-top: 10px;
            padding: 10px;
            background: #e8f4f8;
            border-radius: 8px;
            font-size: 0.9em;
            color: #555;
        }
        .auto-status {
            margin-top: 10px;
            padding: 10px;
            background: #f8f8e8;
            border-radius: 8px;
            font-size: 0.9em;
            color: #555;
        }
        @media (max-width: 768px) {
            .grid { grid-template-columns: 1fr; }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌤️ DryGuard</h1>
            <p>IoT Rain & Temperature Based Laundry Saver</p>
        </div>
        <div class="grid">
            <div class="card">
                <div class="card-title">Temperature</div>
                <div class="card-value" id="temperature">--</div>
            </div>
            <div class="card">
                <div class="card-title">Humidity</div>
                <div class="card-value" id="humidity">--</div>
            </div>
            <div class="card status-card">
                <div class="card-title">Weather Status</div>
                <div class="status-icon" id="statusIcon">☀️</div>
                <div class="card-value" id="weatherStatus">Sunny</div>
            </div>
            <div class="card status-card">
                <div class="card-title">Cover Status</div>
                <div class="status-icon" id="coverIcon">🛡️</div>
                <div class="card-value" id="coverStatus">Protected</div>
            </div>
        </div>
        <div class="drying-status">
            <h2>🧺 Laundry Drying Status</h2>
            <div style="font-size: 1.5em; font-weight: bold;" id="dryingStatus">Not Drying</div>
            <div class="drying-progress">
                <div class="progress-bar" id="progressBar" style="width: 0%;">
                    <span id="progressText">0%</span>
                </div>
            </div>
            <div id="timeRemaining">Calculating...</div>
        </div>
        <div style="text-align: center;">
            <button class="expose-button" onclick="toggleExpose()" id="exposeBtn">☀️ EXPOSE AND DRY</button>
            <button class="expose-button" onclick="protectLaundry()" id="protectBtn" style="background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);">🛡️ PROTECT LAUNDRY</button>
            <div class="servo-status" id="servoStatus">
                Servo Position: <strong id="servoPosition">0°</strong>
            </div>
            <div class="rain-sensor-value" id="rainSensorValue">
                Rain Sensor: <strong id="rainValue">0</strong> (Lower = More Wet, Threshold: <span id="rainThreshold">1500</span>)
            </div>
            <div class="auto-status" id="autoStatus">
                Auto Mode: <strong id="autoMode">Active</strong>
            </div>
        </div>
    </div>
    <script>
        function updateData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('temperature').textContent = data.temp.toFixed(1) + '°C';
                    document.getElementById('humidity').textContent = data.hum.toFixed(1) + '%';
                    document.getElementById('weatherStatus').textContent = data.status;
                    document.getElementById('statusIcon').textContent = data.icon;
                    document.getElementById('coverStatus').textContent = data.coverStatus;
                    document.getElementById('coverIcon').textContent = data.coverIcon;
                    document.getElementById('dryingStatus').textContent = data.dryingStatus;
                    document.getElementById('progressBar').style.width = data.progress + '%';
                    document.getElementById('progressText').textContent = Math.floor(data.progress) + '%';
                    document.getElementById('timeRemaining').textContent = data.timeRemaining;
                    document.getElementById('servoPosition').textContent = data.servoPos + '°';
                    document.getElementById('rainValue').textContent = data.rainSensor;
                    document.getElementById('rainThreshold').textContent = data.rainThreshold;
                    document.getElementById('autoMode').textContent = data.manualOverride ? 'Manual' : 'Active';
                    
                    // Update button states
                    const exposeBtn = document.getElementById('exposeBtn');
                    const protectBtn = document.getElementById('protectBtn');
                    
                    exposeBtn.textContent = data.isExposed ? '🛡️ PROTECT LAUNDRY' : '☀️ EXPOSE AND DRY';
                    exposeBtn.style.background = data.isExposed 
                        ? 'linear-gradient(135deg, #4facfe 0%, #00f2fe 100%)'
                        : 'linear-gradient(135deg, #f093fb 0%, #f5576c 100%)';
                    
                    // Disable buttons if raining
                    if (data.isRaining) {
                        exposeBtn.disabled = true;
                        protectBtn.disabled = true;
                        exposeBtn.title = "Cannot operate while raining";
                        protectBtn.title = "Cannot operate while raining";
                    } else {
                        exposeBtn.disabled = false;
                        protectBtn.disabled = false;
                        exposeBtn.title = "";
                        protectBtn.title = "";
                    }
                })
                .catch(error => console.error('Error fetching data:', error));
        }
        
        function toggleExpose() {
            if (!document.getElementById('exposeBtn').disabled) {
                fetch('/toggle')
                    .then(() => updateData())
                    .catch(error => console.error('Error toggling:', error));
            }
        }
        
        function protectLaundry() {
            if (!document.getElementById('protectBtn').disabled) {
                fetch('/protect')
                    .then(() => updateData())
                    .catch(error => console.error('Error protecting:', error));
            }
        }
        
        // Update data every 2 seconds
        setInterval(updateData, 2000);
        updateData(); // Initial update
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  // Initialize sensors and actuators
  pinMode(RAIN_SENSOR_PIN, INPUT);
  
  // Initialize DHT sensor
  dht.begin();
  delay(2000);  // Give DHT11 time to initialize
  
  // Initialize servo
  ESP32PWM::allocateTimer(0);
  coverServo.setPeriodHertz(50);      // Standard 50Hz servo
  coverServo.attach(SERVO_PIN, 500, 2400); // Adjust pulse width if needed
  
  // Initialize servo to 0 degrees (protected position)
  moveServo(0, false);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  unsigned long wifiStartTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < 15000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi!");
    while (1) {
      delay(1000);
    }
  }
  
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Setup web server routes
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", index_html);
  });
  
  server.on("/data", HTTP_GET, []() {
    String json = getSensorDataJSON();
    server.send(200, "application/json", json);
  });
  
  server.on("/toggle", HTTP_GET, []() {
    if (!isRaining) {  // Only allow manual control when not raining
      toggleServo();
      manualOverride = true;  // Set manual override
      server.send(200, "text/plain", "OK");
    } else {
      server.send(403, "text/plain", "Cannot operate while raining");
    }
  });
  
  server.on("/protect", HTTP_GET, []() {
    if (!isRaining) {  // Only allow manual control when not raining
      moveServo(0, true);
      manualOverride = true;  // Set manual override
      server.send(200, "text/plain", "OK");
    } else {
      server.send(403, "text/plain", "Cannot operate while raining");
    }
  });
  
  server.on("/expose", HTTP_GET, []() {
    if (!isRaining) {  // Only allow manual control when not raining
      moveServo(100, true);
      manualOverride = true;  // Set manual override
      server.send(200, "text/plain", "OK");
    } else {
      server.send(403, "text/plain", "Cannot operate while raining");
    }
  });
  
  server.begin();
  Serial.println("Web server started");
  
  // Initial sensor reading
  updateSensors();
  Serial.println("System initialized. Servo range: 0-100 degrees");
}

void loop() {
  server.handleClient();
  
  unsigned long currentMillis = millis();
  
  // Update sensors periodically
  if (currentMillis - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
    lastSensorUpdate = currentMillis;
    updateSensors();
  }
  
  // Check rain sensor for automatic protection
  checkRainSensor();
  
  // Calculate drying progress
  calculateDryingProgress();
  
  // Auto-protect when laundry is dry (100%)
  if (dryingProgress >= 100.0 && isExposed && !autoReturnWhenDry) {
    Serial.println("Laundry is dry! Auto-protecting...");
    moveServo(0, false);
    autoReturnWhenDry = true;
    manualOverride = false; // Reset manual override
  }
  
  // Reset auto-return flag when laundry is exposed again
  if (!isExposed) {
    autoReturnWhenDry = false;
  }
  
  // Small delay to prevent watchdog timer issues
  delay(10);
}

void updateSensors() {
  // Read DHT11
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  // Validate readings
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT11 read error! Check wiring.");
    temperature = 0.0;
    humidity = 0.0;
    
    // Try reinitializing DHT if it fails multiple times
    static int dhtErrorCount = 0;
    dhtErrorCount++;
    if (dhtErrorCount > 5) {
      Serial.println("Reinitializing DHT sensor...");
      dht.begin();
      delay(2000);
      dhtErrorCount = 0;
    }
  } else {
    Serial.printf("DHT11 - Temp: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
  }
}

void checkRainSensor() {
  rainSensorValue = analogRead(RAIN_SENSOR_PIN);
  unsigned long currentMillis = millis();
  
  // Debounce check
  if (currentMillis - lastRainChange < RAIN_DEBOUNCE_TIME) {
    return;
  }
  
  // Rain sensor logic: lower value = more water detected
  bool currentlyRaining = (rainSensorValue < WET_THRESHOLD);
  
  if (currentlyRaining != rainLastState) {
    lastRainChange = currentMillis;
    rainLastState = currentlyRaining;
    
    if (currentlyRaining) {
      isRaining = true;
      Serial.println("Rain detected!");
      Serial.printf("Rain sensor value: %d (threshold: %d)\n", rainSensorValue, WET_THRESHOLD);
      
      // Auto-protect if exposed (override manual control)
      if (isExposed) {
        Serial.println("Auto-protecting laundry from rain!");
        moveServo(0, false);
        manualOverride = false; // Reset manual override for auto-control
      }
    } else {
      isRaining = false;
      Serial.println("Rain stopped - sensor is dry");
      Serial.printf("Rain sensor value: %d\n", rainSensorValue);
      
      // When rain stops and it's sunny (good drying conditions), auto-expose
      if (!manualOverride && humidity < 70 && temperature > 20) {
        Serial.println("Good drying conditions detected. Auto-exposing laundry...");
        moveServo(100, false);
      }
    }
  }
}

void toggleServo() {
  if (isExposed) {
    moveServo(0, true);  // Full protection
  } else {
    moveServo(100, true);  // Fully exposed (100 degrees)
  }
}

void moveServo(int angle, bool isManual) {
  // Make sure angle is within servo limits (0-100 degrees)
  angle = constrain(angle, 0, 100);
  
  // Don't move if already at target position
  if (servoPosition == angle) {
    return;
  }
  
  Serial.printf("Moving servo from %d° to %d° (Manual: %s)\n", 
                servoPosition, angle, isManual ? "Yes" : "No");
  
  // Move servo gradually to prevent stuttering
  int step = (angle > servoPosition) ? 5 : -5;
  for (int pos = servoPosition; pos != angle; pos += step) {
    // Break if we overshoot
    if ((step > 0 && pos > angle) || (step < 0 && pos < angle)) {
      break;
    }
    coverServo.write(pos);
    delay(30);  // Small delay for smooth movement
  }
  
  // Ensure final position is accurate
  coverServo.write(angle);
  delay(50);
  
  servoPosition = angle;
  isExposed = (angle >= 50);  // Consider exposed if >= 50 degrees
  
  Serial.printf("Servo moved to: %d° (Exposed: %s)\n", angle, isExposed ? "Yes" : "No");
  
  // Reset drying progress when covered
  if (!isExposed && dryingProgress > 0) {
    dryingProgress = 0.0;
    wasDrying = false;
    Serial.println("Drying progress reset (laundry covered)");
  }
  
  // Add a small stabilization delay
  delay(100);
}

void calculateDryingProgress() {
  if (isRaining || !isExposed) {
    // Not drying - decrease progress slightly if raining
    if (isRaining && dryingProgress > 0) {
      dryingProgress = max(0.0f, dryingProgress - 0.2f);
    }
    wasDrying = false;
    return;
  }
  
  // Start timing when drying begins
  if (!wasDrying && dryingProgress == 0) {
    dryingStartTime = millis();
    wasDrying = true;
    Serial.println("Drying started!");
  }
  
  // Calculate drying rate based on temperature and humidity
  // Optimal drying: high temperature (>25°C) and low humidity (<60%)
  float tempFactor = constrain((temperature - 10.0f) / 25.0f, 0.0f, 1.0f); // 0-1 scale
  float humidityFactor = constrain((100.0f - humidity) / 70.0f, 0.0f, 1.0f); // 0-1 scale
  float dryingRate = (tempFactor + humidityFactor) / 2.0f;
  
  // Add bonus for good conditions
  if (temperature > 25 && humidity < 60) {
    dryingRate *= 1.5f; // Faster drying in good conditions
  }
  
  // Increase progress based on drying rate
  dryingProgress += dryingRate * 0.15f;
  dryingProgress = min(100.0f, dryingProgress);
  
  if (dryingProgress >= 100.0f) {
    Serial.println("Laundry is completely dry!");
  } else if (millis() - dryingStartTime > 60000) { // Log every minute
    Serial.printf("Drying: %.1f%% (Rate: %.2f, Temp: %.1f°C, Hum: %.1f%%)\n", 
                  dryingProgress, dryingRate, temperature, humidity);
    dryingStartTime = millis(); // Reset timer
  }
}

String getSensorDataJSON() {
  // Read fresh rain sensor value
  rainSensorValue = analogRead(RAIN_SENSOR_PIN);
  
  // Determine weather status
  String status, icon;
  if (isRaining) {
    status = "Raining";
    icon = "🌧️";
  } else if (humidity > 75) {
    status = "Humid";
    icon = "💧";
  } else if (humidity > 60 && temperature < 25) {
    status = "Cloudy";
    icon = "☁️";
  } else if (temperature > 25 && humidity < 60) {
    status = "Perfect for Drying";
    icon = "☀️🌤️";
  } else {
    status = "Sunny";
    icon = "☀️";
  }
  
  // Determine cover status
  String coverStatus, coverIcon, dryingStatus;
  if (isRaining) {
    coverStatus = "Protected from Rain";
    coverIcon = "🌧️🛡️";
    dryingStatus = "Not Drying (Raining)";
  } else if (!isExposed) {
    coverStatus = "Protected";
    coverIcon = "🛡️";
    dryingStatus = "Not Drying (Covered)";
  } else if (dryingProgress < 30) {
    coverStatus = "Exposed - Wet";
    coverIcon = "☀️💧";
    dryingStatus = "Drying (Wet)";
  } else if (dryingProgress < 70) {
    coverStatus = "Exposed - Damp";
    coverIcon = "☀️🌬️";
    dryingStatus = "Drying (Damp)";
  } else if (dryingProgress < 100) {
    coverStatus = "Exposed - Almost Dry";
    coverIcon = "☀️✅";
    dryingStatus = "Drying (Almost Dry)";
  } else {
    coverStatus = "Exposed - COMPLETELY DRY";
    coverIcon = "☀️🏁";
    dryingStatus = "COMPLETELY DRY!";
  }
  
  // Calculate estimated time
  String timeRemaining;
  if (dryingProgress >= 100) {
    timeRemaining = "Laundry is dry! ✅";
  } else if (!isExposed || isRaining) {
    timeRemaining = "Not drying (protected)";
  } else {
    // Estimate remaining time
    float remainingProgress = 100.0 - dryingProgress;
    float tempFactor = constrain((temperature - 10.0f) / 25.0f, 0.0f, 1.0f);
    float humidityFactor = constrain((100.0f - humidity) / 70.0f, 0.0f, 1.0f);
    float dryingRate = (tempFactor + humidityFactor) / 2.0;
    
    if (dryingRate > 0.01 && dryingProgress > 0) {
      float progressPerMinute = dryingRate * 0.15f * 30; // Convert to % per minute
      int minutesRemaining = (int)(remainingProgress / progressPerMinute);
      int hours = minutesRemaining / 60;
      int mins = minutesRemaining % 60;
      
      if (hours > 0) {
        timeRemaining = "Estimated: " + String(hours) + "h " + String(mins) + "m";
      } else {
        timeRemaining = "Estimated: " + String(mins) + "m";
      }
    } else {
      timeRemaining = "Estimated: >5h (poor conditions)";
    }
  }
  
  String json = "{";
  json += "\"temp\":" + String(temperature) + ",";
  json += "\"hum\":" + String(humidity) + ",";
  json += "\"rainSensor\":" + String(rainSensorValue) + ",";
  json += "\"rainThreshold\":" + String(WET_THRESHOLD) + ",";
  json += "\"status\":\"" + status + "\",";
  json += "\"icon\":\"" + icon + "\",";
  json += "\"coverStatus\":\"" + coverStatus + "\",";
  json += "\"coverIcon\":\"" + coverIcon + "\",";
  json += "\"dryingStatus\":\"" + dryingStatus + "\",";
  json += "\"progress\":" + String(dryingProgress) + ",";
  json += "\"timeRemaining\":\"" + timeRemaining + "\",";
  json += "\"servoPos\":" + String(servoPosition) + ",";
  json += "\"isExposed\":" + String(isExposed ? "true" : "false") + ",";
  json += "\"isRaining\":" + String(isRaining ? "true" : "false") + ",";
  json += "\"manualOverride\":" + String(manualOverride ? "true" : "false");
  json += "}";
  
  return json;
}