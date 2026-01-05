// Limit to only 1 of the 2 cores to ease the learning of FreeRTOS
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Defaults
#define DEFAULT_LED_DELAY_MS 500

// Pins
static const int redLED = 0;

// Task Handles
TaskHandle_t taskA_handle = NULL;

// Create a Mutex handle
SemaphoreHandle_t outOfScopeMutex = NULL;

void vTaskWaitForToggle(){

  // Wait for the Semaphore to be taken
  while(uxSemaphoreGetCount(outOfScopeMutex) == 1){
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }

  // Wait for the Semaphore to be given back
  while(uxSemaphoreGetCount(outOfScopeMutex) == 0){
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void vTaskTake(){
  // Take the semaphore to indicate we are in the task scope
  if(xSemaphoreTake(outOfScopeMutex, 0) == pdTRUE){
    Serial.println("Semaphore successfully taken.");
  } else {
    Serial.println("Failed to take semaphore!");
  }
}

void vTaskGive(){
  // Release the semaphore now that the parameter has been read
  if(xSemaphoreGive(outOfScopeMutex) == pdTRUE){
    Serial.println("Semaphore successfully released.");
  } else {
    Serial.println("Failed to release semaphore!");
  }
}

// Tasks
void taskA(void *parameter) {
  /*
    Task A: Blink the built-in LED at a user-defined interval.
    The initial delay value is passed via the parameter pointer.
    The delay value can be updated via Serial input.
  */

  vTaskTake(); // Take the semaphore to indicate we are in the task scope

  // Get the initial delay value from the parameter
  int ledDelay = *(int *) parameter;

  // Delay just a bit to allow any monitoring to catch up
  vTaskDelay(5 / portTICK_PERIOD_MS);

  vTaskGive(); // Release the semaphore now that the parameter has been read

  Serial.print("LED delay argument is: ");
  Serial.println(ledDelay);

  if(ledDelay <= 0){
    Serial.println("Invalid initial delay value, using default of 500 ms");
    ledDelay = DEFAULT_LED_DELAY_MS; // Default to 500 ms if invalid
  }
  Serial.print("Starting LED blink delay: ");
  Serial.print(ledDelay);
  Serial.println(" ms");

  while(1) {

    // Blink the built-in LED
    digitalWrite(redLED, HIGH);
    vTaskDelay(ledDelay / portTICK_PERIOD_MS);
    digitalWrite(redLED, LOW);
    vTaskDelay(ledDelay / portTICK_PERIOD_MS);

  }
}

// Setup
void setup() {

  int startValue;
  outOfScopeMutex = xSemaphoreCreateMutex();

  // Configure the Serial Terminal
  Serial.begin(250000);

  pinMode(redLED, OUTPUT);

  digitalWrite(redLED, HIGH);
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  digitalWrite(redLED, LOW);
  vTaskDelay(5000 / portTICK_PERIOD_MS);

  // Hello!
  Serial.println();
  Serial.println("---FreeRTOS Chapter 6 Homework---");

  // Get an initial value from the user
  Serial.println("Enter initial LED blink delay in ms:");
  while(Serial.available() == 0){
    vTaskDelay(30 / portTICK_PERIOD_MS);
  }
  String inputString = Serial.readStringUntil('\n');
  startValue = inputString.toInt();
  if(startValue <= 0){
    startValue = DEFAULT_LED_DELAY_MS;
  }
  Serial.print("Received the value: ");
  Serial.print(startValue);
  Serial.println(" from the terminal.");

  // Start the LED task
  xTaskCreatePinnedToCore(
    taskA,
    "Task A",
    2048,
    (void*)&startValue,
    1,
    NULL,
    app_cpu
  );

  Serial.println("Waiting for the LED task to initialize...");
  // Wait for the Task to take, then return, the semaphore
  vTaskWaitForToggle();
  Serial.println("LED Task initialized successfully. DONE with Setup.");
}

// Main Loop
void loop() {
  // Empty. Tasks are running independently.
}