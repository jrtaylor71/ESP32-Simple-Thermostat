#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <PubSubClient.h> // Include the MQTT library
#include <esp_task_wdt.h> // Include the watchdog timer library
#include <time.h>

// Constants
const int SECONDS_PER_HOUR = 3600;
const int WDT_TIMEOUT = 10; // Watchdog timer timeout in seconds

// Globals
Adafruit_AHTX0 aht;
Adafruit_Sensor *ahtTempSensor = nullptr;
Adafruit_Sensor *ahtHumiditySensor = nullptr;
AsyncWebServer server(80);
TFT_eSPI tft = TFT_eSPI();
WiFiClient espClient;
PubSubClient mqttClient(espClient); // Initialize the MQTT client
String inputText = "";
bool isEnteringSSID = true;

// GPIO pins for relays
const int heatRelay1Pin = 13;
const int heatRelay2Pin = 12;
const int coolRelay1Pin = 14;
const int coolRelay2Pin = 26;
const int fanRelayPin = 27;

// Settings
float setTemp = 72.0; // Default set temperature in Fahrenheit
float tempSwing = 1.0;
bool autoChangeover = false;
bool fanRelayNeeded = true;
bool useFahrenheit = true; // Default to Fahrenheit
bool mqttEnabled = false; // Default to MQTT disabled
String location = "54762"; // Default ZIP code
String wifiSSID = "";
String wifiPassword = "";
int fanMinutesPerHour = 15;                                                                     // Default to 15 minutes per hour
unsigned long lastFanRunTime = 0;                                                               // Time when the fan last ran
unsigned long fanRunDuration = 0;                                                               // Duration for which the fan has run in the current hour
String homeAssistantUrl = "http://homeassistant.local:8123/api/states/sensor.esp32_thermostat"; // Replace with your Home Assistant URL
String homeAssistantApiKey = "";                                                                // Home Assistant API Key
int screenBlankTime = 120;                                                                      // Default screen blank time in seconds
unsigned long lastInteractionTime = 0;                                                          // Last interaction time

// MQTT settings
String mqttServer = "192.168.183.238"; // Replace with your MQTT server
int mqttPort = 1883;                    // Replace with your MQTT port
String mqttUsername = "your_username";  // Replace with your MQTT username
String mqttPassword = "your_password";  // Replace with your MQTT password

bool heatingOn = false;
bool coolingOn = false;
bool fanOn = false;
String thermostatMode = "auto"; // Default thermostat mode
String fanMode = "auto"; // Default fan mode

// EEPROM addresses
const int EEPROM_SIZE = 512;
const int ADDR_SET_TEMP = 0;
const int ADDR_TEMP_SWING = ADDR_SET_TEMP + sizeof(float);
const int ADDR_AUTO_CHANGEOVER = ADDR_TEMP_SWING + sizeof(float);
const int ADDR_FAN_RELAY_NEEDED = ADDR_AUTO_CHANGEOVER + sizeof(bool);
const int ADDR_USE_FAHRENHEIT = ADDR_FAN_RELAY_NEEDED + sizeof(bool);
const int ADDR_MQTT_ENABLED = ADDR_USE_FAHRENHEIT + sizeof(bool);
const int ADDR_LOCATION = ADDR_MQTT_ENABLED + sizeof(bool);
const int ADDR_FAN_MINUTES_PER_HOUR = ADDR_LOCATION + 10; // Assuming location is max 10 chars
const int ADDR_HOME_ASSISTANT_API_KEY = ADDR_FAN_MINUTES_PER_HOUR + sizeof(int);
const int ADDR_SCREEN_BLANK_TIME = ADDR_HOME_ASSISTANT_API_KEY + 32; // Assuming API key is max 32 chars
const int ADDR_WIFI_SSID = ADDR_SCREEN_BLANK_TIME + sizeof(int);
const int ADDR_WIFI_PASSWORD = ADDR_WIFI_SSID + 32; // Assuming SSID is max 32 chars

// Function prototypes
void setupWiFi();
void controlRelays(float currentTemp);
void handleWebRequests();
void updateDisplay(float currentTemp, float currentHumidity);
void saveSettings();
void loadSettings();
void sendDataToHomeAssistant(float temperature, float humidity);
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

uint16_t calibrationData[5] = { 300, 3700, 300, 3700, 7 }; // Example calibration data

float currentTemp = 0.0;
float currentHumidity = 0.0;
bool isUpperCaseKeyboard = true;

void setup()
{
    Serial.begin(115200);

    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    loadSettings();

    // Initialize the AHT20 sensor
    Wire.begin();
    if (!aht.begin())
    {
        Serial.println("Could not find AHT? Check wiring");
        ahtTempSensor = nullptr;
        ahtHumiditySensor = nullptr;
    }
    else
    {
        // Get references to the temperature and humidity sensors
        ahtTempSensor = aht.getTemperatureSensor();
        ahtHumiditySensor = aht.getHumiditySensor();
    }

    // Initialize the TFT display
    tft.init();
    tft.setRotation(1); // Set the rotation of the display as needed
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 0);
    tft.println("Thermostat");

    // Calibrate touch screen
    calibrateTouchScreen();

    // Initialize relay pins
    pinMode(heatRelay1Pin, OUTPUT);
    pinMode(heatRelay2Pin, OUTPUT);
    pinMode(coolRelay1Pin, OUTPUT);
    pinMode(coolRelay2Pin, OUTPUT);
    pinMode(fanRelayPin, OUTPUT);

    // Turn off all relays initially
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

    // Initialize time
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    // Add a delay before attempting to connect to WiFi
    delay(5000);

    // Initial display update
    updateDisplay(currentTemp, currentHumidity);
}

void loop()
{
    static unsigned long lastWiFiAttemptTime = 0;
    static unsigned long lastMQTTAttemptTime = 0;
    static unsigned long lastDisplayUpdateTime = 0;
    const unsigned long displayUpdateInterval = 1000; // Update display every second

    // Feed the watchdog timer
    esp_task_wdt_reset();

    // Attempt to connect to WiFi if not connected
    if (WiFi.status() != WL_CONNECTED && millis() - lastWiFiAttemptTime > 10000)
    {
        connectToWiFi();
        lastWiFiAttemptTime = millis();
    }

    // Comment out MQTT-related code
    /*
    if (mqttEnabled)
    {
        // Attempt to reconnect to MQTT if not connected and WiFi is connected
        if (WiFi.status() == WL_CONNECTED && !mqttClient.connected() && millis() - lastMQTTAttemptTime > 5000)
        {
            reconnectMQTT();
            lastMQTTAttemptTime = millis();
        }
        mqttClient.loop();
    }
    */

    // Read sensor data if sensor is available
    if (ahtTempSensor && ahtHumiditySensor)
    {
        sensors_event_t tempEvent, humidityEvent;
        if (ahtTempSensor->getEvent(&tempEvent) && ahtHumiditySensor->getEvent(&humidityEvent))
        {
            // Convert temperature to Fahrenheit if needed
            currentTemp = useFahrenheit ? convertCtoF(tempEvent.temperature) : tempEvent.temperature;
            currentHumidity = humidityEvent.relative_humidity;

            // Control relays based on current temperature
            controlRelays(currentTemp);

            // Send data to Home Assistant
            sendDataToHomeAssistant(currentTemp, currentHumidity);
        }
    }

    // Control fan based on schedule
    controlFanSchedule();

    // Handle screen blanking
    if (millis() - lastInteractionTime > screenBlankTime * 1000)
    {
        tft.fillScreen(TFT_BLACK);
    }

    // Handle button presses
    uint16_t x, y;
    if (tft.getTouch(&x, &y))
    {
        // Wake up the screen if it is blanked
        if (millis() - lastInteractionTime > screenBlankTime * 1000)
        {
            lastInteractionTime = millis();
            updateDisplay(currentTemp, currentHumidity);
        }
        else
        {
            Serial.print("Touch detected at: ");
            Serial.print(x);
            Serial.print(", ");
            Serial.println(y);
            handleButtonPress(x, y);
            handleKeyboardTouch(x, y, isUpperCaseKeyboard);
        }
    }

    // Update display periodically
    if (millis() - lastDisplayUpdateTime > displayUpdateInterval)
    {
        updateDisplay(currentTemp, currentHumidity);
        lastDisplayUpdateTime = millis();
    }

    // Control relays based on current temperature
    controlRelays(currentTemp);

    delay(1000); // Adjust delay as needed
}

void setupWiFi()
{
    char ssidBuffer[33];
    char passwordBuffer[65];

    for (int i = 0; i < 32; i++)
    {
        ssidBuffer[i] = EEPROM.read(ADDR_WIFI_SSID + i);
    }
    ssidBuffer[32] = '\0';
    wifiSSID = String(ssidBuffer);

    for (int i = 0; i < 64; i++)
    {
        passwordBuffer[i] = EEPROM.read(ADDR_WIFI_PASSWORD + i);
    }
    passwordBuffer[64] = '\0';
    wifiPassword = String(passwordBuffer);

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

    // Draw the "-" button
    tft.fillRect(50, 200, 40, 40, TFT_RED);
    tft.setCursor(65, 215);
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
}

void handleButtonPress(uint16_t x, uint16_t y)
{
    if (x > 270 && x < 310 && y > 200 && y < 240)
    {
        setTemp += 1.0;
        saveSettings();
        updateDisplay(currentTemp, currentHumidity);
    }
    else if (x > 50 && x < 90 && y > 200 && y < 240)
    {
        setTemp -= 1.0;
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
        else if (fanMode == "on")
            fanMode = "off";
        else
            fanMode = "auto";

        saveSettings();
        updateDisplay(currentTemp, currentHumidity);
    }
}

void setupMQTT()
{
    mqttClient.setServer(mqttServer.c_str(), mqttPort);
}

void reconnectMQTT()
{
    while (!mqttClient.connected())
    {
        Serial.print("Attempting MQTT connection...");
        if (mqttClient.connect("ESP32Thermostat", mqttUsername.c_str(), mqttPassword.c_str()))
        {
            Serial.println("connected");
            // Subscribe to topics or publish messages here
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

void controlRelays(float currentTemp)
{
    if (autoChangeover)
    {
        // Implement auto changeover logic
    }
    else
    {
        // Heating logic
        if (currentTemp < setTemp - tempSwing)
        {
            digitalWrite(heatRelay1Pin, HIGH); // First stage heat
            heatingOn = true;
            if (fanRelayNeeded)
                digitalWrite(fanRelayPin, HIGH);
            if (currentTemp < setTemp - tempSwing - 1.0)
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
        if (currentTemp > setTemp + tempSwing)
        {
            digitalWrite(coolRelay1Pin, HIGH); // First stage cool
            coolingOn = true;
            if (fanRelayNeeded)
                digitalWrite(fanRelayPin, HIGH);
            if (currentTemp > setTemp + tempSwing + 1.0)
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
    if (fanRelayNeeded && digitalRead(heatRelay1Pin) == LOW && digitalRead(coolRelay1Pin) == LOW)
    {
        digitalWrite(fanRelayPin, LOW); // Turn off fan if not needed
        fanOn = false;
    }
    else
    {
        fanOn = true;
    }
}

void controlFanSchedule()
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

void handleWebRequests()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        String html = "<html><body>";
        html += "<h1>Thermostat Settings</h1>";
        html += "<form action='/set' method='POST'>";
        html += "Set Temp: <input type='text' name='setTemp' value='" + String(setTemp) + "'><br>";
        html += "Temp Swing: <input type='text' name='tempSwing' value='" + String(tempSwing) + "'><br>";
        html += "Auto Changeover: <input type='checkbox' name='autoChangeover' " + String(autoChangeover ? "checked" : "") + "><br>";
        html += "Fan Relay Needed: <input type='checkbox' name='fanRelayNeeded' " + String(fanRelayNeeded ? "checked" : "") + "><br>";
        html += "Use Fahrenheit: <input type='checkbox' name='useFahrenheit' " + String(useFahrenheit ? "checked" : "") + "><br>";
        html += "MQTT Enabled: <input type='checkbox' name='mqttEnabled' " + String(mqttEnabled ? "checked" : "") + "><br>";
        html += "Fan Minutes Per Hour: <input type='text' name='fanMinutesPerHour' value='" + String(fanMinutesPerHour) + "'><br>";
        html += "Location (ZIP): <input type='text' name='location' value='" + location + "'><br>";
        html += "Home Assistant API Key: <input type='text' name='homeAssistantApiKey' value='" + homeAssistantApiKey + "'><br>";
        html += "Screen Blank Time (seconds): <input type='text' name='screenBlankTime' value='" + String(screenBlankTime) + "'><br>";
        html += "<input type='submit' value='Save Settings'>";
        html += "</form>";
        html += "</body></html>";

        request->send(200, "text/html", html); });

    server.on("/set", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("setTemp", true)) {
            setTemp = request->getParam("setTemp", true)->value().toFloat();
        }
        if (request->hasParam("tempSwing", true)) {
            tempSwing = request->getParam("tempSwing", true)->value().toFloat();
        }
        if (request->hasParam("autoChangeover", true)) {
            autoChangeover = request->getParam("autoChangeover", true)->value() == "on";
        }
        if (request->hasParam("fanRelayNeeded", true)) {
            fanRelayNeeded = request->getParam("fanRelayNeeded", true)->value() == "on";
        }
        if (request->hasParam("useFahrenheit", true)) {
            useFahrenheit = request->getParam("useFahrenheit", true)->value() == "on";
        }
        if (request->hasParam("mqttEnabled", true)) {
            mqttEnabled = request->getParam("mqttEnabled", true)->value() == "on";
        }
        if (request->hasParam("fanMinutesPerHour", true)) {
            fanMinutesPerHour = request->getParam("fanMinutesPerHour", true)->value().toInt();
        }
        if (request->hasParam("location", true)) {
            location = request->getParam("location", true)->value();
        }
        if (request->hasParam("homeAssistantApiKey", true)) {
            homeAssistantApiKey = request->getParam("homeAssistantApiKey", true)->value();
        }
        if (request->hasParam("screenBlankTime", true)) {
            screenBlankTime = request->getParam("screenBlankTime", true)->value().toInt();
        }

        saveSettings();
        request->send(200, "text/plain", "Settings saved! Please go back to the previous page."); });

    // Handle RESTful commands from Home Assistant
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
        sensors_event_t tempEvent;
        ahtTempSensor->getEvent(&tempEvent);
        float currentTemp = useFahrenheit ? convertCtoF(tempEvent.temperature) : tempEvent.temperature;
        String response = "{\"temperature\": \"" + String(currentTemp) + "\"}";
        request->send(200, "application/json", response); });

    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        sensors_event_t humidityEvent;
        ahtHumiditySensor->getEvent(&humidityEvent);
        String response = "{\"humidity\": \"" + String(humidityEvent.relative_humidity) + "\"}";
        request->send(200, "application/json", response); });
}

void updateDisplay(float currentTemp, float currentHumidity)
{
    // Clear only the areas that need to be updated
    tft.fillRect(0, 0, 320, 60, TFT_BLACK); // Clear the area for temperature and humidity
    tft.fillRect(0, 80, 320, 40, TFT_BLACK); // Clear the area for set temperature

    // Display temperature and humidity
    tft.setTextSize(3);
    tft.setCursor(0, 0);
    tft.print("Temp: ");
    tft.print(currentTemp);
    tft.println(useFahrenheit ? " F" : " C");

    tft.setCursor(0, 40);
    tft.print("Humidity: ");
    tft.print(currentHumidity);
    tft.println(" %");

    // Display set temperature and other settings
    tft.setTextSize(2);
    tft.setCursor(0, 80);
    tft.print("Set Temp: ");
    tft.print(setTemp);
    tft.println(useFahrenheit ? " F" : " C");

    // Draw buttons at the bottom
    drawButtons();
}

void saveSettings()
{
    EEPROM.put(ADDR_SET_TEMP, setTemp);
    EEPROM.put(ADDR_TEMP_SWING, tempSwing);
    EEPROM.put(ADDR_AUTO_CHANGEOVER, autoChangeover);
    EEPROM.put(ADDR_FAN_RELAY_NEEDED, fanRelayNeeded);
    EEPROM.put(ADDR_USE_FAHRENHEIT, useFahrenheit);
    EEPROM.put(ADDR_MQTT_ENABLED, mqttEnabled);
    EEPROM.put(ADDR_FAN_MINUTES_PER_HOUR, fanMinutesPerHour);
    EEPROM.put(ADDR_SCREEN_BLANK_TIME, screenBlankTime);

    for (int i = 0; i < 10; i++)
    {
        EEPROM.write(ADDR_LOCATION + i, i < location.length() ? location[i] : 0);
    }

    for (int i = 0; i < 32; i++)
    {
        EEPROM.write(ADDR_HOME_ASSISTANT_API_KEY + i, i < homeAssistantApiKey.length() ? homeAssistantApiKey[i] : 0);
    }

    // Save thermostat and fan modes
    EEPROM.write(ADDR_FAN_MINUTES_PER_HOUR + sizeof(int) + 32, thermostatMode == "auto" ? 0 : (thermostatMode == "heat" ? 1 : 2));
    EEPROM.write(ADDR_FAN_MINUTES_PER_HOUR + sizeof(int) + 33, fanMode == "auto" ? 0 : (fanMode == "on" ? 1 : 2));

    saveWiFiSettings();
    EEPROM.commit();
}

void loadSettings()
{
    if (EEPROM.read(ADDR_SET_TEMP) == 0xFF) {
        // EEPROM is not initialized, set default values
        setTemp = 72.0;
        tempSwing = 1.0;
        autoChangeover = false;
        fanRelayNeeded = true;
        useFahrenheit = true;
        mqttEnabled = false;
        location = "54762";
        fanMinutesPerHour = 15;
        homeAssistantApiKey = "";
        screenBlankTime = 120;
        thermostatMode = "auto";
        fanMode = "auto";

        saveSettings();
    } else {
        EEPROM.get(ADDR_SET_TEMP, setTemp);
        EEPROM.get(ADDR_TEMP_SWING, tempSwing);
        EEPROM.get(ADDR_AUTO_CHANGEOVER, autoChangeover);
        EEPROM.get(ADDR_FAN_RELAY_NEEDED, fanRelayNeeded);
        EEPROM.get(ADDR_USE_FAHRENHEIT, useFahrenheit);
        EEPROM.get(ADDR_MQTT_ENABLED, mqttEnabled);
        EEPROM.get(ADDR_FAN_MINUTES_PER_HOUR, fanMinutesPerHour);
        EEPROM.get(ADDR_SCREEN_BLANK_TIME, screenBlankTime);

        char locBuffer[11];
        for (int i = 0; i < 10; i++)
        {
            locBuffer[i] = EEPROM.read(ADDR_LOCATION + i);
        }
        locBuffer[10] = '\0';
        location = String(locBuffer);

        char apiKeyBuffer[33];
        for (int i = 0; i < 32; i++)
        {
            apiKeyBuffer[i] = EEPROM.read(ADDR_HOME_ASSISTANT_API_KEY + i);
        }
        apiKeyBuffer[32] = '\0';
        homeAssistantApiKey = String(apiKeyBuffer);

        char ssidBuffer[33];
        for (int i = 0; i < 32; i++)
        {
            ssidBuffer[i] = EEPROM.read(ADDR_WIFI_SSID + i);
        }
        ssidBuffer[32] = '\0';
        wifiSSID = String(ssidBuffer);

        char passwordBuffer[65];
        for (int i = 0; i < 64; i++)
        {
            passwordBuffer[i] = EEPROM.read(ADDR_WIFI_PASSWORD + i);
        }
        passwordBuffer[64] = '\0';
        wifiPassword = String(passwordBuffer);

        // Load thermostat and fan modes
        int mode = EEPROM.read(ADDR_FAN_MINUTES_PER_HOUR + sizeof(int) + 32);
        thermostatMode = mode == 0 ? "auto" : (mode == 1 ? "heat" : "cool");
        mode = EEPROM.read(ADDR_FAN_MINUTES_PER_HOUR + sizeof(int) + 33);
        fanMode = mode == 0 ? "auto" : (mode == 1 ? "on" : "off");
    }
}

void sendDataToHomeAssistant(float temperature, float humidity)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(homeAssistantUrl);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", "Bearer " + homeAssistantApiKey);

        String payload = "{\"state\": \"" + String(temperature) + "\", \"attributes\": {\"humidity\": \"" + String(humidity) + "\"}}";
        int httpResponseCode = http.POST(payload);

        if (httpResponseCode > 0)
        {
            String response = http.getString();
            Serial.println(httpResponseCode);
            Serial.println(response);
        }
        else
        {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    }
}

float convertCtoF(float celsius)
{
    return celsius * 9.0 / 5.0 + 32.0;
}

void saveWiFiSettings()
{
    for (int i = 0; i < 32; i++)
    {
        EEPROM.write(ADDR_WIFI_SSID + i, i < wifiSSID.length() ? wifiSSID[i] : 0);
    }

    for (int i = 0; i < 64; i++)
    {
        EEPROM.write(ADDR_WIFI_PASSWORD + i, i < wifiPassword.length() ? wifiPassword[i] : 0);
    }

    EEPROM.commit();
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

    // Check if calibration data is stored in EEPROM
    if (EEPROM.read(0) == 0x55)
    {
        for (uint8_t i = 0; i < 5; i++)
        {
            calData[i] = EEPROM.read(1 + i * 2) << 8 | EEPROM.read(2 + i * 2);
        }
        tft.setTouch(calData);
        calDataOK = 1;
    }

    if (calDataOK && tft.getTouchRaw(&calData[0], &calData[1]))
    {
        Serial.println("Touch screen calibration data loaded from EEPROM");
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

        // Store calibration data in EEPROM
        EEPROM.write(0, 0x55);
        for (uint8_t i = 0; i < 5; i++)
        {
            EEPROM.write(1 + i * 2, calData[i] >> 8);
            EEPROM.write(2 + i * 2, calData[i] & 0xFF);
        }
        EEPROM.commit();
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