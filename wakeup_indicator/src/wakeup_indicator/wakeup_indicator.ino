// include the Time library
#include <Time.h>
// include the LiquidCrystal library
#include <LiquidCrystal.h>
// include the EEPROM library
#include <EEPROM.h>

const int LED1 = 2;
const int LED2 = 3;
const int LED3 = 4;
const long LEDdelay = 100; // miliseconds between changing LED states
long previousMillis = 0;
int currentLED = 1;
int currentLEDstate = LOW;

const long powerMon = A0;

int loopcnt = 0;

// Create an LCD Instance
// RS, Enable, D4, D5, D6, D7
LiquidCrystal lcd(9, 8, 10, 11, 12, 13);

void setup(){
  // Initialize the LED
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  
  // Setup the serial port
  Serial.begin(115200);
  
  // Set dummy time
  setTime(int(18),int(2),int(0),int(27),int(2),int(2015));
    
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD (Column, Row)
  lcd.setCursor(0, 0);
  lcd.print("Wakeup Light");
}

void loop(){
  // Get the current millisecond value
  unsigned long currentMillis = millis();
  // Determine if it is time to change LED state
  if((currentMillis - previousMillis) > LEDdelay){
    // Change LEDs
    if(currentLED == 1){
      digitalWrite(LED1, currentLEDstate);
      currentLED = 2;
    }else if(currentLED == 2){
      digitalWrite(LED2, currentLEDstate);
      currentLED = 3;
    }else if(currentLED == 3){
      digitalWrite(LED3, currentLEDstate);
      currentLED = 1;
      if(currentLEDstate == LOW){
        currentLEDstate = HIGH;
      }else{
        currentLEDstate = LOW;
      }
    }
  }
}

// Time Functions
void displayTime(){
  Serial.println("Current set time is:");
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.println(second());
}
