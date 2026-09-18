
#include "uart.h"

#include "peripherals.h"
#include <stddef.h>

void LPUART0_WriteString(const char *text)
{
    const char *end = text;

    while (*end != '\0')
    {
        end++;
    }

    (void)LPUART_WriteBlocking(LPUART0_PERIPHERAL, (const uint8_t *)text, (size_t)(end - text));
}

void LPUART0_WriteU32(uint32_t value)
{
    char buffer[10];
    size_t index = sizeof(buffer);

    do
    {
        index--;
        buffer[index] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U);

    (void)LPUART_WriteBlocking(LPUART0_PERIPHERAL, (const uint8_t *)&buffer[index], sizeof(buffer) - index);
}
