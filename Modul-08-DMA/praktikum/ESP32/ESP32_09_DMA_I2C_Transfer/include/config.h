/*
 * ==========================================================================
 *  ESP32 I2C Transfer with Internal FIFO - Configuration
 * ==========================================================================
 *  Modul 08 - Program 09: I2C Data Transfer
 *
 *  CATATAN PENTING:
 *  - ESP32 I2C menggunakan internal FIFO (32 bytes), BUKAN DMA channel
 *  - Berbeda dengan STM32 yang bisa meng-attach DMA channel ke I2C
 *  - ESP32 I2C FIFO di-handle oleh hardware, mengurangi beban CPU
 *  - Burst read tetap efisien karena FIFO di-fill otomatis
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- I2C Configuration ---- */
#define I2C_MASTER_NUM          I2C_NUM_0
#define I2C_MASTER_SDA_IO       21          /* GPIO21 - SDA */
#define I2C_MASTER_SCL_IO       22          /* GPIO22 - SCL */
#define I2C_MASTER_FREQ_HZ      400000      /* 400 kHz (Fast Mode) */
#define I2C_MASTER_TIMEOUT_MS   1000

/* ---- BMP280 Sensor ---- */
#define BMP280_ADDR             0x76        /* Default address (SDO=GND) */
#define BMP280_ADDR_ALT         0x77        /* Alternative (SDO=VCC) */

/* BMP280 Register Map */
#define BMP280_REG_CHIP_ID      0xD0        /* Chip ID (should be 0x58) */
#define BMP280_REG_RESET        0xE0        /* Reset register */
#define BMP280_REG_STATUS       0xF3        /* Status register */
#define BMP280_REG_CTRL_MEAS    0xF4        /* Control measurement */
#define BMP280_REG_CONFIG       0xF5        /* Config register */
#define BMP280_REG_PRESS_MSB    0xF7        /* Pressure data start */
#define BMP280_REG_TEMP_MSB     0xFA        /* Temperature data start */
#define BMP280_REG_CALIB_START  0x88        /* Calibration data start */
#define BMP280_CALIB_LEN        26          /* 26 bytes of calibration */
#define BMP280_DATA_LEN         6           /* 6 bytes: press(3) + temp(3) */
#define BMP280_CHIP_ID_VALUE    0x58        /* Expected chip ID */

/* BMP280 Control Values */
#define BMP280_OSRS_T_X16       (0x05 << 5) /* Temperature oversampling x16 */
#define BMP280_OSRS_P_X16       (0x05 << 2) /* Pressure oversampling x16 */
#define BMP280_MODE_NORMAL      0x03        /* Normal mode */
#define BMP280_CONFIG_STANDBY   (0x00 << 5) /* Standby 0.5ms */
#define BMP280_CONFIG_FILTER    (0x04 << 2) /* Filter coefficient 16 */

/* ---- Timing Test Configuration ---- */
#define TIMING_TEST_ITERATIONS  100         /* Iterations for each method */
#define PERIODIC_READ_INTERVAL  500         /* ms between periodic reads */

/* ---- Task Configuration ---- */
#define READER_TASK_STACK       4096
#define READER_TASK_PRIO        5

#endif /* CONFIG_H */
