#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "Arduino.h"
#include "MemoryFree.h"

static void vLedTask( void *pvParameters );
static void vButtonTask( void *pvParameters );
static xQueueHandle xButtonQueue;

#ifndef PINS_DEFINED
const int led_red = 13;
const int button_a = 2;
#endif

/*--------------------------------------------------------------------------*/

void setup( void )
{
    Serial.begin(57600);
    Serial.println(__FILE__);

    /* Create the tasks defined within this file. */
    xTaskCreate( vLedTask, ( signed portCHAR * ) "vBlinkTask", 100, NULL,
            tskIDLE_PRIORITY+1, NULL );
    xTaskCreate( vButtonTask, ( signed portCHAR * ) "vButtonTask", 100, NULL, 
            tskIDLE_PRIORITY, NULL );
    
    /* Create the Queue for communication between the tasks */
    xButtonQueue = xQueueCreate( 5, sizeof(uint8_t) );

    Serial.print("FREE=");
    Serial.println(freeMemory());

    vTaskStartScheduler();
}

void loop( void )
{
}

/*--------------------------------------------------------------------------*/

static void vLedTask( void * )
{
    pinMode(led_red,OUTPUT);
    digitalWrite(led_red,OUTPUT);
    uint8_t state;
    
    for( ;; )
    {
        /* Wait until an element is received from the queue */
        if (xQueueReceive(xButtonQueue, &state, portMAX_DELAY))
        {
            // On button UP, toggle the LED
            if ( state == HIGH )
                digitalWrite(led_red,digitalRead(led_red)^1);
        }
    }
}

/*--------------------------------------------------------------------------*/

static void vButtonTask( void * )
{
    pinMode(button_a,INPUT);
    digitalWrite(button_a,HIGH);
    uint8_t newstate, state = HIGH;

    for( ;; )
    {
        if ( digitalRead(button_a) != state )
        {
            // debounce and read again
            vTaskDelay( 20 );
            
            newstate = digitalRead(button_a);
            if ( newstate != state )
            {
                state = newstate;
                xQueueSendToBack(xButtonQueue, &newstate, 0);
            }
        }
        // read again soon 
        vTaskDelay( 20 );
    }
}

// vim:cin:ai:et:sts=4 sw=4
