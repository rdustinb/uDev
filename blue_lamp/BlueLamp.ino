/*
 Modified Fade for many, many LEDs
 Each LED fade is independent of each other and randomized after each cycle.
*/

// Core Numbers
int uppLimit = 20;
int lowLimit = 5;
int BIGHTEST = 0;
int DARKEST = 200;
/* To add more LEDs to the mix (for instance the Teensy 3.1 has 12 PWM channels)
 simply change the LEDCNT value to however many LEDs you have total, and add the
 IO number to the LED[] array list.
*/
const int LEDCNT = 10;
int LED[] = {20,21,22,23,9,10,3,4,5,6};
int LEVEL[LEDCNT];
int DELTA[LEDCNT];

// the setup routine runs once when you press reset:
void setup()  { 
  for(int i=0; i<LEDCNT; i++){
    // LED Port Configs
    pinMode(LED[i], OUTPUT);
    // Initialize starting LED Brightness to Fully On
    LEVEL[i] = BIGHTEST;
    // Initialize the deltas
    DELTA[i] = rand() % (uppLimit - lowLimit) + lowLimit;
  }
} 

// the loop routine runs over and over again forever:
void loop()  { 
  for(int i=0; i<LEDCNT; i++){
    // Set PWM
    analogWrite(LED[i], LEVEL[i]);
    // Update next PWM Value
    LEVEL[i] += DELTA[i];
    // Reverse PWM Increment if necessary
    if (LEVEL[i] <= BIGHTEST) {
      DELTA[i] = rand() % (uppLimit - lowLimit) + lowLimit;
    } else if (LEVEL[i] >= DARKEST) {
      DELTA[i] = -DELTA[i];
    }
  }

  // Stall 30ms
  delay(30);                            
}
