# ESP32 Simple Thermostat - Development Guide

This guide helps developers understand, modify, and extend the ESP32 Simple Thermostat project.

## 🎯 Development Environment Setup

### Prerequisites
- **VS Code** with PlatformIO extension
- **Git** for version control
- **KiCad 6.0+** for PCB modifications (optional)
- **MQTT Client** for testing (MQTT.fx, MQTT Explorer, etc.)

### Initial Setup
```bash
# Clone the repository
git clone <repository-url>
cd ESP32-Simple-Thermostat

# Open in VS Code
code .

# PlatformIO will automatically:
# - Install ESP32 platform
# - Download required libraries
# - Configure build environment
```

### Hardware Setup for Development
1. **ESP32 Development Board**: Any ESP32 WROOM-32 compatible board
2. **ILI9341 TFT Display**: 320x240 with touch (SPI interface)
3. **DHT11 Sensor**: For temperature/humidity testing
4. **Breadboard/Jumper Wires**: For prototyping connections
5. **5V Relay Module**: For HVAC simulation (optional)

## 🏗️ Code Architecture

### Main Components

#### Core State Management
```cpp
// Global state variables - central to system operation
bool heatingOn = false;
bool coolingOn = false; 
bool fanOn = false;
String thermostatMode = "off";    // "off", "heat", "cool", "auto"
String fanMode = "auto";          // "auto", "on", "cycle"
```

#### Settings Persistence Pattern
```cpp
// Save settings to flash memory
void saveSettings() {
    preferences.putFloat("setHeat", setTempHeat);
    preferences.putString("thermoMd", thermostatMode);
    // ... save all configurable parameters
}

// Load settings from flash memory  
void loadSettings() {
    setTempHeat = preferences.getFloat("setHeat", 72.0);  // default: 72.0
    thermostatMode = preferences.getString("thermoMd", "off");  // default: "off"
    // ... load all parameters with defaults
}
```

#### Non-blocking Task Scheduling
```cpp
void loop() {
    static unsigned long lastDisplayUpdate = 0;
    static unsigned long lastSensorRead = 0;
    
    unsigned long currentTime = millis();
    
    // Update display every 1000ms
    if (currentTime - lastDisplayUpdate > 1000) {
        updateDisplay(currentTemp, currentHumidity);
        lastDisplayUpdate = currentTime;
    }
    
    // Read sensors every 2000ms
    if (currentTime - lastSensorRead > 2000) {
        // Read sensors and control relays
        lastSensorRead = currentTime;
    }
}
```

### Key Function Categories

#### 1. Display Functions
```cpp
void updateDisplay(float temp, float humidity);     // Main display update
void drawButtons();                                 // Render touch buttons
void handleButtonPress(uint16_t x, uint16_t y);   // Process touch input
void drawKeyboard(bool uppercase);                 // WiFi setup keyboard
```

#### 2. HVAC Control Functions  
```cpp
void controlRelays(float currentTemp);             // Main control logic
void activateHeating();                           // Multi-stage heating
void activateCooling();                           // Multi-stage cooling
void handleFanControl();                          // Fan operation
void controlFanSchedule();                        // Scheduled fan cycling
```

#### 3. Communication Functions
```cpp
void setupMQTT();                                 // MQTT initialization
void mqttCallback(char* topic, byte* payload, unsigned int length);
void sendMQTTData();                              // Publish sensor data
void handleWebRequests();                        // Web server setup
```

## 🔧 Common Development Tasks

### Adding New Configuration Settings

1. **Add Global Variable**
```cpp
// Add near other global variables
bool newFeatureEnabled = false;
float newParameterValue = 10.0;
```

2. **Update Settings Functions**
```cpp
void saveSettings() {
    // Add to existing function
    preferences.putBool("newFeat", newFeatureEnabled);
    preferences.putFloat("newParam", newParameterValue);
}

void loadSettings() {
    // Add to existing function  
    newFeatureEnabled = preferences.getBool("newFeat", false);  // default: false
    newParameterValue = preferences.getFloat("newParam", 10.0); // default: 10.0
}

void restoreDefaultSettings() {
    // Add to existing function
    newFeatureEnabled = false;
    newParameterValue = 10.0;
}
```

3. **Add Web Interface Support**
```cpp
// In handleWebRequests() function, add to settings page HTML:
html += "New Feature: <input type='checkbox' name='newFeature' " + 
        String(newFeatureEnabled ? "checked" : "") + "><br>";
html += "New Parameter: <input type='text' name='newParam' value='" + 
        String(newParameterValue) + "'><br>";

// In /set POST handler, add parameter processing:
if (request->hasParam("newFeature", true)) {
    newFeatureEnabled = request->getParam("newFeature", true)->value() == "on";
} else {
    newFeatureEnabled = false;
}
if (request->hasParam("newParam", true)) {
    newParameterValue = request->getParam("newParam", true)->value().toFloat();
}
```

### Extending MQTT Functionality

1. **Add New Status Topics**
```cpp
void sendMQTTData() {
    static bool lastFeatureState = false;
    
    // Add to existing function
    if (newFeatureEnabled != lastFeatureState) {
        mqttClient.publish("esp32_thermostat/new_feature", 
                          String(newFeatureEnabled).c_str(), true);
        lastFeatureState = newFeatureEnabled;
    }
}
```

2. **Add Command Topics**
```cpp
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    // Add to existing function
    if (String(topic) == "esp32_thermostat/new_feature/set") {
        newFeatureEnabled = (message == "true" || message == "1");
        saveSettings();
    }
}

void reconnectMQTT() {
    if (mqttClient.connect(hostname.c_str(), mqttUsername.c_str(), mqttPassword.c_str())) {
        // Add to existing subscriptions
        mqttClient.subscribe("esp32_thermostat/new_feature/set");
    }
}
```

3. **Add Home Assistant Discovery**
```cpp
void publishHomeAssistantDiscovery() {
    // Add new sensor discovery
    String configTopic = "homeassistant/binary_sensor/esp32_thermostat_new_feature/config";
    StaticJsonDocument<512> doc;
    doc["name"] = "Thermostat New Feature";
    doc["unique_id"] = "esp32_thermostat_new_feature";
    doc["state_topic"] = "esp32_thermostat/new_feature";
    
    JsonObject device = doc.createNestedObject("device");
    device["identifiers"] = "esp32_thermostat";
    device["name"] = hostname;
    
    char buffer[512];
    serializeJson(doc, buffer);
    mqttClient.publish(configTopic.c_str(), buffer, true);
}
```

### Adding Display Elements

1. **Update Main Display**
```cpp
void updateDisplay(float currentTemp, float currentHumidity) {
    // Add after existing display updates
    static bool prevNewFeatureState = false;
    
    if (newFeatureEnabled != prevNewFeatureState) {
        // Clear area and update display
        tft.fillRect(10, 140, 100, 20, TFT_BLACK);
        
        if (newFeatureEnabled) {
            tft.fillRoundRect(10, 140, 100, 20, 5, TFT_GREEN);
            tft.setTextColor(TFT_WHITE);
            tft.setCursor(15, 145);
            tft.print("NEW FEATURE");
        }
        
        prevNewFeatureState = newFeatureEnabled;
    }
}
```

2. **Add Touch Button**
```cpp
void drawButtons() {
    // Add after existing buttons
    tft.fillRect(270, 100, 40, 30, TFT_PURPLE);
    tft.setCursor(275, 110);
    tft.setTextColor(TFT_WHITE);
    tft.print("NEW");
}

void handleButtonPress(uint16_t x, uint16_t y) {
    // Add button detection
    if (x > 270 && x < 310 && y > 100 && y < 130) {
        newFeatureEnabled = !newFeatureEnabled;
        saveSettings();
        sendMQTTData();
        updateDisplay(currentTemp, currentHumidity);
    }
}
```

## 🐛 Debugging Techniques

### Serial Debug Output
```cpp
// Add debug messages throughout code
Serial.println("Debug: Function started");
Serial.printf("Debug: Variable value = %.2f\n", someVariable);
Serial.print("Debug: String value = "); Serial.println(someString);

// Monitor at 115200 baud rate
// PlatformIO: Ctrl+Alt+S or use Serial Monitor
```

### Web Interface Debugging
```cpp
// Add debug endpoint to web server
server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><body><h1>Debug Info</h1>";
    html += "<p>Free Heap: " + String(ESP.getFreeHeap()) + "</p>";
    html += "<p>Uptime: " + String(millis() / 1000) + " seconds</p>";
    html += "<p>WiFi RSSI: " + String(WiFi.RSSI()) + " dBm</p>";
    html += "<p>Current Mode: " + thermostatMode + "</p>";
    html += "</body></html>";
    request->send(200, "text/html", html);
});
```

### MQTT Debug Messages
```cpp
void sendDebugMQTT(String message) {
    if (mqttClient.connected()) {
        mqttClient.publish("esp32_thermostat/debug", message.c_str());
    }
}

// Usage:
sendDebugMQTT("Heating activated: temp=" + String(currentTemp));
```

## 🧪 Testing Strategies

### Unit Testing Approach
```cpp
// Create test functions for key logic
void testTemperatureLogic() {
    Serial.println("Testing temperature control logic...");
    
    // Test heating activation
    thermostatMode = "heat";
    setTempHeat = 72.0;
    controlRelays(70.0);  // Should activate heating
    
    if (heatingOn) {
        Serial.println("✓ Heating activation test passed");
    } else {
        Serial.println("✗ Heating activation test failed");
    }
}

// Call from setup() during development
void setup() {
    // ... normal setup code ...
    
    #ifdef DEBUG_MODE
    testTemperatureLogic();
    #endif
}
```

### Integration Testing
1. **MQTT Testing**: Use MQTT client to send commands and verify responses
2. **Web Interface Testing**: Test all web endpoints with various parameters
3. **Hardware Testing**: Verify relay outputs with multimeter or LED indicators
4. **Touch Testing**: Verify all buttons respond correctly

### Load Testing
```cpp
// Test system under continuous operation
void stressTest() {
    static unsigned long testStartTime = millis();
    static int testCycles = 0;
    
    // Simulate rapid temperature changes
    if (millis() - testStartTime > 1000) {
        testCycles++;
        float testTemp = 70.0 + (testCycles % 10);
        controlRelays(testTemp);
        testStartTime = millis();
        
        if (testCycles > 100) {
            Serial.println("Stress test completed successfully");
            testCycles = 0;
        }
    }
}
```

## 🔐 Best Practices

### Memory Management
```cpp
// Use static buffers to avoid fragmentation
void handleLargeData() {
    static char buffer[1024];  // Reuse buffer
    // ... process data ...
}

// Check heap regularly
void checkMemory() {
    if (ESP.getFreeHeap() < 10000) {
        Serial.println("Warning: Low memory!");
    }
}
```

### Error Handling
```cpp
// Always validate sensor readings
float readTemperature() {
    float temp = dht.readTemperature(useFahrenheit);
    if (isnan(temp)) {
        Serial.println("Error: Invalid temperature reading");
        return previousTemp;  // Use last known good value
    }
    return temp;
}

// Handle WiFi disconnections gracefully
void checkWiFiStatus() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, entering offline mode");
        // Continue operation without network features
    }
}
```

### Code Organization
```cpp
// Group related constants
namespace ThermostatConstants {
    const float MIN_TEMP = 50.0;
    const float MAX_TEMP = 95.0;
    const unsigned long DISPLAY_UPDATE_INTERVAL = 1000;
    const unsigned long SENSOR_READ_INTERVAL = 2000;
}

// Use descriptive function names
void activateFirstStageHeating();
void checkForSecondStageRequirement();
void updateTemperatureDisplay();
```

## 🚀 Performance Optimization

### Display Updates
```cpp
// Only update changed elements
void updateDisplay(float temp, float humidity) {
    // Check if values actually changed before updating
    if (abs(temp - previousTemp) > 0.1) {
        updateTemperatureDisplay(temp);
        previousTemp = temp;
    }
}
```

### MQTT Optimization
```cpp
// Batch MQTT publishes to reduce network traffic
void sendMQTTBatch() {
    static unsigned long lastBatch = 0;
    
    if (millis() - lastBatch > 5000) {  // Every 5 seconds
        // Send all status updates at once
        sendMQTTData();
        lastBatch = millis();
    }
}
```

### Memory Efficiency
```cpp
// Use PROGMEM for large string constants
const char webPageTemplate[] PROGMEM = R"(
<html>
<head><title>Thermostat</title></head>
<body>%s</body>
</html>
)";
```

## 📦 Packaging and Distribution

### Firmware Preparation
```bash
# Build release version
pio run -e esp32dev

# Create firmware package
mkdir release
cp .pio/build/esp32dev/firmware.bin release/
cp README.md DOCUMENTATION.md release/
```

### PCB Manufacturing Package
The `jlcpcb/` folder contains complete manufacturing files:
- **Gerber files**: Ready for PCB fabrication
- **BOM**: Component sourcing list
- **CPL**: Pick-and-place assembly file

### Version Management
```cpp
// Update version in code
String sw_version = "1.0.4";  // Increment for releases

// Update in documentation
// Update in README.md
// Tag git repository: git tag v1.0.4
```

This development guide provides the foundation for extending and maintaining the ESP32 Simple Thermostat project. Follow these patterns and practices to ensure code quality and system reliability.