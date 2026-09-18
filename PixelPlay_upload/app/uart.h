
#ifndef APP_UART_H_
#define APP_UART_H_

#include <stdint.h>

void LPUART0_WriteString(const char *text);
void LPUART0_WriteU32(uint32_t value);

#endif /* APP_UART_H_ */
