// src/main.c untuk ESP32_10_Multi_Peripheral_RTOS
// Program FreeRTOS dengan semua peripheral dan queue sets/event groups
// Updated for ESP-IDF 5.x

// Menginclude header FreeRTOS untuk task creation dan delay
#include "freertos/FreeRTOS.h"
// Menginclude header FreeRTOS untuk queue
#include "freertos/queue.h"
// Menginclude header FreeRTOS untuk semaphore
#include "freertos/semphr.h"
// Menginclude header FreeRTOS untuk event groups
#include "freertos/event_groups.h"
// Menginclude header FreeRTOS untuk task management
#include "freertos/task.h"
// Menginclude header GPIO ESP32
#include "driver/gpio.h"
// Menginclude header UART ESP32
#include "driver/uart.h"
// Menginclude header ADC ESP32 (new oneshot driver)
#include "esp_adc/adc_oneshot.h"
// Menginclude header LEDC (PWM) ESP32
#include "driver/ledc.h"
// Menginclude header I2C ESP32 (new master driver)
#include "driver/i2c_master.h"
// Menginclude header untuk konfigurasi aplikasi
#include "config.h"
// Menginclude header standard untuk printf
#include "stdio.h"
// Menginclude header standard untuk string
#include "string.h"

// Mendefinisikan struktur untuk button event
typedef struct {
    // Timestamp event
    uint32_t timestamp;
} button_event_t;

// Mendefinisikan struktur untuk UART command
typedef struct {
    // Command string
    char cmd[16];
    // Timestamp command
    uint32_t timestamp;
} uart_cmd_t;

// Mendefinisikan struktur untuk ADC data
typedef struct {
    // Nilai ADC mentah
    uint32_t raw_value;
    // Timestamp pembacaan
    uint32_t timestamp;
} adc_data_t;

// Mendefinisikan handle untuk queue button
QueueHandle_t xButtonQueue = NULL;
// Mendefinisikan handle untuk queue UART
QueueHandle_t xUartQueue = NULL;
// Mendefinisikan handle untuk queue ADC
QueueHandle_t xAdcQueue = NULL;
// Mendefinisikan handle untuk queue set
QueueSetHandle_t xQueueSet = NULL;
// Mendefinisikan handle untuk event group
EventGroupHandle_t xEventGroup = NULL;
// Mendefinisikan handle untuk I2C mutex
SemaphoreHandle_t xI2CMutex = NULL;
// Mendefinisikan handle untuk SPI mutex
SemaphoreHandle_t xSPIMutex = NULL;
// Mendefinisikan handle untuk task LED
TaskHandle_t xLedTaskHandle = NULL;
// Mendefinisikan handle untuk task UART
TaskHandle_t xUartTaskHandle = NULL;
// Mendefinisikan handle untuk task ADC
TaskHandle_t xAdcTaskHandle = NULL;
// Mendefinisikan handle untuk task system monitor
TaskHandle_t xMonTaskHandle = NULL;

// Variabel global untuk statistik sistem
uint32_t button_count = 0;
// Variabel global untuk counter UART
uint32_t uart_count = 0;
// Variabel global untuk counter ADC
uint32_t adc_count = 0;

// Handle untuk ADC oneshot
adc_oneshot_unit_handle_t adc_handle = NULL;
// Handle untuk I2C master bus
i2c_master_bus_handle_t i2c_bus_handle = NULL;

// Fungsi ISR untuk button
void IRAM_ATTR button_isr_handler(void *arg) {
    // Membuat button event
    button_event_t event;
    // Mengisi timestamp
    event.timestamp = xTaskGetTickCountFromISR();
    // Mengirim ke queue dari ISR
    xQueueSendFromISR(xButtonQueue, &event, NULL);
    // Set event group bit untuk button
    xEventGroupSetBitsFromISR(xEventGroup, EVENT_BIT_BUTTON, NULL);
}

// Inisialisasi semua peripheral
void init_all_peripherals(void) {
    // Inisialisasi GPIO
    // Mengatur mode button sebagai input
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    // Mengatur pull-up untuk button
    gpio_pullup_en(BUTTON_GPIO);
    // Mengatur interrupt type falling edge
    gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_NEGEDGE);
    // Mengaktifkan interrupt
    gpio_intr_enable(BUTTON_GPIO);
    // Mendaftarkan ISR
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);
    
    // Mengatur mode LED1 sebagai output
    gpio_set_direction(LED1_GPIO, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED1
    gpio_set_level(LED1_GPIO, 0);
    // Mengatur mode LED2 sebagai output
    gpio_set_direction(LED2_GPIO, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED2
    gpio_set_level(LED2_GPIO, 0);
    
    // Inisialisasi UART
    // Mengatur konfigurasi UART
    uart_config_t uart_config = {
        // Baud rate 115200
        .baud_rate = UART_BAUD_RATE,
        // 8 data bits
        .data_bits = UART_DATA_8_BITS,
        // No parity
        .parity = UART_PARITY_DISABLE,
        // 1 stop bit
        .stop_bits = UART_STOP_BITS_1,
        // No flow control
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    // Mengatur parameter UART
    uart_param_config(UART_PORT, &uart_config);
    // Mengatur pin UART
    uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // Menginstall driver UART
    uart_driver_install(UART_PORT, UART_BUF_SIZE, 0, 0, NULL, 0);
    
    // Inisialisasi ADC menggunakan new oneshot driver
    adc_oneshot_unit_init_cfg_t adc_init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = false,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_init_config, &adc_handle));
    
    adc_oneshot_chan_cfg_t adc_chan_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &adc_chan_config));
    
    // Inisialisasi PWM/LEDC
    // Mengatur timer LEDC
    ledc_timer_config_t ledc_timer = {
        // Resolusi 8 bit
        .duty_resolution = LEDC_TIMER_8_BIT,
        // Frekuensi 1 kHz
        .freq_hz = PWM_FREQUENCY,
        // Mode high speed
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        // Timer 0
        .timer_num = LEDC_TIMER,
        // Clock otomatis
        .clk_cfg = LEDC_AUTO_CLK
    };
    // Mengatur timer
    ledc_timer_config(&ledc_timer);
    
    // Mengatur channel LEDC
    ledc_channel_config_t ledc_channel = {
        // Channel 0
        .channel = LEDC_CHANNEL,
        // Duty cycle awal 0
        .duty = 0,
        // GPIO PWM
        .gpio_num = PWM_GPIO,
        // Mode high speed
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        // Timer 0
        .timer_sel = LEDC_TIMER,
        // Hpoint 0
        .hpoint = 0
    };
    // Mengatur channel
    ledc_channel_config(&ledc_channel);
    
    // Inisialisasi I2C menggunakan new master driver
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .glitch_ignore_cnt = 7,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));
}

// Task untuk LED blink
void task_led_blink(void *pvParameters) {
    // State LED
    uint8_t led1_state = 0;
    // Loop tak terhingga
    while (1) {
        // Toggle LED1
        led1_state = !led1_state;
        // Mengatur level LED1
        gpio_set_level(LED1_GPIO, led1_state);
        // Delay task
        vTaskDelay(TASK_LED_DELAY);
    }
}

// Task untuk UART processing
void task_uart_process(void *pvParameters) {
    // Buffer untuk UART
    uint8_t data[UART_BUF_SIZE];
    // Panjang data
    int len;
    // Command structure
    uart_cmd_t cmd;
    // Loop tak terhingga
    while (1) {
        // Membaca UART
        len = uart_read_bytes(UART_PORT, data, UART_BUF_SIZE, TASK_UART_DELAY);
        // Mengecek ada data
        if (len > 0) {
            // Null-terminate
            data[len] = 0;
            // Copy ke cmd
            strncpy(cmd.cmd, (char *)data, sizeof(cmd.cmd) - 1);
            // Mengakhiri string
            cmd.cmd[sizeof(cmd.cmd) - 1] = '\0';
            // Mengisi timestamp
            cmd.timestamp = xTaskGetTickCount();
            // Mengirim ke queue
            xQueueSend(xUartQueue, &cmd, 0);
            // Increment counter
            uart_count++;
            // Set event bit
            xEventGroupSetBits(xEventGroup, EVENT_BIT_UART);
            // Mencetak
            printf("UART: %s", cmd.cmd);
        }
        // Delay task
        vTaskDelay(TASK_UART_DELAY);
    }
}

// Task untuk ADC read
void task_adc_read(void *pvParameters) {
    // Nilai ADC
    int adc_val;
    // Data ADC
    adc_data_t adc_data;
    // Loop tak terhingga
    while (1) {
        // Membaca ADC
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_val);
        // Mengisi struktur
        adc_data.raw_value = (uint32_t)adc_val;
        // Mengisi timestamp
        adc_data.timestamp = xTaskGetTickCount();
        // Mengirim ke queue
        xQueueSend(xAdcQueue, &adc_data, 0);
        // Increment counter
        adc_count++;
        // Set event bit
        xEventGroupSetBits(xEventGroup, EVENT_BIT_ADC);
        // Delay task
        vTaskDelay(TASK_ADC_DELAY);
    }
}

// Task untuk system monitor
void task_system_monitor(void *pvParameters) {
    // Queue set member
    QueueSetMemberHandle_t xActivatedMember;
    // Button event
    button_event_t btn_evt;
    // UART command
    uart_cmd_t uart_cmd;
    // ADC data
    adc_data_t adc_data;
    // Event bits
    EventBits_t uxBits;
    // Loop tak terhingga
    while (1) {
        // Mengecek queue set
        xActivatedMember = xQueueSelectFromSet(xQueueSet, portMAX_DELAY);
        // Mengecek yang mana yang aktif
        if (xActivatedMember == xButtonQueue) {
            // Menerima dari button queue
            xQueueReceive(xButtonQueue, &btn_evt, 0);
            // Increment counter
            button_count++;
            // Mencetak
            printf("Monitor: Button pressed at %lu\n", btn_evt.timestamp);
        } else if (xActivatedMember == xUartQueue) {
            // Menerima dari UART queue
            xQueueReceive(xUartQueue, &uart_cmd, 0);
            // Mencetak
            printf("Monitor: UART cmd: %s", uart_cmd.cmd);
        } else if (xActivatedMember == xAdcQueue) {
            // Menerima dari ADC queue
            xQueueReceive(xAdcQueue, &adc_data, 0);
            // Mencetak
            printf("Monitor: ADC = %lu\n", adc_data.raw_value);
        }
        // Mengecek event group
        uxBits = xEventGroupGetBits(xEventGroup);
        // Mengecek bit error
        if (uxBits & EVENT_BIT_ERROR) {
            // Mencetak error
            printf("Monitor: ERROR detected!\n");
            // Clear bit error
            xEventGroupClearBits(xEventGroup, EVENT_BIT_ERROR);
        }
        // Mengambil event bits lagi
        uxBits = xEventGroupGetBits(xEventGroup);
        // Mengecek jika semua bit set
        if ((uxBits & (EVENT_BIT_BUTTON | EVENT_BIT_UART | EVENT_BIT_ADC)) == 
            (EVENT_BIT_BUTTON | EVENT_BIT_UART | EVENT_BIT_ADC)) {
            // Mencetak statistik
            printf("Stats: Btn=%lu, UART=%lu, ADC=%lu\n", button_count, uart_count, adc_count);
            // Clear semua bit
            xEventGroupClearBits(xEventGroup, EVENT_BIT_BUTTON | EVENT_BIT_UART | EVENT_BIT_ADC);
        }
    }
}

// Fungsi utama program
void app_main(void) {
    // Mencetak pesan mulai
    printf("ESP32 Multi-Peripheral RTOS Demo Started\n");
    // Mencetak info
    printf("Using Queue Sets and Event Groups\n");
    // Inisialisasi semua peripheral
    init_all_peripherals();
    // Membuat button queue
    xButtonQueue = xQueueCreate(BUTTON_QUEUE_SIZE, sizeof(button_event_t));
    // Membuat UART queue
    xUartQueue = xQueueCreate(UART_QUEUE_SIZE, sizeof(uart_cmd_t));
    // Membuat ADC queue
    xAdcQueue = xQueueCreate(ADC_QUEUE_SIZE, sizeof(adc_data_t));
    // Membuat queue set
    xQueueSet = xQueueCreateSet(BUTTON_QUEUE_SIZE + UART_QUEUE_SIZE + ADC_QUEUE_SIZE);
    // Menambahkan queue ke queue set
    xQueueAddToSet(xButtonQueue, xQueueSet);
    // Menambahkan queue ke queue set
    xQueueAddToSet(xUartQueue, xQueueSet);
    // Menambahkan queue ke queue set
    xQueueAddToSet(xAdcQueue, xQueueSet);
    // Membuat event group
    xEventGroup = xEventGroupCreate();
    // Membuat I2C mutex
    xI2CMutex = xSemaphoreCreateMutex();
    // Membuat SPI mutex
    xSPIMutex = xSemaphoreCreateMutex();
    // Mengecek semua berhasil
    if (xButtonQueue && xUartQueue && xAdcQueue && xQueueSet && xEventGroup && xI2CMutex && xSPIMutex) {
        // Mencetak sukses
        printf("All queues, sets, and mutexes created\n");
        // Task LED blink
        xTaskCreate(task_led_blink, "LEDBlink", TASK_LED_STACK_SIZE, NULL, TASK_LED_PRIORITY, &xLedTaskHandle);
        // Task UART process
        xTaskCreate(task_uart_process, "UARTProc", TASK_UART_STACK_SIZE, NULL, TASK_UART_PRIORITY, &xUartTaskHandle);
        // Task ADC read
        xTaskCreate(task_adc_read, "ADCRead", TASK_ADC_STACK_SIZE, NULL, TASK_ADC_PRIORITY, &xAdcTaskHandle);
        // Task system monitor
        xTaskCreate(task_system_monitor, "SysMon", TASK_MON_STACK_SIZE, NULL, TASK_MON_PRIORITY, &xMonTaskHandle);
        // Mencetak sukses
        printf("All tasks created successfully\n");
        // Mencetak petunjuk
        printf("System running with Queue Sets and Event Groups\n");
    } else {
        // Mencetak error
        printf("Failed to create some resources!\n");
    }
}
