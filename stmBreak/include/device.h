#ifndef __DEVICE_H
#define __DEVICE_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

void setupDevice(void);

void setPwm(uint32_t dutyCycle);
uint32_t getAdc(void);
void uartSend(void* data, uint16_t size);
uint8_t isButtonOnBoardPressed(void);
void ledOnBoardOn(void);
void ledOnBoardOff(void);

#ifdef __cplusplus
 }
#endif

#endif /* __DEVICE_H */
