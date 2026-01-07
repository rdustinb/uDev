// Following along with the class, this will make a simple blink sketch using the HW interrupts
static const uint32_t timer_frequency = 1000000; // 1MHz Timer
static const uint64_t timer_interval = 1000000; // Count to 1,000,000 (1s)

// Define the LED pins
static const int redLED = 0;
static const int greenLED = 2;
static const int blueLED = 4;

// Define the HW Timer
static hw_timer_t *timer = NULL;

/******************************************************************************/
// Interrupt Service Routines

// Define the HW Timer ISR
// IRAM_ATTR - stores the task in instruction RAM
// Different memory type attributes are documented here:
// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/memory-types.html
void IRAM_ATTR onTimer() {

  // Get the current LED state
  int pin_state = digitalRead(redLED);

  // Invert the LED state
  digitalWrite(redLED, !pin_state);

}

/******************************************************************************/
// Main Execution Point
void setup() {

  // Configure the Serial Terminal
  Serial.begin(250000);

  // Wait a moment to start, allowing the Serial Terminal to begin...
  vTaskDelay(task_delay / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("-------- Chapter 9 Basic --------");

  // Configure the LED pins
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);

  // Some HW timers are setup with a prescalar, like this:
  // timer = timerBegin(0, timer_divider, true);
  // Others are setup with just the frequency, like this:
  timer = timerBegin(timer_frequency);

  // Provide an ISR function to the timer (timer, function)
  timerAttachInterrupt(timer, &onTimer);

  // void timerAlarm(hw_timer_t *timer, uint64_t alarm_value, bool autoreload, uint64_t reload_count);
  timerAlarm(timer, timer_interval, true, 0);

}

void loop() {
  // Nothing to do here, everything is handled in the ISR
}
