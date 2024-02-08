// Tagets the 74AHC595, 8bit Shift Registers
// Daisy-chained with their parallel bit outputs
// driving LEDs. The 74AHC595 only uses the clock
// and data pins, and a separate OEn or "Latch"
// signal which pushes the shifted bits out to the
// parallel bus. The OEn pin doesn't depend on th
// clock signal coming in to the 74AHC595.
#include <SPI.h>

// Teensy 3.6, the latch pin can be any GPIO
const int spiLatchPin = 8; 
// Number of LED channels
// Ex: if you have 1 RGB LED, set this to 3
// Ex: if you have 5 red LEDs, set this to 5
// Ex: if you have 8 RGB LEDs, set this to 24
const int LEDCOUNT = 24;

// Defines the number of unique PWM steps, 16 seems like enough
const int PWMSTEPS = 32;

const int SPICLOCKf = 90000000;
SPISettings mySetting(SPICLOCKf, MSBFIRST, SPI_MODE0);

// Stored as LED1 Red, LED1 Green, LED1 Blue, LED2 Red...
// PWM values should be 0 - 255, inclusive
int LED_PWM[LEDCOUNT];
// Pack the PWM Bits shifted out as an array of bytes
byte LED_SPI_BITS[LEDCOUNT/8];