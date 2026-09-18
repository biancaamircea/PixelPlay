#include "ili9341_demo.h"
#include "board.h"
#include "board_ili9341.h"
#include "fsl_debug_console.h"
#include "fsl_port.h"

#define SW3_PRESSED_LEVEL 0U
#define WAVE_POINTS       40U
#define STAR_COUNT        64U
#define STAR_Z_NEAR       (8 * 256)
#define STAR_Z_FAR        (132 * 256)
#define STAR_SPEED_BASE   330
#define WAVE_CENTER_Y     120
#define WAVE_AMPLITUDE    58
#define VIS_THEME_COUNT   6U

static ili9341_t s_lcd;
static int16_t s_wave[WAVE_POINTS];
static int16_t s_old_wave_x[WAVE_POINTS];
static int16_t s_old_wave_y[WAVE_POINTS];
static int16_t s_star_x[STAR_COUNT];
static int16_t s_star_y[STAR_COUNT];
static int32_t s_star_z[STAR_COUNT];
static int16_t s_old_star_x[STAR_COUNT];
static int16_t s_old_star_y[STAR_COUNT];
static volatile uint16_t s_input_value = 2048U;
static volatile bool s_external_input;
static uint32_t s_frame = 1U;
static uint32_t s_theme;
static bool s_was_pressed;
static bool s_initialized;

static void init_sw3(void)
{
    const gpio_pin_config_t sw_config = {
        .pinDirection = kGPIO_DigitalInput,
        .outputLogic = 0U,
    };

    const port_pin_config_t sw_pin_config = {
        kPORT_PullDisable,
        kPORT_LowPullResistor,
        kPORT_FastSlewRate,
        kPORT_PassiveFilterDisable,
        kPORT_OpenDrainDisable,
        kPORT_LowDriveStrength,
        kPORT_NormalDriveStrength,
        kPORT_MuxAlt0,
        kPORT_InputBufferEnable,
        kPORT_InputNormal,
        kPORT_UnlockRegister,
    };

    CLOCK_EnableClock(kCLOCK_GateGPIO1);
    CLOCK_EnableClock(kCLOCK_GatePORT1);
    RESET_ReleasePeripheralReset(kGPIO1_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kPORT1_RST_SHIFT_RSTn);

    PORT_SetPinConfig(PORT1, BOARD_SW3_GPIO_PIN, &sw_pin_config);
    GPIO_PinInit(BOARD_SW3_GPIO, BOARD_SW3_GPIO_PIN, &sw_config);
}

static bool sw3_pressed(void)
{
    return GPIO_PinRead(BOARD_SW3_GPIO, BOARD_SW3_GPIO_PIN) == SW3_PRESSED_LEVEL;
}

static uint8_t tri_wave(uint32_t phase)
{
    phase &= 0xFFU;
    return (phase < 128U) ? (uint8_t)(phase * 2U) : (uint8_t)(255U - ((phase - 128U) * 2U));
}

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((uint16_t)(r & 0xF8U) << 8) | ((uint16_t)(g & 0xFCU) << 3) | (b >> 3));
}

static uint16_t visualizer_color(uint32_t phase, uint32_t theme)
{
    phase = (phase + (theme * 37U)) & 0xFFU;

    switch (theme % VIS_THEME_COUNT)
    {
        case 0U:
            return rgb565(tri_wave(phase), tri_wave(phase + 85U), tri_wave(phase + 170U));
        case 1U:
            return rgb565(255U, tri_wave(phase), tri_wave(phase + 92U));
        case 2U:
            return rgb565(tri_wave(phase + 40U), 255U, tri_wave(phase + 150U));
        case 3U:
            return rgb565(tri_wave(phase + 150U), tri_wave(phase + 20U), 255U);
        case 4U:
            return rgb565(tri_wave(phase + 12U), tri_wave(phase + 130U), 255U - tri_wave(phase + 64U));
        default:
            return rgb565(255U - tri_wave(phase), tri_wave(phase + 55U), tri_wave(phase + 180U));
    }
}

void ILI9341_DemoSetInput(uint16_t value)
{
    s_input_value = (value > 4095U) ? 4095U : value;
    s_external_input = true;
}

static uint16_t synthetic_visualizer_input(uint32_t frame)
{
    const uint32_t synthetic = ((uint32_t)tri_wave(frame * 5U) * 11U) +
                               ((uint32_t)tri_wave((frame * 13U) + 70U) * 5U);
    return (uint16_t)(synthetic & 0x0FFFU);
}

static void draw_segment(ili9341_t *lcd, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    if ((x0 >= 0) && (x0 < (int16_t)lcd->width) && (y0 >= 0) && (y0 < (int16_t)lcd->height) &&
        (x1 >= 0) && (x1 < (int16_t)lcd->width) && (y1 >= 0) && (y1 < (int16_t)lcd->height))
    {
        ili9341_draw_line(lcd, x0, y0, x1, y1, color);
    }
}

static void reset_star(uint32_t index, uint32_t frame)
{
    const uint32_t seed = (index * 73U) + (frame * 17U) + 19U;

    s_star_x[index] = (int16_t)((int32_t)((seed * 37U) % 260U) - 130);
    s_star_y[index] = (int16_t)((int32_t)((seed * 53U) % 190U) - 95);
    s_star_z[index] = (int32_t)(STAR_Z_FAR - (int32_t)(((seed * 29U) % 36U) * 256U));
    s_old_star_x[index] = -1;
    s_old_star_y[index] = -1;
}

static bool project_star(const ili9341_t *lcd, int16_t x, int16_t y, int32_t z_q8, int16_t *screen_x, int16_t *screen_y)
{
    if (z_q8 <= 0)
    {
        return false;
    }

    *screen_x = (int16_t)((lcd->width / 2U) + (((int32_t)x * 18432) / z_q8));
    *screen_y = (int16_t)((lcd->height / 2U) + (((int32_t)y * 18432) / z_q8));

    return (*screen_x >= 0) && (*screen_x < (int16_t)lcd->width) && (*screen_y >= 0) &&
           (*screen_y < (int16_t)lcd->height);
}

static void draw_starfield(ili9341_t *lcd, uint32_t frame, uint32_t theme)
{
    for (uint32_t i = 0U; i < STAR_COUNT; i++)
    {
        if (s_old_star_x[i] >= 0)
        {
            ili9341_fill_rect(lcd, (uint16_t)s_old_star_x[i], (uint16_t)s_old_star_y[i], 4U, 4U, ILI9341_COLOR_BLACK);
        }

        s_star_z[i] -= (int32_t)(STAR_SPEED_BASE + ((i & 7U) * 34U));
        if (s_star_z[i] < STAR_Z_NEAR)
        {
            reset_star(i, frame);
        }

        int16_t x;
        int16_t y;
        if (!project_star(lcd, s_star_x[i], s_star_y[i], s_star_z[i], &x, &y))
        {
            reset_star(i, frame + 3U);
            (void)project_star(lcd, s_star_x[i], s_star_y[i], s_star_z[i], &x, &y);
        }

        const uint16_t size = (s_star_z[i] < (24 * 256)) ? 3U : ((s_star_z[i] < (54 * 256)) ? 2U : 1U);
        const uint16_t color = visualizer_color((frame * 5U) + (i * 17U), theme);
        if (s_old_star_x[i] >= 0)
        {
            draw_segment(lcd, s_old_star_x[i], s_old_star_y[i], x, y, color);
        }
        ili9341_fill_rect(lcd, (uint16_t)x, (uint16_t)y, size, size, color);
        s_old_star_x[i] = x;
        s_old_star_y[i] = y;
    }
}

static void reset_visualizer(ili9341_t *lcd)
{
    for (uint32_t p = 0U; p < WAVE_POINTS; p++)
    {
        s_wave[p] = WAVE_CENTER_Y;
        s_old_wave_x[p] = (int16_t)((p * (lcd->width - 1U)) / (WAVE_POINTS - 1U));
        s_old_wave_y[p] = WAVE_CENTER_Y;
    }

    for (uint32_t i = 0U; i < STAR_COUNT; i++)
    {
        reset_star(i, i * 5U);
    }

    ili9341_fill_screen(lcd, ILI9341_COLOR_BLACK);
}

static void draw_visualizer_frame(ili9341_t *lcd, uint32_t frame, uint32_t theme, uint16_t input)
{
    const int16_t level = (int16_t)(8 + (((uint32_t)input * WAVE_AMPLITUDE) / 4095U));
    int16_t new_x[WAVE_POINTS];
    int16_t new_y[WAVE_POINTS];

    draw_starfield(lcd, frame, theme);

    for (uint32_t p = 1U; p < WAVE_POINTS; p++)
    {
        draw_segment(lcd,
                     s_old_wave_x[p - 1U],
                     s_old_wave_y[p - 1U],
                     s_old_wave_x[p],
                     s_old_wave_y[p],
                     ILI9341_COLOR_BLACK);
    }

    for (uint32_t p = 0U; p < WAVE_POINTS; p++)
    {
        const int16_t wave = (int16_t)tri_wave((frame * 9U) + (p * 18U)) - 128;
        const int16_t harmonic = (int16_t)tri_wave((frame * 5U) + (p * 43U) + 80U) - 128;
        const int16_t x = (int16_t)((p * (lcd->width - 1U)) / (WAVE_POINTS - 1U));
        const int16_t y = (int16_t)(WAVE_CENTER_Y + ((wave * level) / 160) + ((harmonic * level) / 420));

        s_wave[p] = y;
        new_x[p] = x;
        new_y[p] = y;
    }

    for (uint32_t p = 1U; p < WAVE_POINTS; p++)
    {
        const uint16_t glow = visualizer_color((frame * 4U) + (p * 11U) + 90U, theme);
        const uint16_t core = visualizer_color((frame * 7U) + (p * 15U), theme);
        draw_segment(lcd, new_x[p - 1U], (int16_t)(new_y[p - 1U] - 2), new_x[p], (int16_t)(new_y[p] - 2), glow);
        draw_segment(lcd, new_x[p - 1U], new_y[p - 1U], new_x[p], new_y[p], core);
        draw_segment(lcd, new_x[p - 1U], (int16_t)(new_y[p - 1U] + 2), new_x[p], (int16_t)(new_y[p] + 2), glow);
    }

    for (uint32_t p = 0U; p < WAVE_POINTS; p++)
    {
        s_old_wave_x[p] = new_x[p];
        s_old_wave_y[p] = new_y[p];
    }
}

status_t ILI9341_DemoRun(void)
{
    status_t status = ILI9341_DemoInit();
    if (status != kStatus_Success)
    {
        return status;
    }

    while (true)
    {
        ILI9341_DemoUpdate(synthetic_visualizer_input(s_frame));
        SDK_DelayAtLeastUs(12000U, CLOCK_GetCoreSysClkFreq());
    }
}

status_t ILI9341_DemoInit(void)
{
    ili9341_config_t config;

    BOARD_GetIli9341Config(&config);

    status_t status = ili9341_init(&s_lcd, &config);
    if (status != kStatus_Success)
    {
        PRINTF("ILI9341 init failed: %ld\r\n", (long)status);
        return status;
    }

    status = ili9341_set_rotation(&s_lcd, 1U);
    if (status != kStatus_Success)
    {
        return status;
    }

    init_sw3();
    reset_visualizer(&s_lcd);

    s_frame = 1U;
    s_theme = 0U;
    s_was_pressed = sw3_pressed();
    s_initialized = true;

    PRINTF("ILI9341 3D waveform ready. Feed ILI9341_DemoUpdate(value 0..4095).\r\n");
    return kStatus_Success;
}

void ILI9341_DemoUpdate(uint16_t input_value)
{
    if (!s_initialized)
    {
        return;
    }

    if (input_value > 4095U)
    {
        input_value = 4095U;
    }

    const bool pressed = sw3_pressed();

    if (pressed && !s_was_pressed)
    {
        s_theme = (s_theme + 1U) % VIS_THEME_COUNT;
        reset_visualizer(&s_lcd);
    }
    s_was_pressed = pressed;

    ILI9341_DemoSetInput(input_value);
    draw_visualizer_frame(&s_lcd, s_frame++, s_theme, input_value);
}
