#include "main.h"
#include "lcd_tft2.h"
#include "hcsr04.h"
#include "dy_sv17f.h"

#define MOTOR_Pin       GPIO_PIN_0
#define MOTOR_GPIO_Port GPIOB
#define MOTOR_ON        GPIO_PIN_RESET
#define MOTOR_OFF       GPIO_PIN_SET

#define DISTANCE_DANGER_CM 10.0f
#define DISTANCE_NEAR_CM   50.0f
#define SENSOR_PERIOD_TICK 13
#define SOUND_REPEAT_MS    6000
#define SOUND_TRACK        1
#define MOTOR_KEEP_ON      255
#define LEVEL_SAFE         0
#define LEVEL_NEAR         1
#define LEVEL_DANGER       2

SPI_HandleTypeDef hspi2;

static volatile uint8_t read_sensor_flag = 0;
static volatile uint8_t motor_time = 0;
static volatile uint8_t timer_count = 0;

static void Clock_Init(void);
static void SPI2_Init(void);
static void UART2_Init(void);
static void TIM4_Init(void);
static void Motor_Init(void);
static void Motor_Start(uint8_t time_10ms);
static void Check_Distance(float distance, HCSR04_Status status);

int main(void)
{
  HAL_Init();
  Clock_Init();
  SPI2_Init();
  UART2_Init();
  TIM4_Init();
  Motor_Init();

  LCD_Init(&hspi2);
  LCD_ShowBootTest();
  HCSR04_Init();
  DY_SV17F_Init(USART2);
  LCD_DrawDistanceUI();

  while (1)
  {
    if (read_sensor_flag)
    {
      read_sensor_flag = 0;

      float distance = 0.0f;
      HCSR04_Status status = HCSR04_ReadDistanceCm(&distance);

      LCD_UpdateDistanceUI(distance, status);
      Check_Distance(distance, status);
    }
  }
}

static void Check_Distance(float distance, HCSR04_Status status)
{
  static uint8_t last_level = LEVEL_SAFE;
  static uint32_t last_sound_tick = 0;
  uint8_t level = LEVEL_SAFE;
  uint32_t now = HAL_GetTick();

  if (status == HCSR04_STATUS_OK)
  {
    if (distance < DISTANCE_DANGER_CM)
    {
      level = LEVEL_DANGER;
    }
    else if (distance <= DISTANCE_NEAR_CM)
    {
      level = LEVEL_NEAR;
    }
  }

  if (level == LEVEL_DANGER)
  {
    Motor_Start(MOTOR_KEEP_ON);
  }
  else
  {
    Motor_Start(0);
  }

  if (level > 0 && level != last_level &&
      (last_sound_tick == 0 || now - last_sound_tick > SOUND_REPEAT_MS))
  {
    DY_SV17F_PlayTrack(SOUND_TRACK);
    last_sound_tick = now;
  }

  last_level = level;
}

void TIM4_IRQHandler(void)
{
  if ((TIM4->SR & TIM_SR_UIF) != 0)
  {
    TIM4->SR &= ~TIM_SR_UIF;

    timer_count++;

    if (timer_count >= SENSOR_PERIOD_TICK)
    {
      timer_count = 0;
      read_sensor_flag = 1;
    }

    if (motor_time > 0)
    {
      HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, MOTOR_ON);
      motor_time--;
    }
    else
    {
      HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, MOTOR_OFF);
    }
  }
}

static void Motor_Start(uint8_t time_10ms)
{
  motor_time = time_10ms;
}

static void Motor_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();

  gpio.Pin = MOTOR_Pin;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(MOTOR_GPIO_Port, &gpio);

  HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, MOTOR_OFF);
}

static void TIM4_Init(void)
{
  __HAL_RCC_TIM4_CLK_ENABLE();

  TIM4->PSC = 8399;
  TIM4->ARR = 99;
  TIM4->CNT = 0;
  TIM4->DIER |= TIM_DIER_UIE;
  TIM4->CR1 |= TIM_CR1_CEN;

  HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM4_IRQn);
}

static void UART2_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART2_CLK_ENABLE();

  gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &gpio);

  USART2->CR1 = 0;
  USART2->CR2 = 0;
  USART2->CR3 = 0;
  USART2->BRR = (HAL_RCC_GetPCLK1Freq() + 4800) / 9600;
  USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static void SPI2_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_SPI2_CLK_ENABLE();

  gpio.Pin = GPIO_PIN_13 | GPIO_PIN_15;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &gpio);

  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;

  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void Clock_Init(void)
{
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef clk = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  osc.HSEState = RCC_HSE_ON;
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  osc.PLL.PLLM = 8;
  osc.PLL.PLLN = 336;
  osc.PLL.PLLP = RCC_PLLP_DIV2;
  osc.PLL.PLLQ = 7;

  if (HAL_RCC_OscConfig(&osc) != HAL_OK)
  {
    Error_Handler();
  }

  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV4;
  clk.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
