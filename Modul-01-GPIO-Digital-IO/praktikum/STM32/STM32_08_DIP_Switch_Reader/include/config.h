/**
 * ============================================================================
 * @file    config.h
 * @brief   Configuration for 4-bit DIP Switch Reader
 * @project STM32_08_DIP_Switch_Reader
 * ============================================================================
 *
 * Hardware Configuration:
 *   - DIP Switch 1 (bit0) on PB0 (input, internal pull-up)
 *   - DIP Switch 2 (bit1) on PB1 (input, internal pull-up)
 *   - DIP Switch 3 (bit2) on PB3 (input, internal pull-up)  [PB2=BOOT1 avoided]
 *   - DIP Switch 4 (bit3) on PB4 (input, internal pull-up)
 *   - Switch ON = connects pin to GND = reads LOW = logic 1
 *   - Switch OFF = pulled up = reads HIGH = logic 0
 *
 * Wiring Diagram:
 *   PB0 ---[DIP_SW1]--- GND    (bit 0, LSB)
 *   PB1 ---[DIP_SW2]--- GND    (bit 1)
 *   PB3 ---[DIP_SW3]--- GND    (bit 2)
 *   PB4 ---[DIP_SW4]--- GND    (bit 3, MSB)
 *   Note: PB2 is skipped (BOOT1 pin on Blue Pill)
 *
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifdef STM32F103xB
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#else
  #error "Unsupported STM32 target. Define STM32F103xB, STM32F401xC, or STM32F411xE"
#endif

/* ========================= DIP Switch Configuration ================== */
#define DIP_PORT        GPIOB

#define DIP1_PIN        GPIO_PIN_0      /* Bit 0 (LSB) */
#define DIP2_PIN        GPIO_PIN_1      /* Bit 1        */
#define DIP3_PIN        GPIO_PIN_3      /* Bit 2 (skip PB2=BOOT1) */
#define DIP4_PIN        GPIO_PIN_4      /* Bit 3 (MSB) */

#define DIP_ALL_PINS    (DIP1_PIN | DIP2_PIN | DIP3_PIN | DIP4_PIN)

/* ========================= Timing Configuration ====================== */
#define READ_INTERVAL_MS    500     /* DIP switch read interval (ms)     */

#endif /* CONFIG_H */
