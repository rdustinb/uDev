// Limit to only 1 of the 2 cores to ease the learning of FreeRTOS
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Pins
static const int redLED = 0;
static const int greenLED = 2;
static const int blueLED = 4;

// Task Handles
static TaskHandle_t task_1 = NULL;

// Tasks
void testTask(void *parameter) {
  /*
    Task: allocates an array that is intended to try and overflow the task stack
  */

  int a = 1;
  while(1) {

    int b[100];

    // Do something with array, so it isn't removed...
    for(int i=0; i<100; i++) {

        b[i] = a + i;

    }
    a++;
    Serial.print("b[0]: ");
    Serial.println(b[0]);

    // Allocate new memory
    int *ptr = (int*)pvPortMalloc(1 * 1024 * sizeof(int));
    ptr[0] = a;

    // Print the remaining stack memory
    Serial.print("Test Task High water mark (words): ");
    Serial.println(uxTaskGetStackHighWaterMark(NULL));

    Serial.print("ptr[0]: ");
    Serial.println(ptr[0]);

    // Don't deallocate the memory, to show the stack being consumed slowly...
    vPortFree(ptr);

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }

}

// Create a task to toggle the LED
void ledTask(void *parameter) {
  /*
    Task to blink the built-in LED
  */

  int ledSelect = 0;

  while(1) {

    // Cycle through the LEDs
    if(ledSelect == 0) {
      // RED LED
      digitalWrite(redLED, HIGH);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      digitalWrite(redLED, LOW);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      ledSelect = 1;
    } else if(ledSelect == 1) {
      // GREEN LED
      digitalWrite(greenLED, HIGH);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      digitalWrite(greenLED, LOW);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      ledSelect = 2;
    } else {
      // BLUE LED
      digitalWrite(blueLED, HIGH);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      digitalWrite(blueLED, LOW);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      ledSelect = 0;
    }

    // Print the remaining stack memory
    Serial.print("LED Task High water mark (words): ");
    Serial.println(uxTaskGetStackHighWaterMark(NULL));

    // Print out the free heap memory
    Serial.print("Heap free (bytes): ");
    Serial.println(xPortGetFreeHeapSize());

  }
}

void setup() {

  // Configure the Serial Terminal
  Serial.begin(250000);

  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);

  digitalWrite(redLED, HIGH);
  digitalWrite(greenLED, HIGH);
  digitalWrite(blueLED, HIGH);
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, LOW);
  digitalWrite(blueLED, LOW);
  vTaskDelay(5000 / portTICK_PERIOD_MS);

  // Hello!
  Serial.println();
  Serial.println("---FreeRTOS Chapter 4 Stack Overflow Demo---");

  // Start the test task with a small stack size to try and force a stack overflow
  xTaskCreatePinnedToCore(
    testTask,
    "Test Task",
    2048,
    NULL,
    1,
    &task_1,
    app_cpu
  );

  // Start the LED task
  xTaskCreatePinnedToCore(
    ledTask,
    "LED Task",
    1024,
    NULL,
    2,
    NULL,
    app_cpu
  );

}

void loop() {
  // Empty. Tasks are running independently.
}