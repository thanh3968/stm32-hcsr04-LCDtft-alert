#include "dy_sv17f.h"

static USART_TypeDef *dy_uart;

static void uart_send(uint8_t data)
{
  while ((dy_uart->SR & USART_SR_TXE) == 0)
  {
  }

  dy_uart->DR = data;
}

static void send_command(uint8_t cmd, const uint8_t *data, uint8_t len)
{
  uint8_t packet[8];
  uint8_t sum = 0;
  uint8_t index = 0;

  packet[index++] = 0xAA;
  packet[index++] = cmd;
  packet[index++] = len;

  for (uint8_t i = 0; i < len; i++)
  {
    packet[index++] = data[i];
  }

  for (uint8_t i = 0; i < index; i++)
  {
    sum += packet[i];
  }

  packet[index++] = sum;

  for (uint8_t i = 0; i < index; i++)
  {
    uart_send(packet[i]);
  }

  while ((dy_uart->SR & USART_SR_TC) == 0)
  {
  }
}

static void select_flash(void)
{
  uint8_t data = 0x02;
  send_command(0x0B, &data, 1);
}

static void set_volume(uint8_t volume)
{
  if (volume > 30)
  {
    volume = 30;
  }

  send_command(0x13, &volume, 1);
}

void DY_SV17F_Init(USART_TypeDef *uart)
{
  dy_uart = uart;
  HAL_Delay(500);
  select_flash();
  HAL_Delay(100);
  set_volume(30);
  HAL_Delay(100);
}

void DY_SV17F_PlayTrack(uint16_t track)
{
  uint8_t data[2];

  data[0] = track >> 8;
  data[1] = track & 0xFF;
  send_command(0x07, data, 2);
}
