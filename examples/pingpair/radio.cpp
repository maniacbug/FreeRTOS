/**
 * @file radio.cpp
 *
 * All major interaction with the radio is here 
 */

/* Library includes */
#include "RF24.h"

/* Application includes */
#include "globals.h"

/* Forward delcarations */
static void rf_irq_handler(void);

/* RTOS objects */
xSemaphoreHandle xRadioIrqSemaphore;
xSemaphoreHandle xRadioMutex;
xQueueHandle xPayloadQueue;
xQueueHandle xStatusQueue;

/* Hardware Setup */

#ifdef __AVR_ATmega328P__
// Kind of a hack to experiment with this sketch on 328
const int rf_ce_pin = 8; 
const int rf_csn_pin = 9;
const int rf_irq_pin = 0;
#endif

#ifdef __AVR_ATmega1284P__
const int rf_ce_pin = rf_ce; //7;
const int rf_csn_pin = rf_csn; //6;
const int rf_irq_pin = rf_irq; //12;
#endif

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus

RF24 radio(rf_ce_pin,rf_csn_pin);

#ifdef MAPLE_IDE
HardwareSPI SPI(2);
#endif

//
// Role management
//
// Set up role.  This sketch uses the same software for all the nodes in this
// system.  Doing so greatly simplifies testing.
//

// The debug-friendly names of those roles
const char role_friendly_name_0[] PROGMEM = "invalid";
const char role_friendly_name_1[] PROGMEM = "Sender";
const char role_friendly_name_2[] PROGMEM = "Receiver";
const char* const role_friendly_name[] PROGMEM = {
role_friendly_name_0,
role_friendly_name_1,
role_friendly_name_2,
};

// The role of the current running sketch
role_e role = role_receiver;

//
// Status messages
//

const char status_str_0[] PROGMEM = "None";
const char status_str_1[] PROGMEM = "Send OK";
const char status_str_2[] PROGMEM = "Send Failed";
const char status_str_3[] PROGMEM = "*** INVALID ***";
const char* const status_str[] PROGMEM = { 
status_str_0,
status_str_1,
status_str_2,
status_str_3,
};

//
// Topology
//

// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;

/****************************************************************************/

void setup_radio(void)
{
    //
    // Setup and configure rf radio
    //

    radio.begin();

    // We will be using the Ack Payload feature, so please enable it
    radio.enableAckPayload();

    //
    // Open pipes to other nodes for communication
    //

    // This simple sketch opens a single pipe for these two nodes to
    // communicate back and forth.  One listens on it, the other talks to it.

    if ( role == role_sender )
    {
        radio.openWritingPipe(pipe);
        radio.openReadingPipe(1,0);
    }
    else
    {
        radio.openWritingPipe(0);
        radio.openReadingPipe(1,pipe);
    }

    //
    // Start listening
    //

    if ( role == role_sender )
        radio.startListening();

    //
    // Dump the configuration of the rf unit for debugging
    //

    radio.printDetails();

    //
    // Attach interrupt handler to interrupt #0 (using pin 2)
    // on BOTH the sender and receiver
    //

    /* Get called when the button is pressed */
    attachInterrupt(rf_irq_pin,rf_irq_handler,FALLING);

    //
    // Give information about the role
    //

    printf_P(PSTR("ROLE: %S\r\n"),pgm_read_word(&role_friendly_name[role]));
    printf_P(PSTR("*** PRESS 'S' to begin sending to the other node"));
    
    pinMode(led_yellow,OUTPUT);
    digitalWrite(led_yellow,HIGH);
}
/****************************************************************************/

void radio_set_role(role_e newrole)
{
    if ( newrole == role_sender )
    {
        xSemaphoreTake(xRadioMutex, portMAX_DELAY);
        role = role_sender;
        radio.openWritingPipe(pipe);
        radio.openReadingPipe(1,0);
        radio.stopListening();
        xSemaphoreGive(xRadioMutex);
    }
    else if ( newrole == role_receiver )
    {
        // Become the primary receiver (pong back)
        xSemaphoreTake(xRadioMutex, portMAX_DELAY);
        role = role_receiver;
        radio.openWritingPipe(0);
        radio.openReadingPipe(1,pipe);
        radio.startListening();
        xSemaphoreGive(xRadioMutex);
    }
}

/****************************************************************************/

/**
 * The Radio Soft IRQ task
 * It waits the semaphore is given, then takes it and services the radio
 *
 * Ideally, the only thing this softirq interacts with is the radio
 * and the queues.
 *
 * \param pvParameters NULL is passed, unused here.
 */
void vRadioSoftIrqTask(void*)
{
    /* Infinite loop */
    while(1)
    {
        /* Block until the semaphore is given */
        xSemaphoreTake(xRadioIrqSemaphore, portMAX_DELAY);

        /* Get the radio access semaphore for the whole time */
        xSemaphoreTake(xRadioMutex, portMAX_DELAY);

        digitalWrite(status_pending,LOW);
        
        // What happened?
        bool tx,fail,rx;
        radio.whatHappened(tx,fail,rx);

        // Have we successfully transmitted?
        if ( tx )
        {
            status_e status = status_txok;
            xQueueSendToBack(xStatusQueue, &status, 0);
        }

        // Have we failed to transmit?
        if ( fail )
        {
            digitalWrite(status_fail,HIGH);
            status_e status = status_txfail;
            xQueueSendToBack(xStatusQueue, &status, 0);
        }

        // Transmitter can power down for now, because
        // the transmission is done.
        if ( ( tx || fail ) && ( role == role_sender ) )
            radio.powerDown();

        // Did we receive a message?
        if ( rx )
        {
            digitalWrite(status_ok,HIGH);
           
            // Get this payload and queue it
            payload_t payload;
            radio.read( &payload, sizeof(payload) );
            xQueueSendToBack(xPayloadQueue, &payload, 0);
#if 0
            SerialUSB.print("[RAW] ** ");
            SerialUSB.print(sizeof(payload_t));
            SerialUSB.print(" ");
            uint8_t* raw = reinterpret_cast<uint8_t*>(&payload);
            int i = 0;
            while ( i < sizeof(payload) )
            {
                SerialUSB.print(raw[i],DEC);
                SerialUSB.print(" ");
                ++i;
            }
            SerialUSB.println(" **");
#endif
        }

        // Done with the radio now.
        xSemaphoreGive(xRadioMutex);
        
        // Print the stack hi water mark
        xSemaphoreTake(xConsoleMutex, portMAX_DELAY);
        printf_P(PSTR("Stack min: %u\r\n"),uxTaskGetStackHighWaterMark(NULL));
        xSemaphoreGive(xConsoleMutex);
        
    }
}

/****************************************************************************/

/**
 * The irq callback function called when the radio hw irq is raised.
 * It gives the semaphore.
 */
static void rf_irq_handler(void)
{
    digitalWrite(led_yellow,LOW);
    static signed portBASE_TYPE xHigherPriorityTaskWoken;
    /* Give the semaphore */
    xSemaphoreGiveFromISR(xRadioIrqSemaphore, &xHigherPriorityTaskWoken);
}

/****************************************************************************/

// vim:cin:ai:et:sts=4 sw=4
