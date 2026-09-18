#include "ili9341.h"
#include <string.h>

#define ILI9341_NOP     0x00U
#define ILI9341_SWRESET 0x01U
#define ILI9341_SLPOUT  0x11U
#define ILI9341_DISPON  0x29U
#define ILI9341_CASET   0x2AU
#define ILI9341_PASET   0x2BU
#define ILI9341_RAMWR   0x2CU
#define ILI9341_MADCTL  0x36U
#define ILI9341_PIXFMT  0x3AU
#define ILI9341_PWCTR1  0xC0U
#define ILI9341_PWCTR2  0xC1U
#define ILI9341_VMCTR1  0xC5U
#define ILI9341_VMCTR2  0xC7U
#define ILI9341_FRMCTR1 0xB1U
#define ILI9341_DFUNCTR 0xB6U
#define ILI9341_GAMMASET 0x26U
#define ILI9341_GMCTRP1 0xE0U
#define ILI9341_GMCTRN1 0xE1U

#define MADCTL_MY  0x80U
#define MADCTL_MX  0x40U
#define MADCTL_MV  0x20U
#define MADCTL_BGR 0x08U

static const uint8_t s_font5x7[96][5] = {
    {0,0,0,0,0},{0,0,0x5F,0,0},{0,7,0,7,0},{0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,8,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},{0,5,3,0,0},{0,0x1C,0x22,0x41,0},{0,0x41,0x22,0x1C,0},{0x14,8,0x3E,8,0x14},{8,8,0x3E,8,8},{0,0x50,0x30,0,0},{8,8,8,8,8},{0,0x60,0x60,0,0},{0x20,0x10,8,4,2},
    {0x3E,0x51,0x49,0x45,0x3E},{0,0x42,0x7F,0x40,0},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{1,0x71,9,5,3},{0x36,0x49,0x49,0x49,0x36},{6,0x49,0x49,0x29,0x1E},{0,0x36,0x36,0,0},{0,0x56,0x36,0,0},{8,0x14,0x22,0x41,0},{0x14,0x14,0x14,0x14,0x14},{0,0x41,0x22,0x14,8},{2,1,0x51,9,6},
    {0x32,0x49,0x79,0x41,0x3E},{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,9,9,9,1},{0x3E,0x41,0x49,0x49,0x7A},{0x7F,8,8,8,0x7F},{0,0x41,0x7F,0x41,0},{0x20,0x40,0x41,0x3F,1},{0x7F,8,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},{0x7F,2,0x0C,2,0x7F},{0x7F,4,8,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,9,9,9,6},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,9,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},{1,1,0x7F,1,1},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,8,0x14,0x63},{7,8,0x70,8,7},{0x61,0x51,0x49,0x45,0x43},{0,0x7F,0x41,0x41,0},{2,4,8,0x10,0x20},{0,0x41,0x41,0x7F,0},{4,2,1,2,4},{0x40,0x40,0x40,0x40,0x40},
    {0,1,2,4,0},{0x20,0x54,0x54,0x54,0x78},{0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},{0x38,0x44,0x44,0x48,0x7F},{0x38,0x54,0x54,0x54,0x18},{8,0x7E,9,1,2},{0x0C,0x52,0x52,0x52,0x3E},{0x7F,8,4,4,0x78},{0,0x44,0x7D,0x40,0},{0x20,0x40,0x44,0x3D,0},{0x7F,0x10,0x28,0x44,0},{0,0x41,0x7F,0x40,0},{0x7C,4,0x18,4,0x78},{0x7C,8,4,4,0x78},{0x38,0x44,0x44,0x44,0x38},
    {0x7C,0x14,0x14,0x14,8},{8,0x14,0x14,0x18,0x7C},{0x7C,8,4,4,8},{0x48,0x54,0x54,0x54,0x20},{4,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},{0x3C,0x40,0x30,0x40,0x3C},{0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},{0x44,0x64,0x54,0x4C,0x44},{0,8,0x36,0x41,0},{0,0,0x7F,0,0},{0,0x41,0x36,8,0},{0x10,8,8,0x10,8},{0,0,0,0,0}
};

static void write_gpio(const ili9341_gpio_pin_t *pin, bool active)
{
    if ((pin != NULL) && pin->valid)
    {
        GPIO_PinWrite(pin->gpio, pin->pin, (active ^ pin->active_low) ? 1U : 0U);
    }
}

static status_t wait_spi_idle(ili9341_t *lcd)
{
    uint32_t loops = lcd->config.transfer_timeout_loops;
    while ((LPSPI_GetStatusFlags(lcd->config.spi) & (uint32_t)kLPSPI_ModuleBusyFlag) != 0U)
    {
        if (loops-- == 0U)
        {
            lcd->last_status = kStatus_Timeout;
            return kStatus_Timeout;
        }
    }
    return kStatus_Success;
}

static status_t write_bytes(ili9341_t *lcd, const uint8_t *data, size_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return kStatus_Success;
    }
    if (wait_spi_idle(lcd) != kStatus_Success)
    {
        return lcd->last_status;
    }
    lpspi_transfer_t transfer = {
        .txData = (uint8_t *)data,
        .rxData = NULL,
        .dataSize = length,
        .configFlags = kLPSPI_MasterPcs0
    };
    lcd->last_status = LPSPI_MasterTransferBlocking(lcd->config.spi, &transfer);
    return lcd->last_status;
}

static status_t write_command(ili9341_t *lcd, uint8_t command)
{
    write_gpio(&lcd->config.dc, false);
    write_gpio(&lcd->config.cs, true);
    status_t status = write_bytes(lcd, &command, 1U);
    write_gpio(&lcd->config.cs, false);
    return status;
}

static status_t write_data(ili9341_t *lcd, const uint8_t *data, size_t length)
{
    write_gpio(&lcd->config.dc, true);
    write_gpio(&lcd->config.cs, true);
    status_t status = write_bytes(lcd, data, length);
    write_gpio(&lcd->config.cs, false);
    return status;
}

static status_t write_command_data(ili9341_t *lcd, uint8_t command, const uint8_t *data, size_t length)
{
    status_t status = write_command(lcd, command);
    if (status == kStatus_Success)
    {
        status = write_data(lcd, data, length);
    }
    return status;
}

static void delay_ms(uint32_t ms)
{
    SDK_DelayAtLeastUs(ms * 1000U, SystemCoreClock);
}

status_t ili9341_reset(ili9341_t *lcd)
{
    if (lcd == NULL)
    {
        return kStatus_InvalidArgument;
    }
    write_gpio(&lcd->config.cs, false);
    write_gpio(&lcd->config.dc, true);
    if (lcd->config.rst.valid)
    {
        /* RST is active-low on the common ILI9341 SPI modules: idle high, pulse low, return high. */
        write_gpio(&lcd->config.rst, false);
        delay_ms(10U);
        write_gpio(&lcd->config.rst, true);
        delay_ms(20U);
        write_gpio(&lcd->config.rst, false);
        delay_ms(120U);
        return kStatus_Success;
    }
    status_t status = write_command(lcd, ILI9341_SWRESET);
    delay_ms(150U);
    return status;
}

status_t ili9341_init(ili9341_t *lcd, const ili9341_config_t *config)
{
    if ((lcd == NULL) || (config == NULL) || (config->spi == NULL))
    {
        return kStatus_InvalidArgument;
    }

    memset(lcd, 0, sizeof(*lcd));
    lcd->config = *config;
    lcd->width = ILI9341_WIDTH;
    lcd->height = ILI9341_HEIGHT;
    if (lcd->config.transfer_timeout_loops == 0U)
    {
        lcd->config.transfer_timeout_loops = 1000000U;
    }

    write_gpio(&lcd->config.backlight, true);

    status_t status = ili9341_reset(lcd);
    const uint8_t pixfmt[] = {0x55U};
    const uint8_t pwctr1[] = {0x23U};
    const uint8_t pwctr2[] = {0x10U};
    const uint8_t vmctr1[] = {0x3EU, 0x28U};
    const uint8_t vmctr2[] = {0x86U};
    const uint8_t frmctr1[] = {0x00U, 0x18U};
    const uint8_t dfunctr[] = {0x08U, 0x82U, 0x27U};
    const uint8_t gammaset[] = {0x01U};
    const uint8_t gamma_pos[] = {0x0FU,0x31U,0x2BU,0x0CU,0x0EU,0x08U,0x4EU,0xF1U,0x37U,0x07U,0x10U,0x03U,0x0EU,0x09U,0x00U};
    const uint8_t gamma_neg[] = {0x00U,0x0EU,0x14U,0x03U,0x11U,0x07U,0x31U,0xC1U,0x48U,0x08U,0x0FU,0x0CU,0x31U,0x36U,0x0FU};

    if (status == kStatus_Success) status = write_command_data(lcd, ILI9341_PWCTR1, pwctr1, sizeof(pwctr1));
    if (status == kStatus_Success) status = write_command_data(lcd, ILI9341_PWCTR2, pwctr2, sizeof(pwctr2));
    if (status == kStatus_Success) status = write_command_data(lcd, ILI9341_VMCTR1, vmctr1, sizeof(vmctr1));
    if (status == kStatus_Success) status = write_command_data(lcd, ILI9341_VMCTR2, vmctr2, sizeof(vmctr2));
    if (status == kStatus_Success) status = write_command_data(lcd, ILI9341_PIXFMT, pixfmt, sizeof(pixfmt));
    if (status == kStatus_Success) status = write_command_data(lcd, ILI9341_FRMCTR1, frmctr1, sizeof(frmctr1));
    if (status == kStatus_Success) status = write_command_data(lcd, ILI9341_DFUNCTR, dfunctr, sizeof(dfunctr));
    if (status == kStatus_Success) status = write_command_data(lcd, ILI9341_GAMMASET, gammaset, sizeof(gammaset));
    if (status == kStatus_Success) status = write_command_data(lcd, ILI9341_GMCTRP1, gamma_pos, sizeof(gamma_pos));
    if (status == kStatus_Success) status = write_command_data(lcd, ILI9341_GMCTRN1, gamma_neg, sizeof(gamma_neg));
    if (status == kStatus_Success) status = ili9341_set_rotation(lcd, 0U);
    if (status == kStatus_Success) status = write_command(lcd, ILI9341_SLPOUT);
    delay_ms(120U);
    if (status == kStatus_Success) status = write_command(lcd, ILI9341_DISPON);
    delay_ms(20U);
    lcd->last_status = status;
    return status;
}

status_t ili9341_set_rotation(ili9341_t *lcd, uint8_t rotation)
{
    if (lcd == NULL)
    {
        return kStatus_InvalidArgument;
    }
    uint8_t madctl;
    lcd->rotation = rotation & 3U;
    switch (lcd->rotation)
    {
        case 1U:
            madctl = MADCTL_MV | MADCTL_BGR;
            lcd->width = ILI9341_HEIGHT;
            lcd->height = ILI9341_WIDTH;
            break;
        case 2U:
            madctl = MADCTL_MX | MADCTL_BGR;
            lcd->width = ILI9341_WIDTH;
            lcd->height = ILI9341_HEIGHT;
            break;
        case 3U:
            madctl = MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR;
            lcd->width = ILI9341_HEIGHT;
            lcd->height = ILI9341_WIDTH;
            break;
        default:
            madctl = MADCTL_MY | MADCTL_BGR;
            lcd->width = ILI9341_WIDTH;
            lcd->height = ILI9341_HEIGHT;
            break;
    }
    return write_command_data(lcd, ILI9341_MADCTL, &madctl, 1U);
}

status_t ili9341_set_address_window(ili9341_t *lcd, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t data[4];
    data[0] = (uint8_t)(x0 >> 8);
    data[1] = (uint8_t)x0;
    data[2] = (uint8_t)(x1 >> 8);
    data[3] = (uint8_t)x1;
    status_t status = write_command_data(lcd, ILI9341_CASET, data, sizeof(data));
    data[0] = (uint8_t)(y0 >> 8);
    data[1] = (uint8_t)y0;
    data[2] = (uint8_t)(y1 >> 8);
    data[3] = (uint8_t)y1;
    if (status == kStatus_Success) status = write_command_data(lcd, ILI9341_PASET, data, sizeof(data));
    if (status == kStatus_Success) status = write_command(lcd, ILI9341_RAMWR);
    return status;
}

status_t ili9341_draw_pixel(ili9341_t *lcd, uint16_t x, uint16_t y, uint16_t color)
{
    if ((lcd == NULL) || (x >= lcd->width) || (y >= lcd->height))
    {
        return kStatus_InvalidArgument;
    }
    uint8_t data[] = {(uint8_t)(color >> 8), (uint8_t)color};
    status_t status = ili9341_set_address_window(lcd, x, y, x, y);
    if (status == kStatus_Success) status = write_data(lcd, data, sizeof(data));
    return status;
}

status_t ili9341_fill_rect(ili9341_t *lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if ((lcd == NULL) || (x >= lcd->width) || (y >= lcd->height) || (w == 0U) || (h == 0U))
    {
        return kStatus_InvalidArgument;
    }
    if ((x + w) > lcd->width) w = lcd->width - x;
    if ((y + h) > lcd->height) h = lcd->height - y;

    status_t status = ili9341_set_address_window(lcd, x, y, (uint16_t)(x + w - 1U), (uint16_t)(y + h - 1U));
    if (status != kStatus_Success)
    {
        return status;
    }

    uint8_t buffer[256];
    for (size_t i = 0; i < sizeof(buffer); i += 2U)
    {
        buffer[i] = (uint8_t)(color >> 8);
        buffer[i + 1U] = (uint8_t)color;
    }

    uint32_t remaining = (uint32_t)w * (uint32_t)h;
    while ((remaining > 0U) && (status == kStatus_Success))
    {
        uint32_t pixels = remaining > (sizeof(buffer) / 2U) ? (sizeof(buffer) / 2U) : remaining;
        status = write_data(lcd, buffer, pixels * 2U);
        remaining -= pixels;
    }
    return status;
}

status_t ili9341_fill_screen(ili9341_t *lcd, uint16_t color)
{
    return ili9341_fill_rect(lcd, 0U, 0U, lcd->width, lcd->height, color);
}

status_t ili9341_draw_line(ili9341_t *lcd, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t dy = (y0 < y1) ? (y0 - y1) : (y1 - y0);
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;
    while (true)
    {
        if ((x0 >= 0) && (y0 >= 0) && (x0 < (int16_t)lcd->width) && (y0 < (int16_t)lcd->height))
        {
            status_t status = ili9341_draw_pixel(lcd, (uint16_t)x0, (uint16_t)y0, color);
            if (status != kStatus_Success) return status;
        }
        if ((x0 == x1) && (y0 == y1)) break;
        int16_t e2 = (int16_t)(2 * err);
        if (e2 >= dy) { err = (int16_t)(err + dy); x0 = (int16_t)(x0 + sx); }
        if (e2 <= dx) { err = (int16_t)(err + dx); y0 = (int16_t)(y0 + sy); }
    }
    return kStatus_Success;
}

static status_t draw_char(ili9341_t *lcd, uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t background, uint8_t scale)
{
    if ((ch < ' ') || (ch > '~'))
    {
        ch = '?';
    }
    if (scale == 0U)
    {
        scale = 1U;
    }
    const uint8_t *glyph = s_font5x7[(uint8_t)ch - 32U];
    for (uint8_t col = 0U; col < 6U; col++)
    {
        uint8_t line = (col < 5U) ? glyph[col] : 0U;
        for (uint8_t row = 0U; row < 8U; row++)
        {
            uint16_t draw_color = ((line & (1U << row)) != 0U) ? color : background;
            status_t status = ili9341_fill_rect(lcd,
                                                (uint16_t)(x + (col * scale)),
                                                (uint16_t)(y + (row * scale)),
                                                scale,
                                                scale,
                                                draw_color);
            if (status != kStatus_Success)
            {
                return status;
            }
        }
    }
    return kStatus_Success;
}

status_t ili9341_write_string(ili9341_t *lcd,
                              uint16_t x,
                              uint16_t y,
                              const char *text,
                              uint16_t color,
                              uint16_t background,
                              uint8_t scale)
{
    if ((lcd == NULL) || (text == NULL))
    {
        return kStatus_InvalidArgument;
    }
    uint16_t cursor_x = x;
    uint16_t cursor_y = y;
    uint16_t char_w = (uint16_t)(6U * ((scale == 0U) ? 1U : scale));
    uint16_t char_h = (uint16_t)(8U * ((scale == 0U) ? 1U : scale));
    while (*text != '\0')
    {
        if (*text == '\n')
        {
            cursor_x = x;
            cursor_y = (uint16_t)(cursor_y + char_h);
        }
        else
        {
            status_t status = draw_char(lcd, cursor_x, cursor_y, *text, color, background, scale);
            if (status != kStatus_Success)
            {
                return status;
            }
            cursor_x = (uint16_t)(cursor_x + char_w);
        }
        text++;
    }
    return kStatus_Success;
}
