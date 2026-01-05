// Which CPU core to run the tasks on
static const BaseType_t app_cpu = 0;

// Pins
static const int led_pin = LED_BUILTIN;

// Create a queue to share new LED delay value between tasks
// I probably jumped the gun a bit here, but this is a good use case for queues
QueueHandle_t ledDelayQueue = xQueueCreate(2, sizeof(int));

// Tasks
void taskToggleLED(void *parameter){

  int thisDelay = 500;
  int tmpDelayTest = 0;

  while(1) {
    digitalWrite(led_pin, HIGH);
    vTaskDelay(thisDelay / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    vTaskDelay(thisDelay / portTICK_PERIOD_MS);

    // Check if there's a new delay value in the queue, but don't wait if there isn't
    if(xQueueReceive(ledDelayQueue, &tmpDelayTest, 0) == pdTRUE){
      thisDelay = tmpDelayTest;
    }
  }
}

void taskReadSerial(void *parameter){
  /*
    Task to read Serial Terminal input to adjust LED blink rate
  */

  String inputString = "";

  while(1){

    // Only process if there is at least one character in the buffer
    if(Serial.available() > 0){

      // Cast the byte as a character, store it in a temporary character buffer
      char inChar = (char) Serial.read();

      // Branch if the character is a newline
      if(inChar == '\n'){

        // Ignoring the newline, convert the string to an integer
        int newDelay = inputString.toInt();

        if(newDelay > 0){

          // Send the new delay to the queue
          xQueueSend(ledDelayQueue, &newDelay, 0);

          // Print the update information to the terminal
          Serial.print("LED delay updated to: ");
          Serial.println(newDelay);

        } // if(newDelay > 0)

        // Reset the input string for the next received packet
        inputString = "";

      } else {

        // Append the character buffer to the input string
        inputString += inChar;

      } // if(inChar == '\n')

    } // if(Serial.available() > 0)

    // Delay enough that this task can be preempted.
    vTaskDelay(10 / portTICK_PERIOD_MS);

  }
}

void setup() {

  // Configure the Serial Terminal
  Serial.begin(250000);
  Serial.println("Enter LED blink delay in ms:");

  // Set the LED pin as output
  pinMode(led_pin, OUTPUT);

  // Create and run the tasks
  xTaskCreatePinnedToCore(
    taskToggleLED,
    "Toggle LED",
    1024,
    NULL,
    1,
    NULL,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    taskReadSerial,
    "Read Serial",
    2048,
    NULL,
    2,
    NULL,
    app_cpu 
  );

}

void loop() {
  // Empty. Tasks are running independently.
}