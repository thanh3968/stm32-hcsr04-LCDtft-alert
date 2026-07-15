#include "lcd_tft2.h"
#include <stddef.h>
#include <stdio.h>

#define GC9A01_SWRESET 0x01
#define GC9A01_NORON   0x13
#define GC9A01_SLPOUT  0x11
#define GC9A01_DISPON  0x29
#define GC9A01_CASET   0x2A
#define GC9A01_RASET   0x2B
#define GC9A01_RAMWR   0x2C
#define GC9A01_MADCTL  0x36
#define GC9A01_COLMOD  0x3A
#define GC9A01_INVON   0x21

static SPI_HandleTypeDef *lcd_spi;

static void fill_screen(uint16_t color);
static void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
static void fill_round_rect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
static void draw_pixel(int16_t x, int16_t y, uint16_t color);
static void draw_circle(int16_t x, int16_t y, int16_t r, uint16_t color);
static void draw_string(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale);
static void draw_string_center(int16_t center_x, int16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale);
static uint16_t text_width(const char *str, uint8_t scale);

static const uint8_t font5x7[][5] = {
  {0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x5F,0x00,0x00},
  {0x00,0x07,0x00,0x07,0x00},
  {0x14,0x7F,0x14,0x7F,0x14},
  {0x24,0x2A,0x7F,0x2A,0x12},
  {0x23,0x13,0x08,0x64,0x62},
  {0x36,0x49,0x55,0x22,0x50},
  {0x00,0x05,0x03,0x00,0x00},
  {0x00,0x1C,0x22,0x41,0x00},
  {0x00,0x41,0x22,0x1C,0x00},
  {0x14,0x08,0x3E,0x08,0x14},
  {0x08,0x08,0x3E,0x08,0x08},
  {0x00,0x50,0x30,0x00,0x00},
  {0x08,0x08,0x08,0x08,0x08},
  {0x00,0x60,0x60,0x00,0x00},
  {0x20,0x10,0x08,0x04,0x02},
  {0x3E,0x51,0x49,0x45,0x3E},
  {0x00,0x42,0x7F,0x40,0x00},
  {0x42,0x61,0x51,0x49,0x46},
  {0x21,0x41,0x45,0x4B,0x31},
  {0x18,0x14,0x12,0x7F,0x10},
  {0x27,0x45,0x45,0x45,0x39},
  {0x3C,0x4A,0x49,0x49,0x30},
  {0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36},
  {0x06,0x49,0x49,0x29,0x1E},
  {0x00,0x36,0x36,0x00,0x00},
  {0x00,0x56,0x36,0x00,0x00},
  {0x08,0x14,0x22,0x41,0x00},
  {0x14,0x14,0x14,0x14,0x14},
  {0x00,0x41,0x22,0x14,0x08},
  {0x02,0x01,0x51,0x09,0x06},
  {0x32,0x49,0x79,0x41,0x3E},
  {0x7E,0x11,0x11,0x11,0x7E},
  {0x7F,0x49,0x49,0x49,0x36},
  {0x3E,0x41,0x41,0x41,0x22},
  {0x7F,0x41,0x41,0x22,0x1C},
  {0x7F,0x49,0x49,0x49,0x41},
  {0x7F,0x09,0x09,0x09,0x01},
  {0x3E,0x41,0x49,0x49,0x7A},
  {0x7F,0x08,0x08,0x08,0x7F},
  {0x00,0x41,0x7F,0x41,0x00},
  {0x20,0x40,0x41,0x3F,0x01},
  {0x7F,0x08,0x14,0x22,0x41},
  {0x7F,0x40,0x40,0x40,0x40},
  {0x7F,0x02,0x0C,0x02,0x7F},
  {0x7F,0x04,0x08,0x10,0x7F},
  {0x3E,0x41,0x41,0x41,0x3E},
  {0x7F,0x09,0x09,0x09,0x06},
  {0x3E,0x41,0x51,0x21,0x5E},
  {0x7F,0x09,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31},
  {0x01,0x01,0x7F,0x01,0x01},
  {0x3F,0x40,0x40,0x40,0x3F},
  {0x1F,0x20,0x40,0x20,0x1F},
  {0x3F,0x40,0x38,0x40,0x3F},
  {0x63,0x14,0x08,0x14,0x63},
  {0x07,0x08,0x70,0x08,0x07},
  {0x61,0x51,0x49,0x45,0x43},
};

static void cs_low(void)  { HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET); }
static void cs_high(void) { HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET); }
static void dc_cmd(void)  { HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET); }
static void dc_data(void) { HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET); }

static void write_cmd(uint8_t cmd)
{
  dc_cmd();
  cs_low();
  HAL_SPI_Transmit(lcd_spi, &cmd, 1, HAL_MAX_DELAY);
  cs_high();
}

static void write_data(const uint8_t *data, uint16_t len)
{
  dc_data();
  cs_low();
  HAL_SPI_Transmit(lcd_spi, (uint8_t *)data, len, HAL_MAX_DELAY);
  cs_high();
}

static void write_data8(uint8_t data)
{
  write_data(&data, 1);
}

static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  uint8_t data[4];

  write_cmd(GC9A01_CASET);
  data[0] = x0 >> 8;
  data[1] = x0 & 0xFF;
  data[2] = x1 >> 8;
  data[3] = x1 & 0xFF;
  write_data(data, 4);

  write_cmd(GC9A01_RASET);
  data[0] = y0 >> 8;
  data[1] = y0 & 0xFF;
  data[2] = y1 >> 8;
  data[3] = y1 & 0xFF;
  write_data(data, 4);

  write_cmd(GC9A01_RAMWR);
}

static void push_color_repeat(uint16_t color, uint32_t count)
{
  uint8_t buf[128];
  uint8_t hi = color >> 8;
  uint8_t lo = color & 0xFF;

  for (uint16_t i = 0; i < sizeof(buf); i += 2)
  {
    buf[i] = hi;
    buf[i + 1] = lo;
  }

  dc_data();
  cs_low();
  while (count > 0U)
  {
    uint16_t pixels = count > (sizeof(buf) / 2U) ? (sizeof(buf) / 2U) : (uint16_t)count;
    HAL_SPI_Transmit(lcd_spi, buf, pixels * 2U, HAL_MAX_DELAY);
    count -= pixels;
  }
  cs_high();
}

void LCD_Init(SPI_HandleTypeDef *hspi)
{
  GPIO_InitTypeDef gpio = {0};

  lcd_spi = hspi;

  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin | LCD_DC_Pin | LCD_RST_Pin, GPIO_PIN_SET);

  gpio.Pin = LCD_CS_Pin | LCD_DC_Pin | LCD_RST_Pin;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &gpio);

  cs_high();
  dc_data();
  HAL_Delay(10);

  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(50);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(150);

  write_cmd(GC9A01_SWRESET);
  HAL_Delay(150);

  write_cmd(0xEF);
  write_cmd(0xEB); write_data8(0x14);
  write_cmd(0xFE);
  write_cmd(0xEF);
  write_cmd(0xEB); write_data8(0x14);
  write_cmd(0x84); write_data8(0x40);
  write_cmd(0x85); write_data8(0xFF);
  write_cmd(0x86); write_data8(0xFF);
  write_cmd(0x87); write_data8(0xFF);
  write_cmd(0x88); write_data8(0x0A);
  write_cmd(0x89); write_data8(0x21);
  write_cmd(0x8A); write_data8(0x00);
  write_cmd(0x8B); write_data8(0x80);
  write_cmd(0x8C); write_data8(0x01);
  write_cmd(0x8D); write_data8(0x01);
  write_cmd(0x8E); write_data8(0xFF);
  write_cmd(0x8F); write_data8(0xFF);

  write_cmd(GC9A01_MADCTL);
  write_data8(0x48);
  write_cmd(GC9A01_COLMOD);
  write_data8(0x05);

  write_cmd(0x90);
  {
    uint8_t seq[] = {0x08, 0x08, 0x08, 0x08};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0xBD); write_data8(0x06);
  write_cmd(0xBC); write_data8(0x00);
  write_cmd(0xFF);
  {
    uint8_t seq[] = {0x60, 0x01, 0x04};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0xC3); write_data8(0x13);
  write_cmd(0xC4); write_data8(0x13);
  write_cmd(0xC9); write_data8(0x22);
  write_cmd(0xBE); write_data8(0x11);
  write_cmd(0xE1);
  {
    uint8_t seq[] = {0x10, 0x0E};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0xDF);
  {
    uint8_t seq[] = {0x21, 0x0C, 0x02};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0xF0);
  {
    uint8_t seq[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0xF1);
  {
    uint8_t seq[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0xF2);
  {
    uint8_t seq[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0xF3);
  {
    uint8_t seq[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0xED);
  {
    uint8_t seq[] = {0x1B, 0x0B};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0xAE); write_data8(0x77);
  write_cmd(0xCD); write_data8(0x63);
  write_cmd(0x70);
  {
    uint8_t seq[] = {0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0xE8);
  {
    uint8_t seq[] = {0x34, 0x00};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0x62);
  {
    uint8_t seq[] = {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0x63);
  {
    uint8_t seq[] = {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0x64);
  {
    uint8_t seq[] = {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0x66);
  {
    uint8_t seq[] = {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0x67);
  {
    uint8_t seq[] = {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0x74);
  {
    uint8_t seq[] = {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00};
    write_data(seq, sizeof(seq));
  }
  write_cmd(0x98);
  {
    uint8_t seq[] = {0x3E, 0x07};
    write_data(seq, sizeof(seq));
  }

  write_cmd(GC9A01_INVON);
  write_cmd(GC9A01_SLPOUT);
  HAL_Delay(150);
  write_cmd(GC9A01_NORON);
  HAL_Delay(20);
  write_cmd(GC9A01_DISPON);
  HAL_Delay(100);
}

static void fill_screen(uint16_t color)
{
  fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

static void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  if (x >= LCD_WIDTH || y >= LCD_HEIGHT || w <= 0 || h <= 0)
  {
    return;
  }
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if ((x + w) > LCD_WIDTH) { w = LCD_WIDTH - x; }
  if ((y + h) > LCD_HEIGHT) { h = LCD_HEIGHT - y; }
  if (w <= 0 || h <= 0)
  {
    return;
  }

  set_window((uint16_t)x, (uint16_t)y, (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));
  push_color_repeat(color, (uint32_t)w * (uint32_t)h);
}

static void draw_pixel(int16_t x, int16_t y, uint16_t color)
{
  if (x < 0 || y < 0 || x >= LCD_WIDTH || y >= LCD_HEIGHT)
  {
    return;
  }
  set_window((uint16_t)x, (uint16_t)y, (uint16_t)x, (uint16_t)y);
  push_color_repeat(color, 1);
}

static void fill_round_rect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
  if (r <= 0)
  {
    fill_rect(x, y, w, h, color);
    return;
  }

  fill_rect(x + r, y, w - 2 * r, h, color);
  fill_rect(x, y + r, r, h - 2 * r, color);
  fill_rect(x + w - r, y + r, r, h - 2 * r, color);

  int16_t rr = r * r;
  for (int16_t dy = 0; dy < r; dy++)
  {
    for (int16_t dx = 0; dx < r; dx++)
    {
      int16_t cx = r - 1 - dx;
      int16_t cy = r - 1 - dy;
      if ((cx * cx + cy * cy) <= rr)
      {
        draw_pixel(x + dx, y + dy, color);
        draw_pixel(x + w - 1 - dx, y + dy, color);
        draw_pixel(x + dx, y + h - 1 - dy, color);
        draw_pixel(x + w - 1 - dx, y + h - 1 - dy, color);
      }
    }
  }
}

static void draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
  int16_t x = 0;
  int16_t y = r;
  int16_t d = 3 - 2 * r;

  while (y >= x)
  {
    draw_pixel(x0 + x, y0 + y, color);
    draw_pixel(x0 - x, y0 + y, color);
    draw_pixel(x0 + x, y0 - y, color);
    draw_pixel(x0 - x, y0 - y, color);
    draw_pixel(x0 + y, y0 + x, color);
    draw_pixel(x0 - y, y0 + x, color);
    draw_pixel(x0 + y, y0 - x, color);
    draw_pixel(x0 - y, y0 - x, color);

    x++;
    if (d > 0)
    {
      y--;
      d += 4 * (x - y) + 10;
    }
    else
    {
      d += 4 * x + 6;
    }
  }
}

static const uint8_t *glyph_for(char c)
{
  if (c >= 'a' && c <= 'z')
  {
    c = (char)(c - 'a' + 'A');
  }
  if (c < ' ' || c > 'Z')
  {
    c = '?';
  }
  return font5x7[(uint8_t)c - 32U];
}

static void draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t scale)
{
  const uint8_t *glyph = glyph_for(c);

  for (uint8_t col = 0; col < 6; col++)
  {
    uint8_t bits = col < 5 ? glyph[col] : 0x00;
    for (uint8_t row = 0; row < 8; row++)
    {
      uint16_t pixel_color = (bits & (1U << row)) ? color : bg;
      fill_rect(x + col * scale, y + row * scale, scale, scale, pixel_color);
    }
  }
}

static void draw_string(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale)
{
  if (scale == 0U)
  {
    scale = 1U;
  }

  while (str != NULL && *str != '\0')
  {
    draw_char(x, y, *str, color, bg, scale);
    x += 6 * scale;
    str++;
  }
}

static uint16_t text_width(const char *str, uint8_t scale)
{
  uint16_t len = 0;
  while (str != NULL && str[len] != '\0')
  {
    len++;
  }
  return (uint16_t)(len * 6U * scale);
}

static void draw_string_center(int16_t center_x, int16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale)
{
  uint16_t width = text_width(str, scale);
  draw_string((int16_t)(center_x - (int16_t)(width / 2U)), y, str, color, bg, scale);
}

void LCD_ShowBootTest(void)
{
  fill_screen(LCD_COLOR_RED);
  HAL_Delay(120);
  fill_screen(LCD_COLOR_GREEN);
  HAL_Delay(120);
  fill_screen(LCD_COLOR_BORDER);
  HAL_Delay(120);
}

void LCD_DrawDistanceUI(void)
{
  fill_screen(LCD_COLOR_BG);

  draw_circle(120, 120, 117, LCD_COLOR_BORDER);
  draw_circle(120, 120, 116, LCD_COLOR_BORDER_DIM);
  fill_round_rect(35, 13, 170, 36, 10, LCD_COLOR_PANEL);
  draw_string_center(120, 19, "DO KHOANG", LCD_COLOR_TEXT, LCD_COLOR_PANEL, 1);
  draw_string_center(120, 31, "CACH", LCD_COLOR_TEXT, LCD_COLOR_PANEL, 1);

  fill_round_rect(18, 58, 204, 95, 16, LCD_COLOR_PANEL_DARK);
  draw_string(77, 65, "DISTANCE", LCD_COLOR_MUTED, LCD_COLOR_PANEL_DARK, 2);

  fill_round_rect(23, 164, 194, 32, 10, LCD_COLOR_PANEL);
  fill_round_rect(35, 201, 170, 24, 8, LCD_COLOR_PANEL_DARK);
}

void LCD_UpdateDistanceUI(float distance_cm, HCSR04_Status status)
{
  static int last_status = -1;
  static int last_distance_x10 = -100000;

  char text[24];
  uint16_t alert_color;
  const char *alert_text;

  if (status == HCSR04_STATUS_OK)
  {
    int distance_x10 = (int)(distance_cm * 10.0f + 0.5f);

    if (distance_x10 != last_distance_x10 || last_status != status)
    {
      fill_round_rect(27, 83, 186, 57, 12, LCD_COLOR_PANEL_DARK);
      snprintf(text, sizeof(text), "%d.%d cm", distance_x10 / 10, distance_x10 % 10);
      draw_string_center(120, 96, text, LCD_COLOR_VALUE, LCD_COLOR_PANEL_DARK, 4);
      last_distance_x10 = distance_x10;
    }

    if (distance_cm < 10.0f)
    {
      alert_color = LCD_COLOR_RED;
      alert_text = "CANH BAO GAN";
    }
    else if (distance_cm <= 50.0f)
    {
      alert_color = LCD_COLOR_ORANGE;
      alert_text = "KHOANG CACH GAN";
    }
    else
    {
      alert_color = LCD_COLOR_GREEN;
      alert_text = "AN TOAN";
    }
  }
  else
  {
    alert_color = LCD_COLOR_RED;
    alert_text = "MAT TIN HIEU";

    if (last_status != status)
    {
      fill_round_rect(27, 83, 186, 57, 12, LCD_COLOR_PANEL_DARK);
      draw_string_center(120, 96, "--.- cm", LCD_COLOR_MUTED, LCD_COLOR_PANEL_DARK, 4);
      last_distance_x10 = -100000;
    }
  }

  fill_round_rect(23, 164, 194, 32, 10, LCD_COLOR_PANEL);
  draw_string_center(120, 174, alert_text, alert_color, LCD_COLOR_PANEL, 2);

  fill_round_rect(35, 201, 170, 24, 8, LCD_COLOR_PANEL_DARK);
  draw_string_center(120, 209,
                         status == HCSR04_STATUS_OK ? "Sensor OK" : "Sensor Timeout",
                         status == HCSR04_STATUS_OK ? LCD_COLOR_GREEN : LCD_COLOR_RED,
                         LCD_COLOR_PANEL_DARK,
                         1);
  last_status = (int)status;
}
