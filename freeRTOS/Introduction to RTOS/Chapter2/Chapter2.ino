/*
Notes Learned in this Chapter:

It is not recommended to use delay() in FreeRTOS applications.  Instead, use vTaskDelay() to yield the CPU to other
tasks while waiting. This is due to the fact that delay() is a blocking call that prevents the CPU from doing anything 
else while waiting.

By default, FreeRTOS sets the tick period to 1ms

By default, FreeRTOS sets the portTICK_PERIOD_MS to 1

vTaskDelay expects the number of ticks to delay

vTaskDelay instructs FreeRTOS to use one of the internal timers on the CPU to delay by n ticks and allow the

CPU core to do something else while waiting for the timer to expire and fire an ISR.

Vanilla FreeRTOS: schedule the task with xTaskCreate()
ESP-IDF FreeRTOS: schedule the task with xTaskCreatePinnedToCore()

Arguments for the scheduler calls:
  Handle:   Function to be called
  String:   Name of task
  Integer:  Stack size for the function (bytes in ESP32, word in FreeRTOS)
  Pointer:  Memory pointer for argument passing to the function, if desired
  Integer:  Task priority (0 to configMAX_PRIORITIES - 1)
  Handle:   Task handle
  Integer:  Select the core to run this task on

The ESP version of the create task call allows us to only use a single core of the CPU, whereas the Vanilla
FreeRTOS call will let the sceduler pick and choose which core to run the task on. The Vanilla FreeRTOS task call
can be used on the ESP32, just understand that it will only work in single-core mode.

Minimum stack size for an empty task in ESP-IDF is 768 bytes

With Vanilla FreeRTOS, vTaskStartScheduler() needs to be called in main after setting up the tasks.
*/

// Use the homework define to enable the homework task, randomizing the LED blink rate
//#define HOMEWORK
#define HOMEWORK_MIN_DELAY 100
#define HOMEWORK_MAX_DELAY 300

// Limit to only 1 of the 2 cores to ease the learning of FreeRTOS
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Pins
//static const int led_pin = LED_BUILTIN;
static const int led_pin = 2;

// Tasks
void toggleLED_1s(void *parameter) {
  while(1) {
    digitalWrite(led_pin, HIGH);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// Homework:
void toggleLED_rand(void *parameter) {
  int thisRandDelay;
  while(1) {
    thisRandDelay = random(HOMEWORK_MIN_DELAY, HOMEWORK_MAX_DELAY);
    digitalWrite(led_pin, HIGH);
    vTaskDelay(thisRandDelay / portTICK_PERIOD_MS);
    thisRandDelay = random(HOMEWORK_MIN_DELAY, HOMEWORK_MAX_DELAY);
    digitalWrite(led_pin, LOW);
    vTaskDelay(thisRandDelay / portTICK_PERIOD_MS);
  }
}

void setup() {
  pinMode(led_pin, OUTPUT);

  xTaskCreatePinnedToCore(
    #ifndef HOMEWORK
    toggleLED_1s,           // Function to be called
    "Toggle LED 1s",        // Name of task
    #else
    // Homework:
    toggleLED_rand,         // Function to be called
    "Toggle LED Random",    // Name of task
    #endif
    1024,                   // Stack size for the function (bytes in ESP32, word in FreeRTOS)
    NULL,                   // Memory pointer for argument passing to the function, if desired
    2,                      // Task priority (0 to configMAX_PRIORITIES - 1)
    NULL,                   // Task handle
    app_cpu                 // Select the core to run this task on
  );

}

void loop() {
  // put your main code here, to run repeatedly:

}
