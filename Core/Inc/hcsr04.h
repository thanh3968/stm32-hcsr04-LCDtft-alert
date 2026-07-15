#ifndef HCSR04_H
#define HCSR04_H

#include "stm32f4xx_hal.h"

#define HCSR04_TRIG_Pin        GPIO_PIN_6
#define HCSR04_TRIG_GPIO_Port  GPIOB
#define HCSR04_ECHO_Pin        GPIO_PIN_7
#define HCSR04_ECHO_GPIO_Port  GPIOB

typedef enum
{
  HCSR04_STATUS_OK = 0,
  HCSR04_STATUS_TIMEOUT
} HCSR04_Status;

void HCSR04_Init(void);
HCSR04_Status HCSR04_ReadDistanceCm(float *distance_cm);

#endif
