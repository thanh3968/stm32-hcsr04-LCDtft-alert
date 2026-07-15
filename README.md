# STM32 HC-SR04 LCD TFT Alert

Project do khoang cach bang cam bien HC-SR04, hien thi tren LCD TFT tron GC9A01 1.28 inch va canh bao bang loa DY-SV17F + motor rung.

## Phan cung

- STM32F407G
- LCD TFT tron 1.28 inch GC9A01, 240x240, SPI
- Cam bien sieu am HC-SR04
- Module giai ma am thanh DY-SV17F
- Loa 4 Ohm 3W
- Motor rung dien thoai

## Noi day

### LCD GC9A01

| LCD | STM32F407 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| DIN | PB15 / SPI2_MOSI |
| CLK | PB13 / SPI2_SCK |
| CS | PB12 |
| DC | PB1 |
| RST | PB2 |
| BL | 3V3 |

### HC-SR04

| HC-SR04 | STM32F407 |
| --- | --- |
| VCC | 5V |
| GND | GND |
| TRIG | PB6 |
| ECHO | PB7 |

Nen dung mach chia ap cho chan ECHO vi HC-SR04 tra ve muc 5V.

### DY-SV17F

| DY-SV17F | STM32F407 |
| --- | --- |
| VCC/V5 | 5V |
| GND | GND |
| RXD/IO1 | PA2 / USART2_TX |
| TXD/IO0 | PA3 / USART2_RX |
| SPK+ | Loa + |
| SPK- | Loa - |

Chon UART mode:

| Chan | Noi |
| --- | --- |
| CON1 | GND |
| CON2 | GND |
| CON3 | 3V3 |

File am thanh trong DY-SV17F dat ten:

```text
00001.mp3
```

### Motor rung

| Motor | STM32F407 |
| --- | --- |
| Day do | 3V3 |
| Day con lai | PB0 |

Code dang dieu khien motor theo kieu active-low:

```text
PB0 = LOW  -> motor rung
PB0 = HIGH -> motor tat
```

Nen dung transistor/MOSFET neu chay lau dai de bao ve chan GPIO.

## Logic hoat dong

| Khoang cach | LCD | Mau | Loa | Motor |
| --- | --- | --- | --- | --- |
| < 10 cm | CANH BAO GAN | Do | Phat am thanh | Rung lien tuc |
| 10 - 50 cm | KHOANG CACH GAN | Vang/cam | Phat am thanh | Tat |
| > 50 cm | AN TOAN | Xanh | Tat | Tat |

Neu cam bien khong phan hoi, LCD hien thi `Sensor Timeout`.

## Cau truc file chinh

```text
Core/Inc/lcd_tft2.h
Core/Src/lcd_tft2.c

Core/Inc/hcsr04.h
Core/Src/hcsr04.c

Core/Inc/dy_sv17f.h
Core/Src/dy_sv17f.c

Core/Src/main.c
```

## Giao thuc su dung

- LCD GC9A01: SPI2, mau RGB565
- HC-SR04: GPIO trigger + do do rong xung echo
- DY-SV17F: UART2, baudrate 9600
- Motor rung: GPIO output PB0

Lenh phat file `00001.mp3` gui toi DY-SV17F:

```text
AA 07 02 00 01 B4
```

## Cach mo project

1. Mo STM32CubeIDE.
2. Chon `File > Import`.
3. Chon `Existing Projects into Workspace`.
4. Tro toi thu muc project.
5. Build va nap code vao STM32F407G.

