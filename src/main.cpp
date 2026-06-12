/*/* conlusion del equipo: 
integrantes: 
    - Alexa Huerta Sanchez
    - Arumi Mar Romero
el uso de freeRTOS permite gestiornar varias tareas de manera eficiente siempre y cuando se 
respeten las reglas de diseño. el uso del idle hook permitio verificar el comportamiento del scheduler del kernel. 
al configurar correctamente las tareas para que utilicen vTaskDelay, se logro un comportamiento estable para el sistema. 
el uso de volatile en las variables compartidas entre tareas tambien es importante para evitar problemas de 
optimizacion en el compilado.
*/

//librerias y definicion de harware 
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_system.h"

#define LED_PIN GPIO_NUM_4 // pin para conectar el led externo 
#define BOTON_PIN GPIO_NUM_33 // pin para conectar el boton 
#define SENSOR_ADC_CH ADC_CHANNEL_6 // pin para la lectura analogica 

// variables globales 
volatile bool g_ledRapido = true;
volatile bool g_botonPres = false;
volatile bool g_sensorActivo = false;

// para reanudar la tarea
TaskHandle_t hLedRapido = NULL;

void vTaskLedRapido(void *pvParameters) { //parpadeo rapido, prioridad 1
    int ciclos = 0;
    while (1) {
    
        if (g_botonPres) {
            printf("[LED_R] Boton detectado -> activando modo LENTO\n");
            ciclos = 0; 
            vTaskSuspend(NULL); // para que se suspenda a si misma y deje de parpadear rapido
        }
        
        if (g_ledRapido) {
            gpio_set_level(LED_PIN, 1);
            printf("[LED_R] ON tick:%d\n", (int)xTaskGetTickCount());
            vTaskDelay(pdMS_TO_TICKS(100)); // parpadeo 100ms 
            
            gpio_set_level(LED_PIN, 0);
            printf("[LED_R] OFF tick:%d\n", (int)xTaskGetTickCount());
            vTaskDelay(pdMS_TO_TICKS(100));

            ciclos++;
            if (ciclos >= 25) { 
                ciclos = 0;
                g_ledRapido = false; // pasa al modo lento automaticamente
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(100)); 
        }
    }
}

void vTaskLedLento(void *pvParameters) {  //parpadeo lento , prioridad 2
    int tiempoRestante = 5000; // 5 segundos de timeout 
    
    while (1) {
        if (!g_ledRapido) {
            
            if (g_botonPres) {
                g_sensorActivo = true;
            }
            
            gpio_set_level(LED_PIN, 1);
            printf("[LED_L] ON t restante:%dms\n", tiempoRestante);
            vTaskDelay(pdMS_TO_TICKS(500)); // parpadeo 500ms 
            
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            
            tiempoRestante -= 1000;
            
            // timeout de 5 segundos
            if (tiempoRestante <= 0) {
                printf("[LED_L] Timeout 5s -> regresando a modo RAPIDO\n");
                g_ledRapido = true; // vuelve al rapido
                
                if (g_botonPres) {
                    g_botonPres = false;
                    g_sensorActivo = false;
                    vTaskResume(hLedRapido); // reactiva el parpadeo rapido  
                }
                tiempoRestante = 5000; // reinicia el contador 
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(500)); 
            tiempoRestante = 5000; // inicia en 5s cuando sea su turno
        }
    }
}

void vTaskSensor(void *pvParameters) {
    adc_oneshot_unit_init_cfg_t init_config1 = {};
    init_config1.unit_id = ADC_UNIT_1;
    init_config1.ulp_mode = ADC_ULP_MODE_DISABLE;
    
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {};
    config.atten = ADC_ATTEN_DB_12;          // leer hasta 3.3V
    config.bitwidth = ADC_BITWIDTH_DEFAULT;  // resolucion 12 bits 
    
    adc_oneshot_config_channel(adc1_handle, SENSOR_ADC_CH, &config);
    
    while (1) {
        if (g_sensorActivo) {
            int raw = 0;
            adc_oneshot_read(adc1_handle, SENSOR_ADC_CH, &raw);
            float voltaje = (raw * 3.3) / 4095.0; 
            printf("[SENS] ADC: %.2fV tick:%d\n", voltaje, (int)xTaskGetTickCount());
            vTaskDelay(pdMS_TO_TICKS(300));
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void vTaskMonitor(void *pvParameters) {  //monitor, prioridad 4
int ultimoEstado = 1;
    int contadorSegundos = 0;
    
    while (1) {
        int estadoActual = gpio_get_level(BOTON_PIN);

if (ultimoEstado == 1 && estadoActual == 0) {
            printf("\n[MON] BOTON PRESIONADO \n");
            g_botonPres = true;
            g_ledRapido = false;
        }
        
        ultimoEstado = estadoActual;
        
        contadorSegundos++;
        if (contadorSegundos >= 20) {
            printf("[MON] Heap:%d | Stack R:%d\n", 
                   (int)esp_get_free_heap_size(), 
                   (int)uxTaskGetStackHighWaterMark(hLedRapido));
            contadorSegundos = 0;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
extern "C" void vApplicationIdleHook(void) {  // idle hook, prioridad 0
    static TickType_t lastPrintTime = 0;
    TickType_t currentTime = xTaskGetTickCount();

    if ((currentTime - lastPrintTime) >= pdMS_TO_TICKS(1000)) {
        printf("[IDLE] CPU libre esperando evento de boton\n");
        lastPrintTime = currentTime;
    }
}

extern "C" void app_main() {
    printf("practica 1\n");
    
    // configuracion de pines
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(BOTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOTON_PIN, GPIO_PULLUP_ONLY); 

    // creacion de tareas
    xTaskCreate(vTaskLedRapido, "LedRapido", 2048, NULL, 1, &hLedRapido); 
    xTaskCreate(vTaskLedLento, "LedLento", 2048, NULL, 2, NULL);  
    xTaskCreate(vTaskSensor, "Sensor", 2048, NULL, 3, NULL);  
    xTaskCreate(vTaskMonitor, "Monitor", 3072, NULL, 4, NULL); 
}