# Project Modul 01: GPIO Digital I/O
## Smart Home Control Panel dengan STM32 dan ESP32

---

## 📋 Informasi Project

| Item | Keterangan |
|------|------------|
| **Nama Project** | Smart Home Control Panel |
| **Tingkat Kesulitan** | ⭐⭐⭐ (Intermediate) |
| **Platform** | STM32F103C8T6 + ESP32 (Dual MCU) |
| **Framework** | STM32Cube HAL (STM32), ESP-IDF (ESP32) |
| **Durasi Pengerjaan** | 2 minggu |
| **Tipe** | Kelompok (2-3 orang) |

---

## 🎯 Deskripsi Project

Dalam project ini, mahasiswa akan membangun sebuah **Smart Home Control Panel** yang mengintegrasikan **dua mikrokontroler** (STM32 dan ESP32) yang bekerja bersama untuk mengendalikan berbagai perangkat rumah tangga (disimulasikan dengan LED) dan menerima input dari berbagai sensor (disimulasikan dengan push button dan DIP switch).

### Skenario
Bayangkan sebuah panel kontrol rumah pintar dengan fitur:
- **Local Control (STM32):** Push button fisik untuk kontrol langsung
- **Remote Control (ESP32):** Kontrol via Serial/WiFi dari smartphone
- **Status Display:** LED menunjukkan status perangkat
- **Safety Interlock:** Emergency stop untuk situasi darurat
- **Multi-room Control:** DIP switch untuk memilih ruangan

```
┌─────────────────────────────────────────────────────────────────┐
│                    SMART HOME CONTROL PANEL                      │
│  ┌───────────────┐                    ┌───────────────┐         │
│  │   STM32       │    Serial Link     │    ESP32      │         │
│  │  (Local MCU)  │◄──────────────────►│ (Remote MCU)  │         │
│  │               │                    │               │         │
│  │ • 4 Buttons   │                    │ • WiFi/Serial │         │
│  │ • DIP Switch  │                    │ • Remote Cmd  │         │
│  │ • E-Stop      │                    │ • Status Log  │         │
│  └───────┬───────┘                    └───────┬───────┘         │
│          │                                    │                  │
│          └──────────────┬─────────────────────┘                  │
│                         │                                        │
│              ┌──────────┴──────────┐                            │
│              │   OUTPUT (8 LEDs)   │                            │
│              │   Room 1: 🔴🟢      │                            │
│              │   Room 2: 🔴🟢      │                            │
│              │   Room 3: 🔴🟢      │                            │
│              │   Room 4: 🔴🟢      │                            │
│              └─────────────────────┘                            │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🎯 Tujuan Pembelajaran

Setelah menyelesaikan project ini, mahasiswa mampu:

1. **Mengintegrasikan** dua mikrokontroler berbeda dalam satu sistem
2. **Menerapkan** teknik debouncing pada multiple input
3. **Merancang** state machine untuk kontrol kompleks
4. **Mengimplementasikan** protokol komunikasi sederhana antar MCU
5. **Menerapkan** safety interlock dalam desain embedded
6. **Membuat** dokumentasi teknis yang komprehensif

---

## 📐 Spesifikasi Teknis

### Hardware Requirements

#### STM32F103C8T6 (Local Controller)
| Fungsi | Pin | Keterangan |
|--------|-----|------------|
| Button Room 1 | PB0 | Pull-up internal |
| Button Room 2 | PB1 | Pull-up internal |
| Button Room 3 | PB10 | Pull-up internal |
| Button Room 4 | PB11 | Pull-up internal |
| Emergency Stop | PA0 | Pull-up, interrupt enabled |
| DIP Switch Bit 0 | PA4 | Room selector |
| DIP Switch Bit 1 | PA5 | Room selector |
| LED Room 1A | PA1 | Active HIGH |
| LED Room 1B | PA2 | Active HIGH |
| LED Room 2A | PA3 | Active HIGH |
| LED Room 2B | PA6 | Active HIGH |
| Serial TX (to ESP) | PA9 | USART1, 9600 baud |
| Serial RX (from ESP) | PA10 | USART1, 9600 baud |

#### ESP32 DevKitC (Remote Controller)
| Fungsi | Pin | Keterangan |
|--------|-----|------------|
| LED Room 3A | GPIO4 | Active HIGH |
| LED Room 3B | GPIO5 | Active HIGH |
| LED Room 4A | GPIO18 | Active HIGH |
| LED Room 4B | GPIO19 | Active HIGH |
| Status LED | GPIO2 | Built-in, heartbeat |
| Serial TX (to STM) | GPIO17 | UART2, 9600 baud |
| Serial RX (from STM) | GPIO16 | UART2, 9600 baud |
| Serial Monitor | GPIO1/3 | UART0, 115200 |

### Wiring Diagram

```
                STM32F103C8T6                         ESP32 DevKitC
            ┌─────────────────┐                   ┌─────────────────┐
            │                 │                   │                 │
     BTN1 ──┤ PB0        PA1 ├── LED1A           │ GPIO4 ├── LED3A
     BTN2 ──┤ PB1        PA2 ├── LED1B           │ GPIO5 ├── LED3B
     BTN3 ──┤ PB10       PA3 ├── LED2A           │ GPIO18├── LED4A
     BTN4 ──┤ PB11       PA6 ├── LED2B           │ GPIO19├── LED4B
   E-STOP ──┤ PA0             │                   │ GPIO2 ├── Status
            │                 │                   │                 │
   DIP-0 ──┤ PA4        PA9 ├────TX────────RX───┤ GPIO16│
   DIP-1 ──┤ PA5       PA10 ├────RX────────TX───┤ GPIO17│
            │                 │                   │                 │
            │            GND ├─────────────────┬─┤ GND   │
            │                 │                 │ │                 │
            └─────────────────┘                 │ └─────────────────┘
                                               │
                                    Common Ground
```

### Communication Protocol

Format pesan antar MCU (ASCII based):
```
STM32 → ESP32:
  "R3A:1\n"  → Room 3, LED A, ON
  "R3A:0\n"  → Room 3, LED A, OFF
  "R4B:T\n"  → Room 4, LED B, Toggle
  "STS?\n"   → Request status
  "ESTOP\n"  → Emergency stop triggered

ESP32 → STM32:
  "R1A:1\n"  → Remote command: Room 1, LED A, ON
  "ALL:0\n"  → All LEDs OFF
  "ACK\n"    → Acknowledgment
  "HB\n"     → Heartbeat (setiap 5 detik)
```

---

## 📝 Fitur yang Harus Diimplementasikan

### Level 1: Basic (Wajib) - 60 poin

- [ ] **F1.1** LED Control via Local Button (4 room × 2 LED)
- [ ] **F1.2** Software debouncing pada semua button
- [ ] **F1.3** Toggle behavior (tekan = toggle state)
- [ ] **F1.4** Emergency Stop - matikan semua LED saat E-Stop ditekan
- [ ] **F1.5** Serial Monitor debugging output
- [ ] **F1.6** Heartbeat LED pada ESP32 (blink setiap 1 detik)

### Level 2: Intermediate (Wajib) - 25 poin

- [ ] **F2.1** Room Selector dengan DIP Switch
  - DIP = 00 → Control Room 1
  - DIP = 01 → Control Room 2
  - DIP = 10 → Control Room 3
  - DIP = 11 → Control Room 4
- [ ] **F2.2** Inter-MCU Communication via Serial
- [ ] **F2.3** Remote Control dari Serial Monitor ESP32
  - Command: `ON 3A`, `OFF 4B`, `TOGGLE 1A`, `STATUS`
- [ ] **F2.4** Status reporting ke Serial Monitor

### Level 3: Advanced (Bonus) - 15 poin

- [ ] **F3.1** Scene Mode
  - "SCENE NIGHT" → Semua LED dim atau off kecuali nightlight
  - "SCENE PARTY" → LED berkedip pattern
  - "SCENE AWAY" → Random on/off untuk simulasi ada orang
- [ ] **F3.2** Long Press untuk All ON / All OFF
  - Short press (< 500ms) → Toggle single LED
  - Long press (> 2 detik) → All ON atau All OFF
- [ ] **F3.3** Power-on State Recovery
  - Simpan state terakhir di variabel
  - Restore saat boot

---

## 🔧 Struktur Kode

### STM32 Project Structure
```
STM32_Smart_Home_Local/
├── src/
│   └── main.c             # Main program (STM32Cube HAL)
├── include/
│   ├── config.h           # Pin definitions
│   ├── debounce.h         # Debounce library
│   ├── led_control.h      # LED control functions
│   └── protocol.h         # Communication protocol
├── lib/
│   └── README.md
└── platformio.ini
```

### ESP32 Project Structure
```
ESP32_Smart_Home_Remote/
├── src/
│   └── main.c             # Main program (ESP-IDF)
├── include/
│   ├── config.h           # Pin definitions
│   ├── command_parser.h   # Serial command parser
│   ├── led_control.h      # LED control functions
│   └── protocol.h         # Communication protocol
├── lib/
│   └── README.md
└── platformio.ini
```

### Contoh Code Template

**config.h (STM32 - HAL)**
```c
#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f1xx_hal.h"

// Button GPIO Ports and Pins
#define BTN_ROOM1_PORT    GPIOB
#define BTN_ROOM1_PIN     GPIO_PIN_0
#define BTN_ROOM2_PORT    GPIOB
#define BTN_ROOM2_PIN     GPIO_PIN_1
#define BTN_ROOM3_PORT    GPIOB
#define BTN_ROOM3_PIN     GPIO_PIN_10
#define BTN_ROOM4_PORT    GPIOB
#define BTN_ROOM4_PIN     GPIO_PIN_11
#define BTN_ESTOP_PORT    GPIOA
#define BTN_ESTOP_PIN     GPIO_PIN_0

// DIP Switch Pins
#define DIP_BIT0_PORT     GPIOA
#define DIP_BIT0_PIN      GPIO_PIN_4
#define DIP_BIT1_PORT     GPIOA
#define DIP_BIT1_PIN      GPIO_PIN_5

// LED Pins (Room 1-2 on STM32)
#define LED_R1A_PORT      GPIOA
#define LED_R1A_PIN       GPIO_PIN_1
#define LED_R1B_PORT      GPIOA
#define LED_R1B_PIN       GPIO_PIN_2
#define LED_R2A_PORT      GPIOA
#define LED_R2A_PIN       GPIO_PIN_3
#define LED_R2B_PORT      GPIOA
#define LED_R2B_PIN       GPIO_PIN_6

// Timing
#define DEBOUNCE_MS     50
#define LONG_PRESS_MS   2000
#define HEARTBEAT_MS    1000

// Serial
#define SERIAL_BAUD     9600

#endif
```

**main.c (STM32 - HAL) - Skeleton**
```c
#include "stm32f1xx_hal.h"
#include "config.h"

// State variables
static uint8_t led_states[4] = {0};
static volatile uint8_t emergency_mode = 0;

// Debounce variables
static uint32_t last_debounce[5] = {0};
static GPIO_PinState last_btn_state[5] = {GPIO_PIN_SET};

static UART_HandleTypeDef huart1;  // To ESP32
static UART_HandleTypeDef huart2;  // Debug

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    char msg[] = "STM32 Smart Home Ready\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    while (1) {
        handle_buttons();
        handle_serial();
        update_leds();
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == BTN_ESTOP_PIN) {
        emergency_mode = 1;
        // Send ESTOP to ESP32
        char estop[] = "ESTOP\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)estop, strlen(estop), 100);
    }
}

// TODO: Implement other functions
```

---

## 📊 Kriteria Penilaian

| Aspek | Bobot | Kriteria |
|-------|-------|----------|
| **Fungsionalitas** | 40% | Semua fitur Level 1 & 2 bekerja |
| **Kode Quality** | 20% | Clean code, proper structure, comments |
| **Dokumentasi** | 15% | README, wiring diagram, flowchart |
| **Video Demo** | 15% | Jelas, terstruktur, semua fitur ditunjukkan |
| **Bonus Features** | 10% | Level 3 features implemented |

### Rubrik Detail

#### Fungsionalitas (40 poin)
| Poin | Kriteria |
|------|----------|
| 35-40 | Semua fitur L1+L2 bekerja sempurna |
| 28-34 | Semua L1 + sebagian L2 bekerja |
| 20-27 | Semua L1 bekerja |
| 10-19 | Sebagian L1 bekerja |
| 0-9 | Minimal/tidak bekerja |

#### Kode Quality (20 poin)
| Poin | Kriteria |
|------|----------|
| 17-20 | Modular, well-documented, no code smell |
| 13-16 | Terstruktur dengan baik, comments cukup |
| 9-12 | Bekerja tapi tidak terstruktur |
| 5-8 | Banyak code smell, minimal comments |
| 0-4 | Tidak terstruktur, sulit dibaca |

---

## 📦 Deliverables

### 1. Source Code (ZIP)
```
Kelompok_XX_SmartHome.zip
├── STM32_Smart_Home_Local/
│   └── (complete project)
├── ESP32_Smart_Home_Remote/
│   └── (complete project)
└── README.md
```

### 2. Dokumentasi (PDF)
- Halaman judul
- Daftar anggota kelompok
- Wiring diagram (foto + skematik)
- Flowchart program
- Penjelasan singkat setiap fitur
- Kendala dan solusi
- Kesimpulan

### 3. Video Demonstrasi (MP4, max 5 menit)
- Tunjukkan hardware setup
- Demo setiap fitur secara berurutan
- Jelaskan cara kerja singkat
- Tunjukkan Serial Monitor output

---

## 📅 Timeline

| Minggu | Target |
|--------|--------|
| Minggu 1 | Hardware setup, basic LED/button, debouncing |
| Minggu 2 | Inter-MCU communication, remote control, polish |
| H-2 | Dokumentasi dan video |
| Deadline | Submit semua deliverables |

---

## 💡 Tips dan Hints

1. **Mulai dengan simple:** Test LED dan button terpisah dulu
2. **Incremental development:** Satu fitur at a time
3. **Test communication:** Pastikan Serial antar MCU bekerja sebelum integrasi
4. **Version control:** Gunakan Git untuk backup
5. **Debug print:** Gunakan `ESP_LOGI()` (ESP32) atau `HAL_UART_Transmit()` (STM32) untuk tracking
6. **Common ground:** PASTIKAN kedua MCU share ground!

---

## ❓ FAQ

**Q: Boleh menggunakan library external?**
A: Ya, untuk debouncing dan parsing. Dokumentasikan library yang digunakan.

**Q: Bagaimana jika tidak punya ST-Link?**
A: Gunakan USB-to-Serial dan upload via Serial bootloader.

**Q: Apakah harus WiFi untuk ESP32?**
A: Tidak wajib. Serial control sudah cukup untuk Level 1-2.

**Q: Boleh modifikasi protokol komunikasi?**
A: Ya, asalkan didokumentasikan dengan baik.

---

## 📚 Referensi

1. [PlatformIO Documentation](https://docs.platformio.org/)
2. [STM32Cube HAL User Manual](https://www.st.com/resource/en/user_manual/um1850-description-of-stm32f1-hal-and-lowlayer-drivers-stmicroelectronics.pdf)
3. [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
4. [ESP-IDF GPIO API Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
5. [ESP-IDF UART API Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/uart.html)

---

*Project Modul 01 - Praktikum Sistem Embedded*
*Deadline: [Sesuaikan dengan jadwal]*
