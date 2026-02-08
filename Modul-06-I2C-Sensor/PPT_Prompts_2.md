# 🎨 PPT Prompts Modul 06 — Komunikasi Cerdas: I2C Implementasi (Bagian 2: Slide 21-40)

## Petunjuk Penggunaan
Lanjutan slide 21-40. Fokus pada implementasi praktis I2C di ESP32 (ESP-IDF) dan STM32 (HAL), interfacing sensor, dan percobaan praktikum.

---

## Slide 21: I2C pada ESP32 — Overview
```
Buatkan slide dengan judul "I2C pada ESP32 (ESP-IDF)" berisi:
- ESP32 memiliki 2 I2C controller: I2C0 dan I2C1
- Pin default: I2C0 → SDA=GPIO21, SCL=GPIO22
- Pin bisa di-remap ke GPIO manapun (GPIO matrix)
- Mode: Master dan Slave
- Kecepatan: Standard (100K), Fast (400K)
- Driver API: driver/i2c.h (legacy) atau esp_idf/i2c (new driver v5.x+)
- Fitur: Clock stretching, timeout, ACK checking

Diagram: ESP32 block diagram dengan I2C controller highlighted
```

## Slide 22: ESP32 I2C — Konfigurasi Driver
```
Buatkan slide dengan judul "Konfigurasi I2C Master (ESP-IDF)" berisi kode:

i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = GPIO_NUM_21,
    .scl_io_num = GPIO_NUM_22,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 400000,  // 400 kHz
};
i2c_param_config(I2C_NUM_0, &conf);
i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

Penjelasan setiap parameter:
- mode: Master atau Slave
- sda/scl_pullup_en: Internal pull-up (lemah ~45kΩ, tetap perlu eksternal)
- clk_speed: Kecepatan clock I2C
```

## Slide 23: ESP32 I2C — Command Link API
```
Buatkan slide dengan judul "ESP32 I2C Command Link" berisi:
- ESP-IDF menggunakan "command link" untuk build I2C transaction
- Langkah:
  1. i2c_cmd_link_create() → buat command link
  2. i2c_master_start() → START condition
  3. i2c_master_write_byte() → kirim address/data
  4. i2c_master_read() → baca data
  5. i2c_master_stop() → STOP condition
  6. i2c_master_cmd_begin() → eksekusi semua command
  7. i2c_cmd_link_delete() → free memory

Diagram flowchart: Create → Build → Execute → Delete
Contoh kode write register dan read register.
```

## Slide 24: ESP32 I2C — Contoh Read Sensor
```
Buatkan slide dengan judul "Membaca Register Sensor (ESP-IDF)" berisi kode:

// Write register address, then read data
i2c_cmd_handle_t cmd = i2c_cmd_link_create();
i2c_master_start(cmd);
i2c_master_write_byte(cmd, (BMP280_ADDR << 1) | I2C_MASTER_WRITE, true);
i2c_master_write_byte(cmd, reg_addr, true);
i2c_master_start(cmd);  // Repeated START
i2c_master_write_byte(cmd, (BMP280_ADDR << 1) | I2C_MASTER_READ, true);
i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
i2c_master_stop(cmd);
esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
i2c_cmd_link_delete(cmd);

Diagram: Sequence START→ADDR+W→ACK→REG→ACK→RSTART→ADDR+R→ACK→DATA→NACK→STOP
```

## Slide 25: I2C pada STM32 — Overview
```
Buatkan slide dengan judul "I2C pada STM32 (HAL)" berisi:
- STM32F103 (BluePill): I2C1 (PB6/PB7), I2C2 (PB10/PB11)
- STM32F401/F411: I2C1 (PB6/PB7 AF4), I2C2, I2C3
- Mode: Master, Slave, Multi-Master
- Kecepatan: Standard, Fast, Fast Plus (F4 series)
- HAL API: HAL_I2C_Mem_Read/Write, HAL_I2C_Master_Transmit/Receive
- Mode transfer: Polling, Interrupt, DMA

Diagram: STM32 I2C block diagram dengan AF mapping
PENTING: Address di HAL di-shift left 1 bit! 0x76 → 0x76 << 1 = 0xEC
```

## Slide 26: STM32 I2C — Konfigurasi HAL
```
Buatkan slide dengan judul "Konfigurasi I2C (STM32 HAL)" berisi kode:

// Di MX_I2C1_Init() atau manual init
hi2c1.Instance = I2C1;
hi2c1.Init.ClockSpeed = 400000;
hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
hi2c1.Init.OwnAddress1 = 0;
hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
HAL_I2C_Init(&hi2c1);

// GPIO Config: PB6=SCL, PB7=SDA, AF Open-Drain + Pull-up
Penjelasan DutyCycle: 2 = Tlow/Thigh = 2:1 (Fast mode), 16/9 untuk duty cycle alternatif
```

## Slide 27: STM32 I2C — HAL API Functions
```
Buatkan slide dengan judul "STM32 HAL I2C API" berisi tabel:

| Fungsi | Kegunaan |
|--------|----------|
| HAL_I2C_Master_Transmit() | Kirim data ke slave (polling) |
| HAL_I2C_Master_Receive() | Terima data dari slave (polling) |
| HAL_I2C_Mem_Write() | Tulis ke register slave (polling) |
| HAL_I2C_Mem_Read() | Baca register slave (polling) |
| HAL_I2C_IsDeviceReady() | Cek apakah device merespon |
| HAL_I2C_Master_Transmit_IT() | Transmit (interrupt mode) |
| HAL_I2C_Master_Transmit_DMA() | Transmit (DMA mode) |

Highlight: Mem_Read/Write lebih mudah untuk sensor (otomatis set register pointer)
PENTING: Address parameter = 7-bit address << 1
```

## Slide 28: STM32 I2C — Contoh Read Sensor
```
Buatkan slide dengan judul "Membaca Sensor BMP280 (STM32 HAL)" berisi kode:

#define BMP280_ADDR    (0x76 << 1)  // 7-bit → 8-bit
#define BMP280_REG_ID  0xD0

uint8_t chip_id;
HAL_StatusTypeDef ret;

// Baca Chip ID
ret = HAL_I2C_Mem_Read(&hi2c1, BMP280_ADDR, BMP280_REG_ID,
                       I2C_MEMADD_SIZE_8BIT, &chip_id, 1, 1000);
if (ret == HAL_OK) {
    printf("BMP280 Chip ID: 0x%02X\n", chip_id);  // Expect 0x58
}

// Baca 6 byte data (temp + pressure)
uint8_t data[6];
ret = HAL_I2C_Mem_Read(&hi2c1, BMP280_ADDR, 0xF7,
                       I2C_MEMADD_SIZE_8BIT, data, 6, 1000);

Bandingkan dengan ESP-IDF: HAL lebih ringkas (1 fungsi vs command link)
```

## Slide 29: I2C Scanner — Implementasi
```
Buatkan slide dengan judul "I2C Bus Scanner" berisi:
- Tujuan: Deteksi semua device yang terhubung ke bus I2C
- Algoritma: Kirim address 0x01-0x7F, cek ACK

ESP-IDF:
for (addr = 1; addr < 127; addr++) {
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1), true);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(...);
    if (ret == ESP_OK) printf("Found: 0x%02X\n", addr);
}

STM32 HAL:
for (addr = 1; addr < 127; addr++) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, addr<<1, 3, 100) == HAL_OK)
        printf("Found: 0x%02X\n", addr);
}

Output contoh: Found devices at 0x23, 0x3C, 0x57, 0x68, 0x76
```

## Slide 30: Interfacing BMP280
```
Buatkan slide dengan judul "Sensor BMP280: Suhu & Tekanan" berisi:
- Spesifikasi: Suhu (-40 to +85°C, ±1°C), Tekanan (300-1100 hPa, ±1 hPa)
- Interface: I2C (0x76/0x77) atau SPI
- Register penting:
  - 0xD0: Chip ID (0x58 = BMP280)
  - 0xF4: Control (mode, oversampling)
  - 0xF7-0xFC: Raw data (pressure[2:0], temp[2:0])
  - 0x88-0xA1: Calibration data (26 bytes)
- Proses pembacaan:
  1. Baca calibration data (sekali saat init)
  2. Set mode & oversampling
  3. Baca raw data
  4. Compensate dengan formula dari datasheet
Diagram: Register map BMP280
```

## Slide 31: Interfacing BH1750
```
Buatkan slide dengan judul "Sensor BH1750: Intensitas Cahaya" berisi:
- Spesifikasi: 1-65535 lux, resolusi 1 lux, response time 120ms
- Interface: I2C, address 0x23 (ADDR=LOW) atau 0x5C (ADDR=HIGH)
- Mode operasi:
  - Continuously High Res Mode (0x10): 1 lux, 120ms
  - Continuously High Res Mode 2 (0x11): 0.5 lux, 120ms
  - One-time High Res Mode (0x20): 1 lux, 120ms, auto power down
- Pembacaan sangat sederhana:
  1. Kirim mode command (1 byte)
  2. Tunggu 120ms
  3. Baca 2 byte → lux = (MSB << 8 | LSB) / 1.2
- Tidak memerlukan calibration!
Diagram: Timing diagram BH1750
```

## Slide 32: Interfacing DS3231 RTC
```
Buatkan slide dengan judul "DS3231: Real-Time Clock" berisi:
- Spesifikasi: Accuracy ±2 ppm (±1 min/year), -40 to +85°C
- Address: 0x68 (fixed)
- Register map:
  - 0x00: Seconds (BCD format)
  - 0x01: Minutes
  - 0x02: Hours (12/24H)
  - 0x03: Day of week
  - 0x04: Date
  - 0x05: Month
  - 0x06: Year
  - 0x11: Temperature MSB
- Data dalam format BCD! Perlu konversi:
  - BCD→Dec: (val >> 4) * 10 + (val & 0x0F)
  - Dec→BCD: ((val / 10) << 4) | (val % 10)
- Baterai CR2032 untuk backup
```

## Slide 33: Interfacing SSD1306 OLED
```
Buatkan slide dengan judul "SSD1306: OLED Display 128×64" berisi:
- Spesifikasi: 128×64 pixels, monochrome, I2C (0x3C/0x3D)
- Memory: 128×64/8 = 1024 bytes GDDRAM (Graphic Display Data RAM)
- Dua jenis byte yang dikirim:
  - Command byte: Control byte = 0x00, lalu command
  - Data byte: Control byte = 0x40, lalu pixel data
- Init sequence (wajib):
  1. Display OFF (0xAE)
  2. Set clock div (0xD5, 0x80)
  3. Set multiplex (0xA8, 0x3F)
  4. Set display offset (0xD3, 0x00)
  5. Set start line (0x40)
  6. Charge pump (0x8D, 0x14)
  7. Set addressing mode (0x20, 0x00)
  8. Display ON (0xAF)
- Library: Biasanya pakai font table 5×7 untuk text
```

## Slide 34: Interfacing AT24C32 EEPROM
```
Buatkan slide dengan judul "AT24C32: I2C EEPROM (4KB)" berisi:
- Kapasitas: 32Kbit = 4096 bytes
- Address: 0x50-0x57 (3 address pin A0-A2)
- Write: Max 32 bytes per page (page write)
- Write cycle time: 5-10ms (HARUS tunggu sebelum write berikutnya!)
- Address: 2 byte (high + low) untuk 12-bit address space
- Random read: Write 2-byte address → Repeated START → Read
- Sequential read: Baca berturut-turut, address auto-increment

Write sequence:
[START][ADDR+W][ACK][AddrH][ACK][AddrL][ACK][Data0]...[DataN][ACK][STOP]
→ WAIT 5ms (write cycle)

Read sequence:
[START][ADDR+W][ACK][AddrH][ACK][AddrL][ACK][RSTART][ADDR+R][ACK][Data][NACK][STOP]

PENTING: Jangan write cross page boundary! Alamat wrap-around dalam page 32-byte.
```

## Slide 35: MPU6050 IMU (Bonus Sensor)
```
Buatkan slide dengan judul "MPU6050: 6-Axis IMU" berisi:
- Accelerometer: ±2g, ±4g, ±8g, ±16g
- Gyroscope: ±250, ±500, ±1000, ±2000 °/s
- Address: 0x68 (AD0=LOW) atau 0x69 (AD0=HIGH)
- Register penting:
  - 0x75: WHO_AM_I (0x68)
  - 0x6B: Power Management (write 0x00 untuk wake up)
  - 0x3B-0x40: Accel X,Y,Z (6 bytes, 16-bit signed)
  - 0x41-0x42: Temperature (2 bytes)
  - 0x43-0x48: Gyro X,Y,Z (6 bytes, 16-bit signed)
- Konversi: accel_g = raw * scale_factor / 32768.0
- Scale factor: 2g→16384, 4g→8192, 8g→4096, 16g→2048
```

## Slide 36: Multi-Device I2C Bus
```
Buatkan slide dengan judul "Mengelola Multiple Device pada Satu Bus" berisi:
- Strategi:
  1. Scan bus saat init → verifikasi semua device present
  2. Buat abstraction layer per device (struct + function pointer)
  3. Sequential polling: Baca sensor satu per satu
  4. Error handling per device (satu device fail ≠ semua fail)

- Masalah address conflict:
  - DS3231 (0x68) vs MPU6050 (0x68) → gunakan I2C multiplexer
  - TCA9548A: 8-channel I2C mux, address 0x70-0x77
  - Atau gunakan 2 I2C bus (ESP32 punya I2C0 dan I2C1)

- Tips:
  - Jangan baca terlalu cepat (kasih delay antar device)
  - Gunakan mutex jika multi-thread (FreeRTOS)
  - Timeout per transaction (jangan block forever)
```

## Slide 37: Error Handling Implementasi
```
Buatkan slide dengan judul "Error Handling I2C — Best Practices" berisi kode perbandingan:

ESP-IDF:
esp_err_t ret = i2c_master_cmd_begin(...);
switch(ret) {
    case ESP_OK: /* success */ break;
    case ESP_ERR_TIMEOUT: /* bus busy/hang */ break;
    case ESP_FAIL: /* NACK received */ break;
}

STM32 HAL:
HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(...);
switch(ret) {
    case HAL_OK: /* success */ break;
    case HAL_ERROR: /* error */ break;
    case HAL_BUSY: /* bus busy */ break;
    case HAL_TIMEOUT: /* timeout */ break;
}

Recovery strategy:
1. Retry 3x dengan delay
2. Bus reset (toggle SCL 9x)
3. Re-init I2C peripheral
4. Log error untuk debugging
```

## Slide 38: Daftar Percobaan Praktikum
```
Buatkan slide dengan judul "12 Percobaan Praktikum I2C" berisi tabel:

| No | Judul | Sensor/Device |
|----|-------|---------------|
| 01 | I2C Scanner | Bus scan all address |
| 02 | I2C Temp BMP280 | BMP280 temperature |
| 03 | I2C Pressure BMP280 | BMP280 pressure |
| 04 | I2C IMU MPU6050 | MPU6050 accel+gyro |
| 05 | I2C OLED Display | SSD1306 128×64 |
| 06 | I2C EEPROM | AT24C32/AT24C256 |
| 07 | I2C Light BH1750 | BH1750 lux sensor |
| 08 | I2C RTC DS3231 | DS3231 date/time |
| 09 | I2C IO Expander | PCF8574 GPIO |
| 10 | I2C Multi Sensor | Multiple devices |
| 11 | I2C OLED Animation | SSD1306 graphics |
| 12 | I2C Error Recovery | Error handling |

Setiap percobaan tersedia versi ESP32 dan STM32!
```

## Slide 39: Python Debug & Analysis
```
Buatkan slide dengan judul "Python Tool untuk Analisis I2C" berisi:
- Setiap percobaan dilengkapi debug_analysis.py
- Fitur:
  1. Serial parser: Parse output sensor data
  2. Realtime plot: Matplotlib animated graph
  3. Data logging: Export ke CSV
  4. Statistics: Min, Max, Average, StdDev
  5. Error tracking: Catat I2C error rate

Contoh penggunaan:
$ python debug_analysis.py --port /dev/ttyUSB0 --baud 115200

Output: Grafik realtime suhu + tekanan + cahaya
Export: sensor_data_2024-01-15.csv

Screenshot: Contoh plot 3-subplot (temp, pressure, light)
```

## Slide 40: Kesimpulan & Tips
```
Buatkan slide penutup dengan judul "Kesimpulan & Tips I2C" berisi:

✅ I2C adalah protokol paling populer untuk sensor
✅ Hanya 2 pin: SDA + SCL (hemat GPIO!)
✅ Multi-device pada satu bus (up to 112 address)
✅ WAJIB pull-up resistor 4.7kΩ
✅ Address 7-bit: hati-hati shift di STM32 HAL
✅ Error handling = KUNCI reliability

⚠️ Common Mistakes:
1. Lupa pull-up resistor → bus tidak jalan
2. Wrong address (lupa shift atau salah datasheet)
3. Tidak handle NACK → program hang
4. Write EEPROM tanpa delay write cycle
5. Konflik address (DS3231 vs MPU6050)

📌 "Debugging I2C: Logic Analyzer adalah sahabat terbaik Anda"
```

---

*Modul 06 — Praktikum Sistem Embedded*
*PPT Prompts Bagian 2: Implementasi I2C pada ESP32 & STM32*
