#include <Arduino.h>

// Task Handles
TaskHandle_t Task1;
TaskHandle_t Task2;

// Task 1: Blink LED every 1 second
void TaskBlink1(void *pvParameters) {
  pinMode(2, OUTPUT);  // Onboard LED
  while (1) {
    digitalWrite(2, HIGH);
    vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay
    digitalWrite(2, LOW);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// Task 2: Print message every 2 seconds
void TaskPrint(void *pvParameters) {
  while (1) {
    Serial.println("Task 2 is running!");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  
  // Create two FreeRTOS tasks
  xTaskCreate(TaskBlink1, "Blink Task", 1000, NULL, 1, &Task1);
  xTaskCreate(TaskPrint, "Print Task", 1000, NULL, 1, &Task2);
}

void loop() {
  // Nothing here - tasks handle everything
}