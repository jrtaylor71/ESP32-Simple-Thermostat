#ifndef WEBPAGES_H
#define WEBPAGES_H

#include "WebInterface.h"

// Schedule system structures
struct SchedulePeriod {
    int hour;        // 0-23
    int minute;      // 0-59
    float heatTemp;  // Target heating temperature
    float coolTemp;  // Target cooling temperature
    float autoTemp;  // Target auto mode temperature
    bool active;     // Whether this period is enabled
};

struct DaySchedule {
    SchedulePeriod day;    // Day period (default 6:00 AM)
    SchedulePeriod night;  // Night period (default 10:00 PM)
    bool enabled;          // Whether scheduling is enabled for this day
};

// Generate modern status page HTML
String generateStatusPage(float currentTemp, float currentHumidity, float hydronicTemp, 
                         String thermostatMode, String fanMode, String version_info, 
                         String hostname, bool useFahrenheit, bool hydronicHeatingEnabled,
                         int heatRelay1Pin, int heatRelay2Pin, int coolRelay1Pin, 
                         int coolRelay2Pin, int fanRelayPin,
                         // Settings variables for embedded settings tab
                         float setTempHeat, float setTempCool, float setTempAuto,
                         float tempSwing, float autoTempSwing, bool autoChangeover,
                         bool fanRelayNeeded, unsigned long stage1MinRuntime, 
                         float stage2TempDelta, int fanMinutesPerHour,
                         bool stage2HeatingEnabled, bool stage2CoolingEnabled,
                         float hydronicTempLow, float hydronicTempHigh,
                         String wifiSSID, String wifiPassword, String timeZone,
                         bool use24HourClock, bool mqttEnabled, String mqttServer,
                         int mqttPort, String mqttUsername, String mqttPassword,
                         float tempOffset, float humidityOffset, int currentBrightness,
                         bool displaySleepEnabled, unsigned long displaySleepTimeout,
                         // Schedule variables for embedded schedule tab
                         DaySchedule weekSchedule[7], bool scheduleEnabled, String activePeriod,
                         bool scheduleOverride) {
    
    String html = "<!DOCTYPE html><html lang='en'><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>ESP32 Simple Thermostat - Status</title>";
    html += CSS_STYLES;
    html += "</head><body>";
    
    html += "<div class='container'>";
    
    // Header
    html += "<div class='header'>";
    html += "<h1>ESP32 Simple Thermostat</h1>";
    html += "<div class='version'>Version " + version_info + " • " + hostname + "</div>";
    html += "</div>";
    
    // Navigation tabs
    html += "<div class='nav-tabs'>";
    html += "<button class='nav-tab active' onclick='showTab(\"status\")'>Status</button>";
    html += "<button class='nav-tab' onclick='showTab(\"settings\")'>Settings</button>";
    html += "<button class='nav-tab' onclick='showTab(\"schedule\")'>Schedule</button>";
    html += "<button class='nav-tab' onclick='showTab(\"system\")'>System</button>";
    html += "</div>";
    
    // Status tab content
    html += "<div id='status-content' class='tab-content content active'>";
    
    // Main temperature display
    html += "<div class='status-card' style='text-align: center; margin-bottom: 24px;'>";
    html += "<div class='card-header'>";
    html += ICON_TEMPERATURE;
    html += "<h2 class='card-title'>Current Temperature</h2>";
    html += "</div>";
    html += "<div class='temp-display'>" + String(currentTemp, 1) + "<span class='temp-unit'>&deg;" + String(useFahrenheit ? "F" : "C") + "</span></div>";
    html += "</div>";
    
    // Status grid
    html += "<div class='status-grid'>";
    
    // Humidity card
    html += "<div class='status-card'>";
    html += "<div class='card-header'>";
    html += ICON_HUMIDITY;
    html += "<h3 class='card-title'>Humidity</h3>";
    html += "</div>";
    html += "<div style='text-align: center; font-size: 2rem; color: var(--secondary-color);'>";
    html += String(currentHumidity, 1) + "<span style='font-size: 1rem; opacity: 0.7;'>%</span></div>";
    html += "</div>";
    
    // Thermostat mode card
    html += "<div class='status-card'>";
    html += "<div class='card-header'>";
    html += ICON_THERMOSTAT;
    html += "<h3 class='card-title'>Thermostat Mode</h3>";
    html += "</div>";
    html += "<div style='text-align: center; margin: 16px 0;'>";
    String modeClass = "status-";
    if (thermostatMode == "off") modeClass += "off";
    else if (thermostatMode == "auto") modeClass += "auto";
    else modeClass += "on";
    html += "<span class='status-indicator " + modeClass + "'>" + thermostatMode + "</span>";
    html += "</div>";
    html += "<div style='text-align: center; font-size: 0.9rem; opacity: 0.7;'>Fan: " + fanMode + "</div>";
    html += "</div>";
    
    // Hydronic temperature (if enabled)
    if (hydronicHeatingEnabled) {
        html += "<div class='status-card'>";
        html += "<div class='card-header'>";
        html += ICON_TEMPERATURE;
        html += "<h3 class='card-title'>Hydronic Temperature</h3>";
        html += "</div>";
        html += "<div style='text-align: center; font-size: 2rem; color: var(--warning);'>";
        html += String(hydronicTemp, 1) + "<span style='font-size: 1rem; opacity: 0.7;'>&deg;F</span></div>";
        html += "</div>";
    }
    
    html += "</div>"; // End status-grid
    
    // System status section
    html += "<div class='status-card'>";
    html += "<div class='card-header'>";
    html += ICON_RELAY;
    html += "<h3 class='card-title'>System Status</h3>";
    html += "</div>";
    html += "<div class='system-status'>";
    
    // Relay statuses
    bool heat1Status = digitalRead(heatRelay1Pin);
    bool heat2Status = digitalRead(heatRelay2Pin);
    bool cool1Status = digitalRead(coolRelay1Pin);
    bool cool2Status = digitalRead(coolRelay2Pin);
    bool fanStatus = digitalRead(fanRelayPin);
    
    html += "<div class='relay-status" + String(heat1Status ? " active" : "") + "'>";
    html += "<span>Heat Stage 1</span><span class='status-indicator " + String(heat1Status ? "status-on" : "status-off") + "'>" + String(heat1Status ? "ON" : "OFF") + "</span>";
    html += "</div>";
    
    html += "<div class='relay-status" + String(heat2Status ? " active" : "") + "'>";
    html += "<span>Heat Stage 2</span><span class='status-indicator " + String(heat2Status ? "status-on" : "status-off") + "'>" + String(heat2Status ? "ON" : "OFF") + "</span>";
    html += "</div>";
    
    html += "<div class='relay-status" + String(cool1Status ? " active" : "") + "'>";
    html += "<span>Cool Stage 1</span><span class='status-indicator " + String(cool1Status ? "status-on" : "status-off") + "'>" + String(cool1Status ? "ON" : "OFF") + "</span>";
    html += "</div>";
    
    html += "<div class='relay-status" + String(cool2Status ? " active" : "") + "'>";
    html += "<span>Cool Stage 2</span><span class='status-indicator " + String(cool2Status ? "status-on" : "status-off") + "'>" + String(cool2Status ? "ON" : "OFF") + "</span>";
    html += "</div>";
    
    html += "<div class='relay-status" + String(fanStatus ? " active" : "") + "'>";
    html += "<span>Fan</span><span class='status-indicator " + String(fanStatus ? "status-on" : "status-off") + "'>" + String(fanStatus ? "ON" : "OFF") + "</span>";
    html += "</div>";
    
    html += "</div>"; // End system-status
    html += "</div>"; // End status-card
    
    html += "</div>"; // End status-content tab
    
    // Settings tab content (embedded settings form)
    html += "<div id='settings-content' class='tab-content content'>";
    html += "<form action='/set' method='POST' onsubmit='return handleSettingsSubmit(event);'>";
    
    // Basic Settings Section
    html += "<div class='settings-section'>";
    html += "<h3>Basic Settings</h3>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Thermostat Mode</label>";
    html += "<select name='thermostatMode' class='form-select'>";
    html += "<option value='off'" + String(thermostatMode == "off" ? " selected" : "") + ">Off</option>";
    html += "<option value='heat'" + String(thermostatMode == "heat" ? " selected" : "") + ">Heat</option>";
    html += "<option value='cool'" + String(thermostatMode == "cool" ? " selected" : "") + ">Cool</option>";
    html += "<option value='auto'" + String(thermostatMode == "auto" ? " selected" : "") + ">Auto</option>";
    html += "</select>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Fan Mode</label>";
    html += "<select name='fanMode' class='form-select'>";
    html += "<option value='auto'" + String(fanMode == "auto" ? " selected" : "") + ">Auto</option>";
    html += "<option value='on'" + String(fanMode == "on" ? " selected" : "") + ">On</option>";
    html += "<option value='cycle'" + String(fanMode == "cycle" ? " selected" : "") + ">Cycle</option>";
    html += "</select>";
    html += "</div>";
    
    html += "<div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 16px;'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Heat Setpoint</label>";
    html += "<input type='number' name='setTempHeat' value='" + String(setTempHeat, 1) + "' step='0.5' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Cool Setpoint</label>";
    html += "<input type='number' name='setTempCool' value='" + String(setTempCool, 1) + "' step='0.5' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Auto Setpoint</label>";
    html += "<input type='number' name='setTempAuto' value='" + String(setTempAuto, 1) + "' step='0.5' class='form-input'>";
    html += "</div>";
    
    html += "</div>"; // End grid
    
    html += "<div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 16px;'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Temperature Swing</label>";
    html += "<input type='number' name='tempSwing' value='" + String(tempSwing, 1) + "' step='0.1' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Auto Temp Swing</label>";
    html += "<input type='number' name='autoTempSwing' value='" + String(autoTempSwing, 1) + "' step='0.1' class='form-input'>";
    html += "</div>";
    
    html += "</div>"; // End grid
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='autoChangeover' " + String(autoChangeover ? "checked" : "") + ">";
    html += "<label class='form-label'>Auto Changeover</label>";
    html += "</div>";
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='fanRelayNeeded' " + String(fanRelayNeeded ? "checked" : "") + ">";
    html += "<label class='form-label'>Fan Relay Required</label>";
    html += "</div>";
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='useFahrenheit' " + String(useFahrenheit ? "checked" : "") + ">";
    html += "<label class='form-label'>Use Fahrenheit</label>";
    html += "</div>";
    
    html += "</div>"; // End basic settings section
    
    // HVAC Advanced Settings
    html += "<div class='settings-section'>";
    html += "<h3>HVAC Advanced Settings</h3>";
    
    html += "<div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 16px;'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Stage 1 Min Runtime (seconds)</label>";
    html += "<input type='number' name='stage1MinRuntime' value='" + String(stage1MinRuntime) + "' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Stage 2 Temp Delta</label>";
    html += "<input type='number' name='stage2TempDelta' value='" + String(stage2TempDelta, 1) + "' step='0.1' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Fan Minutes Per Hour</label>";
    html += "<input type='number' name='fanMinutesPerHour' value='" + String(fanMinutesPerHour) + "' class='form-input'>";
    html += "</div>";
    
    html += "</div>"; // End grid
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='stage2HeatingEnabled' " + String(stage2HeatingEnabled ? "checked" : "") + ">";
    html += "<label class='form-label'>Enable 2nd Stage Heating</label>";
    html += "</div>";
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='stage2CoolingEnabled' " + String(stage2CoolingEnabled ? "checked" : "") + ">";
    html += "<label class='form-label'>Enable 2nd Stage Cooling</label>";
    html += "</div>";
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='hydronicHeatingEnabled' " + String(hydronicHeatingEnabled ? "checked" : "") + ">";
    html += "<label class='form-label'>Hydronic Heating Enabled</label>";
    html += "</div>";
    
    html += "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 16px;'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Hydronic Temp Low</label>";
    html += "<input type='number' name='hydronicTempLow' value='" + String(hydronicTempLow, 1) + "' step='0.5' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Hydronic Temp High</label>";
    html += "<input type='number' name='hydronicTempHigh' value='" + String(hydronicTempHigh, 1) + "' step='0.5' class='form-input'>";
    html += "</div>";
    
    html += "</div>"; // End grid
    
    html += "</div>"; // End HVAC settings section
    
    // Network & Connectivity Settings
    html += "<div class='settings-section'>";
    html += "<h3>Network & Connectivity</h3>";
    
    html += "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 16px;'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>WiFi SSID</label>";
    html += "<input type='text' name='wifiSSID' value='" + wifiSSID + "' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>WiFi Password</label>";
    html += "<input type='password' name='wifiPassword' value='" + wifiPassword + "' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Hostname</label>";
    html += "<input type='text' name='hostname' value='" + hostname + "' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Time Zone</label>";
    html += "<select name='timeZone' class='form-select'>";
    html += "<option value='EST5EDT,M3.2.0,M11.1.0'" + String(timeZone == "EST5EDT,M3.2.0,M11.1.0" ? " selected" : "") + ">Eastern Time (EST/EDT)</option>";
    html += "<option value='CST6CDT,M3.2.0,M11.1.0'" + String(timeZone == "CST6CDT,M3.2.0,M11.1.0" ? " selected" : "") + ">Central Time (CST/CDT)</option>";
    html += "<option value='MST7MDT,M3.2.0,M11.1.0'" + String(timeZone == "MST7MDT,M3.2.0,M11.1.0" ? " selected" : "") + ">Mountain Time (MST/MDT)</option>";
    html += "<option value='PST8PDT,M3.2.0,M11.1.0'" + String(timeZone == "PST8PDT,M3.2.0,M11.1.0" ? " selected" : "") + ">Pacific Time (PST/PDT)</option>";
    html += "<option value='AKST9AKDT,M3.2.0,M11.1.0'" + String(timeZone == "AKST9AKDT,M3.2.0,M11.1.0" ? " selected" : "") + ">Alaska Time (AKST/AKDT)</option>";
    html += "<option value='HST10'" + String(timeZone == "HST10" ? " selected" : "") + ">Hawaii Time (HST)</option>";
    html += "<option value='GMT0BST,M3.5.0,M10.5.0'" + String(timeZone == "GMT0BST,M3.5.0,M10.5.0" ? " selected" : "") + ">UK Time (GMT/BST)</option>";
    html += "<option value='CET-1CEST,M3.5.0,M10.5.0'" + String(timeZone == "CET-1CEST,M3.5.0,M10.5.0" ? " selected" : "") + ">Central Europe (CET/CEST)</option>";
    html += "<option value='JST-9'" + String(timeZone == "JST-9" ? " selected" : "") + ">Japan Time (JST)</option>";
    html += "<option value='AEST-10AEDT,M10.1.0,M4.1.0'" + String(timeZone == "AEST-10AEDT,M10.1.0,M4.1.0" ? " selected" : "") + ">Australia East (AEST/AEDT)</option>";
    html += "</select>";
    html += "</div>";
    
    html += "</div>"; // End grid
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='use24HourClock' " + String(use24HourClock ? "checked" : "") + ">";
    html += "<label class='form-label'>Use 24-Hour Clock Format</label>";
    html += "</div>";
    
    html += "</div>"; // End Network settings section
    
    // MQTT Settings
    html += "<div class='settings-section'>";
    html += "<h3>MQTT Settings</h3>";
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='mqttEnabled' " + String(mqttEnabled ? "checked" : "") + ">";
    html += "<label class='form-label'>Enable MQTT</label>";
    html += "</div>";
    
    html += "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 16px;'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>MQTT Server</label>";
    html += "<input type='text' name='mqttServer' value='" + mqttServer + "' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>MQTT Port</label>";
    html += "<input type='number' name='mqttPort' value='" + String(mqttPort) + "' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>MQTT Username</label>";
    html += "<input type='text' name='mqttUsername' value='" + mqttUsername + "' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>MQTT Password</label>";
    html += "<input type='password' name='mqttPassword' value='" + mqttPassword + "' class='form-input'>";
    html += "</div>";
    
    html += "</div>"; // End grid
    
    html += "</div>"; // End MQTT settings section
    
    // Sensor & Display Settings
    html += "<div class='settings-section'>";
    html += "<h3>Sensor & Display Settings</h3>";
    
    html += "<div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 16px;'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Temperature Offset (°F)</label>";
    html += "<input type='number' name='tempOffset' value='" + String(tempOffset, 1) + "' step='0.1' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Humidity Offset (%)</label>";
    html += "<input type='number' name='humidityOffset' value='" + String(humidityOffset, 1) + "' step='0.1' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Display Brightness (0-255)</label>";
    html += "<input type='number' name='currentBrightness' value='" + String(currentBrightness) + "' min='30' max='255' class='form-input'>";
    html += "</div>";
    
    html += "</div>"; // End grid
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='displaySleepEnabled' " + String(displaySleepEnabled ? "checked" : "") + ">";
    html += "<label class='form-label'>Enable Display Sleep</label>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Display Sleep Timeout (minutes)</label>";
    html += "<input type='number' name='displaySleepTimeout' value='" + String(displaySleepTimeout / 60000) + "' class='form-input'>";
    html += "</div>";
    
    html += "</div>"; // End sensor settings section
    
    // Settings Actions
    html += "<div class='settings-section'>";
    html += "<h3>Settings Actions</h3>";
    html += "<div class='button-group'>";
    html += "<input type='submit' value='Save All Settings' class='btn btn-primary'>";
    html += "</div>";
    html += "</div>"; // End settings actions section
    
    html += "</form>";
    html += "</div>"; // End settings-content tab
    
    // Schedule tab content (embedded schedule interface)
    html += "<div id='schedule-content' class='tab-content content'>";
    html += "<form action='/schedule_set' method='POST'>";
    
    // Master schedule control section
    html += "<div class='settings-section'>";
    html += "<h3>";
    html += ICON_CLOCK;
    html += " Schedule Control</h3>";
    
    html += "<div class='control-group'>";
    html += "<label class='toggle-switch'>";
    html += "<input type='checkbox' name='scheduleEnabled'";
    if (scheduleEnabled) html += " checked";
    html += ">";
    html += "<span class='toggle-slider'></span>";
    html += "</label>";
    html += "<span class='control-label'>Enable 7-Day Schedule</span>";
    html += "</div>";
    
    // Schedule override controls (always visible)
    html += "<div class='control-group'>";
    html += "<label for='scheduleOverride'>Schedule Override:</label>";
    html += "<select name='scheduleOverride' class='form-select'>";
    html += "<option value='resume'";
    if (!scheduleOverride) html += " selected";
    html += ">Follow Schedule</option>";
    html += "<option value='temporary'";
    if (scheduleOverride) html += " selected";
    html += ">Override for 2 Hours</option>";
    html += "<option value='permanent'>Override Until Resumed</option>";
    html += "</select>";
    html += "</div>";
    
    // Current status display
    html += "<div style='padding: 12px; background: #f5f5f5; border-radius: 8px; margin: 16px 0;'>";
    html += "<p><strong>Current Status:</strong> ";
    if (scheduleEnabled) {
        html += "Schedule Active - " + activePeriod;
        if (scheduleOverride) html += " (Override Active)";
    } else {
        html += "Schedule Disabled";
    }
    html += "</p>";
    html += "</div>";
    
    html += "</div>"; // End schedule control section
    
    // Weekly schedule table (always visible)
    html += "<div class='settings-section'>";
    html += "<h3>";
    html += ICON_CALENDAR;
    html += " Weekly Schedule</h3>";
    html += "<p>Configure day and night temperatures for each day of the week.</p>";
    
    // Schedule table
    html += "<div class='schedule-table'>";
    html += "<div class='schedule-row schedule-header'>";
    html += "<div class='schedule-cell'>Day</div>";
    html += "<div class='schedule-cell'>Enable</div>";
    html += "<div class='schedule-cell'>Day Period</div>";
    html += "<div class='schedule-cell'>Day Temps</div>";
    html += "<div class='schedule-cell'>Night Period</div>";
    html += "<div class='schedule-cell'>Night Temps</div>";
    html += "</div>";
    
    String dayNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    
    for (int day = 0; day < 7; day++) {
        String dayPrefix = "day" + String(day) + "_";
        DaySchedule& schedule = weekSchedule[day];
        
        html += "<div class='schedule-row'>";
        
        // Day name
        html += "<div class='schedule-cell'><strong>" + dayNames[day] + "</strong></div>";
        
        // Enable checkbox
        html += "<div class='schedule-cell'>";
        html += "<label class='toggle-switch small'>";
        html += "<input type='checkbox' name='" + dayPrefix + "enabled'";
        if (schedule.enabled) html += " checked";
        html += ">";
        html += "<span class='toggle-slider'></span>";
        html += "</label>";
        html += "</div>";
        
        // Day period time
        html += "<div class='schedule-cell'>";
        html += "<input type='time' name='" + dayPrefix + "day_time' value='";
        html += String(schedule.day.hour < 10 ? "0" : "") + String(schedule.day.hour) + ":";
        html += String(schedule.day.minute < 10 ? "0" : "") + String(schedule.day.minute);
        html += "' class='form-input time-input'";
        if (!schedule.enabled) html += " disabled";
        html += ">";
        html += "</div>";
        
        // Day temps
        html += "<div class='schedule-cell'>";
        html += "<div class='temp-inputs'>";
        html += "<label class='temp-label'>Heat:</label>";
        html += "<input type='number' name='" + dayPrefix + "day_heat' value='" + String(schedule.day.heatTemp, 1);
        html += "' step='0.5' min='40' max='90' class='form-input temp-input'";
        if (!schedule.enabled) html += " disabled";
        html += ">";
        html += "<label class='temp-label'>Cool:</label>";
        html += "<input type='number' name='" + dayPrefix + "day_cool' value='" + String(schedule.day.coolTemp, 1);
        html += "' step='0.5' min='50' max='95' class='form-input temp-input'";
        if (!schedule.enabled) html += " disabled";
        html += ">";
        html += "<label class='temp-label'>Auto:</label>";
        html += "<input type='number' name='" + dayPrefix + "day_auto' value='" + String(schedule.day.autoTemp, 1);
        html += "' step='0.5' min='45' max='90' class='form-input temp-input'";
        if (!schedule.enabled) html += " disabled";
        html += ">";
        html += "</div>";
        html += "</div>";
        
        // Night period time
        html += "<div class='schedule-cell'>";
        html += "<input type='time' name='" + dayPrefix + "night_time' value='";
        html += String(schedule.night.hour < 10 ? "0" : "") + String(schedule.night.hour) + ":";
        html += String(schedule.night.minute < 10 ? "0" : "") + String(schedule.night.minute);
        html += "' class='form-input time-input'";
        if (!schedule.enabled) html += " disabled";
        html += ">";
        html += "</div>";
        
        // Night temps
        html += "<div class='schedule-cell'>";
        html += "<div class='temp-inputs'>";
        html += "<label class='temp-label'>Heat:</label>";
        html += "<input type='number' name='" + dayPrefix + "night_heat' value='" + String(schedule.night.heatTemp, 1);
        html += "' step='0.5' min='40' max='90' class='form-input temp-input'";
        if (!schedule.enabled) html += " disabled";
        html += ">";
        html += "<label class='temp-label'>Cool:</label>";
        html += "<input type='number' name='" + dayPrefix + "night_cool' value='" + String(schedule.night.coolTemp, 1);
        html += "' step='0.5' min='50' max='95' class='form-input temp-input'";
        if (!schedule.enabled) html += " disabled";
        html += ">";
        html += "<label class='temp-label'>Auto:</label>";
        html += "<input type='number' name='" + dayPrefix + "night_auto' value='" + String(schedule.night.autoTemp, 1);
        html += "' step='0.5' min='45' max='90' class='form-input temp-input'";
        if (!schedule.enabled) html += " disabled";
        html += ">";
        html += "</div>";
        html += "</div>";
        
        html += "</div>"; // End schedule row
    }
    
    html += "</div>"; // End schedule table
    html += "</div>"; // End weekly schedule section
    
    // Schedule actions
    html += "<div class='settings-section'>";
    html += "<h3>Schedule Actions</h3>";
    html += "<div class='button-group'>";
    html += "<input type='submit' value='Save Schedule Settings' class='btn btn-primary'>";
    html += "</div>";
    html += "</div>";
    
    html += "</form>";
    html += "</div>"; // End schedule-content tab
    
    // System tab content
    html += "<div id='system-content' class='tab-content content'>";
    html += "<div class='status-card'>";
    html += "<div class='card-header'>";
    html += ICON_SETTINGS;
    html += "<h2 class='card-title' style='color: #2196F3;'>System Information</h2>";
    html += "</div>";
    html += "<div style='padding: 16px;'>";
    html += "<p><strong>Firmware Version:</strong> <span style='color: #4CAF50;'>" + version_info + "</span></p>";
    html += "<p><strong>Device Hostname:</strong> " + hostname + "</p>";
    html += "<p><strong>WiFi Network:</strong> " + wifiSSID + "</p>";
    html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
    html += "<p><strong>MAC Address:</strong> " + WiFi.macAddress() + "</p>";
    html += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
    html += "<p><strong>Uptime:</strong> " + String(millis() / 1000) + " seconds</p>";
    html += "<p><strong>Flash Size:</strong> " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB</p>";
    html += "<p><strong>Chip Model:</strong> " + String(ESP.getChipModel()) + "</p>";
    html += "<p><strong>CPU Frequency:</strong> " + String(ESP.getCpuFreqMHz()) + " MHz</p>";
    html += "</div>";
    html += "</div>";
    
    html += "<div class='status-card' style='margin-top: 24px;'>";
    html += "<div class='card-header'>";
    html += ICON_UPDATE;
    html += "<h2 class='card-title' style='color: #2196F3;'>📤 Firmware Update</h2>";
    html += "</div>";
    html += "<div style='padding: 16px;'>";
    html += "<form method='POST' action='/update' enctype='multipart/form-data' style='margin-bottom: 16px;'>";
    html += "<div style='border: 2px dashed #ccc; padding: 20px; text-align: center; border-radius: 8px; margin: 16px 0;'>";
    html += "<p><strong>Select Firmware File (.bin):</strong></p>";
    html += "<input type='file' name='update' accept='.bin' required style='margin: 10px 0;'>";
    html += "<br><button type='submit' class='btn btn-primary' onclick='return confirm(\"Are you sure you want to update the firmware? The device will reboot after update.\")'>📤 Upload Firmware</button>";
    html += "</div>";
    html += "</form>";
    html += "<p style='font-size: 0.9em; color: #666;'><em>⚠️ Only upload firmware (.bin) files. Device will reboot automatically after successful update.</em></p>";
    html += "</div>";
    html += "</div>";
    
    html += "<div class='status-card' style='margin-top: 24px;'>";
    html += "<div class='card-header'>";
    html += ICON_SETTINGS;
    html += "<h2 class='card-title' style='color: #FF9800;'>System Actions</h2>";
    html += "</div>";
    html += "<div class='button-group' style='padding: 16px;'>";
    html += "<a href='/reboot' class='btn btn-secondary' onclick='return confirm(\"Are you sure you want to reboot the device?\")'>♻️ Reboot Device</a>";
    html += "<a href='/confirm_restore' class='btn btn-danger' onclick='return confirm(\"WARNING: This will reset all settings to defaults. Are you sure?\")'>⚠️ Factory Reset</a>";
    html += "</div>";
    html += "</div>";
    html += "</div>"; // End system-content tab
    
    html += "</div>"; // End container
    html += JAVASCRIPT_CODE;
    html += "</body></html>";
    
    return html;
}

// Generate modern settings page HTML
String generateSettingsPage(String thermostatMode, String fanMode, float setTempHeat, 
                           float setTempCool, float setTempAuto, float tempSwing, 
                           float autoTempSwing, bool fanRelayNeeded, bool useFahrenheit,
                           bool mqttEnabled, int stage1MinRuntime, float stage2TempDelta,
                           bool stage2HeatingEnabled, bool stage2CoolingEnabled,
                           bool hydronicHeatingEnabled, float hydronicTempLow, 
                           float hydronicTempHigh, int fanMinutesPerHour,
                           String mqttServer, int mqttPort, String mqttUsername,
                           String mqttPassword, String wifiSSID, String wifiPassword,
                           String hostname, bool use24HourClock, String timeZone,
                           float tempOffset, float humidityOffset, bool displaySleepEnabled,
                           unsigned long displaySleepTimeout) {
    
    String html = "<!DOCTYPE html><html lang='en'><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>ESP32 Simple Thermostat - Settings</title>";
    html += CSS_STYLES;
    html += "</head><body>";
    
    html += "<div class='container'>";
    
    // Header
    html += "<div class='header'>";
    html += "<h1>Thermostat Settings</h1>";
    html += "<div class='version'>Configure your smart thermostat</div>";
    html += "</div>";
    
    html += "<div class='content'>";
    
    // Settings form
    html += "<form action='/set' method='POST'>";
    
    // Basic Settings Section
    html += "<div class='settings-section'>";
    html += "<h3>Basic Settings</h3>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Thermostat Mode</label>";
    html += "<select name='thermostatMode' class='form-select'>";
    html += "<option value='off'" + String(thermostatMode == "off" ? " selected" : "") + ">Off</option>";
    html += "<option value='heat'" + String(thermostatMode == "heat" ? " selected" : "") + ">Heat</option>";
    html += "<option value='cool'" + String(thermostatMode == "cool" ? " selected" : "") + ">Cool</option>";
    html += "<option value='auto'" + String(thermostatMode == "auto" ? " selected" : "") + ">Auto</option>";
    html += "</select>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Fan Mode</label>";
    html += "<select name='fanMode' class='form-select'>";
    html += "<option value='auto'" + String(fanMode == "auto" ? " selected" : "") + ">Auto</option>";
    html += "<option value='on'" + String(fanMode == "on" ? " selected" : "") + ">On</option>";
    html += "<option value='cycle'" + String(fanMode == "cycle" ? " selected" : "") + ">Cycle</option>";
    html += "</select>";
    html += "</div>";
    
    html += "<div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 16px;'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Heat Setpoint</label>";
    html += "<input type='number' name='setTempHeat' value='" + String(setTempHeat, 1) + "' step='0.5' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Cool Setpoint</label>";
    html += "<input type='number' name='setTempCool' value='" + String(setTempCool, 1) + "' step='0.5' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Auto Setpoint</label>";
    html += "<input type='number' name='setTempAuto' value='" + String(setTempAuto, 1) + "' step='0.5' class='form-input'>";
    html += "</div>";
    
    html += "</div>"; // End grid
    
    html += "<div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 16px;'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Temperature Swing</label>";
    html += "<input type='number' name='tempSwing' value='" + String(tempSwing, 1) + "' step='0.1' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Auto Temp Swing</label>";
    html += "<input type='number' name='autoTempSwing' value='" + String(autoTempSwing, 1) + "' step='0.1' class='form-input'>";
    html += "</div>";
    
    html += "</div>"; // End grid
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='fanRelayNeeded' " + String(fanRelayNeeded ? "checked" : "") + ">";
    html += "<label class='form-label'>Fan Relay Required</label>";
    html += "</div>";
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='useFahrenheit' " + String(useFahrenheit ? "checked" : "") + ">";
    html += "<label class='form-label'>Use Fahrenheit</label>";
    html += "</div>";
    
    html += "</div>"; // End basic settings section
    
    // HVAC Advanced Settings
    html += "<div class='settings-section'>";
    html += "<h3>HVAC Advanced Settings</h3>";
    
    html += "<div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 16px;'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Stage 1 Min Runtime (seconds)</label>";
    html += "<input type='number' name='stage1MinRuntime' value='" + String(stage1MinRuntime) + "' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Stage 2 Temp Delta</label>";
    html += "<input type='number' name='stage2TempDelta' value='" + String(stage2TempDelta, 1) + "' step='0.1' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Fan Minutes Per Hour</label>";
    html += "<input type='number' name='fanMinutesPerHour' value='" + String(fanMinutesPerHour) + "' class='form-input'>";
    html += "</div>";
    
    html += "</div>"; // End grid
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='stage2HeatingEnabled' " + String(stage2HeatingEnabled ? "checked" : "") + ">";
    html += "<label class='form-label'>Enable 2nd Stage Heating</label>";
    html += "</div>";
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='stage2CoolingEnabled' " + String(stage2CoolingEnabled ? "checked" : "") + ">";
    html += "<label class='form-label'>Enable 2nd Stage Cooling</label>";
    html += "</div>";
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='hydronicHeatingEnabled' " + String(hydronicHeatingEnabled ? "checked" : "") + ">";
    html += "<label class='form-label'>Hydronic Heating Enabled</label>";
    html += "</div>";
    
    if (hydronicHeatingEnabled) {
        html += "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 16px;'>";
        
        html += "<div class='form-group'>";
        html += "<label class='form-label'>Hydronic Temp Low</label>";
        html += "<input type='number' name='hydronicTempLow' value='" + String(hydronicTempLow, 1) + "' step='0.5' class='form-input'>";
        html += "</div>";
        
        html += "<div class='form-group'>";
        html += "<label class='form-label'>Hydronic Temp High</label>";
        html += "<input type='number' name='hydronicTempHigh' value='" + String(hydronicTempHigh, 1) + "' step='0.5' class='form-input'>";
        html += "</div>";
        
        html += "</div>"; // End grid
    }
    
    html += "</div>"; // End HVAC settings section
    
    // Network & Connectivity Settings
    html += "<div class='settings-section'>";
    html += "<h3>Network & Connectivity</h3>";
    
    html += "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 16px;'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>WiFi SSID</label>";
    html += "<input type='text' name='wifiSSID' value='" + wifiSSID + "' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>WiFi Password</label>";
    html += "<input type='password' name='wifiPassword' value='" + wifiPassword + "' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Hostname</label>";
    html += "<input type='text' name='hostname' value='" + hostname + "' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Time Zone</label>";
    html += "<input type='text' name='timeZone' value='" + timeZone + "' class='form-input' placeholder='e.g., CST6CDT,M3.2.0,M11.1.0'>";
    html += "</div>";
    
    html += "</div>"; // End grid
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='use24HourClock' " + String(use24HourClock ? "checked" : "") + ">";
    html += "<label class='form-label'>Use 24-Hour Clock Format</label>";
    html += "</div>";
    
    html += "</div>"; // End Network settings section
    
    // MQTT Settings
    html += "<div class='settings-section'>";
    html += "<h3>MQTT Settings</h3>";
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='mqttEnabled' " + String(mqttEnabled ? "checked" : "") + ">";
    html += "<label class='form-label'>Enable MQTT</label>";
    html += "</div>";
    
    if (mqttEnabled) {
        html += "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 16px;'>";
        
        html += "<div class='form-group'>";
        html += "<label class='form-label'>MQTT Server</label>";
        html += "<input type='text' name='mqttServer' value='" + mqttServer + "' class='form-input'>";
        html += "</div>";
        
        html += "<div class='form-group'>";
        html += "<label class='form-label'>MQTT Port</label>";
        html += "<input type='number' name='mqttPort' value='" + String(mqttPort) + "' class='form-input'>";
        html += "</div>";
        
        html += "<div class='form-group'>";
        html += "<label class='form-label'>MQTT Username</label>";
        html += "<input type='text' name='mqttUsername' value='" + mqttUsername + "' class='form-input'>";
        html += "</div>";
        
        html += "<div class='form-group'>";
        html += "<label class='form-label'>MQTT Password</label>";
        html += "<input type='password' name='mqttPassword' value='" + mqttPassword + "' class='form-input'>";
        html += "</div>";
        
        html += "</div>"; // End grid
    }
    
    html += "</div>"; // End MQTT settings section
    
    // Sensor & Display Settings
    html += "<div class='settings-section'>";
    html += "<h3>Sensor & Display Settings</h3>";
    
    html += "<div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 16px;'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Temperature Offset (°F)</label>";
    html += "<input type='number' name='tempOffset' value='" + String(tempOffset, 1) + "' step='0.1' class='form-input'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Humidity Offset (%)</label>";
    html += "<input type='number' name='humidityOffset' value='" + String(humidityOffset, 1) + "' step='0.1' class='form-input'>";
    html += "</div>";
    
    html += "</div>"; // End grid
    
    html += "<div class='form-checkbox'>";
    html += "<input type='checkbox' name='displaySleepEnabled' " + String(displaySleepEnabled ? "checked" : "") + ">";
    html += "<label class='form-label'>Enable Display Sleep</label>";
    html += "</div>";
    
    if (displaySleepEnabled) {
        html += "<div class='form-group'>";
        html += "<label class='form-label'>Display Sleep Timeout (minutes)</label>";
        html += "<input type='number' name='displaySleepTimeout' value='" + String(displaySleepTimeout / 60000) + "' class='form-input'>";
        html += "</div>";
    }
    
    html += "</div>"; // End sensor settings section
    
    html += "<div class='button-group'>";
    html += "<input type='submit' value='Save All Settings' class='btn btn-primary'>";
    html += "<a href='/' class='btn btn-secondary'>Back to Status</a>";
    html += "</div>";
    
    html += "</form>";
    html += "</div>"; // End content
    html += "</div>"; // End container
    
    html += JAVASCRIPT_CODE;
    html += "</body></html>";
    
    return html;
}

// Generate OTA update page
String generateOTAPage() {
    String html = "<!DOCTYPE html><html lang='en'><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>ESP32 Simple Thermostat - OTA Update</title>";
    html += CSS_STYLES;
    html += "</head><body>";
    
    html += "<div class='container'>";
    
    html += "<div class='header'>";
    html += "<h1>Over-The-Air Update</h1>";
    html += "<div class='version'>Upload new firmware</div>";
    html += "</div>";
    
    html += "<div class='content'>";
    
    html += "<div class='alert alert-warning'>";
    html += "<strong>Warning:</strong> Do not power off the device during the update process. ";
    html += "The update may take several minutes to complete.";
    html += "</div>";
    
    html += "<div class='settings-section'>";
    html += "<h3>Firmware Upload</h3>";
    html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Select Firmware File (.bin)</label>";
    html += "<input type='file' name='update' accept='.bin' class='form-input' style='padding: 8px;'>";
    html += "</div>";
    html += "<div class='button-group'>";
    html += "<input type='submit' value='Upload & Install' class='btn btn-primary' onclick='showUploadProgress()'>";
    html += "<a href='/' class='btn btn-secondary'>Cancel</a>";
    html += "</div>";
    html += "</form>";
    html += "</div>";
    
    html += "<div id='upload-progress' style='display: none;'>";
    html += "<div class='settings-section'>";
    html += "<h3>Uploading...</h3>";
    html += "<div class='progress-bar'>";
    html += "<div class='progress-fill' style='width: 0%;'></div>";
    html += "</div>";
    html += "<p>Please wait while the firmware is being uploaded and installed.</p>";
    html += "</div>";
    html += "</div>";
    
    html += "</div>"; // End content
    html += "</div>"; // End container
    
    html += "<script>";
    html += "function showUploadProgress() {";
    html += "  document.getElementById('upload-progress').style.display = 'block';";
    html += "  // Simulate progress (in real implementation, this would be actual progress)";
    html += "  let progress = 0;";
    html += "  const progressBar = document.querySelector('.progress-fill');";
    html += "  const interval = setInterval(() => {";
    html += "    progress += 2;";
    html += "    progressBar.style.width = progress + '%';";
    html += "    if (progress >= 100) clearInterval(interval);";
    html += "  }, 100);";
    html += "}";
    html += "</script>";
    
    html += "</body></html>";
    
    return html;
}

// Generate factory reset confirmation page
String generateFactoryResetPage() {
    String html = "<!DOCTYPE html><html lang='en'><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>ESP32 Simple Thermostat - Factory Reset</title>";
    html += CSS_STYLES;
    html += "</head><body>";
    
    html += "<div class='container'>";
    
    html += "<div class='header'>";
    html += "<h1>Factory Reset</h1>";
    html += "<div class='version'>Restore default settings</div>";
    html += "</div>";
    
    html += "<div class='content'>";
    
    html += "<div class='alert alert-error'>";
    html += "<strong>Warning:</strong> This action will permanently delete all your settings ";
    html += "and restore the thermostat to factory defaults. This cannot be undone.";
    html += "</div>";
    
    html += "<div class='settings-section'>";
    html += "<h3>Confirm Factory Reset</h3>";
    html += "<p>The following settings will be reset to defaults:</p>";
    html += "<ul style='margin: 16px 0; padding-left: 24px;'>";
    html += "<li>Temperature setpoints and swing settings</li>";
    html += "<li>HVAC staging configuration</li>";
    html += "<li>WiFi credentials</li>";
    html += "<li>MQTT server settings</li>";
    html += "<li>Display and calibration settings</li>";
    html += "<li>All custom preferences</li>";
    html += "</ul>";
    
    html += "<div class='button-group'>";
    html += "<form action='/restore_defaults' method='POST' style='display: inline;'>";
    html += "<button type='submit' class='btn btn-danger' onclick='return confirm(\"Are you absolutely sure? This cannot be undone!\")'>Yes, Reset Everything</button>";
    html += "</form>";
    html += "<a href='/' class='btn btn-secondary'>Cancel</a>";
    html += "</div>";
    html += "</div>";
    
    html += "</div>"; // End content
    html += "</div>"; // End container
    
    html += "</body></html>";
    
    return html;
}

// Generate schedule management page HTML
String generateSchedulePage(DaySchedule weekSchedule[7], bool scheduleEnabled, String activePeriod,
                           bool scheduleOverride, bool use24HourClock) {
    String html = "<!DOCTYPE html><html lang='en'><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>ESP32 Simple Thermostat - Schedule</title>";
    html += CSS_STYLES;
    html += "</head><body>";
    
    html += "<div class='container'>";
    
    // Header
    html += "<div class='header'>";
    html += "<h1>7-Day Temperature Schedule</h1>";
    html += "<div class='version'>Active Period: " + activePeriod;
    if (scheduleOverride) html += " (Override Active)";
    html += "</div>";
    html += "</div>";
    
    // Navigation back
    html += "<div style='margin-bottom: 20px;'>";
    html += "<a href='/' class='btn btn-secondary'>";
    html += ICON_BACK;
    html += " Back to Status</a>";
    html += "</div>";
    
    html += "<form action='/schedule_set' method='POST'>";
    
    // Master schedule control
    html += "<div class='settings-section'>";
    html += "<h3>";
    html += ICON_CLOCK;
    html += " Schedule Control</h3>";
    
    html += "<div class='control-group'>";
    html += "<label class='toggle-switch'>";
    html += "<input type='checkbox' name='scheduleEnabled'";
    if (scheduleEnabled) html += " checked";
    html += ">";
    html += "<span class='toggle-slider'></span>";
    html += "</label>";
    html += "<span class='control-label'>Enable 7-Day Schedule</span>";
    html += "</div>";
    
    // Override controls
    if (scheduleEnabled) {
        html += "<div class='control-group'>";
        html += "<label for='scheduleOverride'>Schedule Override:</label>";
        html += "<select name='scheduleOverride' class='form-control'>";
        html += "<option value='resume'";
        if (!scheduleOverride) html += " selected";
        html += ">Follow Schedule</option>";
        html += "<option value='temporary'";
        if (scheduleOverride) html += " selected";
        html += ">Override for 2 Hours</option>";
        html += "<option value='permanent'>Override Until Resumed</option>";
        html += "</select>";
        html += "</div>";
    }
    
    html += "</div>";
    
    // Schedule table
    if (scheduleEnabled) {
        html += "<div class='settings-section'>";
        html += "<h3>";
        html += ICON_CALENDAR;
        html += " Weekly Schedule</h3>";
        html += "<p>Configure day and night temperatures for each day of the week.</p>";
        
        // Table header
        html += "<div class='schedule-table'>";
        html += "<div class='schedule-row schedule-header'>";
        html += "<div class='schedule-cell'>Day</div>";
        html += "<div class='schedule-cell'>Enable</div>";
        html += "<div class='schedule-cell'>Day Period</div>";
        html += "<div class='schedule-cell'>Day Temps (H/C/A)</div>";
        html += "<div class='schedule-cell'>Night Period</div>";
        html += "<div class='schedule-cell'>Night Temps (H/C/A)</div>";
        html += "</div>";
        
        String dayNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
        
        for (int day = 0; day < 7; day++) {
            String dayPrefix = "day" + String(day) + "_";
            DaySchedule& schedule = weekSchedule[day];
            
            html += "<div class='schedule-row'>";
            
            // Day name
            html += "<div class='schedule-cell'><strong>" + dayNames[day] + "</strong></div>";
            
            // Enable checkbox
            html += "<div class='schedule-cell'>";
            html += "<label class='toggle-switch small'>";
            html += "<input type='checkbox' name='" + dayPrefix + "enabled'";
            if (schedule.enabled) html += " checked";
            html += ">";
            html += "<span class='toggle-slider'></span>";
            html += "</label>";
            html += "</div>";
            
            // Day period time
            html += "<div class='schedule-cell'>";
            html += "<div class='time-input'>";
            html += "<input type='number' name='" + dayPrefix + "d_hour' min='0' max='23' value='" + String(schedule.day.hour) + "' class='time-field'>";
            html += "<span>:</span>";
            html += "<input type='number' name='" + dayPrefix + "d_min' min='0' max='59' value='" + String(schedule.day.minute) + "' class='time-field'>";
            html += "</div>";
            html += "<label class='checkbox-small'>";
            html += "<input type='checkbox' name='" + dayPrefix + "d_active'";
            if (schedule.day.active) html += " checked";
            html += "> Active";
            html += "</label>";
            html += "</div>";
            
            // Day period temperatures
            html += "<div class='schedule-cell'>";
            html += "<div class='temp-input'>";
            html += "<label>Heat:</label>";
            html += "<input type='number' name='" + dayPrefix + "d_heat' min='50' max='95' step='0.5' value='" + String(schedule.day.heatTemp, 1) + "' class='temp-field'>";
            html += "<label>Cool:</label>";
            html += "<input type='number' name='" + dayPrefix + "d_cool' min='50' max='95' step='0.5' value='" + String(schedule.day.coolTemp, 1) + "' class='temp-field'>";
            html += "</div>";
            html += "</div>";
            
            // Night period time
            html += "<div class='schedule-cell'>";
            html += "<div class='time-input'>";
            html += "<input type='number' name='" + dayPrefix + "n_hour' min='0' max='23' value='" + String(schedule.night.hour) + "' class='time-field'>";
            html += "<span>:</span>";
            html += "<input type='number' name='" + dayPrefix + "n_min' min='0' max='59' value='" + String(schedule.night.minute) + "' class='time-field'>";
            html += "</div>";
            html += "<label class='checkbox-small'>";
            html += "<input type='checkbox' name='" + dayPrefix + "n_active'";
            if (schedule.night.active) html += " checked";
            html += "> Active";
            html += "</label>";
            html += "</div>";
            
            // Night period temperatures
            html += "<div class='schedule-cell'>";
            html += "<div class='temp-input'>";
            html += "<label>Heat:</label>";
            html += "<input type='number' name='" + dayPrefix + "n_heat' min='50' max='95' step='0.5' value='" + String(schedule.night.heatTemp, 1) + "' class='temp-field'>";
            html += "<label>Cool:</label>";
            html += "<input type='number' name='" + dayPrefix + "n_cool' min='50' max='95' step='0.5' value='" + String(schedule.night.coolTemp, 1) + "' class='temp-field'>";
            html += "</div>";
            html += "</div>";
            
            html += "</div>"; // End schedule row
        }
        
        html += "</div>"; // End schedule table
        html += "</div>"; // End settings section
    }
    
    // Save button
    html += "<div class='button-group'>";
    html += "<button type='submit' class='btn btn-primary'>";
    html += ICON_SAVE;
    html += " Save Schedule</button>";
    html += "<a href='/' class='btn btn-secondary'>Cancel</a>";
    html += "</div>";
    
    html += "</form>";
    html += "</div>"; // End container
    
    // Add custom CSS for schedule table
    html += "<style>";
    html += ".schedule-table { display: table; width: 100%; border-collapse: collapse; margin: 16px 0; }";
    html += ".schedule-row { display: table-row; }";
    html += ".schedule-cell { display: table-cell; padding: 12px 8px; border: 1px solid #333; vertical-align: middle; }";
    html += ".schedule-header { background: #2c2c2c; font-weight: bold; }";
    html += ".schedule-row:nth-child(even) { background: rgba(255,255,255,0.05); }";
    html += ".time-input { display: flex; align-items: center; gap: 4px; margin-bottom: 8px; }";
    html += ".time-field { width: 45px; padding: 4px; background: #2c2c2c; border: 1px solid #555; color: white; text-align: center; }";
    html += ".temp-input { display: flex; flex-direction: column; gap: 4px; }";
    html += ".temp-input label { font-size: 12px; color: #ccc; }";
    html += ".temp-field { width: 60px; padding: 4px; background: #2c2c2c; border: 1px solid #555; color: white; }";
    html += ".toggle-switch.small { transform: scale(0.8); }";
    html += ".checkbox-small { font-size: 12px; display: flex; align-items: center; gap: 4px; }";
    html += ".checkbox-small input { margin: 0; }";
    html += "@media (max-width: 768px) {";
    html += "  .schedule-table, .schedule-row, .schedule-cell { display: block; }";
    html += "  .schedule-cell { border: none; border-bottom: 1px solid #333; padding: 8px 0; }";
    html += "  .schedule-header { display: none; }";
    html += "  .schedule-cell:before { content: attr(data-label) ': '; font-weight: bold; }";
    html += "}";
    html += "</style>";
    
    html += "</body></html>";
    
    return html;
}

#endif // WEBPAGES_H