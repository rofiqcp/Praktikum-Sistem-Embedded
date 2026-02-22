/**
 * STM32_07_W5500_UDP
 * 
 * UDP communication via W5500 SPI Ethernet on STM32 Blue Pill.
 * Opens socket in UDP mode, sends datagrams, receives responses.
 *
 * Wiring: Same as STM32_05 (SPI1 + CS=PA4, RST=PB0, INT=PB1)
 */
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
/* ---- W5500 Common Registers ---- */
#define W5500_MR          0x0000
#define W5500_GAR0        0x0001
#define W5500_SUBR0       0x0005
#define W5500_SHAR0       0x0009
#define W5500_SIPR0       0x000F
#define W5500_PHYCFGR     0x002E
#define W5500_VERSIONR    0x0039
/* Socket 0 Registers */
#define Sn_MR             0x0000
#define Sn_CR             0x0001
#define Sn_IR             0x0002
#define Sn_SR             0x0003
#define Sn_PORT0          0x0004
#define Sn_DIPR0          0x000C   /* Destination IP */
#define Sn_DPORT0         0x0010   /* Destination Port */
#define Sn_TX_FSR0        0x0020
#define Sn_TX_WR0         0x0024
#define Sn_RX_RSR0        0x0026
#define Sn_RX_RD0         0x0028
/* Socket Commands */
#define Sn_CR_OPEN        0x01
#define Sn_CR_SEND        0x20
#define Sn_CR_RECV        0x40
#define Sn_CR_CLOSE       0x10
/* Socket Status */
#define SOCK_CLOSED       0x00
#define SOCK_UDP          0x22
/* Socket Mode */
#define Sn_MR_UDP         0x02
/* Block Select */
#define W5500_BSB_COMMON  0x00
#define W5500_BSB_S0_REG  0x01
#define W5500_BSB_S0_TX   0x02
#define W5500_BSB_S0_RX   0x03
#define W5500_WRITE       0x04
#define W5500_READ        0x00
#define W5500_OM_VDM      0x00
#define LOCAL_UDP_PORT    5000
#define DEST_UDP_PORT     5001
#define RECV_BUF_SIZE     256
/* ---- Pins ---- */
#define W5500_CS_PORT     GPIOA
#define W5500_CS_PIN      GPIO_PIN_4
#define W5500_RST_PORT    GPIOB
#define W5500_RST_PIN     GPIO_PIN_0
static SPI_HandleTypeDef hspi1;
static UART_HandleTypeDef huart1;
static uint8_t mac_addr[6]    = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x07};
static uint8_t ip_addr[4]     = {192, 168, 1, 100};
static uint8_t subnet[4]      = {255, 255, 255, 0};
static uint8_t gateway[4]     = {192, 168, 1, 1};
static uint8_t dest_ip[4]     = {192, 168, 1, 10};  /* Target PC */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void SPI1_Init(void);
static void UART1_Init(void);
static void UART_Printf(const char *fmt, ...);
/* ---- W5500 SPI Low-Level ---- */
static void W5500_CS_Select(void)   { HAL_GPIO_WritePin(W5500_CS_PORT, W5500_CS_PIN, GPIO_PIN_RESET); }
static void W5500_CS_Deselect(void) { HAL_GPIO_WritePin(W5500_CS_PORT, W5500_CS_PIN, GPIO_PIN_SET); }
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
    uint8_t frame[3], data = 0;
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
static uint16_t W5500_ReadWord(uint16_t addr, uint8_t bsb) {
    return ((uint16_t)W5500_ReadByte(addr, bsb) << 8) | W5500_ReadByte(addr + 1, bsb);
}
static void W5500_WriteWord(uint16_t addr, uint8_t bsb, uint16_t val) {
    W5500_WriteByte(addr, bsb, (uint8_t)(val >> 8));
    W5500_WriteByte(addr + 1, bsb, (uint8_t)(val & 0xFF));
}
/* ---- W5500 Init ---- */
static void W5500_Init(void) {
    HAL_GPIO_WritePin(W5500_RST_PORT, W5500_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(W5500_RST_PORT, W5500_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    W5500_WriteByte(W5500_MR, W5500_BSB_COMMON, 0x80);
    HAL_Delay(100);
    uint8_t ver = W5500_ReadByte(W5500_VERSIONR, W5500_BSB_COMMON);
    UART_Printf("W5500 ver: 0x%02X\r\n", ver);
    W5500_WriteBytes(W5500_GAR0,  W5500_BSB_COMMON, gateway, 4);
    W5500_WriteBytes(W5500_SUBR0, W5500_BSB_COMMON, subnet, 4);
    W5500_WriteBytes(W5500_SHAR0, W5500_BSB_COMMON, mac_addr, 6);
    W5500_WriteBytes(W5500_SIPR0, W5500_BSB_COMMON, ip_addr, 4);
    UART_Printf("IP: %d.%d.%d.%d  Local Port: %d\r\n",
                ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], LOCAL_UDP_PORT);
    UART_Printf("Dest: %d.%d.%d.%d:%d\r\n",
                dest_ip[0], dest_ip[1], dest_ip[2], dest_ip[3], DEST_UDP_PORT);
}
/* ---- Socket Operations ---- */
static void Socket_Open_UDP(uint16_t port) {
    W5500_WriteByte(Sn_MR, W5500_BSB_S0_REG, Sn_MR_UDP);
    W5500_WriteWord(Sn_PORT0, W5500_BSB_S0_REG, port);
    W5500_WriteByte(Sn_CR, W5500_BSB_S0_REG, Sn_CR_OPEN);
    HAL_Delay(5);
    uint8_t sr = W5500_ReadByte(Sn_SR, W5500_BSB_S0_REG);
    UART_Printf("UDP Socket OPEN -> status: 0x%02X\r\n", sr);
}
static void Socket_SendTo(const uint8_t *data, uint16_t len, const uint8_t *dest, uint16_t dport) {
    /* Set destination IP and port */
    W5500_WriteBytes(Sn_DIPR0, W5500_BSB_S0_REG, dest, 4);
    W5500_WriteWord(Sn_DPORT0, W5500_BSB_S0_REG, dport);
    /* Write data to TX buffer */
    uint16_t wr_ptr = W5500_ReadWord(Sn_TX_WR0, W5500_BSB_S0_REG);
    W5500_WriteBytes(wr_ptr, W5500_BSB_S0_TX, data, len);
    wr_ptr += len;
    W5500_WriteWord(Sn_TX_WR0, W5500_BSB_S0_REG, wr_ptr);
    /* Issue SEND command */
    W5500_WriteByte(Sn_CR, W5500_BSB_S0_REG, Sn_CR_SEND);
    HAL_Delay(5);
    /* Wait for SEND done (check Sn_IR SEND_OK bit) */
    uint8_t ir;
    uint32_t timeout = HAL_GetTick() + 1000;
    do {
        ir = W5500_ReadByte(Sn_IR, W5500_BSB_S0_REG);
        if (HAL_GetTick() > timeout) { UART_Printf("  SEND timeout!\r\n"); break; }
    } while (!(ir & 0x10));
    W5500_WriteByte(Sn_IR, W5500_BSB_S0_REG, 0x10); /* Clear SEND_OK */
}
static uint16_t Socket_RecvFrom(uint8_t *buf, uint16_t max_len, uint8_t *src_ip, uint16_t *src_port) {
    uint16_t rx_size = W5500_ReadWord(Sn_RX_RSR0, W5500_BSB_S0_REG);
    if (rx_size == 0) return 0;
    uint16_t rd_ptr = W5500_ReadWord(Sn_RX_RD0, W5500_BSB_S0_REG);
    /* UDP RX buffer: [4-byte src IP][2-byte src port][2-byte data len][data...] */
    uint8_t header[8];
    W5500_ReadBytes(rd_ptr, W5500_BSB_S0_RX, header, 8);
    rd_ptr += 8;
    if (src_ip) { memcpy(src_ip, header, 4); }
    if (src_port) { *src_port = ((uint16_t)header[4] << 8) | header[5]; }
    uint16_t data_len = ((uint16_t)header[6] << 8) | header[7];
    if (data_len > max_len) data_len = max_len;
    W5500_ReadBytes(rd_ptr, W5500_BSB_S0_RX, buf, data_len);
    rd_ptr += data_len;
    W5500_WriteWord(Sn_RX_RD0, W5500_BSB_S0_REG, rd_ptr);
    W5500_WriteByte(Sn_CR, W5500_BSB_S0_REG, Sn_CR_RECV);
    HAL_Delay(1);
    return data_len;
}
static void Socket_Close(void) {
    W5500_WriteByte(Sn_CR, W5500_BSB_S0_REG, Sn_CR_CLOSE);
    HAL_Delay(5);
}
/* ---- Main Task ---- */
static void vMainTask(void *pvParameters) {
    (void)pvParameters;
    uint8_t recv_buf[RECV_BUF_SIZE];
    uint8_t src_ip[4];
    uint16_t src_port;
    uint32_t msg_count = 0;
    UART_Printf("\r\n=== STM32 W5500 UDP Communication ===\r\n");
    W5500_Init();
    Socket_Open_UDP(LOCAL_UDP_PORT);
    for (;;) {
        /* Send periodic UDP datagram */
        msg_count++;
        char msg[64];
        int mlen = snprintf(msg, sizeof(msg), "STM32 UDP #%lu uptime=%lums", msg_count, HAL_GetTick());
        Socket_SendTo((uint8_t *)msg, (uint16_t)mlen, dest_ip, DEST_UDP_PORT);
        UART_Printf("[TX] Sent %d bytes to %d.%d.%d.%d:%d\r\n",
                    mlen, dest_ip[0], dest_ip[1], dest_ip[2], dest_ip[3], DEST_UDP_PORT);
        /* Check for incoming datagrams */
        for (int poll = 0; poll < 50; poll++) {
            uint16_t rx_len = Socket_RecvFrom(recv_buf, RECV_BUF_SIZE - 1, src_ip, &src_port);
            if (rx_len > 0) {
                recv_buf[rx_len] = '\0';
                UART_Printf("[RX] %u bytes from %d.%d.%d.%d:%d -> %s\r\n",
                            rx_len, src_ip[0], src_ip[1], src_ip[2], src_ip[3], src_port, recv_buf);
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        UART_Printf("--- cycle %lu ---\r\n", msg_count);
    }
}
/* ---- UART Printf ---- */
static void UART_Printf(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)len, HAL_MAX_DELAY);
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
static void GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = LED_PIN; gpio.Mode = GPIO_MODE_OUTPUT_PP; gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
    gpio.Pin = W5500_CS_PIN; gpio.Mode = GPIO_MODE_OUTPUT_PP; gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(W5500_CS_PORT, &gpio);
    W5500_CS_Deselect();
    gpio.Pin = W5500_RST_PIN; gpio.Mode = GPIO_MODE_OUTPUT_PP; gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(W5500_RST_PORT, &gpio);
    HAL_GPIO_WritePin(W5500_RST_PORT, W5500_RST_PIN, GPIO_PIN_SET);
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
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask; UART_Printf("!!! Stack overflow: %s\r\n", pcTaskName); for (;;);
}
void vApplicationMallocFailedHook(void) { UART_Printf("!!! Malloc failed\r\n"); for (;;); }
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTCB, StackType_t **ppxIdleStack, uint32_t *pulIdleStackSize) {
    *ppxIdleTCB = &xIdleTaskTCB; *ppxIdleStack = uxIdleTaskStack; *pulIdleStackSize = configMINIMAL_STACK_SIZE;
}
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTCB, StackType_t **ppxTimerStack, uint32_t *pulTimerStackSize) {
    *ppxTimerTCB = &xTimerTaskTCB; *ppxTimerStack = uxTimerTaskStack; *pulTimerStackSize = configTIMER_TASK_STACK_DEPTH;
}
int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();
    SPI1_Init();
    xTaskCreate(vMainTask, "UDPComm", MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    for (;;);
}
