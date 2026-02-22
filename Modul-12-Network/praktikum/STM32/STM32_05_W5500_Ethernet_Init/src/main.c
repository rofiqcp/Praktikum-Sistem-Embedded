/**
 * STM32_05_W5500_Ethernet_Init
 * 
 * Initialize W5500 Ethernet module via SPI on STM32 Blue Pill.
 * Raw SPI register access (no library) to configure network parameters.
 *
 * Wiring:
 *   W5500 MOSI  -> PA7 (SPI1_MOSI)
 *   W5500 MISO  -> PA6 (SPI1_MISO)
 *   W5500 SCLK  -> PA5 (SPI1_SCK)
 *   W5500 CS    -> PA4 (GPIO output)
 *   W5500 RST   -> PB0 (GPIO output)
 *   W5500 INT   -> PB1 (GPIO input)
 *   UART1 TX    -> PA9
 *   UART1 RX    -> PA10
 *   LED         -> PC13
 */
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
/* ---- W5500 Register Definitions ---- */
/* Common Registers (Block Select = 0x00) */
#define W5500_MR          0x0000   /* Mode Register */
#define W5500_GAR0        0x0001   /* Gateway Address */
#define W5500_SUBR0       0x0005   /* Subnet Mask */
#define W5500_SHAR0       0x0009   /* Source MAC Address */
#define W5500_SIPR0       0x000F   /* Source IP Address */
#define W5500_PHYCFGR     0x002E   /* PHY Configuration */
#define W5500_VERSIONR    0x0039   /* Chip Version (should be 0x04) */
/* Block Select Bits (control byte bits [7:3]) */
#define W5500_BSB_COMMON  0x00     /* Common register block */
#define W5500_WRITE       0x04     /* Write access (RWB bit) */
#define W5500_READ        0x00     /* Read access */
#define W5500_OM_VDM      0x00     /* Variable data length mode */
/* ---- Pin Definitions ---- */
#define W5500_CS_PORT     GPIOA
#define W5500_CS_PIN      GPIO_PIN_4
#define W5500_RST_PORT    GPIOB
#define W5500_RST_PIN     GPIO_PIN_0
#define W5500_INT_PORT    GPIOB
#define W5500_INT_PIN     GPIO_PIN_1
/* ---- Handles ---- */
static SPI_HandleTypeDef hspi1;
static UART_HandleTypeDef huart1;
/* ---- Network Configuration ---- */
static uint8_t mac_addr[6]    = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x05};
static uint8_t ip_addr[4]     = {192, 168, 1, 100};
static uint8_t subnet[4]      = {255, 255, 255, 0};
static uint8_t gateway[4]     = {192, 168, 1, 1};
/* ---- Function Prototypes ---- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void SPI1_Init(void);
static void UART1_Init(void);
static void UART_Printf(const char *fmt, ...);
static void W5500_CS_Select(void);
static void W5500_CS_Deselect(void);
static void W5500_HardReset(void);
static void W5500_WriteByte(uint16_t addr, uint8_t bsb, uint8_t data);
static uint8_t W5500_ReadByte(uint16_t addr, uint8_t bsb);
static void W5500_WriteBytes(uint16_t addr, uint8_t bsb, const uint8_t *buf, uint16_t len);
static void W5500_ReadBytes(uint16_t addr, uint8_t bsb, uint8_t *buf, uint16_t len);
static void W5500_Init(void);
static void W5500_PrintStatus(void);
static void vMainTask(void *pvParameters);
/* ---- SPI Chip Select ---- */
static void W5500_CS_Select(void) {
    HAL_GPIO_WritePin(W5500_CS_PORT, W5500_CS_PIN, GPIO_PIN_RESET);
}
static void W5500_CS_Deselect(void) {
    HAL_GPIO_WritePin(W5500_CS_PORT, W5500_CS_PIN, GPIO_PIN_SET);
}
/* ---- W5500 Hard Reset ---- */
static void W5500_HardReset(void) {
    HAL_GPIO_WritePin(W5500_RST_PORT, W5500_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(W5500_RST_PORT, W5500_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
}
/* ---- W5500 SPI Frame: [Addr(16bit)] [Control(8bit)] [Data...] ---- */
static void W5500_WriteByte(uint16_t addr, uint8_t bsb, uint8_t data) {
    uint8_t frame[4];
    frame[0] = (uint8_t)(addr >> 8);
    frame[1] = (uint8_t)(addr & 0xFF);
    frame[2] = (bsb << 3) | W5500_WRITE | W5500_OM_VDM;
    frame[3] = data;
    W5500_CS_Select();
    HAL_SPI_Transmit(&hspi1, frame, 4, HAL_MAX_DELAY);
    W5500_CS_Deselect();
}
static uint8_t W5500_ReadByte(uint16_t addr, uint8_t bsb) {
    uint8_t frame[3];
    uint8_t data = 0;
    frame[0] = (uint8_t)(addr >> 8);
    frame[1] = (uint8_t)(addr & 0xFF);
    frame[2] = (bsb << 3) | W5500_READ | W5500_OM_VDM;
    W5500_CS_Select();
    HAL_SPI_Transmit(&hspi1, frame, 3, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, &data, 1, HAL_MAX_DELAY);
    W5500_CS_Deselect();
    return data;
}
static void W5500_WriteBytes(uint16_t addr, uint8_t bsb, const uint8_t *buf, uint16_t len) {
    uint8_t frame[3];
    frame[0] = (uint8_t)(addr >> 8);
    frame[1] = (uint8_t)(addr & 0xFF);
    frame[2] = (bsb << 3) | W5500_WRITE | W5500_OM_VDM;
    W5500_CS_Select();
    HAL_SPI_Transmit(&hspi1, frame, 3, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, (uint8_t *)buf, len, HAL_MAX_DELAY);
    W5500_CS_Deselect();
}
static void W5500_ReadBytes(uint16_t addr, uint8_t bsb, uint8_t *buf, uint16_t len) {
    uint8_t frame[3];
    frame[0] = (uint8_t)(addr >> 8);
    frame[1] = (uint8_t)(addr & 0xFF);
    frame[2] = (bsb << 3) | W5500_READ | W5500_OM_VDM;
    W5500_CS_Select();
    HAL_SPI_Transmit(&hspi1, frame, 3, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, buf, len, HAL_MAX_DELAY);
    W5500_CS_Deselect();
}
/* ---- W5500 Initialization ---- */
static void W5500_Init(void) {
    W5500_HardReset();
    /* Software reset via MR register */
    W5500_WriteByte(W5500_MR, W5500_BSB_COMMON, 0x80);
    HAL_Delay(100);
    /* Verify chip version */
    uint8_t ver = W5500_ReadByte(W5500_VERSIONR, W5500_BSB_COMMON);
    UART_Printf("W5500 Version: 0x%02X (expected 0x04)\r\n", ver);
    /* Set Gateway */
    W5500_WriteBytes(W5500_GAR0, W5500_BSB_COMMON, gateway, 4);
    /* Set Subnet Mask */
    W5500_WriteBytes(W5500_SUBR0, W5500_BSB_COMMON, subnet, 4);
    /* Set MAC Address */
    W5500_WriteBytes(W5500_SHAR0, W5500_BSB_COMMON, mac_addr, 6);
    /* Set Source IP */
    W5500_WriteBytes(W5500_SIPR0, W5500_BSB_COMMON, ip_addr, 4);
    UART_Printf("W5500 Network configured:\r\n");
    UART_Printf("  MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                mac_addr[0], mac_addr[1], mac_addr[2],
                mac_addr[3], mac_addr[4], mac_addr[5]);
    UART_Printf("  IP:  %d.%d.%d.%d\r\n", ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);
    UART_Printf("  Sub: %d.%d.%d.%d\r\n", subnet[0], subnet[1], subnet[2], subnet[3]);
    UART_Printf("  GW:  %d.%d.%d.%d\r\n", gateway[0], gateway[1], gateway[2], gateway[3]);
}
/* ---- Print W5500 Status ---- */
static void W5500_PrintStatus(void) {
    /* Read back IP to verify write */
    uint8_t read_ip[4];
    W5500_ReadBytes(W5500_SIPR0, W5500_BSB_COMMON, read_ip, 4);
    UART_Printf("Read-back IP: %d.%d.%d.%d\r\n",
                read_ip[0], read_ip[1], read_ip[2], read_ip[3]);
    /* Read back MAC */
    uint8_t read_mac[6];
    W5500_ReadBytes(W5500_SHAR0, W5500_BSB_COMMON, read_mac, 6);
    UART_Printf("Read-back MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                read_mac[0], read_mac[1], read_mac[2],
                read_mac[3], read_mac[4], read_mac[5]);
    /* PHY config register */
    uint8_t phy = W5500_ReadByte(W5500_PHYCFGR, W5500_BSB_COMMON);
    UART_Printf("PHY Config: 0x%02X\r\n", phy);
    UART_Printf("  Link:   %s\r\n", (phy & 0x01) ? "UP" : "DOWN");
    UART_Printf("  Speed:  %s\r\n", (phy & 0x02) ? "100Mbps" : "10Mbps");
    UART_Printf("  Duplex: %s\r\n", (phy & 0x04) ? "Full" : "Half");
}
/* ---- UART Printf Helper ---- */
static void UART_Printf(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)len, HAL_MAX_DELAY);
    }
}
/* ---- Main Task ---- */
static void vMainTask(void *pvParameters) {
    (void)pvParameters;
    UART_Printf("\r\n=== STM32 W5500 Ethernet Init ===\r\n");
    UART_Printf("SPI1: SCK=PA5 MISO=PA6 MOSI=PA7 CS=PA4\r\n");
    UART_Printf("W5500: RST=PB0 INT=PB1\r\n\r\n");
    W5500_Init();
    UART_Printf("\r\n--- Verifying Configuration ---\r\n");
    W5500_PrintStatus();
    uint32_t cycle = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        cycle++;
        UART_Printf("\r\n[Cycle %lu] Status Check:\r\n", cycle);
        uint8_t phy = W5500_ReadByte(W5500_PHYCFGR, W5500_BSB_COMMON);
        UART_Printf("  PHY=0x%02X Link=%s\r\n", phy, (phy & 0x01) ? "UP" : "DOWN");
        /* Toggle LED to show alive */
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    }
}
/* ============================================================
 *  System Clock Configuration (Multi-platform)
 * ============================================================ */
#ifdef STM32F103xB
/* F103: 8 MHz HSE -> PLL x9 -> 72 MHz SYSCLK */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState            = RCC_HSE_ON;
    osc.HSEPredivValue      = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL          = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}
#elif defined(STM32F401xC) || defined(STM32F411xE)
/* F4xx: 25 MHz HSE -> PLL -> 84 MHz SYSCLK */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM       = 25;
    osc.PLL.PLLN       = 336;
    osc.PLL.PLLP       = RCC_PLLP_DIV4;   /* 336/4 = 84 MHz */
    osc.PLL.PLLQ       = 7;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}
#endif
/* ---- GPIO Init ---- */
static void GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    /* LED PC13 */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
    /* W5500 CS - PA4 (manual control, not SPI NSS) */
    gpio.Pin = W5500_CS_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(W5500_CS_PORT, &gpio);
    W5500_CS_Deselect();
    /* W5500 RST - PB0 */
    gpio.Pin = W5500_RST_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(W5500_RST_PORT, &gpio);
    HAL_GPIO_WritePin(W5500_RST_PORT, W5500_RST_PIN, GPIO_PIN_SET);
    /* W5500 INT - PB1 (input) */
    gpio.Pin = W5500_INT_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(W5500_INT_PORT, &gpio);
}
/* ---- SPI1: W5500 (PA5=SCK, PA6=MISO, PA7=MOSI) ---- */
static void SPI1_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
#ifdef STM32F103xB
    g.Pin   = GPIO_PIN_5 | GPIO_PIN_7;  /* SCK, MOSI */
    g.Mode  = GPIO_MODE_AF_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin   = GPIO_PIN_6;               /* MISO */
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
#elif defined(STM32F401xC) || defined(STM32F411xE)
    g.Pin       = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &g);
#endif
    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi1);
}
/* ---- UART1: Debug (PA9 TX, PA10 RX) ---- */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
#ifdef STM32F103xB
    g.Pin   = GPIO_PIN_9;
    g.Mode  = GPIO_MODE_AF_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin   = GPIO_PIN_10;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
#elif defined(STM32F401xC) || defined(STM32F411xE)
    g.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_PULLUP;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &g);
#endif
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = UART_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}
/* ---- FreeRTOS Hooks ---- */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    UART_Printf("!!! Stack overflow: %s\r\n", pcTaskName);
    for (;;);
}
void vApplicationMallocFailedHook(void) {
    UART_Printf("!!! Malloc failed\r\n");
    for (;;);
}
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTCB, StackType_t **ppxIdleStack, uint32_t *pulIdleStackSize) {
    *ppxIdleTCB = &xIdleTaskTCB;
    *ppxIdleStack = uxIdleTaskStack;
    *pulIdleStackSize = configMINIMAL_STACK_SIZE;
}
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTCB, StackType_t **ppxTimerStack, uint32_t *pulTimerStackSize) {
    *ppxTimerTCB = &xTimerTaskTCB;
    *ppxTimerStack = uxTimerTaskStack;
    *pulTimerStackSize = configTIMER_TASK_STACK_DEPTH;
}
/* ---- Main ---- */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();
    SPI1_Init();
    xTaskCreate(vMainTask, "W5500Init", MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    for (;;);
}
