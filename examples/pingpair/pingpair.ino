/*****************************************************************************
  *
  * RF24 Pingpair using FreeRTOS 
  *
  * See .cpp files for the code (INO file is just here to appease the IDE).
  *
  * This requires the RF24 library, of course.
  * http://maniacbug.github.com/RF24
  * 
  * This is designed to work with a node running pingpair_irq.  They use the
  * same radio setup.  Pingpair_irq sends by default, and this receives by
  * default.
  */
#include <RF24.h>
#include <SPI.h>
#include <FreeRTOS.h>
#include <MemoryFree.h>

#ifdef MAPLE_IDE
#include "MapleFreeRTOS.h"
#endif

// vim:cin:ai:sts=4 sw=4 ft=cpp
