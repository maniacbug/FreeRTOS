/**
 * @file globals.h 
 *
 * Single global header file that all modules of the application need.
 */

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#ifdef MAPLE_IDE

/* Framework includes */
#include <WProgram.h>

/* Library includes. */
#include "MapleFreeRTOS.h"

#define SERIAL_BAUD(x)

#endif // MAPLE_IDE

#ifdef ARDUINO

#include <avr/pgmspace.h>
#include <Arduino.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define BOARD_LED_PIN LED
#define BOARD_BUTTON_PIN 2
#define SerialUSB Serial
#define SERIAL_BAUD(x) (x)

#endif // ARDUINO

#include "RF24.h"

// The various roles supported by this sketch
typedef enum { role_sender = 1, role_receiver } role_e;

// The role of the current running sketch
extern role_e role;

enum status_e { status_none = 0, status_txok, status_txfail, status_invalid };

extern const char* const status_str[];

/* Function Prototypes */
extern void vSenderTask(void* pvParameters);
extern void vReceiverTask(void* pvParameters);
extern void vStatusTask(void* pvParameters);
extern void vInputTask(void* pvParameters);
extern void vRadioSoftIrqTask(void* pvParameters);
extern void setup_radio(void);
extern void radio_set_role(role_e);

/* Global Variables */
extern xSemaphoreHandle xRadioIrqSemaphore;
extern xSemaphoreHandle xRadioMutex;
extern xSemaphoreHandle xConsoleMutex;
extern xQueueHandle xPayloadQueue;
extern xQueueHandle xStatusQueue;

extern RF24 radio;

// Status LEDs

#ifndef PINS_DEFINED
const int led_red = 7;
const int led_yellow = 6;
const int led_green = 5;
#endif

#ifndef LED
#define LED led_red
#endif

const int status_pending = led_yellow;
const int status_ok = led_green;
const int status_fail = led_red;

//
// Payload
//

struct payload_t
{
    char type;
    uint32_t value;

    payload_t(void): type(0), value(0) {}
    payload_t( char _type, uint32_t _value ): type(_type), value(_value) {}
} __attribute__ ((packed));

#endif // __GLOBALS_H__

// vim:cin:ai:et:sts=4 sw=4
