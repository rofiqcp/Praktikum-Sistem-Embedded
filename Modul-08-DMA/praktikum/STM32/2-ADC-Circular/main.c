/*
 * Project: STM32 ADC Multi-Channel DMA (Circular Mode)
 * Board: STM32F103C8T6 (Blue Pill)
 * Channels: PA0 (Pot1), PA1 (Pot2), Internal Temp
 */

#include "main.h"

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

/* USER CODE BEGIN PV */
#define ADC_BUF_LEN 3
uint16_t adc_buf[ADC_BUF_LEN]; // Buffer: [0]=PA0, [1]=PA1, [2]=Temp
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();

    /* USER CODE BEGIN 2 */
    // Calibrate ADC
    HAL_ADCEx_Calibration_Start(&hadc1);

    // Start ADC in DMA Mode (Circular)
    // Data will automatically fill adc_buf indefinitely
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, ADC_BUF_LEN);
    /* USER CODE END 2 */

    while (1) {
        /* USER CODE BEGIN 3 */
        // No ADC code needed here! cpu is free!
        
        // Example: Logic based on Pot value
        uint16_t pot1_val = adc_buf[0];
        
        if (pot1_val > 2048) {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // LED ON
        } else {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   // LED OFF
        }
        
        HAL_Delay(50); // Just for loop pacing
        /* USER CODE END 3 */
    }
}

static void MX_ADC1_Init(void) {
    ADC_ChannelConfTypeDef sConfig = {0};

    // ADC1 Init: Scan Mode ENABLE, Continuous ENABLE
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 3;
    HAL_ADC_Init(&hadc1);

    // Channel 0 (PA0)
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    // Channel 1 (PA1)
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_2;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // Channel Temp (Internal)
    sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank = ADC_REGULAR_RANK_3;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

static void MX_DMA_Init(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();
    
    // DMA1 Channel 1 is hardwired to ADC1
    hdma_adc1.Instance = DMA1_Channel1;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE; // ADC Register fixed
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;     // Memory increments
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;          // CIRCULAR MODE
    hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;
    
    HAL_DMA_Init(&hdma_adc1);
    
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);
}
