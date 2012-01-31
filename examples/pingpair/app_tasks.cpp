/**
 * @file app_tasks.cpp
 *
 * All application logic is contain within these tasks
 */

/* System includes */
#include <ctype.h>

/* Application includes */
#include "globals.h"

static uint32_t message_count = 0;

// Note that this module is the only one which should be accessing the console
// ergo the only using this mutex.
xSemaphoreHandle xConsoleMutex;

/****************************************************************************/

/**
 * The input task
 * It monitors the serial input and modifies the behaviour of the program
 * based on user input.
 * \param pvParameter NULL is passed as parameter.
 */
void vInputTask(void*)
{
    portTickType xLastWakeTime = xTaskGetTickCount();
    const portTickType tickPeriod = 20;

    /* Infinite loop */
    while (1)
    {
        if (SerialUSB.available())
        {
            char c = toupper(SerialUSB.read());
            if ( c == 'S' && role == role_receiver )
            {
                xSemaphoreTake(xConsoleMutex, portMAX_DELAY);
                printf_P(PSTR("*** CHANGING TO SENDER ROLE -- PRESS 'R' TO SWITCH BACK\r\n"));
                xSemaphoreGive(xConsoleMutex);

                // Become the primary transmitter (ping out)
                radio_set_role(role_sender);
            }
            else if ( c == 'R' && role == role_sender )
            {
                xSemaphoreTake(xConsoleMutex, portMAX_DELAY);
                printf_P(PSTR("*** CHANGING TO RECEIVE ROLE -- PRESS 'S' TO SWITCH BACK\r\n"));
                xSemaphoreGive(xConsoleMutex);

                // Become the primary receiver (pong back)
                radio_set_role(role_receiver);
            }
            else
            {
                printf_P(PSTR("Unknown: %c\r\n"),c);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, tickPeriod);
    }
}

/****************************************************************************/

/**
 * The Status task
 * It simply monitors the status queue and notifies the user
 * \param pvParameter NULL is passed as parameter.
 */
void vStatusTask(void*)
{
    status_e status;

    /* Infinite loop */
    while (1)
    {
        /* Wait until an element is received from the queue */
        if (xQueueReceive(xStatusQueue, &status, portMAX_DELAY))
        {
            xSemaphoreTake(xConsoleMutex, portMAX_DELAY);
            printf_P(PSTR("Status: %S\r\n"),pgm_read_word(&status_str[status]));
            xSemaphoreGive(xConsoleMutex);
        }
    }
}

/****************************************************************************/

/**
 * The Sender task
 * It sends a ping out repeatedly
 * then waits for a given delay and start over again.
 * \param pvParameter NULL is passed as parameter.
 */
void vSenderTask(void*)
{
    portTickType xLastWakeTime = xTaskGetTickCount();

    // How often to wait between sends.
    const portTickType sendPeriod = 2000;

    /* Infinite task loop */
    while (1)
    {
        if ( role == role_sender )
        {
            digitalWrite(status_pending,HIGH);
            digitalWrite(status_ok,LOW);
            digitalWrite(status_fail,LOW);
            
            // Take the time, and send it.
            payload_t payload('T',xLastWakeTime);

            xSemaphoreTake(xConsoleMutex, portMAX_DELAY);
            printf_P(PSTR("Now sending %u\r\n"),payload.value);
            xSemaphoreGive(xConsoleMutex);

            xSemaphoreTake(xRadioMutex, portMAX_DELAY);
            radio.stopListening();
            radio.startWrite( &payload, sizeof(payload) );
            xSemaphoreGive(xRadioMutex);
        }

        /* Block the task for the defined time */
        vTaskDelayUntil(&xLastWakeTime,
                        sendPeriod - (xLastWakeTime % sendPeriod) );
    }
}

/****************************************************************************/

/**
 * The Receiver task function.
 * It waits until a ping message has been put in the queue, by the radio
 * softirq, and prints it out to the serial usb, and adds an ack payload
 * for hte next time around.
 * \param pvParameter NULL is passed as parameter.
 */
void vReceiverTask(void*)
{
    payload_t payload;

    /* Infinite loop */
    while (1)
    {
        /* Wait until an element is received from the queue */
        if (xQueueReceive(xPayloadQueue, &payload, portMAX_DELAY))
        {
            xSemaphoreTake(xConsoleMutex, portMAX_DELAY);
            printf_P(PSTR("Got payload type=%02x value=%u\r\n"),payload.type,payload.value);
            xSemaphoreGive(xConsoleMutex);

            if ( payload.type == 'T' )
            {
                // Add an ack packet for the next time around.  This is a simple
                // packet counter
                payload_t ack('A',++message_count);

                xSemaphoreTake(xRadioMutex, portMAX_DELAY);
                radio.writeAckPayload( 1, &ack, sizeof(ack) );
                xSemaphoreGive(xRadioMutex);

                xSemaphoreTake(xConsoleMutex, portMAX_DELAY);
                printf_P(PSTR("Sent ack value=%u\r\n"),ack.value);
                xSemaphoreGive(xConsoleMutex);
            }
        }
    }
}

// vim:cin:ai:et:sts=4 sw=4
