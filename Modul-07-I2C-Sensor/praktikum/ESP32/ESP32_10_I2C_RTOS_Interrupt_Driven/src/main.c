/**
 * ==========================================================
 *  Modul 07 - ESP32_10_I2C_RTOS_Interrupt_Driven
 * ==========================================================
 *  Deskripsi:
 *    RTOS interrupt-driven I2C read. GPIO interrupt (tombol/
 *    sensor INT pin) trigger ISR, kirim FreeRTOS notification
 *    ke task yang membaca sensor MPU6050 via I2C. Menunjukkan
 *    integrasi GPIO ISR + RTOS + I2C.
 *  Hardware:
 *    ESP32 DevKit / ESP32-S2 / ESP32-S3
 *  Koneksi Pin:
 *    SDA = GPIO21 (ESP32), GPIO8 (S2/S3)
 *    SCL = GPIO22 (ESP32), GPIO9 (S2/S3)
 *    INT_PIN = GPIO0 (push button dengan pullup)
 *  Instruksi:
 *    1. Pasang MPU6050 dan hubungkan tombol ke GPIO0
 *    2. Ubah nilai #define di atas untuk mencoba variasi
 *    3. Build & upload dengan PlatformIO
 *    4. Tekan tombol untuk trigger I2C read
 *  Variabel yang bisa dicoba (#define):
 *    I2C_SDA_GPIO GPIO_NUM_21
 *    I2C_SCL_GPIO GPIO_NUM_22
 *    I2C_PORT I2C_NUM_0
 *    I2C_FREQ_HZ 400000
 *    MPU6050_ADDR 0x68
 *    INT_GPIO GPIO_NUM_0
 *    DEBOUNCE_MS 50
 * ==========================================================
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Konfigurasi pin I2C
#if defined(CONFIG_IDF_TARGET_ESP32)
#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
#define I2C_SDA_GPIO GPIO_NUM_8
#define I2C_SCL_GPIO GPIO_NUM_9
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
#define I2C_SDA_GPIO GPIO_NUM_8
#define I2C_SCL_GPIO GPIO_NUM_9
#else
#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#endif
#define I2C_PORT I2C_NUM_0
#define I2C_FREQ_HZ 400000

// Alamat MPU6050
#define MPU6050_ADDR 0x68

// GPIO interrupt pin (tombol atau sensor INT)
#define INT_GPIO GPIO_NUM_0

// Debounce
#define DEBOUNCE_MS 50

// Event bit untuk notifikasi
#define I2C_READ_BIT (1 << 0)

static const char *TAG = "ISR_I2C";

// I2C
static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t mpu;

// Event group untuk komunikasi ISR -> task
static EventGroupHandle_t event_group;

// ISR handler
static void IRAM_ATTR gpio_isr_handler(void *arg) {
    // Kirim event dari ISR
    BaseType_t higher_woken = pdFALSE;
    xEventGroupSetBitsFromISR(event_group, I2C_READ_BIT, &higher_woken);
    if (higher_woken) portYIELD_FROM_ISR();
}

// Init MPU6050
static bool init_mpu6050(void) {
    i2c_device_config_t dev_cfg = {
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    if (i2c_master_bus_add_device(bus, &dev_cfg, &mpu) != ESP_OK) return false;
    uint8_t buf[2] = {0x6B, 0x00};
    i2c_master_transmit(mpu, buf, 2, -1);
    return true;
}

// Task yang menunggu notifikasi interrupt
static void i2c_read_task(void *arg) {
    ESP_LOGI(TAG, "I2C read task started, waiting for interrupt...");

    while (1) {
        // Tunggu event bit dari ISR
        EventBits_t bits = xEventGroupWaitBits(event_group,
                                              I2C_READ_BIT,
                                              pdTRUE,  // Clear on exit
                                              pdFALSE, // Wait for any bit
                                              portMAX_DELAY);

        if (bits & I2C_READ_BIT) {
            // Debounce sederhana
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));

            // Baca MPU6050 via I2C
            uint8_t reg = 0x3B;
            uint8_t raw[14];
            esp_err_t ret = i2c_master_transmit_receive(mpu, &reg, 1, raw, 14, -1);

            if (ret == ESP_OK) {
                int16_t ax = (int16_t)((raw[0] << 8) | raw[1]);
                int16_t ay = (int16_t)((raw[2] << 8) | raw[3]);
                int16_t az = (int16_t)((raw[4] << 8) | raw[5]);
                int16_t gx = (int16_t)((raw[8] << 8) | raw[9]);
                int16_t gy = (int16_t)((raw[10] << 8) | raw[11]);
                int16_t gz = (int16_t)((raw[12] << 8) | raw[13]);

                float ax_g = ax / 16384.0f;
                float ay_g = ay / 16384.0f;
                float az_g = az / 16384.0f;

                float roll = atan2f(ay_g, az_g) * 57.2958f;
                float pitch = atan2f(-ax_g, sqrtf(ay_g * ay_g + az_g * az_g)) * 57.2958f;

                printf("ISR_TRIGGERED: ax=%d ay=%d az=%d | Roll=%.1f Pitch=%.1f\n",
                       ax, ay, az, roll, pitch);
                ESP_LOGI(TAG, "I2C read completed");
            } else {
                ESP_LOGE(TAG, "I2C read failed");
            }

            // Clear interrupt status (jika pakai sensor INT pin)
            // Baca register INT_STATUS MPU6050: 0x3A
            uint8_t int_status;
            i2c_master_transmit_receive(mpu, (uint8_t[]){0x3A}, 1, &int_status, 1, -1);
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP32 I2C RTOS Interrupt Driven");

    // Init I2C
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    // Init MPU6050
    if (!init_mpu6050()) {
        ESP_LOGE(TAG, "MPU6050 tidak ditemukan");
        return;
    }

    // Create event group
    event_group = xEventGroupCreate();
    if (event_group == NULL) {
        ESP_LOGE(TAG, "Gagal membuat event group");
        return;
    }

    // Configure GPIO interrupt
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << INT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,  // Falling edge (tombol ditekan)
    };
    gpio_config(&io_conf);

    // Install ISR service
    gpio_install_isr_service(0);
    gpio_isr_handler_add(INT_GPIO, gpio_isr_handler, NULL);

    // Create task
    xTaskCreate(i2c_read_task, "i2c_read_task", 4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "Sistem siap. Tekan tombol di GPIO%d untuk trigger I2C read", INT_GPIO);
}
