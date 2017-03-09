//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include "cmsis_device.h"

#include <stdio.h>
#include <stdlib.h>

#include "Timer.h"
#include "BlinkLed.h"

// ----------------------------------------------------------------------------
//
// Semihosting STM32F4 led blink sample (trace via DEBUG).
//
// In debug configurations, demonstrate how to print a greeting message
// on the trace device. In release configurations the message is
// simply discarded.
//
// To demonstrate semihosting, display a message on the standard output
// and another message on the standard error.
//
// Then demonstrates how to blink a led with 1 Hz, using a
// continuous loop and SysTick delays.
//
// On DEBUG, the uptime in seconds is also displayed on the trace device.
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// Definitions visible only within this translation unit.
namespace
{
// ----- Timing definitions -------------------------------------------------

// Keep the LED on for 2/3 of a second.
constexpr Timer::ticks_t BLINK_ON_TICKS = Timer::FREQUENCY_HZ * 3 / 4;
constexpr Timer::ticks_t BLINK_OFF_TICKS = Timer::FREQUENCY_HZ - BLINK_ON_TICKS;
}

// ----- LED definitions ------------------------------------------------------

// Olimex STM32-H405
// Port C (2) (A-0, B-1, C-2, D-3)
#define BLINK_PORT_NUMBER  (2)
// Pin PC12 (12) - overall pin 53
#define BLINK_PIN_NUMBER   (12)
#define BLINK_ACTIVE_LOW   (false)

BlinkLed blinkLeds =
    { BLINK_PORT_NUMBER, BLINK_PIN_NUMBER, BLINK_ACTIVE_LOW };

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wunused-parameter"
//#pragma GCC diagnostic ignored "-Wmissing-declarations"
//#pragma GCC diagnostic ignored "-Wreturn-type"

int
main(int argc, char* argv[])
{
    // Show the program parameters (passed via semihosting).
    // Output is via the semihosting output channel.
    //trace_dump_args(argc, argv);

    // Send a greeting to the trace device (skipped on Release).
    //trace_puts("Hello ARM World!");

    // Send a message to the standard output.
    puts("Standard output message.");

    // Send a message to the standard error.
    fprintf(stderr, "Standard error message.\n");

    // At this stage the system clock should have already been configured
    // at high speed.
    //trace_printf("System clock: %u Hz\n", SystemCoreClock);

    Timer timer;
    timer.start();

    // Perform all necessary initialisations for the LEDs.
    blinkLeds.powerUp();

    uint32_t seconds = 0;

    blinkLeds.turnOn();

    // First second is long.
    timer.sleep(Timer::FREQUENCY_HZ);

    blinkLeds.turnOff();

    timer.sleep(BLINK_OFF_TICKS);

    ++seconds;
    //trace_printf ("Second %u\n", seconds);

    for (int i = 0; i < 5; i++)
    {
        blinkLeds.turnOn();
        timer.sleep(BLINK_ON_TICKS);

        blinkLeds.turnOff();
        timer.sleep(BLINK_OFF_TICKS);

        ++seconds;
        //trace_printf ("Second %u\n", seconds);
    }

    blinkLeds.turnOn();

    timer.sleep(Timer::FREQUENCY_HZ);

    return 0;
}

//#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
