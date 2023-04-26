#include "uart_led_pkg.h"
#include "spi_led_pkg.h"

void setup() {
  pinMode(spiLatchPin, OUTPUT);
  SPI.begin();
  Serial.begin(115200); // 1Byte every 69.4us

  // Initialize the Packet Structure
  ledRgb.ledNumber = 0;
  ledRgb.red = 0;
  ledRgb.green = 0;
  ledRgb.blue = 0;

  // Zeroize the Packet Bytes
  for(int thisByte=0; thisByte<100; thisByte++){
    packetBytes[thisByte] = 0;
  }
  packetLastByte = 0;

  // Zeroize the LEDs
  for(int thisLEDPWM=0; thisLEDPWM<LEDCOUNT; thisLEDPWM++){
    LED_PWM[thisLEDPWM] = 0;
  }
}

void loop() {
  // If we start getting bytes, delay to allow the bytes incoming to stabilize
  if(Serial.available() != serialAvailableLast){
    serialAvailableLast = Serial.available();
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
  // Read in all bytes
  for(uint8_t thisByte=0; thisByte<100; thisByte++){
    packetBytes[thisByte] = Serial.read();
    if(packetBytes[thisByte] == EOP){
      packetLastByte = thisByte; // Store the last byte index
      break; // If the end of the packet has been received, stop processing...
    }
  }
  
  /*******************************************************************/
  // Branch based on Packet Type
  if(packetBytes[0] == WRITE_ALL_LEDS){
    if(packetLastByte == 7){
      // Convert the ASCII Byte string values to integer values
      calculate_led_rgb();
      // Update the LED PWM Values
      for(int thisLEDPWM=0; thisLEDPWM<LEDCOUNT; thisLEDPWM++){
        if(thisLEDPWM%3 == 0){
          LED_PWM[thisLEDPWM] = ledRgb.red; // RED
        } else if(thisLEDPWM%3 == 1) {
          LED_PWM[thisLEDPWM] = ledRgb.green; // GREEN
        } else if(thisLEDPWM%3 == 2) {
          LED_PWM[thisLEDPWM] = ledRgb.blue; // BLUE
        }
      }
      // Return Good
      Serial.println("0");
    } else {
      // Return Bad
      Serial.println("-1");
    }
  } else if(packetBytes[0] == WRITE_MULTIPLE_LEDS){
    if((packetLastByte%8) == 1){
      // Throw out the command byte
      for(int thisByte=0; thisByte<packetLastByte; thisByte++){
        packetBytes[thisByte] = packetBytes[thisByte+1];
      }
      packetLastByte -= 1;

      // Parse all the LED Tuples
      while(packetBytes[0] != EOP){
        // Stuff the RGB Values into the Struct
        calculate_led_rgb_with_led();

        // Stuff the RGB Values into the LED PWM Array
        LED_PWM[(ledRgb.ledNumber*3) + 0] = ledRgb.red;
        LED_PWM[(ledRgb.ledNumber*3) + 1] = ledRgb.green;
        LED_PWM[(ledRgb.ledNumber*3) + 2] = ledRgb.blue;

        // Shift off the consumed LED Tuple
        for(int thisByte=0; thisByte<=(packetLastByte-8); thisByte++){
          packetBytes[thisByte] = packetBytes[thisByte+8];
        }
        packetLastByte -= 8;
      }
      
      // Return Good
      Serial.println("0");
    } else {
      // Return Bad
      Serial.println("-1");
    }
  }

  /*******************************************************************/
  // Update the Last Byte Count
  serialAvailableLast = Serial.available();
}

void calculate_led_rgb(){
  ledRgb.red   = ((packetBytes[1]-0x30) * 10) + (packetBytes[2]-0x30);
  ledRgb.green = ((packetBytes[3]-0x30) * 10) + (packetBytes[4]-0x30);
  ledRgb.blue  = ((packetBytes[5]-0x30) * 10) + (packetBytes[6]-0x30);
}

void calculate_led_rgb_with_led(){
  ledRgb.ledNumber = ((packetBytes[0]-0x30) * 10) + (packetBytes[1]-0x30);
  ledRgb.red   = ((packetBytes[2]-0x30) * 10) + (packetBytes[3]-0x30);
  ledRgb.green = ((packetBytes[4]-0x30) * 10) + (packetBytes[5]-0x30);
  ledRgb.blue  = ((packetBytes[6]-0x30) * 10) + (packetBytes[7]-0x30);
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