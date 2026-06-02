/*/* conlusion del equipo: 
integrantes: 
    - Alexa Huerta Sanchez
    - Arumi Mar Romero
el uso de freeRTOS permite gestiornar varias tareas de manera eficiente siempre y cuando se 
respeten las reglas de diseño. los cambios mas criticos estuvieron en como algunas funcionres 
requieren acceso exclusivo al hardware, en este caso, la lectura del adc, ya que no podia ejercutarse
dentro del idle. esto se resolvio usando una tarea aislada y una varible global para compartir la lectura del adc. 
luego, el uso adecuado de las prioridades, permite que la tarea principal controle los tiempor del 
led. luego, otro problema fue resolver el acceso al hardware para la lectura del adc 
cumpliendo con la restriccion de tener solo tres tareas. esto se resolvio eliminando los hooks y las variables globales
e integrando el bucle de lectura del sensor directamente dentro de la fase de descanso de la TaskManager
por ultimo, el uso adecuado de las prioridades permite que esta tarea principal 
controle los tiempos de los LEDs y el monitoreo con precisión.

*/
//librerias y definicion de harware 
#include <Arduino.h>
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" //para crear tareas 
#include "esp_freertos_hooks.h" //usar el tiempo de inactividad 
#include <driver/adc.h>

#define LED_PIN 4 // pin para conectar el led externo 
#define BUTTON_PIN 33 // pin para conectar el boton 
#define POT_ADC_CHANNEL 34 // pin para la lectura analogica 

TaskHandle_t taskFastHandle = NULL;  //parpaedo rapido
TaskHandle_t taskSlowHandle = NULL;  //parpadeo lento

//volatile bool isSystemIdleWindow = false;  // flag para avisar cuando debe de imprimir en la terminal 
//TickType_t lastIdlePrintTime = 0; 
//volatile int global_pot_raw = 0; //guargar la lectura del pot 

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
        vTaskResume(taskFastHandle);
        vTaskSuspend(taskSlowHandle);
        vTaskDelay(pdMS_TO_TICKS(5000));

        vTaskSuspend(taskFastHandle);
        vTaskResume(taskSlowHandle);
        vTaskDelay(pdMS_TO_TICKS(5000));

        vTaskSuspend(taskFastHandle);
        vTaskSuspend(taskSlowHandle);
        digitalWrite(LED_PIN, LOW); 
        
        TickType_t startTime = xTaskGetTickCount(); // leer el boton y el pot durante 5 segundos
        while ((xTaskGetTickCount() - startTime) < pdMS_TO_TICKS(5000)) {
            
            int button_state = digitalRead(BUTTON_PIN);
            bool isPressed = (button_state == LOW); 
            
            float voltage = (analogRead(POT_ADC_CHANNEL) / 4095.0) * 3.3;

            Serial.printf("[IDLE] boton presionado: %s | voltaje: %.2f V\n", 
                          isPressed ? "SI" : "NO", voltage);
            
            vTaskDelay(pdMS_TO_TICKS(500)); // esperar medio segundo entre cada impresion 
        }
    }
}
void setup() {
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
       
    //tareas 
    xTaskCreate(TaskFastBlink, "TaskFast", 2048, NULL, 1, &taskFastHandle);
    xTaskCreate(TaskSlowBlink, "TaskSlow", 2048, NULL, 1, &taskSlowHandle);
    xTaskCreate(TaskManager, "TaskManager", 4096, NULL, 2, NULL); //doble memoria para la impresion 
}

void loop() {
    vTaskDelete(NULL);
}