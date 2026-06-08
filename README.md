integrantes:
    Alexa Huerta Sánchez , Arumi Mar Romero

Este proyecto implementa un sistema multitarea de tiempo real utilizando FreeRTOS sobre una ESP32 (desarrollado en PlatformIO). 
El sistema coordina la ejecución de tres tareas principales para controlar estados de parpadeo de un LED externo y realizar el monitoreo del
valor de voltaje de un potenciómetro

  Circuito y funcionamiento: 

El circuito consta de una ESP32 conectado a un LED externo (conectado a una resistencia de 330 ohms), un botón pulsador configurado en modo pull up interno 
y un potenciómetro actuando como entrada analógica

  Arquitectura de software:

Se tienen 3 tareas y además se delega el tiempo muerto a la tarea Idle:

1. TaskFastBlink (prioridad 1): controla un parpadeo rápido del LED externo (100 ms encendido / 100 ms apagado)
   
2. TaskSlowBlink (prioridad 1): controla un parpadeo lento del LED externo (500 ms encendido / 500 ms apagado)
   
3. TaskManager (prioridad 2): opera en ciclos continuos de 15 segundos, divididos en tres fases de 5 segundos cada una:

   Fase 1 (0-5s): reanuda el parpadeo rápido y suspende el lento

    Fase 2 (5-10s): suspende el parpadeo rápido y reanuda el lento

    Fase 3 (10-15s): suspende ambas tareas de parpadeo, asegura el LED en estado apagado;
                     durante estos 5 segundos, realiza lecturas continuas del botón y el potenciómetro, imprimiendo el voltaje calculado (conversión de 12 bits a escala de 0-3.3V) en la terminal cada 500 ms

  Conexiones físicas:

LED externo : GPIO 4,  salida digital, requiere resistencia conectada a GND

Botón pulsador : GPIO 33, entrada digital, configurado con INPUT_PULLUP interno 

Potenciómetro : GPIO 34, entrada analógica, canal ADC1, resolución de 12 bits (0 - 4095)
    
