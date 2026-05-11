// File header untuk konfigurasi STM32 Multi program
#ifndef CONFIG_H
#define CONFIG_H

// Include the correct HAL header based on the MCU
#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif

// Konfigurasi pin dan peripheral
// Sesuaikan dengan kebutuhan program

#endif /* CONFIG_H */
