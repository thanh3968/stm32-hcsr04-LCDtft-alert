#ifndef LCD_TFT2_H
#define LCD_TFT2_H

#include "stm32f4xx_hal.h"
#include "hcsr04.h"

#define LCD_WIDTH  240
#define LCD_HEIGHT 240

#define LCD_CS_Pin        GPIO_PIN_12
#define LCD_CS_GPIO_Port  GPIOB
#define LCD_DC_Pin        GPIO_PIN_1
#define LCD_DC_GPIO_Port  GPIOB
#define LCD_RST_Pin       GPIO_PIN_2
#define LCD_RST_GPIO_Port GPIOB

#define LCD_RGB565(r, g, b) (uint16_t)((((r) & 0xF8U) << 8) | (((g) & 0xFCU) << 3) | ((b) >> 3))

#define LCD_COLOR_BG         LCD_RGB565(8, 12, 20)
#define LCD_COLOR_PANEL      LCD_RGB565(22, 30, 45)
#define LCD_COLOR_PANEL_DARK LCD_RGB565(13, 19, 31)
#define LCD_COLOR_BORDER     LCD_RGB565(64, 120, 160)
#define LCD_COLOR_BORDER_DIM LCD_RGB565(22, 52, 75)
#define LCD_COLOR_TEXT       LCD_RGB565(235, 244, 255)
#define LCD_COLOR_MUTED      LCD_RGB565(132, 150, 170)
#define LCD_COLOR_VALUE      LCD_RGB565(248, 252, 255)
#define LCD_COLOR_RED        LCD_RGB565(255, 70, 80)
#define LCD_COLOR_ORANGE     LCD_RGB565(255, 176, 48)
#define LCD_COLOR_GREEN      LCD_RGB565(62, 220, 135)

void LCD_Init(SPI_HandleTypeDef *hspi);
void LCD_ShowBootTest(void);
void LCD_DrawDistanceUI(void);
void LCD_UpdateDistanceUI(float distance_cm, HCSR04_Status status);

#endif
