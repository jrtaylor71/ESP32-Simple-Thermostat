# ESP32 Simple Thermostat

A comprehensive, feature-rich thermostat system built on the ESP32 platform with professional PCB design. Perfect for DIY smart home automation with full Home Assistant integration.

![Thermostat Display](https://github.com/jrtaylor71/ESP32-Simple-Thermostat/blob/main/img/IMG_20250501_151129758.png)

## 🌟 Key Features

- **📱 Local Touch Control**: ILI9341 TFT LCD with intuitive touch interface
- **🏠 Smart Home Ready**: Full MQTT integration with Home Assistant auto-discovery
- **🌡️ Dual Sensors**: DHT11 for ambient conditions + DS18B20 for hydronic systems
- **⚡ Multi-Stage HVAC**: Support for 2-stage heating and cooling systems
- **💨 Advanced Fan Control**: Auto, continuous, and scheduled cycling modes
- **🌐 Web Interface**: Complete web-based configuration and monitoring
- **📡 Offline Operation**: Full functionality without WiFi connection
- **🔧 Professional PCB**: Custom PCB design for clean, permanent installation
- **🔄 OTA Updates**: Over-the-air firmware updates
- **🔒 Factory Reset**: Built-in reset capability via boot button

## 📦 Backport Status

This firmware is backported from **Smart Thermostat Alt Firmware v1.1.0** (ESP32-S3) to ESP32-WROOM hardware.

**Removed Features:**
- LD2410 motion sensor (display wake)
- Status LEDs (heat/cool/fan indicators)
- Buzzer/audio feedback
- Light sensor (adaptive brightness)

**Retained Features:**
- ✅ 7-day scheduling system
- ✅ Modern tabbed web interface
- ✅ MQTT/Home Assistant integration
- ✅ Multi-stage HVAC control
- ✅ Hydronic heating with pump control
- ✅ Touch-based display wake
- ✅ OTA firmware updates
- ✅ All software improvements from v1.1.0

## 🚀 Quick Start

### Hardware Requirements
- ESP32 WROOM-32 Development Board
- ILI9341 320x240 TFT LCD Touch Display
- DHT11 Temperature/Humidity Sensor (digital pin)
- DS18B20 Temperature Sensor (optional, for hydronic heating)
- 6x Relay Module (5 HVAC + 1 hydronic pump)
- Custom PCB (files included) or breadboard for prototyping

### Software Setup
1. Install [PlatformIO](https://platformio.org/) IDE
2. Clone this repository
3. Open project in PlatformIO
4. Build and upload to ESP32
5. Use touch interface to configure WiFi and settings

### Pin Configuration

#### TFT Display (ILI9341)
```cpp
#define TFT_MISO 19    // SPI MISO
#define TFT_MOSI 23    // SPI MOSI
#define TFT_SCLK 18    // SPI Clock
#define TFT_CS   15    // Chip Select
#define TFT_DC    2    // Data/Command
#define TFT_RST   5    // Reset
#define TFT_BL   32    // Backlight (PWM)
#define TOUCH_CS  4    // Touch Chip Select
```

#### Sensors
```cpp
#define DHTPIN 22           // DHT11 data pin
#define ONE_WIRE_BUS 27     // DS18B20 data pin (optional)
```

#### Relay Outputs
```cpp
const int heatRelay1Pin = 13;   // Stage 1 heating
const int heatRelay2Pin = 12;   // Stage 2 heating
const int coolRelay1Pin = 14;   // Stage 1 cooling
const int coolRelay2Pin = 26;   // Stage 2 cooling
const int fanRelayPin = 25;     // Fan control
const int pumpRelayPin = 33;    // Hydronic pump control
```

## 💻 Web Interface

Access the thermostat's web interface by navigating to its IP address:

### Status Page
![Web Status]()

Real-time monitoring of:
- Current temperature and humidity
- Thermostat and fan modes
- Relay states
- System status

### Settings Page
![Web Settings]()

Complete configuration interface for:
- Temperature setpoints and control modes
- MQTT/Home Assistant integration
- WiFi network settings
- Multi-stage HVAC parameters
- Hydronic heating controls
- Fan scheduling options

## 🏠 Home Assistant Integration

Automatic discovery and integration with Home Assistant:

1. Enable MQTT in thermostat settings
2. Configure MQTT broker details
3. Thermostat appears automatically in Home Assistant
4. Full control via Home Assistant interface
5. Supports climate entity with heating/cooling modes

### MQTT Topics
- **Commands**: `esp32_thermostat/mode/set`, `esp32_thermostat/target_temperature/set`
- **Status**: `esp32_thermostat/current_temperature`, `esp32_thermostat/mode`
- **Discovery**: Automatic Home Assistant device discovery

## 🔧 Professional PCB

The project includes a complete custom PCB design:

![PCB Top](https://github.com/jrtaylor71/ESP32-Simple-Thermostat/blob/main/ESP32-Simple-Thermostat-PCB/ESP32-Simple-Thermostat-PCB-top.png)
![PCB Bottom](https://github.com/jrtaylor71/ESP32-Simple-Thermostat/blob/main/ESP32-Simple-Thermostat-PCB/ESP32-Simple-Thermostat-PCB-bottom.png)

### PCB Features
- Professional 4-layer design
- All components integrated on single board
- Labeled terminal blocks for easy HVAC wiring
- Proper grounding and signal integrity
- Standard mounting holes for enclosure installation

### Manufacturing Files
Complete manufacturing package in `ESP32-Simple-Thermostat-PCB/jlcpcb/`:
- Gerber files for PCB fabrication
- Drill files for via and hole placement
- Bill of Materials (BOM) for component sourcing
- Component Placement List (CPL) for assembly

## 📖 Documentation

- **[Complete Documentation](DOCUMENTATION.md)**: Comprehensive technical documentation
- **[Copilot File](.copilot)**: GitHub Copilot context for development
- **Inline Comments**: Extensive code documentation
- **Serial Debug**: Detailed debug output for troubleshooting

## 🛠️ Advanced Features

### Multi-Stage Operation
- Intelligent staging based on time and temperature
- Configurable stage 2 activation parameters
- Prevents system short-cycling
- Optimizes energy efficiency

### Hydronic Heating Support
- DS18B20 water temperature monitoring
- Safety interlocks prevent operation when water is too cold
- Configurable high/low temperature thresholds
- Perfect for radiant floor heating systems

### Fan Control Options
- **Auto**: Fan runs only with heating/cooling
- **On**: Continuous fan operation
- **Cycle**: Scheduled fan operation (configurable minutes per hour)

### Safety Features
- Watchdog timer prevents system lockups
- Factory reset via boot button (10+ seconds)
- Temperature limit enforcement
- Graceful offline operation

## 🔧 Factory Reset

Press and hold the boot button for more than 10 seconds while the thermostat is running to restore all settings to defaults.

## 📄 License

This project is released under the GNU General Public License v3.0. Free to use, modify, and distribute.

## 🤝 Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## 📞 Support

- **Issues**: Use GitHub Issues for bug reports
- **Discussions**: GitHub Discussions for questions
- **Documentation**: Comprehensive docs included
- **Serial Debug**: Detailed logging at 115200 baud

## ⭐ Version

**Current Version**: 1.1.0 (Backported from ESP32-S3)
- Backported from Smart Thermostat Alt Firmware v1.1.0
- Complete thermostat functionality
- MQTT/Home Assistant integration with auto-discovery
- 7-day scheduling system
- Multi-stage HVAC support with hydronic pump
- DHT11 sensor support
- Professional PCB design (original hardware)
- Web interface and OTA updates

---

**Created for the DIY smart home community**
