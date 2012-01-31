#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "Arduino.h"
#include "MemoryFree.h"

static void vButtonTask( void *pvParameters );
static void vPrintTask( void *pvParameters );
static xQueueHandle xButtonQueue;

#ifndef PINS_DEFINED
const int button_a = 2;
const int button_b = 3;
const int button_c = 4;
#endif

struct button_status
{
    uint8_t pin;
    uint8_t state;
};

uint8_t button_pins[] = { button_a, button_b, button_c };

/*--------------------------------------------------------------------------*/

void setup( void )
{
    Serial.begin(57600);
    Serial.println(__FILE__);

    /* Create the tasks defined within this file. */
    xTaskCreate( vPrintTask, ( signed portCHAR * ) "vPrintTask", 120, NULL, 
            tskIDLE_PRIORITY, NULL );

    // One task for each button
    int i = sizeof(button_pins);
    while ( i-- )
    {
        xTaskCreate( vButtonTask, ( signed portCHAR * ) "vButtonTask", 70, button_pins + i, 
                tskIDLE_PRIORITY, NULL );
    }

    /* Create the Queue for communication between the tasks */
    xButtonQueue = xQueueCreate( sizeof(button_pins), sizeof(button_status) );

    Serial.print("FREE=");
    Serial.println(freeMemory());

    vTaskStartScheduler();
}

void loop( void )
{
}

/*--------------------------------------------------------------------------*/

static void vButtonTask( void * pv )
{
    uint8_t* buttonPin = reinterpret_cast<uint8_t*>(pv);
    
    button_status qstatus;
    qstatus.pin = *buttonPin; 
    qstatus.state = HIGH; 

    pinMode(*buttonPin,INPUT);
    digitalWrite(*buttonPin,HIGH);
    
    for( ;; )
    {
        if ( digitalRead(*buttonPin) != qstatus.state )
        {
            // debounce and read again
            vTaskDelay( 20 );
            
            uint8_t newstate = digitalRead(*buttonPin);
            if ( newstate != qstatus.state )
            {
                qstatus.state = newstate;

                xQueueSendToBack(xButtonQueue, &qstatus, 0);
            }
        }
        // read again soon 
        vTaskDelay( 20 );
    }
}

/*--------------------------------------------------------------------------*/

static void vPrintTask( void * )
{
    button_status qstatus;
    
    for( ;; )
    {
        /* Wait until an element is received from the queue */
        if (xQueueReceive(xButtonQueue, &qstatus, portMAX_DELAY))
        {
            //Serial.print(uxTaskGetStackHighWaterMark(NULL));
            Serial.print(" Button ");
            Serial.print(qstatus.pin);
            Serial.print(" is ");
            Serial.println(qstatus.state?"HIGH":"LOW");
        }
    }
}

// vim:cin:ai:et:sts=4 sw=4
