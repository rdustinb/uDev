#include "uart_led_pkg.h"
#include "spi_led_pkg.h"

void setup() {
  pinMode(spiLatchPin, OUTPUT);
  SPI.begin();
  Serial.begin(115200); // 1Byte every 69.4us

  // Initialize the Packet Structure
  rxBytePacket.command = 0;
  rxBytePacket.ledNumber = 0;
  rxBytePacket.red = 0;
  rxBytePacket.green = 0;
  rxBytePacket.complete_flag = 0;

  // Zeroize the LEDs
  for(int thisLEDPWM=0; thisLEDPWM<LEDCOUNT; thisLEDPWM++){
    LED_PWM[thisLEDPWM] = 0;
  }
}

void loop() {
  // If we start getting bytes, delay to allow the bytes incoming to stabilize
  if(Serial.available() != serial_available_last){
    serial_available_last = Serial.available();
  // Once the incoming bytes have stabilized and completed, process them
  } else if(Serial.available() > 0){
    process_uart_packet();
  // By Default, keep writing to the LED 8bit Shift Registers
  } else {
    process_led_pwm();
  }
}

void process_uart_packet(){
  /*******************************************************************/
  // Capture the Command Byte
  rxBytePacket.command = Serial.read();
  // If changing one LED, get the LED number
  if(rxBytePacket.command == WRITE_LED){
    // Capture the LED Number
    rxBytePacket.ledNumber = (Serial.read() - 0x30) * 10;
    rxBytePacket.ledNumber = rxBytePacket.ledNumber + (Serial.read() - 0x30);
  }
  // Capture the Red Byte
  rxBytePacket.red = (Serial.read() - 0x30) * 10;
  rxBytePacket.red = rxBytePacket.red + (Serial.read() - 0x30);
  // Capture the Green Byte
  rxBytePacket.green = (Serial.read() - 0x30) * 10;
  rxBytePacket.green = rxBytePacket.green + (Serial.read() - 0x30);
  // Capture the Blue Byte
  rxBytePacket.blue = (Serial.read() - 0x30) * 10;
  rxBytePacket.blue = rxBytePacket.blue + (Serial.read() - 0x30);
  // Capture the Complete Flag Byte
  rxBytePacket.complete_flag = Serial.read();
  /*******************************************************************/
  // Consume any erroneous bytes after the packet
  while(Serial.available() > 0){
    Serial.read();
  }
  /*******************************************************************/
  // Check Packet for Validity
  if(
    rxBytePacket.command == WRITE_LED &&
    rxBytePacket.ledNumber < (LEDCOUNT/3) &&
    rxBytePacket.red <= PWMSTEPS &&
    rxBytePacket.green <= PWMSTEPS &&
    rxBytePacket.blue <= PWMSTEPS &&
    rxBytePacket.complete_flag == EOP
  ){
    /*******************************************************************/
    // Update the LED PWM Values
    LED_PWM[rxBytePacket.ledNumber*3 + 0] = rxBytePacket.red; // RED
    LED_PWM[rxBytePacket.ledNumber*3 + 1] = rxBytePacket.green; // GREEN
    LED_PWM[rxBytePacket.ledNumber*3 + 2] = rxBytePacket.blue; // BLUE
    // Return Good
    Serial.println("0");
  } else if(
    rxBytePacket.command == WRITE_ALL_LEDS &&
    rxBytePacket.red <= PWMSTEPS &&
    rxBytePacket.green <= PWMSTEPS &&
    rxBytePacket.blue <= PWMSTEPS &&
    rxBytePacket.complete_flag == EOP
  ){
    /*******************************************************************/
    // Update the LED PWM Values
    for(int thisLEDPWM=0; thisLEDPWM<LEDCOUNT; thisLEDPWM++){
      if(thisLEDPWM%3 == 0){
        LED_PWM[thisLEDPWM] = rxBytePacket.red; // RED
      } else if(thisLEDPWM%3 == 1) {
        LED_PWM[thisLEDPWM] = rxBytePacket.green; // GREEN
      } else if(thisLEDPWM%3 == 2) {
        LED_PWM[thisLEDPWM] = rxBytePacket.blue; // BLUE
      }
    }
    // Return Good
    Serial.println("0");
  } else {
    // Return Bad, drop the packet...
    Serial.println("-1");
  }
  /*******************************************************************/
  // Update the Last Byte Count
  serial_available_last = Serial.available();
}

void process_led_pwm(){
  // Loop through each PWM Step for all LED RGBs...
  for(int pwmLoop=PWMSTEPS-1; pwmLoop>=0; pwmLoop--){
    /*******************************************************************/
    // The SPI Write out expects bytes of data, so we will flip the bits
    // to 1 when the pwmLoop gets to the level stored in the particular
    // LED_PWM array
    for(int thisLED=0; thisLED<LEDCOUNT; thisLED++){
      if(LED_PWM[thisLED] > pwmLoop){
        // OR in the new flag
        LED_SPI_BITS[thisLED/8] = LED_SPI_BITS[thisLED/8] | ((0x1 << thisLED%8) & 0xFF);
      }
    }
    /*******************************************************************/
    // Write to the shift registers
    for(int thisByte=LEDCOUNT/8-1; thisByte>=0; thisByte--){
      // Send the Byte
      SPI.transfer(LED_SPI_BITS[thisByte]);
    }
    // Store what's on the shift registers to the storage registers
    digitalWrite(spiLatchPin, HIGH);
    digitalWrite(spiLatchPin, LOW);
    /*******************************************************************/
    // Reset the bits for the next loop...
    for(int thisLED=0; thisLED<LEDCOUNT; thisLED++){
      LED_SPI_BITS[thisLED/8] = 0;
    }
  }
}