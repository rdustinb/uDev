//#define DEBUGLOOP
//#define DEBUGPRINT
//#define BASICLED

// Receive a Byte Packet
struct {
  uint8_t command;
  uint8_t ledNumber;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t complete_flag;
} rxBytePacket;

// Delay
uint8_t serial_available_last;
uint8_t reading_bytes;
uint8_t read_byte;
uint8_t packet_good;

// Different Packet Byte values
const uint8_t WRITE_LED = 0x53; // Capital "S"
const uint8_t WRITE_ALL_LEDS = 0x41; // Capital "A"
const uint8_t EOP = 0x2e; // ASCII "."