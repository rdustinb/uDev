// Limit to only 1 of the 2 cores to ease the learning of FreeRTOS
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// This file-specific
static const char thisChapterDescription[20] = "Deferred Interrupts";

// Settings
static const uint32_t timer_frequency = 10000000; // 10MHz Timer
static const uint64_t timer_interval = 10000000; // Count to 10,000,000 (1s)
static const TickType_t startup_delay = 5000;

// Pins
static const int redLED = 0;
static const int greenLED = 2;
static const int blueLED = 4;
static const int adc_pin = A0;

// Globals
static hw_timer_t *timer = NULL;
static volatile uint16_t val;
static SemaphoreHandle_t bin_sem = NULL;

/******************************************************************************/
// Interrupt Service Routines
void IRAM_ATTR onTimer() {

  BaseType_t task_woken = pdFALSE;

  // Perform ISR action
  val = analogRead(adc_pin);

  // Give the semaphore and tell the OS that this will wake up a task
  xSemaphoreGiveFromISR(bin_sem, &task_woken);

  // Vanilla FreeRTOS yield to the task
  //portYIELD_FROM_ISR(task_woken);

  // Espressif yield to the task
  if(task_woken) {
    portYIELD_FROM_ISR();
  }
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
  /*
    Perform the standard startup procedure
  */

  // Configure the Serial Terminal
  Serial.begin(250000);

  // Configure the LED pins
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);

  rgbLEDDrive(0, startup_delay, false);

  Serial.println();
  Serial.print("-------- Chapter 9 ");
  Serial.print(thisChapterDescription);
  Serial.println(" --------");

  // Toggle delay is only 25ms, giving a fun rainbow effect for 2.5 seconds...
  for(int thisFlash=0; thisFlash<100; thisFlash++) {
    rgbLEDDrive(thisFlash, 25);
  }
}

void printValue(void *parameters) {
  /*
    Waits for value to be available, then prints it
  */
  while(1) {
    xSemaphoreTake(bin_sem, portMAX_DELAY);
    Serial.println(val);
  }
}

/******************************************************************************/
// Main Execution Point
void setup() {
  // Run the startup procedure
  startupSequence();

  // Create the binary semaphore
  bin_sem = xSemaphoreCreateBinary();

  if(bin_sem == NULL) {
    Serial.println("Semaphore failed to be created, rebooting...");
    ESP.restart();
  }

  // Start the printing task
  xTaskCreatePinnedToCore(
    printValue,
    "Print Value task",
    1024,
    NULL,
    2,
    NULL,
    app_cpu
  );

  // Start the ISR timer...
  timer = timerBegin(timer_frequency);
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, timer_interval, true, 0);
}

void loop() {
}
