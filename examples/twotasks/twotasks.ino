#include "FreeRTOS.h"
#include "task.h"
#include "Arduino.h"
#include "MemoryFree.h"

static void vBlinkTask( void *pvParameters );
static void vPrintTask( void *pvParameters );

#ifndef PINS_DEFINED
const int led_red = 13;
#endif

/*--------------------------------------------------------------------------*/

void setup( void )
{
    Serial.begin(57600);
    Serial.println(__FILE__);

    /* Create the tasks defined within this file. */
    xTaskCreate( vBlinkTask, ( signed portCHAR * ) "vBlinkTask", 50, NULL,
            tskIDLE_PRIORITY+1, NULL );
    xTaskCreate( vPrintTask, ( signed portCHAR * ) "vPrintTask", 120, NULL, 
            tskIDLE_PRIORITY+2, NULL );

    Serial.print("FREE=");
    Serial.println(freeMemory());

    vTaskStartScheduler();
}

void loop( void )
{
}

/*--------------------------------------------------------------------------*/

static void vBlinkTask( void * )
{
    pinMode(led_red,OUTPUT);
    for( ;; )
    {
        digitalWrite(led_red,digitalRead(led_red)^1);
        vTaskDelay( 250 );
    }
}

/*--------------------------------------------------------------------------*/

static void vPrintTask( void * )
{
    portTickType xLastWakeTime = xTaskGetTickCount();

    for( ;; )
    {
        Serial.println(xTaskGetTickCount());
        vTaskDelayUntil(&xLastWakeTime,1000);
    }
}

// vim:cin:ai:et:sts=4 sw=4
