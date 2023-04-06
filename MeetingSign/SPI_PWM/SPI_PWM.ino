// Tagets the 74AHC595, 8bit Shift Registers
// Daisy-chained with their parallel bit outputs
// driving LEDs. The 74AHC595 only uses the clock
// and data pins, and a separate OEn or "Latch"
// signal which pushes the shifted bits out to the
// parallel bus. The OEn pin doesn't depend on th
// clock signal coming in to the 74AHC595.
#include <SPI.h>

//#define DEBUG

// Teensy 3.6, the latch pin can be any GPIO
const int spiLatchPin = 8; 
// Number of LED channels
// Ex: if you have 1 RGB LED, set this to 3
// Ex: if you have 5 red LEDs, set this to 5
// Ex: if you have 8 RGB LEDs, set this to 24
const int LEDCOUNT = 24;

// Defines the number of unique PWM steps, 16 seems like enough
const int PWMSTEPS = 16;

const int SPICLOCKf = 110000000;
SPISettings mySetting(SPICLOCKf, MSBFIRST, SPI_MODE0);

// Stored as LED1 Red, LED1 Green, LED1 Blue, LED2 Red...
// PWM values should be 0 - 255, inclusive
int LED_PWM[LEDCOUNT];
// Pack the PWM Bits shifted out as an array of bytes
byte LED_SPI_BITS[LEDCOUNT/8];

// Setup sets the 74AHC595 Latch Pin mode and starts the SPI interface
void setup() {
  pinMode(spiLatchPin, OUTPUT);
  digitalWrite(spiLatchPin, LOW);
  Serial.begin(115200);
  SPI.begin();
  // Set the LEDs

  for(int thisLEDPWM=0; thisLEDPWM<24; thisLEDPWM++){
    if(thisLEDPWM%3 == 0){
      LED_PWM[thisLEDPWM] = 2; // RED
    } else if(thisLEDPWM%3 == 1) {
      LED_PWM[thisLEDPWM] = 2; // GREEN
    } else if(thisLEDPWM%3 == 2) {
      LED_PWM[thisLEDPWM] = 16; // BLUE
    }
  }
}

void loop() {
  #ifdef DEBUG
  for(int thisLEDPWM=0; thisLEDPWM<24; thisLEDPWM++){
    Serial.println(LED_PWM[thisLEDPWM]);
  }
  #endif
  // Loop through each PWM Step for all LED RGBs...
  for(int pwmLoop=PWMSTEPS-1; pwmLoop>=0; pwmLoop--){
    // The SPI Write out expects bytes of data, so we will flip the bits
    // to 1 when the pwmLoop gets to the level stored in the particular
    // LED_PWM array
    for(int thisLED=0; thisLED<LEDCOUNT; thisLED++){
      if(LED_PWM[thisLED] > pwmLoop){
        // OR in the new flag
        LED_SPI_BITS[thisLED/8] = LED_SPI_BITS[thisLED/8] | ((0x1 << thisLED%8) & 0xFF);
      }
    }
    
    //LED_SPI_BITS[0] = 0x49; // Set RED on everything
    //LED_SPI_BITS[1] = 0x92;
    //LED_SPI_BITS[2] = 0x24;

    //LED_SPI_BITS[0] = 0x92; // Set GREEN on everything
    //LED_SPI_BITS[1] = 0x24;
    //LED_SPI_BITS[2] = 0x49;

    //LED_SPI_BITS[0] = 0x24; // Set BLUE on everything
    //LED_SPI_BITS[1] = 0x49;
    //LED_SPI_BITS[2] = 0x92;

    // Write to the shift registers
    for(int thisByte=LEDCOUNT/8-1; thisByte>=0; thisByte--){
      // Debug
      #ifdef DEBUG
      Serial.print("PWM Loop ");
      Serial.print(pwmLoop);
      Serial.print("; ");
      Serial.print("0x");
      Serial.println(LED_SPI_BITS[thisByte], HEX);
      #endif

      // Send the Byte
      SPI.transfer(LED_SPI_BITS[thisByte]);
    }

    // Store what's on the shift registers to the storage registers
    digitalWrite(spiLatchPin, HIGH);
    digitalWrite(spiLatchPin, LOW);

    // Reset the bits for the next loop...
    for(int thisLED=0; thisLED<LEDCOUNT; thisLED++){
      LED_SPI_BITS[thisLED/8] = 0;
    }
  }
  #ifdef DEBUG
  delay(10000);
  #endif
}
