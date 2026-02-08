# Modul 06: Komunikasi Cerdas Antar-Chip — I2C Bus & Sensor Integration

## 🎯 Capaian Pembelajaran

Setelah menyelesaikan bab ini, mahasiswa diharapkan mampu:

1. Memahami prinsip kerja protokol I2C (Inter-Integrated Circuit)
2. Mengidentifikasi karakteristik sinyal SDA dan SCL pada I2C
3. Mengkonfigurasi I2C pada STM32 dan ESP32 sebagai Master
4. Membaca data dari berbagai sensor I2C (BME280, DS3231, OLED SSD1306)
5. Mengakses EEPROM eksternal melalui I2C
6. Menangani multiple device pada I2C bus
7. Melakukan troubleshooting masalah komunikasi I2C

---

## 📚 Materi Pembelajaran

### 1. Pendahuluan I2C

#### 1.1 Sejarah dan Latar Belakang

I2C (Inter-Integrated Circuit) dikembangkan oleh Philips Semiconductor (sekarang NXP) pada tahun 1982. Protokol ini dirancang untuk komunikasi antar chip dalam satu PCB dengan kebutuhan pin minimal.

**Karakteristik Utama I2C:**
- Hanya membutuhkan 2 wire: SDA (Data) dan SCL (Clock)
- Multi-master dan multi-slave capable
- Addressing system untuk membedakan device
- Kecepatan hingga 3.4 Mbps (High Speed Mode)
- Open-drain output dengan pull-up resistor

#### 1.2 Perbandingan dengan Protokol Lain

| Fitur | I2C | SPI | UART |
|-------|-----|-----|------|
| **Jumlah Wire** | 2 (+ GND) | 4+ | 2 (+ GND) |
| **Topology** | Bus | Point-to-point/Bus | Point-to-point |
| **Max Device** | 127 (7-bit addr) | Unlimited (CS pins) | 1 |
| **Duplex** | Half | Full | Full |
| **Max Speed** | 3.4 Mbps | 10+ Mbps | ~1 Mbps |
| **Complexity** | Medium | Low | Low |

### 2. Teori Dasar I2C

#### 2.1 Arsitektur I2C Bus

```
                    Vcc (3.3V or 5V)
                        │
                  ┌─────┴─────┐
                 Rp          Rp     (Pull-up Resistors: 4.7kΩ typical)
                  │           │
        ┌─────────┼───────────┼─────────┐
        │         │           │         │
   ┌────┴────┐┌───┴───┐  ┌────┴────┐┌───┴───┐
   │  Master ││Slave 1│  │ Slave 2 ││Slave 3│
   │  (MCU)  ││(Sensor)│ │ (EEPROM)││ (RTC) │
   └────┬────┘└───┬───┘  └────┬────┘└───┬───┘
        │         │           │         │
        └─────────┴───────────┴─────────┘
                        │
                       GND

        SDA ─────────────────────────────
        SCL ─────────────────────────────
```

#### 2.2 Sinyal I2C

**SDA (Serial Data):**
- Bidirectional data line
- Open-drain output
- Data valid saat SCL HIGH
- Dapat berubah saat SCL LOW

**SCL (Serial Clock):**
- Clock line dari Master
- Open-drain output
- Menentukan timing transfer data
- Clock stretching supported

#### 2.3 Timing Diagram

```
START Condition:
        ┌───────────────────────
SDA ────┘         
            ┌───────────────────
SCL ────────┘

Data Transfer (1 bit):
        ┌───────┐       ┌───────┐
SDA ────┘  D7   └───────┘  D6   └───
            ┌───┐           ┌───┐
SCL ────────┘   └───────────┘   └───
        │   │   │       │   │   │
        Setup  Hold     Setup  Hold

STOP Condition:
                    ┌───────────────
SDA ────────────────┘
        ────────────────────────────
SCL 

Repeated START:
            ┌───────┐
SDA ────────┘       └───
        ────────────────┐
SCL             ┌───────┘
```

#### 2.4 Frame Format

**7-bit Address Mode:**
```
┌─────┬───────────────────┬─────┬─────┬────────────────┬─────┬──────┐
│START│    7-bit Address  │ R/W │ ACK │   8-bit Data   │ ACK │ STOP │
│  1  │ A6 A5 A4 A3 A2 A1 A0│  1  │  1  │ D7...D0       │  1  │  1   │
└─────┴───────────────────┴─────┴─────┴────────────────┴─────┴──────┘
```

**R/W Bit:**
- 0 = Write (Master → Slave)
- 1 = Read (Slave → Master)

**ACK/NACK:**
- ACK (Acknowledge): SDA LOW saat SCL pulse ke-9
- NACK (Not Acknowledge): SDA HIGH saat SCL pulse ke-9

#### 2.5 I2C Speed Modes

| Mode | Speed | Aplikasi |
|------|-------|----------|
| Standard Mode | 100 kbps | Sensor, EEPROM |
| Fast Mode | 400 kbps | Display, IMU |
| Fast Mode Plus | 1 Mbps | High-speed sensors |
| High Speed | 3.4 Mbps | Special applications |

### 3. I2C pada STM32F103

#### 3.1 Fitur I2C STM32F103

- 2 I2C peripheral (I2C1, I2C2)
- 7-bit dan 10-bit addressing
- Multi-master capable
- DMA support
- SMBus compatible
- Clock stretching support

**Pin Mapping:**

| Peripheral | SCL | SDA | Remap SCL | Remap SDA |
|------------|-----|-----|-----------|-----------|
| I2C1 | PB6 | PB7 | PB8 | PB9 |
| I2C2 | PB10 | PB11 | - | - |

#### 3.2 Konfigurasi I2C STM32 (HAL)

```c
I2C_HandleTypeDef hi2c1;

void I2C1_Init(void) {
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // Configure GPIO: PB6 (SCL), PB7 (SDA)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;      // Open-drain
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // Configure I2C
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;              // 400 kHz
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}
```

#### 3.3 Operasi I2C STM32

**Scan I2C Bus:**
```c
void I2C_Scan(void) {
    printf("Scanning I2C bus...\n");
    
    for (uint8_t addr = 1; addr < 128; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK) {
            printf("Device found at 0x%02X\n", addr);
        }
    }
}
```

**Write Data:**
```c
HAL_StatusTypeDef I2C_Write(uint8_t dev_addr, uint8_t reg, uint8_t* data, uint16_t len) {
    return HAL_I2C_Mem_Write(&hi2c1, dev_addr << 1, reg, 
                             I2C_MEMADD_SIZE_8BIT, data, len, 100);
}
```

**Read Data:**
```c
HAL_StatusTypeDef I2C_Read(uint8_t dev_addr, uint8_t reg, uint8_t* data, uint16_t len) {
    return HAL_I2C_Mem_Read(&hi2c1, dev_addr << 1, reg,
                            I2C_MEMADD_SIZE_8BIT, data, len, 100);
}
```

### 4. I2C pada ESP32

#### 4.1 Fitur I2C ESP32

- 2 I2C controller (I2C_NUM_0, I2C_NUM_1)
- Flexible GPIO mapping (any GPIO)
- 7-bit dan 10-bit addressing
- Master dan Slave mode
- Clock stretching support
- Arbitration support

#### 4.2 Konfigurasi I2C ESP32 (Arduino)

```cpp
#include <Wire.h>

void setup() {
    // Default: SDA=21, SCL=22
    Wire.begin();
    
    // Or specify pins
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // Set clock frequency
    Wire.setClock(400000);  // 400 kHz
}
```

#### 4.3 I2C ESP32 dengan ESP-IDF

```cpp
#include <driver/i2c.h>

void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21,
        .scl_io_num = 22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000
    };
    
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}
```

### 5. Sensor I2C Populer

#### 5.1 BME280 - Temperature, Humidity, Pressure Sensor

**Spesifikasi:**
| Parameter | Range | Accuracy |
|-----------|-------|----------|
| Temperature | -40°C to +85°C | ±1.0°C |
| Humidity | 0-100% RH | ±3% RH |
| Pressure | 300-1100 hPa | ±1 hPa |

**I2C Address:** 0x76 atau 0x77 (tergantung SDO pin)

**Register Map:**
```
0xD0 - Chip ID (should read 0x60 for BME280)
0xF2 - ctrl_hum (humidity control)
0xF4 - ctrl_meas (temperature & pressure control)
0xF5 - config (rate, filter, interface)
0xF7-0xFE - Data registers (pressure, temperature, humidity)
```

**Kode Pembacaan BME280:**
```cpp
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

void setup() {
    Serial.begin(115200);
    
    if (!bme.begin(0x76)) {
        Serial.println("BME280 not found!");
        while(1);
    }
    
    // Configure oversampling
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                    Adafruit_BME280::SAMPLING_X16,  // temp
                    Adafruit_BME280::SAMPLING_X16,  // pressure
                    Adafruit_BME280::SAMPLING_X16,  // humidity
                    Adafruit_BME280::FILTER_X16,
                    Adafruit_BME280::STANDBY_MS_0_5);
}

void loop() {
    float temp = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;  // hPa
    float altitude = bme.readAltitude(1013.25);    // Sea level pressure
    
    Serial.printf("Temp: %.2f°C, Hum: %.2f%%, Press: %.2f hPa, Alt: %.2f m\n",
                  temp, humidity, pressure, altitude);
    delay(1000);
}
```

#### 5.2 SSD1306 - OLED Display

**Spesifikasi:**
- Resolusi: 128×64 atau 128×32 pixels
- Monochrome
- I2C Address: 0x3C atau 0x3D
- Voltage: 3.3V atau 5V

**Kode OLED SSD1306:**
```cpp
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        while(1);
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Hello World!");
    display.display();
}
```

#### 5.3 DS3231 - Real-Time Clock

**Spesifikasi:**
- Accuracy: ±2ppm (±1 min/year)
- Battery backup (CR2032)
- Temperature compensated crystal
- I2C Address: 0x68 (fixed)

**Register Map:**
```
0x00 - Seconds (BCD)
0x01 - Minutes (BCD)
0x02 - Hours (BCD)
0x03 - Day of Week
0x04 - Date (BCD)
0x05 - Month (BCD)
0x06 - Year (BCD)
0x07-0x0D - Alarms
0x0E - Control
0x0F - Status
0x11 - Temperature (MSB)
```

**Kode DS3231:**
```cpp
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
    Serial.begin(115200);
    
    if (!rtc.begin()) {
        Serial.println("RTC not found!");
        while(1);
    }
    
    // Set time if lost power
    if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

void loop() {
    DateTime now = rtc.now();
    
    Serial.printf("%04d/%02d/%02d %02d:%02d:%02d\n",
                  now.year(), now.month(), now.day(),
                  now.hour(), now.minute(), now.second());
    
    // Read temperature from DS3231
    float temp = rtc.getTemperature();
    Serial.printf("RTC Temp: %.2f°C\n", temp);
    
    delay(1000);
}
```

#### 5.4 24LC256 - I2C EEPROM

**Spesifikasi:**
- Capacity: 256 Kbit (32KB)
- Page size: 64 bytes
- I2C Address: 0x50-0x57 (A0, A1, A2 pins)
- Write cycle: 5ms max

**Kode EEPROM 24LC256:**
```cpp
#include <Wire.h>

#define EEPROM_ADDR 0x50

void EEPROM_WriteByte(uint16_t addr, uint8_t data) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)(addr >> 8));    // MSB
    Wire.write((uint8_t)(addr & 0xFF));  // LSB
    Wire.write(data);
    Wire.endTransmission();
    delay(5);  // Wait for write cycle
}

uint8_t EEPROM_ReadByte(uint16_t addr) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr & 0xFF));
    Wire.endTransmission();
    
    Wire.requestFrom(EEPROM_ADDR, 1);
    return Wire.read();
}

void EEPROM_WritePage(uint16_t addr, uint8_t* data, uint8_t len) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr & 0xFF));
    Wire.write(data, len);
    Wire.endTransmission();
    delay(5);
}
```

### 6. Multi-Device I2C

#### 6.1 Scanning Multiple Devices

```cpp
void scanI2CDevices() {
    Serial.println("I2C Scanner");
    Serial.println("Scanning...");
    
    int devices = 0;
    
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.printf("Device found at 0x%02X", addr);
            
            // Identify known devices
            switch (addr) {
                case 0x3C:
                case 0x3D: Serial.print(" (SSD1306 OLED)"); break;
                case 0x50: Serial.print(" (24LC EEPROM)"); break;
                case 0x68: Serial.print(" (DS3231 RTC)"); break;
                case 0x76:
                case 0x77: Serial.print(" (BME280)"); break;
            }
            Serial.println();
            devices++;
        }
    }
    
    Serial.printf("Found %d device(s)\n", devices);
}
```

#### 6.2 Integrated System Example

```cpp
// Multiple I2C devices working together
void updateDisplay() {
    // Read sensor data
    float temp = bme.readTemperature();
    float humidity = bme.readHumidity();
    DateTime now = rtc.now();
    
    // Update OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("Time: %02d:%02d:%02d", 
                   now.hour(), now.minute(), now.second());
    display.setCursor(0, 16);
    display.printf("Temp: %.1fC", temp);
    display.setCursor(0, 32);
    display.printf("Hum: %.1f%%", humidity);
    display.display();
    
    // Log to EEPROM
    static uint16_t logAddr = 0;
    EEPROM_WriteByte(logAddr++, (uint8_t)temp);
    if (logAddr >= 32768) logAddr = 0;  // Wrap around
}
```

### 7. I2C Bus Recovery

#### 7.1 Common I2C Problems

| Problem | Symptom | Cause | Solution |
|---------|---------|-------|----------|
| SDA stuck LOW | No communication | Slave holding SDA | Bus recovery |
| No ACK | NACK received | Wrong address/device unpowered | Check connections |
| Timeout | Operation hangs | Clock stretching too long | Increase timeout |
| Data corruption | Wrong data | EMI, wrong pull-up | Add filtering |

#### 7.2 Bus Recovery Algorithm

```cpp
void i2c_bus_recovery(int sda_pin, int scl_pin) {
    pinMode(sda_pin, INPUT_PULLUP);
    pinMode(scl_pin, OUTPUT);
    
    // Generate 9 clock pulses to release SDA
    for (int i = 0; i < 9; i++) {
        digitalWrite(scl_pin, LOW);
        delayMicroseconds(5);
        digitalWrite(scl_pin, HIGH);
        delayMicroseconds(5);
        
        // Check if SDA is released
        if (digitalRead(sda_pin) == HIGH) break;
    }
    
    // Generate STOP condition
    pinMode(sda_pin, OUTPUT);
    digitalWrite(sda_pin, LOW);
    delayMicroseconds(5);
    digitalWrite(scl_pin, HIGH);
    delayMicroseconds(5);
    digitalWrite(sda_pin, HIGH);
    delayMicroseconds(5);
    
    // Reinitialize I2C
    Wire.begin(sda_pin, scl_pin);
}
```

### 8. Pull-up Resistor Calculation

#### 8.1 Calculation Formula

```
Rp_min = (Vcc - Vol) / Iol
Rp_max = tr / (0.8473 × Cb)

Where:
- Vol = 0.4V (max low level output)
- Iol = 3mA (max sink current)
- tr = rise time (300ns for Standard, 100ns for Fast)
- Cb = bus capacitance (pF)
```

#### 8.2 Recommended Values

| Mode | Bus Cap | Rp Value |
|------|---------|----------|
| Standard (100kHz) | <100pF | 4.7kΩ |
| Standard (100kHz) | <200pF | 2.2kΩ |
| Fast (400kHz) | <100pF | 2.2kΩ |
| Fast (400kHz) | <200pF | 1kΩ |

### 9. Best Practices

#### 9.1 Hardware Design
- Use appropriate pull-up resistors
- Keep I2C traces short (<30cm)
- Use decoupling capacitors near devices
- Consider separate I2C buses for high-speed and low-speed devices

#### 9.2 Software Design
- Always check return values
- Implement timeout handling
- Use bus recovery mechanism
- Validate data with CRC when available

#### 9.3 Debugging Tips
```cpp
// Debug wrapper for I2C operations
HAL_StatusTypeDef I2C_Debug_Write(uint8_t addr, uint8_t reg, uint8_t* data, uint16_t len) {
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1, addr << 1, reg,
                                                  I2C_MEMADD_SIZE_8BIT, 
                                                  data, len, 100);
    if (status != HAL_OK) {
        printf("I2C Write Error: addr=0x%02X, reg=0x%02X, status=%d\n",
               addr, reg, status);
        printf("Error code: 0x%08X\n", hi2c1.ErrorCode);
    }
    return status;
}
```

### 10. Rangkuman

1. **I2C** adalah protokol serial synchronous dengan 2 wire (SDA, SCL)
2. **Addressing** 7-bit memungkinkan hingga 127 device pada satu bus
3. **STM32F103** memiliki 2 I2C peripheral dengan DMA support
4. **ESP32** memiliki 2 I2C controller dengan flexible GPIO mapping
5. **Sensor populer**: BME280 (environment), DS3231 (RTC), SSD1306 (OLED)
6. **Pull-up resistors** penting untuk integritas sinyal
7. **Bus recovery** diperlukan untuk menangani kondisi error

---

## 📊 Diagram Perbandingan Platform

```
┌────────────────────────────────────────────────────────────────┐
│                    I2C Comparison                               │
├──────────────────────┬─────────────────────┬───────────────────┤
│      Feature         │     STM32F103       │      ESP32        │
├──────────────────────┼─────────────────────┼───────────────────┤
│ I2C Peripherals      │         2           │        2          │
│ Default Pins         │   PB6/7, PB10/11    │   GPIO21/22       │
│ Pin Remapping        │    Limited          │    Full flexible  │
│ Max Speed            │    400kHz (std)     │    1MHz           │
│ DMA Support          │        Yes          │       Yes         │
│ Clock Stretching     │        Yes          │       Yes         │
│ Multi-Master         │        Yes          │       Yes         │
│ Internal Pull-up     │        No           │       Yes         │
└──────────────────────┴─────────────────────┴───────────────────┘
```

---

## 📖 Referensi

1. I2C-bus Specification and User Manual (NXP UM10204)
2. STM32F103 Reference Manual (RM0008) - Chapter 26: I2C
3. ESP32 Technical Reference Manual - Chapter 11: I2C Controller
4. BME280 Datasheet (Bosch)
5. SSD1306 Datasheet (Solomon Systech)
6. DS3231 Datasheet (Maxim Integrated)
7. "Mastering STM32" by Carmine Noviello - Chapter 13: I2C

