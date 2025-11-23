# ESP32-Simple-Thermostat Backport

This is a backported version of the Smart Thermostat Alt Firmware, adapted for basic ESP32-WROOM hardware with simplified hardware requirements.

## Hardware Removed from Original
- LD2410 Motion Sensor (pins 15, 16, 18)
- Status LEDs (Heat, Cool, Fan indicators) 
- Buzzer/Audio feedback
- Serial2 UART port usage
- Light sensor (adaptive brightness)
- Advanced GPIO configurations

## Hardware Requirements (Basic ESP32)
- ESP32-WROOM DevKit v1 (4MB Flash)
- ILI9341 320x240 TFT LCD with XPT2046 Touch Controller
- DHT11 Temperature/Humidity Sensor (digital single-pin)
- DS18B20 Temperature Sensor (optional, for hydronic heating)
- 6x Relay Module for HVAC control (5 HVAC + 1 pump)

## Pin Configuration (ESP32-WROOM)
```
TFT Display:
- MISO: GPIO 19
- MOSI: GPIO 23  
- SCLK: GPIO 18
- CS:   GPIO 15
- DC:   GPIO 2
- RST:  GPIO 5
- BL:   GPIO 32 (PWM backlight control)

Touch Screen:
- CS:   GPIO 4

Sensors:
- DHT11 Data: GPIO 22

Relays:
- Heat 1: GPIO 13
- Heat 2: GPIO 12
- Cool 1: GPIO 14
- Cool 2: GPIO 26
- Fan:    GPIO 25
- Pump:   GPIO 33

OneWire (DS18B20):
- Data: GPIO 27
```

## Software Features Retained
- ✅ 7-Day Scheduling System with day/night periods
- ✅ Modern Tabbed Web Interface (Status, Settings, Schedule, System)
- ✅ MQTT Integration with Home Assistant auto-discovery
- ✅ Multi-stage HVAC control with intelligent staging
- ✅ Hydronic heating support with safety interlocks  
- ✅ Advanced fan control (Auto, On, Cycle modes)
- ✅ Temperature calibration and offset settings
- ✅ Display sleep/wake via touch only
- ✅ OTA firmware updates
- ✅ Factory reset (10-second boot button)
- ✅ Comprehensive web configuration
- ✅ Persistent settings storage
- ✅ Dual-core FreeRTOS architecture

## Files to Copy to Your ESP32-Simple-Thermostat Repository
1. `src/Main-Thermostat-Basic.cpp` → `src/Main-Thermostat.cpp`
2. `include/WebPages-Basic.h` → `include/WebPages.h`  
3. `include/WebInterface.h` (unchanged - can use as-is)
4. `platformio-basic.ini` → `platformio.ini`

## Installation
1. Copy files to your existing ESP32-Simple-Thermostat repository
2. Update pin connections per the pin configuration above
3. Compile and upload using PlatformIO
4. Configure via web interface at device IP address

## Memory Usage
- Flash: 78.1% (1,023,273 bytes / 1,310,720 bytes)
- RAM: 14.9% (48,708 bytes / 327,680 bytes)
- Compilation: Successful with no errors

## Version
Backported from Smart Thermostat Alt Firmware v1.1.0
- All advanced software features
- Simplified hardware requirements
- Compatible with original ESP32-Simple-Thermostat hardware design