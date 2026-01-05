// Limit to only 1 of the 2 cores to ease the learning of FreeRTOS
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Queue Depths
#define MESSAGEQUEUE_DEPTH 32 // Needs to hold the message "Blinked xxx times\0", where xxx could be up to 7 digits
#define DELAYQUEUE_DEPTH 1
#define DEFAULT_LED_DELAY_MS 500

// Pins
static const int redLED = 0;
static const int greenLED = 2;
static const int blueLED = 4;

// Task Handles
static TaskHandle_t task_A = NULL;
static TaskHandle_t task_B = NULL;

// Queue Handles
QueueHandle_t queue_2_messages = NULL;
QueueHandle_t queue_1_delays = NULL;

// Tasks
void taskA(void *parameter) {
  /*
    Task A: monitor the Serial Terminal for input messages, echo's them back to the terminal.
    If the message 'delay xxx' is received, it sends the delay value to Task B via a queue.
    Any message received on queue_2_messages is printed to the Serial Terminal.
  */

  // Print the startup message
  Serial.println();
  Serial.println("---FreeRTOS Chapter 5 Homework---");

  String inputString = "";
  char receivedChar;
  char messageBuffer[MESSAGEQUEUE_DEPTH];
  int messageIndex = 0;
  int previousDelay;
  int newDelay = DEFAULT_LED_DELAY_MS;

  while(1) {

    // Check for Serial input
    if(Serial.available() > 0){

      // Read the incoming character
      char inChar = (char) Serial.read();

      // Check for newline character
      if(inChar == '\n'){

        // Always echo back to the terminal...
        Serial.println(inputString);

        // Process the input string
        if(inputString.startsWith("delay ")){

          // Store the previous delay to use as timeout
          previousDelay = newDelay;

          // Extract the delay value, stripping the 'delay ' prefix
          newDelay = inputString.substring(6).toInt();

          // Send the new delay to Task B via the queue, wait up to previousDelay ms to send
          xQueueSend(queue_1_delays, &newDelay, previousDelay / portTICK_PERIOD_MS);

        }

        // Clear the input string for the next message
        inputString = "";

      } else {

        // Append the character to the input string
        inputString += inChar;

      }

    }

    // Loop-append any characters received from Task B via queue_2_messages
    while(xQueueReceive(queue_2_messages, &receivedChar, 0) == pdTRUE){

      // Store the received character in the message buffer
      messageBuffer[messageIndex++] = receivedChar;

      // If the received character is the null terminator, print the message and reset the index
      // Or if we've reached the buffer limit, print what we have and reset
      if(receivedChar == '\0' || messageIndex >= MESSAGEQUEUE_DEPTH - 1){
        Serial.println(messageBuffer);
        messageIndex = 0;
      }
    }

    // Since this task is triggered by a user input, add a small delay to prevent CPU hogging
    vTaskDelay(30 / portTICK_PERIOD_MS);

  }

}

void taskB(void *parameter) {
  /*
    Task B: Blink the built-in LED at a rate specified by Task A via queue_1_delays.
    Send the word "Blinked" back to Task A via queue_2_messages each time the LED blinks 100 times.
  */

  int ledDelay = DEFAULT_LED_DELAY_MS;
  char messageBuffer[MESSAGEQUEUE_DEPTH];
  int ledBlinkCount = 0;

  while(1) {

    int newDelay;

    // Check for a new delay value from Task A, don't wait if there isn't one
    if(xQueueReceive(queue_1_delays, &newDelay, 0) == pdTRUE){
      ledDelay = newDelay;
    }

    // Blink the built-in LED
    rgbLEDDrive(ledBlinkCount, ledDelay); // Use the RGB LED driver function

    // Increment the blink count
    ledBlinkCount++;

    if(ledBlinkCount%100 == 0){

      // Prepare and send the update message back to Task A
      snprintf(messageBuffer, MESSAGEQUEUE_DEPTH, "Blinked %d times\0", ledBlinkCount);

      // Push the message to the queue, character by character
      for(int i = 0; i < strlen(messageBuffer) + 1; i++){
        xQueueSend(queue_2_messages, &messageBuffer[i], 0);
      }
    }

  }
}

void rgbLEDDrive(int value, int ledDelay) {
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
  vTaskDelay(ledDelay / portTICK_PERIOD_MS);
}

// Setup
void setup() {

  // Configure the Serial Terminal
  Serial.begin(250000);

  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);

  rgbLEDDrive(0, 5000); // All on for 5 seconds

  // Create the queues, queue_2_messages passes N characters, queue_1_delays passes 1 integer
  queue_2_messages = xQueueCreate(MESSAGEQUEUE_DEPTH, sizeof(char));
  queue_1_delays = xQueueCreate(DELAYQUEUE_DEPTH, sizeof(int));

  // Start Task A, Higher Priority as it handles user input
  xTaskCreatePinnedToCore(
    taskA,
    "Task A",
    4096,
    NULL,
    2,
    &task_A,
    app_cpu
  );

  // Start Task B
  xTaskCreatePinnedToCore(
    taskB,
    "Task B",
    4096,
    NULL,
    1,
    &task_B,
    app_cpu
  );

}

// Main Loop
void loop() {
  // Nothing to do here, everything is handled in tasks
}