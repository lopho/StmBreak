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

static GPIO_TypeDef* sLED_PORT = GPIOC;
static const unsigned int sLED_PIN_LOW = GPIO_BSRR_BS_12;
static const unsigned int sLED_PIN_HIGH = GPIO_BSRR_BR_12;

static GPIO_TypeDef* sADC_PORT = GPIOC;
static const ADC_TypeDef* sADC = ADC1;
static const unsigned int sADC_PIN = GPIO_PIN_1;
static const unsigned int sADC_CHANNEL = ADC_CHANNEL_11;

struct ledTaskParam
{
    char* name;
    int   interval;
};

struct adcTaskParam
{
    char* name;
    int   interval;
    ADC_HandleTypeDef handle;
};

static void ledTask(void* pvParameters)
{
    // Force x to stay in a FPU reg.
    //
    register float x = 0;
    struct ledTaskParam *p = (ledTaskParam*)pvParameters;

    while(1)
    {
        sLED_PORT->BSRR = sLED_PIN_LOW;
        x += 1;
        printf("%s: x = %i\n", p->name, (int)x);
        vTaskDelay(p->interval);
        sLED_PORT->BSRR = sLED_PIN_HIGH;
        vTaskDelay(p->interval);
    }
}

static void adcTask(void* pvParameters)
{
    register uint32_t adcValue;
    struct adcTaskParam* p = (adcTaskParam*)pvParameters;
    HAL_ADC_Start(&p->handle);

    while (1)
    {
        if (HAL_ADC_PollForConversion(&p->handle, 1000000) == HAL_OK)
        {
            adcValue = HAL_ADC_GetValue(&p->handle);
            printf("ADC: x = %i\n", adcValue);
            vTaskDelay(p->interval);
        }
    }
}

static void setPwm()
{

}

static void initAdc(ADC_HandleTypeDef& o_adcHandle)
{
     GPIO_InitTypeDef GPIO_adcInitStructure;
     GPIO_adcInitStructure.Pin = sADC_PIN;
     GPIO_adcInitStructure.Mode = GPIO_MODE_ANALOG;
     GPIO_adcInitStructure.Pull = GPIO_NOPULL;
     HAL_GPIO_Init(sADC_PORT, &GPIO_adcInitStructure);


     o_adcHandle.Instance = ADC1;

     o_adcHandle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
     o_adcHandle.Init.Resolution = ADC_RESOLUTION_12B;
     o_adcHandle.Init.ScanConvMode = DISABLE;
     o_adcHandle.Init.ContinuousConvMode = ENABLE;
     o_adcHandle.Init.DiscontinuousConvMode = DISABLE;
     o_adcHandle.Init.NbrOfDiscConversion = 0;
     o_adcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
     o_adcHandle.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
     o_adcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
     o_adcHandle.Init.NbrOfConversion = 1;
     o_adcHandle.Init.DMAContinuousRequests = ENABLE;
     o_adcHandle.Init.EOCSelection = DISABLE;

     HAL_ADC_Init(&o_adcHandle);

     ADC_ChannelConfTypeDef ADC_adcChannel;

     ADC_adcChannel.Channel = sADC_CHANNEL;
     ADC_adcChannel.Rank = 1;
     ADC_adcChannel.SamplingTime = ADC_SAMPLETIME_480CYCLES;
     ADC_adcChannel.Offset = 0;

     if (HAL_ADC_ConfigChannel(&o_adcHandle, &ADC_adcChannel) != HAL_OK)
     {
         asm("bkpt 255");
     }

     printf("Starting ADC task\n");
     struct adcTaskParam* p;

     p = (adcTaskParam*)malloc(sizeof(struct adcTaskParam));
     p->name     = (char*)malloc(16);
     p->interval = 4;
     sprintf(p->name, "ADC_TASK");
     p->handle = o_adcHandle;

     xTaskCreate(adcTask, p->name, 1024, p, tskIDLE_PRIORITY, NULL);

}

static void initTask(void* /*pvParameters*/)
{
    GPIO_InitTypeDef GPIO_ledInitStructure;

    // Configure pin in output push/pull mode
    GPIO_ledInitStructure.Pin = sLED_PIN_LOW;
    GPIO_ledInitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_ledInitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_ledInitStructure.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(sLED_PORT, &GPIO_ledInitStructure);

    sLED_PORT->BSRR = sLED_PIN_LOW;
    vTaskDelay(100);
    sLED_PORT->BSRR = sLED_PIN_HIGH;
    vTaskDelay(100);

    for (int i=0; i<5; i++) {

        printf("Starting task %d..\n", i);

        struct ledTaskParam *p;

        p = (ledTaskParam*)malloc(sizeof(struct ledTaskParam));
        p->name     = (char*)malloc(16);
        p->interval = (i+1) * (6-i);
        sprintf(p->name, "task_%d", i);

        xTaskCreate(ledTask, p->name, 1024, p, tskIDLE_PRIORITY, NULL);
    }

    ADC_HandleTypeDef adcHandle;
    initAdc(adcHandle);

    while(1);
}

int main(int /*argc*/, char** /*argv*/)
{
    HAL_Init();

    HAL_GPIO_DeInit(sLED_PORT, sLED_PIN_LOW);
    HAL_GPIO_DeInit(sADC_PORT, sADC_PIN);

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();


    // FreeRTOS assumes 4 preemption- and 0 subpriority-bits
    //
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    //HAL_NVIC_SetPriority(ADC_IRQn, 0, 0);
    //HAL_NVIC_EnableIRQ(ADC_IRQn);

    // Create init task and start the scheduler
    //
    xTaskCreate(initTask, "init", 1024, NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();

    return EXIT_SUCCESS;
}

// ----------------------------------------------------------------------------
