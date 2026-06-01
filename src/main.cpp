/*el uso de freeRTOS permite gestiornar varias tareas de manera eficiente siempre y cuando se 
respeten las reglas de diseño. los cambios mas criticos estuvieron en como algunas funcionres 
requieren acceso exclusivo al hardware, en este caso, la lectura del adc, ya que no podia ejercutarse
dentro del idle. esto se resolvio usando una tarea aislada y una varible global para compartir la lectura del adc. 
luego, el uso adecuado de las prioridades, permite que la tarea principal controle los tiempor del 
led. 
*/
//librerias y definicion de harware 
#include <Arduino.h>
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" //para crear tareas 
#include "esp_freertos_hooks.h" //usar el tiempo de inactividad 

#define LED_PIN 4 // pin para conectar el led externo 
#define BUTTON_PIN 33 // pin para conectar el boton 
#define POT_ADC_CHANNEL 34 // pin para la lectura analogica 

TaskHandle_t taskFastHandle = NULL;  //parpaedo rapido
TaskHandle_t taskSlowHandle = NULL;  //parpadeo lento

volatile bool isSystemIdleWindow = false;  // flag para avisar cuando debe de imprimir en la terminal 
TickType_t lastIdlePrintTime = 0; 

volatile int global_pot_raw = 0; //guargar la lectura del pot 

void TaskReadADC(void *pvParameters) {
    while (true) {
        global_pot_raw = analogRead(POT_ADC_CHANNEL);
        vTaskDelay(pdMS_TO_TICKS(50));  //esperar 50 ms antes de la siguiente lectura
    }
}

bool my_idle_hook(void) {  //esta funcion se ejecuta cada vez que el sistema esta inactivo: cuando no hay tareas listas para ejecutarse
    if (isSystemIdleWindow) {
        TickType_t currentTime = xTaskGetTickCount(); //revisa la actual, la resta con la ultima vez que imprimio y si pasaron más de 500 milisegundos, entra a ejecutar
        if ((currentTime - lastIdlePrintTime) * portTICK_PERIOD_MS >= 500) { 

            int button_state = digitalRead(BUTTON_PIN);
            bool isPressed = (button_state == LOW); 
            
            int pot_raw = global_pot_raw;
            int voltage_mv = (pot_raw * 3300) / 4095;

            ets_printf("[IDLE] boton presionado: %s | voltaje: %d mV\n", 
                   isPressed ? "SI" : "NO", voltage_mv);
            
            lastIdlePrintTime = currentTime;
        }
    }
    return true; 
}

void TaskFastBlink(void *pvParameters) { //parpadeo rapido 
    while (true) { 
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(100));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void TaskSlowBlink(void *pvParameters) { //parpadeo lento 
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

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    esp_register_freertos_idle_hook(my_idle_hook);
       
    //tareas 
    xTaskCreate(TaskReadADC, "TaskADC", 2048, NULL, 1, NULL);
    xTaskCreate(TaskFastBlink, "TaskFast", 2048, NULL, 1, &taskFastHandle);
    xTaskCreate(TaskSlowBlink, "TaskSlow", 2048, NULL, 1, &taskSlowHandle);
    xTaskCreate(TaskManager, "TaskManager", 2048, NULL, 2, NULL);
}

void loop() {
    vTaskDelete(NULL);
}