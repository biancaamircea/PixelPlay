#ifndef ILI9341_H_
#define ILI9341_H_

#include "fsl_common.h"
#include "fsl_gpio.h"
#include "fsl_lpspi.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ILI9341_WIDTH  240U
#define ILI9341_HEIGHT 320U

#define ILI9341_COLOR_BLACK   0x0000U
#define ILI9341_COLOR_BLUE    0x001FU
#define ILI9341_COLOR_RED     0xF800U
#define ILI9341_COLOR_GREEN   0x07E0U
#define ILI9341_COLOR_CYAN    0x07FFU
#define ILI9341_COLOR_MAGENTA 0xF81FU
#define ILI9341_COLOR_YELLOW  0xFFE0U
#define ILI9341_COLOR_WHITE   0xFFFFU
#define ILI9341_COLOR_ORANGE  0xFD20U

typedef struct
{
    GPIO_Type *gpio;
    uint32_t pin;
    bool active_low;
    bool valid;
} ili9341_gpio_pin_t;

typedef struct
{
    LPSPI_Type *spi;
    uint32_t spi_source_clock_hz;
    uint32_t spi_baudrate_hz;
    uint32_t transfer_timeout_loops;
    ili9341_gpio_pin_t cs;
    ili9341_gpio_pin_t dc;
    ili9341_gpio_pin_t rst;
    ili9341_gpio_pin_t backlight;
} ili9341_config_t;

typedef struct
{
    ili9341_config_t config;
    uint16_t width;
    uint16_t height;
    uint8_t rotation;
    status_t last_status;
} ili9341_t;

status_t ili9341_init(ili9341_t *lcd, const ili9341_config_t *config);
status_t ili9341_reset(ili9341_t *lcd);
status_t ili9341_set_rotation(ili9341_t *lcd, uint8_t rotation);
status_t ili9341_set_address_window(ili9341_t *lcd, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
status_t ili9341_draw_pixel(ili9341_t *lcd, uint16_t x, uint16_t y, uint16_t color);
status_t ili9341_fill_rect(ili9341_t *lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
status_t ili9341_fill_screen(ili9341_t *lcd, uint16_t color);
status_t ili9341_draw_line(ili9341_t *lcd, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
status_t ili9341_write_string(ili9341_t *lcd,
                              uint16_t x,
                              uint16_t y,
                              const char *text,
                              uint16_t color,
                              uint16_t background,
                              uint8_t scale);

#endif /* ILI9341_H_ */
