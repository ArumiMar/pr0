#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"

#define LED_PIN 2
#define BUTTON_PIN 33
#define POT_ADC_CHANNEL 32

TaskHandle_t taskFastHandle = NULL;
TaskHandle_t taskSlowHandle = NULL;

volatile bool isSystemIdleWindow = false;
TickType_t lastIdlePrintTime = 0;

bool my_idle_hook(void) {
    if (isSystemIdleWindow) {
        TickType_t currentTime = xTaskGetTickCount();
        if ((currentTime - lastIdlePrintTime) * portTICK_PERIOD_MS >= 500) {

            int button_state = digitalRead(BUTTON_PIN);
            bool isPressed = (button_state == LOW); 
            
            int pot_raw = analogRead(POT_ADC_CHANNEL);
            float voltage = (pot_raw / 4095.0) * 3.3;

            char str_voltage[10]; 
            dtostrf(voltage, 4, 2, str_voltage);

            

            ets_printf("[IDLE] boton presionado: %s | voltaje: %.2f mV\n", 
                   isPressed ? "SI" : "NO", str_voltage);
            
            lastIdlePrintTime = currentTime;
        }
    }
    return true; 
}

void TaskFastBlink(void *pvParameters) {
    while (true) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(100));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void TaskSlowBlink(void *pvParameters) {
    while (true) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(500));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void TaskManager(void *pvParameters) {
    while (true) {

        isSystemIdleWindow = false;
        vTaskResume(taskFastHandle);
        vTaskSuspend(taskSlowHandle);
        vTaskDelay(pdMS_TO_TICKS(5000));

        vTaskSuspend(taskFastHandle);
        vTaskResume(taskSlowHandle);
        vTaskDelay(pdMS_TO_TICKS(5000));

        vTaskSuspend(taskFastHandle);
        vTaskSuspend(taskSlowHandle);
        digitalWrite(LED_PIN, LOW); 
        
        isSystemIdleWindow = true;
        vTaskDelay(pdMS_TO_TICKS(5000)); 
    }
}

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);

    pinMode(POT_ADC_CHANNEL, INPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    analogSetAttenuation(ADC_11db);
    esp_register_freertos_idle_hook(my_idle_hook);
    analogSetWidth(12);

    xTaskCreate(TaskFastBlink, "TaskFast", 2048, NULL, 1, &taskFastHandle);
    xTaskCreate(TaskSlowBlink, "TaskSlow", 2048, NULL, 1, &taskSlowHandle);

    xTaskCreate(TaskManager, "TaskManager", 2048, NULL, 2, NULL);
}

void loop() {
    vTaskDelete(NULL);
}