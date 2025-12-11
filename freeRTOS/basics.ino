// Basic Arduino + FreeRTOS example
// Two tasks: one blinks LED and sends a counter to a queue,
// the other reads the queue and prints the counter via Serial.
//
// Works on ESP32 (FreeRTOS built-in) and on Arduino boards with a FreeRTOS library.
// Adjust stack sizes/priorities for your platform.

#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

static QueueHandle_t counterQueue = nullptr;
const uint8_t BLINK_PIN = LED_BUILTIN;

void blinkTask(void *pvParameters) {
  pinMode(BLINK_PIN, OUTPUT);
  uint32_t counter = 0;

  for (;;) {
    digitalWrite(BLINK_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(BLINK_PIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(800));

    // send a copy of the counter to the queue (non-blocking)
    counter++;
    xQueueSend(counterQueue, &counter, 0);
  }
}

void printTask(void *pvParameters) {
  uint32_t value;
  Serial.println("printTask started");
  for (;;) {
    // wait indefinitely for an item from the queue
    if (xQueueReceive(counterQueue, &value, portMAX_DELAY) == pdPASS) {
      Serial.print("Received counter: ");
      Serial.println(value);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // give Serial some time (optional)
  vTaskDelay(pdMS_TO_TICKS(100));

  // create a queue that holds up to 10 uint32_t values
  counterQueue = xQueueCreate(10, sizeof(uint32_t));
  if (counterQueue == nullptr) {
    Serial.println("Failed to create queue");
    while (true) { vTaskDelay(pdMS_TO_TICKS(1000)); }
  }

  // create tasks
  // stack sizes and priorities may need adjustment per platform
  xTaskCreate(blinkTask, "Blink", 1024, nullptr, 1, nullptr);
  xTaskCreate(printTask, "Printer", 2048, nullptr, 1, nullptr);

  Serial.println("Tasks created");
}

void loop() {
  // FreeRTOS tasks do the work — keep loop empty or put low-priority background work here
  vTaskDelay(pdMS_TO_TICKS(1000));
}