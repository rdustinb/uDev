//#define DEBUGLOOP
//#define DEBUGPRINT
//#define BASICLED
//#include "spi_led_pkg.h"

// Receive a Byte Packet
uint8_t packetBytes[100];
uint8_t packetLastByte;

struct {
  uint8_t ledNumber;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} ledRgb;

// Delay
uint8_t serialAvailableLast;
uint8_t readingBytes;
uint8_t readByte;
uint8_t packetGood;

// Different Packet Byte values
const uint8_t WRITE_MULTIPLE_LEDS = 0x4D; // Capital "M"
const uint8_t WRITE_ALL_LEDS = 0x41; // Capital "A"
const uint8_t EOP = 0x2e; // ASCII "."