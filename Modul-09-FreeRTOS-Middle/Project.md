# Project Modul 09: RTOS Sensor Hub System

## Informasi Project

| Item | Keterangan |
|---|---|
| Modul | 09 — FreeRTOS Middle - GPIO, Interrupt, Encoder, Serial, DAC, ADC, I2C, SPI |
| Struktur praktikum | 10 STM32 + 10 ESP32 + 5 Multi STM32-ESP32 |
| Platform | ESP32 DevKit + STM32F103/STM32F4 |
| Tema | RTOS Sensor Hub System |
| Durasi | 2 minggu |
| Output | Sistem hardware, source code, laporan, video demo |

---

## 1. Deskripsi Umum

Project Modul 09 menggabungkan seluruh materi FreeRTOS Middle menjadi **RTOS Sensor Hub System**. ESP32 dan STM32 bekerja bersama untuk membaca sensor, menghasilkan sinyal analog, berkomunikasi serial, dan mengelola task RTOS yang stabil.

Sistem wajib selaras dengan struktur final praktikum:

- **10 eksperimen STM32**: GPIO task blink, external interrupt, encoder 2-pin interrupt, serial communication, DAC RTOS, ADC RTOS, I2C RTOS, SPI RTOS, multi task GPIO-ADC-UART, FreeRTOS semaphore mutex.
- **10 eksperimen ESP32**: GPIO task blink, external interrupt, encoder 2-pin interrupt, serial communication, DAC RTOS, ADC RTOS, I2C RTOS, SPI RTOS, multi task GPIO-ADC-UART, FreeRTOS semaphore mutex.
- **5 eksperimen Multi STM32-ESP32**: RTOS GPIO task sync, RTOS ADC-DAC communication, RTOS I2C sensor sharing, RTOS SPI data exchange, RTOS sensor hub system.

---

## 2. Skenario

Sebuah sistem sensor hub untuk monitoring lingkungan dan kontrol analog harus mengumpulkan data dari berbagai sumber:

- Pembacaan analog (ADC) dari potensiometer atau sensor suhu.
- Pembacaan encoder untuk input user (rotary).
- Pembacaan sensor I2C (BME280) untuk kondisi lingkungan.
- Pembacaan data SPI (ADXL345) untuk akselerasi.
- Output analog (DAC) untuk kontrol tegangan.
- Komunikasi serial antar-MCU.
- Task RTOS yang berjalan secara konkuren dan sinkron.

Data diolah oleh STM32 (sensor node) dan ESP32 (gateway), kemudian ditampilkan pada serial monitor dan dipertukarkan antar-MCU.

---

## 3. Arsitektur Sistem

```text
           PC/Monitor
               │
         ┌─────▼─────┐          UART
         │   ESP32   │◄────────────────────────┐
         │ Gateway   │                         │
         └─────┬─────┘                         │
               │ UART/SPI/I2C                   │
               │                                │
         ┌─────▼─────┐                         │
         │   STM32   │─────────────────────────┘
         │ Sensor Hub │
         └─────┬─────┘
               │
    ┌──────────┼──────────┬──────────┐
    │          │          │          │
  ADC        Encoder     I2C       SPI
 Potensiometer 2-pin    BME280   ADXL345
  /Sensor               0x76
```

ESP32 sebagai gateway mengelola komunikasi ke PC dan bertindak sebagai sinkronisasi master. STM32 sebagai sensor node mengoleksi data sensor dan mengirim ke ESP32.

---

## 4. Peran Platform

### ESP32

- Gateway dan koordinator sistem RTOS.
- FreeRTOS native ESP32.
- GPIO tasks blink LED dengan `vTaskDelay()`.
- External interrupt dengan binary semaphore.
- Encoder 2-pin interrupt dengan queue.
- UART communication dengan queue/ISR.
- DAC task generate sinyal analog.
- ADC task baca potensiometer.
- I2C task baca sensor dengan mutex.
- SPI task komunikasi dengan mutex.
- Komunikasi UART dengan STM32 untuk tukar data.

### STM32

- Sensor node deterministik dengan FreeRTOS (CMSIS-RTOS/STM32Cube).
- GPIO tasks blink LED.
- External interrupt EXTI dengan semaphore.
- Encoder 2-pin dengan EXTI dan queue.
- UART RX interrupt dengan queue.
- ADC task dengan DMA/polling.
- DAC task generate sinyal.
- I2C task dengan HAL dan mutex.
- SPI task dengan HAL dan mutex.
- Kirim data teraggregasi ke ESP32 via UART.

---

## 5. Peripherals Wajib

| Peripheral | STM32 Pin | ESP32 Pin | Fungsi | RTOS Primitive |
|---|---|---|---|---|
| LED | PA5/PC13 | GPIO2 | GPIO task blink | vTaskDelay |
| Button | PA0 | GPIO0 | External interrupt | Binary Semaphore |
| Encoder A | PB0 | GPIO4 | Encoder interrupt | Queue |
| Encoder B | PB1 | GPIO5 | Encoder interrupt | Queue |
| ADC | PA0 (ADC1_IN0) | GPIO34 (ADC1_CH6) | Baca analog | Queue |
| DAC | PA4 (DAC1) | GPIO25 (DAC1) | Output analog | vTaskDelay |
| I2C SDA | PB7 | GPIO21 | Sensor I2C | Mutex |
| I2C SCL | PB6 | GPIO22 | Sensor I2C | Mutex |
| SPI MOSI | PA7 | GPIO23 | SPI comm | Mutex |
| SPI MISO | PA6 | GPIO19 | SPI comm | Mutex |
| SPI SCK | PA5 | GPIO18 | SPI comm | Mutex |
| UART TX | PA2 (USART2) | GPIO17 (UART2) | MCU-MCU comm | Queue |
| UART RX | PA3 (USART2) | GPIO16 (UART2) | MCU-MCU comm | Queue |

---

## 6. Struktur Task RTOS

### STM32 Tasks

```c
// Task definitions
void vLEDTask(void *arg);        // GPIO blink
void vButtonTask(void *arg);      // External interrupt
void vEncoderTask(void *arg);     // Encoder 2-pin
void vADCTask(void *arg);         // ADC read
void vDACTask(void *arg);         // DAC output
void vI2CTask(void *arg);        // I2C sensor
void vSPITask(void *arg);         // SPI comm
void vUARTTxTask(void *arg);      // UART send to ESP32
void vUARTRxTask(void *arg);      // UART receive from ESP32
void vMonitorTask(void *arg);      // Serial monitor
```

### ESP32 Tasks

```c
// Task definitions
void vLEDTask(void *arg);        // GPIO blink
void vButtonTask(void *arg);      // External interrupt
void vEncoderTask(void *arg);     // Encoder 2-pin
void vADCTask(void *arg);         // ADC read
void vDACTask(void *arg);         // DAC output
void vI2CTask(void *arg);        // I2C sensor
void vSPITask(void *arg);         // SPI comm
void vUARTTxTask(void *arg);      // UART send to STM32
void vUARTRxTask(void *arg);      // UART receive from STM32
void vMonitorTask(void *arg);      // Serial monitor
```

---

## 7. RTOS Primitives yang Digunakan

| Primitive | Tujuan | Contoh Penggunaan |
|---|---|---|
| Binary Semaphore | ISR-to-task signaling | Button interrupt → task toggle LED |
| Mutex | Shared resource protection | I2C bus, SPI bus, UART |
| Queue | Inter-task data transfer | ADC value → display task, UART RX data |
| Event Group | Task synchronization | Multiple events wait |
| Counting Semaphore | Resource pool | Buffer pool management |

---

## 8. Alur Operasi

### Startup

1. Init semua peripheral (GPIO, EXTI, ADC, DAC, I2C, SPI, UART).
2. Create RTOS primitives (semaphore, mutex, queue, event group).
3. Create semua tasks.
4. Start scheduler (STM32) atau biarkan otomatis (ESP32).
5. Tampilkan startup message via UART.

### Normal Loop

1. STM32 baca ADC periodik, kirim ke queue.
2. STM32 baca encoder via interrupt, update counter.
3. STM32 baca sensor I2C dengan mutex protection.
4. STM32 generate DAC output periodik.
5. STM32 kirim data teraggregasi ke ESP32 via UART.
6. ESP32 terima data dari STM32, tampilkan di monitor.
7. ESP32 jalankan task DAC/ADC/I2C/SPI lokal.
8. ESP32 kirim command ke STM32 via UART.

### Error Mode

1. Jika sensor I2C error, release mutex dan retry.
2. Jika UART communication timeout, retry atau mark offline.
3. Task watchdog untuk monitor task hang.
4. ISR error handling tanpa blocking.

---

## 9. Mapping 25 Eksperimen ke Project

| Eksperimen | Kontribusi Project |
|---|---|
| STM32_01, ESP32_01 | GPIO task blink LED |
| STM32_02, ESP32_02 | External interrupt handling |
| STM32_03, ESP32_03 | Encoder 2-pin interrupt |
| STM32_04, ESP32_04 | Serial communication UART |
| STM32_05, ESP32_05 | DAC signal generation |
| STM32_06, ESP32_06 | ADC reading |
| STM32_07, ESP32_07 | I2C sensor reading |
| STM32_08, ESP32_08 | SPI communication |
| STM32_09, ESP32_09 | Multi task integration |
| STM32_10, ESP32_10 | Semaphore and mutex usage |
| MULTI_01 | RTOS task sync antar MCU |
| MULTI_02 | ADC-DAC data exchange |
| MULTI_03 | I2C sensor sharing |
| MULTI_04 | SPI data exchange |
| MULTI_05 | Final sensor hub integration |

---

## 10. Fitur Minimal Wajib

1. STM32 dan ESP32 menjalankan minimal 3 task RTOS.
2. External interrupt ditangani dengan semaphore (bukan polling).
3. Encoder 2-pin dibaca dengan interrupt RTOS (queue).
4. UART communication menggunakan queue (ISR-to-task).
5. ADC dibaca dengan task periodik, hasil dikirim via queue.
6. DAC generate sinyal periodik dengan task.
7. I2C access dilindungi mutex (jika shared).
8. SPI access dilindungi mutex (jika shared).
9. Komunikasi antar-MCU menggunakan UART dengan protocol sederhana.
10. Error handling pada semua peripheral (timeout, retry).

---

## 11. Output Tampilan Minimum

### Serial Monitor (STM32)

```text
STM32 RTOS Sensor Hub
Task LED: running
Task ADC: 2048 (1.65V)
Task Encoder: count=42, dir=CW
Task I2C: T=28.5C, P=1013hPa
Task SPI: ACC=1.02g
Sending to ESP32...
```

### Serial Monitor (ESP32)

```text
ESP32 RTOS Gateway
Received from STM32:
  ADC: 2048 (1.65V)
  Encoder: 42, CW
  I2C: T=28.5C, P=1013hPa
  SPI: ACC=1.02g
DAC Output: 2.5V
Local ADC: 512 (0.41V)
```

---

## 12. Rubrik Penilaian

| Komponen | Bobot |
|---|---:|
| GPIO tasks RTOS (STM32 + ESP32) | 10% |
| External interrupt dengan RTOS | 10% |
| Encoder 2-pin interrupt RTOS | 10% |
| Serial communication RTOS | 10% |
| DAC dan ADC dengan RTOS | 15% |
| I2C dan SPI dengan RTOS | 15% |
| Multi-task integration | 10% |
| Komunikasi antar-MCU RTOS | 15% |
| Error handling dan robustness | 5% |
| Kode modular, laporan, video | 5% |

---

## 13. Struktur Kode Disarankan

```text
project_modul_09/
├─ esp32/
│  ├─ main.c
│  ├─ tasks_led.c
│  ├─ tasks_encoder.c
│  ├─ tasks_uart.c
│  ├─ tasks_adc.c
│  ├─ tasks_dac.c
│  ├─ tasks_i2c.c
│  ├─ tasks_spi.c
│  └─ protocol.c
├─ stm32/
│  ├─ main.c
│  ├─ tasks_led.c
│  ├─ tasks_encoder.c
│  ├─ tasks_uart.c
│  ├─ tasks_adc.c
│  ├─ tasks_dac.c
│  ├─ tasks_i2c.c
│  ├─ tasks_spi.c
│  └─ protocol.c
└─ shared/
    ├─ protocol.h
    └─ rtos_config.h
```

Dokumen baru tidak wajib dibuat; struktur ini hanya panduan implementasi.

---

## 14. Checklist Demo

- [ ] Menunjukkan hardware ESP32 + STM32.
- [ ] Menunjukkan LED blink dengan RTOS task.
- [ ] Menunjukkan external interrupt dengan semaphore.
- [ ] Menunjukkan encoder 2-pin dengan interrupt RTOS.
- [ ] Menunjukkan serial communication dengan queue.
- [ ] Menunjukkan DAC output (osiloskop).
- [ ] Menunjukkan ADC reading (potensiometer).
- [ ] Menunjukkan I2C sensor reading (jika tersedia).
- [ ] Menunjukkan SPI communication (jika tersedia).
- [ ] Menunjukkan komunikasi antar-MCU via UART.
- [ ] Menunjukkan multi-task berjalan bersamaan.

---

## 15. Catatan Keselamatan dan Keandalan

- Jangan gunakan blocking function (`HAL_Delay()`) di task RTOS; gunakan `vTaskDelay()` atau `osDelay()`.
- Jangan gunakan blocking function di ISR; gunakan ISR-safe API (`FromISR`).
- Pastikan stack size cukup untuk setiap task (cek dengan `uxTaskGetStackHighWaterMark()`).
- Gunakan mutex untuk shared resource, bukan `taskENTER_CRITICAL()` (agar scheduler tetap berjalan).
- Pastikan priority task sesuai: ISR handler harus prioritas tinggi, task background prioritas rendah.
- Gunakan `configCHECK_FOR_STACK_OVERFLOW` untuk debug stack overflow.
- Periksa heap RTOS cukup untuk task dan primitive (`configTOTAL_HEAP_SIZE`).
