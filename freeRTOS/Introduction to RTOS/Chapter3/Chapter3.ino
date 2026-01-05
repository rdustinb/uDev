/*
    Chapter3.ino - Example code for Chapter 3
    Learn FreeRTOS for ESP32

    Compiling and Uploading:
        arduino-cli compile -v -b esp32:esp32:adafruit_feather_esp32s2 <folder_path>
        arduino-cli upload -v -p /dev/cu.usbmodem101 -b esp32:esp32:adafruit_feather_esp32s2 <folder_path>
    
    When considering context switching between tasks, there is the concept of register corruption, 
    where the registers of one task may be overwritten by another task during a context switch. 

    TODO: Does FreeRTOS handle context switching register corruption and how?

    Task Preemption is heavily demoed in this chapter.

    TODO: create a sketch that uses two tasks, one to blink the LED and the other to monitor the serial input from the
    user to obtain new blink rates for the LED. The serial monitor task should have a higher priority than the LED
    task so that user input is handled as quickly as possible.
*/

// Limit to only 1 of the 2 cores to ease the learning of FreeRTOS
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Pins
static const int led_pin = LED_BUILTIN;

// Task Handles
static TaskHandle_t task_1 = NULL;
static TaskHandle_t task_2 = NULL;
static TaskHandle_t task_3 = NULL;

// Tasks
void startTask1(void *parameter) {
  /*
    Task 1: print to Serial Terminal with a lower priority
  */
  while(1) {
    for(int i=0; i<33; i++) {
      Serial.print("_");
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    // Block for 1 second to allow the other tasks to run
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void startTask2(void *parameter) {
  /*
    Task 3: print to Serial Terminal with the highiest priority
  */
  while(1) {
    for(int i=0; i<25; i++) {
      Serial.print("-");
      vTaskDelay(33 / portTICK_PERIOD_MS);
    }
    // Block for 1 second to allow the other tasks to run
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void startTask3(void *parameter) {
  /*
    Task 3: print to Serial Terminal with the highiest priority
  */
  while(1) {
    Serial.print('^');
    // Print a single character every 100 ms
    vTaskDelay(45 / portTICK_PERIOD_MS);
  }
}

void setup() {

  // Configure the Serial Terminal
  Serial.begin(300);

  pinMode(led_pin, OUTPUT);

  digitalWrite(led_pin, HIGH);
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  digitalWrite(led_pin, LOW);

  // Delay so that the serial terminal has a chance to initialize
  Serial.println();
  Serial.println("---FreeRTOS Chapter 3 Task Preemption Demo---");

  // Start the other tasks
  xTaskCreatePinnedToCore(
    startTask1,
    "Task 1",
    1024,
    NULL,
    1,
    &task_1,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    startTask2,
    "Task 2",
    1024,
    NULL,
    2,
    &task_2,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    startTask3,
    "Task 3",
    1024,
    NULL,
    3,
    &task_3,
    app_cpu
  );
}

void loop() {
  /*
    The main loop acts as a third task to control the execution of the other two tasks.
  */
  for (int i=0; i<3; i++) {
    vTaskSuspend(task_3);
    vTaskDelay(425 / portTICK_PERIOD_MS);
    vTaskResume(task_3);
    vTaskDelay(667 / portTICK_PERIOD_MS);
  }

  //// Delete the lower priority task
  //if (task_1 != NULL) {
  //  vTaskDelete(task_1);
  //  task_1 = NULL;
  //}
}