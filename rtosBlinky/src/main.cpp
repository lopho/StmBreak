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
        vTaskDelay((p->interval+0.5f)/2);
        sLED_PORT->BSRR = sLED_PIN_HIGH;
        vTaskDelay((p->interval+0.5f)/2);
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

static void initLed()
{
    HAL_GPIO_DeInit(sLED_PORT, sLED_PIN_LOW);

    __HAL_RCC_GPIOC_CLK_ENABLE();

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

    printf("Starting led task\n");

    struct ledTaskParam* p;

    p = (ledTaskParam*)malloc(sizeof(struct ledTaskParam));
    p->name     = (char*)malloc(16);
    p->interval = 1;
    sprintf(p->name, "led_task");

    xTaskCreate(ledTask, p->name, 1024, p, tskIDLE_PRIORITY, NULL);
}

static void initAdc(ADC_HandleTypeDef& o_adcHandle)
{
    HAL_GPIO_DeInit(sADC_PORT, sADC_PIN);

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();

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
     p->interval = 1;
     sprintf(p->name, "ADC_TASK");
     p->handle = o_adcHandle;

     xTaskCreate(adcTask, p->name, 1024, p, tskIDLE_PRIORITY, NULL);
}

static void initPwm(TIM_HandleTypeDef& o_pwmHandle)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    //__TIM2_CLK_ENABLE();
//    o_pwmHandle.Instance = TIM1;
//    o_pwmHandle.Init.Prescaler = 40000;
//    o_pwmHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
//    o_pwmHandle.Init.Period = 500;
//    o_pwmHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
//    o_pwmHandle.Init.RepetitionCounter = 0;
//    HAL_TIM_Base_Init(&o_pwmHandle);
//    HAL_TIM_Base_Start(&o_pwmHandle);
//
//
//    o_pwmHandle.Channel
//    TIM_OC_InitTypeDef outputChannelInit = {0,};
//    outputChannelInit.OCMode = TIM_OCMODE_PWM1;
//    outputChannelInit.Pulse = 400;
//    //outputChannelInit.OutputState = TIM_OutputState_Enable;
//    outputChannelInit.OCFastMode = TIM_OCFAST_ENABLE;
//    outputChannelInit.OCPolarity = TIM_OCPOLARITY_HIGH;
//
//    TIM_OC1Init(TIM1, &outputChannelInit);
//    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
//
//    GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF1_TIM1);
//    //GPIO_AF1_TIM1

//    TIM_OC_InitTypeDef sConfigOC;
//
//    o_pwmHandle.Instance = TIM1;
//    o_pwmHandle.Init.Prescaler = 6;
//    o_pwmHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
//    o_pwmHandle.Init.Period = 1300;
//    o_pwmHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
//    HAL_TIM_PWM_Init(&o_pwmHandle);
//
//    sConfigOC.OCMode = TIM_OCMODE_PWM1;
//    sConfigOC.OCIdleState = TIM_OCIDLESTATE_SET;
//    sConfigOC.Pulse = 650;
//    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
//    sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
//    HAL_TIM_PWM_ConfigChannel(&o_pwmHandle, &sConfigOC, TIM_CHANNEL_1);
//
//    GPIO_InitTypeDef GPIO_InitStruct;
//    GPIO_InitStruct.Pin = GPIO_PIN_8;
//    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//    GPIO_InitStruct.Pull = GPIO_NOPULL;
//    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
//    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
//    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//
//    //HAL_TIM_Base_Start(&o_pwmHandle);
//    HAL_TIM_PWM_Start(&o_pwmHandle, TIM_CHANNEL_1);

    TIM_ClockConfigTypeDef sClockSourceConfig;
    TIM_MasterConfigTypeDef sMasterConfig;
    TIM_OC_InitTypeDef sConfigOC;
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;

    o_pwmHandle.Instance = TIM1;
    o_pwmHandle.Init.Prescaler = 8;
    o_pwmHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    o_pwmHandle.Init.Period = 1000;
    o_pwmHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    o_pwmHandle.Init.RepetitionCounter = 0;
    if (HAL_TIM_Base_Init(&o_pwmHandle) != HAL_OK)
    {
      //Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&o_pwmHandle, &sClockSourceConfig) != HAL_OK)
    {
      //Error_Handler();
    }

    if (HAL_TIM_PWM_Init(&o_pwmHandle) != HAL_OK)
    {
      //Error_Handler();
    }

  //  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  //  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  //  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  //  {
  //    Error_Handler();
  //  }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 500;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&o_pwmHandle, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
    {
      //Error_Handler();
    }

  //  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  //  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  //  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  //  sBreakDeadTimeConfig.DeadTime = 0;
  //  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  //  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  //  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  //  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  //  {
  //    Error_Handler();
  //  }

    GPIO_InitTypeDef GPIO_InitStruct;
    if(o_pwmHandle.Instance==TIM1)
    {
    /* USER CODE BEGIN TIM1_MspPostInit 0 */

    /* USER CODE END TIM1_MspPostInit 0 */

      /**TIM1 GPIO Configuration
      PA9     ------> TIM1_CH2
      */
      GPIO_InitStruct.Pin = GPIO_PIN_9;
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
      GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
      HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USER CODE BEGIN TIM1_MspPostInit 1 */

    /* USER CODE END TIM1_MspPostInit 1 */
    }

    HAL_TIM_PWM_Start(&o_pwmHandle, TIM_CHANNEL_2);
}

static void initTask(void* /*pvParameters*/)
{
    TIM_HandleTypeDef pwmHandle;
    initPwm(pwmHandle);

//    initLed();
//
//    ADC_HandleTypeDef adcHandle;
//    initAdc(adcHandle);

    while(1);
}

int main(int /*argc*/, char** /*argv*/)
{
    HAL_Init();

    // FreeRTOS assumes 4 preemption- and 0 subpriority-bits
    //
    //HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    //HAL_NVIC_SetPriority(ADC_IRQn, 0, 0);
    //HAL_NVIC_EnableIRQ(ADC_IRQn);


    // Create init task and start the scheduler
    //
    xTaskCreate(initTask, "init", 1024, NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();

    return EXIT_SUCCESS;
}

// ----------------------------------------------------------------------------
