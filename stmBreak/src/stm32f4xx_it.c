/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "cmsis_device.h"
#include "cmsis_os.h"
#include "diag/Trace.h"
#include <math.h>

/* External variables --------------------------------------------------------*/

extern TIM_HandleTypeDef htim7;
extern UART_HandleTypeDef huart1;

/* ****************************************************************************/
/*            Cortex-M4 Processor Interruption and Exception Handlers         */
/* ****************************************************************************/

/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void)
{
  /* NonMaskableInt_IRQn */
}

/**
 * @brief This function handles Hard fault interrupt.
 */
//void HardFault_Handler(void)
//{
//  __asm volatile (
//    " movs r0,#4       \n"
//    " movs r1, lr      \n"
//    " tst r0, r1       \n"
//    " beq _MSP         \n"
//    " mrs r0, psp      \n"
//    " b _HALT          \n"
//  "_MSP:               \n"
//    " mrs r0, msp      \n"
//  "_HALT:              \n"
//    " ldr r1,[r0,#20]  \n"
//    " bkpt #0          \n"
//  );
//  /* HardFault_IRQn */
//  while (1)
//  {
//  }
//}

/* The fault handler implementation calls a function called
 prvGetRegistersFromStack(). */
void HardFault_Handler(void) __attribute__( ( naked ) );
void HardFault_Handler(void)
{
  __asm volatile
  (
    " tst lr, #4                                                \n"
    " ite eq                                                    \n"
    " mrseq r0, msp                                             \n"
    " mrsne r0, psp                                             \n"
    " ldr r1, [r0, #24]                                         \n"
    " ldr r2, handler2_address_const                            \n"
    " bx r2                                                     \n"
    " handler2_address_const: .word prvGetRegistersFromStack    \n"
  );
}

void prvGetRegistersFromStack(uint32_t *pulFaultStackAddress)
{
  /* These are volatile to try and prevent the compiler/linker optimizing them
   away as the variables never actually get used.  If the debugger won't show the
   values of the variables, make them global my moving their declaration outside
   of this function. */
  volatile uint32_t r0;
  volatile uint32_t r1;
  volatile uint32_t r2;
  volatile uint32_t r3;
  volatile uint32_t r12;
  volatile uint32_t lr; /* Link register. */
  volatile uint32_t pc; /* Program counter. */
  volatile uint32_t psr;/* Program status register. */

  r0  = pulFaultStackAddress[0];
  r1  = pulFaultStackAddress[1];
  r2  = pulFaultStackAddress[2];
  r3  = pulFaultStackAddress[3];

  r12 = pulFaultStackAddress[4];
  lr  = pulFaultStackAddress[5];
  pc  = pulFaultStackAddress[6];
  psr = pulFaultStackAddress[7];

  /* When the following line is hit, the variables contain the register values. */
  while(1);
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void)
{
  /* MemoryManagement_IRQn */
  while (1);
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault.
 */
void BusFault_Handler(void)
{
  /* BusFault_IRQn */
  while (1);
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void)
{
  /* UsageFault_IRQn */
  while (1);
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void)
{
  /* DebugMonitor_IRQn */
  asm("nop");
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void)
{
  /* SysTick_IRQn */
  osSystickHandler();
}

/* ****************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/* ****************************************************************************/

/**
 * @brief This function handles TIM2 global interrupt.
 */
void TIM7_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim7);
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM2 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM7)
  {
    HAL_IncTick();
  }
}

void EXTI2_IRQHandler(void)
{
  HAL_Delay(10);
  while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2));
  //osSemaphoreRelease(semaphoreId);
  //portEND_SWITCHING_ISR(&taskHasWoken)
  HAL_StatusTypeDef rx_code = 0;
  HAL_StatusTypeDef tx_code = 0;
//    while (1)
  {
    unsigned char* sendByte = "MOTHERFUCKER\0";
    tx_code = HAL_UART_Transmit(&huart1, sendByte, 13, 10000);
    unsigned char byte = 0;
    rx_code = HAL_UART_Receive(&huart1, &byte, 1, 0);
    unsigned char dasByte = byte;
    trace_printf("RX: %s\n", dasByte);
//        trace_printf("%i %i", tx_code, rx_code);
    asm("nop");
  }
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

extern uint32_t brakeMaxValue;
extern uint32_t brakeMinValue;
extern BrakeFunction brakeFunction;

void EXTI3_IRQHandler(void)
{
  HAL_Delay(50);
  while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_3));
  HAL_Delay(50);

  if (brakeFunction % BF_NR_ITEMS == 0)
  {
    brakeFunction = BF_NR_ITEMS;
  }
  brakeFunction--;

  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

void EXTI4_IRQHandler(void)
{
  HAL_Delay(50);
  while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4));
  HAL_Delay(50);

  brakeFunction++;
  if (brakeFunction % BF_NR_ITEMS == 0)
  {
    brakeFunction = 0;
  }


  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

void EXTI9_5_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(
      GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  trace_printf("Interrupt from EXTI%i\n", (int) log2(GPIO_Pin));
}

