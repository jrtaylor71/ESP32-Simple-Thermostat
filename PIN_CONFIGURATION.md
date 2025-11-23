# ESP32 Simple Thermostat - Pin Configuration Reference

**Hardware**: ESP32-WROOM DevKit v1  
**Firmware**: v1.1.0 (Backported)  
**Last Updated**: November 21, 2025

---

## Quick Reference Table

| GPIO | Function | Type | Notes |
|------|----------|------|-------|
| 0 | Boot Button | Input | Factory reset (hold 10s) |
| 2 | TFT_DC | Output | Display Data/Command |
| 4 | TOUCH_CS | Output | Touch Chip Select |
| 5 | TFT_RST | Output | Display Reset |
| 12 | Heat Stage 2 | Output | Relay control |
| 13 | Heat Stage 1 | Output | Relay control |
| 14 | Cool Stage 1 | Output | Relay control |
| 15 | TFT_CS | Output | Display Chip Select |
| 18 | TFT_SCLK | Output | SPI Clock (shared) |
| 19 | TFT_MISO | Input | SPI MISO (shared) |
| 22 | DHT11 | Input | Temp/Humidity sensor |
| 23 | TFT_MOSI | Output | SPI MOSI (shared) |
| 25 | Fan Relay | Output | Fan control |
| 26 | Cool Stage 2 | Output | Relay control |
| 27 | DS18B20 | I/O | OneWire sensor |
| 32 | TFT_BL | Output | Backlight PWM |
| 33 | Pump Relay | Output | Hydronic pump |

---

## Detailed Configuration

### Display System (ILI9341 + XPT2046)

```cpp
// TFT Display SPI Interface
#define TFT_MISO 19    // Master In Slave Out
#define TFT_MOSI 23    // Master Out Slave In
#define TFT_SCLK 18    // SPI Clock
#define TFT_CS   15    // Display Chip Select
#define TFT_DC    2    // Data/Command Select
#define TFT_RST   5    // Display Reset
#define TFT_BL   32    // Backlight (PWM capable)

// Touch Controller (shares SPI bus)
#define TOUCH_CS  4    // Touch Chip Select
// TOUCH_MOSI = 23   (shared with TFT)
// TOUCH_MISO = 19   (shared with TFT)
// TOUCH_CLK  = 18   (shared with TFT)
```

**Physical Connection**:
- Uses hardware SPI (VSPI on ESP32)
- Touch shares SPI bus with display
- Separate CS pins for display and touch
- Backlight controlled via PWM on GPIO 32

---

### Temperature Sensors

```cpp
// DHT11 - Ambient Temperature & Humidity
#define DHTPIN 22      // Digital pin
#define DHTTYPE DHT11  // Sensor type
DHT dht(DHTPIN, DHTTYPE);
```
**Physical Connection**:
- 3-pin digital sensor
- Pin 1: VCC (3.3V or 5V)
- Pin 2: Data (GPIO 22)
- Pin 3: GND
- 10kΩ pull-up resistor recommended

```cpp
// DS18B20 - Hydronic Water Temperature
#define ONE_WIRE_BUS 27  // OneWire pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
```
**Physical Connection**:
- 3-pin OneWire sensor
- Pin 1: GND
- Pin 2: Data (GPIO 27)
- Pin 3: VCC (3.3V or 5V)
- 4.7kΩ pull-up resistor required

---

### HVAC Control Relays

```cpp
// Multi-Stage Heating/Cooling Control
const int heatRelay1Pin = 13;  // Heat Stage 1 (W1)
const int heatRelay2Pin = 12;  // Heat Stage 2 (W2)
const int coolRelay1Pin = 14;  // Cool Stage 1 (Y1)
const int coolRelay2Pin = 26;  // Cool Stage 2 (Y2)
const int fanRelayPin = 25;    // Fan Control (G)
const int pumpRelayPin = 33;   // Hydronic Pump
```

**Wiring Guide**:
```
HVAC Terminal Block:
- R  (24VAC Red)     → Common to all relay inputs
- W1 (Heat Stage 1)  → Relay 1 NO (GPIO 13)
- W2 (Heat Stage 2)  → Relay 2 NO (GPIO 12)
- Y1 (Cool Stage 1)  → Relay 3 NO (GPIO 14)
- Y2 (Cool Stage 2)  → Relay 4 NO (GPIO 26)
- G  (Fan)           → Relay 5 NO (GPIO 25)
- C  (24VAC Common)  → Common to all relay outputs

Hydronic Pump:
- Pump Power         → Relay 6 NO (GPIO 33)
```

**Relay Module**:
- Active HIGH output (GPIO HIGH = relay ON)
- Use isolated relay module (optocoupler)
- 24VAC HVAC systems require proper isolation
- NC (Normally Closed) terminals not used

---

### Control Inputs

```cpp
// Built-in Boot Button
#define BOOT_BUTTON 0  // GPIO 0 (built-in)
```

**Usage**:
- Normal: Boot mode selector (programming)
- Runtime: Factory reset (hold 10+ seconds)
- Available on all ESP32 DevKit boards
- Active LOW (pressed = LOW)

---

## SPI Bus Sharing

The SPI bus (VSPI) is shared between display and touchscreen:

```
VSPI Bus Devices:
├── ILI9341 Display (CS: GPIO 15)
│   ├── MISO: GPIO 19
│   ├── MOSI: GPIO 23
│   └── CLK:  GPIO 18
│
└── XPT2046 Touch (CS: GPIO 4)
    ├── MISO: GPIO 19 (shared)
    ├── MOSI: GPIO 23 (shared)
    └── CLK:  GPIO 18 (shared)
```

**Important**: Both devices share data lines but have separate chip select (CS) pins.

---

## PWM Channels Used

```cpp
// Backlight Control
const int PWM_CHANNEL = 0;      // Channel 0
const int PWM_FREQ = 5000;      // 5kHz frequency
const int PWM_RESOLUTION = 8;   // 8-bit (0-255)
const int MIN_BRIGHTNESS = 30;  // Minimum value

// Attached to GPIO 32 (TFT_BL)
ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
ledcAttachPin(TFT_BACKLIGHT_PIN, PWM_CHANNEL);
```

---

## GPIO Restrictions & Notes

### Input-Only Pins (NOT USED)
- GPIO 34-39: Input only, no pull-up/pull-down
- Not suitable for output or I2C

### Strapping Pins (USED WITH CAUTION)
- **GPIO 0**: Boot button (our usage is safe)
- GPIO 2: Used for TFT_DC (OK, no conflicts)
- GPIO 5: Used for TFT_RST (OK)
- GPIO 12: Used for relay (OK, but avoid during boot)
- GPIO 15: Used for TFT_CS (OK)

### Boot Behavior
- GPIO 12 must be LOW during boot for 3.3V flash
- GPIO 15 must be HIGH during boot
- Our configuration is compatible with standard boot

### Reserved/Unavailable
- GPIO 6-11: Connected to flash (DO NOT USE)

---

## Power Requirements

### ESP32 Module
- Operating Voltage: 3.3V (regulated internally)
- Input Voltage: 5V via USB or VIN pin
- Current Draw: ~500mA peak (WiFi transmit)

### Peripherals
- TFT Display: ~100mA (with backlight)
- DHT11: ~2.5mA
- DS18B20: ~1.5mA
- Relay Module: Depends on design (typically isolated)

**Recommendation**: 
- Use 5V 2A power supply
- Ensure stable power for WiFi operation
- Separate power for relay module coils

---

## Wiring Color Code Suggestion

```
Display (8-wire):
- Red:    VCC (3.3V or 5V depending on module)
- Black:  GND
- Yellow: TFT_CS (GPIO 15)
- Green:  TFT_RST (GPIO 5)
- Blue:   TFT_DC (GPIO 2)
- Purple: TFT_MOSI (GPIO 23)
- Gray:   TFT_SCLK (GPIO 18)
- White:  TFT_BL (GPIO 32)
- Orange: TFT_MISO (GPIO 19)
- Brown:  TOUCH_CS (GPIO 4)

DHT11 (3-wire):
- Red:    VCC (3.3V/5V)
- Yellow: Data (GPIO 22)
- Black:  GND

DS18B20 (3-wire):
- Red:    VCC (3.3V/5V)
- Yellow: Data (GPIO 27)
- Black:  GND

Relays (per relay):
- Signal: GPIO (see table above)
- VCC:    5V (module power)
- GND:    Common ground
```

---

## Troubleshooting

### Display Not Working
1. Check SPI wiring (MOSI, MISO, CLK, CS)
2. Verify power to display module
3. Check TFT_CS (GPIO 15) and TFT_DC (GPIO 2)
4. Test backlight separately (GPIO 32)

### Touch Not Responding
1. Verify TOUCH_CS (GPIO 4) connection
2. Check SPI bus shared correctly
3. Ensure display module includes touch controller
4. Calibration may be needed

### DHT11 Reading Errors
1. Check data pin (GPIO 22)
2. Verify 10kΩ pull-up resistor
3. Power cycle sensor
4. Try different sensor if persistent

### DS18B20 Not Found
1. Check OneWire pin (GPIO 27)
2. Verify 4.7kΩ pull-up resistor
3. Confirm sensor address
4. Check for proper parasitic/external power

### Relays Not Switching
1. Verify GPIO assignment matches wiring
2. Check relay module power supply
3. Test GPIOs with LED first
4. Ensure proper isolation

---

## Testing Commands

### Serial Monitor (115200 baud)
```
Boot messages show:
- DHT11 initialization
- DS18B20 detection
- Display initialization
- WiFi connection
- Relay states
```

### Web Interface Test
```
http://[device-ip]/status
Returns: JSON with all sensor and relay states
```

### GPIO Test Code
```cpp
// Test relay output
digitalWrite(heatRelay1Pin, HIGH);  // Turn on
delay(1000);
digitalWrite(heatRelay1Pin, LOW);   // Turn off
```

---

## Schematic Symbol Reference

```
        ESP32-WROOM DevKit v1
        
    3.3V  [ ]  [ ] GND
      EN  [ ]  [ ] GPIO 23 (TFT_MOSI)
  GPIO 36 [ ]  [ ] GPIO 22 (DHT11)
  GPIO 39 [ ]  [ ] TXD0
  GPIO 34 [ ]  [ ] RXD0
  GPIO 35 [ ]  [ ] GPIO 21
  GPIO 32 [ ]  [ ] GND     ⬅️ TFT_BL
  GPIO 33 [ ]  [ ] GPIO 19 (TFT_MISO) ⬅️ Pump
  GPIO 25 [ ]  [ ] GPIO 18 (TFT_SCLK) ⬅️ Fan
  GPIO 26 [ ]  [ ] GPIO 5  (TFT_RST)  ⬅️ Cool 2
  GPIO 27 [ ]  [ ] GPIO 17             ⬅️ DS18B20
  GPIO 14 [ ]  [ ] GPIO 16             ⬅️ Cool 1
  GPIO 12 [ ]  [ ] GPIO 4  (TOUCH_CS) ⬅️ Heat 2
      GND [ ]  [ ] GPIO 0  (Boot)
  GPIO 13 [ ]  [ ] GPIO 2  (TFT_DC)   ⬅️ Heat 1
      SD2 [ ]  [ ] GPIO 15 (TFT_CS)
      SD3 [ ]  [ ] SD1
      CMD [ ]  [ ] SD0
      5V  [ ]  [ ] CLK
```

---

**Document Version**: 1.0  
**Compatible Firmware**: v1.1.0 (Backported)  
**Hardware Platform**: ESP32-WROOM DevKit v1  
**Last Verified**: November 21, 2025
