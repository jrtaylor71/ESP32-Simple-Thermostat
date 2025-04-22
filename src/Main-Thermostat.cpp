/*
 * This file is part of ESP32-Simple-Thermostat.
 *
 * ESP32-Simple-Thermostat is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ESP32-Simple-Thermostat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Copyright (c) 2025 Jonn Taylor (jrtaylor@taylordatacom.com) */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <DHT.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <PubSubClient.h> // Include the MQTT library
#include <esp_task_wdt.h> // Include the watchdog timer library
#include <time.h>
#include <ArduinoJson.h> // Include the ArduinoJson library
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Update.h> // For OTA firmware update

// Constants
const int SECONDS_PER_HOUR = 3600;
const int WDT_TIMEOUT = 10; // Watchdog timer timeout in seconds
#define DHTPIN 22 // Define the pin where the DHT11 is connected
#define DHTTYPE DHT11 // Define the type of DHT sensor

// DS18B20 sensor setup
#define ONE_WIRE_BUS 27 // Define the pin where the DS18B20 is connected
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
float hydronicTemp = 0.0;
bool hydronicHeatingEnabled = false;

// Hydronic heating settings
float hydronicTempLow = 120.0; // Default low temperature for hydronic heating
float hydronicTempHigh = 130.0; // Default high temperature for hydronic heating

// Hybrid staging settings
unsigned long stage1MinRuntime = 300; // Default minimum runtime for first stage in seconds (5 minutes)
float stage2TempDelta = 2.0; // Default temperature delta for second stage activation
unsigned long stage1StartTime = 0; // Time when stage 1 was activated
bool stage1Active = false; // Flag to track if stage 1 is active
bool stage2Active = false; // Flag to track if stage 2 is active
bool stage2HeatingEnabled = true; // Enable/disable 2nd stage heating
bool stage2CoolingEnabled = true; // Enable/disable 2nd stage cooling

// Globals
DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(80);
TFT_eSPI tft = TFT_eSPI();
WiFiClient espClient;
PubSubClient mqttClient(espClient); // Initialize the MQTT client
Preferences preferences; // Preferences instance
String inputText = "";
bool isEnteringSSID = true;

// GPIO pins for relays
const int heatRelay1Pin = 13;
const int heatRelay2Pin = 12;
const int coolRelay1Pin = 14;
const int coolRelay2Pin = 26;
const int fanRelayPin = 25;

// Settings
float setTempHeat = 72.0; // Default set temperature for heating in Fahrenheit
float setTempCool = 76.0; // Default set temperature for cooling in Fahrenheit
float setTempAuto = 74.0; // Default set temperature for auto mode
float tempSwing = 1.0;
float autoTempSwing = 3.0;
bool autoChangeover = true;
bool fanRelayNeeded = false;
bool useFahrenheit = true; // Default to Fahrenheit
bool mqttEnabled = false; // Default to MQTT disabled
String wifiSSID = "";
String wifiPassword = "";
int fanMinutesPerHour = 15;                                                                     // Default to 15 minutes per hour
unsigned long lastFanRunTime = 0;                                                               // Time when the fan last ran
unsigned long fanRunDuration = 0;                                                               // Duration for which the fan has run in the current hour
unsigned long lastInteractionTime = 0;                                                          // Last interaction time
const float tempDifferential = 4.0; // Fixed differential between heat and cool for auto changeover
bool use24HourClock = true; // Default to 24-hour clock

// MQTT settings
String mqttServer = "0.0.0.0"; // Replace with your MQTT server
int mqttPort = 1883;                    // Replace with your MQTT port
String mqttUsername = "your_username";  // Replace with your MQTT username
String mqttPassword = "your_password";  // Replace with your MQTT password
String timeZone = "CST6CDT,M3.2.0,M11.1.0"; // Default time zone (Central Standard Time)

// Add a preference for hostname
String hostname = "ESP32-Simple-Thermostat"; // Default hostname
String sw_version = "1.0.0"; // Default software version


bool heatingOn = false;
bool coolingOn = false;
bool fanOn = false;
String thermostatMode = "off"; // Default thermostat mode
String fanMode = "auto"; // Default fan mode

// Add these declarations at the beginning of the file or in the appropriate section for global variables
bool settingsChanged = false;
bool mqttCallbackActive = false;
unsigned long lastMQTTMessageTime = 0;
const unsigned long mqttDebounceTime = 1000; // 1 second debounce time

bool tempHeatChanged = false;
bool tempCoolChanged = false;
bool tempSwingChanged = false;
bool thermostatModeChanged = false;
bool fanModeChanged = false;
bool handlingMQTTMessage = false; // Add this flag

// Function prototypes
void setupWiFi();
void controlRelays(float currentTemp);
void handleWebRequests();
void updateDisplay(float currentTemp, float currentHumidity);
void saveSettings();
void loadSettings();
void setupMQTT();
void reconnectMQTT();
float convertCtoF(float celsius);
void controlFanSchedule();
void saveWiFiSettings();
void drawKeyboard(bool isUpperCaseKeyboard);
void handleKeyPress(int row, int col);
void drawButtons();
void handleButtonPress(uint16_t x, uint16_t y);
void handleKeyboardTouch(uint16_t x, uint16_t y, bool isUpperCaseKeyboard);
void connectToWiFi();
void enterWiFiCredentials();
void calibrateTouchScreen();
void restoreDefaultSettings();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void sendMQTTData();
void publishHomeAssistantDiscovery();
void turnOffAllRelays();
void activateHeating();
void activateCooling();
void handleFanControl();

uint16_t calibrationData[5] = { 300, 3700, 300, 3700, 7 }; // Example calibration data

float currentTemp = 0.0;
float currentHumidity = 0.0;
bool isUpperCaseKeyboard = true;
float previousTemp = 0.0;
float previousHumidity = 0.0;
float previousSetTemp = 0.0;
bool firstHourAfterBoot = true; // Flag to track the first hour after bootup

void setup()
{
    Serial.begin(115200);

    // Initialize Preferences
    preferences.begin("thermostat", false);
    loadSettings();

    // Debug print to check if homeAssistantEnabled is loaded correctly
    // Serial.print("Home Assistant Enabled: ");
    // Serial.println(homeAssistantEnabled);

    // Initialize the DHT11 sensor
    dht.begin();

    // Initialize the TFT display
    tft.init();
    tft.setRotation(1); // Set the rotation of the display as needed
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 0);
    tft.println("Loading Settings...");

    // Calibrate touch screen
    calibrateTouchScreen();

    // Initialize relay pins
    pinMode(heatRelay1Pin, OUTPUT);
    pinMode(heatRelay2Pin, OUTPUT);
    pinMode(coolRelay1Pin, OUTPUT);
    pinMode(coolRelay2Pin, OUTPUT);
    pinMode(fanRelayPin, OUTPUT);

    // Ensure all relays are off during bootup
    digitalWrite(heatRelay1Pin, LOW);
    digitalWrite(heatRelay2Pin, LOW);
    digitalWrite(coolRelay1Pin, LOW);
    digitalWrite(coolRelay2Pin, LOW);
    digitalWrite(fanRelayPin, LOW);

    // Setup WiFi
    setupWiFi();

    // Start the web server
    handleWebRequests();
    server.begin();

    if (mqttEnabled) {
        setupMQTT();
    }
    lastInteractionTime = millis();

    // Initialize buttons
    drawButtons();

    // Initialize time from NTP server
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    setenv("TZ", timeZone.c_str(), 1);
    tzset();

    // Add a delay before attempting to connect to WiFi
    delay(5000);

    // Initial display update
    updateDisplay(currentTemp, currentHumidity);

    // Initialize the DS18B20 sensor
    ds18b20.begin();
}

void loop()
{
    static unsigned long lastWiFiAttemptTime = 0;
    static unsigned long lastMQTTAttemptTime = 0;
    static unsigned long lastDisplayUpdateTime = 0;
    const unsigned long displayUpdateInterval = 1000; // Update display every 

    // Feed the watchdog timer
    esp_task_wdt_reset();

    // Attempt to connect to WiFi if not connected
    if (WiFi.status() != WL_CONNECTED && millis() - lastWiFiAttemptTime > 10000)
    {
        connectToWiFi();
        lastWiFiAttemptTime = millis();
    }
    
    if (mqttEnabled)
    {
        // Attempt to reconnect to MQTT if not connected and WiFi is connected
        if (WiFi.status() == WL_CONNECTED && !mqttClient.connected() && millis() - lastMQTTAttemptTime > 5000)
        {
            reconnectMQTT();
            lastMQTTAttemptTime = millis();
        }
        mqttClient.loop();
        sendMQTTData();
    }
    else
    {
        if (mqttClient.connected())
        {
            mqttClient.disconnect();
        }
    }

    // Read sensor data if sensor is available
    currentTemp = dht.readTemperature(useFahrenheit);
    currentHumidity = dht.readHumidity();

    if (!isnan(currentTemp) && !isnan(currentHumidity))
    {
        // Control relays based on current temperature
        controlRelays(currentTemp);
    }

    // Control fan based on schedule
    controlFanSchedule();

    // Handle button presses
    uint16_t x, y;
    if (tft.getTouch(&x, &y))
    {
        Serial.print("Touch detected at: ");
        Serial.print(x);
        Serial.print(", ");
        Serial.println(y);
        handleButtonPress(x, y);
        handleKeyboardTouch(x, y, isUpperCaseKeyboard);
    }

    // Update display periodically
    if (millis() - lastDisplayUpdateTime > displayUpdateInterval)
    {
        updateDisplay(currentTemp, currentHumidity);
        lastDisplayUpdateTime = millis();
    }

    // Control relays based on current temperature
    controlRelays(currentTemp);

    // Read hydronic temperature
    ds18b20.requestTemperatures();
    hydronicTemp = ds18b20.getTempFByIndex(0);

    // delay(50); // Adjust delay as needed
}

void setupWiFi()
{
    hostname = preferences.getString("hostname", "ESP32-Simple-Thermostat"); // Load hostname from preferences
    WiFi.setHostname(hostname.c_str()); // Set the WiFi device name

    wifiSSID = preferences.getString("wifiSSID", "");
    wifiPassword = preferences.getString("wifiPassword", "");

    if (wifiSSID != "" && wifiPassword != "")
    {
        WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
        unsigned long startAttemptTime = millis();

        // Only try to connect for 10 seconds
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
        {
            delay(1000);
            Serial.println("Connecting to WiFi...");
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("Connected to WiFi");
        }
        else
        {
            Serial.println("Failed to connect to WiFi");
            enterWiFiCredentials();
        }
    }
    else
    {
        // No WiFi credentials found, prompt user to enter them via touch screen
        Serial.println("No WiFi credentials found. Please enter them via the touch screen.");
        enterWiFiCredentials();
    }
}

void enterWiFiCredentials()
{
    drawKeyboard(isUpperCaseKeyboard);
    while (WiFi.status() != WL_CONNECTED)
    {
        uint16_t x, y;
        if (tft.getTouch(&x, &y))
        {
            handleKeyboardTouch(x, y, isUpperCaseKeyboard);
        }
        delay(1000);
        Serial.println("Waiting for WiFi credentials...");
    }
}

void drawKeyboard(bool isUpperCaseKeyboard)
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 0);
    tft.println(isEnteringSSID ? "Enter SSID:" : "Enter Password:");
    tft.setCursor(0, 30);
    tft.println(inputText);

    const char* keys[5][10];
    if (isUpperCaseKeyboard)
    {
        const char* upperKeys[5][10] = {
            {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
            {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"},
            {"A", "S", "D", "F", "G", "H", "J", "K", "L", "DEL"},
            {"Z", "X", "C", "V", "B", "N", "M", "SPACE", "CLR", "OK"},
            {"!", "@", "#", "$", "%", "^", "&", "*", "(", "SHIFT"}
        };
        memcpy(keys, upperKeys, sizeof(upperKeys));
    }
    else
    {
        const char* lowerKeys[5][10] = {
            {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
            {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p"},
            {"a", "s", "d", "f", "g", "h", "j", "k", "l", "DEL"},
            {"z", "x", "c", "v", "b", "n", "m", "SPACE", "CLR", "OK"},
            {"!", "@", "#", "$", "%", "^", "&", "*", "(", "SHIFT"}
        };
        memcpy(keys, lowerKeys, sizeof(lowerKeys));
    }

    int keyWidth = 25;  // Reduced by 10%
    int keyHeight = 25; // Reduced by 10%
    int xOffset = 18;   // Adjusted for reduced size
    int yOffset = 81;   // Adjusted for reduced size

    for (int row = 0; row < 5; row++)
    {
        for (int col = 0; col < 10; col++)
        {
            tft.drawRect(col * (keyWidth + 3) + xOffset, row * (keyHeight + 3) + yOffset, keyWidth, keyHeight, TFT_WHITE);
            tft.setCursor(col * (keyWidth + 3) + xOffset + 5, row * (keyHeight + 3) + yOffset + 5);
            tft.print(keys[row][col]);
        }
    }
}

void handleKeyPress(int row, int col)
{
    const char* keys[5][10];
    if (isUpperCaseKeyboard)
    {
        const char* upperKeys[5][10] = {
            {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
            {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"},
            {"A", "S", "D", "F", "G", "H", "J", "K", "L", "DEL"},
            {"Z", "X", "C", "V", "B", "N", "M", "SPACE", "CLR", "OK"},
            {"!", "@", "#", "$", "%", "^", "&", "*", "(", "SHIFT"}
        };
        memcpy(keys, upperKeys, sizeof(upperKeys));
    }
    else
    {
        const char* lowerKeys[5][10] = {
            {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
            {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p"},
            {"a", "s", "d", "f", "g", "h", "j", "k", "l", "DEL"},
            {"z", "x", "c", "v", "b", "n", "m", "SPACE", "CLR", "OK"},
            {"!", "@", "#", "$", "%", "^", "&", "*", "(", "SHIFT"}
        };
        memcpy(keys, lowerKeys, sizeof(lowerKeys));
    }

    const char* keyLabel = keys[row][col];

    if (strcmp(keyLabel, "DEL") == 0)
    {
        if (inputText.length() > 0)
        {
            inputText.remove(inputText.length() - 1);
        }
    }
    else if (strcmp(keyLabel, "SPACE") == 0)
    {
        inputText += " ";
    }
    else if (strcmp(keyLabel, "CLR") == 0)
    {
        inputText = "";
    }
    else if (strcmp(keyLabel, "OK") == 0)
    {
        if (isEnteringSSID)
        {
            wifiSSID = inputText;
            inputText = "";
            isEnteringSSID = false;
            drawKeyboard(isUpperCaseKeyboard);
        }
        else
        {
            wifiPassword = inputText;
            saveWiFiSettings();
            WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
            unsigned long startAttemptTime = millis();

            // Only try to connect for 10 seconds
            while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
            {
                delay(1000);
                Serial.println("Connecting to WiFi...");
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.println("Connected to WiFi");
                delay(2000);
                ESP.restart();
            }
            else
            {
                Serial.println("Failed to connect to WiFi");
            }
        }
    }
    else if (strcmp(keyLabel, "SHIFT") == 0)
    {
        isUpperCaseKeyboard = !isUpperCaseKeyboard;
        drawKeyboard(isUpperCaseKeyboard);
    }
    else
    {
        inputText += keyLabel;
    }

    tft.fillRect(0, 30, 320, 30, TFT_BLACK);
    tft.setCursor(0, 30);
    tft.println(inputText);
}

void drawButtons()
{
    // Draw the "+" button
    tft.fillRect(270, 200, 40, 40, TFT_GREEN);
    tft.setCursor(285, 215);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.print("+");

    // Move the "-" button to the far bottom left corner
    tft.fillRect(0, 200, 40, 40, TFT_RED);
    tft.setCursor(15, 215);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.print("-");

    // Draw the thermostat mode button
    tft.fillRect(130, 200, 60, 40, TFT_BLUE);
    tft.setCursor(140, 215);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.print(thermostatMode);

    // Draw the fan mode button
    tft.fillRect(200, 200, 60, 40, TFT_ORANGE);
    tft.setCursor(210, 215);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.print(fanMode);

    // Add the word "FAN" above the fan mode button
    tft.setCursor(210, 180); // Adjusted y-coordinate to move the text up
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.print("FAN");

    // Add the word "MODE" above the mode button
    tft.setCursor(140, 180); // Adjusted y-coordinate to move the text up
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.print("MODE");
}

void handleButtonPress(uint16_t x, uint16_t y)
{
    if (x > 270 && x < 310 && y > 200 && y < 240)
    {
        if (thermostatMode == "heat")
        {
            setTempHeat += 0.5;
            if (setTempHeat > 95) setTempHeat = 95;
            if (setTempHeat < 50) setTempHeat = 50;
            if (thermostatMode == "auto" && setTempCool - setTempHeat < tempDifferential)
            {
                setTempCool = setTempHeat + tempDifferential;
                if (!handlingMQTTMessage) mqttClient.publish("thermostat/setTempCool", String(setTempCool).c_str(), true);
            }
            if (!handlingMQTTMessage) mqttClient.publish("thermostat/setTempHeat", String(setTempHeat).c_str(), true);
        }
        else if (thermostatMode == "cool")
        {
            setTempCool += 0.5;
            if (setTempCool > 95) setTempCool = 95;
            if (setTempCool < 50) setTempCool = 50;
            if (thermostatMode == "auto" && setTempCool - setTempHeat < tempDifferential)
            {
                setTempHeat = setTempCool - tempDifferential;
                if (!handlingMQTTMessage) mqttClient.publish("thermostat/setTempHeat", String(setTempHeat).c_str(), true);
            }
            if (!handlingMQTTMessage) mqttClient.publish("thermostat/setTempCool", String(setTempCool).c_str(), true);
        }
        else if (thermostatMode == "auto")
        {
            setTempAuto += 0.5;
            if (setTempAuto > 95) setTempAuto = 95;
            if (setTempAuto < 50) setTempAuto = 50;
            if (!handlingMQTTMessage) mqttClient.publish("thermostat/setTempAuto", String(setTempAuto).c_str(), true);
        }
        saveSettings();
        sendMQTTData();
        updateDisplay(currentTemp, currentHumidity);
    }
    else if (x > 0 && x < 40 && y > 200 && y < 240) // Adjusted coordinates for the "-" button
    {
        if (thermostatMode == "heat")
        {
            setTempHeat -= 0.5;
            if (setTempHeat > 95) setTempHeat = 95;
            if (setTempHeat < 50) setTempHeat = 50;
            if (thermostatMode == "auto" && setTempCool - setTempHeat < tempDifferential)
            {
                setTempCool = setTempHeat + tempDifferential;
                if (!handlingMQTTMessage) mqttClient.publish("thermostat/setTempCool", String(setTempCool).c_str(), true);
            }
            if (!handlingMQTTMessage) mqttClient.publish("thermostat/setTempHeat", String(setTempHeat).c_str(), true);
        }
        else if (thermostatMode == "cool")
        {
            setTempCool -= 0.5;
            if (setTempCool > 95) setTempCool = 95;
            if (setTempCool < 50) setTempCool = 50;
            if (thermostatMode == "auto" && setTempCool - setTempHeat < tempDifferential)
            {
                setTempHeat = setTempCool - tempDifferential;
                if (!handlingMQTTMessage) mqttClient.publish("thermostat/setTempHeat", String(setTempHeat).c_str(), true);
            }
            if (!handlingMQTTMessage) mqttClient.publish("thermostat/setTempCool", String(setTempCool).c_str(), true);
        }
        else if (thermostatMode == "auto")
        {
            setTempAuto -= 0.5;
            if (setTempAuto > 95) setTempAuto = 95;
            if (setTempAuto < 50) setTempAuto = 50;
            if (!handlingMQTTMessage) mqttClient.publish("thermostat/setTempAuto", String(setTempAuto).c_str(), true);
        }
        saveSettings();
        sendMQTTData();
        updateDisplay(currentTemp, currentHumidity);
    }
    else if (x > 130 && x < 190 && y > 200 && y < 240)
    {
        // Change thermostat mode
        if (thermostatMode == "auto")
            thermostatMode = "heat";
        else if (thermostatMode == "heat")
            thermostatMode = "cool";
        else if (thermostatMode == "cool")
            thermostatMode = "off";
        else
            thermostatMode = "auto";

        saveSettings();
        sendMQTTData();
        updateDisplay(currentTemp, currentHumidity);
    }
    else if (x > 200 && x < 260 && y > 200 && y < 240)
    {
        // Change fan mode
        if (fanMode == "auto")
            fanMode = "on";
        else if (fanMode == "on")
            fanMode = "cycle";
        else
            fanMode = "auto";

        saveSettings();
        sendMQTTData();
        updateDisplay(currentTemp, currentHumidity);
    }
    else
    {
        // Clear the display if touch is detected outside button areas
        tft.fillScreen(TFT_BLACK);
        updateDisplay(currentTemp, currentHumidity);
    }
}

void setupMQTT()
{
    mqttClient.setServer(mqttServer.c_str(), mqttPort);
    mqttClient.setBufferSize(1024); // Ensure buffer size is sufficient for large payloads
    mqttClient.setCallback(mqttCallback);
}

void reconnectMQTT()
{
    while (!mqttClient.connected())
    {
        Serial.print("Attempting MQTT connection...");
        if (mqttClient.connect(hostname.c_str(), mqttUsername.c_str(), mqttPassword.c_str())) {
            Serial.println("connected");

            // Subscribe to necessary topics
            mqttClient.subscribe("esp32_thermostat/target_temperature/set");
            mqttClient.subscribe("esp32_thermostat/mode/set");
            mqttClient.subscribe("esp32_thermostat/fan_mode/set");

            // Publish Home Assistant discovery messages
            publishHomeAssistantDiscovery();
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void publishHomeAssistantDiscovery()
{
    if (mqttEnabled)
    {
        char buffer[1024];
        StaticJsonDocument<1024> doc;

        // Publish discovery message for the thermostat device
        String configTopic = "homeassistant/climate/esp32_thermostat/config";
        doc["name"] = ""; // Use hostname instead of hardcoded name
        doc["unique_id"] = "esp32_thermostat";
        doc["current_temperature_topic"] = "esp32_thermostat/current_temperature";
        doc["current_humidity_topic"] = "esp32_thermostat/current_humidity";
        doc["temperature_command_topic"] = "esp32_thermostat/target_temperature/set";
        doc["temperature_state_topic"] = "esp32_thermostat/target_temperature";
        doc["mode_command_topic"] = "esp32_thermostat/mode/set";
        doc["mode_state_topic"] = "esp32_thermostat/mode";
        doc["fan_mode_command_topic"] = "esp32_thermostat/fan_mode/set";
        doc["fan_mode_state_topic"] = "esp32_thermostat/fan_mode";
        doc["availability_topic"] = "esp32_thermostat/availability";
        doc["min_temp"] = 50; // Minimum temperature in Fahrenheit
        doc["max_temp"] = 90; // Maximum temperature in Fahrenheit
        doc["temp_step"] = 0.5; // Temperature step

        JsonArray modes = doc.createNestedArray("modes");
        modes.add("off");
        modes.add("heat");
        modes.add("cool");
        modes.add("auto");

        JsonArray fanModes = doc.createNestedArray("fan_modes");
        fanModes.add("auto");
        fanModes.add("on");
        fanModes.add("cycle");

        JsonObject device = doc.createNestedObject("device");
        device["identifiers"] = "esp32_thermostat";
        device["name"] = hostname;
        device["manufacturer"] = "TDC";
        device["model"] = "Simple Thermostat";
        device["sw_version"] = sw_version;

        serializeJson(doc, buffer);
        mqttClient.publish(configTopic.c_str(), buffer, true);

        // Publish availability message
        mqttClient.publish("esp32_thermostat/availability", "online", true);

        // Debug log for payload
        Serial.println("Published Home Assistant discovery payload:");
        Serial.println(buffer);
    }
    else
    {
        // Remove all entities by publishing empty payloads
        mqttClient.publish("homeassistant/climate/esp32_thermostat/config", "");
        mqttClient.publish("esp32_thermostat/availability", "offline", true);
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(message);

    if (String(topic) == "esp32_thermostat/mode/set")
    {
        if (message != thermostatMode)
        {
            thermostatMode = message;
            Serial.print("Updated thermostat mode to: ");
            Serial.println(thermostatMode);
            controlRelays(currentTemp); // Apply changes to relays
        }
    }
    else if (String(topic) == "esp32_thermostat/fan_mode/set")
    {
        if (message != fanMode)
        {
            fanMode = message;
            Serial.print("Updated fan mode to: ");
            Serial.println(fanMode);
            controlRelays(currentTemp); // Apply changes to relays
        }
    }
    else if (String(topic) == "esp32_thermostat/target_temperature/set")
    {
        float newTargetTemp = message.toFloat();
        if (thermostatMode == "heat" && newTargetTemp != setTempHeat)
        {
            setTempHeat = newTargetTemp;
            Serial.print("Updated heating target temperature to: ");
            Serial.println(setTempHeat);
        }
        else if (thermostatMode == "cool" && newTargetTemp != setTempCool)
        {
            setTempCool = newTargetTemp;
            Serial.print("Updated cooling target temperature to: ");
            Serial.println(setTempCool);
        }
        else if (thermostatMode == "auto" && newTargetTemp != setTempAuto)
        {
            setTempAuto = newTargetTemp;
            Serial.print("Updated auto target temperature to: ");
            Serial.println(setTempAuto);
        }
        controlRelays(currentTemp); // Apply changes to relays
    }
}

void sendMQTTData()
{
    static float lastTemp = 0.0;
    static float lastHumidity = 0.0;
    static float lastSetTempHeat = 0.0;
    static float lastSetTempCool = 0.0;
    static float lastSetTempAuto = 0.0; // Track last auto temp
    static String lastThermostatMode = "";
    static String lastFanMode = "";

    if (mqttClient.connected())
    {
        // Publish current temperature
        if (!isnan(currentTemp) && currentTemp != lastTemp)
        {
            mqttClient.publish("esp32_thermostat/current_temperature", String(currentTemp).c_str(), true);
            lastTemp = currentTemp;
        }

        // Publish current humidity
        if (!isnan(currentHumidity) && currentHumidity != lastHumidity)
        {
            mqttClient.publish("esp32_thermostat/current_humidity", String(currentHumidity).c_str(), true);
            lastHumidity = currentHumidity;
        }

        // Publish target temperature (set temperature for heating, cooling, or auto)
        if (thermostatMode == "heat" && setTempHeat != lastSetTempHeat)
        {
            mqttClient.publish("esp32_thermostat/target_temperature", String(setTempHeat).c_str(), true);
            lastSetTempHeat = setTempHeat;
        }
        else if (thermostatMode == "cool" && setTempCool != lastSetTempCool)
        {
            mqttClient.publish("esp32_thermostat/target_temperature", String(setTempCool).c_str(), true);
            lastSetTempCool = setTempCool;
        }
        else if (thermostatMode == "auto" && setTempAuto != lastSetTempAuto)
        {
            mqttClient.publish("esp32_thermostat/target_temperature", String(setTempAuto).c_str(), true);
            lastSetTempAuto = setTempAuto;
        }

        // Publish thermostat mode
        if (thermostatMode != lastThermostatMode)
        {
            mqttClient.publish("esp32_thermostat/mode", thermostatMode.c_str(), true);
            lastThermostatMode = thermostatMode;
        }

        // Publish fan mode
        if (fanMode != lastFanMode)
        {
            mqttClient.publish("esp32_thermostat/fan_mode", fanMode.c_str(), true);
            lastFanMode = fanMode;
        }

        // Publish availability
        mqttClient.publish("esp32_thermostat/availability", "online", true);
    }
}

void controlRelays(float currentTemp)
{
    // Serial.printf("controlRelays: mode=%s, temp=%.1f, setHeat=%.1f, setCool=%.1f, setAuto=%.1f\n", thermostatMode.c_str(), currentTemp, setTempHeat, setTempCool, setTempAuto);

    if (thermostatMode == "off")
    {
        turnOffAllRelays();
        return;
    }

    if (thermostatMode == "heat")
    {
        if (currentTemp < setTempHeat - tempSwing)
        {
            activateHeating();
        }
        else
        {
            turnOffAllRelays();
        }
    }
    else if (thermostatMode == "cool")
    {
        if (currentTemp > setTempCool + tempSwing)
        {
            activateCooling();
        }
        else
        {
            turnOffAllRelays();
        }
    }
    else if (thermostatMode == "auto")
    {
        if (currentTemp < setTempAuto - autoTempSwing)
        {
            activateHeating();
        }
        else if (currentTemp > setTempAuto + autoTempSwing)
        {
            activateCooling();
        }
        else
        {
            turnOffAllRelays();
        }
    }

    handleFanControl();
    // Serial.printf("controlRelays result: heating=%d, cooling=%d, fan=%d\n", heatingOn, coolingOn, fanOn);
}

void turnOffAllRelays()
{
    digitalWrite(heatRelay1Pin, LOW);
    digitalWrite(heatRelay2Pin, LOW);
    digitalWrite(coolRelay1Pin, LOW);
    digitalWrite(coolRelay2Pin, LOW);
    digitalWrite(fanRelayPin, LOW);
    heatingOn = coolingOn = fanOn = false;
    stage1Active = false; // Reset stage 1 active flag
    stage2Active = false; // Reset stage 2 active flag
}

void activateHeating() {
    if (hydronicHeatingEnabled) {
        if (hydronicTemp <= hydronicTempLow) {
            // Pause heating if hydronic temperature is too low
            digitalWrite(heatRelay1Pin, LOW);
            digitalWrite(heatRelay2Pin, LOW);
            heatingOn = false;
            stage1Active = false;
            stage2Active = false;
            return;
        } else if (hydronicTemp >= hydronicTempHigh) {
            // Resume heating if hydronic temperature is high enough
            // Continue with normal heating logic below
        }
    }

    // Default heating behavior with hybrid staging
    heatingOn = true;
    coolingOn = false;
    
    // Check if stage 1 is not active yet
    if (!stage1Active) {
        digitalWrite(heatRelay1Pin, HIGH); // Activate stage 1
        stage1Active = true;
        stage1StartTime = millis(); // Record the start time
        stage2Active = false; // Ensure stage 2 is off initially
    } 
    // Check if it's time to activate stage 2 based on hybrid approach
    else if (!stage2Active && 
             ((millis() - stage1StartTime) / 1000 >= stage1MinRuntime) && // Minimum run time condition
             (currentTemp < setTempHeat - tempSwing - stage2TempDelta) && // Temperature delta condition
             stage2HeatingEnabled) { // Check if stage 2 heating is enabled
        digitalWrite(heatRelay2Pin, HIGH); // Activate stage 2
        stage2Active = true;
    }
    
    if (fanRelayNeeded) digitalWrite(fanRelayPin, HIGH);
}

void activateCooling()
{
    // Default cooling behavior with hybrid staging
    coolingOn = true;
    heatingOn = false;
    
    // Check if stage 1 is not active yet
    if (!stage1Active) {
        digitalWrite(coolRelay1Pin, HIGH); // Activate stage 1
        stage1Active = true;
        stage1StartTime = millis(); // Record the start time
        stage2Active = false; // Ensure stage 2 is off initially
    } 
    // Check if it's time to activate stage 2 based on hybrid approach
    else if (!stage2Active && 
            ((millis() - stage1StartTime) / 1000 >= stage1MinRuntime) && // Minimum run time condition
            (currentTemp > setTempCool + tempSwing + stage2TempDelta) && // Temperature delta condition
            stage2CoolingEnabled) { // Check if stage 2 cooling is enabled
        digitalWrite(coolRelay2Pin, HIGH); // Activate stage 2
        stage2Active = true;
    }
    
    if (fanRelayNeeded) digitalWrite(fanRelayPin, HIGH);
}

void handleFanControl()
{
    if (fanMode == "on")
    {
        digitalWrite(fanRelayPin, HIGH);
        fanOn = true;
    }
    else if (fanMode == "auto" && !heatingOn && !coolingOn)
    {
        digitalWrite(fanRelayPin, LOW);
        fanOn = false;
    }
}

void controlFanSchedule()
{
    if (firstHourAfterBoot)
    {
        unsigned long currentTime = millis();
        if (currentTime >= SECONDS_PER_HOUR * 1000) // Check if an hour has passed
        {
            firstHourAfterBoot = false; // Reset the flag after the first hour
        }
        return; // Skip fan schedule during the first hour
    }

    if (fanMode == "cycle")
    {
        unsigned long currentTime = millis();
        unsigned long elapsedTime = (currentTime - lastFanRunTime) / 1000; // Convert to seconds

        // Calculate the duration in seconds the fan should run per hour
        unsigned long fanRunSecondsPerHour = fanMinutesPerHour * 60;

        // If an hour has passed, reset the fan run duration
        if (elapsedTime >= SECONDS_PER_HOUR)
        {
            lastFanRunTime = currentTime;
            fanRunDuration = 0;
        }

        // Run the fan based on the schedule
        if (fanRunDuration < fanRunSecondsPerHour)
        {
            digitalWrite(fanRelayPin, HIGH);
            fanOn = true;
        }
        else
        {
            digitalWrite(fanRelayPin, LOW);
            fanOn = false;
        }

        // Update the fan run duration
        if (fanOn)
        {
            fanRunDuration += elapsedTime;
        }
    }
    // Retain auto mode for backward compatibility
    else if (fanMode == "auto")
    {
        // No scheduled fan running in auto mode
        // Optionally, you could keep the old schedule logic here if desired
    }
}

void handleWebRequests()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        String html = "<html><head><meta http-equiv='refresh' content='10'></head><body>"; // Changed refresh time to 10 seconds
        html += "<h1>Thermostat Status</h1>";
        html += "<p>Firmware Version: " + sw_version + "</p>";
        html += "<p>Current Temperature: " + String(currentTemp) + (useFahrenheit ? " F" : " C") + "</p>";
        html += "<p>Current Humidity: " + String(currentHumidity) + " %</p>";
        if (hydronicHeatingEnabled) {
            html += "<p>Hydronic Temperature: " + String(hydronicTemp) + " F</p>";
        }
        html += "<p>Thermostat Mode: " + thermostatMode + "</p>";
        html += "<p>Fan Mode: " + fanMode + "</p>";
        html += "<p>Heating Relay 1: " + String(digitalRead(heatRelay1Pin) ? "ON" : "OFF") + "</p>";
        html += "<p>Heating Relay 2: " + String(digitalRead(heatRelay2Pin) ? "ON" : "OFF") + "</p>";
        html += "<p>Cooling Relay 1: " + String(digitalRead(coolRelay1Pin) ? "ON" : "OFF") + "</p>";
        html += "<p>Cooling Relay 2: " + String(digitalRead(coolRelay2Pin) ? "ON" : "OFF") + "</p>";
        html += "<p>Fan Relay: " + String(digitalRead(fanRelayPin) ? "ON" : "OFF") + "</p>";
        html += "<form action='/settings' method='GET'>";
        html += "<input type='submit' value='Settings'>";
        html += "</form>";
        // Add a reboot button to the main page
        html += "<form action='/reboot' method='POST'>";
        html += "<input type='submit' value='Reboot'>";
        html += "</form>";
        html += "</body></html>";

        request->send(200, "text/html", html);
    });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        String html = "<html><body>";
        html += "<h1>Thermostat Settings</h1>";
        html += "<form action='/set' method='POST'>";
        html += "Thermostat Mode: <select name='thermostatMode'>";
        html += "<option value='off'" + String(thermostatMode == "off" ? " selected" : "") + ">Off</option>";
        html += "<option value='heat'" + String(thermostatMode == "heat" ? " selected" : "") + ">Heat</option>";
        html += "<option value='cool'" + String(thermostatMode == "cool" ? " selected" : "") + ">Cool</option>";
        html += "<option value='auto'" + String(thermostatMode == "auto" ? " selected" : "") + ">Auto</option>";
        html += "</select><br>";
        html += "Fan Mode: <select name='fanMode'>";
        html += "<option value='auto'" + String(fanMode == "auto" ? " selected" : "") + ">Auto</option>";
        html += "<option value='on'" + String(fanMode == "on" ? " selected" : "") + ">On</option>";
        html += "<option value='cycle'" + String(fanMode == "cycle" ? " selected" : "") + ">Cycle</option>";
        html += "</select><br>";
        html += "Set Temp Heat: <input type='text' name='setTempHeat' value='" + String(setTempHeat) + "'><br>";
        html += "Set Temp Cool: <input type='text' name='setTempCool' value='" + String(setTempCool) + "'><br>";
        html += "Set Temp Auto: <input type='text' name='setTempAuto' value='" + String(setTempAuto) + "'><br>";
        html += "Temp Swing: <input type='text' name='tempSwing' value='" + String(tempSwing) + "'><br>";
        html += "Auto Temp Swing: <input type='text' name='autoTempSwing' value='" + String(autoTempSwing) + "'><br>";
//         html += "Auto Changeover: <input type='checkbox' name='autoChangeover' " + String(autoChangeover ? "checked" : "") + "><br>";
        html += "Fan Relay Needed: <input type='checkbox' name='fanRelayNeeded' " + String(fanRelayNeeded ? "checked" : "") + "><br>";
        html += "Use Fahrenheit: <input type='checkbox' name='useFahrenheit' " + String(useFahrenheit ? "checked" : "") + "><br>";
        html += "MQTT Enabled: <input type='checkbox' name='mqttEnabled' " + String(mqttEnabled ? "checked" : "") + "><br>";
        html += "Hybrid Staging Settings:<br>";
        html += "Stage 1 Min Runtime (seconds): <input type='text' name='stage1MinRuntime' value='" + String(stage1MinRuntime) + "'><br>";
        html += "Stage 2 Temp Delta: <input type='text' name='stage2TempDelta' value='" + String(stage2TempDelta) + "'><br>";
        html += "Enable 2nd Stage Heating: <input type='checkbox' name='stage2HeatingEnabled' " + String(stage2HeatingEnabled ? "checked" : "") + "><br>";
        html += "Enable 2nd Stage Cooling: <input type='checkbox' name='stage2CoolingEnabled' " + String(stage2CoolingEnabled ? "checked" : "") + "><br>";
        html += "Hydronic Heating Enabled: <input type='checkbox' name='hydronicHeatingEnabled' " + String(hydronicHeatingEnabled ? "checked" : "") + "><br>";
        html += "Hydronic Temp Low: <input type='text' name='hydronicTempLow' value='" + String(hydronicTempLow) + "'><br>";
        html += "Hydronic Temp High: <input type='text' name='hydronicTempHigh' value='" + String(hydronicTempHigh) + "'><br>";
        html += "Fan Minutes Per Hour: <input type='text' name='fanMinutesPerHour' value='" + String(fanMinutesPerHour) + "'><br>";
        html += "MQTT Server: <input type='text' name='mqttServer' value='" + mqttServer + "'><br>";
        html += "MQTT Port: <input type='text' name='mqttPort' value='" + String(mqttPort) + "'><br>";
        html += "MQTT Username: <input type='text' name='mqttUsername' value='" + mqttUsername + "'><br>";
        html += "MQTT Password: <input type='text' name='mqttPassword' value='" + mqttPassword + "'><br>";
        html += "WiFi SSID: <input type='text' name='wifiSSID' value='" + wifiSSID + "'><br>";
        html += "WiFi Password: <input type='text' name='wifiPassword' value='" + wifiPassword + "'><br>";
        html += "Hostname: <input type='text' name='hostname' value='" + hostname + "'><br>";
        html += "Clock Format: <select name='clockFormat'>";
        html += "<option value='24' " + String(use24HourClock ? "selected" : "") + ">24-hour</option>";
        html += "<option value='12' " + String(!use24HourClock ? "selected" : "") + ">12-hour</option>";
        html += "</select><br>";
        html += "Time Zone: <select name='timeZone'>";
        html += "<option value='UTC' " + String(timeZone == "UTC" ? "selected" : "") + ">UTC</option>";
        html += "<option value='PST8PDT,M3.2.0,M11.1.0' " + String(timeZone == "PST8PDT,M3.2.0,M11.1.0" ? "selected" : "") + ">Pacific Time (PST)</option>";
        html += "<option value='MST7MDT,M3.2.0,M11.1.0' " + String(timeZone == "MST7MDT,M3.2.0,M11.1.0" ? "selected" : "") + ">Mountain Time (MST)</option>";
        html += "<option value='CST6CDT,M3.2.0,M11.1.0' " + String(timeZone == "CST6CDT,M3.2.0,M11.1.0" ? "selected" : "") + ">Central Time (CST)</option>";
        html += "<option value='EST5EDT,M3.2.0,M11.1.0' " + String(timeZone == "EST5EDT,M3.2.0,M11.1.0" ? "selected" : "") + ">Eastern Time (EST)</option>";
        html += "<option value='AST4ADT,M3.2.0,M11.1.0' " + String(timeZone == "AST4ADT,M3.2.0,M11.1.0" ? "selected" : "") + ">Atlantic Time (AST)</option>";
        html += "<option value='NST3:30NDT,M3.2.0,M11.1.0' " + String(timeZone == "NST3:30NDT,M3.2.0,M11.1.0" ? "selected" : "") + ">Newfoundland Time (NST)</option>";
        html += "<option value='HST10HDT,M3.2.0,M11.1.0' " + String(timeZone == "HST10HDT,M3.2.0,M11.1.0" ? "selected" : "") + ">Hawaii-Aleutian Time (HST)</option>";
        html += "<option value='AKST9AKDT,M3.2.0,M11.1.0' " + String(timeZone == "AKST9AKDT,M3.2.0,M11.1.0" ? "selected" : "") + ">Alaska Time (AKST)</option>";
        html += "<option value='AEST-10AEDT,M10.1.0,M4.1.0/3' " + String(timeZone == "AEST-10AEDT,M10.1.0,M4.1.0/3" ? "selected" : "") + ">Australian Eastern Time (AEST)</option>";
        html += "<option value='ACST-9:30ACDT,M10.1.0,M4.1.0/3' " + String(timeZone == "ACST-9:30ACDT,M10.1.0,M4.1.0/3" ? "selected" : "") + ">Australian Central Time (ACST)</option>";
        html += "<option value='AWST-8' " + String(timeZone == "AWST-8" ? "selected" : "") + ">Australian Western Time (AWST)</option>";
        html += "<option value='NZST-12NZDT,M9.5.0,M4.1.0/3' " + String(timeZone == "NZST-12NZDT,M9.5.0,M4.1.0/3" ? "selected" : "") + ">New Zealand Time (NZST)</option>";
        html += "<option value='WET0WEST,M3.5.0/1,M10.5.0' " + String(timeZone == "WET0WEST,M3.5.0/1,M10.5.0" ? "selected" : "") + ">Western European Time (WET)</option>";
        html += "<option value='CET-1CEST,M3.5.0,M10.5.0/3' " + String(timeZone == "CET-1CEST,M3.5.0,M10.5.0/3" ? "selected" : "") + ">Central European Time (CET)</option>";
        html += "<option value='EET-2EEST,M3.5.0/3,M10.5.0/4' " + String(timeZone == "EET-2EEST,M3.5.0/3,M10.5.0/4" ? "selected" : "") + ">Eastern European Time (EET)</option>";
        html += "<option value='ART-3' " + String(timeZone == "ART-3" ? "selected" : "") + ">Argentina Time (ART)</option>";
        html += "<option value='BRT-3' " + String(timeZone == "BRT-3" ? "selected" : "") + ">Brasilia Time (BRT)</option>";
        html += "<option value='CLT-4' " + String(timeZone == "CLT-4" ? "selected" : "") + ">Chile Time (CLT)</option>";
        html += "<option value='COT-5' " + String(timeZone == "COT-5" ? "selected" : "") + ">Colombia Time (COT)</option>";
        html += "<option value='ECT-5' " + String(timeZone == "ECT-5" ? "selected" : "") + ">Ecuador Time (ECT)</option>";
        html += "<option value='GYT-4' " + String(timeZone == "GYT-4" ? "selected" : "") + ">Guyana Time (GYT)</option>";
        html += "<option value='PYT-4' " + String(timeZone == "PYT-4" ? "selected" : "") + ">Paraguay Time (PYT)</option>";
        html += "<option value='PET-5' " + String(timeZone == "PET-5" ? "selected" : "") + ">Peru Time (PET)</option>";
        html += "<option value='VET-4:30' " + String(timeZone == "VET-4:30" ? "selected" : "") + ">Venezuelan Time (VET)</option>";
        html += "<option value='GMT0BST,M3.5.0/1,M10.5.0' " + String(timeZone == "GMT0BST,M3.5.0/1,M10.5.0" ? "selected" : "") + ">Greenwich Mean Time (GMT)</option>";
        html += "<option value='IST-5:30' " + String(timeZone == "IST-5:30" ? "selected" : "") + ">India Standard Time (IST)</option>";
        html += "<option value='JST-9' " + String(timeZone == "JST-9" ? "selected" : "") + ">Japan Standard Time (JST)</option>";
        html += "<option value='KST-9' " + String(timeZone == "KST-9" ? "selected" : "") + ">Korea Standard Time (KST)</option>";
        html += "<option value='CST-8' " + String(timeZone == "CST-8" ? "selected" : "") + ">China Standard Time (CST)</option>";
        html += "<option value='SGT-8' " + String(timeZone == "SGT-8" ? "selected" : "") + ">Singapore Time (SGT)</option>";
        html += "<option value='HKT-8' " + String(timeZone == "HKT-8" ? "selected" : "") + ">Hong Kong Time (HKT)</option>";
        html += "<option value='MYT-8' " + String(timeZone == "MYT-8" ? "selected" : "") + ">Malaysia Time (MYT)</option>";
        html += "<option value='PHT-8' " + String(timeZone == "PHT-8" ? "selected" : "") + ">Philippine Time (PHT)</option>";
        html += "<option value='WITA-8' " + String(timeZone == "WITA-8" ? "selected" : "") + ">Central Indonesia Time (WITA)</option>";
        html += "<option value='WIB-7' " + String(timeZone == "WIB-7" ? "selected" : "") + ">Western Indonesia Time (WIB)</option>";
        html += "<option value='WIT-9' " + String(timeZone == "WIT-9" ? "selected" : "") + ">Eastern Indonesia Time (WIT)</option>";
        html += "<option value='NZST-12NZDT,M9.5.0,M4.1.0/3' " + String(timeZone == "NZST-12NZDT,M9.5.0,M4.1.0/3" ? "selected" : "") + ">New Zealand Time (NZST)</option>";
        html += "</select><br>";
        html += "<input type='submit' value='Save Settings'>";
        html += "</form>";
        html += "<form action='/' method='GET'><input type='submit' value='Back to Status'></form>";
        html += "<form action='/update' method='GET'><input type='submit' value='OTA Update'></form>";
        html += "<form action='/confirm_restore' method='GET'><input type='submit' value='Restore Defaults'></form>";
        html += "</body></html>";

        request->send(200, "text/html", html);
    });

    server.on("/confirm_restore", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        String html = "<html><body>";
        html += "<h1>Confirm Restore Defaults</h1>";
        html += "<p>Are you sure you want to restore default settings? This will overwrite your current settings.</p>";
        html += "<form action='/restore_defaults' method='POST'>";
        html += "<input type='submit' value='Yes, Restore Defaults'>";
        html += "</form>";
        html += "<form action='/' method='GET'>";
        html += "<input type='submit' value='Cancel'>";
        html += "</form>";
        html += "</body></html>";

        request->send(200, "text/html", html); });

    server.on("/restore_defaults", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        restoreDefaultSettings();
        request->send(200, "text/plain", "Default settings restored! Please go back to the previous page."); });

    server.on("/set", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("setTempHeat", true)) {
            setTempHeat = request->getParam("setTempHeat", true)->value().toFloat();
            if (setTempHeat < 50) setTempHeat = 50;
            if (setTempHeat > 95) setTempHeat = 95;
        }
        if (request->hasParam("setTempCool", true)) {
            setTempCool = request->getParam("setTempCool", true)->value().toFloat();
            if (setTempCool < 50) setTempCool = 50;
            if (setTempCool > 95) setTempCool = 95;
        }
        if (request->hasParam("setTempAuto", true)) {
            setTempAuto = request->getParam("setTempAuto", true)->value().toFloat();
            if (setTempAuto < 50) setTempAuto = 50;
            if (setTempAuto > 95) setTempAuto = 95;
        }
        if (request->hasParam("tempSwing", true)) {
            tempSwing = request->getParam("tempSwing", true)->value().toFloat();
        }
        if (request->hasParam("autoTempSwing", true)) {
            autoTempSwing = request->getParam("autoTempSwing", true)->value().toFloat();
        }
        if (request->hasParam("autoChangeover", true)) {
            autoChangeover = request->getParam("autoChangeover", true)->value() == "on";
        }
        if (request->hasParam("fanRelayNeeded", true)) {
            fanRelayNeeded = request->getParam("fanRelayNeeded", true)->value() == "on";
        } else {
            fanRelayNeeded = false; // Ensure fanRelayNeeded is set to false if not present in the form
        }
        if (request->hasParam("useFahrenheit", true)) {
            useFahrenheit = request->getParam("useFahrenheit", true)->value() == "on";
        }
        if (request->hasParam("mqttEnabled", true)) {
            mqttEnabled = request->getParam("mqttEnabled", true)->value() == "on";
        } else {
            mqttEnabled = false; // Ensure mqttEnabled is set to false if not present in the form
        }
        if (request->hasParam("hydronicHeatingEnabled", true)) {
            hydronicHeatingEnabled = request->getParam("hydronicHeatingEnabled", true)->value() == "on"; // Handle hydronic heating option
        } else {
            hydronicHeatingEnabled = false; // Ensure hydronicHeatingEnabled is set to false if not present in the form
        }
        if (request->hasParam("hydronicTempLow", true)) {
            hydronicTempLow = request->getParam("hydronicTempLow", true)->value().toFloat(); // Handle hydronic temp low
        }
        if (request->hasParam("hydronicTempHigh", true)) {
            hydronicTempHigh = request->getParam("hydronicTempHigh", true)->value().toFloat(); // Handle hydronic temp high
        }
        if (request->hasParam("fanMinutesPerHour", true)) {
            fanMinutesPerHour = request->getParam("fanMinutesPerHour", true)->value().toInt();
        }
        if (request->hasParam("mqttServer", true)) {
            mqttServer = request->getParam("mqttServer", true)->value(); // Ensure mqttServer is updated correctly
        }
        if (request->hasParam("mqttPort", true)) {
            mqttPort = request->getParam("mqttPort", true)->value().toInt(); // Ensure mqttPort is updated correctly
        }
        if (request->hasParam("mqttUsername", true)) {
            mqttUsername = request->getParam("mqttUsername", true)->value(); // Ensure mqttUsername is updated correctly
        }
        if (request->hasParam("mqttPassword", true)) {
            mqttPassword = request->getParam("mqttPassword", true)->value(); // Ensure mqttPassword is updated correctly
        }
        if (request->hasParam("wifiSSID", true)) {
            wifiSSID = request->getParam("wifiSSID", true)->value(); // Ensure wifiSSID is updated correctly
        }
        if (request->hasParam("wifiPassword", true)) {
            String newWifiPassword = request->getParam("wifiPassword", true)->value();
            if (!newWifiPassword.isEmpty()) {
                wifiPassword = newWifiPassword; // Update wifiPassword only if a new one is provided
            }
        }
        if (request->hasParam("hostname", true)) {
            hostname = request->getParam("hostname", true)->value(); // Ensure hostname is updated correctly
        }
        if (request->hasParam("clockFormat", true)) {
            use24HourClock = request->getParam("clockFormat", true)->value() == "24";
        }
        if (request->hasParam("timeZone", true)) {
            timeZone = request->getParam("timeZone", true)->value();
            setenv("TZ", timeZone.c_str(), 1);
            tzset();
        }
        if (request->hasParam("thermostatMode", true)) {
            thermostatMode = request->getParam("thermostatMode", true)->value();
        }
        if (request->hasParam("fanMode", true)) {
            fanMode = request->getParam("fanMode", true)->value();
        }
        if (request->hasParam("stage1MinRuntime", true)) {
            stage1MinRuntime = request->getParam("stage1MinRuntime", true)->value().toInt();
        }
        if (request->hasParam("stage2TempDelta", true)) {
            stage2TempDelta = request->getParam("stage2TempDelta", true)->value().toFloat();
        }
        if (request->hasParam("stage2HeatingEnabled", true)) {
            stage2HeatingEnabled = request->getParam("stage2HeatingEnabled", true)->value() == "on";
        } else {
            stage2HeatingEnabled = false; // Ensure stage2HeatingEnabled is set to false if not present in the form
        }
        if (request->hasParam("stage2CoolingEnabled", true)) {
            stage2CoolingEnabled = request->getParam("stage2CoolingEnabled", true)->value() == "on";
        } else {
            stage2CoolingEnabled = false; // Ensure stage2CoolingEnabled is set to false if not present in the form
        }

        saveSettings();
        sendMQTTData();
        publishHomeAssistantDiscovery(); // Publish discovery messages after saving settings
        String response = "<html><body><h1>Settings saved!</h1>";
        response += "<p>Returning to the home page in 5 seconds...</p>";
        response += "<script>setTimeout(function(){ window.location.href = '/'; }, 5000);</script>";
        response += "</body></html>";
        request->send(200, "text/html", response); });

    server.on("/set_heating", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("heating", true)) {
            String heatingState = request->getParam("heating", true)->value();
            heatingOn = heatingState == "on";
            digitalWrite(heatRelay1Pin, heatingOn ? HIGH : LOW);
            digitalWrite(heatRelay2Pin, heatingOn ? HIGH : LOW);
            request->send(200, "application/json", "{\"heating\": \"" + heatingState + "\"}");
        } else {
            request->send(400, "application/json", "{\"error\": \"Invalid request\"}");
        } });

    server.on("/set_cooling", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("cooling", true)) {
            String coolingState = request->getParam("cooling", true)->value();
            coolingOn = coolingState == "on";
            digitalWrite(coolRelay1Pin, coolingOn ? HIGH : LOW);
            digitalWrite(coolRelay2Pin, coolingOn ? HIGH : LOW);
            request->send(200, "application/json", "{\"cooling\": \"" + coolingState + "\"}");
        } else {
            request->send(400, "application/json", "{\"error\": \"Invalid request\"}");
        } });

    server.on("/set_fan", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("fan", true)) {
            String fanState = request->getParam("fan", true)->value();
            fanOn = fanState == "on";
            digitalWrite(fanRelayPin, fanOn ? HIGH : LOW);
            request->send(200, "application/json", "{\"fan\": \"" + fanState + "\"}");
        } else {
            request->send(400, "application/json", "{\"error\": \"Invalid request\"}");
        } });

    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        float currentTemp = dht.readTemperature(useFahrenheit);
        String response = "{\"temperature\": \"" + String(currentTemp) + "\"}";
        request->send(200, "application/json", response); });

    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        float currentHumidity = dht.readHumidity();
        String response = "{\"humidity\": \"" + String(currentHumidity) + "\"}";
        request->send(200, "application/json", response); });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        String response = "{\"currentTemp\": \"" + String(currentTemp) + "\",";
        response += "\"currentHumidity\": \"" + String(currentHumidity) + "\",";
        response += "\"setTempHeat\": \"" + String(setTempHeat) + "\",";
        response += "\"setTempCool\": \"" + String(setTempCool) + "\",";
        response += "\"setTempAuto\": \"" + String(setTempAuto) + "\",";
        response += "\"tempSwing\": \"" + String(tempSwing) + "\",";
        response += "\"thermostatMode\": \"" + thermostatMode + "\",";
        response += "\"fanMode\": \"" + fanMode + "\"}";
        request->send(200, "application/json", response); });

    server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("setTempHeat", true)) {
            setTempHeat = request->getParam("setTempHeat", true)->value().toFloat();
            if (setTempHeat < 50) setTempHeat = 50;
            if (setTempHeat > 95) setTempHeat = 95;
        }
        if (request->hasParam("setTempCool", true)) {
            setTempCool = request->getParam("setTempCool", true)->value().toFloat();
            if (setTempCool < 50) setTempCool = 50;
            if (setTempCool > 95) setTempCool = 95;
        }
        if (request->hasParam("setTempAuto", true)) {
            setTempAuto = request->getParam("setTempAuto", true)->value().toFloat();
            if (setTempAuto < 50) setTempAuto = 50;
            if (setTempAuto > 95) setTempAuto = 95;
        }
        if (request->hasParam("tempSwing", true)) {
            tempSwing = request->getParam("tempSwing", true)->value().toFloat();
        }
        if (request->hasParam("thermostatMode", true)) {
            thermostatMode = request->getParam("thermostatMode", true)->value();
        }
        if (request->hasParam("fanMode", true)) {
            fanMode = request->getParam("fanMode", true)->value();
        }

        saveSettings();
        sendMQTTData();
        request->send(200, "application/json", "{\"status\": \"success\"}");
    });

    server.on("/reboot", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        request->send(200, "text/plain", "Rebooting...");
        delay(1000);
        ESP.restart();
    });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        String html = "<html><body>";
        html += "<h1>OTA Firmware Update</h1>";
        html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
        html += "<input type='file' name='firmware'>";
        html += "<input type='submit' value='Update Firmware'>";
        html += "</form>";
        html += "<form action='/' method='GET'><input type='submit' value='Back to Status'></form>";
        html += "</body></html>";
        request->send(200, "text/html", html);
    });

    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        if (Update.hasError()) {
            request->send(500, "text/plain", "Firmware Update Failed!");
        } else {
            request->send(200, "text/plain", "Firmware Update Successful! Rebooting...");
            delay(1000);
            ESP.restart();
        }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
    {
        static size_t updateSize = 0;
        if (!index) {
            Serial.printf("Update Start: %s\n", filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
            updateSize = 0;
        }
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
        }
        updateSize += len;
        if (final) {
            if (Update.end(true)) {
                Serial.printf("Update Success: %u bytes\n", updateSize);
            } else {
                Update.printError(Serial);
            }
        }
    });
}

void updateDisplay(float currentTemp, float currentHumidity)
{
    // Get current time
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        char timeStr[32];
        if (use24HourClock) {
            strftime(timeStr, sizeof(timeStr), "%H:%M %A", &timeinfo);
        } else {
            strftime(timeStr, sizeof(timeStr), "%I:%M %p %A", &timeinfo);
        }

        // Clear the area for time display
        tft.fillRect(0, 0, 240, 20, TFT_BLACK);

        // Display current time and day of the week
        tft.setTextSize(2);
        tft.setCursor(0, 0);
        tft.print(timeStr);
    }

    // Update temperature and humidity only if they have changed
    if (currentTemp != previousTemp || currentHumidity != previousHumidity)
    {
        // Clear only the areas that need to be updated
        tft.fillRect(240, 20, 80, 40, TFT_BLACK); // Clear temperature area
        tft.fillRect(240, 60, 80, 40, TFT_BLACK); // Clear humidity area

        // Display temperature and humidity on the right side vertically
        tft.setTextSize(2); // Adjust text size to fit the display
        tft.setRotation(1); // Set rotation for vertical display
        tft.setCursor(240, 20); // Adjust cursor position for temperature
        char tempStr[6];
        dtostrf(currentTemp, 4, 1, tempStr); // Convert temperature to string with 1 decimal place
        tft.print(tempStr);
        tft.println(useFahrenheit ? "F" : "C");

        tft.setCursor(240, 60); // Adjust cursor position for humidity
        char humidityStr[6];
        dtostrf(currentHumidity, 4, 1, humidityStr); // Convert humidity to string with 1 decimal place
        tft.print(humidityStr);
        tft.println("%");

        // Update previous values
        previousTemp = currentTemp;
        previousHumidity = currentHumidity;
    }

    // Update set temperature only if it has changed and mode is not "off"
    if (thermostatMode != "off")
    {
        float currentSetTemp = (thermostatMode == "heat") ? setTempHeat : (thermostatMode == "cool") ? setTempCool : setTempAuto;
        if (currentSetTemp != previousSetTemp)
        {
            // Clear only the area that needs to be updated
            tft.fillRect(60, 100, 200, 40, TFT_BLACK); // Clear set temperature area

            // Display set temperature in the center of the display
            tft.setTextSize(4);
            tft.setCursor(60, 100);
            char tempStr[6];
            dtostrf(currentSetTemp, 4, 1, tempStr); // Convert set temperature to string with 1 decimal place
            tft.print(tempStr);
            tft.println(useFahrenheit ? " F" : " C");

            // Update previous value
            previousSetTemp = currentSetTemp;
        }
    }
    else
    {
        // Clear the set temperature area if mode is "off"
        tft.fillRect(60, 100, 200, 40, TFT_BLACK);
    }

    // Draw buttons at the bottom
    drawButtons();
}

void saveSettings()
{
    Serial.println("Saving settings:");
    Serial.print("setTempHeat: "); Serial.println(setTempHeat);
    Serial.print("setTempCool: "); Serial.println(setTempCool);
    Serial.print("setTempAuto: "); Serial.println(setTempAuto);
    Serial.print("tempSwing: "); Serial.println(tempSwing);
    Serial.print("autoTempSwing: "); Serial.println(autoTempSwing);
    Serial.print("autoChangeover: "); Serial.println(autoChangeover);
    Serial.print("fanRelayNeeded: "); Serial.println(fanRelayNeeded);
    Serial.print("useFahrenheit: "); Serial.println(useFahrenheit);
    Serial.print("mqttEnabled: "); Serial.println(mqttEnabled);
    Serial.print("fanMinutesPerHour: "); Serial.println(fanMinutesPerHour);
    Serial.print("mqttServer: "); Serial.println(mqttServer);
    Serial.print("mqttPort: "); Serial.println(mqttPort);
    Serial.print("mqttUsername: "); Serial.println(mqttUsername);
    Serial.print("mqttPassword: "); Serial.println(mqttPassword);
    Serial.print("wifiSSID: "); Serial.println(wifiSSID);
    Serial.print("wifiPassword: "); Serial.println(wifiPassword);
    Serial.print("thermostatMode: "); Serial.println(thermostatMode);
    Serial.print("fanMode: "); Serial.println(fanMode);
    Serial.print("timeZone: "); Serial.println(timeZone);
    Serial.print("use24HourClock: "); Serial.println(use24HourClock);
    Serial.print("hydronicHeatingEnabled: "); Serial.println(hydronicHeatingEnabled);
    Serial.print("hydronicTempLow: "); Serial.println(hydronicTempLow);
    Serial.print("hydronicTempHigh: "); Serial.println(hydronicTempHigh);
    Serial.print("hostname: "); Serial.println(hostname);
    Serial.print("stage1MinRuntime: "); Serial.println(stage1MinRuntime);
    Serial.print("stage2TempDelta: "); Serial.println(stage2TempDelta);
    Serial.print("stage2HeatingEnabled: "); Serial.println(stage2HeatingEnabled);
    Serial.print("stage2CoolingEnabled: "); Serial.println(stage2CoolingEnabled);

    preferences.putFloat("setHeat", setTempHeat);
    preferences.putFloat("setCool", setTempCool);
    preferences.putFloat("setAuto", setTempAuto);
    preferences.putFloat("swing", tempSwing);
    preferences.putFloat("autoSwing", autoTempSwing);
    preferences.putBool("autoChg", autoChangeover);
    preferences.putBool("fanRelay", fanRelayNeeded);
    preferences.putBool("useF", useFahrenheit);
    preferences.putBool("mqttEn", mqttEnabled);
    preferences.putInt("fanMinHr", fanMinutesPerHour);
    preferences.putString("mqttSrv", mqttServer);
    preferences.putInt("mqttPrt", mqttPort);
    preferences.putString("mqttUsr", mqttUsername);
    preferences.putString("mqttPwd", mqttPassword);
    preferences.putString("wifiID", wifiSSID);
    preferences.putString("wifiPwd", wifiPassword);
    preferences.putString("thermoMd", thermostatMode);
    preferences.putString("fanMd", fanMode);
    preferences.putString("tz", timeZone);
    preferences.putBool("use24Clk", use24HourClock);
    preferences.putBool("hydHeat", hydronicHeatingEnabled);
    preferences.putFloat("hydLow", hydronicTempLow);
    preferences.putFloat("hydHigh", hydronicTempHigh);
    preferences.putString("host", hostname);
    preferences.putUInt("stg1MnRun", stage1MinRuntime);
    preferences.putFloat("stg2Delta", stage2TempDelta);
    preferences.putBool("stg2HeatEn", stage2HeatingEnabled);
    preferences.putBool("stg2CoolEn", stage2CoolingEnabled);
    
    saveWiFiSettings();

    // Debug print to confirm settings are saved
    Serial.print("Settings saved.");
}

void loadSettings()
{
    setTempHeat = preferences.getFloat("setHeat", 72.0);
    setTempCool = preferences.getFloat("setCool", 76.0);
    setTempAuto = preferences.getFloat("setAuto", 74.0);
    tempSwing = preferences.getFloat("swing", 1.0);
    autoTempSwing = preferences.getFloat("autoSwing", 3.0);
    autoChangeover = preferences.getBool("autoChg", true);
    fanRelayNeeded = preferences.getBool("fanRelay", false);
    useFahrenheit = preferences.getBool("useF", true);
    mqttEnabled = preferences.getBool("mqttEn", false);
    fanMinutesPerHour = preferences.getInt("fanMinHr", 15);
    mqttServer = preferences.getString("mqttSrv", "192.168.183.238");
    mqttPort = preferences.getInt("mqttPrt", 1883);
    mqttUsername = preferences.getString("mqttUsr", "your_username");
    mqttPassword = preferences.getString("mqttPwd", "your_password");
    wifiSSID = preferences.getString("wifiID", "");
    wifiPassword = preferences.getString("wifiPwd", "");
    thermostatMode = preferences.getString("thermoMd", "off");
    fanMode = preferences.getString("fanMd", "auto");
    timeZone = preferences.getString("tz", "CST6CDT,M3.2.0,M11.1.0");
    use24HourClock = preferences.getBool("use24Clk", true);
    hydronicHeatingEnabled = preferences.getBool("hydHeat", false);
    hydronicTempLow = preferences.getFloat("hydLow", 120.0);
    hydronicTempHigh = preferences.getFloat("hydHigh", 130.0);
    hostname = preferences.getString("host", "ESP32-Simple-Thermostat");
    stage1MinRuntime = preferences.getUInt("stg1MnRun", 300);
    stage2TempDelta = preferences.getFloat("stg2Delta", 2.0);
    stage2HeatingEnabled = preferences.getBool("stg2HeatEn", true);
    stage2CoolingEnabled = preferences.getBool("stg2CoolEn", true);
    
    // Debug print to confirm settings are loaded
    Serial.println("Loading settings:");
    Serial.print("setTempHeat: "); Serial.println(setTempHeat);
    Serial.print("setTempCool: "); Serial.println(setTempCool);
    Serial.print("setTempAuto: "); Serial.println(setTempAuto);
    Serial.print("tempSwing: "); Serial.println(tempSwing);
    Serial.print("autoTempSwing: "); Serial.println(autoTempSwing);
    Serial.print("autoChangeover: "); Serial.println(autoChangeover);
    Serial.print("fanRelayNeeded: "); Serial.println(fanRelayNeeded);
    Serial.print("useFahrenheit: "); Serial.println(useFahrenheit);
    Serial.print("mqttEnabled: "); Serial.println(mqttEnabled);
    Serial.print("fanMinutesPerHour: "); Serial.println(fanMinutesPerHour);
    Serial.print("mqttServer: "); Serial.println(mqttServer);
    Serial.print("mqttPort: "); Serial.println(mqttPort);
    Serial.print("mqttUsername: "); Serial.println(mqttUsername);
    Serial.print("mqttPassword: "); Serial.println(mqttPassword);
    Serial.print("wifiSSID: "); Serial.println(wifiSSID);
    Serial.print("wifiPassword: "); Serial.println(wifiPassword);
    Serial.print("thermostatMode: "); Serial.println(thermostatMode);
    Serial.print("fanMode: "); Serial.println(fanMode);
    Serial.print("timeZone: "); Serial.println(timeZone);
    Serial.print("use24HourClock: "); Serial.println(use24HourClock);
    Serial.print("hydronicHeatingEnabled: "); Serial.println(hydronicHeatingEnabled);
    Serial.print("hydronicTempLow: "); Serial.println(hydronicTempLow);
    Serial.print("hydronicTempHigh: "); Serial.println(hydronicTempHigh);
    Serial.print("hostname: "); Serial.println(hostname);
    Serial.print("stage1MinRuntime: "); Serial.println(stage1MinRuntime);
    Serial.print("stage2TempDelta: "); Serial.println(stage2TempDelta);
    Serial.print("stage2HeatingEnabled: "); Serial.println(stage2HeatingEnabled);
    Serial.print("stage2CoolingEnabled: "); Serial.println(stage2CoolingEnabled);

    // Debug print to confirm settings are loaded
    Serial.print("Settings loaded.");
}

float convertCtoF(float celsius)
{
    return celsius * 9.0 / 5.0 + 32.0;
}

void saveWiFiSettings()
{
    preferences.putString("wifiSSID", wifiSSID);
    preferences.putString("wifiPassword", wifiPassword);
}

void connectToWiFi()
{
    if (wifiSSID != "" && wifiPassword != "")
    {
        Serial.print("Connecting to WiFi with SSID: ");
        Serial.println(wifiSSID);
        Serial.print("Password: ");
        Serial.println(wifiPassword);

        WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
        unsigned long startAttemptTime = millis();

        // Only try to connect for 10 seconds
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
        {
            delay(1000);
            Serial.println("Connecting to WiFi...");
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("Connected to WiFi");
        }
        else
        {
            Serial.println("Failed to connect to WiFi");
        }
    }
    else
    {
        // Code to accept WiFi credentials from touch screen or web interface.
        Serial.println("No WiFi credentials found. Please enter them via the web interface.");
        drawKeyboard(isUpperCaseKeyboard);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(1000);
            Serial.println("Waiting for WiFi credentials...");
        }
    }
}

void calibrateTouchScreen()
{
    uint16_t calData[5];
    uint8_t calDataOK = 0;

    // Check if calibration data is stored in Preferences
    if (preferences.getBytesLength("calData") == sizeof(calData))
    {
        preferences.getBytes("calData", calData, sizeof(calData));
        tft.setTouch(calData);
        calDataOK = 1;
    }

    if (calDataOK && tft.getTouchRaw(&calData[0], &calData[1]))
    {
        Serial.println("Touch screen calibration data loaded from Preferences");
    }
    else
    {
        Serial.println("Calibrating touch screen...");
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(20, 0);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);

        tft.println("Touch corners as indicated");
        tft.setTextFont(1);
        tft.println();

        tft.calibrateTouch(calData, TFT_WHITE, TFT_RED, 15);

        // Store calibration data in Preferences
        preferences.putBytes("calData", calData, sizeof(calData));
    }
}

void handleKeyboardTouch(uint16_t x, uint16_t y, bool isUpperCaseKeyboard)
{
    for (int row = 0; row < 5; row++)
    {
        for (int col = 0; col < 10; col++)
        {
            int keyWidth = 25;  // Reduced by 10%
            int keyHeight = 25; // Reduced by 10%
            int xOffset = 18;   // Adjusted for reduced size
            int yOffset = 81;   // Adjusted for reduced size

            if (x > col * (keyWidth + 3) + xOffset && x < col * (keyWidth + 3) + xOffset + keyWidth &&
                y > row * (keyHeight + 3) + yOffset && y < row * (keyHeight + 3) + yOffset + keyHeight)
            {
                Serial.print("Key pressed at row: ");
                Serial.print(row);
                Serial.print(", col: ");
                Serial.println(col);
                handleKeyPress(row, col);
                return;
            }
        }
    }
}

void restoreDefaultSettings()
{
    setTempHeat = 72.0;
    setTempCool = 76.0;
    setTempAuto = 74.0;
    tempSwing = 1.0;
    autoTempSwing = 3.0;
    autoChangeover = true;
    fanRelayNeeded = false;
    useFahrenheit = true;
    mqttEnabled = false;
    wifiSSID = "";
    wifiPassword = "";
    fanMinutesPerHour = 15;
    mqttServer = "0.0.0.0";
    mqttUsername = "your_username";
    mqttPassword = "your_password";
    thermostatMode = "off";
    fanMode = "auto";
    timeZone = "CST6CDT,M3.2.0,M11.1.0"; // Reset time zone to default
    use24HourClock = true; // Reset clock format to default
    hydronicHeatingEnabled = false; // Reset hydronic heating setting to default
    hostname = "ESP32-Simple-Thermostat"; // Reset hostname to default

    mqttPort = 1883; // Reset MQTT port default
    hydronicTempLow = 120.0; // Reset hydronic low temp
    hydronicTempHigh = 130.0; // Reset hydronic high temp
    stage2HeatingEnabled = true; // Reset stage 2 heating enabled to default
    stage2CoolingEnabled = true; // Reset stage 2 cooling enabled to default

    saveSettings();

    // Reset the ESP32
    ESP.restart();
}