#include "hcsr04.h"

#define HCSR04_WAIT_RISE_TIMEOUT_US 60000U
#define HCSR04_ECHO_TIMEOUT_US      38000U

static void delay_us(uint32_t us)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000U);

  while ((DWT->CYCCNT - start) < ticks)
  {
  }
}

static uint32_t micros(void)
{
  return DWT->CYCCNT / (HAL_RCC_GetHCLKFreq() / 1000000U);
}

void HCSR04_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(HCSR04_TRIG_GPIO_Port, HCSR04_TRIG_Pin, GPIO_PIN_RESET);

  gpio.Pin = HCSR04_TRIG_Pin;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(HCSR04_TRIG_GPIO_Port, &gpio);

  gpio.Pin = HCSR04_ECHO_Pin;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(HCSR04_ECHO_GPIO_Port, &gpio);

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  HAL_Delay(50);
}

HCSR04_Status HCSR04_ReadDistanceCm(float *distance_cm)
{
  uint32_t start_us;
  uint32_t echo_start;
  uint32_t echo_width;

  HAL_GPIO_WritePin(HCSR04_TRIG_GPIO_Port, HCSR04_TRIG_Pin, GPIO_PIN_RESET);
  delay_us(2);
  HAL_GPIO_WritePin(HCSR04_TRIG_GPIO_Port, HCSR04_TRIG_Pin, GPIO_PIN_SET);
  delay_us(10);
  HAL_GPIO_WritePin(HCSR04_TRIG_GPIO_Port, HCSR04_TRIG_Pin, GPIO_PIN_RESET);

  start_us = micros();
  while (HAL_GPIO_ReadPin(HCSR04_ECHO_GPIO_Port, HCSR04_ECHO_Pin) == GPIO_PIN_RESET)
  {
    if ((micros() - start_us) > HCSR04_WAIT_RISE_TIMEOUT_US)
    {
      return HCSR04_STATUS_TIMEOUT;
    }
  }

  echo_start = micros();
  while (HAL_GPIO_ReadPin(HCSR04_ECHO_GPIO_Port, HCSR04_ECHO_Pin) == GPIO_PIN_SET)
  {
    if ((micros() - echo_start) > HCSR04_ECHO_TIMEOUT_US)
    {
      return HCSR04_STATUS_TIMEOUT;
    }
  }

  echo_width = micros() - echo_start;
  if (distance_cm != 0)
  {
    *distance_cm = (float)echo_width / 58.0f;
  }

  return HCSR04_STATUS_OK;
}
