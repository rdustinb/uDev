// Limit to only 1 of the 2 cores to ease the learning of FreeRTOS
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Following along with the class, this will make a simple blink sketch using the HW interrupts
static const uint32_t timer_frequency = 10000000; // 10MHz Timer
static const uint64_t timer_interval = 1000000; // Count to 1,000,000 (100ms)
static const TickType_t startup_delay = 5000;
static const TickType_t task_delay = 2000;

// Define the LED pins
static const int redLED = 0;
static const int greenLED = 2;
static const int blueLED = 4;

// Define the HW Timer
static hw_timer_t *timer = NULL;
static volatile int isr_counter; // 'volatile' indicates that this variable can be modified outside of the currently running task
static int isr_counter_stored;
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED; // Causes a core accessing this Mutex to loop forever until it is available

/******************************************************************************/
// Interrupt Service Routines

// Define the HW Timer ISR
// IRAM_ATTR - stores the task in instruction RAM
// Different memory type attributes are documented here:
// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/memory-types.html
void IRAM_ATTR onTimer() {

  // Espressif ISR Lock / Critical Section
  // Disables other interrupts for this core...
  // Rules:
  //  Do not call other API functions within a critical section
  portENTER_CRITICAL_ISR(&spinlock);
  isr_counter++;
  portEXIT_CRITICAL_ISR(&spinlock);

}

/******************************************************************************/
// Tasks
void rgbLEDDrive(int value, int ledDelay, bool symmetricDelay = true) {
  /*
    Drive the RGB LED in a gradient based on the input value
  */

  if(value%7 == 0) {
    // All On - white
    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, HIGH);
    digitalWrite(blueLED, HIGH);
    vTaskDelay(ledDelay / portTICK_PERIOD_MS);
  } else if(value%7 == 1) {
    // Red
    digitalWrite(redLED, HIGH);
    vTaskDelay(ledDelay / portTICK_PERIOD_MS);
  } else if(value%7 == 2) {
    // Red + Green = Yellow
    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, HIGH);
    vTaskDelay(ledDelay / portTICK_PERIOD_MS);
  } else if(value%7 == 3) {
    // Green
    digitalWrite(greenLED, HIGH);
    vTaskDelay(ledDelay / portTICK_PERIOD_MS);
  } else if(value%7 == 4) {
    // Green + Blue = Cyan
    digitalWrite(greenLED, HIGH);
    digitalWrite(blueLED, HIGH);
    vTaskDelay(ledDelay / portTICK_PERIOD_MS);
  } else if(value%7 == 5) {
    // Blue
    digitalWrite(blueLED, HIGH);
    vTaskDelay(ledDelay / portTICK_PERIOD_MS);
  } else if(value%7 == 6) {
    // Blue + Red = Magenta
    digitalWrite(blueLED, HIGH);
    digitalWrite(redLED, HIGH);
    vTaskDelay(ledDelay / portTICK_PERIOD_MS);
  }

  // Turn all LEDs off
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, LOW);
  digitalWrite(blueLED, LOW);
  if(symmetricDelay) {
    vTaskDelay(ledDelay / portTICK_PERIOD_MS);
  }
}

void startupSequence() {

  rgbLEDDrive(0, startup_delay, false);

  Serial.println();
  Serial.println("-------- Chapter 9 Advanced --------");

  // Toggle delay is only 25ms, giving a fun rainbow effect for 2.5 seconds...
  for(int thisFlash=0; thisFlash<100; thisFlash++) {
    rgbLEDDrive(thisFlash, 25);
  }
}

void printValue(void *parameter) {

  // Tasks must loop forever and never return...
  while(1) {

    // Only decrement if the value is greater than 0
    while(isr_counter > 0) {
      Serial.println(isr_counter);
      if(isr_counter != isr_counter_stored) {
        Serial.println(" -- Variable incremented outside task!");
      }

      portENTER_CRITICAL(&spinlock);
      isr_counter--;
      isr_counter_stored = isr_counter;
      portEXIT_CRITICAL(&spinlock);

      // Add a little delay to show how the ISR will increment the variable while we
      // are processing this loop...
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    rgbLEDDrive(1, 25);

    // Wait a while for the ISR to increment the variable a few times
    vTaskDelay(task_delay / portTICK_PERIOD_MS);

  }

}

/******************************************************************************/
// Main Execution Point
void setup() {

  // Configure the Serial Terminal
  Serial.begin(250000);

  // Configure the LED pins
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);

  // Run the Startup Sequence
  startupSequence();

  // Start the printing task...
  xTaskCreatePinnedToCore(
    printValue,
    "Print Value task",
    1024,
    NULL,
    1,
    NULL,
    app_cpu
  );

  // Start the ISR timer...
  timer = timerBegin(timer_frequency);
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, timer_interval, true, 0);

  // Delete the setup and loop tasks
  vTaskDelete(NULL);
}

void loop() {
  // Nothing to do here, everything is handled in the ISR
}
