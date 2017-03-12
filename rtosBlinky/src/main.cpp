//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include "cmsis_device.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>

// ----------------------------------------------------------------------------
//
// Semihosting STM32F4 empty sample (trace via DEBUG).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

GPIO_TypeDef* ledPort = GPIOC;
const unsigned int ledClock = RCC_AHB1ENR_GPIOCEN;
const unsigned int ledPinLow = GPIO_BSRR_BS_12;
const unsigned int ledPinHigh = GPIO_BSRR_BR_12;

struct task_param
{
    char* name;
    int   interval;
};

static void fpu_task(void *pvParameters)
{
    // Force x to stay in a FPU reg.
    //
    register int x = 0;
    struct task_param *p = (task_param*)pvParameters;

    while(1)
    {
        ledPort->BSRR = ledPinLow;
        x += 1;
        printf("%s: x = %i\n", p->name, x);
        vTaskDelay(p->interval);
        ledPort->BSRR = ledPinHigh;
        vTaskDelay(p->interval);
    }
}

static void init_task(void *pvParameters)
{
    RCC->AHB1ENR |= ledClock;

    GPIO_InitTypeDef GPIO_InitStructure;

    // Configure pin in output push/pull mode
    GPIO_InitStructure.Pin = ledPinLow;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ledPort, &GPIO_InitStructure);

    ledPort->BSRR = ledPinLow;
    vTaskDelay(100);
    ledPort->BSRR = ledPinHigh;
    vTaskDelay(100);

    for (int i=0; i<5; i++) {

        printf("Starting task %d..\n", i);

        struct task_param *p;

        p = (task_param*)malloc(sizeof(struct task_param));
        p->name     = (char*)malloc(16);
        p->interval = (i+1) * (6-i);
        sprintf(p->name, "task_%d", i);

        xTaskCreate(fpu_task, p->name, 1024, p, tskIDLE_PRIORITY, NULL);
    }

    while(1);
}


int main(int argc, char* argv[])
{
    // FreeRTOS assumes 4 preemption- and 0 subpriority-bits
    //
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    // Create init task and start the scheduler
    //
    xTaskCreate(init_task, "init", 1024, NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();

    return EXIT_SUCCESS;
}

// ----------------------------------------------------------------------------
