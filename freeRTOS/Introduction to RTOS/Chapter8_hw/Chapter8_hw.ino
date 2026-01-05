// Mimic an LCD Screen backlight timeout control.
// Create a task to echo characters to the terminal as they are typed. If no characters are typed for a timeout period,
// the backlight (an LED) is turned off. Typing characters turns the backlight back on immediately.

// Limit to only 1 of the 2 cores to ease the learning of FreeRTOS
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Define the backlight timeout period (ms)
#define BACKLIGHT_TIMEOUT_MS 5000

// Define the backlight LED pins
static const int redLED = 0;
static const int greenLED = 2;
static const int blueLED = 4;

// Turn on the Backlight
void turnOnBacklight() {
  // Turn on the backlight (all LEDs on)
  digitalWrite(redLED, HIGH);
  digitalWrite(greenLED, HIGH);
  digitalWrite(blueLED, HIGH);
}

// Timer Callback to turn off the backlight
void backlightTimerCallback( TimerHandle_t xTimer ) {
  // Turn off the backlight (all LEDs off)
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, LOW);
  digitalWrite(blueLED, LOW);
}

// Task to monitor Serial input and control backlight
void serialMonitorTask(void *parameter) {
  /*
    Serial Monitor Task: monitors the Serial Terminal for input characters.
    Each character received resets the backlight timeout timer and turns on the backlight.
  */

  // Create the backlight timer
  TimerHandle_t backlightTimer = xTimerCreate(
    "Backlight Timer",
    pdMS_TO_TICKS(BACKLIGHT_TIMEOUT_MS),
    pdFALSE,
    ( void * ) 0,
    backlightTimerCallback
  );

  xTimerStart(backlightTimer, 0);

  while(1) {

    // Check for Serial input
    if(Serial.available() > 0){

      // Read the incoming character
      char inChar = (char) Serial.read();

      // Echo back to the terminal
      Serial.print(inChar);

      // Turn on the backlight (all LEDs on)
      turnOnBacklight();

      // Start or reset the timer
      if(xTimerIsTimerActive(backlightTimer) != pdFALSE) {
        xTimerReset(backlightTimer, 0);
      } else {
        xTimerStart(backlightTimer, 0);
      }

    }

  }

}

// Setup of everything
void setup() {

  // Configure the Serial Terminal
  Serial.begin(250000);

  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);

  // Start the Serial Monitor Task
  xTaskCreatePinnedToCore(
    serialMonitorTask,
    "Serial Monitor Task",
    4096,
    NULL,
    1,
    NULL,
    app_cpu
  );

  // Turn on the backlight after setup completes...
  turnOnBacklight();

}

void loop() {
  // Nothing to do here
}