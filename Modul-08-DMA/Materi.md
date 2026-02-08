# Modul 08: Direct Memory Access (DMA)


## Daftar Isi

## Capaian Pembelajaran

Setelah menyelesaikan bab ini, mahasiswa diharapkan mampu:

1. Memahami arsitektur dan prinsip kerja Direct Memory Access (DMA).
2. Mengkonfigurasi DMA Controller pada STM32F103 (Channels, Priorities, Data Width).
3. Mengimplementasikan transfer Memory-to-Memory, Peripheral-to-Memory, dan Memory-to-Peripheral.
4. Menggunakan DMA Circular Mode untuk penanganan data stream kontinu (ADC, UART).
5. Memahami implementasi DMA pada ESP32 (General Purpose DMA).
6. Menganalisis peningkatan efisiensi CPU saat menggunakan DMA.

---


## 1. Pendahuluan DMA

### 1.1 Apa itu DMA?

**Direct Memory Access (DMA)** adalah fitur hardware pada mikrokontroler modern yang memungkinkan periferal (seperti ADC, UART, SPI) untuk mentransfer data langsung ke memori (RAM) atau sebaliknya, **tanpa intervensi CPU**.

**Analogi:**
- **CPU Transfer:** CEO (CPU) memindahkan dokumen satu per satu dari mesin fax ke lemari arsip. CEO tidak bisa meeting atau kerja lain.
- **DMA Transfer:** CEO menyuruh Office Boy (DMA Controller) memindahkan dokumen. CEO bebas melakukan meeting (kalkulasi math, logic, UI update) sementara OB bekerja di background.

### 1.2 Mengapa DMA Penting?

1. **Efisiensi CPU:** Membebaskan CPU untuk melakukan pemrosesan data, bukan pemindahan data.
2. **Kecepatan Tinggi:** Transfer data bisa dilakukan secepat bus memori, jauh lebih cepat dari loop software `for(i=0; i<N; i++)`.
3. **Low Power:** CPU bisa masuk mode sleep (Sleep/WFI) sementara DMA terus bekerja.
4. **Latency Deterministik:** Transfer data tidak terganggu oleh interupsi lain (kecuali bus contention).

---

## 2. Arsitektur DMA

### 2.1 Komponen Utama

```
       ┌──────────┐
       │   CPU    │
       └────┬─────┘
            │ System Bus
            │
┌───────────▼───────────┐         Data Transfer Path
│     BUS MATRIX        │ ◄═══════════════════════════════════╗
└─────┬──────────────┬──┘                                     ║
      │              │                                        ║
┌─────▼────┐    ┌────▼─────┐                              ┌───▼────┐
│   RAM    │    │ PERIPH   │                              │ DMA    │
│ (Memory) │    │(ADC/UART)│                              │ CTRL   │
└──────────┘    └──────────┘                              └────────┘
```

Sebuah transaksi DMA membutuhkan 3 parameter utama:
1. **Source Address:** Dari mana data diambil (e.g., `&ADC1->DR` atau `buffer_src`).
2. **Destination Address:** Ke mana data dikirim (e.g., `buffer_dst` atau `&USART1->DR`).
3. **Data Size (Length):** Berapa banyak data yang ditransfer.

### 2.2 Transfer Modes

1. **Normal Mode:** DMA berhenti setelah mentransfer sejumlah N data. Harus di-trigger ulang manual. Cocok untuk buffer tunggal.
2. **Circular Mode:** Setelah transfer selesai, pointer kembali ke awal buffer dan transfer berlanjut otomatis. Cocok untuk Ring Buffer, Audio Stream, ADC Continuous Scan.

### 2.3 Data Width & Alignment

DMA mendukung lebar data berbeda:
- **Byte (8-bit):** `uint8_t`
- **Half-Word (16-bit):** `uint16_t`
- **Word (32-bit):** `uint32_t`

**PENTING:** Source dan Destination width bisa berbeda. DMA akan melakukan packing/unpacking otomatis (tergantung arsitektur).

---

## 3. DMA pada STM32F103

STM32F103 memiliki **2 DMA Controller** dengan total 12 Channel:
- **DMA1:** 7 Channel
- **DMA2:** 5 Channel (Hanya di High-density devices, Blue Pill biasanya hanya DMA1 full access).

### 3.1 Mapping Channel DMA1

Setiap peripheral di-hardwire ke channel tertentu (berbeda dengan ESP32/Modern MCU yang pakai Matrix).

| Channel | Periferal Utama |
|---------|-----------------|
| DMA1 Ch1 | **ADC1** |
| DMA1 Ch2 | **SPI1_RX**, USART3_TX |
| DMA1 Ch3 | **SPI1_TX**, USART3_RX |
| DMA1 Ch4 | **USART1_TX**, I2C2_TX |
| DMA1 Ch5 | **USART1_RX**, I2C2_RX |
| DMA1 Ch6 | **I2C1_TX**, TIM3_CH1 |
| DMA1 Ch7 | **I2C1_RX**, TIM2_CH2/4 |

*(Lihat Reference Manual RM0008 Table 78 untuk detail lengkap)*

### 3.2 Prioritas DMA

Jika beberapa channel request DMA bersamaan, arbiter memilih berdasarkan:
1. **Software Priority:** Low, Medium, High, Very High (Configurable).
2. **Hardware Priority:** Channel nomor lebih kecil menang (Ch1 > Ch2).

### 3.3 Interrupt Events

1. **Transfer Complete (TC):** Semua data selesai ditransfer.
2. **Half Transfer (HT):** Setengah buffer terisi. Sangat berguna untuk Double Buffering (Process first half while DMA fills second half).
3. **Transfer Error (TE):** Terjadi kesalahan bus.

---

## 4. DMA pada ESP32

ESP32 memiliki arsitektur DMA yang lebih kompleks dan fleksibel, biasanya terintegrasi dengan peripheral controller (bukan central DMA controller sederhana seperti STM32F1).

### 4.1 General Purpose DMA (GDMA) - ESP32-S3/C3
Pada varian baru, terdapat GDMA. Namun pada ESP32 classic (Xtensa LX6), DMA terikat erat dengan peripheral:

1. **I2S DMA:** Digunakan untuk Audio (I2S), DAC, dan ADC sampling kecepatan tinggi, serta Camera interface dan LCD parallel (8080).
2. **SPI DMA:** Untuk transfer SPI high speed (>10 MHz) dan storage.
3. **RMT (Remote Control):** Memiliki DMA-like behavior untuk signal generation.

### 4.2 Linked List Descriptors
ESP32 DMA menggunakan **Linked List Descriptors**. Kita bisa menyusun rantai buffer yang tersebar di memori, dan DMA akan melompat dari satu buffer ke buffer lain secara otomatis tanpa intervensi CPU. Ini sangat powerful untuk scatter-gather operations.

---

## 5. Implementasi Coding

### 5.1 STM32 HAL DMA — Memory to Memory

Contoh menyalin array `src` ke `dst` tanpa CPU:

```c
#include "stm32f1xx_hal.h"

DMA_HandleTypeDef hdma_mem2mem;
uint32_t src_buffer[256];
uint32_t dst_buffer[256];

void DMA_Mem2Mem_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_mem2mem.Instance                 = DMA1_Channel1;
    hdma_mem2mem.Init.Direction           = DMA_MEMORY_TO_MEMORY;
    hdma_mem2mem.Init.PeriphInc           = DMA_PINC_ENABLE;
    hdma_mem2mem.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_mem2mem.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_mem2mem.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hdma_mem2mem.Init.Mode                = DMA_NORMAL;
    hdma_mem2mem.Init.Priority            = DMA_PRIORITY_MEDIUM;
    HAL_DMA_Init(&hdma_mem2mem);
}

void DMA_Mem2Mem_Start(void)
{
    /* Isi data sumber */
    for (int i = 0; i < 256; i++) {
        src_buffer[i] = i * 0x11;
    }

    /* Mulai transfer DMA (non-blocking) */
    HAL_DMA_Start_IT(&hdma_mem2mem,
                     (uint32_t)src_buffer,
                     (uint32_t)dst_buffer, 256);
}

/* Callback saat transfer selesai */
void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_mem2mem);
}
```

### 5.2 STM32 HAL DMA — ADC Circular

Membaca ADC terus-menerus ke buffer tanpa CPU involvement:

```c
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
uint16_t adc_buffer[100];
volatile uint8_t buffer_ready = 0;

void ADC_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();

    /* DMA untuk ADC */
    hdma_adc1.Instance                 = DMA1_Channel1;
    hdma_adc1.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode                = DMA_CIRCULAR;  /* Terus berulang */
    hdma_adc1.Init.Priority            = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma_adc1);

    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);
}

void ADC_Start(void)
{
    /* ADC berjalan terus + DMA circular → adc_buffer selalu fresh */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer, 100);
}

/* Callback: buffer penuh (100 samples ready) */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    buffer_ready = 1;  /* Proses 50 sample terakhir (index 50-99) */
}

/* Callback: setengah buffer (50 samples ready) */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    buffer_ready = 2;  /* Proses 50 sample pertama (index 0-49) */
}
```

### 5.3 ESP32 DMA — SPI Transfer

ESP32 menggunakan DMA secara transparan melalui driver API:

```c
#include "driver/spi_master.h"

spi_device_handle_t spi_dev;

void spi_dma_init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = GPIO_NUM_23,
        .miso_io_num = GPIO_NUM_19,
        .sclk_io_num = GPIO_NUM_18,
        .max_transfer_sz = 4096,  /* DMA buffer size */
    };
    /* Parameter terakhir = DMA channel (SPI_DMA_CH_AUTO) */
    spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 10 * 1000 * 1000,  /* 10 MHz */
        .mode = 0,
        .spics_io_num = GPIO_NUM_5,
        .queue_size = 4,
    };
    spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_dev);
}

void spi_dma_transfer(uint8_t *tx_data, uint8_t *rx_data, size_t len)
{
    spi_transaction_t trans = {
        .length    = len * 8,      /* Dalam bit */
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };
    /* Transfer via DMA — non-blocking untuk CPU */
    spi_device_transmit(spi_dev, &trans);
}
```

### 5.4 ESP32 DMA — I2S Audio Streaming

```c
#include "driver/i2s_std.h"

i2s_chan_handle_t tx_handle;

void i2s_dma_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num  = 8;     /* Jumlah DMA descriptor */
    chan_cfg.dma_frame_num = 256;   /* Frames per descriptor */
    i2s_new_channel(&chan_cfg, &tx_handle, NULL);

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_16BIT,
                        I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .bclk = GPIO_NUM_26,
            .ws   = GPIO_NUM_25,
            .dout = GPIO_NUM_22,
        },
    };
    i2s_channel_init_std_mode(tx_handle, &std_cfg);
    i2s_channel_enable(tx_handle);
}

/* DMA streaming — CPU hanya menyiapkan buffer */
void audio_stream_task(void *param)
{
    int16_t buffer[512];
    size_t bytes_written;

    while (1) {
        /* Generate atau decode audio ke buffer */
        generate_audio_samples(buffer, 512);

        /* I2S driver menggunakan DMA secara otomatis */
        i2s_channel_write(tx_handle, buffer, sizeof(buffer),
                          &bytes_written, portMAX_DELAY);
    }
}
```

---

## 6. Studi Kasus: Double Buffering (Ping-Pong)

DMA memungkinkan pola **Double Buffering** untuk continuous processing tanpa data loss:

```
Double Buffering Timeline:
┌─────────────────────────────────────────────────────┐
│ Waktu →                                             │
│                                                     │
│ DMA:  [===Fill A===][===Fill B===][===Fill A===]    │
│ CPU:  [  idle  ][==Process A==][==Process B==]      │
│                                                     │
│ Half-Transfer IRQ ──→ CPU proses buffer A           │
│ Transfer Complete ──→ CPU proses buffer B           │
│                                                     │
│ Hasil: Zero-downtime data acquisition!              │
└─────────────────────────────────────────────────────┘
```

**Implementasi Ping-Pong pada STM32:**

```c
#define TOTAL_BUFFER_SIZE  1024
uint16_t dma_buffer[TOTAL_BUFFER_SIZE];  /* Ping + Pong */

volatile enum { PING_READY, PONG_READY, NONE } buffer_state = NONE;

void start_continuous_acquisition(void)
{
    /* DMA Circular mode → otomatis wrap-around */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)dma_buffer, TOTAL_BUFFER_SIZE);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    /* Ping ready: index 0 .. 511 */
    buffer_state = PING_READY;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    /* Pong ready: index 512 .. 1023 */
    buffer_state = PONG_READY;
}

/* Main loop processing */
void process_loop(void)
{
    if (buffer_state == PING_READY) {
        process_data(&dma_buffer[0], TOTAL_BUFFER_SIZE / 2);
        buffer_state = NONE;
    }
    else if (buffer_state == PONG_READY) {
        process_data(&dma_buffer[TOTAL_BUFFER_SIZE / 2],
                     TOTAL_BUFFER_SIZE / 2);
        buffer_state = NONE;
    }
}
```

---

## 7. DMA UART — Transmit dan Receive

Kombinasi DMA + UART sangat umum untuk transfer data besar tanpa blocking CPU:

### 7.1 STM32 UART DMA

```c
uint8_t uart_rx_buffer[256];
uint8_t uart_tx_buffer[256];

void UART_DMA_Init(void)
{
    /* DMA untuk UART TX: DMA1_Channel4 (USART1_TX) */
    /* DMA untuk UART RX: DMA1_Channel5 (USART1_RX) */
    
    /* RX dalam mode circular — selalu menerima data */
    HAL_UART_Receive_DMA(&huart1, uart_rx_buffer, sizeof(uart_rx_buffer));
}

void UART_DMA_Send(const char *msg)
{
    uint16_t len = strlen(msg);
    memcpy(uart_tx_buffer, msg, len);
    HAL_UART_Transmit_DMA(&huart1, uart_tx_buffer, len);
}

/* Idle line detection — mendeteksi akhir frame */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == &huart1) {
        /* Data diterima sebanyak Size bytes */
        uart_rx_buffer[Size] = '\0';
        printf("Received: %s\n", (char *)uart_rx_buffer);
        
        /* Restart DMA reception */
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_rx_buffer,
                                     sizeof(uart_rx_buffer));
    }
}
```

### 7.2 ESP32 UART DMA

ESP32 UART driver sudah menggunakan ring buffer internal, tetapi bisa dikonfigurasi untuk menggunakan DMA:

```c
#include "driver/uart.h"

void uart_dma_init(void)
{
    uart_config_t uart_cfg = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM_1, &uart_cfg);
    uart_set_pin(UART_NUM_1, 17, 16, -1, -1);
    
    /* Buffer size besar untuk DMA-like behavior */
    uart_driver_install(UART_NUM_1, 2048, 2048, 20, NULL, 0);
}
```

---

## 8. Best Practices DMA

### 8.1 Design Guidelines

1. **Gunakan DMA untuk transfer > 8 bytes** — overhead setup DMA tidak worth it untuk data kecil
2. **Circular mode untuk periodic data** — ADC sampling, audio streaming
3. **Normal mode untuk one-shot transfer** — SPI flash read/write
4. **Selalu enable DMA clock** sebelum konfigurasi
5. **Perhatikan alignment** — Word transfer butuh alamat kelipatan 4

### 8.2 Kapan Menggunakan DMA vs CPU Copy?

| Skenario | Metode | Alasan |
|----------|--------|--------|
| Salin 4 byte | CPU | Overhead DMA terlalu besar |
| Salin 1KB buffer | DMA | CPU bebas selama transfer |
| ADC 10 SPS | CPU polling | Jarang, DMA overkill |
| ADC 100 kSPS | DMA Circular | CPU tidak mampu polling secepat ini |
| UART 9600 baud | CPU/Interrupt | Data rate rendah |
| UART 921600 baud | DMA | High throughput |
| SPI Flash 1MB | DMA | Transfer besar |
| Audio 44.1kHz stereo | DMA + I2S | Continuous stream |

### 8.3 Menghindari Masalah Umum

```c
/* ❌ SALAH: Buffer di stack (akan corrupt setelah function return) */
void bad_dma_example(void)
{
    uint8_t buffer[100];  /* Stack variable! */
    HAL_UART_Transmit_DMA(&huart1, buffer, 100);
    /* DMA masih membaca buffer saat function sudah return! */
}

/* ✅ BENAR: Buffer global atau static */
static uint8_t buffer[100];  /* Persistent memory */
void good_dma_example(void)
{
    memcpy(buffer, data, 100);
    HAL_UART_Transmit_DMA(&huart1, buffer, 100);
}
```

---

## 9. Daftar Program Praktikum

| No | Platform | Nama Program | Topik | Tingkat |
|----|----------|-------------|-------|---------|
| 01 | ESP32 | DMA_Mem_Copy | Memory-to-memory copy | Dasar |
| 02 | ESP32 | DMA_ADC | ADC continuous via DMA | Menengah |
| 03 | ESP32 | DMA_SPI | SPI transfer via DMA | Menengah |
| 04 | ESP32 | DMA_I2S | Audio streaming via I2S DMA | Lanjut |
| 05 | ESP32 | DMA_UART | UART high-speed via DMA | Menengah |
| 06 | ESP32 | DMA_PingPong | Double buffering pattern | Lanjut |
| 07 | STM32 | DMA_Mem_Copy | Memory-to-memory copy | Dasar |
| 08 | STM32 | DMA_ADC | ADC circular DMA | Menengah |
| 09 | STM32 | DMA_SPI | SPI DMA transfer | Menengah |
| 10 | STM32 | DMA_UART | UART DMA TX/RX | Menengah |
| 11 | STM32 | DMA_PingPong | Double buffering pattern | Lanjut |
| 12 | STM32 | DMA_Multi_Channel | Multi-channel DMA setup | Lanjut |

---

## 10. Troubleshooting DMA

| Gejala | Penyebab Umum | Solusi |
|--------|---------------|--------|
| Data tidak tersalin | Clock DMA belum enable | `__HAL_RCC_DMA1_CLK_ENABLE()` |
| Data berantakan/shift | Data alignment salah | Cek Byte/HalfWord/Word alignment |
| Transfer berhenti | Normal Mode (bukan Circular) | Restart DMA atau gunakan Circular |
| Hard Fault | Akses alamat invalid | Pastikan pointer valid dan size benar |
| Buffer corruption | Buffer di stack, bukan global | Gunakan static/global buffer |
| DMA conflict | Channel dipakai peripheral lain | Cek mapping table di reference manual |
| Interrupt tidak fire | NVIC belum enable | `HAL_NVIC_EnableIRQ(DMAx_Channelx_IRQn)` |

---

## Referensi

1. **STM32F103 Reference Manual (RM0008)** — Chapter 10: DMA Controller
2. **Mastering STM32** (Carmine Noviello) — Chapter 11: Direct Memory Access
3. **AN2548** — Using the STM32F0/F1/F3 DMA controller
4. **ESP-IDF Programming Guide** — General Purpose DMA, I2S DMA
5. **Kolban's Book on ESP32** — DMA sections

5. **FreeRTOS & DMA**: Handling DMA interrupts in RTOS environment.

