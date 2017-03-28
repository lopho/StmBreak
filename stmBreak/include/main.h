#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

typedef enum BrakeFunction
{
  BF_OFF,
  BF_ON,

  BF_LINEAR,

  BF_EXP2,
  BF_EXP3,
  BF_EXP4,

  BF_TOGGLE,

  BF_USER1,
  BF_USER2,
  BF_USER3,

  BF_NR_ITEMS
} BrakeFunction;

#ifdef __cplusplus
 }
#endif

#endif /* __MAIN_H */
