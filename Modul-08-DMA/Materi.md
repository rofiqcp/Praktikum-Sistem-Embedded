# BAB 08: Direct Memory Access (DMA)

## 🎯 Capaian Pembelajaran

Setelah menyelesaikan bab ini, mahasiswa diharapkan mampu:

1. Memahami arsitektur dan prinsip kerja Direct Memory Access (DMA).
2. Mengkonfigurasi DMA Controller pada STM32F103 (Channels, Priorities, Data Width).
3. Mengimplementasikan transfer Memory-to-Memory, Peripheral-to-Memory, dan Memory-to-Peripheral.
4. Menggunakan DMA Circular Mode untuk penanganan data stream kontinu (ADC, UART).
5. Memahami implementasi DMA pada ESP32 (General Purpose DMA).
6. Menganalisis peningkatan efisiensi CPU saat menggunakan DMA.

---

## 📚 Materi Pembelajaran

### 1. Pendahuluan DMA

#### 1.1 Apa itu DMA?

**Direct Memory Access (DMA)** adalah fitur hardware pada mikrokontroler modern yang memungkinkan periferal (seperti ADC, UART, SPI) untuk mentransfer data langsung ke memori (RAM) atau sebaliknya, **tanpa intervensi CPU**.

**Analogi:**
- **CPU Transfer:** CEO (CPU) memindahkan dokumen satu per satu dari mesin fax ke lemari arsip. CEO tidak bisa meeting atau kerja lain.
- **DMA Transfer:** CEO menyuruh Office Boy (DMA Controller) memindahkan dokumen. CEO bebas melakukan meeting (kalkulasi math, logic, UI update) sementara OB bekerja di background.

#### 1.2 Mengapa DMA Penting?

1. **Efisiensi CPU:** Membebaskan CPU untuk melakukan pemrosesan data, bukan pemindahan data.
2. **Kecepatan Tinggi:** Transfer data bisa dilakukan secepat bus memori, jauh lebih cepat dari loop software `for(i=0; i<N; i++)`.
3. **Low Power:** CPU bisa masuk mode sleep (Sleep/WFI) sementara DMA terus bekerja.
4. **Latency Deterministik:** Transfer data tidak terganggu oleh interupsi lain (kecuali bus contention).

---

### 2. Arsitektur DMA

#### 2.1 Komponen Utama

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

#### 2.2 Transfer Modes

1. **Normal Mode:** DMA berhenti setelah mentransfer sejumlah N data. Harus di-trigger ulang manual. Cocok untuk buffer tunggal.
2. **Circular Mode:** Setelah transfer selesai, pointer kembali ke awal buffer dan transfer berlanjut otomatis. Cocok untuk Ring Buffer, Audio Stream, ADC Continuous Scan.

#### 2.3 Data Width & Alignment

DMA mendukung lebar data berbeda:
- **Byte (8-bit):** `uint8_t`
- **Half-Word (16-bit):** `uint16_t`
- **Word (32-bit):** `uint32_t`

**PENTING:** Source dan Destination width bisa berbeda. DMA akan melakukan packing/unpacking otomatis (tergantung arsitektur).

---

### 3. DMA pada STM32F103

STM32F103 memiliki **2 DMA Controller** dengan total 12 Channel:
- **DMA1:** 7 Channel
- **DMA2:** 5 Channel (Hanya di High-density devices, Blue Pill biasanya hanya DMA1 full access).

#### 3.1 Mapping Channel DMA1

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

#### 3.2 Prioritas DMA

Jika beberapa channel request DMA bersamaan, arbiter memilih berdasarkan:
1. **Software Priority:** Low, Medium, High, Very High (Configurable).
2. **Hardware Priority:** Channel nomor lebih kecil menang (Ch1 > Ch2).

#### 3.3 Interrupt Events

1. **Transfer Complete (TC):** Semua data selesai ditransfer.
2. **Half Transfer (HT):** Setengah buffer terisi. Sangat berguna untuk Double Buffering (Process first half while DMA fills second half).
3. **Transfer Error (TE):** Terjadi kesalahan bus.

---

### 4. DMA pada ESP32

ESP32 memiliki arsitektur DMA yang lebih kompleks dan fleksibel, biasanya terintegrasi dengan peripheral controller (bukan central DMA controller sederhana seperti STM32F1).

#### 4.1 General Purpose DMA (GDMA) - ESP32-S3/C3
Pada varian baru, terdapat GDMA. Namun pada ESP32 classic (Xtensa LX6), DMA terikat erat dengan peripheral:

1. **I2S DMA:** Digunakan untuk Audio (I2S), DAC, dan ADC sampling kecepatan tinggi, serta Camera interface dan LCD parallel (8080).
2. **SPI DMA:** Untuk transfer SPI high speed (>10 MHz) dan storage.
3. **RMT (Remote Control):** Memiliki DMA-like behavior untuk signal generation.

#### 4.2 Linked List Descriptors
ESP32 DMA menggunakan **Linked List Descriptors**. Kita bisa menyusun rantai buffer yang tersebar di memori, dan DMA akan melompat dari satu buffer ke buffer lain secara otomatis tanpa intervensi CPU. Ini sangat powerful untuk scatter-gather operations.

---

### 5. Implementasi Coding

#### 5.1 STM32 HAL DMA - Memory to Memory
Contoh menyalin array `src` ke `dst`.

```c
/* Init */
DMA_HandleTypeDef hdma_memtomem_dma1_channel1;
hdma_memtomem_dma1_channel1.Instance = DMA1_Channel1;
hdma_memtomem_dma1_channel1.Init.Direction = DMA_MEMORY_TO_MEMORY;
hdma_memtomem_dma1_channel1.Init.PeriphInc = DMA_PINC_ENABLE; // Increment Source
hdma_memtomem_dma1_channel1.Init.MemInc = DMA_MINC_ENABLE;    // Increment Dest
hdma_memtomem_dma1_channel1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
hdma_memtomem_dma1_channel1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
hdma_memtomem_dma1_channel1.Init.Mode = DMA_NORMAL;
hdma_memtomem_dma1_channel1.Init.Priority = DMA_PRIORITY_LOW;
HAL_DMA_Init(&hdma_memtomem_dma1_channel1);

/* Start Transfer */
HAL_DMA_Start(&hdma_memtomem_dma1_channel1, (uint32_t)srcBuffer, 
              (uint32_t)dstBuffer, BUFFER_SIZE);

/* Wait for completion (Polling) */
HAL_DMA_PollForTransfer(&hdma_memtomem_dma1_channel1, HAL_DMA_FULL_TRANSFER, 1000);
```

#### 5.2 STM32 HAL DMA - ADC Circular
Membaca ADC terus menerus ke buffer tanpa CPU.

```c
/* Main Loop */
uint16_t adc_buffer[100];

// Start ADC in DMA mode
HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, 100);

// Data di adc_buffer akan update otomatis terus menerus
// Kita bisa baca kapan saja, atau gunakan interrupt:

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    // Buffer penuh (100 samples ready)
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
    // Setengah buffer penuh (50 samples ready)
}
```

---

### 6. Studi Kasus: Audio Processing / Sensor Logging

DMA memungkinkan pola **Double Buffering (Ping-Pong Buffering)**:

1. Buat Buffer ukuran 2N (misal 1024 byte).
2. Set DMA Circular Mode.
3. Saat interrupt **Half Complete (HT)** (byte 0-511 terisi), CPU memproses bagian pertama data tersebut (Ping) sambil DMA mengisi bagian kedua (Pong).
4. Saat interrupt **Transfer Complete (TC)** (byte 512-1023 terisi), CPU memproses bagian kedua sambil DMA overwrite bagian pertama (Ping).

Hasil: **Continuous Processing** tanpa henti dan tanpa data loss.

---

### 7. Troubleshooting DMA

| Gejala | Penyebab Umum | Solusi |
|--------|---------------|--------|
| Data tidak tersalin | Clock DMA belum enable | `__HAL_RCC_DMA1_CLK_ENABLE()` |
| Data berantakan/shift | Data alignment salah | Cek Byte/HalfWord/Word alignment |
| Transfer berhenti | Normal Mode (bukan Circular) | Restart DMA atau gunakan Circular |
| Hard Fault | Akses alamat invalid | Pastikan pointer valid dan size benar |
| Cache coherency (F7/H7) | Data di cache belum flush ke RAM | Gunakan `SCB_CleanDCache()` (Not applicable for F103) |

---

## 📖 Referensi

1. **STM32F103 Reference Manual (RM0008)**: Chapter 10 DMA Controller.
2. **Mastering STM32** (Carmine Noviello): Chapter 11 Direct Memory Access.
3. **AN2548**: Using the STM32F0/F1/F3 DMA controller.
4. **Espressif API Guide**: General Purpose DMA.
5. **FreeRTOS & DMA**: Handling DMA interrupts in RTOS environment.

