/* STM32_07_SPI_LCD_RTOS_MultiTask - supports bluepill_f103c8 / stm32f401cc / stm32f411ce */
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

SPI_HandleTypeDef hspi1;
SemaphoreHandle_t xLCDMutex;

typedef struct {
    uint16_t x;
    uint16_t y;
    int16_t vx;
    int16_t vy;
    uint16_t color;
} Ball;

Ball ball = {64, 80, 2, 2, COLOR_RED};
uint32_t frame_count = 0;

void SystemClock_Config(void);
void GPIO_Init(void);
void SPI1_Init(void);
void LCD_WriteCmd(uint8_t cmd);
void LCD_WriteData(uint8_t data);
void LCD_WriteData16(uint16_t data);
void LCD_SetAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void LCD_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void LCD_Init(void);
void LCD_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg);

void LCD_WriteCmd(uint8_t cmd) {
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
}

void LCD_WriteData(uint8_t data) {
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
}

void LCD_WriteData16(uint16_t data) {
    uint8_t buf[2] = {data >> 8, data & 0xFF};
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_SPI_Transmit(&hspi1, buf, 2, HAL_MAX_DELAY);
}

void LCD_SetAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    LCD_WriteCmd(0x2A);
    LCD_WriteData16(x0);
    LCD_WriteData16(x1);
    LCD_WriteCmd(0x2B);
    LCD_WriteData16(y0);
    LCD_WriteData16(y1);
    LCD_WriteCmd(0x2C);
}

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
    
    LCD_SetAddrWindow(x, y, x + w - 1, y + h - 1);
    
    uint32_t pixels = w * h;
    for (uint32_t i = 0; i < pixels; i++) {
        LCD_WriteData16(color);
    }
}

void LCD_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    LCD_FillRect(x0 - r, y0, 2 * r + 1, 1, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        LCD_FillRect(x0 - x, y0 + y, 2 * x + 1, 1, color);
        LCD_FillRect(x0 - x, y0 - y, 2 * x + 1, 1, color);
        LCD_FillRect(x0 - y, y0 + x, 2 * y + 1, 1, color);
        LCD_FillRect(x0 - y, y0 - x, 2 * y + 1, 1, color);
    }
}

void LCD_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg) {
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t line = (c - 32) * 8 + i;
        for (uint8_t j = 0; j < 5; j++) {
            if (line & (1 << j)) {
                LCD_FillRect(x + j, y + i, 1, 1, color);
            } else {
                LCD_FillRect(x + j, y + i, 1, 1, bg);
            }
        }
    }
}

void LCD_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg) {
    while (*str) {
        LCD_DrawChar(x, y, *str++, color, bg);
        x += 6;
    }
}

void LCD_Init(void) {
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(100);

    LCD_WriteCmd(0x01);
    HAL_Delay(150);
    LCD_WriteCmd(0x11);
    HAL_Delay(255);

    LCD_WriteCmd(0x3A);
    LCD_WriteData(0x05);

    LCD_WriteCmd(0x36);
    LCD_WriteData(0x60);

    LCD_WriteCmd(0x29);
    HAL_Delay(100);

    HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_SET);

    LCD_FillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK);
}

void vDisplayTask(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(xLCDMutex, portMAX_DELAY) == pdTRUE) {
            LCD_DrawCircle(ball.x, ball.y, 8, COLOR_BLACK);
            
            ball.x += ball.vx;
            ball.y += ball.vy;
            
            if (ball.x <= 8 || ball.x >= LCD_WIDTH - 8) {
                ball.vx = -ball.vx;
            }
            if (ball.y <= 8 || ball.y >= LCD_HEIGHT - 8) {
                ball.vy = -ball.vy;
            }
            
            LCD_DrawCircle(ball.x, ball.y, 8, ball.color);
            frame_count++;
            
            xSemaphoreGive(xLCDMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(ANIMATION_DELAY_MS));
    }
}

void vStatusTask(void *pvParameters) {
    char status_buf[32];
    
    while (1) {
        if (xSemaphoreTake(xLCDMutex, portMAX_DELAY) == pdTRUE) {
            snprintf(status_buf, sizeof(status_buf), "Frame: %lu", frame_count);
            LCD_DrawString(5, 5, status_buf, COLOR_WHITE, COLOR_BLACK);
            
            snprintf(status_buf, sizeof(status_buf), "Heap: %u", xPortGetFreeHeapSize());
            LCD_DrawString(5, 15, status_buf, COLOR_YELLOW, COLOR_BLACK);
            
            xSemaphoreGive(xLCDMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(STATUS_UPDATE_MS));
    }
}

void vAnimationTask(void *pvParameters) {
    uint16_t colors[] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA};
    uint8_t color_idx = 0;
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        
        if (xSemaphoreTake(xLCDMutex, portMAX_DELAY) == pdTRUE) {
            color_idx = (color_idx + 1) % 6;
            ball.color = colors[color_idx];
            xSemaphoreGive(xLCDMutex);
        }
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    SPI1_Init();
    
    LCD_Init();
    
    xLCDMutex = xSemaphoreCreateMutex();
    
    xTaskCreate(vDisplayTask, "Display", DISPLAY_TASK_STACK, NULL, DISPLAY_TASK_PRIORITY, NULL);
    xTaskCreate(vStatusTask, "Status", STATUS_TASK_STACK, NULL, STATUS_TASK_PRIORITY, NULL);
    xTaskCreate(vAnimationTask, "Animation", ANIMATION_TASK_STACK, NULL, ANIMATION_TASK_PRIORITY, NULL);
    
    vTaskStartScheduler();
    
    while (1) {}
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

#if defined(STM32F103xB)
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL16;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#elif defined(STM32F401xC) || defined(STM32F411xE)
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = 8;
    RCC_OscInitStruct.PLL.PLLN            = 84;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ            = 4;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#endif
}

void GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LCD_CS_PIN | LCD_DC_PIN | LCD_RST_PIN | LCD_BL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
}

void SPI1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

#if defined(STM32F103xB)
    /* F1: AF_PP without Alternate field, no MISO needed for LCD (TX-only) */
    GPIO_InitStruct.Pin   = LCD_SCK_PIN | LCD_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#elif defined(STM32F401xC) || defined(STM32F411xE)
    GPIO_InitStruct.Pin       = LCD_SCK_PIN | LCD_MOSI_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi1);
}

void SysTick_Handler(void) {
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        extern void xPortSysTickHandler(void);
        xPortSysTickHandler();
    }
}

void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName) {
    while (1) {}
}

void vApplicationMallocFailedHook(void) {
    while (1) {}
}
