/**
 * @file setup.cpp
 *
 * Instantiate and set up all objects, run main loop
 */

/* System includes */

/* LIbrary includes */
#include "MemoryFree.h"

/* Application includes */
#include "globals.h"

#include "printf.h"

/****************************************************************************/

/**
 * Setup
 *
 * Creates all the objects needed by the application and launches the
 * task loop.
 */
void setup( void )
{
    // Set up the LED to steady on
    pinMode(BOARD_LED_PIN, OUTPUT);
    digitalWrite(BOARD_LED_PIN, HIGH);

    // Setup the button as input
    pinMode(BOARD_BUTTON_PIN, INPUT);
    digitalWrite(BOARD_BUTTON_PIN, HIGH);

    // Wait until the user is watching
    SerialUSB.begin(SERIAL_BAUD(57600));
    printf_begin();
    printf_P(PSTR("FREE=%u\r\n"),freeMemory());
    printf_P(PSTR("Press any key to continue\r\n"));
    while ( ! SerialUSB.available() )
    {
    }
    SerialUSB.read();
    printf_P(PSTR("Starting ~/Source/Arduino/1284P/FreeRTOS/pingpair_freertos...\r\n"));

    pinMode(status_pending,OUTPUT);
    pinMode(status_ok,OUTPUT);
    pinMode(status_fail,OUTPUT);

    // setup the radio
    setup_radio();

    /* Create the Semaphore for synchronization between hw & softirq */
    vSemaphoreCreateBinary( xRadioIrqSemaphore);

    /* Create the Mutex for accessing the radio */
    xRadioMutex = xSemaphoreCreateMutex();

    /* Create the Mutex for accessing the console */
    xConsoleMutex = xSemaphoreCreateMutex();

    /* Create the Payload Queue which carries messages received from the radio */
    xPayloadQueue = xQueueCreate( 5, sizeof(payload_t) );

    /* Create the Status Queue which carries status received from the radio */
    xStatusQueue = xQueueCreate( 5, sizeof(status_e) );

    /* Add the tasks to the scheduler */
    xTaskCreate(vRadioSoftIrqTask, (const signed char*)PSTR("RA"),
                configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL );
    xTaskCreate(vReceiverTask, (const signed char*)PSTR("RC"),
                configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL );
    xTaskCreate(vSenderTask, (const signed char*)PSTR("SE"),
                configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL );
    xTaskCreate(vStatusTask, (const signed char*)PSTR("ST"),
                configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );
    xTaskCreate(vInputTask, (const signed char*)PSTR("IN"),
                configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );

    /* Start the scheduler. */
    vTaskStartScheduler();
}

#ifdef ARDUINO
extern "C" void _write(int,char* str,size_t len)
{
    while(len--)
        Serial.print(*str++);
}
void loop(void)
{
}
#endif


// vim:cin:ai:et:sts=4 sw=4
