/**
 * ==========================================================
 *  Program     : PWM Buzzer Melody - Twinkle Twinkle
 *  Modul       : 05 - DAC & PWM
 *  MCU         : STM32F103C8 / STM32F401CC / STM32F411CE
 *  Deskripsi   : Memainkan melodi "Twinkle Twinkle Little Star"
 *                menggunakan passive buzzer pada TIM2_CH1 (PA0).
 *                Frekuensi PWM diubah untuk menghasilkan nada.
 * ==========================================================
 */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#define SYSTEM_CLOCK  72000000UL
#elif defined(STM32F4)
#include "stm32f4xx_hal.h"
#define SYSTEM_CLOCK  84000000UL
#endif

#include <stdio.h>
#include <string.h>

/* ===================== Handle Peripheral ===================== */
UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim2;

/* ===================== Definisi Frekuensi Not ===================== */
/* Not musik: frekuensi dalam Hz */
#define NOTE_C4   262
#define NOTE_CS4  277
#define NOTE_D4   294
#define NOTE_DS4  311
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_FS4  370
#define NOTE_G4   392
#define NOTE_GS4  415
#define NOTE_A4   440
#define NOTE_AS4  466
#define NOTE_B4   494
#define NOTE_C5   523
#define NOTE_D5   587
#define NOTE_E5   659
#define NOTE_F5   698
#define NOTE_G5   784
#define NOTE_A5   880
#define NOTE_REST 0     /* Diam (istirahat) */

/* Durasi not (dalam milidetik) */
#define WHOLE     1600
#define HALF      800
#define QUARTER   400
#define EIGHTH    200
#define SIXTEENTH 100

/* ===================== Struktur Not Musik ===================== */
typedef struct {
    uint16_t frekuensi;
    uint16_t durasi;
} Note_t;

/* ===================== Melodi Twinkle Twinkle Little Star ===================== */
static const Note_t melodi[] = {
    /* Twinkle twinkle little star */
    {NOTE_C4, QUARTER}, {NOTE_C4, QUARTER}, {NOTE_G4, QUARTER}, {NOTE_G4, QUARTER},
    {NOTE_A4, QUARTER}, {NOTE_A4, QUARTER}, {NOTE_G4, HALF},

    /* How I wonder what you are */
    {NOTE_F4, QUARTER}, {NOTE_F4, QUARTER}, {NOTE_E4, QUARTER}, {NOTE_E4, QUARTER},
    {NOTE_D4, QUARTER}, {NOTE_D4, QUARTER}, {NOTE_C4, HALF},

    /* Up above the world so high */
    {NOTE_G4, QUARTER}, {NOTE_G4, QUARTER}, {NOTE_F4, QUARTER}, {NOTE_F4, QUARTER},
    {NOTE_E4, QUARTER}, {NOTE_E4, QUARTER}, {NOTE_D4, HALF},

    /* Like a diamond in the sky */
    {NOTE_G4, QUARTER}, {NOTE_G4, QUARTER}, {NOTE_F4, QUARTER}, {NOTE_F4, QUARTER},
    {NOTE_E4, QUARTER}, {NOTE_E4, QUARTER}, {NOTE_D4, HALF},

    /* Twinkle twinkle little star */
    {NOTE_C4, QUARTER}, {NOTE_C4, QUARTER}, {NOTE_G4, QUARTER}, {NOTE_G4, QUARTER},
    {NOTE_A4, QUARTER}, {NOTE_A4, QUARTER}, {NOTE_G4, HALF},

    /* How I wonder what you are */
    {NOTE_F4, QUARTER}, {NOTE_F4, QUARTER}, {NOTE_E4, QUARTER}, {NOTE_E4, QUARTER},
    {NOTE_D4, QUARTER}, {NOTE_D4, QUARTER}, {NOTE_C4, HALF},

    /* Akhir: istirahat */
    {NOTE_REST, WHOLE}
};

static const uint16_t jumlah_not = sizeof(melodi) / sizeof(melodi[0]);

/* ===================== Retarget printf ===================== */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ===================== SysTick Handler ===================== */
void SysTick_Handler(void) {
    HAL_IncTick();
}

/* ===================== Konfigurasi Clock ===================== */
void SystemClock_Config(void) {
#ifdef STM32F1
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#elif defined(STM32F4)
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 84;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#endif
}

/* ===================== Inisialisasi UART1 ===================== */
void MX_USART1_UART_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

#ifdef STM32F1
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#elif defined(STM32F4)
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/* ===================== Inisialisasi PWM TIM2 untuk Buzzer ===================== */
void MX_TIM2_Buzzer_Init(void) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
#ifdef STM32F1
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#elif defined(STM32F4)
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    /* Konfigurasi awal (akan di-update per not) */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 65535;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

/* ===================== Fungsi Memainkan Nada ===================== */
void Buzzer_PlayNote(uint16_t frequency, uint16_t duration_ms) {
    if (frequency == NOTE_REST || frequency == 0) {
        /* Diam: matikan PWM output */
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
        HAL_Delay(duration_ms);
        return;
    }

    /* Hitung period untuk frekuensi yang diminta */
    uint32_t period = (SYSTEM_CLOCK / frequency) - 1;
    uint32_t prescaler = 0;

    /* Jika period terlalu besar, gunakan prescaler */
    if (period > 65535) {
        prescaler = (period / 65536) + 1;
        period = (SYSTEM_CLOCK / ((prescaler + 1) * frequency)) - 1;
    }

    __HAL_TIM_SET_PRESCALER(&htim2, prescaler);
    __HAL_TIM_SET_AUTORELOAD(&htim2, period);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, period / 2); /* 50% duty */

    /* Generate update event */
    htim2.Instance->EGR = TIM_EGR_UG;

    /* Mainkan not selama durasi yang ditentukan */
    /* 90% berbunyi, 10% jeda (untuk artikulasi) */
    HAL_Delay((duration_ms * 9) / 10);

    /* Matikan buzzer untuk jeda antar not */
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
    HAL_Delay(duration_ms / 10);
}

/* ===================== Nama Not dari Frekuensi ===================== */
const char* Freq_to_Name(uint16_t freq) {
    switch (freq) {
        case NOTE_C4:  return "C4 ";
        case NOTE_CS4: return "C#4";
        case NOTE_D4:  return "D4 ";
        case NOTE_DS4: return "D#4";
        case NOTE_E4:  return "E4 ";
        case NOTE_F4:  return "F4 ";
        case NOTE_FS4: return "F#4";
        case NOTE_G4:  return "G4 ";
        case NOTE_GS4: return "G#4";
        case NOTE_A4:  return "A4 ";
        case NOTE_AS4: return "A#4";
        case NOTE_B4:  return "B4 ";
        case NOTE_C5:  return "C5 ";
        case NOTE_REST:return "---";
        default:       return "???";
    }
}

/* ===================== Program Utama ===================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_TIM2_Buzzer_Init();

    printf("\r\n=== PWM Buzzer Melody ===\r\n");
    printf("Passive buzzer pada TIM2_CH1 (PA0)\r\n");
    printf("Melodi: Twinkle Twinkle Little Star\r\n");
    printf("Jumlah not: %u\r\n\r\n", jumlah_not);

    uint32_t play_count = 0;

    while (1) {
        play_count++;
        printf("--- Putar ke-%lu ---\r\n", play_count);

        /* Mainkan seluruh melodi */
        for (uint16_t i = 0; i < jumlah_not; i++) {
            printf("[MELODY] Not %2u/%u: %s | Freq=%4u Hz | Dur=%u ms\r\n",
                   i + 1, jumlah_not,
                   Freq_to_Name(melodi[i].frekuensi),
                   melodi[i].frekuensi,
                   melodi[i].durasi);

            Buzzer_PlayNote(melodi[i].frekuensi, melodi[i].durasi);
        }

        printf("[MELODY] Selesai! Jeda 3 detik...\r\n\r\n");
        HAL_Delay(3000);
    }
}
