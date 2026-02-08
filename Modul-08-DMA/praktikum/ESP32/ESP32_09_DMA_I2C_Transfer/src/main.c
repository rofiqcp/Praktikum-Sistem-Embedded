/*
 * ==========================================================================
 *  ESP32 I2C Transfer with Internal FIFO - BMP280 Sensor
 * ==========================================================================
 *  Modul 08 - Program 09: I2C Data Transfer
 *
 *  PERBEDAAN I2C DMA: ESP32 vs STM32
 *  ┌─────────────────────────────────────────────────────────────────┐
 *  │ STM32:                                                         │
 *  │  - I2C peripheral + DMA channel terpisah                      │
 *  │  - DMA_Stream → I2C_DR register                               │
 *  │  - CPU completely free selama transfer                         │
 *  │  - Bisa transfer ratusan byte tanpa CPU intervention           │
 *  │                                                                 │
 *  │ ESP32:                                                         │
 *  │  - I2C peripheral memiliki FIFO internal (32 bytes)           │
 *  │  - TIDAK ada DMA channel yang bisa di-attach ke I2C           │
 *  │  - Hardware FIFO mengurangi interrupt overhead                 │
 *  │  - Driver ESP-IDF mengelola FIFO secara transparan            │
 *  │  - Untuk transfer besar (>32 byte), FIFO di-refill otomatis   │
 *  └─────────────────────────────────────────────────────────────────┘
 *
 *  Program ini:
 *  1. Inisialisasi I2C master
 *  2. BMP280 sensor driver (ID, kalibrasi, konfigurasi, pembacaan)
 *  3. Perbandingan: single-register vs burst read vs periodic
 *  4. Timing measurement untuk setiap metode
 *
 *  Hardware:
 *  - GPIO21 (SDA) → BMP280 SDA
 *  - GPIO22 (SCL) → BMP280 SCL
 *  - BMP280 VCC → 3.3V, GND → GND
 * ==========================================================================
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "config.h"

static const char *TAG = "I2C_TRANSFER";

/* ==========================================================================
 *  BMP280 Calibration Data Structure
 * ========================================================================== */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} bmp280_calib_t;

static bmp280_calib_t calib;
static int32_t t_fine = 0;     /* Fine temperature for pressure compensation */
static uint8_t bmp280_addr = BMP280_ADDR;  /* Active I2C address */

/* ---- Timing Results ---- */
typedef struct {
    int64_t single_reg_total;       /* Single register reads total time */
    int64_t burst_read_total;       /* Burst read total time */
    int64_t periodic_total;         /* Periodic task-based total time */
    int     iterations;
    float   single_avg_us;
    float   burst_avg_us;
    float   periodic_avg_us;
} timing_result_t;

static timing_result_t timing;

/* ---- Sensor Data ---- */
static float temperature_c = 0;
static float pressure_hpa = 0;
static uint32_t read_count = 0;

/* ==========================================================================
 *  I2C Master Initialization
 * ==========================================================================
 *  ESP32 I2C menggunakan i2c_param_config() + i2c_driver_install()
 *  Driver otomatis mengelola FIFO internal 32 bytes.
 *
 *  Di STM32: I2C init + DMA init terpisah + linking I2C-DMA
 *  Di ESP32: cukup init I2C saja, FIFO sudah built-in
 * ========================================================================== */

static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /*
     * i2c_driver_install:
     * - Mengalokasikan internal buffer dan FIFO
     * - ESP32 I2C FIFO = 32 bytes (hardware)
     * - Driver mengelola FIFO fill/drain secara otomatis
     * - TIDAK perlu konfigurasi DMA terpisah seperti STM32
     */
    ret = i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C master initialized:");
    ESP_LOGI(TAG, "  SDA: GPIO%d, SCL: GPIO%d", I2C_MASTER_SDA_IO,
             I2C_MASTER_SCL_IO);
    ESP_LOGI(TAG, "  Clock: %d Hz (Fast Mode)", I2C_MASTER_FREQ_HZ);
    ESP_LOGI(TAG, "  Internal FIFO: 32 bytes (hardware)");
    ESP_LOGI(TAG, "  I2C di ESP32 menggunakan internal FIFO, "
             "bukan DMA channel terpisah");

    return ESP_OK;
}

/* ==========================================================================
 *  Low-Level I2C Read/Write Functions
 * ========================================================================== */

/**
 * @brief Write single byte to register
 */
static esp_err_t bmp280_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };

    return i2c_master_write_to_device(I2C_MASTER_NUM, bmp280_addr,
                                       buf, sizeof(buf),
                                       pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

/**
 * @brief Read single byte from register (Method 1: single-register read)
 *
 * Ini adalah cara paling sederhana tapi paling lambat.
 * Setiap byte memerlukan start-address-data-stop sequence.
 */
static esp_err_t bmp280_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, bmp280_addr,
                                         &reg, 1, value, 1,
                                         pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

/**
 * @brief Burst read multiple bytes (Method 2: burst read)
 *
 * Lebih efisien: satu start condition, baca banyak byte.
 * ESP32 FIFO handles the data buffering internally.
 * Mirip dengan STM32 DMA-backed I2C read tapi tanpa DMA channel.
 */
static esp_err_t bmp280_burst_read(uint8_t start_reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, bmp280_addr,
                                         &start_reg, 1, data, len,
                                         pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

/* ==========================================================================
 *  BMP280 Sensor Driver
 * ========================================================================== */

/**
 * @brief Scan I2C bus and find BMP280
 */
static esp_err_t bmp280_scan(void)
{
    ESP_LOGI(TAG, "Scanning I2C bus for BMP280...");

    /* Try primary address */
    uint8_t chip_id = 0;
    esp_err_t ret = bmp280_read_reg(BMP280_REG_CHIP_ID, &chip_id);

    if (ret == ESP_OK && chip_id == BMP280_CHIP_ID_VALUE) {
        ESP_LOGI(TAG, "BMP280 found at 0x%02X (chip ID: 0x%02X)",
                 bmp280_addr, chip_id);
        return ESP_OK;
    }

    /* Try alternative address */
    bmp280_addr = BMP280_ADDR_ALT;
    ret = bmp280_read_reg(BMP280_REG_CHIP_ID, &chip_id);

    if (ret == ESP_OK && chip_id == BMP280_CHIP_ID_VALUE) {
        ESP_LOGI(TAG, "BMP280 found at 0x%02X (chip ID: 0x%02X)",
                 bmp280_addr, chip_id);
        return ESP_OK;
    }

    ESP_LOGW(TAG, "BMP280 not found! Chip ID read: 0x%02X (expected 0x58)",
             chip_id);
    ESP_LOGW(TAG, "Program will use simulated data for demonstration.");

    return ESP_ERR_NOT_FOUND;
}

/**
 * @brief Read calibration data (26 bytes from 0x88-0xA1)
 *
 * Menggunakan burst read untuk efisiensi.
 * Di STM32 dengan DMA: setup DMA channel → start transfer → wait interrupt
 * Di ESP32: langsung panggil burst_read, FIFO handles it.
 */
static esp_err_t bmp280_read_calibration(void)
{
    uint8_t raw[BMP280_CALIB_LEN];

    esp_err_t ret = bmp280_burst_read(BMP280_REG_CALIB_START, raw,
                                       BMP280_CALIB_LEN);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Calibration read failed, using default values");
        /* Set default calibration values for simulation */
        calib.dig_T1 = 27504;
        calib.dig_T2 = 26435;
        calib.dig_T3 = -1000;
        calib.dig_P1 = 36477;
        calib.dig_P2 = -10685;
        calib.dig_P3 = 3024;
        calib.dig_P4 = 2855;
        calib.dig_P5 = 140;
        calib.dig_P6 = -7;
        calib.dig_P7 = 15500;
        calib.dig_P8 = -14600;
        calib.dig_P9 = 6000;
        return ESP_OK;
    }

    /* Parse calibration data (little-endian) */
    calib.dig_T1 = (uint16_t)(raw[1] << 8 | raw[0]);
    calib.dig_T2 = (int16_t)(raw[3] << 8 | raw[2]);
    calib.dig_T3 = (int16_t)(raw[5] << 8 | raw[4]);
    calib.dig_P1 = (uint16_t)(raw[7] << 8 | raw[6]);
    calib.dig_P2 = (int16_t)(raw[9] << 8 | raw[8]);
    calib.dig_P3 = (int16_t)(raw[11] << 8 | raw[10]);
    calib.dig_P4 = (int16_t)(raw[13] << 8 | raw[12]);
    calib.dig_P5 = (int16_t)(raw[15] << 8 | raw[14]);
    calib.dig_P6 = (int16_t)(raw[17] << 8 | raw[16]);
    calib.dig_P7 = (int16_t)(raw[19] << 8 | raw[18]);
    calib.dig_P8 = (int16_t)(raw[21] << 8 | raw[20]);
    calib.dig_P9 = (int16_t)(raw[23] << 8 | raw[22]);

    ESP_LOGI(TAG, "Calibration data read (26 bytes burst):");
    ESP_LOGI(TAG, "  dig_T1=%u, dig_T2=%d, dig_T3=%d",
             calib.dig_T1, calib.dig_T2, calib.dig_T3);
    ESP_LOGI(TAG, "  dig_P1=%u, dig_P2=%d, dig_P3=%d",
             calib.dig_P1, calib.dig_P2, calib.dig_P3);

    return ESP_OK;
}

/**
 * @brief Configure BMP280 for measurement
 */
static esp_err_t bmp280_configure(void)
{
    esp_err_t ret;

    /* Reset sensor */
    ret = bmp280_write_reg(BMP280_REG_RESET, 0xB6);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Reset write failed (sensor may not be present)");
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Configure: filter and standby */
    ret = bmp280_write_reg(BMP280_REG_CONFIG,
                            BMP280_CONFIG_STANDBY | BMP280_CONFIG_FILTER);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Config write failed");
    }

    /* Configure: oversampling and normal mode */
    ret = bmp280_write_reg(BMP280_REG_CTRL_MEAS,
                            BMP280_OSRS_T_X16 | BMP280_OSRS_P_X16 |
                            BMP280_MODE_NORMAL);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Ctrl_meas write failed");
    }

    ESP_LOGI(TAG, "BMP280 configured:");
    ESP_LOGI(TAG, "  Temperature oversampling: x16");
    ESP_LOGI(TAG, "  Pressure oversampling: x16");
    ESP_LOGI(TAG, "  Mode: Normal");
    ESP_LOGI(TAG, "  Filter: coefficient 16");

    return ESP_OK;
}

/* ==========================================================================
 *  BMP280 Compensation Formulas
 * ==========================================================================
 *  From BMP280 datasheet section 8.1
 * ========================================================================== */

/**
 * @brief Compensate raw temperature to °C
 */
static float bmp280_compensate_temperature(int32_t adc_T)
{
    int32_t var1, var2;

    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) *
            ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) *
            ((int32_t)calib.dig_T3)) >> 14;

    t_fine = var1 + var2;
    float T = (float)((t_fine * 5 + 128) >> 8) / 100.0f;

    return T;
}

/**
 * @brief Compensate raw pressure to hPa
 */
static float bmp280_compensate_pressure(int32_t adc_P)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) +
           ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;

    if (var1 == 0) return 0;

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);

    return (float)p / 25600.0f;
}

/* ==========================================================================
 *  Method 1: Single-Register Reads (one byte at a time)
 * ==========================================================================
 *  Paling lambat: setiap byte memerlukan transaksi I2C terpisah.
 *  START → ADDR → REG → RESTART → ADDR → DATA → STOP (per byte)
 * ========================================================================== */

static void method1_single_register_read(float *temp, float *press)
{
    uint8_t data[6];

    /* Read 6 bytes one at a time - 6 separate I2C transactions */
    for (int i = 0; i < 6; i++) {
        bmp280_read_reg(BMP280_REG_PRESS_MSB + i, &data[i]);
    }

    /* Parse raw values */
    int32_t adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) |
                    ((int32_t)data[2] >> 4);
    int32_t adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) |
                    ((int32_t)data[5] >> 4);

    /* If no sensor, use simulated values */
    if (adc_T == 0 && adc_P == 0) {
        static float sim_phase = 0;
        adc_T = 400000 + (int32_t)(5000 * sinf(sim_phase));
        adc_P = 300000 + (int32_t)(2000 * cosf(sim_phase * 0.7f));
        sim_phase += 0.01f;
    }

    *temp = bmp280_compensate_temperature(adc_T);
    *press = bmp280_compensate_pressure(adc_P);
}

/* ==========================================================================
 *  Method 2: Burst Read (multiple bytes in one transaction)
 * ==========================================================================
 *  Lebih efisien: satu transaksi I2C untuk semua 6 bytes.
 *  START → ADDR → REG → RESTART → ADDR → D0 D1 D2 D3 D4 D5 → STOP
 *
 *  ESP32 I2C FIFO secara otomatis menampung data selama burst read.
 *  Di STM32: ini setara dengan DMA-backed I2C read.
 * ========================================================================== */

static void method2_burst_read(float *temp, float *press)
{
    uint8_t data[BMP280_DATA_LEN];

    /* Read all 6 bytes in one I2C transaction */
    esp_err_t ret = bmp280_burst_read(BMP280_REG_PRESS_MSB, data,
                                       BMP280_DATA_LEN);

    int32_t adc_P, adc_T;

    if (ret == ESP_OK) {
        adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) |
                ((int32_t)data[2] >> 4);
        adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) |
                ((int32_t)data[5] >> 4);
    } else {
        /* Simulated values */
        static float sim_phase = 0;
        adc_T = 400000 + (int32_t)(5000 * sinf(sim_phase));
        adc_P = 300000 + (int32_t)(2000 * cosf(sim_phase * 0.7f));
        sim_phase += 0.01f;
    }

    /* Handle zero readings with simulation */
    if (adc_T == 0 && adc_P == 0) {
        static float sim_p2 = 0;
        adc_T = 400000 + (int32_t)(5000 * sinf(sim_p2));
        adc_P = 300000 + (int32_t)(2000 * cosf(sim_p2 * 0.7f));
        sim_p2 += 0.01f;
    }

    *temp = bmp280_compensate_temperature(adc_T);
    *press = bmp280_compensate_pressure(adc_P);
}

/* ==========================================================================
 *  Timing Comparison Test
 * ========================================================================== */

static void run_timing_comparison(void)
{
    float temp, press;
    int64_t start, elapsed;

    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║        I2C TRANSFER TIMING COMPARISON               ║\n");
    printf("╠══════════════════════════════════════════════════════╣\n");
    printf("║  Comparing transfer methods (%d iterations each)    ║\n",
           TIMING_TEST_ITERATIONS);
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    /* ---- Method 1: Single Register Reads ---- */
    ESP_LOGI(TAG, "Testing Method 1: Single-register reads...");
    start = esp_timer_get_time();
    for (int i = 0; i < TIMING_TEST_ITERATIONS; i++) {
        method1_single_register_read(&temp, &press);
    }
    timing.single_reg_total = esp_timer_get_time() - start;
    timing.single_avg_us = (float)timing.single_reg_total /
                            TIMING_TEST_ITERATIONS;
    ESP_LOGI(TAG, "  Total: %lld us, Avg: %.1f us/read",
             timing.single_reg_total, timing.single_avg_us);

    vTaskDelay(pdMS_TO_TICKS(100));

    /* ---- Method 2: Burst Read ---- */
    ESP_LOGI(TAG, "Testing Method 2: Burst read (6 bytes)...");
    start = esp_timer_get_time();
    for (int i = 0; i < TIMING_TEST_ITERATIONS; i++) {
        method2_burst_read(&temp, &press);
    }
    timing.burst_read_total = esp_timer_get_time() - start;
    timing.burst_avg_us = (float)timing.burst_read_total /
                           TIMING_TEST_ITERATIONS;
    ESP_LOGI(TAG, "  Total: %lld us, Avg: %.1f us/read",
             timing.burst_read_total, timing.burst_avg_us);

    vTaskDelay(pdMS_TO_TICKS(100));

    /* ---- Method 3: Periodic Task-Based (timed reads) ---- */
    ESP_LOGI(TAG, "Testing Method 3: Periodic task-based reads...");
    int periodic_count = 20;
    start = esp_timer_get_time();
    for (int i = 0; i < periodic_count; i++) {
        method2_burst_read(&temp, &press);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    timing.periodic_total = esp_timer_get_time() - start;
    timing.periodic_avg_us = (float)timing.periodic_total / periodic_count;
    ESP_LOGI(TAG, "  Total: %lld us, Avg: %.1f us/read (includes delay)",
             timing.periodic_total, timing.periodic_avg_us);

    timing.iterations = TIMING_TEST_ITERATIONS;

    /* ---- Print Comparison Table ---- */
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║                 TIMING RESULTS                          ║\n");
    printf("╠═══════════════╦══════════════╦═══════════╦══════════════╣\n");
    printf("║ Method        ║ Total (us)   ║ Avg (us)  ║ Speedup     ║\n");
    printf("╠═══════════════╬══════════════╬═══════════╬══════════════╣\n");
    printf("║ Single-reg    ║ %10lld  ║ %7.1f   ║ 1.00x (base)║\n",
           timing.single_reg_total, timing.single_avg_us);
    printf("║ Burst read    ║ %10lld  ║ %7.1f   ║ %.2fx       ║\n",
           timing.burst_read_total, timing.burst_avg_us,
           timing.single_avg_us / (timing.burst_avg_us > 0 ?
                                    timing.burst_avg_us : 1));
    printf("╠═══════════════╩══════════════╩═══════════╩══════════════╣\n");
    printf("║                                                          ║\n");
    printf("║  PENJELASAN:                                             ║\n");
    printf("║  I2C di ESP32 menggunakan internal FIFO (32 bytes),      ║\n");
    printf("║  bukan DMA channel terpisah seperti STM32.               ║\n");
    printf("║                                                          ║\n");
    printf("║  Burst read lebih efisien karena:                        ║\n");
    printf("║  • Satu START condition vs 6 START conditions            ║\n");
    printf("║  • FIFO menampung data secara otomatis                   ║\n");
    printf("║  • Lebih sedikit overhead protokol I2C                   ║\n");
    printf("║                                                          ║\n");
    printf("║  Di STM32: DMA I2C → data langsung ke RAM               ║\n");
    printf("║  Di ESP32: FIFO I2C → driver copy ke RAM                 ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
}

/* ==========================================================================
 *  Periodic Reading Task
 * ========================================================================== */

static void periodic_read_task(void *arg)
{
    ESP_LOGI(TAG, "Periodic reading task started (interval=%dms)",
             PERIODIC_READ_INTERVAL);

    while (1) {
        float temp, press;
        int64_t start = esp_timer_get_time();

        method2_burst_read(&temp, &press);

        int64_t elapsed = esp_timer_get_time() - start;

        temperature_c = temp;
        pressure_hpa = press;
        read_count++;

        /* Print readings */
        printf("╔══════════════════════════════════════════════════╗\n");
        printf("║  BMP280 Reading #%lu                             ║\n",
               (unsigned long)read_count);
        printf("╠══════════════════════════════════════════════════╣\n");
        printf("║  Temperature: %7.2f °C                         ║\n",
               temperature_c);
        printf("║  Pressure:    %7.2f hPa                        ║\n",
               pressure_hpa);
        printf("║  Read time:   %7lld us (burst, FIFO-based)     ║\n",
               elapsed);
        printf("╚══════════════════════════════════════════════════╝\n");

        vTaskDelay(pdMS_TO_TICKS(PERIODIC_READ_INTERVAL));
    }
}

/* ==========================================================================
 *  I2C Bus Scan (utility)
 * ========================================================================== */

static void i2c_bus_scan(void)
{
    printf("\n  I2C Bus Scan:\n");
    printf("  ──────────────────────────────────\n");

    int devices_found = 0;

    for (uint8_t addr = 1; addr < 127; addr++) {
        uint8_t dummy;
        esp_err_t ret = i2c_master_write_read_device(
            I2C_MASTER_NUM, addr, &addr, 0, &dummy, 0,
            pdMS_TO_TICKS(50));

        if (ret == ESP_OK) {
            const char *name = "Unknown";
            if (addr == 0x76 || addr == 0x77) name = "BMP280/BME280";
            else if (addr == 0x3C || addr == 0x3D) name = "SSD1306 OLED";
            else if (addr == 0x68 || addr == 0x69) name = "MPU6050";
            else if (addr >= 0x20 && addr <= 0x27) name = "PCF8574";

            printf("  ✓ Device found: 0x%02X (%s)\n", addr, name);
            devices_found++;
        }
    }

    if (devices_found == 0) {
        printf("  ✗ No devices found on I2C bus\n");
        printf("    (Program will use simulated BMP280 data)\n");
    }

    printf("  Total devices: %d\n\n", devices_found);
}

/* ==========================================================================
 *  Main Application
 * ========================================================================== */

void app_main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║    ESP32 I2C Transfer (FIFO) - Modul 08 Program 09     ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  BMP280 sensor via I2C dengan internal FIFO             ║\n");
    printf("║                                                          ║\n");
    printf("║  ESP32 I2C: FIFO internal 32 bytes (bukan DMA)          ║\n");
    printf("║  STM32 I2C: DMA channel terpisah                        ║\n");
    printf("║                                                          ║\n");
    printf("║  Perbandingan: single-reg vs burst vs periodic          ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    /* Initialize I2C master */
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed!");
        return;
    }

    /* Scan I2C bus */
    i2c_bus_scan();

    /* Initialize BMP280 */
    ret = bmp280_scan();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "BMP280 not found, using simulated data");
    }

    /* Read calibration data */
    bmp280_read_calibration();

    /* Configure sensor */
    bmp280_configure();

    /* Wait for first measurement */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Run timing comparison */
    run_timing_comparison();

    /* Start periodic reading task */
    ESP_LOGI(TAG, "\nStarting periodic reading task...\n");
    xTaskCreate(periodic_read_task, "bmp280_read",
                READER_TASK_STACK, NULL, READER_TASK_PRIO, NULL);
}
