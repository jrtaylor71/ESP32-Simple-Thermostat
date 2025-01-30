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
float tempSwing = 1.0;
bool autoChangeover = false;
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
String mqttServer = "192.168.183.238"; // Replace with your MQTT server
int mqttPort = 1883;                    // Replace with your MQTT port
String mqttUsername = "your_username";  // Replace with your MQTT username
String mqttPassword = "your_password";  // Replace with your MQTT password
String timeZone = "CST6CDT,M3.2.0,M11.1.0"; // Default time zone (Central Standard Time)

bool heatingOn = false;
bool coolingOn = false;
bool fanOn = false;
String thermostatMode = "off"; // Default thermostat mode
String fanMode = "auto"; // Default fan mode

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
    wifiSSID = preferences.getString("wifiSSID", "");
    wifiPassword = preferences.getString("wifiPassword", "");

    WiFi.setHostname("ESP32-Simple-Thermostat"); // Set the WiFi device name

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
        if (thermostatMode == "heat" || thermostatMode == "auto")
        {
            setTempHeat += 1.0;
        }
        else if (thermostatMode == "cool")
        {
            setTempCool += 1.0;
        }
        saveSettings();
        updateDisplay(currentTemp, currentHumidity);
    }
    else if (x > 0 && x < 40 && y > 200 && y < 240) // Adjusted coordinates for the "-" button
    {
        if (thermostatMode == "heat" || thermostatMode == "auto")
        {
            setTempHeat -= 1.0;
        }
        else if (thermostatMode == "cool")
        {
            setTempCool -= 1.0;
        }
        saveSettings();
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
        updateDisplay(currentTemp, currentHumidity);
    }
    else if (x > 200 && x < 260 && y > 200 && y < 240)
    {
        // Change fan mode
        if (fanMode == "auto")
            fanMode = "on";
        else
            fanMode = "auto";

        saveSettings();
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
    mqttClient.setBufferSize(512); // Increase buffer size for larger messages
    mqttClient.setCallback(mqttCallback);
}

void reconnectMQTT()
{
    while (!mqttClient.connected())
    {
        Serial.print("Attempting MQTT connection...");
        if (mqttClient.connect("ESP32Thermostat", mqttUsername.c_str(), mqttPassword.c_str()))
        {
            Serial.println("connected");
            // Subscribe to topics
            mqttClient.subscribe("thermostat/setTempHeat");
            mqttClient.subscribe("thermostat/setTempCool");
            mqttClient.subscribe("thermostat/tempSwing");
            mqttClient.subscribe("thermostat/thermostatMode");
            mqttClient.subscribe("thermostat/fanMode");
            mqttClient.subscribe("thermostat/availability"); // Subscribe to availability topic

            // Publish Home Assistant MQTT discovery messages
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
        char buffer[512];
        JsonDocument doc;
        doc.clear();

        // Publish discovery message for thermostat
        String thermostatConfig = "homeassistant/climate/esp32_thermostat/config";
        doc["name"] = "ESP32 Thermostat";
        doc["unique_id"] = "esp32_thermostat";
        doc["current_temperature_topic"] = "thermostat/currentTemp";
        doc["temperature_command_topic"] = "thermostat/setTempHeat";
        doc["temperature_state_topic"] = "thermostat/setTempHeat";
        doc["mode_command_topic"] = "thermostat/thermostatMode";
        doc["mode_state_topic"] = "thermostat/thermostatMode";
        doc["availability_topic"] = "thermostat/availability";
        JsonArray modes = doc["modes"].to<JsonArray>();
        modes.add("off");
        modes.add("heat");
        modes.add("cool");
        modes.add("auto");
        doc["fan_mode_command_topic"] = "thermostat/fanMode";
        doc["fan_mode_state_topic"] = "thermostat/fanMode";
        JsonArray fan_modes = doc["fan_modes"].to<JsonArray>();
        fan_modes.add("auto");
        fan_modes.add("on");
        serializeJson(doc, buffer);
        mqttClient.publish(thermostatConfig.c_str(), buffer, true);

        // Publish discovery message for humidity sensor
        String humidityConfig = "homeassistant/sensor/esp32_humidity/config";
        doc.clear();
        doc["name"] = "ESP32 Humidity";
        doc["unique_id"] = "esp32_humidity";
        doc["state_topic"] = "thermostat/currentHumidity";
        doc["unit_of_measurement"] = "%";
        doc["availability_topic"] = "thermostat/availability";
        serializeJson(doc, buffer);
        mqttClient.publish(humidityConfig.c_str(), buffer, true);

        // Publish availability message
        mqttClient.publish("thermostat/availability", "online", true);
    }
    else
    {
        // Remove all entities by publishing empty payloads
        mqttClient.publish("homeassistant/climate/esp32_thermostat/config", "");
        mqttClient.publish("homeassistant/sensor/esp32_humidity/config", "");
        mqttClient.publish("thermostat/availability", "offline", true);
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

    if (String(topic) == "thermostat/setTempHeat")
    {
        setTempHeat = message.toFloat();
    }
    else if (String(topic) == "thermostat/setTempCool")
    {
        setTempCool = message.toFloat();
    }
    else if (String(topic) == "thermostat/tempSwing")
    {
        tempSwing = message.toFloat();
    }
    else if (String(topic) == "thermostat/thermostatMode")
    {
        thermostatMode = message;
    }
    else if (String(topic) == "thermostat/fanMode")
    {
        fanMode = message;
    }

    saveSettings();
}

void sendMQTTData()
{
    if (mqttClient.connected())
    {
        mqttClient.publish("thermostat/currentTemp", String(currentTemp).c_str());
        mqttClient.publish("thermostat/currentHumidity", String(currentHumidity).c_str());
        mqttClient.publish("thermostat/setTempHeat", String(setTempHeat).c_str());
        mqttClient.publish("thermostat/setTempCool", String(setTempCool).c_str());
        mqttClient.publish("thermostat/tempSwing", String(tempSwing).c_str());
        mqttClient.publish("thermostat/thermostatMode", thermostatMode.c_str());
        mqttClient.publish("thermostat/fanMode", fanMode.c_str());
    }
}

void controlRelays(float currentTemp)
{
    if (thermostatMode == "off")
    {
        // Turn off all relays
        digitalWrite(heatRelay1Pin, LOW);
        digitalWrite(heatRelay2Pin, LOW);
        digitalWrite(coolRelay1Pin, LOW);
        digitalWrite(coolRelay2Pin, LOW);
        digitalWrite(fanRelayPin, LOW);
        heatingOn = false;
        coolingOn = false;
        fanOn = false;
    }
    else if (autoChangeover)
    {
        // Auto changeover logic
        if (currentTemp < setTempHeat - tempSwing)
        {
            digitalWrite(heatRelay1Pin, HIGH); // First stage heat
            heatingOn = true;
            coolingOn = false;
            if (fanRelayNeeded)
                digitalWrite(fanRelayPin, HIGH);
            if (currentTemp < setTempHeat - tempSwing - 1.0)
            {
                digitalWrite(heatRelay2Pin, HIGH); // Second stage heat
            }
            else
            {
                digitalWrite(heatRelay2Pin, LOW);
            }
        }
        else if (currentTemp > setTempCool + tempSwing)
        {
            digitalWrite(coolRelay1Pin, HIGH); // First stage cool
            coolingOn = true;
            heatingOn = false;
            if (fanRelayNeeded)
                digitalWrite(fanRelayPin, HIGH);
            if (currentTemp > setTempCool + tempSwing + 1.0)
            {
                digitalWrite(coolRelay2Pin, HIGH); // Second stage cool
            }
            else
            {
                digitalWrite(coolRelay2Pin, LOW);
            }
        }
        else
        {
            digitalWrite(heatRelay1Pin, LOW);
            digitalWrite(heatRelay2Pin, LOW);
            digitalWrite(coolRelay1Pin, LOW);
            digitalWrite(coolRelay2Pin, LOW);
            heatingOn = false;
            coolingOn = false;
        }
    }
    else
    {
        // Heating logic
        if (currentTemp < setTempHeat - tempSwing)
        {
            digitalWrite(heatRelay1Pin, HIGH); // First stage heat
            heatingOn = true;
            if (fanRelayNeeded)
                digitalWrite(fanRelayPin, HIGH);
            if (currentTemp < setTempHeat - tempSwing - 1.0)
            {
                digitalWrite(heatRelay2Pin, HIGH); // Second stage heat
            }
            else
            {
                digitalWrite(heatRelay2Pin, LOW);
            }
        }
        else
        {
            digitalWrite(heatRelay1Pin, LOW);
            digitalWrite(heatRelay2Pin, LOW);
            heatingOn = false;
        }

        // Cooling logic
        if (currentTemp > setTempCool + tempSwing)
        {
            digitalWrite(coolRelay1Pin, HIGH); // First stage cool
            coolingOn = true;
            if (fanRelayNeeded)
                digitalWrite(fanRelayPin, HIGH);
            if (currentTemp > setTempCool + tempSwing + 1.0)
            {
                digitalWrite(coolRelay2Pin, HIGH); // Second stage cool
            }
            else
            {
                digitalWrite(coolRelay2Pin, LOW);
            }
        }
        else
        {
            digitalWrite(coolRelay1Pin, LOW);
            digitalWrite(coolRelay2Pin, LOW);
            coolingOn = false;
        }
    }

    // Fan logic
    if (fanMode == "on")
    {
        digitalWrite(fanRelayPin, HIGH); // Turn on fan
        fanOn = true;
    }
    else if (fanMode == "auto")
    {
        if (!heatingOn && !coolingOn)
        {
            digitalWrite(fanRelayPin, LOW); // Turn off fan if not needed
            fanOn = false;
        }
    }    

    // Hydronic heating logic
    if (hydronicHeatingEnabled && heatingOn)
    {
        if (hydronicTemp < hydronicTempLow)
        {
            digitalWrite(fanRelayPin, LOW); // Turn off fan if hydronic temp is below low threshold
        }
        else if (hydronicTemp >= hydronicTempHigh)
        {
            digitalWrite(fanRelayPin, HIGH); // Turn on fan if hydronic temp is high threshold or above
        }
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

    if (fanMode == "auto")
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
}

void handleWebRequests()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        String html = "<html><body>";
        html += "<h1>Thermostat Settings</h1>";
        html += "<form action='/set' method='POST'>";
        html += "Set Temp Heat: <input type='text' name='setTempHeat' value='" + String(setTempHeat) + "'><br>";
        html += "Set Temp Cool: <input type='text' name='setTempCool' value='" + String(setTempCool) + "'><br>";
        html += "Temp Swing: <input type='text' name='tempSwing' value='" + String(tempSwing) + "'><br>";
        html += "Auto Changeover: <input type='checkbox' name='autoChangeover' " + String(autoChangeover ? "checked" : "") + "><br>";
        html += "Fan Relay Needed: <input type='checkbox' name='fanRelayNeeded' " + String(fanRelayNeeded ? "checked" : "") + "><br>";
        html += "Use Fahrenheit: <input type='checkbox' name='useFahrenheit' " + String(useFahrenheit ? "checked" : "") + "><br>";
        html += "MQTT Enabled: <input type='checkbox' name='mqttEnabled' " + String(mqttEnabled ? "checked" : "") + "><br>";
        html += "Hydronic Heating Enabled: <input type='checkbox' name='hydronicHeatingEnabled' " + String(hydronicHeatingEnabled ? "checked" : "") + "><br>"; // Add hydronic heating option
        html += "Hydronic Temp Low: <input type='text' name='hydronicTempLow' value='" + String(hydronicTempLow) + "'><br>"; // Add hydronic temp low input
        html += "Hydronic Temp High: <input type='text' name='hydronicTempHigh' value='" + String(hydronicTempHigh) + "'><br>"; // Add hydronic temp high input
        html += "Fan Minutes Per Hour: <input type='text' name='fanMinutesPerHour' value='" + String(fanMinutesPerHour) + "'><br>";
        html += "MQTT Server: <input type='text' name='mqttServer' value='" + mqttServer + "'><br>";
        html += "MQTT Port: <input type='text' name='mqttPort' value='" + String(mqttPort) + "'><br>"; // Add MQTT port input
        html += "MQTT Username: <input type='text' name='mqttUsername' value='" + mqttUsername + "'><br>";
        html += "MQTT Password: <input type='text' name='mqttPassword' value='" + mqttPassword + "'><br>";
        html += "WiFi SSID: <input type='text' name='wifiSSID' value='" + wifiSSID + "'><br>";
        html += "WiFi Password: <input type='text' name='wifiPassword' value='" + wifiPassword + "'><br>";
        html += "Clock Format: <select name='clockFormat'>"; // Add clock format drop-down list
        html += "<option value='24' " + String(use24HourClock ? "selected" : "") + ">24-hour</option>";
        html += "<option value='12' " + String(!use24HourClock ? "selected" : "") + ">12-hour</option>";
        html += "</select><br>";
        html += "Time Zone: <select name='timeZone'>"; // Add time zone drop-down list
        html += "<option value='UTC' " + String(timeZone == "UTC" ? "selected" : "") + ">UTC</option>";
        html += "<option value='PST8PDT,M3.2.0,M11.1.0' " + String(timeZone == "PST8PDT,M3.2.0,M11.1.0" ? "selected" : "") + ">Pacific Time (PST)</option>";
        html += "<option value='MST7MDT,M3.2.0,M11.1.0' " + String(timeZone == "MST7MDT,M3.2.0,M11.1.0" ? "selected" : "") + ">Mountain Time (MST)</option>";
        html += "<option value='CST6CDT,M3.2.0,M11.1.0' " + String(timeZone == "CST6CDT,M3.2.0,M11.1.0" ? "selected" : "") + ">Central Time (CST)</option>";
        html += "<option value='EST5EDT,M3.2.0,M11.1.0' " + String(timeZone == "EST5EDT,M3.2.0,M11.1.0" ? "selected" : "") + ">Eastern Time (EST)</option>";
        html += "</select><br>";
        html += "<input type='submit' value='Save Settings'>";
        html += "</form>";
        html += "<form action='/confirm_restore' method='GET'>";
        html += "<input type='submit' value='Restore Defaults'>";
        html += "</form>";
        html += "</body></html>";

        request->send(200, "text/html", html); });

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
        }
        if (request->hasParam("setTempCool", true)) {
            setTempCool = request->getParam("setTempCool", true)->value().toFloat();
        }
        if (request->hasParam("tempSwing", true)) {
            tempSwing = request->getParam("tempSwing", true)->value().toFloat();
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
        if (request->hasParam("clockFormat", true)) {
            use24HourClock = request->getParam("clockFormat", true)->value() == "24";
        }
        if (request->hasParam("timeZone", true)) {
            timeZone = request->getParam("timeZone", true)->value();
            setenv("TZ", timeZone.c_str(), 1);
            tzset();
        }

        saveSettings();
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
        response += "\"tempSwing\": \"" + String(tempSwing) + "\",";
        response += "\"thermostatMode\": \"" + thermostatMode + "\",";
        response += "\"fanMode\": \"" + fanMode + "\"}";
        request->send(200, "application/json", response); });

    server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("setTempHeat", true)) {
            setTempHeat = request->getParam("setTempHeat", true)->value().toFloat();
        }
        if (request->hasParam("setTempCool", true)) {
            setTempCool = request->getParam("setTempCool", true)->value().toFloat();
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
        request->send(200, "application/json", "{\"status\": \"success\"}");
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
        float currentSetTemp = (thermostatMode == "heat" || thermostatMode == "auto") ? setTempHeat : setTempCool;
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
    preferences.putFloat("setTempHeat", setTempHeat);
    preferences.putFloat("setTempCool", setTempCool);
    preferences.putFloat("tempSwing", tempSwing);
    preferences.putBool("autoChangeover", autoChangeover);
    preferences.putBool("fanRelayNeeded", fanRelayNeeded);
    preferences.putBool("useFahrenheit", useFahrenheit);
    preferences.putBool("mqttEnabled", mqttEnabled);
    preferences.putInt("fanMinPerHr", fanMinutesPerHour); // Shortened key
    preferences.putString("mqttServer", mqttServer);
    preferences.putInt("mqttPort", mqttPort); // Save MQTT port
    preferences.putString("mqttUser", mqttUsername); // Shortened key
    preferences.putString("mqttPass", mqttPassword); // Shortened key
    preferences.putString("wifiSSID", wifiSSID);
    preferences.putString("wifiPass", wifiPassword); // Shortened key
    preferences.putString("thermoMode", thermostatMode); // Shortened key
    preferences.putString("fanMode", fanMode);
    preferences.putString("timeZone", timeZone); // Save time zone
    preferences.putBool("use24HourClock", use24HourClock); // Save clock format
    preferences.putBool("hydroHeatEn", hydronicHeatingEnabled); // Shortened key
    preferences.putFloat("hydroTempLow", hydronicTempLow); // Save hydronic low temperature
    preferences.putFloat("hydroTempHigh", hydronicTempHigh); // Save hydronic high temperature

    saveWiFiSettings();

    // Debug print to confirm settings are saved
    Serial.println("Settings saved.");
}

void loadSettings()
{
    setTempHeat = preferences.getFloat("setTempHeat", 72.0);
    setTempCool = preferences.getFloat("setTempCool", 76.0);
    tempSwing = preferences.getFloat("tempSwing", 1.0);
    autoChangeover = preferences.getBool("autoChangeover", false);
    fanRelayNeeded = preferences.getBool("fanRelayNeeded", false);
    useFahrenheit = preferences.getBool("useFahrenheit", true);
    mqttEnabled = preferences.getBool("mqttEnabled", false);
    fanMinutesPerHour = preferences.getInt("fanMinPerHr", 15); // Shortened key
    mqttServer = preferences.getString("mqttServer", "192.168.183.238");
    mqttPort = preferences.getInt("mqttPort", 1883); // Load MQTT port
    mqttUsername = preferences.getString("mqttUser", "your_username"); // Shortened key
    mqttPassword = preferences.getString("mqttPass", "your_password"); // Shortened key
    wifiSSID = preferences.getString("wifiSSID", "");
    wifiPassword = preferences.getString("wifiPass", ""); // Shortened key
    thermostatMode = preferences.getString("thermoMode", "off"); // Shortened key
    fanMode = preferences.getString("fanMode", "auto");
    timeZone = preferences.getString("timeZone", "CST6CDT,M3.2.0,M11.1.0"); // Load time zone
    use24HourClock = preferences.getBool("use24HourClock", true); // Load clock format
    hydronicHeatingEnabled = preferences.getBool("hydroHeatEn", false); // Shortened key
    hydronicTempLow = preferences.getFloat("hydroTempLow", 120.0); // Load hydronic low temperature
    hydronicTempHigh = preferences.getFloat("hydroTempHigh", 130.0); // Load hydronic high temperature

    // Debug print to confirm settings are loaded
    Serial.println("Settings loaded.");
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
    tempSwing = 1.0;
    autoChangeover = false;
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

    saveSettings();

    // Reset the ESP32
    ESP.restart();
}