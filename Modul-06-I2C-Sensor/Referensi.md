# 📚 Referensi Modul 06: I2C & Sensor

## 📖 Referensi Utama

### 1. Dokumentasi Resmi ESP-IDF
| No | Judul | URL | Topik |
|----|-------|-----|-------|
| 1 | ESP-IDF I2C Driver | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html | I2C master/slave driver |
| 2 | ESP-IDF I2C Example | https://github.com/espressif/esp-idf/tree/master/examples/peripherals/i2c | Contoh implementasi |
| 3 | ESP32 Technical Reference | https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf | Ch.11: I2C Controller |
| 4 | ESP32-S2 I2C | https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/api-reference/peripherals/i2c.html | I2C pada ESP32-S2 |
| 5 | ESP32-S3 I2C | https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2c.html | I2C pada ESP32-S3 |

### 2. Dokumentasi STM32 HAL
| No | Judul | URL | Topik |
|----|-------|-----|-------|
| 1 | STM32F1 HAL I2C | https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf | Ch.26: I2C Interface |
| 2 | STM32F4 HAL I2C | https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf | Ch.18: I2C Interface |
| 3 | HAL I2C API | https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers-stmicroelectronics.pdf | HAL_I2C functions |
| 4 | AN4235: I2C Timing Config | https://www.st.com/resource/en/application_note/an4235-i2c-timing-configuration-tool-for-stm32f3xxxx-and-stm32f0xxxx-microcontrollers-stmicroelectronics.pdf | I2C timing setup |

### 3. Buku Teks
| No | Judul | Penulis | Bab/Halaman |
|----|-------|---------|-------------|
| 1 | Mastering STM32 (2nd Ed.) | Carmine Noviello | Ch.14: I2C |
| 2 | Kolban's Book on ESP32 | Neil Kolban | p.269-274: I2C |
| 3 | The Definitive Guide to ARM Cortex-M3/M4 | Joseph Yiu | Ch.15: Communication |
| 4 | Embedded Systems with ARM Cortex-M | Yifeng Zhu | Ch.22: I2C Protocol |

---

## 📋 Datasheet Sensor & Modul

### Sensor Suhu & Tekanan
| No | Komponen | Datasheet | Address | Keterangan |
|----|----------|-----------|---------|------------|
| 1 | BMP280 | https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf | 0x76/0x77 | Temp + Pressure, 16-20 bit |
| 2 | BME280 | https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf | 0x76/0x77 | + Humidity (alternatif) |
| 3 | SHT31 | https://sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_SHT3x_DIS.pdf | 0x44/0x45 | High-accuracy temp+humidity |

### Sensor Cahaya
| No | Komponen | Datasheet | Address | Keterangan |
|----|----------|-----------|---------|------------|
| 1 | BH1750 (GY-302) | https://www.mouser.com/datasheet/2/348/bh1750fvi-e-186247.pdf | 0x23/0x5C | Digital ambient light, 1-65535 lux |

### Sensor Gerak (IMU)
| No | Komponen | Datasheet | Address | Keterangan |
|----|----------|-----------|---------|------------|
| 1 | MPU6050 | https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf | 0x68/0x69 | Accel + Gyro 6-axis |
| 2 | MPU9250 | https://invensense.tdk.com/wp-content/uploads/2015/02/PS-MPU-9250A-01-v1.1.pdf | 0x68/0x69 | 9-axis IMU (+ magnetometer) |

### RTC (Real-Time Clock)
| No | Komponen | Datasheet | Address | Keterangan |
|----|----------|-----------|---------|------------|
| 1 | DS3231 | https://datasheets.maximintegrated.com/en/ds/DS3231.pdf | 0x68 | High-precision RTC ±2ppm |
| 2 | PCF8563 | https://www.nxp.com/docs/en/data-sheet/PCF8563.pdf | 0x51 | Low-power RTC (alternatif) |

### EEPROM
| No | Komponen | Datasheet | Address | Keterangan |
|----|----------|-----------|---------|------------|
| 1 | AT24C32 | https://ww1.microchip.com/downloads/en/DeviceDoc/doc0336.pdf | 0x50-0x57 | 32Kbit (4KB), 32-byte page |
| 2 | AT24C256 | https://ww1.microchip.com/downloads/en/DeviceDoc/AT24C256C-I2C-Compatible-Two-Wire-Serial-EEPROM-20006066A.pdf | 0x50-0x57 | 256Kbit (32KB) |

### Display OLED
| No | Komponen | Datasheet | Address | Keterangan |
|----|----------|-----------|---------|------------|
| 1 | SSD1306 | https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf | 0x3C/0x3D | 128×64 monochrome OLED |
| 2 | SH1106 | https://www.velleman.eu/downloads/29/infosheets/sh1106_datasheet.pdf | 0x3C/0x3D | 128×64 (alternatif, page mode) |

### I/O Expander
| No | Komponen | Datasheet | Address | Keterangan |
|----|----------|-----------|---------|------------|
| 1 | PCF8574 | https://www.ti.com/lit/ds/symlink/pcf8574.pdf | 0x20-0x27 | 8-bit I/O expander |
| 2 | MCP23017 | https://ww1.microchip.com/downloads/en/devicedoc/20001952c.pdf | 0x20-0x27 | 16-bit I/O expander |

---

## 📐 Spesifikasi Protokol I2C

### Dokumen Standar
| No | Judul | URL | Keterangan |
|----|-------|-----|------------|
| 1 | I2C-bus Specification (NXP) | https://www.nxp.com/docs/en/user-guide/UM10204.pdf | Standar resmi I2C rev.7 |
| 2 | SMBus Specification | http://smbus.org/specs/ | System Management Bus |

### Parameter Electrical I2C

| Parameter | Standard Mode | Fast Mode | Fast Mode Plus |
|-----------|:------------:|:---------:|:--------------:|
| Clock Speed | 100 kHz | 400 kHz | 1 MHz |
| Rise Time (max) | 1000 ns | 300 ns | 120 ns |
| Fall Time (max) | 300 ns | 300 ns | 120 ns |
| Pull-up (typical) | 10 kΩ | 4.7 kΩ | 2.2 kΩ |
| Bus Capacitance (max) | 400 pF | 400 pF | 550 pF |

---

## 🔗 Tutorial & Artikel Pendukung

### ESP32 + I2C
| No | Judul | URL |
|----|-------|-----|
| 1 | ESP32 I2C Communication Tutorial | https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/ |
| 2 | ESP32 BMP280 Tutorial | https://randomnerdtutorials.com/esp32-bmp280-arduino-ide-pressure-temperature/ |
| 3 | ESP32 SSD1306 OLED Guide | https://randomnerdtutorials.com/esp32-ssd1306-oled-display-arduino-ide/ |
| 4 | ESP32 MPU6050 Tutorial | https://randomnerdtutorials.com/esp32-mpu-6050-accelerometer-gyroscope-arduino/ |

### STM32 + I2C
| No | Judul | URL |
|----|-------|-----|
| 1 | STM32 I2C HAL Tutorial | https://deepbluembedded.com/stm32-i2c-tutorial-hal-examples-slave-dma/ |
| 2 | STM32 BMP280 HAL | https://controllerstech.com/stm32-i2c-configuration-using-registers/ |
| 3 | STM32 OLED SSD1306 | https://controllerstech.com/oled-display-using-i2c-stm32/ |
| 4 | Mastering STM32 I2C | https://controllerstech.com/stm32-i2c-configuration-using-registers/ |

### Python Serial & Analisis Data
| No | Judul | URL |
|----|-------|-----|
| 1 | PySerial Documentation | https://pyserial.readthedocs.io/en/latest/ |
| 2 | Matplotlib Real-time Plot | https://matplotlib.org/stable/api/animation_api.html |
| 3 | Python CSV Module | https://docs.python.org/3/library/csv.html |

---

## 🛠️ Tools & Software

### Development
| Tool | Fungsi | URL |
|------|--------|-----|
| PlatformIO | Build system | https://platformio.org/ |
| STM32CubeMX | STM32 config generator | https://www.st.com/stm32cubemx |
| ESP-IDF Extension | VS Code extension | https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension |

### Debug & Analysis
| Tool | Fungsi | URL |
|------|--------|-----|
| Logic Analyzer (Saleae) | Decode I2C bus traffic | https://www.saleae.com/ |
| PulseView (Sigrok) | Open-source logic analyzer | https://sigrok.org/wiki/PulseView |
| I2C Scanner | Detect devices on bus | Built-in (Percobaan 01) |
| i2c-tools (Linux) | CLI I2C debug | `apt install i2c-tools` |

---

## 📊 Tabel Address I2C Umum

| Address (7-bit) | Device | Keterangan |
|:----------------:|--------|------------|
| 0x20-0x27 | PCF8574 / MCP23017 | I/O Expander |
| 0x23, 0x5C | BH1750 | Light Sensor |
| 0x3C, 0x3D | SSD1306 / SH1106 | OLED Display |
| 0x44, 0x45 | SHT31 | Temp/Humidity |
| 0x48-0x4F | ADS1115 | ADC 16-bit |
| 0x50-0x57 | AT24Cxx | EEPROM |
| 0x68 | DS3231 / MPU6050 | RTC / IMU |
| 0x69 | MPU6050 (AD0=H) | IMU alternate |
| 0x76, 0x77 | BMP280 / BME280 | Temp/Pressure |

> ⚠️ **Konflik Address**: DS3231 (0x68) dan MPU6050 (0x68) tidak bisa digunakan bersamaan tanpa I2C multiplexer (TCA9548A).

---

*Modul 06 — Praktikum Sistem Embedded*
*Daftar Referensi I2C & Sensor*
