#ifndef MAIN_H
#define MAIN_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported variables --------------------------------------------------------*/
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;

/* Exported function prototypes ----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
void MX_GPIO_Init(void);
void MX_I2C1_Init(void);
void MX_USART1_UART_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
