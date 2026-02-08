/**
 * ============================================================================
 * FILE: main.c
 * PROJECT: 03-Absolute_Timing_Control
 * 
 * DESKRIPSI PROGRAM:
 * Program ini mendemonstrasikan perbedaan antara dua metode delay di FreeRTOS:
 * 1. vTaskDelay() - Delay RELATIF (timing tidak presisi, ada jitter)
 * 2. vTaskDelayUntil() - Delay ABSOLUT (timing presisi, bebas jitter)
 * 
 * ============================================================================
 * KONSEP DASAR: RELATIVE vs ABSOLUTE TIMING
 * ============================================================================
 * 
 * RELATIVE TIMING (vTaskDelay):
 * ┌──────────────────────────────────────────────────────────────────┐
 * │  Eksekusi    │ vTaskDelay(100) │  Eksekusi    │ vTaskDelay(100) │
 * │   (10ms)     │   ───────>      │   (15ms)     │   ───────>      │
 * └──────────────┴─────────────────┴──────────────┴─────────────────┘
 * │<── 110ms ───>│                 │<── 115ms ───>│
 * 
 * Periode TIDAK KONSISTEN karena delay dihitung SETELAH eksekusi selesai.
 * 
 * ABSOLUTE TIMING (vTaskDelayUntil):
 * ┌──────────────────────────────────────────────────────────────────┐
 * │  Eksekusi    │     Delay      │  Eksekusi    │     Delay       │
 * │   (10ms)     │   (90ms)       │   (15ms)     │   (85ms)        │
 * └──────────────┴────────────────┴──────────────┴─────────────────┘
 * │<────── 100ms ──────>│         │<────── 100ms ──────>│
 * 
 * Periode KONSISTEN 100ms karena delay dihitung dari waktu WAKE terakhir.
 * Delay otomatis dikurangi sesuai waktu eksekusi.
 * 
 * ============================================================================
 * APLIKASI PRACTICAL:
 * - Sampling sensor dengan frekuensi tepat (accelerometer, gyroscope)
 * - Kontrol motor dengan PWM timing presisi
 * - Akuisisi data untuk FFT (membutuhkan sampling rate konstan)
 * - Protokol komunikasi dengan timing ketat
 * 
 * HARDWARE TARGET: STM32F103C8T6 (Blue Pill)
 * - CPU: 72MHz (HSE 8MHz + PLL x9)
 * - LED: PC13 (aktif LOW) - Toggle saat sampling
 * - ADC: PA0 (ADC1_CH0) - Simulasi sensor
 * - UART: PA9 (TX), PA10 (RX) @ 115200 baud
 * ============================================================================
 */

/* ============================================================================
 * INCLUDE HEADERS
 * ============================================================================ */

#include "stm32f1xx_hal.h"      /* HAL library untuk STM32F1 series */
#include "FreeRTOS.h"          /* FreeRTOS kernel utama */
#include "task.h"              /* API untuk task management */
#include <string.h>            /* Library untuk strlen() */
#include <stdio.h>             /* Library untuk sprintf() */

/* ============================================================================
 * HANDLE PERIPHERAL GLOBAL
 * ============================================================================
 * Handle ini digunakan untuk mengakses peripheral dari berbagai fungsi.
 * Dideklarasikan global agar bisa digunakan oleh task dan fungsi lain.
 * ============================================================================ */

UART_HandleTypeDef huart1;     /* Handle untuk USART1 - komunikasi serial debug */
ADC_HandleTypeDef hadc1;       /* Handle untuk ADC1 - pembacaan sensor analog */

/* ============================================================================
 * DEKLARASI FUNGSI (FUNCTION PROTOTYPES)
 * ============================================================================
 * Forward declaration agar fungsi bisa dipanggil sebelum didefinisikan.
 * ============================================================================ */

/* Fungsi konfigurasi sistem dan peripheral */
void SystemClock_Config(void);               /* Konfigurasi clock system 72MHz */
static void MX_GPIO_Init(void);              /* Inisialisasi GPIO (LED) */
static void MX_USART1_UART_Init(void);       /* Inisialisasi UART debug */
static void MX_ADC1_Init(void);              /* Inisialisasi ADC sensor */

/* Fungsi utilitas */
void UART_SendString(const char *str);       /* Kirim string via UART */

/* Task FreeRTOS - dua task untuk perbandingan */
void vRelativeDelayTask(void *pvParameters); /* Task dengan vTaskDelay (ada jitter) */
void vAbsoluteDelayTask(void *pvParameters); /* Task dengan vTaskDelayUntil (presisi) */

/* FreeRTOS hook functions */
void vApplicationMallocFailedHook(void);     /* Dipanggil jika malloc gagal */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);  /* Stack overflow */

/* ============================================================================
 * FUNGSI UTILITAS: UART_SendString
 * ============================================================================
 * Mengirim string melalui UART1 untuk debugging.
 * 
 * PARAMETER:
 *   str - Pointer ke string yang akan dikirim (null-terminated)
 * 
 * CATATAN:
 *   Fungsi ini BLOCKING - akan menunggu sampai transmisi selesai.
 *   Untuk aplikasi real-time, pertimbangkan menggunakan DMA atau interrupt.
 * ============================================================================ */
void UART_SendString(const char *str) {
    /* Transmit string via UART dengan timeout maksimum */
    /* HAL_MAX_DELAY = menunggu selamanya sampai transmisi selesai */
    HAL_UART_Transmit(&huart1,                /* Handle UART */
                      (uint8_t*)str,          /* Data yang dikirim */
                      strlen(str),            /* Panjang data */
                      HAL_MAX_DELAY);         /* Timeout */
}

/* ============================================================================
 * TASK 1: vRelativeDelayTask - DEMONSTRASI TIMING RELATIF
 * ============================================================================
 * Task ini menggunakan vTaskDelay() yang menghasilkan timing TIDAK PRESISI.
 * 
 * PERILAKU:
 * - Setiap iterasi: eksekusi kode + delay 100ms
 * - Total periode = waktu eksekusi + 100ms (bervariasi!)
 * - Jika eksekusi memakan 10ms, periode = 110ms
 * - Jika eksekusi memakan 20ms, periode = 120ms
 * 
 * OUTPUT YANG DIHARAPKAN:
 * - Delta (waktu antar iterasi) akan bervariasi
 * - Nilai delta akan > 100 ticks karena ada waktu eksekusi
 * - Menunjukkan "jitter" atau ketidakkonsistenan timing
 * ============================================================================ */
void vRelativeDelayTask(void *pvParameters) {
    /* ---------------------------------------------------------------------------
     * VARIABEL LOKAL
     * --------------------------------------------------------------------------- */
    uint32_t counter = 0;          /* Counter untuk menghitung iterasi */
    char buffer[80];               /* Buffer untuk format string output */
    TickType_t lastTick;           /* Menyimpan tick terakhir untuk hitung delta */
    TickType_t currentTick;        /* Tick saat ini */
    TickType_t delta;              /* Selisih waktu antar iterasi */
    
    /* Cast parameter yang tidak digunakan untuk menghindari warning */
    (void)pvParameters;            /* Parameter tidak digunakan dalam task ini */
    
    /* Kirim pesan startup ke UART */
    UART_SendString("[RELATIVE] Task Started - menggunakan vTaskDelay()\r\n");
    UART_SendString("[RELATIVE] Delta akan BERVARIASI karena timing relatif\r\n\r\n");
    
    /* Inisialisasi tick terakhir */
    lastTick = xTaskGetTickCount();  /* Ambil tick count saat ini */
    
    /* ---------------------------------------------------------------------------
     * INFINITE LOOP - Task berjalan selamanya
     * --------------------------------------------------------------------------- */
    for(;;) {
        /* Increment counter setiap iterasi */
        counter++;
        
        /* -----------------------------------------------------------------------
         * HITUNG DELTA (SELISIH WAKTU)
         * -----------------------------------------------------------------------
         * Delta = currentTick - lastTick
         * Nilai ini menunjukkan berapa tick yang berlalu sejak iterasi terakhir.
         * Dengan timing relatif, delta akan LEBIH BESAR dari periode delay
         * karena waktu eksekusi kode juga dihitung.
         * ----------------------------------------------------------------------- */
        currentTick = xTaskGetTickCount();   /* Ambil tick saat ini */
        delta = currentTick - lastTick;      /* Hitung selisih */
        lastTick = currentTick;              /* Simpan untuk iterasi berikutnya */
        
        /* -----------------------------------------------------------------------
         * SIMULASI WORKLOAD VARIABEL
         * -----------------------------------------------------------------------
         * Untuk mendemonstrasikan jitter, kita buat waktu eksekusi bervariasi.
         * 
         * counter % 5 menghasilkan: 0, 1, 2, 3, 4, 0, 1, 2, ...
         * Dikalikan WORK_VARIATION_FACTOR (5000) menghasilkan:
         * 0, 5000, 10000, 15000, 20000, 0, 5000, ...
         * 
         * Ini mensimulasikan skenario real dimana waktu pemrosesan bervariasi:
         * - Parsing data dengan panjang berbeda
         * - Komputasi dengan kompleksitas berbeda
         * - I/O dengan latency berbeda
         * ----------------------------------------------------------------------- */
        volatile uint32_t work = (counter % 5) * WORK_VARIATION_FACTOR;
        volatile uint32_t i;
        for(i = 0; i < work; i++) {
            /* Loop kosong untuk simulasi kerja CPU */
            /* volatile mencegah compiler mengoptimasi loop ini */
        }
        
        /* -----------------------------------------------------------------------
         * CETAK LAPORAN SETIAP N ITERASI
         * -----------------------------------------------------------------------
         * Untuk mengurangi spam di serial monitor, kita hanya cetak
         * setiap REPORT_INTERVAL iterasi (default: setiap 10 iterasi).
         * 
         * Yang dilaporkan:
         * - Counter: nomor iterasi
         * - Delta: ticks sejak iterasi terakhir (expected: 100, actual: >100)
         * ----------------------------------------------------------------------- */
        if(counter % REPORT_INTERVAL == 0) {
            sprintf(buffer, 
                    "[REL] Count: %lu, Delta: %lu ticks (expected 100, jitter visible)\r\n", 
                    counter, 
                    (unsigned long)delta);
            UART_SendString(buffer);
        }
        
        /* -----------------------------------------------------------------------
         * DELAY RELATIF: vTaskDelay()
         * -----------------------------------------------------------------------
         * CARA KERJA:
         * 1. Task memanggil vTaskDelay(100)
         * 2. Task masuk state BLOCKED selama 100 ticks
         * 3. Setelah 100 ticks DARI TITIK INI, task menjadi READY
         * 
         * MASALAH:
         * Delay dihitung SETELAH semua kode di atas selesai.
         * Jika kode di atas memakan 10ms, total periode = 10ms + 100ms = 110ms
         * 
         * pdMS_TO_TICKS(): Macro untuk konversi milidetik ke ticks
         * Dengan configTICK_RATE_HZ = 1000, 100ms = 100 ticks
         * ----------------------------------------------------------------------- */
        vTaskDelay(pdMS_TO_TICKS(SAMPLING_PERIOD_MS));
    }
}

/* ============================================================================
 * TASK 2: vAbsoluteDelayTask - DEMONSTRASI TIMING ABSOLUT
 * ============================================================================
 * Task ini menggunakan vTaskDelayUntil() untuk timing PRESISI.
 * 
 * PERILAKU:
 * - Periode SELALU tepat 100ms terlepas dari waktu eksekusi
 * - Jika eksekusi memakan 10ms, delay otomatis menjadi 90ms
 * - Jika eksekusi memakan 20ms, delay otomatis menjadi 80ms
 * 
 * OUTPUT YANG DIHARAPKAN:
 * - Delta akan KONSISTEN di 100 ticks
 * - Tidak ada jitter (kecuali ada interrupt dengan prioritas lebih tinggi)
 * - Cocok untuk sampling sensor, kontrol motor, dll
 * 
 * CATATAN PENTING:
 * Jika waktu eksekusi > periode, vTaskDelayUntil akan return segera
 * tanpa blocking. Ini bisa terdeteksi dengan mengecek return value.
 * ============================================================================ */
void vAbsoluteDelayTask(void *pvParameters) {
    /* ---------------------------------------------------------------------------
     * VARIABEL LOKAL
     * --------------------------------------------------------------------------- */
    TickType_t xLastWakeTime;      /* KRUSIAL: Menyimpan waktu wake terakhir */
    uint32_t counter = 0;          /* Counter untuk menghitung iterasi */
    char buffer[100];              /* Buffer untuk format string output */
    TickType_t lastTick;           /* Untuk menghitung delta (verifikasi) */
    TickType_t currentTick;        /* Tick saat ini */
    TickType_t delta;              /* Selisih waktu antar iterasi */
    
    /* Cast parameter yang tidak digunakan untuk menghindari warning */
    (void)pvParameters;            /* Parameter tidak digunakan dalam task ini */
    
    /* Kirim pesan startup ke UART */
    UART_SendString("[ABSOLUTE] Task Started - menggunakan vTaskDelayUntil()\r\n");
    UART_SendString("[ABSOLUTE] Delta akan KONSISTEN karena timing absolut\r\n\r\n");
    
    /* ---------------------------------------------------------------------------
     * INISIALISASI xLastWakeTime - SANGAT PENTING!
     * ---------------------------------------------------------------------------
     * xLastWakeTime HARUS diinisialisasi dengan tick count saat ini
     * SEBELUM loop dimulai. Ini menjadi referensi waktu untuk semua
     * iterasi berikutnya.
     * 
     * vTaskDelayUntil() akan:
     * 1. Menghitung waktu wake berikutnya = xLastWakeTime + periode
     * 2. Block sampai waktu tersebut
     * 3. Update xLastWakeTime untuk iterasi berikutnya
     * --------------------------------------------------------------------------- */
    xLastWakeTime = xTaskGetTickCount();   /* Inisialisasi reference time */
    lastTick = xLastWakeTime;              /* Untuk perhitungan delta */
    
    /* ---------------------------------------------------------------------------
     * INFINITE LOOP - Task berjalan selamanya
     * --------------------------------------------------------------------------- */
    for(;;) {
        /* Increment counter setiap iterasi */
        counter++;
        
        /* -----------------------------------------------------------------------
         * HITUNG DELTA UNTUK VERIFIKASI
         * -----------------------------------------------------------------------
         * Meskipun kita yakin timing presisi, kita tetap hitung delta
         * untuk membuktikan ke user bahwa timing memang konsisten.
         * ----------------------------------------------------------------------- */
        currentTick = xTaskGetTickCount();   /* Ambil tick saat ini */
        delta = currentTick - lastTick;      /* Hitung selisih */
        lastTick = currentTick;              /* Simpan untuk iterasi berikutnya */
        
        /* -----------------------------------------------------------------------
         * SIMULASI WORKLOAD VARIABEL (SAMA SEPERTI RELATIVE TASK)
         * -----------------------------------------------------------------------
         * Kita buat waktu eksekusi yang sama bervariasinya dengan task relative.
         * Tujuannya: menunjukkan bahwa meskipun waktu eksekusi bervariasi,
         * vTaskDelayUntil() tetap menjaga periode konstan.
         * ----------------------------------------------------------------------- */
        volatile uint32_t work = (counter % 5) * WORK_VARIATION_FACTOR;
        volatile uint32_t i;
        for(i = 0; i < work; i++) {
            /* Loop kosong untuk simulasi kerja CPU */
        }
        
        /* -----------------------------------------------------------------------
         * BACA ADC - SIMULASI SAMPLING SENSOR
         * -----------------------------------------------------------------------
         * Dalam aplikasi nyata, timing presisi penting untuk:
         * - Sampling sensor dengan rate konstan (untuk FFT, filter digital)
         * - Akuisisi data untuk analisis spektral
         * - Kontrol loop dengan periode tetap
         * 
         * ADC di PA0 bisa dihubungkan ke potentiometer atau sensor analog.
         * ----------------------------------------------------------------------- */
        HAL_ADC_Start(&hadc1);               /* Mulai konversi ADC */
        
        /* Poll sampai konversi selesai (timeout 10ms) */
        if(HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            /* Baca nilai ADC (0-4095 untuk 12-bit ADC) */
            uint16_t adcValue = HAL_ADC_GetValue(&hadc1);
            
            /* Cetak laporan setiap N iterasi */
            if(counter % REPORT_INTERVAL == 0) {
                sprintf(buffer, 
                        "[ABS] Count: %lu, Delta: %lu ticks (expected 100), ADC: %u\r\n", 
                        counter, 
                        (unsigned long)delta, 
                        adcValue);
                UART_SendString(buffer);
            }
        }
        
        HAL_ADC_Stop(&hadc1);                /* Stop ADC untuk hemat daya */
        
        /* -----------------------------------------------------------------------
         * TOGGLE LED - INDIKATOR VISUAL SAMPLING
         * -----------------------------------------------------------------------
         * LED berkedip setiap iterasi menunjukkan sampling aktif.
         * Frekuensi kedip = 1/(2*SAMPLING_PERIOD_MS) = 5Hz dengan periode 100ms
         * ----------------------------------------------------------------------- */
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        
        /* -----------------------------------------------------------------------
         * DELAY ABSOLUT: vTaskDelayUntil()
         * -----------------------------------------------------------------------
         * CARA KERJA:
         * 1. Hitung waktu wake berikutnya = xLastWakeTime + 100 ticks
         * 2. Jika waktu sekarang < waktu wake berikutnya:
         *    - Block sampai waktu wake berikutnya
         * 3. Jika waktu sekarang >= waktu wake berikutnya:
         *    - Return segera (periode terlewat!)
         * 4. Update xLastWakeTime = waktu wake berikutnya
         * 
         * KEUNGGULAN:
         * - Waktu eksekusi kode di atas TIDAK mempengaruhi periode
         * - Periode tetap konsisten 100ms
         * - Kompensasi otomatis untuk waktu eksekusi
         * 
         * PARAMETER:
         * - &xLastWakeTime: Pointer ke variabel yang menyimpan waktu wake terakhir
         *   (akan di-update oleh fungsi)
         * - pdMS_TO_TICKS(100): Periode yang diinginkan dalam ticks
         * ----------------------------------------------------------------------- */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SAMPLING_PERIOD_MS));
    }
}

/* ============================================================================
 * FUNGSI: SystemClock_Config
 * ============================================================================
 * Mengkonfigurasi system clock STM32F103 ke 72MHz menggunakan HSE + PLL.
 * 
 * KONFIGURASI:
 * - HSE (High Speed External): 8MHz crystal
 * - PLL Multiplier: x9
 * - SYSCLK: 8MHz x 9 = 72MHz
 * - AHB: 72MHz (tidak dibagi)
 * - APB1: 36MHz (dibagi 2, max 36MHz)
 * - APB2: 72MHz (tidak dibagi)
 * - ADC: 12MHz (APB2/6, max 14MHz)
 * 
 * PENTING UNTUK TIMING:
 * Clock yang akurat dan stabil adalah dasar dari timing presisi.
 * Pastikan crystal 8MHz yang digunakan memiliki toleransi rendah.
 * ============================================================================ */
void SystemClock_Config(void) {
    /* Struktur konfigurasi untuk oscillator */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    /* Struktur konfigurasi untuk clock */
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    /* Struktur konfigurasi untuk peripheral clock (ADC) */
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /* -------------------------------------------------------------------------
     * KONFIGURASI HSE DAN PLL
     * ------------------------------------------------------------------------- */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;  /* Gunakan HSE */
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;                    /* Aktifkan HSE */
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;     /* Tidak dibagi */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;                /* Aktifkan PLL */
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;        /* PLL source = HSE */
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;                /* PLL x9 = 72MHz */
    
    /* Apply konfigurasi oscillator */
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* -------------------------------------------------------------------------
     * KONFIGURASI SYSTEM CLOCK
     * ------------------------------------------------------------------------- */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |          /* AHB clock */
                                  RCC_CLOCKTYPE_SYSCLK |        /* System clock */
                                  RCC_CLOCKTYPE_PCLK1 |         /* APB1 clock */
                                  RCC_CLOCKTYPE_PCLK2;          /* APB2 clock */
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;   /* SYSCLK = PLL */
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;          /* AHB = 72MHz */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;           /* APB1 = 36MHz (max) */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;           /* APB2 = 72MHz */
    
    /* Apply konfigurasi clock dengan Flash latency 2 wait states (untuk >48MHz) */
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

    /* -------------------------------------------------------------------------
     * KONFIGURASI PERIPHERAL CLOCK (ADC)
     * ------------------------------------------------------------------------- */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;     /* Konfigurasi ADC clock */
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;        /* ADC = 72/6 = 12MHz */
    
    /* Apply konfigurasi peripheral clock */
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

/* ============================================================================
 * FUNGSI: MX_GPIO_Init
 * ============================================================================
 * Menginisialisasi GPIO untuk LED indicator.
 * 
 * KONFIGURASI LED (PC13):
 * - Mode: Output Push-Pull
 * - Speed: Low (hemat daya, cukup untuk LED)
 * - Pull: None
 * - Initial: HIGH (LED mati, karena aktif LOW)
 * ============================================================================ */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};  /* Struktur konfigurasi GPIO */

    /* Enable clock untuk GPIOC dan GPIOA */
    __HAL_RCC_GPIOC_CLK_ENABLE();            /* LED di PC13 */
    __HAL_RCC_GPIOA_CLK_ENABLE();            /* Button/ADC di PA0 */

    /* -------------------------------------------------------------------------
     * KONFIGURASI LED PC13
     * -------------------------------------------------------------------------
     * LED onboard Blue Pill terhubung ke PC13 dengan konfigurasi aktif LOW.
     * Saat pin LOW, LED menyala. Saat pin HIGH, LED mati.
     * ------------------------------------------------------------------------- */
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_SET);  /* LED mati awal */
    
    GPIO_InitStruct.Pin = LED_GPIO_PIN;              /* Pin PC13 */
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;      /* Output Push-Pull */
    GPIO_InitStruct.Pull = GPIO_NOPULL;              /* Tidak ada pull-up/down */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;     /* Low speed cukup untuk LED */
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);  /* Apply konfigurasi */
}

/* ============================================================================
 * FUNGSI: MX_ADC1_Init
 * ============================================================================
 * Menginisialisasi ADC1 untuk membaca sensor analog di PA0.
 * 
 * KONFIGURASI ADC:
 * - Channel: 0 (PA0)
 * - Resolution: 12-bit (0-4095)
 * - Scan: Disabled (single channel)
 * - Continuous: Disabled (single conversion)
 * - Trigger: Software
 * - Sampling Time: 71.5 cycles (untuk stabilitas)
 * 
 * TIMING ADC:
 * - ADC Clock: 12MHz (dari APB2/6)
 * - Sampling: 71.5 cycles
 * - Conversion: 12.5 cycles
 * - Total: 84 cycles = 7µs per conversion
 * ============================================================================ */
static void MX_ADC1_Init(void) {
    ADC_ChannelConfTypeDef sConfig = {0};    /* Struktur konfigurasi channel */

    /* Enable clock untuk ADC1 */
    __HAL_RCC_ADC1_CLK_ENABLE();

    /* -------------------------------------------------------------------------
     * KONFIGURASI ADC INSTANCE
     * ------------------------------------------------------------------------- */
    hadc1.Instance = SENSOR_ADC_INSTANCE;                       /* ADC1 */
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;                 /* Single channel */
    hadc1.Init.ContinuousConvMode = DISABLE;                    /* Single conversion */
    hadc1.Init.DiscontinuousConvMode = DISABLE;                 /* Tidak pakai discontinuous */
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;           /* Trigger software */
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;                 /* Data rata kanan */
    hadc1.Init.NbrOfConversion = 1;                             /* 1 konversi */
    HAL_ADC_Init(&hadc1);                                       /* Apply konfigurasi */

    /* -------------------------------------------------------------------------
     * KONFIGURASI CHANNEL ADC
     * ------------------------------------------------------------------------- */
    sConfig.Channel = SENSOR_ADC_CHANNEL;                       /* Channel 0 (PA0) */
    sConfig.Rank = ADC_REGULAR_RANK_1;                          /* Rank pertama */
    sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;           /* 71.5 cycles sampling */
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);                    /* Apply konfigurasi */

    /* -------------------------------------------------------------------------
     * KALIBRASI ADC
     * -------------------------------------------------------------------------
     * Kalibrasi penting untuk akurasi pembacaan ADC.
     * Harus dilakukan setelah ADC diinisialisasi dan sebelum digunakan.
     * ------------------------------------------------------------------------- */
    HAL_ADCEx_Calibration_Start(&hadc1);
}

/* ============================================================================
 * FUNGSI: MX_USART1_UART_Init
 * ============================================================================
 * Menginisialisasi USART1 untuk komunikasi serial debugging.
 * 
 * KONFIGURASI UART:
 * - Baudrate: 115200 bps
 * - Word Length: 8 bits
 * - Stop Bits: 1
 * - Parity: None
 * - Mode: TX dan RX
 * - Flow Control: None
 * 
 * PIN:
 * - PA9: TX (Alternate Function Push-Pull)
 * - PA10: RX (Input Floating)
 * ============================================================================ */
static void MX_USART1_UART_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};  /* Struktur konfigurasi GPIO */

    /* Enable clock untuk USART1 dan GPIOA */
    __HAL_RCC_USART1_CLK_ENABLE();           /* Clock untuk USART1 */
    __HAL_RCC_GPIOA_CLK_ENABLE();            /* Clock untuk GPIOA (PA9, PA10) */

    /* -------------------------------------------------------------------------
     * KONFIGURASI GPIO UNTUK UART
     * ------------------------------------------------------------------------- */
    /* PA9 = TX (Alternate Function Push-Pull) */
    GPIO_InitStruct.Pin = DEBUG_UART_TX_PIN;         /* Pin PA9 */
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;          /* Alternate Function Push-Pull */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;    /* High speed untuk komunikasi */
    HAL_GPIO_Init(DEBUG_UART_TX_PORT, &GPIO_InitStruct);
    
    /* PA10 = RX (Input) */
    GPIO_InitStruct.Pin = DEBUG_UART_RX_PIN;         /* Pin PA10 */
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;          /* Input mode */
    GPIO_InitStruct.Pull = GPIO_NOPULL;              /* Tidak ada pull-up/down */
    HAL_GPIO_Init(DEBUG_UART_RX_PORT, &GPIO_InitStruct);

    /* -------------------------------------------------------------------------
     * KONFIGURASI USART1
     * ------------------------------------------------------------------------- */
    huart1.Instance = DEBUG_UART_INSTANCE;           /* USART1 */
    huart1.Init.BaudRate = DEBUG_UART_BAUDRATE;      /* 115200 bps */
    huart1.Init.WordLength = UART_WORDLENGTH_8B;     /* 8-bit data */
    huart1.Init.StopBits = UART_STOPBITS_1;          /* 1 stop bit */
    huart1.Init.Parity = UART_PARITY_NONE;           /* Tidak ada parity */
    huart1.Init.Mode = UART_MODE_TX_RX;              /* TX dan RX enabled */
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;     /* Tidak ada hardware flow control */
    huart1.Init.OverSampling = UART_OVERSAMPLING_16; /* 16x oversampling */
    HAL_UART_Init(&huart1);                          /* Apply konfigurasi */
}

/* ============================================================================
 * FUNGSI: vApplicationMallocFailedHook
 * ============================================================================
 * Callback yang dipanggil FreeRTOS ketika alokasi memori gagal.
 * 
 * KAPAN DIPANGGIL:
 * - pvPortMalloc() return NULL
 * - xTaskCreate() gagal mengalokasi stack
 * - xQueueCreate() gagal mengalokasi memori queue
 * 
 * PENYEBAB UMUM:
 * - configTOTAL_HEAP_SIZE terlalu kecil
 * - Terlalu banyak task/queue dibuat
 * - Memory fragmentation (pada heap_2.c)
 * 
 * TINDAKAN:
 * - Disable interrupt untuk mencegah kerusakan lebih lanjut
 * - Loop selamanya (agar bisa dideteksi debugger)
 * ============================================================================ */
void vApplicationMallocFailedHook(void) {
    /* Kirim pesan error jika UART masih berfungsi */
    UART_SendString("\r\n!!! FATAL: Malloc gagal! Heap penuh. !!!\r\n");
    
    /* Disable interrupt dan loop selamanya */
    taskDISABLE_INTERRUPTS();
    for(;;) {
        /* Blink LED cepat untuk indikasi error */
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        for(volatile uint32_t i = 0; i < 100000; i++);
    }
}

/* ============================================================================
 * FUNGSI: vApplicationStackOverflowHook
 * ============================================================================
 * Callback yang dipanggil FreeRTOS ketika stack overflow terdeteksi.
 * 
 * PARAMETER:
 *   xTask - Handle dari task yang mengalami overflow
 *   pcTaskName - Nama task yang overflow
 * 
 * METODE DETEKSI (configCHECK_FOR_STACK_OVERFLOW = 2):
 * 1. Cek apakah stack pointer keluar dari area stack
 * 2. Cek apakah "watermark" di ujung stack tertimpa
 * 
 * PENYEBAB UMUM:
 * - Stack size terlalu kecil untuk task
 * - Variabel lokal terlalu besar (buffer besar di stack)
 * - Rekursi terlalu dalam
 * 
 * PENCEGAHAN:
 * - Gunakan uxTaskGetStackHighWaterMark() untuk monitor
 * - Tambah stack size jika watermark hampir habis
 * ============================================================================ */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    /* Cast parameter untuk menghindari warning */
    (void)xTask;
    
    /* Kirim pesan error dengan nama task */
    UART_SendString("\r\n!!! FATAL: Stack Overflow pada task: ");
    UART_SendString(pcTaskName);
    UART_SendString(" !!!\r\n");
    
    /* Disable interrupt dan loop selamanya */
    taskDISABLE_INTERRUPTS();
    for(;;) {
        /* Blink LED cepat untuk indikasi error */
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        for(volatile uint32_t i = 0; i < 50000; i++);
    }
}

/* ============================================================================
 * FUNGSI: SysTick_Handler
 * ============================================================================
 * Interrupt handler untuk SysTick timer.
 * 
 * FUNGSI GANDA:
 * 1. HAL_IncTick(): Increment counter untuk HAL_Delay() dan timeout
 * 2. xPortSysTickHandler(): FreeRTOS tick handler untuk scheduling
 * 
 * FREKUENSI: 1000 Hz (sesuai configTICK_RATE_HZ)
 * 
 * CATATAN:
 * xPortSysTickHandler hanya boleh dipanggil jika scheduler sudah berjalan.
 * xTaskGetSchedulerState() mengecek hal ini.
 * ============================================================================ */
void SysTick_Handler(void) {
    HAL_IncTick();                                     /* Untuk HAL timing */
    
    /* Panggil FreeRTOS tick handler hanya jika scheduler berjalan */
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();                         /* FreeRTOS tick processing */
    }
}

/* ============================================================================
 * FUNGSI: main
 * ============================================================================
 * Entry point program. Menginisialisasi hardware dan memulai FreeRTOS.
 * 
 * ALUR PROGRAM:
 * 1. Inisialisasi HAL
 * 2. Konfigurasi system clock 72MHz
 * 3. Inisialisasi peripheral (GPIO, ADC, UART)
 * 4. Cetak pesan startup
 * 5. Buat task-task FreeRTOS
 * 6. Start scheduler
 * 7. (Tidak akan sampai sini jika scheduler berjalan)
 * ============================================================================ */
int main(void) {
    /* -------------------------------------------------------------------------
     * INISIALISASI HAL
     * -------------------------------------------------------------------------
     * HAL_Init() melakukan:
     * - Konfigurasi Flash prefetch
     * - Setup SysTick untuk HAL_Delay (akan dikonfigurasi ulang oleh FreeRTOS)
     * - Inisialisasi low-level hardware
     * ------------------------------------------------------------------------- */
    HAL_Init();
    
    /* Konfigurasi clock system ke 72MHz */
    SystemClock_Config();
    
    /* Inisialisasi peripheral */
    MX_GPIO_Init();                /* LED di PC13 */
    MX_ADC1_Init();                /* ADC untuk sensor */
    MX_USART1_UART_Init();         /* UART untuk debug */

    /* -------------------------------------------------------------------------
     * PESAN STARTUP
     * -------------------------------------------------------------------------
     * Informasi program untuk user yang melihat serial monitor.
     * ------------------------------------------------------------------------- */
    UART_SendString("\r\n");
    UART_SendString("============================================================\r\n");
    UART_SendString("  Program 3: Absolute Timing Control\r\n");
    UART_SendString("============================================================\r\n");
    UART_SendString("\r\n");
    UART_SendString("DESKRIPSI:\r\n");
    UART_SendString("Program ini membandingkan dua metode delay FreeRTOS:\r\n");
    UART_SendString("1. vTaskDelay() - Timing RELATIF (ada jitter)\r\n");
    UART_SendString("2. vTaskDelayUntil() - Timing ABSOLUT (presisi)\r\n");
    UART_SendString("\r\n");
    UART_SendString("CARA MENGAMATI:\r\n");
    UART_SendString("- [REL] Delta akan bervariasi (>100 ticks)\r\n");
    UART_SendString("- [ABS] Delta akan konsisten (=100 ticks)\r\n");
    UART_SendString("\r\n");
    UART_SendString("Periode target: ");
    char buf[20];
    sprintf(buf, "%d ms\r\n\r\n", SAMPLING_PERIOD_MS);
    UART_SendString(buf);
    UART_SendString("============================================================\r\n\r\n");

    /* -------------------------------------------------------------------------
     * MEMBUAT TASK FREERTOS
     * -------------------------------------------------------------------------
     * 
     * Task 1: vRelativeDelayTask
     * - Prioritas: 1 (REL_TASK_PRIORITY)
     * - Stack: 256 words (1024 bytes)
     * - Menggunakan vTaskDelay() - timing tidak presisi
     * 
     * Task 2: vAbsoluteDelayTask  
     * - Prioritas: 2 (ABS_TASK_PRIORITY) - lebih tinggi
     * - Stack: 256 words (1024 bytes)
     * - Menggunakan vTaskDelayUntil() - timing presisi
     * 
     * Karena AbsTask prioritas lebih tinggi, dia akan berjalan duluan
     * saat kedua task ready. Tapi karena keduanya delay, mereka bergantian.
     * ------------------------------------------------------------------------- */
    
    /* Buat task dengan relative delay (menunjukkan jitter) */
    BaseType_t result1 = xTaskCreate(
        vRelativeDelayTask,         /* Pointer ke fungsi task */
        REL_TASK_NAME,              /* Nama task untuk debugging */
        REL_TASK_STACK_SIZE,        /* Stack size dalam words */
        NULL,                       /* Parameter (tidak digunakan) */
        REL_TASK_PRIORITY,          /* Prioritas task (1) */
        NULL                        /* Handle task (tidak disimpan) */
    );
    
    /* Buat task dengan absolute delay (timing presisi) */
    BaseType_t result2 = xTaskCreate(
        vAbsoluteDelayTask,         /* Pointer ke fungsi task */
        ABS_TASK_NAME,              /* Nama task untuk debugging */
        ABS_TASK_STACK_SIZE,        /* Stack size dalam words */
        NULL,                       /* Parameter (tidak digunakan) */
        ABS_TASK_PRIORITY,          /* Prioritas task (2) - lebih tinggi */
        NULL                        /* Handle task (tidak disimpan) */
    );
    
    /* Cek apakah task berhasil dibuat */
    if(result1 != pdPASS || result2 != pdPASS) {
        UART_SendString("ERROR: Gagal membuat task! Heap tidak cukup?\r\n");
        /* Indikasi error dengan LED */
        while(1) {
            HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
            HAL_Delay(100);
        }
    }
    
    UART_SendString("Task berhasil dibuat. Memulai scheduler...\r\n\r\n");

    /* -------------------------------------------------------------------------
     * START FREERTOS SCHEDULER
     * -------------------------------------------------------------------------
     * Setelah vTaskStartScheduler() dipanggil:
     * - Scheduler mulai menjalankan task berdasarkan prioritas
     * - Fungsi ini TIDAK AKAN RETURN jika berhasil
     * - Control diserahkan sepenuhnya ke scheduler
     * 
     * Jika scheduler gagal start (biasanya karena heap kurang untuk idle task):
     * - Fungsi akan return
     * - Kode di bawah while(1) akan dieksekusi
     * ------------------------------------------------------------------------- */
    vTaskStartScheduler();

    /* -------------------------------------------------------------------------
     * INFINITE LOOP (SEHARUSNYA TIDAK TERCAPAI)
     * -------------------------------------------------------------------------
     * Jika sampai sini, berarti scheduler gagal start.
     * Penyebab: heap tidak cukup untuk membuat idle task.
     * ------------------------------------------------------------------------- */
    UART_SendString("ERROR: Scheduler gagal start!\r\n");
    while(1) {
        /* Blink LED lambat untuk indikasi scheduler gagal */
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        HAL_Delay(500);
    }
}
