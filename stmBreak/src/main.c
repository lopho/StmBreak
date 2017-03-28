/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "device.h"
#include "flash.h"
#include "cmsis_os.h"
#include "diag/Trace.h"
#include <math.h>
#include <stdlib.h>

/* Peripheral handles --------------------------------------------------------*/

/* Task threads --------------------------------------------------------------*/
static FlashBank userBank1;
static FlashBank userBank2;
static FlashBank userBank3;
static uint16_t functionLUT[1000];

osThreadId adcTaskHandle;
osThreadId usartTaskHandle;
osThreadId userButtonTaskHandle;

osPoolId paramPoolId;
osMessageQId adcMessageQ;


/* Structs and typedefs ------------------------------------------------------*/
typedef struct TaskParameter
{
  osMessageQId messageQ;
} TaskParameter;

/* Function prototypes -------------------------------------------------------*/
static void Error_Handler(void);

void adcTask(void const* argument);
void usartTask(void const* argument);
void userButtonTask(void const* argument);

/* Main ----------------------------------------------------------------------*/
/**
 * main
 * @return exit code
 */
int main(void)
{
  setupDevice();
  setupFlash();

  /* Create flash memory for loading and saving user functions */
  userBank1 = createFlashBank(1000, FLASH_16B);
  userBank2 = createFlashBank(1000, FLASH_16B);
  userBank3 = createFlashBank(1000, FLASH_16B);

  /* Load user data from flash banks */
  readFromFlashBank(functionLUT, 1000, &userBank1);

  /* Create memory pools */
  osPoolDef(paramPool, 8, TaskParameter);
  paramPoolId = osPoolCreate(osPool(paramPool));

  /* Create message queues */
  osMessageQDef(adcQ, 16, uint32_t);
  adcMessageQ = osMessageCreate(osMessageQ(adcQ), NULL);

  /* Create the thread(s) */
  TaskParameter* adcParameter = (TaskParameter*)osPoolAlloc(paramPoolId);
  adcParameter->messageQ = adcMessageQ;
  osThreadDef(adcThread, adcTask, osPriorityHigh, 0, 128);
  adcTaskHandle = osThreadCreate(osThread(adcThread), adcParameter);

  TaskParameter* uartParameter = (TaskParameter*)osPoolAlloc(paramPoolId);
  uartParameter->messageQ = adcMessageQ;
  osThreadDef(usartThread, usartTask, osPriorityRealtime, 0, 128);
  usartTaskHandle = osThreadCreate(osThread(usartThread), uartParameter);

  TaskParameter* userButtonParameter = (TaskParameter*)osPoolAlloc(paramPoolId);
  userButtonParameter->messageQ = adcMessageQ;
  osThreadDef(userButtonThread, userButtonTask, osPriorityHigh, 0, 128);
  userButtonTaskHandle = osThreadCreate(osThread(userButtonThread), userButtonParameter);

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  while(1);

  return EXIT_SUCCESS;
}

uint32_t brakeMaxValue = 1000;
uint32_t brakeMinValue = 0;

static float functionOff(const float x)
{
  return 0;
}

static float functionOn(const float x)
{
  return brakeMaxValue;
}

static float functionLinear(const float x)
{
  return x;
}

static float functionToggle(const float x)
{
  return (((int) (x + 0.5)) % 2) ? brakeMinValue : brakeMaxValue;
}

static float functionExp(const float x, const uint8_t p)
{
  //return brakeMaxValue - (((powf(brakeMaxValue - x, p)) / powf(brakeMaxValue, p)) * brakeMaxValue);
  return brakeMaxValue - powf(brakeMaxValue - x, p);
}

static float functionExp2(const float x)
{
  return functionExp(x, 2);
}

static float functionExp3(const float x)
{
  return functionExp(x, 2);
}

static float functionExp4(const float x)
{
  return functionExp(x, 2);
}

static float functionUser(const float x, const uint16_t* userData)
{
  return userData[(uint16_t)(x + 0.5f)];
}

static uint16_t userData1[1000] = {0};
static uint16_t userData2[1000] = {0};
static uint16_t userData3[1000] = {0};

static float functionUser1(const float x)
{
  return functionUser(x, userData1);
}

static float functionUser2(const float x)
{
  return functionUser(x, userData2);
}

static float functionUser3(const float x)
{
  return functionUser(x, userData3);
}

float (*brakeFunctions[BF_NR_ITEMS+1])(float) =
{
  [BF_OFF] =    functionOff,
  [BF_ON] =     functionOn,

  [BF_LINEAR] = functionLinear,

  [BF_EXP2] =   functionExp2,
  [BF_EXP3] =   functionExp3,
  [BF_EXP4] =   functionExp4,

  [BF_TOGGLE] = functionToggle,

  [BF_USER1] =  functionUser1,
  [BF_USER2] =  functionUser2,
  [BF_USER3] =  functionUser3,

  [BF_NR_ITEMS] = functionOff
};

BrakeFunction brakeFunction = BF_OFF;


void adcTask(void const* argument)
{
  const TaskParameter* parameter = (TaskParameter*)argument;
  uint32_t adcRaw;
  float adcValue;
  register uint32_t dutyCycle;
  while (1)
  {
    trace_printf("ADC\n");
    adcRaw = getAdc();
    adcValue = brakeMaxValue - ((((adcRaw + 1) / 4096.0f) * brakeMaxValue) * 2);
    //functions[brakeFunction](adcValue) + 0.5f;
    dutyCycle = (*brakeFunctions[brakeFunction])(adcValue) + 0.5f;
    trace_printf("function: %i\n", brakeFunction);
    setPwm(dutyCycle);
    osMessagePut(parameter->messageQ, dutyCycle, 0);
//    trace_printf ("ADC raw: %i\nADC input: x = %i\nDutyCycle output: y = %i\n",
//                  adcRaw, (int) adcValue, dutyCycle);
  }
}

void usartTask(void const* argument)
{
  const TaskParameter* parameter = (TaskParameter*)argument;
  while (1)
  {
    trace_printf("UART\n");
    osEvent data = osMessageGet(parameter->messageQ, 1000);
    trace_printf("%i\n", data.value.v);
    uartSend(data.value.p, 4);
  }
}

void userButtonTask(void const* argument)
{
  const TaskParameter* parameter = (TaskParameter*)argument;
  while (1)
  {
    trace_printf("BUTTON\n");

    if (isButtonOnBoardPressed())
    {
      ledOnBoardOn();
    }
    else
    {
      ledOnBoardOff();
    }
    osThreadYield();
  }
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
static void Error_Handler(void)
{
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT

/**
 * @brief Reports the name of the source file and the source line number
 * where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
  trace_printf("Wrong parameters value: file %s on line %d\r\n", file, line);
}

#endif

