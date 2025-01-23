#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <PubSubClient.h> // Include the MQTT library

// Constants
const int SECONDS_PER_HOUR = 3600;

// Globals
Preferences preferences;
Adafruit_AHTX0 aht;
Adafruit_Sensor *ahtTempSensor = nullptr;
Adafruit_Sensor *ahtHumiditySensor = nullptr;
AsyncWebServer server(80);
TFT_eSPI tft = TFT_eSPI();
WiFiClient espClient;
PubSubClient mqttClient(espClient); // Initialize the MQTT client

// GPIO pins for relays
const int heatRelay1Pin = 15;
const int heatRelay2Pin = 2;
const int coolRelay1Pin = 4;
const int coolRelay2Pin = 16;
const int fanRelayPin = 17;

// Settings
float setTemp = 72.0; // Default set temperature in Fahrenheit
float tempSwing = 1.0;
bool autoChangeover = false;
bool fanRelayNeeded = true;
bool useFahrenheit = true; // Default to Fahrenheit
String location = "90210"; // Default ZIP code
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
String mqttServer = "mqtt.example.com"; // Replace with your MQTT server
int mqttPort = 1883;                    // Replace with your MQTT port
String mqttUsername = "your_username";  // Replace with your MQTT username
String mqttPassword = "your_password";  // Replace with your MQTT password

bool heatingOn = false;
bool coolingOn = false;
bool fanOn = false;

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

void setup()
{
    Serial.begin(115200);

    // Initialize preferences for storing settings
    preferences.begin("thermostat", false);
    loadSettings();

    // Setup WiFi
    setupWiFi();

    // Initialize the AHT20 sensor
    Wire.begin();
    if (!aht.begin())
    {
        Serial.println("Could not find AHT? Check wiring");
        while (1)
            delay(10);
    }

    // Get references to the temperature and humidity sensors
    ahtTempSensor = aht.getTemperatureSensor();
    ahtHumiditySensor = aht.getHumiditySensor();

    // Initialize the TFT display
    tft.init();
    tft.setRotation(1); // Set the rotation of the display as needed
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 0);
    tft.println("Thermostat");

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

    // Start the web server
    handleWebRequests();
    server.begin();

    setupMQTT();
    lastInteractionTime = millis();
}

void loop()
{
    if (!mqttClient.connected())
    {
        reconnectMQTT();
    }
    mqttClient.loop();

    // Read sensor data
    sensors_event_t tempEvent, humidityEvent;
    ahtTempSensor->getEvent(&tempEvent);
    ahtHumiditySensor->getEvent(&humidityEvent);

    // Convert temperature to Fahrenheit if needed
    float currentTemp = useFahrenheit ? convertCtoF(tempEvent.temperature) : tempEvent.temperature;

    // Update display
    updateDisplay(currentTemp, humidityEvent.relative_humidity);

    // Control relays based on current temperature
    controlRelays(currentTemp);

    // Control fan based on schedule
    controlFanSchedule();

    // Send data to Home Assistant
    sendDataToHomeAssistant(currentTemp, humidityEvent.relative_humidity);

    // Handle screen blanking
    if (millis() - lastInteractionTime > screenBlankTime * 1000)
    {
        tft.fillScreen(TFT_BLACK);
    }

    delay(1000); // Adjust delay as needed
}

void setupWiFi()
{
    preferences.begin("wifiCreds", false);
    wifiSSID = preferences.getString("ssid", "");
    wifiPassword = preferences.getString("password", "");

    if (wifiSSID != "" && wifiPassword != "")
    {
        WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(1000);
            Serial.println("Connecting to WiFi...");
        }
        Serial.println("Connected to WiFi");
    }
    else
    {
        // Code to accept WiFi credentials from touch screen or web interface.
        Serial.println("No WiFi credentials found. Please enter them via the web interface.");
        server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            String html = "<html><body>";
            html += "<h1>Enter WiFi Credentials</h1>";
            html += "<form action='/set_wifi' method='POST'>";
            html += "SSID: <input type='text' name='ssid'><br>";
            html += "Password: <input type='password' name='password'><br>";
            html += "<input type='submit' value='Save'>";
            html += "</form>";
            html += "</body></html>";
            request->send(200, "text/html", html); });

        server.on("/set_wifi", HTTP_POST, [](AsyncWebServerRequest *request)
                  {
            if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
                wifiSSID = request->getParam("ssid", true)->value();
                wifiPassword = request->getParam("password", true)->value();
                preferences.putString("ssid", wifiSSID);
                preferences.putString("password", wifiPassword);
                request->send(200, "text/plain", "WiFi credentials saved! Please restart the device.");
            } else {
                request->send(400, "text/plain", "Invalid request");
            } });

        server.begin();
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(1000);
            Serial.println("Waiting for WiFi credentials...");
        }
    }

    // Setup MQTT
    setupMQTT();
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
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 30);
    tft.print("Temp: ");
    tft.print(currentTemp);
    tft.println(useFahrenheit ? " F" : " C");

    tft.setCursor(0, 60);
    tft.print("Humidity: ");
    tft.print(currentHumidity);
    tft.println(" %");

    tft.setCursor(0, 90);
    tft.print("Set Temp: ");
    tft.print(setTemp);
    tft.println(useFahrenheit ? " F" : " C");

    tft.setCursor(0, 120);
    tft.print("Swing: ");
    tft.print(tempSwing);
    tft.println(useFahrenheit ? " F" : " C");

    tft.setCursor(0, 150);
    tft.print("Auto Chng: ");
    tft.println(autoChangeover ? "On" : "Off");

    tft.setCursor(0, 180);
    tft.print("Fan Relay: ");
    tft.println(fanRelayNeeded ? "On" : "Off");

    tft.setCursor(0, 210);
    tft.print("Fan Minutes: ");
    tft.println(fanMinutesPerHour);

    tft.setCursor(0, 240);
    tft.print("Location: ");
    tft.println(location);
}

void saveSettings()
{
    preferences.putFloat("setTemp", setTemp);
    preferences.putFloat("tempSwing", tempSwing);
    preferences.putBool("autoChangeover", autoChangeover);
    preferences.putBool("fanRelayNeeded", fanRelayNeeded);
    preferences.putBool("useFahrenheit", useFahrenheit);
    preferences.putString("location", location);
    preferences.putInt("fanMinutesPerHour", fanMinutesPerHour);
    preferences.putString("homeAssistantApiKey", homeAssistantApiKey);
    preferences.putInt("screenBlankTime", screenBlankTime);
}

void loadSettings()
{
    setTemp = preferences.getFloat("setTemp", 72.0);
    tempSwing = preferences.getFloat("tempSwing", 0.5);
    autoChangeover = preferences.getBool("autoChangeover", false);
    fanRelayNeeded = preferences.getBool("fanRelayNeeded", true);
    useFahrenheit = preferences.getBool("useFahrenheit", true);
    location = preferences.getString("location", "90210");
    fanMinutesPerHour = preferences.getInt("fanMinutesPerHour", 15);
    homeAssistantApiKey = preferences.getString("homeAssistantApiKey", "");
    screenBlankTime = preferences.getInt("screenBlankTime", 120);
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