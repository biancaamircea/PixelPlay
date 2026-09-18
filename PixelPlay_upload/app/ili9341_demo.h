#ifndef ILI9341_DEMO_H_
#define ILI9341_DEMO_H_

#include "fsl_common.h"

status_t ILI9341_DemoInit(void);
void ILI9341_DemoUpdate(uint16_t input_value);
status_t ILI9341_DemoRun(void);
void ILI9341_DemoSetInput(uint16_t value);

#endif /* ILI9341_DEMO_H_ */
