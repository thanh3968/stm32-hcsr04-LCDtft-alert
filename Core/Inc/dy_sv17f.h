#ifndef DY_SV17F_H
#define DY_SV17F_H

#include "stm32f4xx_hal.h"

void DY_SV17F_Init(USART_TypeDef *uart);
void DY_SV17F_PlayTrack(uint16_t track);

#endif
