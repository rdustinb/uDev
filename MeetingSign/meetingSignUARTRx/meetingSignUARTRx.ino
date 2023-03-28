//#define DEBUGLOOP
//#define DEBUGPRINT

// Receive a Byte Packet
struct {
  uint8_t command;
  uint8_t data;
  uint8_t complete_flag;
} rxBytePacket;

// Delay
uint8_t serial_available_last;
uint8_t reading_bytes;
uint8_t read_byte;

// Different Packet Byte values
const uint8_t WRITE_LED = 0x4c; // Capital "L"
const uint8_t ON = 0x31; // ASCII "1"
const uint8_t OFF = 0x30; // ASCII "0"
const uint8_t EOP = 0x2e; // ASCII "."

// LED
const int LED = 13;

void setup() {
  Serial.begin(115200); // 1Byte every 69.4us
  pinMode(LED, OUTPUT);
  read_byte = 0;
}

void loop() {
  // If we start getting bytes, delay to allow the bytes incoming to stabilize
  if(Serial.available() != serial_available_last){
    delay(2); // Wait a small amount to allow byte reception to stabilize
    #ifdef DEBUGPRINT
    Serial.print("Current bytes available: ");
    Serial.println(Serial.available());
    #endif
    serial_available_last = Serial.available();
  // Once the incoming bytes have stabilized and completed, process them
  } else if(Serial.available() > 0){
    #ifdef DEBUGLOOP
    // Byte reception is stable
    for(int thisByte=0; thisByte<serial_available_last; thisByte++){
      read_byte = Serial.read();
      Serial.print("I got: ");
      Serial.println(read_byte, DEC);
    }
    #else
    // Capture the Command Byte
    rxBytePacket.command = Serial.read();
    // Capture the Data Byte
    rxBytePacket.data = Serial.read();
    // Capture the Complete Flag Byte
    rxBytePacket.complete_flag = Serial.read();
    // Consume the rest of the read bytes
    while(Serial.available() > 0){
      Serial.read();
    }
    // Print Info
    #ifdef DEBUGPRINT
    Serial.println("Packet Received");
    Serial.println(rxBytePacket.command, HEX);
    Serial.println(rxBytePacket.data, HEX);
    Serial.println(rxBytePacket.complete_flag, HEX);
    #endif
    // Decode The Packet
    if(
      rxBytePacket.command == WRITE_LED &&
      rxBytePacket.complete_flag == EOP
    ){
      // Change the LED
      if(rxBytePacket.data == ON){
        digitalWrite(LED, HIGH); // Turn ON the LED
      } else {
        digitalWrite(LED, LOW); // Turn OFF the LED
      }
      #ifdef DEBUGPRINT
      Serial.println("Packet processed...");
      #else
      Serial.println("0"); // Simply return a 0
      #endif
    } else {
      #ifdef DEBUGPRINT
      Serial.println("Command byte or Complete Flag byte were invalid. Packet dropped...");
      #else
      Serial.println("-1"); // Simply return -1
      #endif
    }
    // Sync the control byte
    serial_available_last = Serial.available();
    #endif
  }
}
