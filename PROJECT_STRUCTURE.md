# ESP32 Simple Thermostat - Project Structure

This document provides an overview of the project structure and organization.

## 📁 Project Directory Structure

```
ESP32-Simple-Thermostat/
├── 📄 README.md                          # Project overview and quick start guide (UPDATED)
├── 📄 DOCUMENTATION.md                   # Comprehensive technical documentation (UPDATED)
├── 📄 CURRENT_STATE.md                   # Current project status and deployment info (NEW)
├── 📄 PIN_CONFIGURATION.md               # Complete pin reference guide (NEW)
├── 📄 BACKPORT_README.md                 # Backport summary from ESP32-S3 (UPDATED)
├── 📄 BACKPORT_GUIDE.md                  # Backport implementation guide (UPDATED)
├── 📄 DEVELOPMENT_GUIDE.md               # Development and contribution guide
├── 📄 .copilot                          # GitHub Copilot configuration and context
├── 📄 PROJECT_STRUCTURE.md              # This file - project organization guide
├── 📄 platformio.ini                    # PlatformIO build configuration (backported)
├── 📄 platformio-basic.ini              # Backup of backported configuration
├── 📄 ESP32-Simple-Thermostat.code-workspace # VS Code workspace settings
├── 📄 firmware.bin                      # Compiled firmware (generated)
│
├── 📁 src/                              # Source code directory
│   └── 📄 Main-Thermostat.cpp          # Main application source (~3742 lines, backported)
│
├── 📁 include/                          # Header files
│   ├── 📄 WebInterface.h               # Web server route handlers (backported)
│   ├── 📄 WebPages.h                   # HTML templates for web UI (backported)
│   └── 📄 TFT_Setup_ESP32_S3_Thermostat.h # TFT config (reference only)
│
├── 📁 ESP32-Simple-Thermostat-PCB/     # Custom PCB design files
│   ├── 📄 ESP32-Simple-Thermostat-PCB.kicad_sch    # KiCad schematic file
│   ├── 📄 ESP32-Simple-Thermostat-PCB.kicad_pcb    # KiCad PCB layout file
│   ├── 📄 ESP32-Simple-Thermostat-PCB.kicad_pro    # KiCad project file
│   ├── 📄 ESP32-Simple-Thermostat-PCB.kicad_prl    # KiCad local settings
│   ├── 📄 fp-info-cache                            # KiCad footprint cache
│   ├── 📁 jlcpcb/                                  # Manufacturing files
│   │   ├── 📁 gerber/                              # Gerber files for PCB fab
│   │   │   ├── 📄 ESP32-Simple-Thermostat-PCB-CuTop.gbr
│   │   │   ├── 📄 ESP32-Simple-Thermostat-PCB-CuBottom.gbr
│   │   │   ├── 📄 ESP32-Simple-Thermostat-PCB-SilkTop.gbr
│   │   │   ├── 📄 ESP32-Simple-Thermostat-PCB-SilkBottom.gbr
│   │   │   ├── 📄 ESP32-Simple-Thermostat-PCB-MaskTop.gbr
│   │   │   ├── 📄 ESP32-Simple-Thermostat-PCB-MaskBottom.gbr
│   │   │   ├── 📄 ESP32-Simple-Thermostat-PCB-EdgeCuts.gbr
│   │   │   ├── 📄 ESP32-Simple-Thermostat-PCB-PTH.drl
│   │   │   └── 📄 ESP32-Simple-Thermostat-PCB-NPTH.drl
│   │   └── 📁 production_files/                    # Assembly files
│   │       ├── 📄 BOM-ESP32-Simple-Thermostat-PCB.csv  # Bill of Materials
│   │       └── 📄 CPL-ESP32-Simple-Thermostat-PCB.csv  # Component Placement
│   └── 📁 #auto_saved_files#                       # KiCad auto-save files
│
└── 📁 img/                              # Project images and screenshots
    ├── 📄 IMG_20250501_151129758.png    # Hardware photo
    ├── 📄 web-status.png                # Web interface status page
    ├── 📄 web-settings.png              # Web interface settings page
    └── 📄 Screenshot*.png               # Additional screenshots
```

## 📋 File Descriptions

### Core Project Files

#### `README.md` (UPDATED)
- Project overview and introduction
- Backport status from ESP32-S3 firmware
- Quick start guide and setup instructions
- Key features and capabilities overview
- Hardware requirements and pin configurations
- Web interface screenshots
- Home Assistant integration guide

#### `DOCUMENTATION.md` (UPDATED)
- Comprehensive technical documentation
- Backport changes and modifications
- Detailed architecture explanation
- Complete function reference
- Hardware integration guide
- Troubleshooting and maintenance
- Development and customization guide
- Version history with backport information

#### `CURRENT_STATE.md` (NEW)
- Current project status and readiness
- Complete backport summary
- Compilation results and memory usage
- Full pin configuration table
- Software features retained/removed
- Deployment instructions
- Testing checklist
- Known working configuration

#### `PIN_CONFIGURATION.md` (NEW)
- Quick reference pin table
- Detailed wiring instructions
- SPI bus sharing explanation
- PWM channel usage
- GPIO restrictions and notes
- Troubleshooting guide
- Testing procedures
- Schematic reference

#### `BACKPORT_README.md` (UPDATED)
- Summary of backport from ESP32-S3
- Hardware changes and removals
- Pin configuration for ESP32-WROOM
- Software features retained
- Installation instructions
- Memory usage statistics

#### `BACKPORT_GUIDE.md` (UPDATED)
- Detailed backport implementation
- Files copied and modified
- Key code changes made
- Functions removed/commented
- Sensor changes (AHT20 → DHT11)
- Installation completion status

#### `DEVELOPMENT_GUIDE.md`
- Development and contribution guidelines
- Code style and conventions
- Testing procedures
- Pull request process

#### `.copilot`
- GitHub Copilot configuration file
- Provides AI context for development assistance
- Code patterns and architectural guidance
- Development guidelines and best practices
- Common tasks and troubleshooting

#### `platformio.ini` (BACKPORTED)
- PlatformIO build configuration for ESP32-WROOM
- Library dependencies (DHT sensor library)
- Build flags for TFT_eSPI
- Pin definitions via build flags
- Serial monitor settings

#### `platformio-basic.ini`
- Backup/reference copy of backported configuration
- Original backport source file

### Source Code

#### `src/Main-Thermostat.cpp` (BACKPORTED)
- Single comprehensive source file (~3742 lines)
- Backported from Smart Thermostat Alt Firmware v1.1.0 (ESP32-S3)
- Contains complete thermostat implementation
- Hardware features commented out (LEDs, buzzer, light sensor, motion sensor)
- Sensor changed from AHT20 to DHT11
- Added pump relay control
- Organized into logical function groups:
  - **Setup & Configuration**: `setup()`, `loadSettings()`, `saveSettings()`
  - **Main Loop**: `loop()` with non-blocking task management
  - **Display Management**: `updateDisplay()`, `drawButtons()`, touch handling
  - **HVAC Control**: `controlRelays()`, staging logic, fan control, pump control
  - **Communication**: MQTT, WiFi, web server handlers
  - **Scheduling**: 7-day schedule system with day/night periods
  - **User Interface**: Touch keyboard, button handling, tabbed web interface
  - **Utilities**: Temperature conversion, calibration, factory reset

### Header Files

#### `include/WebInterface.h` (BACKPORTED)
- Web server route handlers
- HTTP endpoint definitions
- JSON API responses
- OTA update handling
- Advanced tabbed interface backend

#### `include/WebPages.h` (BACKPORTED)
- HTML templates for web UI
- Modern tabbed interface design
- Status, Settings, Schedule, and System tabs
- JavaScript for dynamic updates
- CSS styling embedded

#### `include/TFT_Setup_ESP32_S3_Thermostat.h`
- Original ESP32-S3 TFT configuration
- Reference only (not actively used)
- Pin definitions overridden by platformio.ini

### PCB Design Files

#### KiCad Design Files
- **`.kicad_sch`**: Complete schematic design
- **`.kicad_pcb`**: Professional 4-layer PCB layout
- **`.kicad_pro`**: Project configuration and design rules
- **`.kicad_prl`**: Local project settings

#### Manufacturing Files (`jlcpcb/`)
- **Gerber Files**: Industry-standard PCB fabrication files
  - Copper layers (top/bottom)
  - Solder mask layers
  - Silkscreen layers
  - Board outline (edge cuts)
- **Drill Files**: Via and hole placement
  - PTH (Plated Through Holes)
  - NPTH (Non-Plated Through Holes)
- **Production Files**: Assembly documentation
  - BOM (Bill of Materials) for component sourcing
  - CPL (Component Placement List) for pick-and-place assembly

### Documentation Images

#### Hardware Photos
- `IMG_20250501_151129758.png`: Development board setup photo

#### Web Interface Screenshots
- `web-status.png`: Real-time status monitoring page
- `web-settings.png`: Configuration interface page
- Additional screenshots showing various system states

## 🔧 Development Workflow

### Getting Started
1. **Clone Repository**: `git clone <repository-url>`
2. **Open in PlatformIO**: Load project in VS Code with PlatformIO extension
3. **Install Dependencies**: PlatformIO automatically installs libraries from `platformio.ini`
4. **Build Project**: PlatformIO Build (Ctrl+Alt+B)
5. **Upload Firmware**: PlatformIO Upload (Ctrl+Alt+U)

### Development Tools
- **Primary IDE**: VS Code with PlatformIO extension
- **PCB Design**: KiCad 6.0 or later
- **Version Control**: Git (GitHub repository)
- **Serial Monitor**: PlatformIO Serial Monitor (115200 baud)
- **MQTT Testing**: MQTT.fx, MQTT Explorer, or similar client

### Code Organization Strategy
- **Single Source File**: Simplifies deployment and compilation
- **Function Grouping**: Related functions organized together
- **Global State**: State variables declared at file scope
- **Extensive Logging**: Serial debug output throughout
- **Modular Functions**: Each major feature in dedicated functions

## 📚 Learning Resources

### Understanding the Codebase
1. **Start with `README.md`**: Get project overview and setup
2. **Review `DOCUMENTATION.md`**: Understand architecture and features
3. **Study `.copilot`**: Learn development patterns and guidelines
4. **Examine `Main-Thermostat.cpp`**: Follow code organization and logic flow

### Key Code Sections to Study
1. **`setup()` Function**: Initialization sequence and hardware setup
2. **`loop()` Function**: Main task scheduling and state management
3. **`controlRelays()`**: Core thermostat control logic
4. **`updateDisplay()`**: User interface and display management
5. **MQTT Functions**: Smart home integration implementation

### PCB Design Learning
1. **Open KiCad Files**: Study schematic and PCB layout
2. **Review Manufacturing Files**: Understand production process
3. **Analyze BOM**: Component selection and sourcing
4. **Study Layout**: Professional PCB design techniques

## 🚀 Extension Points

### Code Extensibility
- **Additional Sensors**: Add support for more sensor types
- **New Display Elements**: Extend UI with additional information
- **Enhanced MQTT**: Add more Home Assistant entities
- **Scheduling**: Implement time-based temperature schedules
- **Data Logging**: Add historical data collection

### Hardware Extensions
- **Outdoor Temperature**: Weather compensation sensor
- **Multiple Zones**: Support for multi-zone HVAC systems
- **Energy Monitoring**: Power consumption tracking
- **Backup Display**: Secondary display for system status

### PCB Modifications
- **Component Updates**: Newer/alternative component footprints
- **Connector Changes**: Different terminal block options
- **Additional I/O**: More relay outputs or sensor inputs
- **Form Factor**: Different board sizes or mounting options

## 🔍 Troubleshooting File Locations

### Debug Information Sources
- **Serial Output**: Connect to ESP32 USB port at 115200 baud
- **Web Interface**: Status page shows real-time system state
- **MQTT Topics**: Monitor published topics for communication issues
- **Preferences Storage**: Settings stored in ESP32 flash memory

### Common File Issues
- **Compilation Errors**: Check `platformio.ini` library versions
- **Upload Failures**: Verify ESP32 connection and drivers
- **Display Problems**: Review TFT_eSPI library configuration
- **PCB Files**: Ensure KiCad version compatibility

This project structure provides a solid foundation for both learning and extending the ESP32 Smart Thermostat system. Each file serves a specific purpose in the overall architecture, making the project maintainable and extensible.