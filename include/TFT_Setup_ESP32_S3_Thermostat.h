// Custom TFT_eSPI setup for ESP32-S3 Smart Thermostat
// This setup matches the smart-thermostat hardware v0.04 pin configuration

#define USER_SETUP_ID 200

// Define the display driver
#define ILI9341_DRIVER

// For ESP32-S3 we can use either HSPI or FSPI
// Using HSPI (SPI3) for compatibility
#define USE_HSPI_PORT

// Pin definitions matching smart-thermostat hardware
#define TFT_MISO 19  // GPIO19 - HSPI MISO
#define TFT_MOSI 23  // GPIO23 - HSPI MOSI  
#define TFT_SCLK 18  // GPIO18 - HSPI SCLK
#define TFT_CS   15  // GPIO15 - Chip select control pin
#define TFT_DC    2  // GPIO2  - Data Command control pin
#define TFT_RST   4  // GPIO4  - Reset pin (could use -1 if tied to ESP32 EN pin)
#define TFT_BL   21  // GPIO21 - LED back-light control pin

// Touch screen pins (if needed)
//#define TOUCH_CS  16  // Chip select pin (T_CS) of touch screen - not used initially

// Load fonts
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel high font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel high font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel high font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT

// SPI frequency
#define SPI_FREQUENCY       27000000  // Safe frequency for ESP32-S3
#define SPI_READ_FREQUENCY   6000000  // 6 MHz is safe for reading
#define SPI_TOUCH_FREQUENCY  2500000  // Touch screen frequency