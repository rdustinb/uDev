// 5 Tasks produce values to a circular buffer, writing it's task number 3 times.
// 2 Tasks consume values from the circular buffer, printing them to the Serial Monitor.
// Suggest using two semaphores, one for free slots, one for used slots.
// A mutex is needed to protect the circular buffer during read/write operations.

// Limit to only 1 of the 2 cores to ease the learning of FreeRTOS
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// The number of tasks to create
#define NUM_PRODUCERS 10
#define NUM_CONSUMERS 2

// The number of times each task prints its message
#define NUM_PRINTS 3

// Define the loop delay min/max for the producer tasks
#define PRODUCERLOOPDELAY_MIN 20
#define PRODUCERLOOPDELAY_MAX 100

// The Circular Buffer size
// Making the buffer a non-integer-multiple of NUM_PRODUCERS, but still larger than NUM_PRODUCERS*NUM_PRINTS, seems to 
// prevent task starvation in testing.
#define CIRCULAR_BUFFER_SIZE (int)(NUM_PRODUCERS * NUM_PRINTS * 1.3)

// Define the Circular Buffer
int buf[CIRCULAR_BUFFER_SIZE];
int head = 0; // Points to the next write position
int tail = 0; // Points to the next read position

// Create a Mutex handle
SemaphoreHandle_t circularBufferSemaphore = NULL;
SemaphoreHandle_t circularBufferMutex = NULL;
SemaphoreHandle_t serialMonitorMutex = NULL;

// Producer Tasks
void producerTask(void *parameter) {
  /*
    Producer Task: produces data and places it into the circular buffer.
  */

  int taskID = *(int*) parameter;

  // Run Forever...
  while(1) {

    /*************************************************/
    // Critical Section
    if(xSemaphoreTake(circularBufferMutex, 0) == pdTRUE) {

      // Don't block the circular buffer if it's full, make sure there is space for NUM_PRINTS entries
      if( (uxSemaphoreGetCount(circularBufferSemaphore) + NUM_PRINTS) <= CIRCULAR_BUFFER_SIZE ) {

        // Write the task number to the buffer
        for(int i=0; i<NUM_PRINTS; i++) {
            buf[head] = taskID; // Write the task ID to the buffer
            head = (head + 1) % CIRCULAR_BUFFER_SIZE; // Move head

            xSemaphoreGive(circularBufferSemaphore); // Indicate to the consumer(s) that there is new data
        }

      }

      // Unlock the circular buffer
      xSemaphoreGive(circularBufferMutex);

      // Prevent task hogging if it wrote data
      randomSeed(analogRead(0));
      vTaskDelay(random(PRODUCERLOOPDELAY_MIN, PRODUCERLOOPDELAY_MAX) / portTICK_PERIOD_MS);
    }
    /*************************************************/

  }
}

// Consumer Task
void consumerTask(void *parameter) {
  /*
    Consumer Task: consumes data from the circular buffer and prints it to the Serial Monitor.
  */

  int taskID = *(int*) parameter;
  int value;
  bool valueUpdated;

  // Run Forever...
  while(1) {

    // Start with the value being stale
    valueUpdated = false;

    /*************************************************/
    // Critical Section
    if(xSemaphoreTake(circularBufferMutex, 0) == pdTRUE) {

      // Don't block the circular buffer if it's empty
      if(uxSemaphoreGetCount(circularBufferSemaphore) > 0) {

        value = buf[tail]; // Read the value from the buffer
        tail = (tail + 1) % CIRCULAR_BUFFER_SIZE; // Move tail
    
        valueUpdated = true; // Self-indicate that we have a new value

        xSemaphoreTake(circularBufferSemaphore, 0); // Indicate to the producer(s) that there is a free slot
    
      }

      xSemaphoreGive(circularBufferMutex); // Unlock the circular buffer
    }
    /*************************************************/

    if(valueUpdated) {
      /*************************************************/
      // Critical Section - Serial Monitor
      // Because this task must print to the Serial Monitor, waiting for the Mutex forever is acceptable
      if(xSemaphoreTake(serialMonitorMutex, portMAX_DELAY) == pdTRUE) {

        Serial.print("Consumer Task ");
        Serial.print(taskID);
        Serial.print(" consumed value: ");
        Serial.println(value);

        xSemaphoreGive(serialMonitorMutex); // Unlock the Serial Monitor

      }
      /*************************************************/
    }

    // Standard Delay to prevent CPU hogging
    vTaskDelay(1 / portTICK_PERIOD_MS);

  }
}

// The Setup of everything
void setup() {

  char producerTaskNameString[20];
  char consumerTaskNameString[20];

  // Setup the Serial Monitor
  Serial.begin(250000);

  // Create the Circular Buffer Mutex
  circularBufferMutex = xSemaphoreCreateMutex();

  // Create the Serial Monitor Mutex
  serialMonitorMutex = xSemaphoreCreateMutex();

  // Create the Counting Semaphore
  circularBufferSemaphore = xSemaphoreCreateCounting(CIRCULAR_BUFFER_SIZE, 0);

  vTaskDelay(5000 / portTICK_PERIOD_MS);

  // Generate the Producers
  for(int i=0; i<NUM_PRODUCERS; i++) {

    sprintf( producerTaskNameString, "Producer Task %d", i );

    xTaskCreatePinnedToCore(
      producerTask,
      producerTaskNameString,
      2048,
      (void*) &i,
      1,
      NULL,
      app_cpu
    );
  }

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // Generate the Consumers
  for(int i=0; i<NUM_CONSUMERS; i++) {

    sprintf( consumerTaskNameString, "Consumer Task %d", i );

    xTaskCreatePinnedToCore(
      consumerTask,
      consumerTaskNameString,
      2048,
      (void*) &i,
      2,
      NULL,
      app_cpu
    );
  }

  // Hack: Delay to allow tasks to start before entering loop
  // I'd like to do this in a better way...
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void loop() {

}