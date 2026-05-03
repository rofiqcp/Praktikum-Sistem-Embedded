# Modul 07: SPI Bus dan Storage


## Daftar Isi

## Capaian Pembelajaran

Setelah menyelesaikan bab ini, mahasiswa diharapkan mampu:

1. Memahami protokol SPI (Serial Peripheral Interface) secara mendalam
2. Mengkonfigurasi SPI pada STM32 dan ESP32 menggunakan HAL dan Arduino
3. Menginterfacing dengan SD Card untuk data logging
4. Menggunakan Flash memory eksternal (W25Q series)
5. Mengimplementasikan file system (FatFS, SPIFFS, LittleFS)
6. Melakukan optimasi transfer data dengan DMA-SPI
7. Menerapkan best practices untuk reliable storage

---


## 1. Pendahuluan SPI Bus

### 1.1 Apa itu SPI?

**Serial Peripheral Interface (SPI)** adalah protokol komunikasi serial synchronous full-duplex yang dikembangkan oleh Motorola. SPI menggunakan arsitektur **Master-Slave** dimana satu Master dapat berkomunikasi dengan multiple Slave devices.

**Karakteristik Utama SPI:**
| Parameter | Keterangan |
|-----------|------------|
| Tipe | Synchronous, Full-duplex |
| Topology | Single Master, Multiple Slave |
| Clock | Disediakan oleh Master |
| Speed | Hingga 80+ MHz (device dependent) |
| Distance | Short distance (PCB level) |
| Wires | 4 wires + 1 CS per slave |

### 1.2 SPI Bus Lines

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           SPI BUS ARCHITECTURE                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│    ┌──────────────┐           ┌──────────────┐     ┌──────────────┐        │
│    │    MASTER    │           │   SLAVE 1    │     │   SLAVE 2    │        │
│    │              │           │              │     │              │        │
│    │         MOSI ├──────────►│ MOSI         │────►│ MOSI         │        │
│    │              │           │              │     │              │        │
│    │         MISO │◄──────────┤ MISO         │◄────┤ MISO         │        │
│    │              │           │              │     │              │        │
│    │         SCLK ├──────────►│ SCLK         │────►│ SCLK         │        │
│    │              │           │              │     │              │        │
│    │         CS0  ├──────────►│ CS           │     │              │        │
│    │         CS1  ├───────────┼──────────────┼────►│ CS           │        │
│    └──────────────┘           └──────────────┘     └──────────────┘        │
│                                                                              │
│    Signal Descriptions:                                                      │
│    ═══════════════════                                                      │
│    MOSI = Master Out, Slave In (Data dari Master ke Slave)                  │
│    MISO = Master In, Slave Out (Data dari Slave ke Master)                  │
│    SCLK = Serial Clock (Clock signal dari Master)                           │
│    CS/SS = Chip Select / Slave Select (Active LOW)                          │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.3 SPI Clock Configuration (CPOL dan CPHA)

SPI memiliki 4 mode berdasarkan kombinasi **Clock Polarity (CPOL)** dan **Clock Phase (CPHA)**:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          SPI CLOCK MODES                                     │
├──────┬──────┬──────┬────────────────────────────────────────────────────────┤
│ Mode │ CPOL │ CPHA │ Description                                             │
├──────┼──────┼──────┼────────────────────────────────────────────────────────┤
│  0   │  0   │  0   │ Clock idle LOW, sample on RISING edge (most common)    │
│  1   │  0   │  1   │ Clock idle LOW, sample on FALLING edge                 │
│  2   │  1   │  0   │ Clock idle HIGH, sample on FALLING edge                │
│  3   │  1   │  1   │ Clock idle HIGH, sample on RISING edge                 │
└──────┴──────┴──────┴────────────────────────────────────────────────────────┘

Mode 0 (CPOL=0, CPHA=0):        Mode 3 (CPOL=1, CPHA=1):
        ┌───┐   ┌───┐                   ┌───┐   ┌───┐
SCLK    │   │   │   │           ───────┘   └───┘   └───
    ────┘   └───┘   └───        
          ↑       ↑                   ↑       ↑
       Sample  Sample              Sample  Sample
       
    Most common untuk:          Common untuk:
    - SD Card                   - Some sensors
    - Most flash memory         - Display controllers
    - General purpose
```

### 1.4 SPI vs I2C vs UART Comparison

| Feature | SPI | I2C | UART |
|---------|-----|-----|------|
| Speed | Highest (80+ MHz) | Medium (3.4 MHz max) | Lowest (115200 baud) |
| Wires | 4 + n CS | 2 | 2 |
| Full Duplex | Yes | No | Yes |
| Multi-Master | Limited | Yes | No |
| Device Addressing | CS line | 7-bit address | N/A |
| Distance | Short | Medium | Long |
| Complexity | Low | Medium | Low |
| Flow Control | None | ACK/NACK | Optional |

---

## 2. SPI pada STM32F103C8T6

### 2.1 Hardware Overview

STM32F103C8T6 memiliki **2 SPI peripherals**:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    STM32F103 SPI PINOUT                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│    SPI1 (APB2 - up to 36 MHz):                                              │
│    ┌────────────────────────────────────┐                                   │
│    │  Function    │  Pin (Default)      │                                   │
│    ├──────────────┼─────────────────────┤                                   │
│    │  SCLK        │  PA5                │                                   │
│    │  MISO        │  PA6                │                                   │
│    │  MOSI        │  PA7                │                                   │
│    │  NSS         │  PA4 (optional)     │                                   │
│    └────────────────────────────────────┘                                   │
│                                                                              │
│    SPI1 (Remapped):                                                          │
│    ┌────────────────────────────────────┐                                   │
│    │  Function    │  Pin (Remapped)     │                                   │
│    ├──────────────┼─────────────────────┤                                   │
│    │  SCLK        │  PB3                │                                   │
│    │  MISO        │  PB4                │                                   │
│    │  MOSI        │  PB5                │                                   │
│    │  NSS         │  PA15               │                                   │
│    └────────────────────────────────────┘                                   │
│                                                                              │
│    SPI2 (APB1 - up to 18 MHz):                                              │
│    ┌────────────────────────────────────┐                                   │
│    │  Function    │  Pin                │                                   │
│    ├──────────────┼─────────────────────┤                                   │
│    │  SCLK        │  PB13               │                                   │
│    │  MISO        │  PB14               │                                   │
│    │  MOSI        │  PB15               │                                   │
│    │  NSS         │  PB12 (optional)    │                                   │
│    └────────────────────────────────────┘                                   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 SPI Clock Calculation

```
SPI Clock Frequency = APB Clock / Prescaler

For SPI1 (APB2 = 72 MHz):
┌───────────┬──────────────┐
│ Prescaler │ SPI Clock    │
├───────────┼──────────────┤
│    2      │  36 MHz      │
│    4      │  18 MHz      │
│    8      │   9 MHz      │
│   16      │  4.5 MHz     │
│   32      │  2.25 MHz    │
│   64      │  1.125 MHz   │
│  128      │  562.5 kHz   │
│  256      │  281.25 kHz  │
└───────────┴──────────────┘

For SPI2 (APB1 = 36 MHz):
┌───────────┬──────────────┐
│ Prescaler │ SPI Clock    │
├───────────┼──────────────┤
│    2      │  18 MHz      │
│    4      │   9 MHz      │
│    8      │  4.5 MHz     │
│   16      │  2.25 MHz    │
└───────────┴──────────────┘
```

### 2.3 HAL SPI Configuration

```c
/* SPI Handle Structure */
SPI_HandleTypeDef hspi1;

void MX_SPI1_Init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;           // Master mode
    hspi1.Init.Direction = SPI_DIRECTION_2LINES; // Full duplex
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;     // 8-bit data
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;   // CPOL = 0
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;       // CPHA = 0
    hspi1.Init.NSS = SPI_NSS_SOFT;               // Software CS
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // 9 MHz
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;      // MSB first
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}
```

---

## 3. SPI pada ESP32

### 3.1 ESP32 SPI Hardware

ESP32 memiliki **4 SPI controllers**, namun hanya 2 yang available untuk user:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         ESP32 SPI CONTROLLERS                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│    SPI0/SPI1: Reserved for internal flash (not available)                   │
│                                                                              │
│    HSPI (SPI2) - Default Pins:                                              │
│    ┌────────────────────────────────────┐                                   │
│    │  Function    │  GPIO               │                                   │
│    ├──────────────┼─────────────────────┤                                   │
│    │  SCLK        │  GPIO14             │                                   │
│    │  MISO        │  GPIO12             │                                   │
│    │  MOSI        │  GPIO13             │                                   │
│    │  CS          │  GPIO15             │                                   │
│    └────────────────────────────────────┘                                   │
│                                                                              │
│    VSPI (SPI3) - Default Pins:                                              │
│    ┌────────────────────────────────────┐                                   │
│    │  Function    │  GPIO               │                                   │
│    ├──────────────┼─────────────────────┤                                   │
│    │  SCLK        │  GPIO18             │                                   │
│    │  MISO        │  GPIO19             │                                   │
│    │  MOSI        │  GPIO23             │                                   │
│    │  CS          │  GPIO5              │                                   │
│    └────────────────────────────────────┘                                   │
│                                                                              │
│    Note: GPIO pins dapat di-remap ke hampir semua GPIO                      │
│          Maximum clock: 80 MHz (dengan certain conditions)                  │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 ESP32 Arduino SPI Library

```cpp
#include <SPI.h>

// Default VSPI
SPIClass vspi(VSPI);

// Custom HSPI
SPIClass hspi(HSPI);

void setup() {
    // Initialize VSPI with default pins
    vspi.begin();  // GPIO18=SCLK, GPIO19=MISO, GPIO23=MOSI
    
    // Initialize HSPI with custom pins
    hspi.begin(14, 12, 13, 15);  // SCLK, MISO, MOSI, SS
    
    // Configure SPI settings
    vspi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
}
```

---

## 4. SD Card Interface

### 4.1 SD Card SPI Mode

SD Card mendukung dua mode: **SD mode** (4-bit) dan **SPI mode** (1-bit). Untuk embedded systems, SPI mode lebih umum digunakan karena lebih sederhana.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      SD CARD PINOUT (SPI MODE)                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│     ┌─────────────────────────────┐                                         │
│     │         SD CARD             │                                         │
│     │     ┌───────────────┐       │                                         │
│     │     │ 1 │ 2 │ 3 │ 4 │       │                                         │
│     │     │ 5 │ 6 │ 7 │ 8 │ 9 │   │                                         │
│     │     └───────────────────┘   │                                         │
│     └─────────────────────────────┘                                         │
│                                                                              │
│     Pin Mapping (SPI Mode):                                                  │
│     ┌───────┬────────────┬───────────────────────────────────────┐          │
│     │  Pin  │  SD Mode   │  SPI Mode                             │          │
│     ├───────┼────────────┼───────────────────────────────────────┤          │
│     │   1   │  CD/DAT3   │  CS (Chip Select) - Active LOW        │          │
│     │   2   │  CMD       │  MOSI (DI - Data In)                  │          │
│     │   3   │  VSS1      │  GND                                  │          │
│     │   4   │  VDD       │  3.3V                                 │          │
│     │   5   │  CLK       │  SCLK (Clock)                         │          │
│     │   6   │  VSS2      │  GND                                  │          │
│     │   7   │  DAT0      │  MISO (DO - Data Out)                 │          │
│     │   8   │  DAT1      │  Reserved (IRQ - optional)            │          │
│     │   9   │  DAT2      │  Reserved                             │          │
│     └───────┴────────────┴───────────────────────────────────────┘          │
│                                                                              │
│     IMPORTANT: SD Card operates at 3.3V!                                    │
│     Use level shifter for 5V systems.                                       │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 SD Card SPI Commands

```
┌────────────────────────────────────────────────────────────────────────────┐
│                        SD CARD COMMAND STRUCTURE                            │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│    Command Format (48 bits = 6 bytes):                                      │
│    ┌──────┬──────────┬──────────────────┬─────────┐                        │
│    │ 0 1 │ Command  │    Argument      │  CRC    │                        │
│    │ 2 bits│ 6 bits │    32 bits       │ 7+1 bits│                        │
│    └──────┴──────────┴──────────────────┴─────────┘                        │
│                                                                             │
│    Common Commands:                                                         │
│    ┌────────┬─────────────────────────────────────────────────┐            │
│    │  CMD   │  Description                                    │            │
│    ├────────┼─────────────────────────────────────────────────┤            │
│    │  CMD0  │  GO_IDLE_STATE - Reset card to idle             │            │
│    │  CMD1  │  SEND_OP_COND - Initialize card                 │            │
│    │  CMD8  │  SEND_IF_COND - Check voltage range (SDHC)      │            │
│    │  CMD9  │  SEND_CSD - Read Card Specific Data             │            │
│    │  CMD10 │  SEND_CID - Read Card Identification            │            │
│    │  CMD16 │  SET_BLOCKLEN - Set block length                │            │
│    │  CMD17 │  READ_SINGLE_BLOCK - Read one block             │            │
│    │  CMD24 │  WRITE_SINGLE_BLOCK - Write one block           │            │
│    │  CMD55 │  APP_CMD - Prefix for application commands      │            │
│    │ ACMD41 │  SD_SEND_OP_COND - Initialize SD card           │            │
│    │  CMD58 │  READ_OCR - Read Operation Conditions Register  │            │
│    └────────┴─────────────────────────────────────────────────┘            │
│                                                                             │
└────────────────────────────────────────────────────────────────────────────┘
```

### 4.3 SD Card Initialization Sequence

```
┌────────────────────────────────────────────────────────────────────────────┐
│                      SD CARD INITIALIZATION FLOW                            │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│    ┌─────────────┐                                                         │
│    │    START    │                                                         │
│    └──────┬──────┘                                                         │
│           ▼                                                                 │
│    ┌─────────────────────────┐                                             │
│    │ Power up delay (>1ms)   │                                             │
│    │ Set SPI clock <400kHz   │                                             │
│    └───────────┬─────────────┘                                             │
│                ▼                                                            │
│    ┌─────────────────────────┐                                             │
│    │ Send >74 clock cycles   │                                             │
│    │ with CS HIGH            │                                             │
│    └───────────┬─────────────┘                                             │
│                ▼                                                            │
│    ┌─────────────────────────┐     No                                      │
│    │ CMD0 (GO_IDLE_STATE)    │────────► Retry/Error                        │
│    │ Response = 0x01?        │                                             │
│    └───────────┬─────────────┘                                             │
│                ▼ Yes                                                        │
│    ┌─────────────────────────┐     No                                      │
│    │ CMD8 (SEND_IF_COND)     │────────► SD Ver1 or MMC                     │
│    │ Response valid?         │                                             │
│    └───────────┬─────────────┘                                             │
│                ▼ Yes (SDv2)                                                 │
│    ┌─────────────────────────┐                                             │
│    │ ACMD41 (SD_SEND_OP_COND)│◄────┐                                       │
│    │ with HCS bit            │     │ Repeat until                          │
│    └───────────┬─────────────┘     │ ready                                 │
│                │                   │                                        │
│                ▼                   │                                        │
│    ┌─────────────────────────┐     │                                       │
│    │ Response = 0x00?        │─No──┘                                       │
│    └───────────┬─────────────┘                                             │
│                ▼ Yes                                                        │
│    ┌─────────────────────────┐                                             │
│    │ CMD58 (READ_OCR)        │                                             │
│    │ Check CCS bit for SDHC  │                                             │
│    └───────────┬─────────────┘                                             │
│                ▼                                                            │
│    ┌─────────────────────────┐                                             │
│    │ Increase SPI clock      │                                             │
│    │ (up to 25MHz)           │                                             │
│    └───────────┬─────────────┘                                             │
│                ▼                                                            │
│    ┌─────────────┐                                                         │
│    │  SD READY   │                                                         │
│    └─────────────┘                                                         │
│                                                                             │
└────────────────────────────────────────────────────────────────────────────┘
```

---

## 5. Flash Memory (W25Qxx Series)

### 5.1 W25Q Overview

Winbond W25Qxx adalah serial NOR flash memory dengan SPI interface:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        W25Qxx SERIES COMPARISON                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│    ┌──────────┬─────────┬────────────┬────────────┬──────────────┐         │
│    │  Model   │ Density │   Pages    │  Sectors   │  Blocks      │         │
│    ├──────────┼─────────┼────────────┼────────────┼──────────────┤         │
│    │ W25Q16   │  2 MB   │  8,192     │    512     │    32        │         │
│    │ W25Q32   │  4 MB   │ 16,384     │  1,024     │    64        │         │
│    │ W25Q64   │  8 MB   │ 32,768     │  2,048     │   128        │         │
│    │ W25Q128  │ 16 MB   │ 65,536     │  4,096     │   256        │         │
│    └──────────┴─────────┴────────────┴────────────┴──────────────┘         │
│                                                                              │
│    Memory Organization:                                                      │
│    - Page size: 256 bytes (minimum write unit)                              │
│    - Sector size: 4KB (minimum erase unit)                                  │
│    - Block size: 64KB                                                       │
│    - Clock: up to 104 MHz (standard), 80 MHz (recommended)                  │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 5.2 W25Qxx Pinout

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        W25Qxx PINOUT (SOIC-8)                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│              ┌──────┬───┬──────┐                                            │
│              │  1   │   │   8  │                                            │
│              │ /CS  │   │ VCC  │ ─── 3.3V                                   │
│              ├──────┤   ├──────┤                                            │
│              │  2   │   │   7  │                                            │
│              │ DO   │   │/HOLD │ ─── 3.3V (or GPIO)                         │
│              ├──────┤   ├──────┤                                            │
│              │  3   │   │   6  │                                            │
│              │ /WP  │   │ CLK  │ ─── SPI SCLK                               │
│              ├──────┤   ├──────┤                                            │
│              │  4   │   │   5  │                                            │
│              │ GND  │   │ DI   │ ─── SPI MOSI                               │
│              └──────┴───┴──────┘                                            │
│                                                                              │
│    Pin Description:                                                          │
│    1. /CS   - Chip Select (Active LOW)                                      │
│    2. DO    - Data Out (MISO)                                               │
│    3. /WP   - Write Protect (tie to VCC if not used)                        │
│    4. GND   - Ground                                                        │
│    5. DI    - Data In (MOSI)                                                │
│    6. CLK   - Serial Clock                                                  │
│    7. /HOLD - Hold pin (tie to VCC if not used)                             │
│    8. VCC   - 2.7V to 3.6V                                                  │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 5.3 W25Qxx Commands

```c
// W25Qxx Instruction Set
#define W25Q_WRITE_ENABLE           0x06
#define W25Q_WRITE_DISABLE          0x04
#define W25Q_READ_STATUS_REG1       0x05
#define W25Q_READ_STATUS_REG2       0x35
#define W25Q_WRITE_STATUS_REG       0x01
#define W25Q_READ_DATA              0x03
#define W25Q_FAST_READ              0x0B
#define W25Q_PAGE_PROGRAM           0x02
#define W25Q_SECTOR_ERASE           0x20    // 4KB
#define W25Q_BLOCK_ERASE_32KB       0x52
#define W25Q_BLOCK_ERASE_64KB       0xD8
#define W25Q_CHIP_ERASE             0xC7
#define W25Q_POWER_DOWN             0xB9
#define W25Q_RELEASE_POWER_DOWN     0xAB
#define W25Q_DEVICE_ID              0x90
#define W25Q_JEDEC_ID               0x9F

// Status Register Bits
#define W25Q_SR1_BUSY               0x01    // Busy flag
#define W25Q_SR1_WEL                0x02    // Write Enable Latch
```

---

## 6. File Systems untuk Embedded

### 6.1 FatFS (File Allocation Table)

FatFS adalah file system paling umum untuk SD Card:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          FatFS ARCHITECTURE                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│    ┌─────────────────────────────────────────────────────────┐              │
│    │                   Application Layer                      │              │
│    │           f_open, f_read, f_write, f_close              │              │
│    └─────────────────────────┬───────────────────────────────┘              │
│                              │                                               │
│    ┌─────────────────────────▼───────────────────────────────┐              │
│    │                    FatFS Module                          │              │
│    │        File/Directory Management, FAT Operations         │              │
│    └─────────────────────────┬───────────────────────────────┘              │
│                              │                                               │
│    ┌─────────────────────────▼───────────────────────────────┐              │
│    │                   diskio.c (User Provided)               │              │
│    │      disk_initialize, disk_read, disk_write, disk_ioctl │              │
│    └─────────────────────────┬───────────────────────────────┘              │
│                              │                                               │
│    ┌─────────────────────────▼───────────────────────────────┐              │
│    │                   Hardware Driver                        │              │
│    │                    SPI/SDIO Driver                       │              │
│    └─────────────────────────────────────────────────────────┘              │
│                                                                              │
│    FAT Types:                                                                │
│    - FAT12: For very small volumes (<16 MB)                                 │
│    - FAT16: Up to 2 GB                                                      │
│    - FAT32: Up to 2 TB (32 GB practical limit)                              │
│    - exFAT: >32 GB (requires license)                                       │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 6.2 SPIFFS dan LittleFS (ESP32)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    ESP32 FILE SYSTEMS COMPARISON                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│    ┌─────────────────┬───────────────────┬─────────────────────┐            │
│    │    Feature      │      SPIFFS       │     LittleFS        │            │
│    ├─────────────────┼───────────────────┼─────────────────────┤            │
│    │ Directories     │ No (flat)         │ Yes                 │            │
│    │ Wear Leveling   │ Yes               │ Yes                 │            │
│    │ Power-loss Safe │ Partial           │ Yes                 │            │
│    │ Performance     │ Slower            │ Faster              │            │
│    │ RAM Usage       │ Higher            │ Lower               │            │
│    │ File Size       │ Limited           │ Better              │            │
│    │ Maintenance     │ Deprecated        │ Active              │            │
│    └─────────────────┴───────────────────┴─────────────────────┘            │
│                                                                              │
│    Recommendation: Use LittleFS for new projects                            │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 7. DMA dengan SPI

### 7.1 SPI DMA Transfer

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SPI DMA TRANSFER FLOW                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│    Without DMA (Polling/Interrupt):                                          │
│    ┌────────┐  ┌─────┐  ┌──────────┐  ┌─────┐  ┌────────┐                  │
│    │  CPU   │──│WRITE│──│SPI Buffer│──│ SPI │──│ Device │                  │
│    │        │◄─│READ │◄─│          │◄─│ HW  │◄─│        │                  │
│    └────────┘  └─────┘  └──────────┘  └─────┘  └────────┘                  │
│       ▲                                                                      │
│       │ CPU busy waiting or handling interrupts                             │
│                                                                              │
│    With DMA:                                                                 │
│    ┌────────┐  ┌─────┐  ┌──────────┐  ┌─────┐  ┌────────┐                  │
│    │  CPU   │  │ DMA │══│SPI Buffer│══│ SPI │══│ Device │                  │
│    │        │  │     │  │          │  │ HW  │  │        │                  │
│    └───┬────┘  └──┬──┘  └──────────┘  └─────┘  └────────┘                  │
│        │         │                                                          │
│        │ Setup   │ Transfer (CPU free)                                      │
│        ▼         ▼                                                          │
│    ┌──────────────────┐                                                     │
│    │  CPU does other  │ ─── Parallel execution!                             │
│    │  tasks           │                                                     │
│    └──────────────────┘                                                     │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 7.2 STM32 SPI DMA Example

```c
/* DMA Configuration for SPI1 TX */
hdma_spi1_tx.Instance = DMA1_Channel3;
hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
hdma_spi1_tx.Init.Mode = DMA_NORMAL;
hdma_spi1_tx.Init.Priority = DMA_PRIORITY_HIGH;

HAL_DMA_Init(&hdma_spi1_tx);
__HAL_LINKDMA(&hspi1, hdmatx, hdma_spi1_tx);

/* Usage */
HAL_SPI_Transmit_DMA(&hspi1, txBuffer, size);
// CPU is free while transfer happens
```

---

## 8. Best Practices

### 8.1 SPI Design Guidelines

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      SPI BEST PRACTICES CHECKLIST                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  Hardware:                                                                   │
│  ☐ Keep SPI traces short (<10 cm untuk high speed)                          │
│  ☐ Use decoupling capacitors (100nF) dekat slave device                     │
│  ☐ Match impedance untuk high-speed designs                                 │
│  ☐ Separate analog dan digital grounds                                      │
│  ☐ Use series resistors (22-100Ω) untuk signal integrity                    │
│                                                                              │
│  Software:                                                                   │
│  ☐ Always check device ready sebelum operations                             │
│  ☐ Implement proper CS timing (setup dan hold times)                        │
│  ☐ Use DMA untuk large transfers                                            │
│  ☐ Handle SPI errors gracefully                                             │
│  ☐ Verify data dengan read-back atau CRC                                    │
│                                                                              │
│  Flash Memory:                                                               │
│  ☐ Always erase sebelum write                                               │
│  ☐ Implement wear leveling untuk frequently written data                    │
│  ☐ Wait for busy flag setelah write/erase                                   │
│  ☐ Use block erase untuk large areas (faster)                               │
│                                                                              │
│  SD Card:                                                                    │
│  ☐ Initialize dengan slow clock (<400 kHz)                                  │
│  ☐ Increase clock setelah initialization                                    │
│  ☐ Properly unmount sebelum power off                                       │
│  ☐ Handle card removal gracefully                                           │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 9. Internal Flash Memory

Selain menggunakan external flash (W25Qxx), mikrokontroler memiliki internal flash yang bisa digunakan untuk menyimpan konfigurasi, kalibrasi, dan data non-volatile.

### 9.1 STM32F103 Internal Flash

STM32F103C8T6 memiliki 64KB flash (128KB pada beberapa chip) dengan page size 1KB:

```c
#include "stm32f1xx_hal.h"

/* Alamat flash untuk data user (page terakhir) */
#define USER_FLASH_ADDR  0x0800FC00  /* Page 63 (1KB dari akhir) */

/* Tulis data ke internal flash */
HAL_StatusTypeDef Flash_Write(uint32_t addr, uint32_t *data, uint32_t len)
{
    HAL_FLASH_Unlock();
    
    /* Erase halaman dulu (wajib sebelum write) */
    FLASH_EraseInitTypeDef erase = {
        .TypeErase   = FLASH_TYPEERASE_PAGES,
        .PageAddress = addr,
        .NbPages     = 1,
    };
    uint32_t page_error;
    HAL_FLASHEx_Erase(&erase, &page_error);
    
    /* Write data per 32-bit word */
    for (uint32_t i = 0; i < len; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                          addr + (i * 4), data[i]);
    }
    
    HAL_FLASH_Lock();
    return HAL_OK;
}

/* Baca data dari flash */
void Flash_Read(uint32_t addr, uint32_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        data[i] = *(volatile uint32_t *)(addr + (i * 4));
    }
}

/* Contoh: Simpan kalibrasi sensor */
typedef struct {
    float offset;
    float gain;
    uint32_t checksum;
} CalibData_t;

void save_calibration(float offset, float gain)
{
    CalibData_t cal = {
        .offset   = offset,
        .gain     = gain,
        .checksum = 0xDEADBEEF
    };
    Flash_Write(USER_FLASH_ADDR, (uint32_t *)&cal,
                sizeof(cal) / 4);
}
```

### 9.2 ESP32 NVS (Non-Volatile Storage) — Detail

NVS pada ESP32 menggunakan flash partition khusus untuk menyimpan key-value pairs. Lebih aman dan efisien dari raw flash write:

```c
#include "nvs_flash.h"
#include "nvs.h"

void nvs_example(void)
{
    /* Inisialisasi NVS */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    nvs_handle_t handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    
    /* Simpan berbagai tipe data */
    nvs_set_i32(handle, "boot_count", 42);
    nvs_set_str(handle, "device_name", "Sensor-01");
    
    /* Simpan data binary (blob) */
    float calibration[] = {1.0f, 0.5f, -0.3f};
    nvs_set_blob(handle, "cal_data", calibration, sizeof(calibration));
    
    nvs_commit(handle);  /* Wajib! Flush ke flash */
    
    /* Baca kembali */
    int32_t count;
    nvs_get_i32(handle, "boot_count", &count);
    printf("Boot count: %ld\n", count);
    
    char name[32];
    size_t len = sizeof(name);
    nvs_get_str(handle, "device_name", name, &len);
    printf("Device: %s\n", name);
    
    nvs_close(handle);
}

/* Iterasi semua key di namespace */
void nvs_list_keys(void)
{
    nvs_iterator_t it = NULL;
    nvs_entry_find("nvs", "storage", NVS_TYPE_ANY, &it);
    
    while (it != NULL) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        printf("Key: %-16s  Type: %d\n", info.key, info.type);
        nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
}
```

### 9.3 ESP32 Partition Table

ESP32 menggunakan partition table untuk membagi flash menjadi beberapa area:

```
Default Partition Table (4MB Flash):
┌────────────────────────────────────────────────────┐
│ Offset    │ Size   │ Name       │ Type             │
├───────────┼────────┼────────────┼──────────────────┤
│ 0x009000  │  4KB   │ nvs        │ data/nvs         │
│ 0x00A000  │  4KB   │ otadata    │ data/ota         │
│ 0x00E000  │  8KB   │ phy_init   │ data/phy         │
│ 0x010000  │ 1MB    │ factory    │ app/factory       │
│ 0x110000  │ 1MB    │ ota_0      │ app/ota_0        │
│ 0x210000  │ 1MB    │ ota_1      │ app/ota_1        │
│ 0x310000  │ 960KB  │ spiffs     │ data/spiffs      │
└────────────────────────────────────────────────────┘
```

**Custom Partition Table (partitions.csv):**

```csv
# Name,   Type, SubType,  Offset,  Size,  Flags
nvs,      data, nvs,      0x9000,  24K,
otadata,  data, ota,      0xf000,  8K,
phy_init, data, phy,      0x11000, 4K,
factory,  app,  factory,  0x20000, 1M,
storage,  data, spiffs,   0x120000,896K,
```

---

## 10. Troubleshooting Guide

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| No response dari device | CS not connected/wrong | Verify CS wiring dan polarity |
| Data corruption | Clock terlalu fast | Reduce SPI clock speed |
| Intermittent errors | Noise/long wires | Shorten traces, add decoupling |
| SD Card tidak init | Wrong initialization | Follow proper init sequence |
| Flash write fails | Sector not erased | Erase sebelum write |
| MISO always high/low | MISO wiring wrong | Check connections |
| Wrong data read | Mode mismatch | Match CPOL/CPHA dengan device |
| NVS full | Partition penuh | `nvs_flash_erase()` + reinit |
| Flash wear-out | Terlalu sering write | Gunakan NVS (built-in wear leveling) |

---

## 11. Daftar Program Praktikum

| No | Platform | Nama Program | Topik | Tingkat |
|----|----------|-------------|-------|---------|
| 01 | ESP32 | SPI_Basic | SPI Master dasar | Dasar |
| 02 | ESP32 | SPI_Flash | W25Qxx Flash read/write | Menengah |
| 03 | ESP32 | SPI_SD_Card | SD Card SPI mode | Menengah |
| 04 | ESP32 | SPI_OLED | SSD1306 OLED via SPI | Menengah |
| 05 | ESP32 | SPI_Multi_Device | Multiple SPI slaves | Lanjut |
| 06 | ESP32 | SPI_DMA | DMA SPI transfer | Lanjut |
| 07 | STM32 | SPI_Basic | SPI Master dasar | Dasar |
| 08 | STM32 | SPI_Flash | W25Qxx Flash read/write | Menengah |
| 09 | STM32 | SPI_SD_Card | SD Card SPI mode + FatFS | Menengah |
| 10 | STM32 | SPI_OLED | SSD1306 OLED via SPI | Menengah |
| 11 | STM32 | SPI_Multi_Device | Multiple SPI slaves | Lanjut |
| 12 | STM32 | SPI_DMA | DMA SPI transfer | Lanjut |

---

## Referensi

1. **STM32 Reference Manual RM0008** - SPI Chapter, Flash Chapter
2. **ESP32 Technical Reference Manual** - SPI Chapter
3. **SD Specifications Part 1** - Physical Layer Simplified
4. **W25Q64 Datasheet** - Winbond Serial Flash
5. **FatFS Module Application Note** - elm-chan.org
6. **"Mastering STM32"** - Carmine Noviello, Chapter 16: SPI, Chapter Flash
7. **"Kolban's Book on ESP32"** - NVS & Partition Table sections
8. **ESP-IDF NVS Documentation** - https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html

