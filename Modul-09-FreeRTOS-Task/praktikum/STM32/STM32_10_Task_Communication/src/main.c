/**
 * ============================================================================
 * FILE: main.c
 * PROJECT: 06-Struct_Message_Passing
 * 
 * JUDUL: Pengiriman Data Struct via Queue
 * 
 * DESKRIPSI:
 * Demo pengiriman data kompleks (struct) melalui FreeRTOS Queue.
 * Producer task membaca ADC dan membungkus dalam struct SensorPacket_t,
 * kemudian mengirim ke Consumer melalui queue. Consumer menerima dan
 * menampilkan data via UART.
 * 
 * ============================================================================
 * ARSITEKTUR PROGRAM
 * ============================================================================
 * 
 *    ┌─────────────────────────────────────────────────────────────────────┐
 *    │                         SISTEM OVERVIEW                            │
 *    ├─────────────────────────────────────────────────────────────────────┤
 *    │                                                                     │
 *    │   ┌─────────┐    ┌──────────────────┐    ┌─────────────────────┐   │
 *    │   │   ADC   │───►│   vProducerTask  │───►│                     │   │
 *    │   │ (PA0)   │    │   (Baca sensor)  │    │                     │   │
 *    │   └─────────┘    └──────────────────┘    │       QUEUE         │   │
 *    │                                          │   ┌─────────────┐   │   │
 *    │                                          │   │ Packet[0]   │   │   │
 *    │                                          │   │ Packet[1]   │   │   │
 *    │                                          │   │ Packet[2]   │   │   │
 *    │                                          │   │ ...         │   │   │
 *    │                                          │   └─────────────┘   │   │
 *    │   ┌─────────┐    ┌──────────────────┐    │                     │   │
 *    │   │  UART   │◄───│  vConsumerTask   │◄───│                     │   │
 *    │   │ (PA9)   │    │  (Proses data)   │    └─────────────────────┘   │
 *    │   └─────────┘    └──────────────────┘                              │
 *    │                                                                     │
 *    └─────────────────────────────────────────────────────────────────────┘
 * 
 * ============================================================================
 * DEFINISI STRUCT
 * ============================================================================
 * 
 *    typedef struct {
 *        uint16_t   adcRaw;       // Nilai ADC 0-4095 (12-bit)
 *        uint16_t   sampleIndex;  // Counter sample ke-berapa
 *        TickType_t tickStamp;    // Waktu pengambilan (tick)
 *    } SensorPacket_t;
 *    
 *    Total size: 2 + 2 + 4 = 8 bytes per packet
 *    
 *    LAYOUT MEMORY:
 *    ┌────────────┬────────────┬────────────────────────┐
 *    │  adcRaw    │ sampleIdx  │      tickStamp         │
 *    │  (2 byte)  │  (2 byte)  │       (4 byte)         │
 *    └────────────┴────────────┴────────────────────────┘
 *    Offset: 0       2            4                     8
 * 
 * ============================================================================
 * ALUR EKSEKUSI
 * ============================================================================
 * 
 *    Time ───────────────────────────────────────────────────────►
 *    
 *    Producer: ████░░░░░░░░░████░░░░░░░░░████░░░░░░░░░████
 *              [read]       [read]       [read]       [read]
 *              [send]       [send]       [send]       [send]
 *                  │            │            │            │
 *                  ▼            ▼            ▼            ▼
 *    Queue:   [P1]         [P2]         [P3]         [P4]
 *                  │            │            │            │
 *                  ▼            ▼            ▼            ▼
 *    Consumer:     ████         ████         ████         ████
 *                 [recv]       [recv]       [recv]       [recv]
 *                 [print]      [print]      [print]      [print]
 * 
 * ============================================================================
 * EXPECTED OUTPUT (UART 115200 baud)
 * ============================================================================
 * 
 *    === Struct Message Passing Demo ===
 *    
 *    [PROD] Sample #1: ADC=2048 @ tick=500
 *    [CONS] Received: ADC=2048, Index=1, Tick=500, Latency=2ms
 *    
 *    [PROD] Sample #2: ADC=2100 @ tick=1000
 *    [CONS] Received: ADC=2100, Index=2, Tick=1000, Latency=1ms
 * 
 * ============================================================================
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * DEFINISI STRUCT PAKET SENSOR
 * ============================================================================
 * 
 * Struct ini akan di-COPY ke dalam queue, bukan pointer-nya.
 * Artinya setiap slot queue akan menyimpan salinan lengkap dari struct.
 */
typedef struct {
    uint16_t   adcRaw;        /* Nilai ADC mentah (0-4095) */
    uint16_t   sampleIndex;   /* Counter nomor sample */
    TickType_t tickStamp;     /* Timestamp dalam tick */
} SensorPacket_t;

/* ============================================================================
 * HANDLE GLOBAL
 * ============================================================================ */

static QueueHandle_t xSensorQueue = NULL;

static UART_HandleTypeDef huart1;
static ADC_HandleTypeDef  hadc1;

/* ============================================================================
 * PROTOTYPE
 * ============================================================================ */

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART_Init(void);
static void ADC_Init(void);
static void UART_SendString(const char *str);

static void vProducerTask(void *pvParameters);
static void vConsumerTask(void *pvParameters);

/* ============================================================================
 * IMPLEMENTASI TASK
 * ============================================================================ */

/**
 * @brief Task Producer - Membaca ADC dan mengirim ke queue
 * 
 * ILUSTRASI PRODUCER:
 * ═══════════════════════════════════════════════════════════════════════════
 *    
 *    Loop setiap 500ms:
 *    ┌───────────────────────────────────────────────────────────────────────┐
 *    │                                                                       │
 *    │   ┌─────────────────┐                                                 │
 *    │   │ 1. Baca ADC     │  HAL_ADC_GetValue() → adcRaw = 2048            │
 *    │   └────────┬────────┘                                                 │
 *    │            ▼                                                          │
 *    │   ┌─────────────────┐                                                 │
 *    │   │ 2. Isi Struct   │  pkt.adcRaw = 2048                             │
 *    │   │                 │  pkt.sampleIndex = 5                           │
 *    │   │                 │  pkt.tickStamp = xTaskGetTickCount()           │
 *    │   └────────┬────────┘                                                 │
 *    │            ▼                                                          │
 *    │   ┌─────────────────┐                                                 │
 *    │   │ 3. Kirim Queue  │  xQueueSend(xSensorQueue, &pkt, timeout)       │
 *    │   │                 │                                                 │
 *    │   │   ┌─────────┐   │     ┌─────────────────────────────────┐        │
 *    │   │   │  pkt    │───┼────►│ Queue: [pkt] [  ] [  ] [  ] [  ]│        │
 *    │   │   └─────────┘   │     └─────────────────────────────────┘        │
 *    │   └────────┬────────┘                                                 │
 *    │            ▼                                                          │
 *    │   ┌─────────────────┐                                                 │
 *    │   │ 4. Delay 500ms  │  vTaskDelay(pdMS_TO_TICKS(500))                │
 *    │   └─────────────────┘                                                 │
 *    │                                                                       │
 *    └───────────────────────────────────────────────────────────────────────┘
 *    
 * ═══════════════════════════════════════════════════════════════════════════
 */
static void vProducerTask(void *pvParameters)
{
    (void)pvParameters;
    
    SensorPacket_t packet;
    uint16_t sampleCounter = 0;
    char buffer[80];
    
    UART_SendString("\r\n[PROD] Producer task started\r\n");
    
    for(;;)
    {
        sampleCounter++;
        
        /* 1. Baca nilai ADC */
        HAL_ADC_Start(&hadc1);
        if(HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
        {
            packet.adcRaw = HAL_ADC_GetValue(&hadc1);
        }
        else
        {
            packet.adcRaw = 0xFFFF;  /* Tandai error */
        }
        HAL_ADC_Stop(&hadc1);
        
        /* 2. Isi field struct lainnya */
        packet.sampleIndex = sampleCounter;
        packet.tickStamp = xTaskGetTickCount();
        
        /* 3. Log sebelum kirim */
        snprintf(buffer, sizeof(buffer),
                "[PROD] Sample #%u: ADC=%u @ tick=%lu\r\n",
                packet.sampleIndex, packet.adcRaw, packet.tickStamp);
        UART_SendString(buffer);
        
        /* 4. Kirim struct ke queue (copy, bukan pointer!) */
        if(xQueueSend(xSensorQueue, &packet, pdMS_TO_TICKS(20)) != pdPASS)
        {
            UART_SendString("[PROD] ERROR: Queue full!\r\n");
        }
        
        /* 5. Delay periodik */
        vTaskDelay(pdMS_TO_TICKS(PRODUCER_PERIOD_MS));
    }
}

/**
 * @brief Task Consumer - Menerima dan memproses data dari queue
 * 
 * ILUSTRASI CONSUMER:
 * ═══════════════════════════════════════════════════════════════════════════
 *    
 *    Loop terus-menerus (blocking pada queue):
 *    ┌───────────────────────────────────────────────────────────────────────┐
 *    │                                                                       │
 *    │   ┌─────────────────┐                                                 │
 *    │   │ 1. Tunggu Queue │  xQueueReceive(queue, &rx, portMAX_DELAY)      │
 *    │   │   (BLOCKING)    │                                                 │
 *    │   │                 │     ┌─────────────────────────────────┐        │
 *    │   │   ┌─────────┐   │     │ Queue: [pkt] [  ] [  ] [  ] [  ]│        │
 *    │   │   │   rx    │◄──┼─────│         ↑                       │        │
 *    │   │   └─────────┘   │     │        HEAD                     │        │
 *    │   └────────┬────────┘     └─────────────────────────────────┘        │
 *    │            ▼                                                          │
 *    │   ┌─────────────────┐                                                 │
 *    │   │ 2. Proses Data  │  Hitung latency, validasi, dll                 │
 *    │   └────────┬────────┘                                                 │
 *    │            ▼                                                          │
 *    │   ┌─────────────────┐                                                 │
 *    │   │ 3. Tampilkan    │  Print ke UART                                 │
 *    │   └────────┬────────┘                                                 │
 *    │            ▼                                                          │
 *    │   ┌─────────────────┐                                                 │
 *    │   │ 4. Toggle LED   │  Indikasi visual                               │
 *    │   └─────────────────┘                                                 │
 *    │                                                                       │
 *    └───────────────────────────────────────────────────────────────────────┘
 *    
 * ═══════════════════════════════════════════════════════════════════════════
 */
static void vConsumerTask(void *pvParameters)
{
    (void)pvParameters;
    
    SensorPacket_t rxPacket;
    char buffer[120];
    TickType_t receiveTime;
    uint32_t latencyMs;
    
    UART_SendString("[CONS] Consumer task started, waiting for data...\r\n\r\n");
    
    for(;;)
    {
        /* 1. Tunggu data dari queue (blocking) */
        if(xQueueReceive(xSensorQueue, &rxPacket, portMAX_DELAY) == pdPASS)
        {
            /* 2. Catat waktu terima untuk hitung latency */
            receiveTime = xTaskGetTickCount();
            latencyMs = (receiveTime - rxPacket.tickStamp) * 1000 / configTICK_RATE_HZ;
            
            /* 3. Toggle LED sebagai indikasi */
            HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
            
            /* 4. Tampilkan data yang diterima */
            snprintf(buffer, sizeof(buffer),
                    "[CONS] Received: ADC=%u, Index=%u, Tick=%lu, Latency=%lums\r\n\r\n",
                    rxPacket.adcRaw,
                    rxPacket.sampleIndex,
                    rxPacket.tickStamp,
                    latencyMs);
            UART_SendString(buffer);
        }
    }
}

/* ============================================================================
 * KONFIGURASI HARDWARE
 * ============================================================================ */

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
    
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

static void GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* LED PC13 */
    GPIO_InitStruct.Pin = LED_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
    
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_SET);
}

static void UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = DEBUG_UART_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DEBUG_UART_TX_PORT, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = DEBUG_UART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DEBUG_UART_RX_PORT, &GPIO_InitStruct);
    
    huart1.Instance = DEBUG_UART_INSTANCE;
    huart1.Init.BaudRate = DEBUG_UART_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

static void ADC_Init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);
    
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    HAL_ADCEx_Calibration_Start(&hadc1);
}

static void UART_SendString(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

/* ============================================================================
 * HOOKS FreeRTOS
 * ============================================================================ */

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for(;;);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for(;;);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART_Init();
    ADC_Init();
    
    UART_SendString("\r\n================================\r\n");
    UART_SendString("06-Struct_Message_Passing\r\n");
    UART_SendString("STM32F103 + FreeRTOS\r\n");
    UART_SendString("================================\r\n");
    UART_SendString("\r\n=== Struct Message Passing Demo ===\r\n");
    
    /* 
     * Buat queue untuk SensorPacket_t.
     * Queue akan meng-COPY struct, bukan menyimpan pointer.
     * Ini aman untuk variabel lokal di stack producer.
     */
    xSensorQueue = xQueueCreate(QUEUE_LENGTH, sizeof(SensorPacket_t));
    
    if(xSensorQueue == NULL)
    {
        UART_SendString("ERROR: Failed to create queue!\r\n");
        while(1);
    }
    
    xTaskCreate(vProducerTask, "Producer", PRODUCER_STACK, NULL,
                PRODUCER_PRIO, NULL);
    
    xTaskCreate(vConsumerTask, "Consumer", CONSUMER_STACK, NULL,
                CONSUMER_PRIO, NULL);
    
    vTaskStartScheduler();
    
    for(;;);
}
