#include "board_ili9341.h"
#include "pin_mux.h"
#include "peripherals.h"
#include <string.h>

/*
 * FRDM-MCXA153 <-> 2.4" ILI9341 SPI TFT wiring.
 *
 * The board pinout image labels the 12-pin Pmod-style SPI header as:
 *   J2[1]  P1_3  CS   -> LCD TFT_CS
 *   J2[3]  P1_0  SDO  -> LCD TFT_MOSI / SDI
 *   J2[5]  P1_2  SDI  -> LCD TFT_MISO / SDO, optional
 *   J2[7]  P1_1  SCK  -> LCD TFT_CLK / SCK
 *   J2[9]  GND        -> LCD GND
 *   J2[11] VDD        -> LCD VCC, use 3.3 V only
 *
 * Extra LCD control GPIOs are routed to available FRDM header pins:
 *   J1[2]  P1_4       -> LCD TFT_DC
 *   J1[4]  P1_5       -> LCD TFT_RST
 *   J1[3]  P1_6       -> LCD LED/backlight, optional
 *
 * The display module's Arduino-style names are used only as signal names.
 * This project does not use Arduino libraries, and the FRDM-MCXA153 pins
 * must not be driven with 5 V logic.
 */

/* Pmod-style SPI header: J2[1] / P1_3 / CS -> LCD TFT_CS. */
#define BOARD_ILI9341_CS_GPIO GPIO1
#define BOARD_ILI9341_CS_PIN  3U

void BOARD_GetIli9341Config(ili9341_config_t *config)
{
    memset(config, 0, sizeof(*config));
    config->spi = LPSPI0_PERIPHERAL;
    config->spi_source_clock_hz = LPSPI0_CLOCK_FREQ;
    config->spi_baudrate_hz = LPSPI0_config.baudRate;
    config->transfer_timeout_loops = 1000000U;
    config->cs = (ili9341_gpio_pin_t){BOARD_ILI9341_CS_GPIO, BOARD_ILI9341_CS_PIN, true, false};
    config->dc = (ili9341_gpio_pin_t){BOARD_INITPINS_BOARD_ILI9341_DC_PIN_GPIO,
                                      BOARD_INITPINS_BOARD_ILI9341_DC_PIN_GPIO_PIN,
                                      false,
                                      true};
    config->rst = (ili9341_gpio_pin_t){BOARD_INITPINS_BOARD_ILI9341_RST_PIN_GPIO,
                                       BOARD_INITPINS_BOARD_ILI9341_RST_PIN_GPIO_PIN,
                                       true,
                                       true};
    config->backlight = (ili9341_gpio_pin_t){BOARD_INITPINS_BOARD_ILI9341_BL_PIN_GPIO,
                                             BOARD_INITPINS_BOARD_ILI9341_BL_PIN_GPIO_PIN,
                                             false,
                                             true};
}
