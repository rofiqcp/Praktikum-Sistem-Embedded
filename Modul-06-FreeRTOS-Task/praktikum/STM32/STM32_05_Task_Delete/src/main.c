/**
 * ================================================================================
 * PROGRAM 1: DYNAMIC TASK INJECTION - STM32F103C8T6 (Blue Pill)
 * ================================================================================
 * Deskripsi:
 *   Membuat dan menghapus task secara dinamis pada runtime menggunakan xTaskCreate
 *   dan vTaskDelete berdasarkan input tombol. Program memantau penggunaan heap RAM
 *   dan menampilkan informasi melalui UART (serial monitor).
 *
 * Hardware:
 *   - STM32F103C8T6 (Blue Pill)
 *   - LED built-in di PC13
 *   - Button built-in di PA0 (active low)
 *   - UART1 untuk debug output (PA9=TX, PA10=RX, 115200 baud)
 *
 * Framework:
 *   - STM32Cube HAL
 *   - FreeRTOS
 *
 * Cara Kerja:
 *   1. Program dimulai dengan main task yang berjalan terus-menerus
 *   2. Main task membaca status tombol di PA0
 *   3. Jika tombol ditekan (falling edge), program membuat task dinamis
 *   4. Task dinamis mengedip LED dan menampilkan counter
 *   5. Jika tombol ditekan lagi, task dinamis dihapus
 *   6. Program menampilkan heap usage setiap kali ada perubahan
 *
 * Output Serial (115200 baud):
 *   - Status pembuatan dan penghapusan task
 *   - Status heap memory
 *   - Counter dari task dinamis
 *
 * ================================================================================
 */

/* ================================================================================
 * INCLUDE LIBRARIES DAN HEADER FILES
 * ================================================================================ */

/* STM32 Hardware Abstraction Layer untuk akses hardware */
#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif

/* FreeRTOS kernel - untuk multi-tasking dan scheduling */
#include "FreeRTOS.h"

/* Modul task FreeRTOS */
#include "task.h"

/* String library untuk strlen, sprintf, dll */
#include <string.h>

/* Standard I/O library untuk formatted output */
#include <stdio.h>

/* Deklarasi xPortSysTickHandler untuk fix warning */
extern void xPortSysTickHandler(void);

/* FreeRTOS configuration - sudah include lewat build path */
#include "FreeRTOSConfig.h"

/* ================================================================================
 * VARIABEL GLOBAL DAN STRUKTUR DATA
 * ================================================================================ */

/* Handle untuk UART1 yang digunakan untuk komunikasi debug */
UART_HandleTypeDef huart1;

/* Handle task untuk menyimpan referensi task dinamis yang sedang berjalan */
TaskHandle_t xDynamicTaskHandle = NULL;

/* Flag volatile untuk status tombol yang ditekan */
volatile uint8_t buttonPressed = 0;

/* ================================================================================
 * FUNCTION PROTOTYPES (Deklarasi fungsi)
 * ================================================================================ */

/* Konfigurasi clock system STM32F103 ke 72MHz */
void SystemClock_Config(void);

/* Inisialisasi GPIO untuk LED dan tombol */
static void MX_GPIO_Init(void);

/* Inisialisasi UART1 untuk debug output */
static void MX_USART1_UART_Init(void);

/* Task utama yang memantau tombol dan membuat/menghapus task dinamis */
void vMainTask(void *pvParameters);

/* Task dinamis yang dibuat/dihapus pada runtime - mengedip LED */
void vDynamicTask(void *pvParameters);

/* Mengirim string melalui UART ke serial monitor */
void UART_SendString(const char *str);

/* Menampilkan status heap memory (free memory dan minimum) */
void PrintHeapStatus(void);


/* ================================================================================
 * IMPLEMENTASI FUNGSI UTILITY (Fungsi Pembantu)
 * ================================================================================ */

/**
 * Mengirim string melalui UART ke serial monitor
 *
 * Parameter:
 *   str: pointer ke string yang akan dikirim (null-terminated)
 */
void UART_SendString(const char *str)
{
    /* Kirim string melalui UART dengan panjang string */
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

/**
 * Menampilkan status heap memory ke serial monitor
 *
 * Menampilkan:
 *   - Free Heap: jumlah memory yang masih tersedia untuk dynamic allocation
 *   - Min Ever: minimum free heap yang pernah ada sejak boot
 */
void PrintHeapStatus(void)
{
    /* Buffer untuk menyimpan string output */
    char buffer[100];

    /* Dapatkan jumlah free heap saat ini (bytes) */
    size_t freeHeap = xPortGetFreeHeapSize();

    /* Dapatkan minimum free heap sejak boot (untuk deteksi memory leak) */
    size_t minEverFreeHeap = xPortGetMinimumEverFreeHeapSize();

    /* Format dan kirim string ke serial monitor */
    sprintf(buffer, "Free Heap: %u bytes, Min Ever: %u bytes\r\n", 
            (unsigned int)freeHeap, (unsigned int)minEverFreeHeap);
    UART_SendString(buffer);
}


/* ================================================================================
 * IMPLEMENTASI TASK UTAMA
 * ================================================================================ */

/**
 * Task Utama - Memantau tombol dan membuat/menghapus task dinamis
 *
 * Fungsi:
 *   1. Membaca status tombol PA0
 *   2. Deteksi falling edge (tombol dari 1 ke 0)
 *   3. Debounce dengan delay 50ms
 *   4. Jika tidak ada task dinamis, buat task baru
 *   5. Jika ada task dinamis, hapus task tersebut
 *   6. Tampilkan status heap setiap perubahan
 *   7. Loop setiap 10ms
 *
 * Parameter:
 *   pvParameters: parameter yang diteruskan saat membuat task (tidak digunakan)
 */
void vMainTask(void *pvParameters)
{
    /* Simpan state tombol sebelumnya untuk deteksi edge (1=released, 0=pressed) */
    uint8_t lastButtonState = 1;

    /* Variabel untuk state tombol saat ini */
    uint8_t currentButtonState;

    /* Kirim pesan startup ke serial monitor */
    UART_SendString("Main Task Started\r\n");

    /* Tampilkan status heap saat startup */
    PrintHeapStatus();

    /* Loop task utama - berjalan terus menerus sampai scheduler dihentikan */
    for(;;)
    {
        /* Baca pin PA0 (tombol) - 1=released (high), 0=pressed (low) */
        currentButtonState = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);

        /* Deteksi falling edge: tombol berubah dari released (1) ke pressed (0) */
        if(lastButtonState == 1 && currentButtonState == 0)
        {
            /* Debounce delay - tunggu 50ms untuk memastikan bukan noise */
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY_MS));

            /* Cek lagi untuk memastikan tombol masih pressed */
            currentButtonState = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);

            if(currentButtonState == 0)
            {
                /* Tombol confirmed pressed - lakukan aksi */

                if(xDynamicTaskHandle == NULL)
                {
                    /* AKSI 1: Task dinamis belum ada - buat task baru */

                    /* Kirim pesan ke serial monitor */
                    UART_SendString("\r\n--- Creating Dynamic Task ---\r\n");

                    /* Buat task baru dengan xTaskCreate */
                    BaseType_t result = xTaskCreate(
                        vDynamicTask,               /* Pointer ke fungsi task */
                        "DynTask",                  /* Nama task (untuk debugging) */
                        DYNAMIC_TASK_STACK_SIZE,    /* Stack size dalam bytes */
                        NULL,                       /* Parameter (tidak digunakan) */
                        DYNAMIC_TASK_PRIORITY,      /* Prioritas task */
                        &xDynamicTaskHandle         /* Output: handle task */
                    );

                    /* Cek apakah pembuatan task berhasil */
                    if(result == pdPASS)
                    {
                        /* Pembuatan berhasil */
                        UART_SendString("Dynamic Task Created Successfully!\r\n");
                    }
                    else
                    {
                        /* Pembuatan gagal - kemungkinan karena heap tidak cukup */
                        UART_SendString("Failed to Create Task!\r\n");
                    }

                    /* Tampilkan status heap setelah membuat task */
                    PrintHeapStatus();
                }
                else
                {
                    /* AKSI 2: Task dinamis sudah ada - hapus task tersebut */

                    /* Kirim pesan ke serial monitor */
                    UART_SendString("\r\n--- Deleting Dynamic Task ---\r\n");

                    /* Hapus task menggunakan handle yang tersimpan */
                    vTaskDelete(xDynamicTaskHandle);

                    /* Set handle ke NULL karena task sudah dihapus */
                    xDynamicTaskHandle = NULL;

                    /* Kirim pesan konfirmasi penghapusan */
                    UART_SendString("Dynamic Task Deleted!\r\n");

                    /* Tampilkan status heap setelah menghapus task */
                    PrintHeapStatus();
                }
            }
        }

        /* Update state tombol sebelumnya untuk iterasi berikutnya */
        lastButtonState = currentButtonState;

        /* Delay task utama 10ms sebelum membaca tombol lagi */
        /* pdMS_TO_TICKS() mengkonversi millisecond ke tick count FreeRTOS */
        vTaskDelay(pdMS_TO_TICKS(MAIN_TASK_DELAY_MS));
    }
}


/* ================================================================================
 * IMPLEMENTASI TASK DINAMIS
 * ================================================================================ */

/**
 * Task Dinamis - Mengedip LED dan menampilkan counter (dibuat/dihapus runtime)
 *
 * Fungsi:
 *   1. Menampilkan pesan startup
 *   2. Loop dengan toggle LED pada PC13 setiap 200ms
 *   3. Menampilkan counter setiap 10 iterasi (2 detik)
 *   4. Terus berjalan sampai vTaskDelete() dipanggil
 *
 * Parameter:
 *   pvParameters: parameter yang diteruskan saat membuat task (tidak digunakan)
 */
void vDynamicTask(void *pvParameters)
{
    /* Counter untuk menghitung iterasi task */
    uint32_t counter = 0;

    /* Buffer untuk format output */
    char buffer[50];

    /* Kirim pesan bahwa task dinamis sudah berjalan */
    UART_SendString("Dynamic Task Running...\r\n");

    /* Loop task - berjalan terus sampai vTaskDelete() dipanggil */
    for(;;)
    {
        /* Toggle LED di PC13 (kanan antara ON dan OFF) */
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);

        /* Increment counter setiap iterasi */
        counter++;

        /* Setiap 10 iterasi, tampilkan counter ke serial monitor */
        if(counter % DYNAMIC_TASK_PRINT_INTERVAL == 0)
        {
            /* Format string dengan counter value */
            sprintf(buffer, "Dynamic Task Counter: %lu\r\n", counter);

            /* Kirim ke serial monitor */
            UART_SendString(buffer);
        }

        /* Delay 200ms sebelum toggle LED berikutnya */
        /* pdMS_TO_TICKS() mengkonversi millisecond ke tick count FreeRTOS */
        vTaskDelay(pdMS_TO_TICKS(DYNAMIC_TASK_LED_DELAY_MS));
    }
}


/* ================================================================================
 * KONFIGURASI HARDWARE - SYSTEM CLOCK
 * ================================================================================ */

/**
 * Mengkonfigurasi clock system ke 72MHz
 *
 * Spesifikasi:
 *   - HSE (High Speed External): 8MHz
 *   - PLL multiplier: x9
 *   - Hasil: 8MHz x 9 = 72MHz
 *   - FLASH latency: 2 wait state
 *   - APB1 divider: /2 (untuk menjaga APB1 di bawah 36MHz)
 *   - APB2 divider: /1
 */
void SystemClock_Config(void)
{
    /* Struktur untuk konfigurasi oscillator (clock sumber) */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};

    /* Struktur untuk konfigurasi clock distribution */
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* ---- KONFIGURASI OSCILLATOR ---- */

    /* Pilih HSE sebagai sumber clock (External crystal 8MHz) */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;

    /* Aktifkan HSE */
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;

    /* HSE predivider untuk PLL = /1 (gunakan 8MHz langsung) */
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;

    /* Aktifkan PLL (Phase Locked Loop) */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;

    /* Sumber PLL adalah HSE */
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

    /* PLL multiplier = x9 (8MHz x 9 = 72MHz) */
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

    /* Apply oscillator configuration */
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* ---- KONFIGURASI CLOCK DISTRIBUTION ---- */

    /* Pilih clock yang akan dikonfigurasi: AHB, APB1, APB2, SYSCLK */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;

    /* Sumber SYSCLK adalah PLL output (72MHz) */
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;

    /* AHB divider = /1 (AHB = 72MHz) */
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;

    /* APB1 divider = /2 (APB1 = 36MHz, maksimal limit untuk APB1) */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;

    /* APB2 divider = /1 (APB2 = 72MHz) */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    /* Apply clock configuration dengan 2 wait state FLASH */
    /* FLASH latency = 2 karena SYSCLK 72MHz > 48MHz */
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}


/* ================================================================================
 * KONFIGURASI HARDWARE - GPIO
 * ================================================================================ */

/**
 * Inisialisasi GPIO untuk LED (PC13) dan tombol (PA0)
 *
 * Konfigurasi:
 *   - PC13: Output push-pull untuk LED (built-in di Blue Pill)
 *   - PA0: Input dengan pull-up untuk tombol (active low)
 */
static void MX_GPIO_Init(void)
{
    /* Struktur untuk konfigurasi GPIO */
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable clock untuk port GPIOC (untuk LED) */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Enable clock untuk port GPIOA (untuk tombol) */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* ---- KONFIGURASI LED DI PC13 ---- */

    /* Set LED awal dalam state HIGH (LED OFF pada Blue Pill karena active low) */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

    /* Konfigurasi pin untuk LED */
    GPIO_InitStruct.Pin = LED_PIN;

    /* Mode: output push-pull (dapat drive high dan low) */
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;

    /* Pull-up: tidak digunakan untuk output */
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    /* Kecepatan: LOW (cukup untuk LED yang tidak perlu kecepatan tinggi) */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    /* Apply konfigurasi ke GPIOC */
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* ---- KONFIGURASI TOMBOL DI PA0 ---- */

    /* Konfigurasi pin untuk tombol */
    GPIO_InitStruct.Pin = BUTTON_PIN;

    /* Mode: input biasa (tidak interrupt, polling) */
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;

    /* Pull-up: aktifkan untuk memastikan high saat tombol tidak ditekan */
    GPIO_InitStruct.Pull = GPIO_PULLUP;

    /* Apply konfigurasi ke GPIOA */
    HAL_GPIO_Init(BUTTON_PORT, &GPIO_InitStruct);
}


/* ================================================================================
 * KONFIGURASI HARDWARE - UART
 * ================================================================================ */

/**
 * Inisialisasi UART1 untuk debug output ke serial monitor
 *
 * Konfigurasi:
 *   - PA9 (TX): Alternate function push-pull untuk transmit
 *   - PA10 (RX): Input untuk receive (tidak digunakan dalam program ini)
 *   - Baud rate: 115200 (dapat diubah di config_rtos.h)
 *   - Data bits: 8
 *   - Stop bits: 1
 *   - Parity: None
 */
static void MX_USART1_UART_Init(void)
{
    /* Enable clock untuk USART1 peripheral */
    __HAL_RCC_USART1_CLK_ENABLE();

    /* Enable clock untuk GPIOA (untuk pin UART) */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Struktur untuk konfigurasi GPIO */
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* ---- KONFIGURASI TX PIN (PA9) ---- */

    /* Pin: PA9 */
    GPIO_InitStruct.Pin = GPIO_PIN_9;

    /* Mode: Alternate function push-pull (UART1_TX) */
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;

    /* Kecepatan: HIGH (untuk UART 115200 baud) */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    /* Apply konfigurasi */
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ---- KONFIGURASI RX PIN (PA10) ---- */

    /* Pin: PA10 */
    GPIO_InitStruct.Pin = GPIO_PIN_10;

    /* Mode: Input biasa (UART1_RX) */
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;

    /* Pull: Tidak ada pull-up/down (default floating) */
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    /* Apply konfigurasi */
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ---- KONFIGURASI UART1 PARAMETER ---- */

    /* Set instance ke USART1 */
    huart1.Instance = USART1;

    /* Baud rate: 115200 (dari config_rtos.h) */
    huart1.Init.BaudRate = UART_BAUDRATE;

    /* Data bits: 8 bit */
    huart1.Init.WordLength = UART_WORDLENGTH_8B;

    /* Stop bits: 1 stop bit */
    huart1.Init.StopBits = UART_STOPBITS_1;

    /* Parity: None (tanpa parity check) */
    huart1.Init.Parity = UART_PARITY_NONE;

    /* Mode: TX dan RX keduanya aktif */
    huart1.Init.Mode = UART_MODE_TX_RX;

    /* Hardware flow control: Tidak ada (tidak pakai CTS/RTS) */
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;

    /* Oversampling: 16x (lebih akurat, standar) */
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    /* Initialize UART dengan parameter yang sudah dikonfigurasi */
    HAL_UART_Init(&huart1);
}


/* ================================================================================
 * INTERRUPT SERVICE ROUTINES (ISR) - EXCEPTION HANDLERS
 * ================================================================================ */

/**
 * Non-Maskable Interrupt Handler
 *
 * NMI terjadi untuk kondisi kritis yang tidak bisa dihindari
 * Dalam program ini: loop infinite (halt processor)
 */
void NMI_Handler(void)
{
    /* Loop infinite - jika ada NMI, program berhenti di sini */
    while(1)
    {
    }
}

/**
 * Hard Fault Interrupt Handler
 *
 * Hard fault adalah exception paling serius di ARM Cortex-M
 * Penyebab: stack overflow, invalid memory access, undefined instruction, dll
 * Dalam program ini: loop infinite (halt processor) untuk debugging
 */
void HardFault_Handler(void)
{
    /* Loop infinite - jika ada hard fault, program berhenti di sini */
    while(1)
    {
    }
}

/**
 * Memory Management Fault Handler
 *
 * Terjadi jika ada akses memory protection unit (MPU) yang ilegal
 * Jarang terjadi di STM32F103 kecuali MPU diaktifkan
 */
void MemManage_Handler(void)
{
    /* Loop infinite - halt processor */
    while(1)
    {
    }
}

/**
 * Bus Fault Handler
 *
 * Terjadi jika ada error pada bus system (contoh: prefetch abort)
 * Biasanya karena akses memory dengan timing buruk atau data abort
 */
void BusFault_Handler(void)
{
    /* Loop infinite - halt processor */
    while(1)
    {
    }
}

/**
 * Usage Fault Handler
 *
 * Terjadi jika ada instruksi undefined atau illegal state
 * Contoh: instruksi yang tidak didukung atau division by zero
 */
void UsageFault_Handler(void)
{
    /* Loop infinite - halt processor */
    while(1)
    {
    }
}

/**
 * Debug Monitor Handler
 *
 * Dipanggil saat debug exception terjadi
 * Biasanya digunakan untuk debugging saja
 */
void DebugMon_Handler(void)
{
    /* Kosong - tidak ada aksi khusus untuk debug monitor */
}

/**
 * SysTick Handler (dipanggil oleh FreeRTOS kernel)
 *
 * Handler ini seharusnya dipanggil oleh FreeRTOS secara otomatis
 * Weak link ke xPortSysTickHandler dari port FreeRTOS
 */
void SysTick_Handler(void)
{
    /* Call ke FreeRTOS SysTick handler */
    xPortSysTickHandler();
}

/* ================================================================================
 * FREERTOS HOOK FUNCTIONS
 * ================================================================================ */

/**
 * malloc Failed Hook - Dipanggil saat xPortMalloc gagal alokasi memory
 *
 * Alasan gagal: heap sudah penuh
 * Solusi: perbesar configTOTAL_HEAP_SIZE atau kurangi alokasi task
 */
void vApplicationMallocFailedHook(void)
{
    /* Assertion failure - halt processor untuk debugging */
    configASSERT(0);
}

/**
 * Stack Overflow Hook - Dipanggil saat task stack overflow terdeteksi
 *
 * Stack overflow: task menggunakan memory lebih dari stack yang dialokasikan
 * Solusi: perbesar stack size saat membuat task atau kurangi variabel lokal
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    /* Suppress unused parameter warning */
    (void)xTask;
    (void)pcTaskName;

    /* Assertion failure - halt processor untuk debugging */
    configASSERT(0);
}

/* ================================================================================
 * FREERTOS STATIC ALLOCATION FUNCTIONS
 * ================================================================================ */

/**
 * Get Idle Task Memory - Menyediakan memory untuk idle task secara static
 *
 * FreeRTOS membutuhkan memory untuk idle task:
 * - TCB (Task Control Block): task descriptor
 * - Stack: memory untuk task execution
 *
 * Digunakan saat configSUPPORT_STATIC_ALLOCATION = 1
 */
#if configSUPPORT_STATIC_ALLOCATION == 1

/* TCB (Task Control Block) static untuk idle task */
static StaticTask_t xIdleTaskTCB;

/* Stack static untuk idle task */
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   configSTACK_DEPTH_TYPE *pulIdleTaskStackSize)
{
    /* Provide TCB buffer */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Provide stack buffer */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Provide stack size */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* ---- Timer Task Memory (jika software timer diaktifkan) ---- */

#if configUSE_TIMERS == 1

/* TCB static untuk timer task */
static StaticTask_t xTimerTaskTCB;

/* Stack static untuk timer task */
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE *pulTimerTaskStackSize)
{
    /* Provide TCB buffer */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Provide stack buffer */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Provide stack size */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

#endif /* configUSE_TIMERS == 1 */

#endif /* configSUPPORT_STATIC_ALLOCATION */

/* ================================================================================
 * MAIN FUNCTION - ENTRY POINT PROGRAM
 * ================================================================================ */

/**
 * Main Function - Titik masuk program
 *
 * Urutan:
 *   1. Inisialisasi HAL
 *   2. Konfigurasi clock system ke 72MHz
 *   3. Inisialisasi GPIO untuk LED dan tombol
 *   4. Inisialisasi UART untuk debug output
 *   5. Kirim pesan welcome
 *   6. Buat main task
 *   7. Mulai FreeRTOS scheduler
 *   8. Program tidak akan keluar dari loop infinite vTaskStartScheduler()
 */
int main(void)
{
    /* ---- INISIALISASI HARDWARE ---- */

    /* Initialize Hardware Abstraction Layer (HAL) */
    HAL_Init();

    /* Konfigurasi system clock ke 72MHz */
    SystemClock_Config();

    /* Inisialisasi GPIO (LED, tombol) */
    MX_GPIO_Init();

    /* Inisialisasi UART1 untuk debug output */
    MX_USART1_UART_Init();

    /* ---- WELCOME MESSAGE ---- */

    /* Kirim welcome message ke serial monitor */
    UART_SendString("\r\n=== Program 1: Dynamic Task Injection ===\r\n");
    UART_SendString("Press button PA0 to create/delete dynamic task\r\n");
    UART_SendString("Monitor heap usage in real-time\r\n");
    UART_SendString("\r\n");

    /* ---- MEMBUAT FREERTOS TASKS ---- */

    /* Buat main task yang akan memantau tombol */
    xTaskCreate(
        vMainTask,              /* Pointer ke fungsi task */
        "MainTask",             /* Nama task untuk debugging */
        MAIN_TASK_STACK_SIZE,   /* Stack size */
        NULL,                   /* Parameter task */
        MAIN_TASK_PRIORITY,     /* Prioritas task */
        NULL                    /* Handle task (tidak digunakan) */
    );

    /* ---- MULAI FREERTOS SCHEDULER ---- */

    /* Start FreeRTOS scheduler - program tidak akan return dari sini */
    vTaskStartScheduler();

    /* ---- FALLBACK (jika scheduler gagal) ---- */

    /* Jika scheduler gagal start, loop infinite di sini */
    while(1)
    {
    }

    return 0;
}

/* ================================================================================ */
/* END OF FILE - main.c */
/* ================================================================================ */
