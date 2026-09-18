/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board.h"
#include "app.h"
#include "peripherals.h"
#include "pin_mux.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app/board_ili9341.h"
#include "app/ili9341.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "fsl_lpadc.h"
#include "fsl_port.h"

#if defined(__GNUC__)
#define APP_UNUSED __attribute__((unused))
#else
#define APP_UNUSED
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define ADC_SW2_CHANNEL 14U
#define ADC_COMMAND_ID 1U
#define ADC_TRIGGER_ID 0U
#define ADC_PRINT_TICKS 100U
#define PWM_FADE_DUTY_MAX 100U
#define GAME_FRAME_DELAY_US 0U
#define GAME_SW2_PRESSED_THRESHOLD 1000U
#define SW3_PRESSED_LEVEL 0U
#define BUTTON_DEBOUNCE_US 40000U
#define GAME_SKY_COLOR ILI9341_COLOR_CYAN
#define GAME_FLOOR_COLOR 0x4208U
#define GAME_ROAD_COLOR ILI9341_COLOR_BLACK
#define GAME_ROAD_EDGE_COLOR ILI9341_COLOR_WHITE
#define GAME_PLAYER_COLOR ILI9341_COLOR_YELLOW
#define GAME_OBSTACLE_COLOR ILI9341_COLOR_RED
#define GAME_TEXT_COLOR ILI9341_COLOR_WHITE
#define MENU_BG_COLOR ILI9341_COLOR_BLACK
#define MENU_TITLE_COLOR ILI9341_COLOR_CYAN
#define MENU_SELECTED_COLOR ILI9341_COLOR_YELLOW
#define MENU_NORMAL_COLOR ILI9341_COLOR_WHITE
#define MENU_HINT_COLOR 0x8410U
#define MENU_PHOTO_OPTION 2U
#define MENU_VIDEO_OPTION 3U
#define MENU_TOUCH_OPTION 255U
#define MENU_OPTION_COUNT 4U
#define CAMERA_TRIGGER_GPIO GPIO3
#define CAMERA_TRIGGER_PORT PORT3
#define CAMERA_TRIGGER_PIN 14U
#define CAMERA_STATUS_GPIO GPIO1
#define CAMERA_STATUS_PORT PORT1
#define CAMERA_STATUS_PIN 10U
#define CAMERA_TRIGGER_PULSE_US 200000U
#define CAMERA_VIDEO_TRIGGER_PULSE_US 1500000U
#define TOUCH_MOSI_GPIO GPIO1
#define TOUCH_MOSI_PORT PORT1
#define TOUCH_MOSI_PIN 0U
#define TOUCH_MISO_GPIO GPIO1
#define TOUCH_MISO_PORT PORT1
#define TOUCH_MISO_PIN 2U
#define TOUCH_SCK_GPIO GPIO1
#define TOUCH_SCK_PORT PORT1
#define TOUCH_SCK_PIN 1U
#define TOUCH_CMD_X 0xD0U
#define TOUCH_CMD_Y 0x90U
#define TOUCH_CMD_Z1 0xB0U
#define TOUCH_CMD_Z2 0xC0U
#define TOUCH_RAW_MIN 20U
#define TOUCH_RAW_MAX 4075U
#define TOUCH_Z1_MIN 120U
#define TOUCH_Z2_MIN_DELTA 80U
#define TOUCH_Z2_MAX 4050U
#define TOUCH_START_CONFIRM_US 25000U
#define GAME_HORIZON_Y 54U
#define GAME_ROAD_NEAR_HALF_W 118U
#define GAME_ROAD_FAR_HALF_W 18U
#define GAME_LANE_COUNT 3U
#define GAME_CENTER_LANE 1U
#define GAME_PLAYER_BASE_W 22
#define GAME_PLAYER_BASE_H 32
#define GAME_OBSTACLE_COUNT 3U
#define GAME_OBSTACLE_MIN_Z_Q8 (70 * 256)
#define GAME_OBSTACLE_MAX_Z_Q8 (240 * 256)
#define GAME_OBSTACLE_BASE_SPEED_Q8 1300U
#define GAME_OBSTACLE_MAX_SPEED_Q8 3200U
#define GAME_DIRTY_PAD 8
#define GAME_SCORE_W 120U
#define GAME_SCORE_H 24U
#define GAME_OVER_X 72U
#define GAME_OVER_Y 82U
#define GAME_OVER_W 150U
#define GAME_OVER_H 54U
#define FLAPPY_SKY_COLOR 0x867FU
#define FLAPPY_GROUND_COLOR 0x7BE0U
#define FLAPPY_PIPE_COLOR ILI9341_COLOR_GREEN
#define FLAPPY_BIRD_COLOR ILI9341_COLOR_YELLOW
#define FLAPPY_TEXT_BG 0x0013U
#define FLAPPY_BIRD_X 68
#define FLAPPY_BIRD_SIZE 14
#define FLAPPY_GROUND_Y 210
#define FLAPPY_PIPE_W 34
#define FLAPPY_GAP_H 86
#define FLAPPY_SCORE_W 118U
#define FLAPPY_SCORE_H 30U
#define FLAPPY_GRAVITY_Q4 6
#define FLAPPY_FLAP_VELOCITY_Q4 -56
#define FLAPPY_SCROLL_SPEED 5
#define FLAPPY_FRAME_DELAY_US 28000U
#define DINO_BG_COLOR ILI9341_COLOR_WHITE
#define DINO_FG_COLOR ILI9341_COLOR_BLACK
#define DINO_GROUND_Y 214
#define DINO_X 48
#define DINO_W 32
#define DINO_H 38
#define DINO_CACTUS_W 16
#define DINO_CACTUS_H 30
#define DINO_GRAVITY_Q4 12
#define DINO_JUMP_VELOCITY_Q4 -170
#define DINO_SCROLL_SPEED 10
#define DINO_FRAME_DELAY_US 12000U
#define CLICK_BG_COLOR 0x0013U
#define CLICK_PANEL_COLOR 0x0000U
#define CLICK_ACCENT_COLOR ILI9341_COLOR_CYAN
#define CLICK_TEXT_COLOR ILI9341_COLOR_WHITE
#define CLICK_SCORE_COLOR ILI9341_COLOR_YELLOW
#define CLICK_FRAME_DELAY_US 50000U

typedef struct _touch_irq_candidate
{
    GPIO_Type *gpio;
    PORT_Type *port;
    uint32_t pin;
    const char *label;
} touch_irq_candidate_t;

static const touch_irq_candidate_t s_touchIrqCandidates[] = {
    {GPIO2, PORT2, 4U, "J1[6]"},
};

static const touch_irq_candidate_t s_touchCsCandidates[] = {
    {GPIO3, PORT3, 1U, "J1[16]"},
};

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void PWM0_LED_InitOutput(void);
static void PWM0_LED_UpdateFade(void);
static void SW2_ADC_InitInput(void);
static uint16_t SW2_ADC_ReadRaw(void);
static bool SW2_IsPressed(void);
static bool SW3_IsPressed(void);
static bool Button_DebouncedRead(bool (*readPressed)(void), bool previousPressed);
static void Game_FillRectClipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
static uint16_t Game_CurrentSpeedQ8(void);
static uint16_t Game_RoadHalfWidthAtY(uint16_t y);
static int16_t Game_LaneCenterAtY(uint8_t lane, uint16_t y);
static uint16_t Game_ProjectTQ8(int32_t zQ8);
static uint16_t Game_ProjectY(int32_t zQ8);
static uint16_t Game_ProjectScale(int32_t zQ8);
static uint8_t Game_RandomLane(uint32_t seed);
static void Game_GetObstacleMainRect(int32_t zQ8, uint8_t lane, int16_t *x, int16_t *y, int16_t *w, int16_t *h);
static void Game_GetObstacleRect(int32_t zQ8, uint8_t lane, int16_t *x, int16_t *y, int16_t *w, int16_t *h);
static void Game_ResetObstacle(uint32_t index, uint32_t seed);
static void Game_Reset(void);
static void Game_DrawRoad(void);
static void Game_GetRectUnion(int16_t ax,
                              int16_t ay,
                              int16_t aw,
                              int16_t ah,
                              int16_t bx,
                              int16_t by,
                              int16_t bw,
                              int16_t bh,
                              int16_t *x,
                              int16_t *y,
                              int16_t *w,
                              int16_t *h);
static void Game_ClearGameplayRect(int16_t x, int16_t y, int16_t w, int16_t h);
static void Game_GetPlayerRect(uint8_t lane, int16_t *x, int16_t *y, int16_t *w, int16_t *h);
static void Game_ErasePlayerRect(void);
static void Game_DrawPlayerAtLane(uint8_t lane);
static void Game_DrawPlayer(void);
static void Game_DrawObstacle(uint32_t index);
static void Game_DrawScore(void);
static void Game_DrawGameOverOverlay(void);
static void Game_DrawScene(void) APP_UNUSED;
static bool Game_HandleInput(bool moveLeftEdge, bool moveRightEdge) APP_UNUSED;
static bool Game_CheckCollision(void);
static void Game_UpdateWorld(void) APP_UNUSED;
static void Flappy_Reset(void);
static void Flappy_DrawScene(void);
static void Flappy_HandleInput(bool flapPressed, bool flapEdge, bool backEdge);
static void Flappy_UpdateWorld(void);
static bool Flappy_CheckCollision(void);
static void Flappy_ResetPipe(uint32_t index, int16_t x);
static void Dino_Reset(void);
static void Dino_DrawScene(void);
static void Dino_HandleInput(bool jumpEdge, bool backEdge);
static void Dino_UpdateWorld(void);
static bool Dino_CheckCollision(void);
static void StackTower_Reset(void) APP_UNUSED;
static void StackTower_Update(void) APP_UNUSED;
static void StackTower_Draw(void) APP_UNUSED;
static void StackTower_HandleInput(bool dropEdge, bool backEdge) APP_UNUSED;
static void StackTower_DropBlock(void) APP_UNUSED;
static const char *Menu_PlayerName(void);
static void Menu_DrawOption(uint16_t y, const char *text, bool selected);
static void Menu_DrawWelcome(void);
static void Menu_DrawAnastasiaAvatar(uint16_t x, uint16_t y);
static void Menu_DrawBiancaAvatar(uint16_t x, uint16_t y);
static void Menu_DrawPlayerSelect(void);
static void Menu_DrawGameSelect(void);
static void Menu_DrawGamePlaceholder(void);
static void Menu_HandleInput(bool nextPressed, bool nextEdge, bool selectEdge);
static void Camera_InitPins(void);
static void Camera_TriggerCapture(void);
static void Camera_TriggerVideo(void);
static bool Camera_StatusActive(void);
static void Touch_InitPins(void);
static bool Touch_IrqActive(void);
static const char *Touch_ActiveIrqLabel(void);
static void Touch_SelectCs(bool selected);
static const char *Touch_CurrentCsLabel(void);
static bool Touch_ReadRaw(uint16_t *xRaw, uint16_t *yRaw, uint16_t *z1Raw, uint16_t *z2Raw);
static bool Touch_RawLooksPressed(uint16_t xRaw, uint16_t yRaw, uint16_t z1Raw, uint16_t z2Raw, bool irqActive);
static void Touch_DrawStatus(uint16_t xRaw, uint16_t yRaw, uint16_t z1Raw, uint16_t z2Raw);

typedef enum _app_screen
{
    kAppScreenWelcome = 0,
    kAppScreenPlayerSelect,
    kAppScreenGameSelect,
    kAppScreenGamePlaceholder,
} app_screen_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/
static volatile uint16_t counterPrintTick = 0;
static volatile uint32_t printCounter = 0;
static volatile bool flag = false;
static volatile uint8_t pwmDutyCycle = 0;
static uint16_t s_lastSw2Raw = 4095U;
static ili9341_t s_lcd;
static uint16_t s_lcdWidth;
static uint16_t s_lcdHeight;
static uint16_t s_centerX;
static uint16_t s_nearY;
static int32_t s_obstacleZQ8[GAME_OBSTACLE_COUNT];
static int32_t s_prevObstacleZQ8[GAME_OBSTACLE_COUNT];
static uint8_t s_obstacleLane[GAME_OBSTACLE_COUNT];
static uint8_t s_prevObstacleLane[GAME_OBSTACLE_COUNT];
static int16_t s_prevObstacleX[GAME_OBSTACLE_COUNT];
static int16_t s_prevObstacleY[GAME_OBSTACLE_COUNT];
static int16_t s_prevObstacleW[GAME_OBSTACLE_COUNT];
static int16_t s_prevObstacleH[GAME_OBSTACLE_COUNT];
static uint8_t s_playerLane;
static uint8_t s_prevPlayerLane;
static int16_t s_prevPlayerX;
static int16_t s_prevPlayerY;
static int16_t s_prevPlayerW;
static int16_t s_prevPlayerH;
static uint32_t s_score;
static uint32_t s_frame;
static bool s_gameOver;
static bool s_sw2WasPressed;
static bool s_sw3WasPressed;
static uint32_t s_prevScore;
static bool s_prevGameOver;
static bool s_fullRedrawNeeded;
static app_screen_t s_appScreen;
static uint8_t s_selectedPlayer;
static uint8_t s_selectedGame;
static bool s_menuRedrawNeeded;
static bool s_photoRequested;
static bool s_photoDoneShown;
static bool s_touchWasPressed;
static uint16_t s_touchLastX;
static uint16_t s_touchLastY;
static uint32_t s_touchCsCandidateIndex;
static int16_t s_flappyBirdYQ4;
static int16_t s_flappyVelocityQ4;
static int16_t s_flappyPipeX[2];
static int16_t s_flappyGapY[2];
static bool s_flappyPipeScored[2];
static uint32_t s_flappyScore;
static uint32_t s_flappyFrame;
static bool s_flappyGameOver;
static bool s_flappyFullRedrawNeeded;
static int16_t s_flappyPrevBirdY;
static int16_t s_flappyPrevPipeX[2];
static uint32_t s_flappyPrevScore;
static bool s_flappyPrevGameOver;
static int16_t s_dinoYQ4;
static int16_t s_dinoVelocityQ4;
static int16_t s_dinoCactusX;
static uint32_t s_dinoScore;
static uint32_t s_dinoFrame;
static bool s_dinoGameOver;
static bool s_dinoFullRedrawNeeded;
static int16_t s_dinoPrevY;
static int16_t s_dinoPrevCactusX;
static uint32_t s_dinoPrevScore;
static bool s_dinoPrevGameOver;
static int16_t s_stackBaseX;
static int16_t s_stackBaseW;
static int16_t s_stackActiveX;
static int16_t s_stackPrevActiveX;
static int16_t s_stackActiveY;
static int16_t s_stackPrevActiveY;
static int16_t s_stackTargetY;
static int16_t s_stackPrevBaseX;
static uint8_t s_stackSpeed;
static uint8_t s_stackLevel;
static uint32_t s_stackScore;
static uint32_t s_stackBestScore;
static uint32_t s_stackCountdownFrame;
static uint32_t s_stackMessageFrame;
static bool s_stackGameOver;
static bool s_stackFullRedrawNeeded;
static bool s_stackRunning;

/*******************************************************************************
 * Code
 ******************************************************************************/

void SysTick_Handler(void)
{
    counterPrintTick++;
    if (counterPrintTick >= ADC_PRINT_TICKS)
    {
        counterPrintTick = 0;
        flag = true;
        printCounter++;
        GPIO_PortToggle(BOARD_LED_GPIO, 1u << BOARD_LED_GPIO_PIN);
    }

    PWM0_LED_UpdateFade();
}

static void PWM0_LED_InitOutput(void)
{
    PWM_SetupFaultDisableMap(FLEXPWM0_PERIPHERAL, FLEXPWM0_SM0, FLEXPWM0_SM0_A, kPWM_faultchannel_0, 0U);
    PWM_OutputEnable(FLEXPWM0_PERIPHERAL, FLEXPWM0_SM0_A, FLEXPWM0_SM0);
    PWM_SetPwmLdok(FLEXPWM0_PERIPHERAL, kPWM_Control_Module_0, true);
}

static void PWM0_LED_UpdateFade(void)
{
    PWM_UpdatePwmDutycycle(FLEXPWM0_PERIPHERAL, FLEXPWM0_SM0, FLEXPWM0_SM0_A, kPWM_SignedCenterAligned,
                           pwmDutyCycle);
    PWM_SetPwmLdok(FLEXPWM0_PERIPHERAL, kPWM_Control_Module_0, true);

    pwmDutyCycle++;
    if (pwmDutyCycle >= PWM_FADE_DUTY_MAX)
    {
        pwmDutyCycle = 0;
    }
}

static void SW2_ADC_InitInput(void)
{
    lpadc_conv_command_config_t commandConfig;
    lpadc_conv_trigger_config_t triggerConfig;

    LPADC_GetDefaultConvCommandConfig(&commandConfig);
    commandConfig.channelNumber = ADC_SW2_CHANNEL;
    LPADC_SetConvCommandConfig(ADC0_PERIPHERAL, ADC_COMMAND_ID, &commandConfig);

    LPADC_GetDefaultConvTriggerConfig(&triggerConfig);
    triggerConfig.targetCommandId = ADC_COMMAND_ID;
    triggerConfig.enableHardwareTrigger = false;
    LPADC_SetConvTriggerConfig(ADC0_PERIPHERAL, ADC_TRIGGER_ID, &triggerConfig);
}

static uint16_t SW2_ADC_ReadRaw(void)
{
    lpadc_conv_result_t result;
    uint32_t timeout = 1000U;

    LPADC_DoSoftwareTrigger(ADC0_PERIPHERAL, 1UL << ADC_TRIGGER_ID);
    while (!LPADC_GetConvResult(ADC0_PERIPHERAL, &result))
    {
        if (timeout-- == 0U)
        {
            return s_lastSw2Raw;
        }
    }

    s_lastSw2Raw = (uint16_t)result.convValue;
    return s_lastSw2Raw;
}

static bool SW2_IsPressed(void)
{
    return SW2_ADC_ReadRaw() < GAME_SW2_PRESSED_THRESHOLD;
}

static bool SW3_IsPressed(void)
{
    return GPIO_PinRead(BOARD_INITPINS_SW3_GPIO, BOARD_INITPINS_SW3_GPIO_PIN) == SW3_PRESSED_LEVEL;
}

static bool Button_DebouncedRead(bool (*readPressed)(void), bool previousPressed)
{
    bool pressed = readPressed();

    if (pressed != previousPressed)
    {
        SDK_DelayAtLeastUs(BUTTON_DEBOUNCE_US, CLOCK_GetCoreSysClkFreq());
        pressed = readPressed();
    }

    return pressed;
}

static void Game_FillRectClipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    int16_t x0 = x;
    int16_t y0 = y;
    int16_t x1 = (int16_t)(x + w);
    int16_t y1 = (int16_t)(y + h);

    if ((w <= 0) || (h <= 0) || (x1 <= 0) || (y1 <= 0) || (x0 >= (int16_t)s_lcdWidth) ||
        (y0 >= (int16_t)s_lcdHeight))
    {
        return;
    }

    if (x0 < 0)
    {
        x0 = 0;
    }
    if (y0 < 0)
    {
        y0 = 0;
    }
    if (x1 > (int16_t)s_lcdWidth)
    {
        x1 = (int16_t)s_lcdWidth;
    }
    if (y1 > (int16_t)s_lcdHeight)
    {
        y1 = (int16_t)s_lcdHeight;
    }

    if ((x1 > x0) && (y1 > y0))
    {
        (void)ili9341_fill_rect(&s_lcd, (uint16_t)x0, (uint16_t)y0, (uint16_t)(x1 - x0), (uint16_t)(y1 - y0), color);
    }
}

static uint16_t Game_CurrentSpeedQ8(void)
{
    uint32_t speed = GAME_OBSTACLE_BASE_SPEED_Q8 + (s_score * 12U);

    if (speed > GAME_OBSTACLE_MAX_SPEED_Q8)
    {
        speed = GAME_OBSTACLE_MAX_SPEED_Q8;
    }

    return (uint16_t)speed;
}

static uint16_t Game_RoadHalfWidthAtY(uint16_t y)
{
    const uint16_t span = (uint16_t)((s_nearY > GAME_HORIZON_Y) ? (s_nearY - GAME_HORIZON_Y) : 1U);
    uint16_t offset;
    uint32_t width;

    if (y <= GAME_HORIZON_Y)
    {
        return GAME_ROAD_FAR_HALF_W;
    }
    if (y >= s_nearY)
    {
        return GAME_ROAD_NEAR_HALF_W;
    }

    offset = (uint16_t)(y - GAME_HORIZON_Y);
    width = GAME_ROAD_FAR_HALF_W;
    width += (((uint32_t)(GAME_ROAD_NEAR_HALF_W - GAME_ROAD_FAR_HALF_W) * offset) / span);

    return (uint16_t)width;
}

static int16_t Game_LaneCenterAtY(uint8_t lane, uint16_t y)
{
    const int16_t halfWidth = (int16_t)Game_RoadHalfWidthAtY(y);
    const int16_t laneWidth = (int16_t)((halfWidth * 2) / (int16_t)GAME_LANE_COUNT);
    const int16_t left = (int16_t)s_centerX - halfWidth;

    if (lane >= GAME_LANE_COUNT)
    {
        lane = GAME_CENTER_LANE;
    }

    return (int16_t)(left + ((int16_t)laneWidth * (int16_t)lane) + (laneWidth / 2));
}

static uint16_t Game_ProjectTQ8(int32_t zQ8)
{
    const int32_t zRange = GAME_OBSTACLE_MAX_Z_Q8 - GAME_OBSTACLE_MIN_Z_Q8;
    int32_t tQ8;

    if (zQ8 < GAME_OBSTACLE_MIN_Z_Q8)
    {
        zQ8 = GAME_OBSTACLE_MIN_Z_Q8;
    }
    if (zQ8 > GAME_OBSTACLE_MAX_Z_Q8)
    {
        zQ8 = GAME_OBSTACLE_MAX_Z_Q8;
    }

    tQ8 = (int32_t)(((uint32_t)(GAME_OBSTACLE_MAX_Z_Q8 - zQ8) * 256U) / (uint32_t)zRange);
    return (uint16_t)(((uint32_t)tQ8 * (uint32_t)tQ8) / 256U);
}

static uint16_t Game_ProjectY(int32_t zQ8)
{
    const uint16_t yRange = (uint16_t)(s_nearY - GAME_HORIZON_Y);
    const uint16_t perspectiveQ8 = Game_ProjectTQ8(zQ8);
    const uint32_t y = GAME_HORIZON_Y + (((uint32_t)perspectiveQ8 * yRange) / 256U);

    return (uint16_t)y;
}

static uint16_t Game_ProjectScale(int32_t zQ8)
{
    const uint16_t perspectiveQ8 = Game_ProjectTQ8(zQ8);
    return (uint16_t)(1U + (((uint32_t)perspectiveQ8 * 4U) / 256U));
}

static uint8_t Game_RandomLane(uint32_t seed)
{
    return (uint8_t)(seed % GAME_LANE_COUNT);
}

static void Game_GetObstacleMainRect(int32_t zQ8, uint8_t lane, int16_t *x, int16_t *y, int16_t *w, int16_t *h)
{
    const uint16_t screenY = Game_ProjectY(zQ8);
    const uint16_t scale = Game_ProjectScale(zQ8);
    const int16_t centerX = Game_LaneCenterAtY(lane, screenY);

    *w = (int16_t)(10U * scale);
    *h = (int16_t)(9U * scale);
    *x = (int16_t)(centerX - (*w / 2));
    *y = (int16_t)screenY - *h;
}

static void Game_GetObstacleRect(int32_t zQ8, uint8_t lane, int16_t *x, int16_t *y, int16_t *w, int16_t *h)
{
    int16_t mainX;
    int16_t mainY;
    int16_t mainW;
    int16_t mainH;
    const int16_t scale = (int16_t)Game_ProjectScale(zQ8);

    Game_GetObstacleMainRect(zQ8, lane, &mainX, &mainY, &mainW, &mainH);

    if (scale <= 1)
    {
        *x = mainX;
        *y = mainY;
        *w = mainW;
        *h = mainH;
        return;
    }

    const int16_t barX = (int16_t)(mainX - (2 * scale));
    const int16_t barY = (int16_t)(mainY + (3 * scale));
    const int16_t barW = (int16_t)(mainW + (4 * scale));
    const int16_t barH = (int16_t)(2 * scale);
    const int16_t right = (int16_t)(((mainX + mainW) > (barX + barW)) ? (mainX + mainW) : (barX + barW));
    const int16_t bottom = (int16_t)(((mainY + mainH) > (barY + barH)) ? (mainY + mainH) : (barY + barH));

    *x = (mainX < barX) ? mainX : barX;
    *y = (mainY < barY) ? mainY : barY;
    *w = (int16_t)(right - *x);
    *h = (int16_t)(bottom - *y);
}

static void Game_ResetObstacle(uint32_t index, uint32_t seed)
{
    const int32_t spacingQ8 = ((GAME_OBSTACLE_MAX_Z_Q8 - GAME_OBSTACLE_MIN_Z_Q8) / (int32_t)GAME_OBSTACLE_COUNT);
    int32_t zQ8 = GAME_OBSTACLE_MAX_Z_Q8 + ((int32_t)index * spacingQ8);

    zQ8 += (int32_t)((seed % 45U) * 256U);
    s_obstacleZQ8[index] = zQ8;
    s_obstacleLane[index] = Game_RandomLane((seed * 37U) + (index * 11U));
}

static void Game_Reset(void)
{
    s_centerX = (uint16_t)(s_lcdWidth / 2U);
    s_nearY = (uint16_t)(s_lcdHeight - 10U);
    s_playerLane = GAME_CENTER_LANE;
    s_prevPlayerLane = s_playerLane;
    s_score = 0;
    s_frame = 0;
    s_gameOver = false;
    s_prevScore = s_score;
    s_prevGameOver = s_gameOver;
    s_fullRedrawNeeded = true;

    for (uint32_t i = 0U; i < GAME_OBSTACLE_COUNT; i++)
    {
        Game_ResetObstacle(i, (i * 23U) + 5U);
        s_prevObstacleZQ8[i] = s_obstacleZQ8[i];
        s_prevObstacleLane[i] = s_obstacleLane[i];
        Game_GetObstacleRect(s_obstacleZQ8[i],
                             s_obstacleLane[i],
                             &s_prevObstacleX[i],
                             &s_prevObstacleY[i],
                             &s_prevObstacleW[i],
                             &s_prevObstacleH[i]);
    }
    Game_GetPlayerRect(s_playerLane, &s_prevPlayerX, &s_prevPlayerY, &s_prevPlayerW, &s_prevPlayerH);

    (void)ili9341_fill_screen(&s_lcd, GAME_SKY_COLOR);
}

static void Game_DrawRoad(void)
{
    (void)ili9341_fill_rect(&s_lcd, 0U, 0U, s_lcdWidth, GAME_HORIZON_Y, GAME_SKY_COLOR);
    Game_FillRectClipped(0, (int16_t)GAME_HORIZON_Y, (int16_t)s_lcdWidth, (int16_t)(s_lcdHeight - GAME_HORIZON_Y),
                         GAME_FLOOR_COLOR);
    for (uint16_t y = GAME_HORIZON_Y; y < s_lcdHeight; y = (uint16_t)(y + 10U))
    {
        const uint16_t halfWidth = Game_RoadHalfWidthAtY(y);
        const uint16_t stripH = ((uint16_t)(y + 10U) > s_lcdHeight) ? (uint16_t)(s_lcdHeight - y) : 10U;
        Game_FillRectClipped((int16_t)(s_centerX - halfWidth), (int16_t)y, (int16_t)(halfWidth * 2U), (int16_t)stripH,
                             GAME_ROAD_COLOR);
    }
}

static void Game_GetRectUnion(int16_t ax,
                              int16_t ay,
                              int16_t aw,
                              int16_t ah,
                              int16_t bx,
                              int16_t by,
                              int16_t bw,
                              int16_t bh,
                              int16_t *x,
                              int16_t *y,
                              int16_t *w,
                              int16_t *h)
{
    const int16_t ar = (int16_t)(ax + aw);
    const int16_t ab = (int16_t)(ay + ah);
    const int16_t br = (int16_t)(bx + bw);
    const int16_t bb = (int16_t)(by + bh);
    const int16_t right = (ar > br) ? ar : br;
    const int16_t bottom = (ab > bb) ? ab : bb;

    *x = (ax < bx) ? ax : bx;
    *y = (ay < by) ? ay : by;
    *w = (int16_t)(right - *x);
    *h = (int16_t)(bottom - *y);
}

static void Game_ClearGameplayRect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t clearX = (int16_t)(x - GAME_DIRTY_PAD);
    int16_t clearY = (int16_t)(y - GAME_DIRTY_PAD);
    int16_t clearW = (int16_t)(w + (2 * GAME_DIRTY_PAD));
    int16_t clearH = (int16_t)(h + (2 * GAME_DIRTY_PAD));
    int16_t clearBottom;

    if ((clearW <= 0) || (clearH <= 0))
    {
        return;
    }

    clearBottom = (int16_t)(clearY + clearH);
    if (clearBottom <= (int16_t)GAME_HORIZON_Y)
    {
        Game_FillRectClipped(clearX, clearY, clearW, clearH, GAME_SKY_COLOR);
        return;
    }

    if (clearY < (int16_t)GAME_HORIZON_Y)
    {
        const int16_t skyH = (int16_t)(GAME_HORIZON_Y - clearY);
        Game_FillRectClipped(clearX, clearY, clearW, skyH, GAME_SKY_COLOR);
        clearY = (int16_t)GAME_HORIZON_Y;
    }

    for (int16_t stripY = clearY; stripY < clearBottom; stripY = (int16_t)(stripY + 5))
    {
        int16_t stripH = 5;
        const uint16_t halfWidth = Game_RoadHalfWidthAtY((uint16_t)stripY);
        const int16_t roadLeft = (int16_t)(s_centerX - halfWidth);
        const int16_t roadRight = (int16_t)(s_centerX + halfWidth);
        const int16_t clearRight = (int16_t)(clearX + clearW);
        const int16_t drawLeft = (clearX > roadLeft) ? clearX : roadLeft;
        const int16_t drawRight = (clearRight < roadRight) ? clearRight : roadRight;

        if ((stripY + stripH) > clearBottom)
        {
            stripH = (int16_t)(clearBottom - stripY);
        }

        Game_FillRectClipped(clearX, stripY, clearW, stripH, GAME_FLOOR_COLOR);
        if (drawRight > drawLeft)
        {
            Game_FillRectClipped(drawLeft, stripY, (int16_t)(drawRight - drawLeft), stripH, GAME_ROAD_COLOR);
        }
    }
}

static void Game_GetPlayerRect(uint8_t lane, int16_t *x, int16_t *y, int16_t *w, int16_t *h)
{
    const int16_t playerX = Game_LaneCenterAtY(lane, s_nearY);
    const int16_t baseY = (int16_t)(s_nearY - 6U);

    *w = GAME_PLAYER_BASE_W;
    *h = GAME_PLAYER_BASE_H;
    *x = (int16_t)(playerX - (GAME_PLAYER_BASE_W / 2));
    *y = (int16_t)(baseY - GAME_PLAYER_BASE_H);
}

static void Game_ErasePlayerRect(void)
{
    Game_ClearGameplayRect(s_prevPlayerX, s_prevPlayerY, s_prevPlayerW, s_prevPlayerH);
}

static void Game_DrawPlayerAtLane(uint8_t lane)
{
    int16_t left;
    int16_t bodyTop;
    int16_t w;
    int16_t h;

    Game_GetPlayerRect(lane, &left, &bodyTop, &w, &h);
    (void)w;
    (void)h;

    Game_FillRectClipped((int16_t)(left + 6), (int16_t)(bodyTop + 9), 10, 15, GAME_PLAYER_COLOR);
    Game_FillRectClipped((int16_t)(left + 7), bodyTop, 8, 8, GAME_PLAYER_COLOR);
    Game_FillRectClipped((int16_t)(left + 2), (int16_t)(bodyTop + 13), 5, 10, GAME_PLAYER_COLOR);
    Game_FillRectClipped((int16_t)(left + 15), (int16_t)(bodyTop + 13), 5, 10, GAME_PLAYER_COLOR);
    Game_FillRectClipped((int16_t)(left + 5), (int16_t)(bodyTop + 24), 5, 8, GAME_PLAYER_COLOR);
    Game_FillRectClipped((int16_t)(left + 13), (int16_t)(bodyTop + 24), 5, 8, GAME_PLAYER_COLOR);
}

static void Game_DrawPlayer(void)
{
    Game_DrawPlayerAtLane(s_playerLane);
}

static void Game_DrawObstacle(uint32_t index)
{
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    const uint16_t scale = Game_ProjectScale(s_obstacleZQ8[index]);

    Game_GetObstacleMainRect(s_obstacleZQ8[index], s_obstacleLane[index], &x, &y, &w, &h);
    Game_FillRectClipped(x, y, w, h, GAME_OBSTACLE_COLOR);
    if (scale > 1U)
    {
        Game_FillRectClipped((int16_t)(x - (2 * (int16_t)scale)), (int16_t)(y + (3 * (int16_t)scale)),
                             (int16_t)(w + (4 * (int16_t)scale)), (int16_t)(2U * scale), GAME_OBSTACLE_COLOR);
    }
}

static void Game_DrawScore(void)
{
    char scoreText[16];

    (void)ili9341_fill_rect(&s_lcd, 0U, 0U, GAME_SCORE_W, GAME_SCORE_H, GAME_SKY_COLOR);
    (void)snprintf(scoreText, sizeof(scoreText), "%lu", (unsigned long)s_score);
    (void)ili9341_write_string(&s_lcd, 6U, 6U, scoreText, GAME_TEXT_COLOR, GAME_SKY_COLOR, 2U);
}

static void Game_DrawGameOverOverlay(void)
{
    (void)ili9341_fill_rect(&s_lcd, GAME_OVER_X, GAME_OVER_Y, GAME_OVER_W, GAME_OVER_H, GAME_ROAD_COLOR);
    (void)ili9341_write_string(&s_lcd, 90U, 88U, "GAME OVER", ILI9341_COLOR_RED, GAME_ROAD_COLOR, 2U);
    (void)ili9341_write_string(&s_lcd, 90U, 112U, "SW2/SW3", GAME_TEXT_COLOR, GAME_ROAD_COLOR, 2U);
}

static void Game_DrawScene(void)
{
    bool drawn[GAME_OBSTACLE_COUNT] = {false};

    if (s_fullRedrawNeeded)
    {
        Game_DrawRoad();
        Game_DrawScore();
        s_fullRedrawNeeded = false;
    }
    else
    {
        for (uint32_t i = 0U; i < GAME_OBSTACLE_COUNT; i++)
        {
            int16_t currentX;
            int16_t currentY;
            int16_t currentW;
            int16_t currentH;
            int16_t unionX;
            int16_t unionY;
            int16_t unionW;
            int16_t unionH;

            Game_GetObstacleRect(s_obstacleZQ8[i], s_obstacleLane[i], &currentX, &currentY, &currentW, &currentH);
            Game_GetRectUnion(s_prevObstacleX[i],
                              s_prevObstacleY[i],
                              s_prevObstacleW[i],
                              s_prevObstacleH[i],
                              currentX,
                              currentY,
                              currentW,
                              currentH,
                              &unionX,
                              &unionY,
                              &unionW,
                              &unionH);
            Game_ClearGameplayRect(unionX, unionY, unionW, unionH);
        }
        if (s_playerLane != s_prevPlayerLane)
        {
            Game_ErasePlayerRect();
        }
        if (s_score != s_prevScore)
        {
            Game_DrawScore();
        }
    }

    for (uint32_t pass = 0U; pass < GAME_OBSTACLE_COUNT; pass++)
    {
        int32_t farthestZ = -1;
        uint32_t farthestIndex = 0U;

        for (uint32_t i = 0U; i < GAME_OBSTACLE_COUNT; i++)
        {
            if (!drawn[i] && (s_obstacleZQ8[i] > farthestZ))
            {
                farthestZ = s_obstacleZQ8[i];
                farthestIndex = i;
            }
        }
        Game_DrawObstacle(farthestIndex);
        drawn[farthestIndex] = true;
    }

    Game_DrawPlayer();
    if (s_gameOver && !s_prevGameOver)
    {
        Game_DrawGameOverOverlay();
    }

    for (uint32_t i = 0U; i < GAME_OBSTACLE_COUNT; i++)
    {
        s_prevObstacleZQ8[i] = s_obstacleZQ8[i];
        s_prevObstacleLane[i] = s_obstacleLane[i];
        Game_GetObstacleRect(s_obstacleZQ8[i],
                             s_obstacleLane[i],
                             &s_prevObstacleX[i],
                             &s_prevObstacleY[i],
                             &s_prevObstacleW[i],
                             &s_prevObstacleH[i]);
    }
    s_prevPlayerLane = s_playerLane;
    Game_GetPlayerRect(s_playerLane, &s_prevPlayerX, &s_prevPlayerY, &s_prevPlayerW, &s_prevPlayerH);
    s_prevScore = s_score;
    s_prevGameOver = s_gameOver;
}

static bool Game_HandleInput(bool moveLeftEdge, bool moveRightEdge)
{
    uint8_t oldLane = s_playerLane;

    if (s_gameOver)
    {
        if (moveLeftEdge || moveRightEdge)
        {
            Game_Reset();
            return true;
        }
        return false;
    }

    if (moveLeftEdge && (s_playerLane > 0U))
    {
        s_playerLane--;
    }
    if (moveRightEdge && (s_playerLane < (GAME_LANE_COUNT - 1U)))
    {
        s_playerLane++;
    }

    return s_playerLane != oldLane;
}

static bool Game_CheckCollision(void)
{
    for (uint32_t i = 0U; i < GAME_OBSTACLE_COUNT; i++)
    {
        if ((s_obstacleLane[i] == s_playerLane) &&
            (s_obstacleZQ8[i] < (GAME_OBSTACLE_MIN_Z_Q8 + (18 * 256))))
        {
            return true;
        }
    }

    return false;
}

static void Game_UpdateWorld(void)
{
    if (s_gameOver)
    {
        return;
    }

    for (uint32_t i = 0U; i < GAME_OBSTACLE_COUNT; i++)
    {
        s_obstacleZQ8[i] -= (int32_t)Game_CurrentSpeedQ8();
        if (s_obstacleZQ8[i] < GAME_OBSTACLE_MIN_Z_Q8)
        {
            s_score++;
            Game_ResetObstacle(i, s_frame + s_score + i);
        }
    }

    if (Game_CheckCollision())
    {
        s_gameOver = true;
    }

    s_frame++;
}

static void Flappy_ResetPipe(uint32_t index, int16_t x)
{
    s_flappyPipeX[index] = x;
    s_flappyGapY[index] = (int16_t)(54 + ((s_flappyFrame * 31U + index * 53U + s_flappyScore * 17U) % 82U));
    s_flappyPipeScored[index] = false;
}

static void Flappy_Reset(void)
{
    s_flappyBirdYQ4 = (int16_t)(92 * 16);
    s_flappyVelocityQ4 = 0;
    s_flappyScore = 0;
    s_flappyFrame = 0;
    s_flappyGameOver = false;
    s_flappyFullRedrawNeeded = true;
    s_flappyPrevBirdY = -100;
    s_flappyPrevPipeX[0] = -100;
    s_flappyPrevPipeX[1] = -100;
    s_flappyPrevScore = UINT32_MAX;
    s_flappyPrevGameOver = false;
    Flappy_ResetPipe(0U, (int16_t)s_lcdWidth);
    Flappy_ResetPipe(1U, (int16_t)(s_lcdWidth + 150U));
    Flappy_DrawScene();
}

static bool Flappy_CheckCollision(void)
{
    const int16_t birdX = FLAPPY_BIRD_X;
    const int16_t birdY = (int16_t)(s_flappyBirdYQ4 / 16);
    const int16_t birdR = (int16_t)(FLAPPY_BIRD_SIZE / 2);

    if ((birdY - birdR) <= 0 || (birdY + birdR) >= FLAPPY_GROUND_Y)
    {
        return true;
    }

    for (uint32_t i = 0U; i < 2U; i++)
    {
        const int16_t pipeX = s_flappyPipeX[i];
        const int16_t pipeRight = (int16_t)(pipeX + FLAPPY_PIPE_W);
        const int16_t gapTop = s_flappyGapY[i];
        const int16_t gapBottom = (int16_t)(gapTop + FLAPPY_GAP_H);
        const bool xOverlap = ((birdX + birdR) >= pipeX) && ((birdX - birdR) <= pipeRight);
        const bool yBlocked = ((birdY - birdR) <= gapTop) || ((birdY + birdR) >= gapBottom);

        if (xOverlap && yBlocked)
        {
            return true;
        }
    }

    return false;
}

static void Flappy_UpdateWorld(void)
{
    if (s_flappyGameOver)
    {
        return;
    }

    s_flappyVelocityQ4 = (int16_t)(s_flappyVelocityQ4 + FLAPPY_GRAVITY_Q4);
    if (s_flappyVelocityQ4 > 90)
    {
        s_flappyVelocityQ4 = 90;
    }
    s_flappyBirdYQ4 = (int16_t)(s_flappyBirdYQ4 + s_flappyVelocityQ4);

    for (uint32_t i = 0U; i < 2U; i++)
    {
        s_flappyPipeX[i] = (int16_t)(s_flappyPipeX[i] - FLAPPY_SCROLL_SPEED);
        if (!s_flappyPipeScored[i] && ((s_flappyPipeX[i] + FLAPPY_PIPE_W) < FLAPPY_BIRD_X))
        {
            s_flappyScore++;
            s_flappyPipeScored[i] = true;
        }
        if ((s_flappyPipeX[i] + FLAPPY_PIPE_W) < 0)
        {
            Flappy_ResetPipe(i, (int16_t)(s_lcdWidth + 20U));
        }
    }

    if (Flappy_CheckCollision())
    {
        s_flappyGameOver = true;
    }
    s_flappyFrame++;
}

static void Flappy_HandleInput(bool flapPressed, bool flapEdge, bool backEdge)
{
    if (backEdge)
    {
        s_appScreen = kAppScreenGameSelect;
        s_menuRedrawNeeded = true;
        return;
    }

    if (flapEdge)
    {
        if (s_flappyGameOver)
        {
            Flappy_Reset();
        }
        else
        {
            s_flappyVelocityQ4 = FLAPPY_FLAP_VELOCITY_Q4;
        }
    }
    else if (flapPressed && !s_flappyGameOver)
    {
        s_flappyVelocityQ4 = FLAPPY_FLAP_VELOCITY_Q4;
    }
}

static void Flappy_ClearRectAvoidBird(int16_t x, int16_t y, int16_t w, int16_t h, int16_t birdY)
{
    const int16_t clearRight = (int16_t)(x + w);
    const int16_t clearBottom = (int16_t)(y + h);
    const int16_t birdLeft = (int16_t)(FLAPPY_BIRD_X - 18);
    const int16_t birdRight = (int16_t)(FLAPPY_BIRD_X + 20);
    const int16_t birdTop = (int16_t)(birdY - 13);
    const int16_t birdBottom = (int16_t)(birdY + 13);
    const bool overlapsBird = (x < birdRight) && (clearRight > birdLeft) && (y < birdBottom) && (clearBottom > birdTop);

    if (!overlapsBird)
    {
        Game_FillRectClipped(x, y, w, h, FLAPPY_SKY_COLOR);
        return;
    }

    Game_FillRectClipped(x, y, w, (int16_t)(birdTop - y), FLAPPY_SKY_COLOR);
    Game_FillRectClipped(x, birdBottom, w, (int16_t)(clearBottom - birdBottom), FLAPPY_SKY_COLOR);
    Game_FillRectClipped(x, birdTop, (int16_t)(birdLeft - x), (int16_t)(birdBottom - birdTop), FLAPPY_SKY_COLOR);
    Game_FillRectClipped(birdRight, birdTop, (int16_t)(clearRight - birdRight), (int16_t)(birdBottom - birdTop),
                         FLAPPY_SKY_COLOR);
}

static void Flappy_EraseOldBirdTrail(int16_t oldY, int16_t newY)
{
    const int16_t birdX = (int16_t)(FLAPPY_BIRD_X - 16);
    const int16_t birdW = 34;
    const int16_t oldTop = (int16_t)(oldY - 10);
    const int16_t oldBottom = (int16_t)(oldY + 12);
    const int16_t newTop = (int16_t)(newY - 10);
    const int16_t newBottom = (int16_t)(newY + 12);

    if (oldY < 0)
    {
        return;
    }

    if (newTop > oldTop)
    {
        Game_FillRectClipped(birdX, oldTop, birdW, (int16_t)(newTop - oldTop), FLAPPY_SKY_COLOR);
    }
    if (newBottom < oldBottom)
    {
        Game_FillRectClipped(birdX, newBottom, birdW, (int16_t)(oldBottom - newBottom), FLAPPY_SKY_COLOR);
    }
}

static void Flappy_FillPipeSlicePart(int16_t rectX,
                                     int16_t rectY,
                                     int16_t rectW,
                                     int16_t rectH,
                                     int16_t sliceX,
                                     int16_t sliceW)
{
    int16_t drawX = rectX;
    int16_t drawRight = (int16_t)(rectX + rectW);
    const int16_t sliceRight = (int16_t)(sliceX + sliceW);

    if (drawX < sliceX)
    {
        drawX = sliceX;
    }
    if (drawRight > sliceRight)
    {
        drawRight = sliceRight;
    }
    if (drawRight > drawX)
    {
        Game_FillRectClipped(drawX, rectY, (int16_t)(drawRight - drawX), rectH, FLAPPY_PIPE_COLOR);
    }
}

static void Flappy_DrawPipeSlice(int16_t pipeX, int16_t gapTop, int16_t sliceX, int16_t sliceW)
{
    const int16_t gapBottom = (int16_t)(gapTop + FLAPPY_GAP_H);

    Flappy_FillPipeSlicePart(pipeX, 0, FLAPPY_PIPE_W, gapTop, sliceX, sliceW);
    Flappy_FillPipeSlicePart((int16_t)(pipeX - 3), (int16_t)(gapTop - 10), (int16_t)(FLAPPY_PIPE_W + 6), 10, sliceX,
                             sliceW);
    Flappy_FillPipeSlicePart(pipeX, gapBottom, FLAPPY_PIPE_W, (int16_t)(FLAPPY_GROUND_Y - gapBottom), sliceX, sliceW);
    Flappy_FillPipeSlicePart((int16_t)(pipeX - 3), gapBottom, (int16_t)(FLAPPY_PIPE_W + 6), 10, sliceX, sliceW);
}

static void Flappy_DrawScoreBar(void)
{
    char scoreText[20];

    (void)snprintf(scoreText, sizeof(scoreText), "SCOR %lu", (unsigned long)s_flappyScore);
    (void)ili9341_write_string(&s_lcd, 8U, 7U, scoreText, ILI9341_COLOR_WHITE, FLAPPY_SKY_COLOR, 2U);
}

static void Flappy_DrawGameOverScreen(void)
{
    char scoreText[24];

    (void)ili9341_fill_screen(&s_lcd, FLAPPY_TEXT_BG);
    (void)ili9341_write_string(&s_lcd, 50U, 42U, "GAME OVER", ILI9341_COLOR_RED, FLAPPY_TEXT_BG, 3U);
    (void)snprintf(scoreText, sizeof(scoreText), "SCOR: %lu", (unsigned long)s_flappyScore);
    (void)ili9341_write_string(&s_lcd, 78U, 100U, scoreText, ILI9341_COLOR_WHITE, FLAPPY_TEXT_BG, 2U);

    (void)ili9341_fill_rect(&s_lcd, 38U, 148U, 244U, 34U, MENU_SELECTED_COLOR);
    (void)ili9341_write_string(&s_lcd, 62U, 157U, "SW2 RESTART", MENU_BG_COLOR, MENU_SELECTED_COLOR, 2U);
    (void)ili9341_fill_rect(&s_lcd, 38U, 196U, 244U, 34U, MENU_NORMAL_COLOR);
    (void)ili9341_write_string(&s_lcd, 72U, 205U, "SW3 MENIU", MENU_BG_COLOR, MENU_NORMAL_COLOR, 2U);
}

static void Flappy_DrawScene(void)
{
    const int16_t birdY = (int16_t)(s_flappyBirdYQ4 / 16);
    const bool fullRedraw = s_flappyFullRedrawNeeded;
    bool scoreRedrawNeeded = fullRedraw || (s_flappyScore != s_flappyPrevScore);

    if (s_flappyGameOver)
    {
        if (!s_flappyPrevGameOver)
        {
            Flappy_DrawGameOverScreen();
        }
        s_flappyPrevGameOver = true;
        return;
    }

    if (fullRedraw)
    {
        (void)ili9341_fill_screen(&s_lcd, FLAPPY_SKY_COLOR);
        (void)ili9341_fill_rect(&s_lcd, 0U, FLAPPY_GROUND_Y, s_lcdWidth, (uint16_t)(s_lcdHeight - FLAPPY_GROUND_Y),
                                FLAPPY_GROUND_COLOR);
        (void)ili9341_fill_rect(&s_lcd, 0U, (uint16_t)(FLAPPY_GROUND_Y - 4), s_lcdWidth, 4U, 0x07E0U);
        s_flappyFullRedrawNeeded = false;
    }
    else if (!s_flappyGameOver || !s_flappyPrevGameOver)
    {
        for (uint32_t i = 0U; i < 2U; i++)
        {
            const int16_t oldRight = (int16_t)(s_flappyPrevPipeX[i] + FLAPPY_PIPE_W + 3);
            const int16_t newRight = (int16_t)(s_flappyPipeX[i] + FLAPPY_PIPE_W + 3);
            if ((s_flappyPipeX[i] < s_flappyPrevPipeX[i]) && ((s_flappyPrevPipeX[i] - s_flappyPipeX[i]) <= 12))
            {
                Flappy_ClearRectAvoidBird(newRight, 0, (int16_t)(oldRight - newRight), FLAPPY_GROUND_Y,
                                          s_flappyPrevBirdY);
            }
            else
            {
                Flappy_ClearRectAvoidBird((int16_t)(s_flappyPrevPipeX[i] - 4), 0, (int16_t)(FLAPPY_PIPE_W + 8),
                                          FLAPPY_GROUND_Y, s_flappyPrevBirdY);
            }
        }
        Flappy_EraseOldBirdTrail(s_flappyPrevBirdY, birdY);
        (void)ili9341_fill_rect(&s_lcd, 0U, (uint16_t)(FLAPPY_GROUND_Y - 4), s_lcdWidth, 4U, 0x07E0U);
    }

    for (uint32_t i = 0U; i < 2U; i++)
    {
        const int16_t pipeX = s_flappyPipeX[i];
        const int16_t gapTop = s_flappyGapY[i];
        const int16_t delta = (int16_t)(s_flappyPrevPipeX[i] - pipeX);
        if (fullRedraw || (delta <= 0) || (delta > 12))
        {
            Flappy_DrawPipeSlice(pipeX, gapTop, (int16_t)(pipeX - 3), (int16_t)(FLAPPY_PIPE_W + 6));
        }
        else
        {
            Flappy_DrawPipeSlice(pipeX, gapTop, (int16_t)(pipeX - 3), (int16_t)(delta + 3));
        }
    }

    Game_FillRectClipped((int16_t)(FLAPPY_BIRD_X - 8), (int16_t)(birdY - 7), 16, 14, FLAPPY_BIRD_COLOR);
    Game_FillRectClipped((int16_t)(FLAPPY_BIRD_X + 2), (int16_t)(birdY - 4), 8, 8, ILI9341_COLOR_WHITE);
    Game_FillRectClipped((int16_t)(FLAPPY_BIRD_X + 7), (int16_t)(birdY - 1), 3, 3, ILI9341_COLOR_BLACK);
    Game_FillRectClipped((int16_t)(FLAPPY_BIRD_X - 13), (int16_t)(birdY - 1), 7, 5, 0xFD20U);

    if (scoreRedrawNeeded)
    {
        Flappy_DrawScoreBar();
        s_flappyPrevScore = s_flappyScore;
    }

    s_flappyPrevBirdY = birdY;
    s_flappyPrevPipeX[0] = s_flappyPipeX[0];
    s_flappyPrevPipeX[1] = s_flappyPipeX[1];
    s_flappyPrevGameOver = s_flappyGameOver;
}

static void Dino_DrawGround(void)
{
    (void)ili9341_fill_rect(&s_lcd, 0U, DINO_GROUND_Y, s_lcdWidth, 3U, DINO_FG_COLOR);
}

static void Dino_DrawScore(void)
{
    char scoreText[20];

    (void)ili9341_fill_rect(&s_lcd, 0U, 0U, 128U, 26U, DINO_BG_COLOR);
    (void)snprintf(scoreText, sizeof(scoreText), "SCOR %lu", (unsigned long)s_dinoScore);
    (void)ili9341_write_string(&s_lcd, 8U, 7U, scoreText, DINO_FG_COLOR, DINO_BG_COLOR, 2U);
}

static void Dino_DrawDinoShape(int16_t y, uint16_t color)
{
    const int16_t x = DINO_X;

    Game_FillRectClipped(x, (int16_t)(y + 10), 23, 28, color);
    Game_FillRectClipped((int16_t)(x + 15), y, 20, 17, color);
    Game_FillRectClipped((int16_t)(x + 5), (int16_t)(y + 36), 7, 11, color);
    Game_FillRectClipped((int16_t)(x + 19), (int16_t)(y + 36), 7, 11, color);
    Game_FillRectClipped((int16_t)(x - 10), (int16_t)(y + 19), 12, 7, color);
}

static void Dino_DrawDino(int16_t y)
{
    Dino_DrawDinoShape(y, DINO_FG_COLOR);
    Game_FillRectClipped((int16_t)(DINO_X + 30), (int16_t)(y + 5), 5, 5, DINO_BG_COLOR);
}

static void Dino_EraseOldDinoTrail(int16_t oldY, int16_t newY)
{
    (void)newY;

    if (oldY < 0)
    {
        return;
    }

    Dino_DrawDinoShape(oldY, DINO_BG_COLOR);
}

static void Dino_DrawCactus(int16_t x)
{
    const int16_t y = (int16_t)(DINO_GROUND_Y - DINO_CACTUS_H);

    Game_FillRectClipped(x, y, DINO_CACTUS_W, DINO_CACTUS_H, DINO_FG_COLOR);
    Game_FillRectClipped((int16_t)(x - 8), (int16_t)(y + 10), 8, 7, DINO_FG_COLOR);
    Game_FillRectClipped((int16_t)(x + DINO_CACTUS_W), (int16_t)(y + 17), 8, 7, DINO_FG_COLOR);
}

static void Dino_FillCactusSlicePart(int16_t rectX,
                                     int16_t rectY,
                                     int16_t rectW,
                                     int16_t rectH,
                                     int16_t sliceX,
                                     int16_t sliceW)
{
    int16_t drawX = rectX;
    int16_t drawRight = (int16_t)(rectX + rectW);
    const int16_t sliceRight = (int16_t)(sliceX + sliceW);

    if (drawX < sliceX)
    {
        drawX = sliceX;
    }
    if (drawRight > sliceRight)
    {
        drawRight = sliceRight;
    }
    if (drawRight > drawX)
    {
        Game_FillRectClipped(drawX, rectY, (int16_t)(drawRight - drawX), rectH, DINO_FG_COLOR);
    }
}

static void Dino_DrawCactusSlice(int16_t x, int16_t sliceX, int16_t sliceW)
{
    const int16_t y = (int16_t)(DINO_GROUND_Y - DINO_CACTUS_H);

    Dino_FillCactusSlicePart(x, y, DINO_CACTUS_W, DINO_CACTUS_H, sliceX, sliceW);
    Dino_FillCactusSlicePart((int16_t)(x - 8), (int16_t)(y + 10), 8, 7, sliceX, sliceW);
    Dino_FillCactusSlicePart((int16_t)(x + DINO_CACTUS_W), (int16_t)(y + 17), 8, 7, sliceX, sliceW);
}

static void Dino_DrawGameOverScreen(void)
{
    char scoreText[24];

    (void)ili9341_fill_screen(&s_lcd, DINO_BG_COLOR);
    (void)ili9341_write_string(&s_lcd, 50U, 44U, "GAME OVER", ILI9341_COLOR_RED, DINO_BG_COLOR, 3U);
    (void)snprintf(scoreText, sizeof(scoreText), "SCOR: %lu", (unsigned long)s_dinoScore);
    (void)ili9341_write_string(&s_lcd, 78U, 104U, scoreText, DINO_FG_COLOR, DINO_BG_COLOR, 2U);
    (void)ili9341_write_string(&s_lcd, 66U, 156U, "SW2 RESTART", DINO_FG_COLOR, DINO_BG_COLOR, 2U);
    (void)ili9341_write_string(&s_lcd, 80U, 190U, "SW3 MENIU", DINO_FG_COLOR, DINO_BG_COLOR, 2U);
}

static void Dino_Reset(void)
{
    s_dinoYQ4 = (int16_t)((DINO_GROUND_Y - DINO_H) * 16);
    s_dinoVelocityQ4 = 0;
    s_dinoCactusX = (int16_t)(s_lcdWidth + 30U);
    s_dinoScore = 0;
    s_dinoFrame = 0;
    s_dinoGameOver = false;
    s_dinoFullRedrawNeeded = true;
    s_dinoPrevY = -100;
    s_dinoPrevCactusX = -100;
    s_dinoPrevScore = UINT32_MAX;
    s_dinoPrevGameOver = false;
    Dino_DrawScene();
}

static bool Dino_CheckCollision(void)
{
    const int16_t dinoX = (int16_t)(DINO_X + 6);
    const int16_t dinoY = (int16_t)((s_dinoYQ4 / 16) + 9);
    const int16_t dinoRight = (int16_t)(dinoX + DINO_W - 12);
    const int16_t dinoBottom = (int16_t)(dinoY + DINO_H - 10);
    const int16_t cactusX = (int16_t)(s_dinoCactusX + 3);
    const int16_t cactusRight = (int16_t)(s_dinoCactusX + DINO_CACTUS_W - 3);
    const int16_t cactusY = (int16_t)(DINO_GROUND_Y - DINO_CACTUS_H);
    const bool xOverlap = (dinoRight >= cactusX) && (dinoX <= cactusRight);
    const bool yOverlap = (dinoBottom >= cactusY) && (dinoY <= DINO_GROUND_Y);

    return xOverlap && yOverlap;
}

static void Dino_UpdateWorld(void)
{
    const int16_t groundYQ4 = (int16_t)((DINO_GROUND_Y - DINO_H) * 16);

    if (s_dinoGameOver)
    {
        return;
    }

    s_dinoVelocityQ4 = (int16_t)(s_dinoVelocityQ4 + DINO_GRAVITY_Q4);
    if (s_dinoVelocityQ4 > 150)
    {
        s_dinoVelocityQ4 = 150;
    }
    s_dinoYQ4 = (int16_t)(s_dinoYQ4 + s_dinoVelocityQ4);
    if (s_dinoYQ4 > groundYQ4)
    {
        s_dinoYQ4 = groundYQ4;
        s_dinoVelocityQ4 = 0;
    }

    s_dinoCactusX = (int16_t)(s_dinoCactusX - DINO_SCROLL_SPEED);
    if ((s_dinoCactusX + DINO_CACTUS_W) < 0)
    {
        s_dinoCactusX = (int16_t)(s_lcdWidth + 25U + ((s_dinoFrame * 19U) % 70U));
        s_dinoScore++;
    }

    if (Dino_CheckCollision())
    {
        s_dinoGameOver = true;
    }
    s_dinoFrame++;
}

static void Dino_HandleInput(bool jumpEdge, bool backEdge)
{
    const int16_t groundYQ4 = (int16_t)((DINO_GROUND_Y - DINO_H) * 16);

    if (backEdge)
    {
        s_appScreen = kAppScreenGameSelect;
        s_menuRedrawNeeded = true;
        return;
    }

    if (jumpEdge)
    {
        if (s_dinoGameOver)
        {
            Dino_Reset();
        }
        else if (s_dinoYQ4 >= groundYQ4)
        {
            s_dinoVelocityQ4 = DINO_JUMP_VELOCITY_Q4;
        }
    }
}

static void Dino_DrawScene(void)
{
    const int16_t dinoY = (int16_t)(s_dinoYQ4 / 16);

    if (s_dinoGameOver)
    {
        if (!s_dinoPrevGameOver)
        {
            Dino_DrawGameOverScreen();
        }
        s_dinoPrevGameOver = true;
        return;
    }

    if (s_dinoFullRedrawNeeded)
    {
        (void)ili9341_fill_screen(&s_lcd, DINO_BG_COLOR);
        Dino_DrawGround();
        s_dinoFullRedrawNeeded = false;
    }
    else
    {
        const int16_t oldCactusRight = (int16_t)(s_dinoPrevCactusX + DINO_CACTUS_W + 8);
        const int16_t newCactusRight = (int16_t)(s_dinoCactusX + DINO_CACTUS_W + 8);
        if (s_dinoPrevY != dinoY)
        {
            Dino_EraseOldDinoTrail(s_dinoPrevY, dinoY);
        }
        if ((s_dinoCactusX < s_dinoPrevCactusX) && ((s_dinoPrevCactusX - s_dinoCactusX) <= 18))
        {
            Game_FillRectClipped(newCactusRight, (int16_t)(DINO_GROUND_Y - DINO_CACTUS_H), 
                                 (int16_t)(oldCactusRight - newCactusRight), DINO_CACTUS_H, DINO_BG_COLOR);
        }
        else
        {
            Game_FillRectClipped((int16_t)(s_dinoPrevCactusX - 8), (int16_t)(DINO_GROUND_Y - DINO_CACTUS_H), 40,
                                 DINO_CACTUS_H, DINO_BG_COLOR);
        }
        Dino_DrawGround();
    }

    if (s_dinoPrevCactusX < 0 || (s_dinoPrevCactusX - s_dinoCactusX) <= 0 || (s_dinoPrevCactusX - s_dinoCactusX) > 18)
    {
        Dino_DrawCactus(s_dinoCactusX);
    }
    else
    {
        Dino_DrawCactusSlice(s_dinoCactusX, (int16_t)(s_dinoCactusX - 8), (int16_t)((s_dinoPrevCactusX - s_dinoCactusX) + 8));
    }
    if (s_dinoPrevY != dinoY)
    {
        Dino_DrawDino(dinoY);
    }
    if (s_dinoScore != s_dinoPrevScore)
    {
        Dino_DrawScore();
        s_dinoPrevScore = s_dinoScore;
    }

    s_dinoPrevY = dinoY;
    s_dinoPrevCactusX = s_dinoCactusX;
    s_dinoPrevGameOver = s_dinoGameOver;
}

static void StackTower_DrawHud(void)
{
    char line[22];

    (void)ili9341_fill_rect(&s_lcd, 0U, 0U, s_lcdWidth, 36U, CLICK_PANEL_COLOR);
    (void)snprintf(line, sizeof(line), "SCORE: %lu", (unsigned long)s_stackScore);
    (void)ili9341_write_string(&s_lcd, 8U, 8U, line, CLICK_TEXT_COLOR, CLICK_PANEL_COLOR, 1U);
    (void)snprintf(line, sizeof(line), "BEST: %lu", (unsigned long)s_stackBestScore);
    (void)ili9341_write_string(&s_lcd, 170U, 8U, line, CLICK_TEXT_COLOR, CLICK_PANEL_COLOR, 1U);
}

static void StackTower_DrawPlacedBlock(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    Game_FillRectClipped(x, y, w, 28, color);
    Game_FillRectClipped((int16_t)(x + 4), (int16_t)(y + 4), (int16_t)(w - 8), 20, CLICK_BG_COLOR);
}

static void StackTower_DrawActiveBlock(void)
{
    StackTower_DrawPlacedBlock(46, 188, 228, CLICK_ACCENT_COLOR);
    (void)ili9341_write_string(&s_lcd, 70U, 195U, "SW2 APASA", CLICK_TEXT_COLOR, CLICK_BG_COLOR, 2U);
}

static void StackTower_Reset(void)
{
    s_stackBaseW = 0;
    s_stackBaseX = 0;
    s_stackPrevBaseX = -100;
    s_stackTargetY = 0;
    s_stackActiveX = 0;
    s_stackActiveY = 0;
    s_stackPrevActiveX = -100;
    s_stackPrevActiveY = -100;
    s_stackSpeed = 0U;
    s_stackLevel = 0U;
    s_stackScore = 0U;
    s_stackCountdownFrame = 0U;
    s_stackMessageFrame = 0U;
    s_stackGameOver = false;
    s_stackFullRedrawNeeded = true;
    s_stackRunning = true;
    StackTower_Draw();
}

static void StackTower_DropBlock(void)
{
    s_stackScore++;
    if (s_stackScore > s_stackBestScore)
    {
        s_stackBestScore = s_stackScore;
    }
    s_stackMessageFrame = 8U;
    s_stackFullRedrawNeeded = true;
}

static void StackTower_HandleInput(bool dropEdge, bool backEdge)
{
    if (backEdge)
    {
        s_appScreen = kAppScreenGameSelect;
        s_menuRedrawNeeded = true;
        return;
    }

    if (dropEdge)
    {
        StackTower_DropBlock();
    }
}

static void StackTower_Update(void)
{
    if (s_stackMessageFrame > 0U)
    {
        s_stackMessageFrame--;
    }
}

static void StackTower_Draw(void)
{
    char scoreText[18];

    if (!s_stackFullRedrawNeeded)
    {
        return;
    }

    (void)ili9341_fill_screen(&s_lcd, CLICK_BG_COLOR);
    StackTower_DrawHud();
    (void)ili9341_write_string(&s_lcd, 56U, 52U, "CLICK SCORE", CLICK_ACCENT_COLOR, CLICK_BG_COLOR, 2U);
    (void)snprintf(scoreText, sizeof(scoreText), "%lu", (unsigned long)s_stackScore);
    (void)ili9341_write_string(&s_lcd, 122U, 96U, scoreText, CLICK_SCORE_COLOR, CLICK_BG_COLOR, 4U);
    StackTower_DrawActiveBlock();
    if (s_stackMessageFrame > 0U)
    {
        (void)ili9341_write_string(&s_lcd, 104U, 154U, "+1", ILI9341_COLOR_GREEN, CLICK_BG_COLOR, 3U);
    }
    (void)ili9341_write_string(&s_lcd, 74U, 228U, "SW3 MENIU", CLICK_TEXT_COLOR, CLICK_BG_COLOR, 2U);
    s_stackFullRedrawNeeded = false;
}

static const char *Menu_PlayerName(void)
{
    return (s_selectedPlayer == 0U) ? "ANASTASIA" : "BIANCA";
}

static void Menu_DrawOption(uint16_t y, const char *text, bool selected)
{
    const uint16_t bg = selected ? MENU_SELECTED_COLOR : MENU_BG_COLOR;
    const uint16_t fg = selected ? MENU_BG_COLOR : MENU_NORMAL_COLOR;

    (void)ili9341_fill_rect(&s_lcd, 24U, y, (uint16_t)(s_lcdWidth - 48U), 24U, bg);
    (void)ili9341_write_string(&s_lcd, 36U, (uint16_t)(y + 5U), selected ? ">" : " ", fg, bg, 2U);
    (void)ili9341_write_string(&s_lcd, 62U, (uint16_t)(y + 5U), text, fg, bg, 2U);
}

static void Menu_DrawWelcome(void)
{
    (void)ili9341_fill_screen(&s_lcd, MENU_BG_COLOR);

    (void)ili9341_fill_rect(&s_lcd, 0U, 0U, s_lcdWidth, 26U, 0x0013U);
    (void)ili9341_fill_rect(&s_lcd, 0U, (uint16_t)(s_lcdHeight - 28U), s_lcdWidth, 28U, 0x0013U);

    (void)ili9341_write_string(&s_lcd, 44U, 38U, "WELCOME", MENU_TITLE_COLOR, MENU_BG_COLOR, 3U);
    (void)ili9341_write_string(&s_lcd, 42U, 86U, "MINI ARCADE", MENU_SELECTED_COLOR, MENU_BG_COLOR, 2U);

    (void)ili9341_fill_rect(&s_lcd, 34U, 126U, 24U, 24U, ILI9341_COLOR_MAGENTA);
    (void)ili9341_fill_rect(&s_lcd, 40U, 132U, 12U, 12U, ILI9341_COLOR_YELLOW);
    (void)ili9341_fill_rect(&s_lcd, 96U, 118U, 34U, 34U, ILI9341_COLOR_CYAN);
    (void)ili9341_fill_rect(&s_lcd, 105U, 127U, 16U, 16U, ILI9341_COLOR_BLUE);
    (void)ili9341_fill_rect(&s_lcd, 168U, 126U, 24U, 24U, ILI9341_COLOR_GREEN);
    (void)ili9341_fill_rect(&s_lcd, 174U, 132U, 12U, 12U, ILI9341_COLOR_WHITE);

    (void)ili9341_fill_rect(&s_lcd, 52U, 184U, 136U, 34U, MENU_SELECTED_COLOR);
    (void)ili9341_write_string(&s_lcd, 80U, 193U, "START", MENU_BG_COLOR, MENU_SELECTED_COLOR, 2U);
    (void)ili9341_write_string(&s_lcd, 24U, 242U, "APASA SW2 SAU SW3", MENU_HINT_COLOR, MENU_BG_COLOR, 2U);
}

static void Menu_DrawAnastasiaAvatar(uint16_t x, uint16_t y)
{
    (void)ili9341_fill_rect(&s_lcd, x, y, 84U, 82U, 0x1810U);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 6U), (uint16_t)(y + 6U), 72U, 70U, 0xF81FU);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 12U), (uint16_t)(y + 12U), 60U, 58U, 0xFE7AU);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 20U), (uint16_t)(y + 18U), 44U, 16U, 0xB145U);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 18U), (uint16_t)(y + 28U), 48U, 30U, 0xFF38U);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 28U), (uint16_t)(y + 38U), 6U, 6U, ILI9341_COLOR_BLACK);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 50U), (uint16_t)(y + 38U), 6U, 6U, ILI9341_COLOR_BLACK);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 38U), (uint16_t)(y + 51U), 8U, 4U, ILI9341_COLOR_RED);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 26U), (uint16_t)(y + 63U), 32U, 7U, 0xFD20U);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 10U), (uint16_t)(y + 8U), 8U, 8U, ILI9341_COLOR_YELLOW);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 66U), (uint16_t)(y + 8U), 8U, 8U, ILI9341_COLOR_YELLOW);
}

static void Menu_DrawBiancaAvatar(uint16_t x, uint16_t y)
{
    (void)ili9341_fill_rect(&s_lcd, x, y, 84U, 82U, 0x1018U);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 6U), (uint16_t)(y + 6U), 72U, 70U, ILI9341_COLOR_CYAN);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 14U), (uint16_t)(y + 14U), 56U, 56U, 0xA65FU);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 20U), (uint16_t)(y + 18U), 44U, 18U, 0x39E7U);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 18U), (uint16_t)(y + 30U), 48U, 28U, 0xFF38U);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 28U), (uint16_t)(y + 39U), 6U, 6U, ILI9341_COLOR_BLUE);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 50U), (uint16_t)(y + 39U), 6U, 6U, ILI9341_COLOR_BLUE);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 38U), (uint16_t)(y + 52U), 10U, 4U, ILI9341_COLOR_MAGENTA);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 24U), (uint16_t)(y + 63U), 36U, 7U, ILI9341_COLOR_GREEN);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 12U), (uint16_t)(y + 10U), 7U, 7U, ILI9341_COLOR_WHITE);
    (void)ili9341_fill_rect(&s_lcd, (uint16_t)(x + 65U), (uint16_t)(y + 10U), 7U, 7U, ILI9341_COLOR_WHITE);
}

static void Menu_DrawPlayerSelect(void)
{
    (void)ili9341_fill_screen(&s_lcd, MENU_BG_COLOR);
    (void)ili9341_write_string(&s_lcd, 28U, 18U, "ALEGE USER", MENU_TITLE_COLOR, MENU_BG_COLOR, 2U);

    Menu_DrawAnastasiaAvatar(26U, 58U);
    Menu_DrawBiancaAvatar(130U, 58U);

    (void)ili9341_write_string(&s_lcd, 20U, 150U, "SW2", MENU_SELECTED_COLOR, MENU_BG_COLOR, 2U);
    (void)ili9341_write_string(&s_lcd, 12U, 176U, "ANASTASIA", MENU_NORMAL_COLOR, MENU_BG_COLOR, 2U);
    (void)ili9341_write_string(&s_lcd, 136U, 150U, "SW3", MENU_SELECTED_COLOR, MENU_BG_COLOR, 2U);
    (void)ili9341_write_string(&s_lcd, 136U, 176U, "BIANCA", MENU_NORMAL_COLOR, MENU_BG_COLOR, 2U);

    (void)ili9341_write_string(&s_lcd, 28U, 226U, "APASA BUTONUL", MENU_HINT_COLOR, MENU_BG_COLOR, 2U);
}

static void Menu_DrawGameSelect(void)
{
    char title[20];

    (void)ili9341_fill_screen(&s_lcd, MENU_BG_COLOR);
    (void)snprintf(title, sizeof(title), "%s", Menu_PlayerName());
    (void)ili9341_write_string(&s_lcd, 54U, 22U, title, MENU_TITLE_COLOR, MENU_BG_COLOR, 2U);
    (void)ili9341_write_string(&s_lcd, 58U, 50U, "ALEGE JOC", MENU_TITLE_COLOR, MENU_BG_COLOR, 2U);

    Menu_DrawOption(84U, "FLAPPY BIRD", s_selectedGame == 0U);
    Menu_DrawOption(114U, "DINO RUN", s_selectedGame == 1U);
    Menu_DrawOption(144U, "FA POZA", s_selectedGame == MENU_PHOTO_OPTION);
    Menu_DrawOption(174U, "FILMEAZA", s_selectedGame == MENU_VIDEO_OPTION);

    (void)ili9341_write_string(&s_lcd, 22U, 272U, "SW2 schimba", MENU_HINT_COLOR, MENU_BG_COLOR, 2U);
    (void)ili9341_write_string(&s_lcd, 22U, 300U, "SW3 confirma", MENU_HINT_COLOR, MENU_BG_COLOR, 2U);
}

static void Menu_DrawGamePlaceholder(void)
{
    char playerText[20];
    char gameText[16];

    (void)ili9341_fill_screen(&s_lcd, MENU_BG_COLOR);
    (void)snprintf(playerText, sizeof(playerText), "%s", Menu_PlayerName());
    if (s_selectedGame == MENU_PHOTO_OPTION)
    {
        (void)snprintf(gameText, sizeof(gameText), "POZA");
    }
    else if (s_selectedGame == MENU_VIDEO_OPTION)
    {
        (void)snprintf(gameText, sizeof(gameText), "VIDEO");
    }
    else if (s_selectedGame == MENU_TOUCH_OPTION)
    {
        (void)snprintf(gameText, sizeof(gameText), "TOUCH");
    }
    else if (s_selectedGame == 0U)
    {
        (void)snprintf(gameText, sizeof(gameText), "FLAPPY BIRD");
    }
    else if (s_selectedGame == 1U)
    {
        (void)snprintf(gameText, sizeof(gameText), "DINO RUN");
    }
    else
    {
        (void)snprintf(gameText, sizeof(gameText), "JOC %u", (unsigned int)(s_selectedGame + 1U));
    }

    (void)ili9341_write_string(&s_lcd, 54U, 46U, playerText, MENU_TITLE_COLOR, MENU_BG_COLOR, 2U);
    (void)ili9341_write_string(&s_lcd, 94U, 86U, gameText, MENU_SELECTED_COLOR, MENU_BG_COLOR, 2U);
    if (s_selectedGame == MENU_PHOTO_OPTION)
    {
        (void)ili9341_write_string(&s_lcd, 44U, 126U, "CERERE TRIMISA", MENU_NORMAL_COLOR, MENU_BG_COLOR, 2U);
        if (Camera_StatusActive())
        {
            (void)ili9341_write_string(&s_lcd, 42U, 154U, "POZA FACUTA", MENU_SELECTED_COLOR, MENU_BG_COLOR, 2U);
        }
        else
        {
            (void)ili9341_write_string(&s_lcd, 26U, 154U, "VERIFICA LAPTOP", MENU_HINT_COLOR, MENU_BG_COLOR, 2U);
        }
    }
    else if (s_selectedGame == MENU_VIDEO_OPTION)
    {
        (void)ili9341_write_string(&s_lcd, 44U, 126U, "FILMEZ ACUM", MENU_NORMAL_COLOR, MENU_BG_COLOR, 2U);
        if (Camera_StatusActive())
        {
            (void)ili9341_write_string(&s_lcd, 32U, 154U, "VIDEO SALVAT", MENU_SELECTED_COLOR, MENU_BG_COLOR, 2U);
        }
        else
        {
            (void)ili9341_write_string(&s_lcd, 26U, 154U, "VERIFICA LAPTOP", MENU_HINT_COLOR, MENU_BG_COLOR, 2U);
        }
    }
    else if (s_selectedGame == MENU_TOUCH_OPTION)
    {
        (void)ili9341_write_string(&s_lcd, 32U, 126U, "ATINGE ECRANUL", MENU_NORMAL_COLOR, MENU_BG_COLOR, 2U);
        (void)ili9341_fill_rect(&s_lcd, 36U, 156U, 248U, 58U, 0x2104U);
        (void)ili9341_write_string(&s_lcd, 50U, 176U, "WAIT TOUCH", MENU_HINT_COLOR, 0x2104U, 2U);
        s_touchWasPressed = false;
        s_touchLastX = 0U;
        s_touchLastY = 0U;
    }
    else if (s_selectedGame == 0U)
    {
        Flappy_Reset();
    }
    else if (s_selectedGame == 1U)
    {
        Dino_Reset();
    }
    else
    {
        (void)ili9341_write_string(&s_lcd, 66U, 126U, "IN CURAND", MENU_NORMAL_COLOR, MENU_BG_COLOR, 2U);
    }
    if ((s_selectedGame != 0U) && (s_selectedGame != 1U))
    {
        (void)ili9341_write_string(&s_lcd, 20U, 214U, "SW2 inapoi jocuri", MENU_HINT_COLOR, MENU_BG_COLOR, 2U);
        (void)ili9341_write_string(&s_lcd, 20U, 242U, "SW3 inapoi start", MENU_HINT_COLOR, MENU_BG_COLOR, 2U);
    }
}

static void Menu_HandleInput(bool nextPressed, bool nextEdge, bool selectEdge)
{
    if (s_appScreen == kAppScreenWelcome)
    {
        if (nextEdge || selectEdge)
        {
            s_appScreen = kAppScreenPlayerSelect;
            s_menuRedrawNeeded = true;
        }
        return;
    }

    if (s_appScreen == kAppScreenPlayerSelect)
    {
        if (nextEdge)
        {
            s_selectedPlayer = 0U;
            s_selectedGame = 0U;
            s_appScreen = kAppScreenGameSelect;
            s_menuRedrawNeeded = true;
        }
        if (selectEdge)
        {
            s_selectedPlayer = 1U;
            s_appScreen = kAppScreenGameSelect;
            s_selectedGame = 0U;
            s_menuRedrawNeeded = true;
        }
        return;
    }

    if (s_appScreen == kAppScreenGameSelect)
    {
        if (nextEdge)
        {
            s_selectedGame = (uint8_t)((s_selectedGame + 1U) % MENU_OPTION_COUNT);
            s_menuRedrawNeeded = true;
        }
        if (selectEdge)
        {
            if (s_selectedGame == MENU_PHOTO_OPTION)
            {
                Camera_TriggerCapture();
                s_photoRequested = true;
                s_photoDoneShown = false;
            }
            else if (s_selectedGame == MENU_VIDEO_OPTION)
            {
                Camera_TriggerVideo();
                s_photoRequested = true;
                s_photoDoneShown = false;
            }
            s_appScreen = kAppScreenGamePlaceholder;
            s_menuRedrawNeeded = true;
        }
        return;
    }

    if (s_appScreen == kAppScreenGamePlaceholder)
    {
        if (s_selectedGame == 0U)
        {
            Flappy_HandleInput(nextPressed, nextEdge, selectEdge);
            return;
        }
        if (s_selectedGame == 1U)
        {
            Dino_HandleInput(nextEdge, selectEdge);
            return;
        }
        if (nextEdge)
        {
            s_photoRequested = false;
            s_photoDoneShown = false;
            s_appScreen = kAppScreenGameSelect;
            s_menuRedrawNeeded = true;
        }
        if (selectEdge)
        {
            s_photoRequested = false;
            s_photoDoneShown = false;
            s_appScreen = kAppScreenPlayerSelect;
            s_menuRedrawNeeded = true;
        }
    }
}

static void Camera_InitPins(void)
{
    const gpio_pin_config_t triggerConfig = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 0U,
    };
    const gpio_pin_config_t statusConfig = {
        .pinDirection = kGPIO_DigitalInput,
        .outputLogic = 0U,
    };

    CLOCK_EnableClock(kCLOCK_GatePORT1);
    CLOCK_EnableClock(kCLOCK_GatePORT3);
    CLOCK_EnableClock(kCLOCK_GateGPIO1);
    CLOCK_EnableClock(kCLOCK_GateGPIO3);
    RESET_ReleasePeripheralReset(kPORT1_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kGPIO1_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kGPIO3_RST_SHIFT_RSTn);

    PORT_SetPinMux(CAMERA_TRIGGER_PORT, CAMERA_TRIGGER_PIN, kPORT_MuxAlt0);
    PORT_SetPinMux(CAMERA_STATUS_PORT, CAMERA_STATUS_PIN, kPORT_MuxAlt0);

    GPIO_PinInit(CAMERA_TRIGGER_GPIO, CAMERA_TRIGGER_PIN, &triggerConfig);
    GPIO_PinInit(CAMERA_STATUS_GPIO, CAMERA_STATUS_PIN, &statusConfig);
    GPIO_PinWrite(CAMERA_TRIGGER_GPIO, CAMERA_TRIGGER_PIN, 0U);
}

static void Camera_TriggerCapture(void)
{
    GPIO_PinWrite(CAMERA_TRIGGER_GPIO, CAMERA_TRIGGER_PIN, 1U);
    SDK_DelayAtLeastUs(CAMERA_TRIGGER_PULSE_US, CLOCK_GetCoreSysClkFreq());
    GPIO_PinWrite(CAMERA_TRIGGER_GPIO, CAMERA_TRIGGER_PIN, 0U);
}

static void Camera_TriggerVideo(void)
{
    GPIO_PinWrite(CAMERA_TRIGGER_GPIO, CAMERA_TRIGGER_PIN, 1U);
    SDK_DelayAtLeastUs(CAMERA_VIDEO_TRIGGER_PULSE_US, CLOCK_GetCoreSysClkFreq());
    GPIO_PinWrite(CAMERA_TRIGGER_GPIO, CAMERA_TRIGGER_PIN, 0U);
}

static bool Camera_StatusActive(void)
{
    return GPIO_PinRead(CAMERA_STATUS_GPIO, CAMERA_STATUS_PIN) != 0U;
}

static void Touch_SetSpiPinsGpio(void)
{
    const gpio_pin_config_t outConfig = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 0U,
    };
    const gpio_pin_config_t inConfig = {
        .pinDirection = kGPIO_DigitalInput,
        .outputLogic = 0U,
    };

    PORT_SetPinMux(TOUCH_MOSI_PORT, TOUCH_MOSI_PIN, kPORT_MuxAlt0);
    PORT_SetPinMux(TOUCH_SCK_PORT, TOUCH_SCK_PIN, kPORT_MuxAlt0);
    PORT_SetPinMux(TOUCH_MISO_PORT, TOUCH_MISO_PIN, kPORT_MuxAlt0);
    GPIO_PinInit(TOUCH_MOSI_GPIO, TOUCH_MOSI_PIN, &outConfig);
    GPIO_PinInit(TOUCH_SCK_GPIO, TOUCH_SCK_PIN, &outConfig);
    GPIO_PinInit(TOUCH_MISO_GPIO, TOUCH_MISO_PIN, &inConfig);
    GPIO_PinWrite(TOUCH_SCK_GPIO, TOUCH_SCK_PIN, 0U);
}

static void Touch_RestoreLpspiPins(void)
{
    PORT_SetPinMux(TOUCH_MOSI_PORT, TOUCH_MOSI_PIN, kPORT_MuxAlt2);
    PORT_SetPinMux(TOUCH_SCK_PORT, TOUCH_SCK_PIN, kPORT_MuxAlt2);
    PORT_SetPinMux(TOUCH_MISO_PORT, TOUCH_MISO_PIN, kPORT_MuxAlt2);
}

static void Touch_Clock(void)
{
    GPIO_PinWrite(TOUCH_SCK_GPIO, TOUCH_SCK_PIN, 1U);
    SDK_DelayAtLeastUs(1U, CLOCK_GetCoreSysClkFreq());
    GPIO_PinWrite(TOUCH_SCK_GPIO, TOUCH_SCK_PIN, 0U);
    SDK_DelayAtLeastUs(1U, CLOCK_GetCoreSysClkFreq());
}

static uint16_t Touch_ReadAdc(uint8_t command)
{
    uint16_t value = 0U;

    for (int8_t bit = 7; bit >= 0; bit--)
    {
        GPIO_PinWrite(TOUCH_MOSI_GPIO, TOUCH_MOSI_PIN, ((command & (1U << bit)) != 0U) ? 1U : 0U);
        Touch_Clock();
    }

    for (uint8_t bit = 0U; bit < 16U; bit++)
    {
        GPIO_PinWrite(TOUCH_SCK_GPIO, TOUCH_SCK_PIN, 1U);
        SDK_DelayAtLeastUs(1U, CLOCK_GetCoreSysClkFreq());
        value = (uint16_t)((value << 1U) | (GPIO_PinRead(TOUCH_MISO_GPIO, TOUCH_MISO_PIN) & 1U));
        GPIO_PinWrite(TOUCH_SCK_GPIO, TOUCH_SCK_PIN, 0U);
        SDK_DelayAtLeastUs(1U, CLOCK_GetCoreSysClkFreq());
    }

    return (uint16_t)((value >> 3U) & 0x0FFFU);
}

static void Touch_InitPins(void)
{
    const gpio_pin_config_t csConfig = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 1U,
    };
    const gpio_pin_config_t irqConfig = {
        .pinDirection = kGPIO_DigitalInput,
        .outputLogic = 0U,
    };
    const port_pin_config_t irqPinConfig = {
        .pullSelect = kPORT_PullUp,
        .pullValueSelect = kPORT_LowPullResistor,
        .slewRate = kPORT_FastSlewRate,
        .passiveFilterEnable = kPORT_PassiveFilterDisable,
        .openDrainEnable = kPORT_OpenDrainDisable,
        .driveStrength = kPORT_LowDriveStrength,
        .driveStrength1 = kPORT_NormalDriveStrength,
        .mux = kPORT_MuxAlt0,
        .inputBuffer = kPORT_InputBufferEnable,
        .invertInput = kPORT_InputNormal,
        .lockRegister = kPORT_UnlockRegister,
    };

    CLOCK_EnableClock(kCLOCK_GatePORT3);
    CLOCK_EnableClock(kCLOCK_GatePORT2);
    CLOCK_EnableClock(kCLOCK_GateGPIO3);
    CLOCK_EnableClock(kCLOCK_GateGPIO2);
    RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kPORT2_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kGPIO3_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kGPIO2_RST_SHIFT_RSTn);

    for (uint32_t index = 0U; index < (sizeof(s_touchCsCandidates) / sizeof(s_touchCsCandidates[0])); index++)
    {
        PORT_SetPinMux(s_touchCsCandidates[index].port, s_touchCsCandidates[index].pin, kPORT_MuxAlt0);
        GPIO_PinInit(s_touchCsCandidates[index].gpio, s_touchCsCandidates[index].pin, &csConfig);
        GPIO_PinWrite(s_touchCsCandidates[index].gpio, s_touchCsCandidates[index].pin, 1U);
    }
    for (uint32_t index = 0U; index < (sizeof(s_touchIrqCandidates) / sizeof(s_touchIrqCandidates[0])); index++)
    {
        PORT_SetPinConfig(s_touchIrqCandidates[index].port, s_touchIrqCandidates[index].pin, &irqPinConfig);
        GPIO_PinInit(s_touchIrqCandidates[index].gpio, s_touchIrqCandidates[index].pin, &irqConfig);
    }
}

static void Touch_SelectCs(bool selected)
{
    for (uint32_t index = 0U; index < (sizeof(s_touchCsCandidates) / sizeof(s_touchCsCandidates[0])); index++)
    {
        GPIO_PinWrite(s_touchCsCandidates[index].gpio, s_touchCsCandidates[index].pin, 1U);
    }

    if (s_touchCsCandidateIndex >= (sizeof(s_touchCsCandidates) / sizeof(s_touchCsCandidates[0])))
    {
        s_touchCsCandidateIndex = 0U;
    }

    GPIO_PinWrite(s_touchCsCandidates[s_touchCsCandidateIndex].gpio,
                  s_touchCsCandidates[s_touchCsCandidateIndex].pin,
                  selected ? 0U : 1U);
}

static const char *Touch_CurrentCsLabel(void)
{
    if (s_touchCsCandidateIndex >= (sizeof(s_touchCsCandidates) / sizeof(s_touchCsCandidates[0])))
    {
        s_touchCsCandidateIndex = 0U;
    }

    return s_touchCsCandidates[s_touchCsCandidateIndex].label;
}

static bool Touch_ReadRaw(uint16_t *xRaw, uint16_t *yRaw, uint16_t *z1Raw, uint16_t *z2Raw)
{
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
    uint16_t z1;
    uint16_t z2;

    Touch_SetSpiPinsGpio();
    Touch_SelectCs(true);
    SDK_DelayAtLeastUs(2U, CLOCK_GetCoreSysClkFreq());
    x1 = Touch_ReadAdc(TOUCH_CMD_X);
    y1 = Touch_ReadAdc(TOUCH_CMD_Y);
    z1 = Touch_ReadAdc(TOUCH_CMD_Z1);
    z2 = Touch_ReadAdc(TOUCH_CMD_Z2);
    x2 = Touch_ReadAdc(TOUCH_CMD_X);
    y2 = Touch_ReadAdc(TOUCH_CMD_Y);
    Touch_SelectCs(false);
    Touch_RestoreLpspiPins();

    *xRaw = (uint16_t)((x1 + x2) / 2U);
    *yRaw = (uint16_t)((y1 + y2) / 2U);
    *z1Raw = z1;
    *z2Raw = z2;

    return Touch_RawLooksPressed(*xRaw, *yRaw, *z1Raw, *z2Raw, Touch_IrqActive());
}

static bool Touch_IrqActive(void)
{
    return Touch_ActiveIrqLabel() != NULL;
}

static const char *Touch_ActiveIrqLabel(void)
{
    for (uint32_t index = 0U; index < (sizeof(s_touchIrqCandidates) / sizeof(s_touchIrqCandidates[0])); index++)
    {
        if (GPIO_PinRead(s_touchIrqCandidates[index].gpio, s_touchIrqCandidates[index].pin) == 0U)
        {
            return s_touchIrqCandidates[index].label;
        }
    }

    return NULL;
}

static bool Touch_RawLooksPressed(uint16_t xRaw, uint16_t yRaw, uint16_t z1Raw, uint16_t z2Raw, bool irqActive)
{
    const bool coordinatesInRange =
        (xRaw > TOUCH_RAW_MIN) && (xRaw < TOUCH_RAW_MAX) && (yRaw > TOUCH_RAW_MIN) && (yRaw < TOUCH_RAW_MAX);
    const bool pressureInRange =
        (z1Raw > TOUCH_Z1_MIN) && (z2Raw > (uint16_t)(z1Raw + TOUCH_Z2_MIN_DELTA)) && (z2Raw < TOUCH_Z2_MAX);

    return irqActive && coordinatesInRange && pressureInRange;
}

static void Touch_DrawStatus(uint16_t xRaw, uint16_t yRaw, uint16_t z1Raw, uint16_t z2Raw)
{
    char line[24];
    const char *irqLabel = Touch_ActiveIrqLabel();
    const bool irqActive = irqLabel != NULL;
    const bool pressed = Touch_RawLooksPressed(xRaw, yRaw, z1Raw, z2Raw, irqActive);

    if (s_touchWasPressed == pressed && (xRaw == s_touchLastX) && (yRaw == s_touchLastY))
    {
        return;
    }

    s_touchWasPressed = pressed;
    s_touchLastX = xRaw;
    s_touchLastY = yRaw;
    (void)ili9341_fill_rect(&s_lcd, 36U, 156U, 248U, 58U, 0x2104U);
    (void)ili9341_write_string(&s_lcd, 54U, 164U, pressed ? "TOUCH OK" : "RAW READ", pressed ? ILI9341_COLOR_GREEN : MENU_HINT_COLOR, 0x2104U, 2U);
    (void)snprintf(line, sizeof(line), "X%04u Y%04u", (unsigned int)xRaw, (unsigned int)yRaw);
    (void)ili9341_write_string(&s_lcd, 44U, 190U, line, MENU_SELECTED_COLOR, 0x2104U, 2U);
    (void)snprintf(line, sizeof(line), "Z%04u/%04u", (unsigned int)z1Raw, (unsigned int)z2Raw);
    (void)ili9341_write_string(&s_lcd, 74U, 210U, line, MENU_NORMAL_COLOR, 0x2104U, 1U);
    (void)ili9341_write_string(&s_lcd, 168U, 210U, irqActive ? irqLabel : "IRQ OFF", irqActive ? ILI9341_COLOR_GREEN : MENU_HINT_COLOR, 0x2104U, 1U);
    (void)snprintf(line, sizeof(line), "CS %s", Touch_CurrentCsLabel());
    (void)ili9341_write_string(&s_lcd, 84U, 224U, line, MENU_HINT_COLOR, 0x2104U, 1U);
}

/*!
 * @brief Main function
 */
int main(void)
{
    ili9341_config_t lcdConfig;
    status_t status;

    /* Board pin init */
    BOARD_InitHardware();
    PWM0_LED_InitOutput();
    SW2_ADC_InitInput();
    Camera_InitPins();
    Touch_InitPins();

    BOARD_GetIli9341Config(&lcdConfig);
    status = ili9341_init(&s_lcd, &lcdConfig);
    if (status != kStatus_Success)
    {
        PRINTF("ILI9341 init failed: %ld\r\n", (long)status);
        while (1)
        {
        }
    }

    status = ili9341_set_rotation(&s_lcd, 1U);
    if (status != kStatus_Success)
    {
        PRINTF("ILI9341 rotation failed: %ld\r\n", (long)status);
        while (1)
        {
        }
    }

    s_lcdWidth = s_lcd.width;
    s_lcdHeight = s_lcd.height;
    s_sw2WasPressed = SW2_IsPressed();
    s_sw3WasPressed = SW3_IsPressed();
    s_appScreen = kAppScreenWelcome;
    s_selectedPlayer = 0U;
    s_selectedGame = 0U;
    s_menuRedrawNeeded = true;
    s_photoRequested = false;
    s_photoDoneShown = false;

    while (1)
    {
        const bool sw3Pressed = Button_DebouncedRead(SW3_IsPressed, s_sw3WasPressed);
        const bool sw2Pressed = Button_DebouncedRead(SW2_IsPressed, s_sw2WasPressed);
        const bool sw2Edge = sw2Pressed && !s_sw2WasPressed;
        const bool sw3Edge = sw3Pressed && !s_sw3WasPressed;

        s_sw2WasPressed = sw2Pressed;
        s_sw3WasPressed = sw3Pressed;

        Menu_HandleInput(sw2Pressed, sw2Edge, sw3Edge);
        if (s_appScreen == kAppScreenWelcome)
        {
            uint16_t xRaw;
            uint16_t yRaw;
            uint16_t z1Raw;
            uint16_t z2Raw;
            if (Touch_ReadRaw(&xRaw, &yRaw, &z1Raw, &z2Raw))
            {
                SDK_DelayAtLeastUs(TOUCH_START_CONFIRM_US, CLOCK_GetCoreSysClkFreq());
                if (Touch_ReadRaw(&xRaw, &yRaw, &z1Raw, &z2Raw))
                {
                    s_appScreen = kAppScreenPlayerSelect;
                    s_menuRedrawNeeded = true;
                    SDK_DelayAtLeastUs(250000U, CLOCK_GetCoreSysClkFreq());
                }
            }
        }
        if (s_menuRedrawNeeded)
        {
            if (s_appScreen == kAppScreenWelcome)
            {
                Menu_DrawWelcome();
            }
            else if (s_appScreen == kAppScreenPlayerSelect)
            {
                Menu_DrawPlayerSelect();
            }
            else if (s_appScreen == kAppScreenGameSelect)
            {
                Menu_DrawGameSelect();
            }
            else
            {
                Menu_DrawGamePlaceholder();
            }
            s_menuRedrawNeeded = false;
        }
        if ((s_appScreen == kAppScreenGamePlaceholder) && s_photoRequested && !s_photoDoneShown &&
            Camera_StatusActive())
        {
            (void)ili9341_fill_rect(&s_lcd, 20U, 150U, 210U, 24U, MENU_BG_COLOR);
            if (s_selectedGame == MENU_VIDEO_OPTION)
            {
                (void)ili9341_write_string(&s_lcd, 32U, 154U, "VIDEO SALVAT", MENU_SELECTED_COLOR, MENU_BG_COLOR, 2U);
            }
            else
            {
                (void)ili9341_write_string(&s_lcd, 42U, 154U, "POZA FACUTA", MENU_SELECTED_COLOR, MENU_BG_COLOR, 2U);
            }
            s_photoDoneShown = true;
        }
        if ((s_appScreen == kAppScreenGamePlaceholder) && (s_selectedGame == MENU_TOUCH_OPTION))
        {
            uint16_t xRaw;
            uint16_t yRaw;
            uint16_t z1Raw;
            uint16_t z2Raw;
            (void)Touch_ReadRaw(&xRaw, &yRaw, &z1Raw, &z2Raw);
            Touch_DrawStatus(xRaw, yRaw, z1Raw, z2Raw);
        }
        if ((s_appScreen == kAppScreenGamePlaceholder) && (s_selectedGame == 0U))
        {
            Flappy_UpdateWorld();
            Flappy_DrawScene();
            SDK_DelayAtLeastUs(FLAPPY_FRAME_DELAY_US, CLOCK_GetCoreSysClkFreq());
        }
        if ((s_appScreen == kAppScreenGamePlaceholder) && (s_selectedGame == 1U))
        {
            Dino_UpdateWorld();
            Dino_DrawScene();
            SDK_DelayAtLeastUs(DINO_FRAME_DELAY_US, CLOCK_GetCoreSysClkFreq());
        }
        if (GAME_FRAME_DELAY_US > 0U)
        {
            SDK_DelayAtLeastUs(GAME_FRAME_DELAY_US, CLOCK_GetCoreSysClkFreq());
        }
    }
}
