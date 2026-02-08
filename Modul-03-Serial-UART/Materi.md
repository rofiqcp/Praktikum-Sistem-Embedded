# BAB 03: Serial UART Communication

## 🎯 Capaian Pembelajaran

Setelah menyelesaikan bab ini, mahasiswa diharapkan mampu:

1. Memahami prinsip dasar komunikasi serial asynchronous (UART)
2. Menguasai konfigurasi parameter UART (baud rate, data bits, parity, stop bits)
3. Mengimplementasikan komunikasi UART pada STM32 dan ESP32
4. Membangun protokol komunikasi sederhana antar-MCU
5. Mengintegrasikan UART dengan interrupt untuk komunikasi non-blocking
6. Menerapkan teknik buffer management dan error handling

---

## 📚 Materi Pembelajaran

### 1. Pendahuluan Komunikasi Serial

#### 1.1 Apa itu Komunikasi Serial?

**Komunikasi Serial** adalah metode transmisi data di mana bit-bit data dikirim secara berurutan (satu per satu) melalui satu jalur komunikasi. Berbeda dengan komunikasi paralel yang mengirim multiple bit sekaligus, serial communication lebih efisien untuk jarak jauh karena hanya memerlukan sedikit kabel.

```
Parallel Communication (8-bit):
┌────────────────────────────────────────┐
│  D7 ════════════════════════════►      │
│  D6 ════════════════════════════►      │
│  D5 ════════════════════════════►      │
│  D4 ════════════════════════════►      │  8 wires
│  D3 ════════════════════════════►      │
│  D2 ════════════════════════════►      │
│  D1 ════════════════════════════►      │
│  D0 ════════════════════════════►      │
└────────────────────────────────────────┘

Serial Communication:
┌────────────────────────────────────────┐
│  TX ═══════[D7][D6][D5][D4][D3][D2][D1][D0]═══►  1 wire
└────────────────────────────────────────┘
```

#### 1.2 Jenis Komunikasi Serial

| Tipe | Synchronous | Asynchronous |
|------|-------------|--------------|
| **Clock** | Shared clock line | No clock, timing agreement |
| **Speed** | Faster | Slower |
| **Wires** | Data + Clock | Data only |
| **Contoh** | SPI, I2C | UART, RS-232 |
| **Kompleksitas** | Higher | Lower |

#### 1.3 UART (Universal Asynchronous Receiver-Transmitter)

**UART** adalah protokol komunikasi serial asynchronous yang paling umum digunakan. Disebut "asynchronous" karena tidak memerlukan sinyal clock bersama - kedua device harus sepakat tentang timing (baud rate) sebelumnya.

**Karakteristik UART:**
- Full-duplex (TX dan RX bersamaan)
- Point-to-point (1 transmitter → 1 receiver)
- Tidak memerlukan clock line
- Self-clocking menggunakan start/stop bits

---

### 2. Frame Data UART

#### 2.1 Struktur Frame

```
┌────────────────────────────────────────────────────────────────┐
│                       UART Frame Structure                      │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  IDLE  │ START │ D0 │ D1 │ D2 │ D3 │ D4 │ D5 │ D6 │ D7 │ P │ STOP │ IDLE
│  (HIGH)│  (0)  │    │    │    │    │    │    │    │    │   │  (1) │ (HIGH)
│        │       │◄────────── DATA BITS (5-9) ─────────►│   │      │
│        │       │                                       │   │      │
│        │◄─1 bit─►                                      │◄opt►◄1-2 bits►
│                                                                │
└────────────────────────────────────────────────────────────────┘

Timing (9600 baud, 8N1):
├── 104.17µs ──┼── 104.17µs ──┼─ ... ─┼── 104.17µs ──┤
│   Start Bit  │    D0        │       │   Stop Bit   │
│              │              │       │              │
Total Frame = 10 bits × 104.17µs = 1.04ms per byte
```

#### 2.2 Komponen Frame

| Komponen | Fungsi | Keterangan |
|----------|--------|------------|
| **Idle** | Line dalam keadaan diam | Selalu HIGH (logic 1) |
| **Start Bit** | Menandai awal data | Selalu LOW (logic 0), 1 bit |
| **Data Bits** | Payload data | 5-9 bits (biasanya 8) |
| **Parity Bit** | Error detection | Optional: Even, Odd, None |
| **Stop Bit** | Menandai akhir frame | HIGH, 1 atau 2 bits |

#### 2.3 Baud Rate

**Baud Rate** adalah jumlah simbol (bit) yang ditransmisikan per detik. Kedua device HARUS menggunakan baud rate yang sama.

**Baud Rate Standar:**
| Baud Rate | Bit Time | Byte Time (8N1) | Keterangan |
|-----------|----------|-----------------|------------|
| 9600 | 104.17 µs | 1.04 ms | Legacy, reliable |
| 19200 | 52.08 µs | 0.52 ms | Common |
| 38400 | 26.04 µs | 0.26 ms | Common |
| 57600 | 17.36 µs | 0.17 ms | Common |
| 115200 | 8.68 µs | 86.8 µs | Modern standard |
| 230400 | 4.34 µs | 43.4 µs | High speed |
| 460800 | 2.17 µs | 21.7 µs | Very high speed |
| 921600 | 1.09 µs | 10.9 µs | Maximum common |

**Perhitungan Throughput:**
```
Throughput (8N1) = Baud_Rate / 10 bits per byte
                 = 115200 / 10
                 = 11,520 bytes/second
                 = 11.25 KB/s
```

#### 2.4 Notasi Konfigurasi

Format: **[Data Bits][Parity][Stop Bits]**

Contoh umum:
- **8N1**: 8 data bits, No parity, 1 stop bit (paling umum)
- **8E1**: 8 data bits, Even parity, 1 stop bit
- **7O2**: 7 data bits, Odd parity, 2 stop bits

---

### 3. UART pada STM32F103C8T6

#### 3.1 USART Peripheral STM32

STM32F103C8T6 memiliki 3 USART (Universal Synchronous/Asynchronous Receiver-Transmitter):

| USART | TX Pin | RX Pin | Clock | Fitur |
|-------|--------|--------|-------|-------|
| USART1 | PA9 | PA10 | APB2 (72MHz) | Full speed |
| USART2 | PA2 | PA3 | APB1 (36MHz) | Standard |
| USART3 | PB10 | PB11 | APB1 (36MHz) | Standard |

```
┌─────────────────────────────────────────────────────────────┐
│              STM32F103 USART Block Diagram                   │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  APB Clock ──► Baud Rate Generator ──► TX Shift Register    │
│                      │                       │              │
│                      │                       ▼              │
│                      │              TX Data Register        │
│                      │                       │              │
│                      ▼                       ▼              │
│               Clock Divider            TX Pin (PA9)         │
│                      │                                      │
│                      │                                      │
│               RX Pin (PA10)                                 │
│                      │                                      │
│                      ▼                                      │
│              RX Shift Register                              │
│                      │                                      │
│                      ▼                                      │
│              RX Data Register ──► NVIC (Interrupt)          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 3.2 Perhitungan Baud Rate STM32

```
USARTDIV = f_CLK / (16 × Baud_Rate)

Contoh: USART1 @ 115200 baud, f_CLK = 72MHz

USARTDIV = 72,000,000 / (16 × 115200)
         = 72,000,000 / 1,843,200
         = 39.0625

Register BRR = Mantissa.Fraction
Mantissa = 39 = 0x27
Fraction = 0.0625 × 16 = 1 = 0x1

BRR = 0x271
```

#### 3.3 Konfigurasi Register STM32 USART

```c
// USART1 Initialization @ 115200 baud, 8N1

// 1. Enable clocks
RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_IOPAEN;

// 2. Configure GPIO
// PA9 (TX) - Alternate function push-pull, 50MHz
GPIOA->CRH &= ~(GPIO_CRH_CNF9 | GPIO_CRH_MODE9);
GPIOA->CRH |= GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9;

// PA10 (RX) - Input floating
GPIOA->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
GPIOA->CRH |= GPIO_CRH_CNF10_0;

// 3. Set baud rate (115200 @ 72MHz)
USART1->BRR = 0x271;  // 39.0625

// 4. Configure frame format (8N1)
USART1->CR1 = 0;  // 8 data bits, no parity
USART1->CR2 = 0;  // 1 stop bit

// 5. Enable TX, RX, and USART
USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
```

#### 3.4 Fungsi Kirim dan Terima STM32

```c
// Blocking transmit
void USART1_SendChar(char c) {
    while (!(USART1->SR & USART_SR_TXE));  // Wait until TX empty
    USART1->DR = c;
}

void USART1_SendString(const char* str) {
    while (*str) {
        USART1_SendChar(*str++);
    }
}

// Blocking receive
char USART1_ReceiveChar(void) {
    while (!(USART1->SR & USART_SR_RXNE));  // Wait until RX not empty
    return USART1->DR;
}

// Non-blocking receive (polling)
int USART1_Available(void) {
    return (USART1->SR & USART_SR_RXNE) ? 1 : 0;
}
```

#### 3.5 UART dengan Interrupt STM32

```c
// Enable RXNE interrupt
void USART1_EnableInterrupt(void) {
    USART1->CR1 |= USART_CR1_RXNEIE;
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 2);
}

// Interrupt handler
volatile char rxBuffer[256];
volatile uint8_t rxIndex = 0;

void USART1_IRQHandler(void) {
    if (USART1->SR & USART_SR_RXNE) {
        char received = USART1->DR;
        
        if (rxIndex < sizeof(rxBuffer) - 1) {
            rxBuffer[rxIndex++] = received;
        }
        
        // Check for end of message (newline)
        if (received == '\n') {
            rxBuffer[rxIndex] = '\0';
            processMessage();
            rxIndex = 0;
        }
    }
}
```

---

### 4. UART pada ESP32

#### 4.1 UART Peripheral ESP32

ESP32 memiliki 3 UART:

| UART | TX Default | RX Default | Keterangan |
|------|------------|------------|------------|
| UART0 | GPIO1 | GPIO3 | USB-Serial (programming) |
| UART1 | GPIO10 | GPIO9 | User available |
| UART2 | GPIO17 | GPIO16 | User available |

**Catatan:** UART0 biasanya digunakan untuk programming dan Serial Monitor. Gunakan UART1 atau UART2 untuk komunikasi dengan device lain.

#### 4.2 Konfigurasi UART ESP32 (ESP-IDF)

```c
#include "driver/uart.h"
#include "esp_log.h"

#define UART_PORT      UART_NUM_2
#define TX_PIN         GPIO_NUM_17
#define RX_PIN         GPIO_NUM_16
#define BUF_SIZE       256

static const char *TAG = "UART";

void uart_init(void) {
    // Konfigurasi parameter UART
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install UART driver dengan TX & RX buffer
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, TX_PIN, RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART%d initialized: 115200 8N1", UART_PORT);
}
```

#### 4.3 Fungsi UART ESP-IDF

```c
// === Transmit ===
// Kirim string
uart_write_bytes(UART_PORT, "Hello\n", 6);

// Kirim formatted string
char buf[64];
int len = snprintf(buf, sizeof(buf), "Value: %d\n", x);
uart_write_bytes(UART_PORT, buf, len);

// Kirim raw byte
uint8_t byte = 0x55;
uart_write_bytes(UART_PORT, (const char *)&byte, 1);

// Kirim buffer
uart_write_bytes(UART_PORT, (const char *)buffer, length);

// === Receive ===
uint8_t data[128];
// Blocking read dengan timeout (100ms = 100 / portTICK_PERIOD_MS)
int rxBytes = uart_read_bytes(UART_PORT, data, sizeof(data),
                              100 / portTICK_PERIOD_MS);
if (rxBytes > 0) {
    data[rxBytes] = '\0';  // Null-terminate jika string
    ESP_LOGI(TAG, "Received %d bytes: %s", rxBytes, data);
}

// === Buffer management ===
size_t buffered;
uart_get_buffered_data_len(UART_PORT, &buffered);  // Bytes in RX buffer
uart_wait_tx_done(UART_PORT, 100 / portTICK_PERIOD_MS);  // Wait TX complete
uart_flush(UART_PORT);  // Flush RX buffer
```

#### 4.4 UART Event-Driven dengan ESP-IDF

```c
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define UART_PORT   UART_NUM_2
#define BUF_SIZE    256

static QueueHandle_t uart_queue;

void uart_event_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_PORT, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_queue, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, GPIO_NUM_17, GPIO_NUM_16,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

static void uart_event_task(void *pvParameters) {
    uart_event_t event;
    uint8_t data[BUF_SIZE];

    for (;;) {
        if (xQueueReceive(uart_queue, &event, portMAX_DELAY)) {
            switch (event.type) {
            case UART_DATA:
                int len = uart_read_bytes(UART_PORT, data, event.size,
                                          100 / portTICK_PERIOD_MS);
                if (len > 0) {
                    data[len] = '\0';
                    ESP_LOGI(TAG, "Received: %s", (char *)data);
                }
                break;
            case UART_FIFO_OVF:
                ESP_LOGW(TAG, "FIFO overflow");
                uart_flush_input(UART_PORT);
                break;
            case UART_BUFFER_FULL:
                ESP_LOGW(TAG, "Buffer full");
                uart_flush_input(UART_PORT);
                break;
            default:
                break;
            }
        }
    }
}
```

---

### 5. Komunikasi MCU-to-MCU

#### 5.1 Koneksi Hardware

```
┌───────────────────┐         ┌───────────────────┐
│     STM32         │         │      ESP32        │
│                   │         │                   │
│   PA9 (TX) ───────┼────────►│ GPIO16 (RX)       │
│   PA10 (RX) ◄─────┼─────────│ GPIO17 (TX)       │
│   GND ────────────┼─────────│ GND               │
│                   │         │                   │
└───────────────────┘         └───────────────────┘

PENTING:
1. TX connects to RX (crossed)
2. GND MUST be connected (common reference)
3. Voltage levels must match (3.3V - 3.3V) ✓
4. Both devices use same baud rate
```

#### 5.2 Level Shifting (jika diperlukan)

Jika voltages berbeda (misal 5V Arduino ke 3.3V ESP32):

```
5V Device                3.3V Device
   │                         │
   TX───[1kΩ]───┬───[2kΩ]───GND
                │
                └───────────RX
                
Voltage divider: 5V × 2k/(1k+2k) = 3.3V ✓
```

#### 5.3 Protokol Komunikasi Sederhana

**Text-based Protocol:**
```
Format: $CMD,PARAM1,PARAM2*CHECKSUM\r\n

Contoh:
$LED,ON,1*A5\r\n     → Turn on LED 1
$TEMP,GET*B2\r\n     → Request temperature
$DATA,25.5,60*C3\r\n → Send temperature and humidity
```

**Implementation:**
```c
// STM32 (Sender) - using HAL
void sendCommand(UART_HandleTypeDef *huart, const char *cmd) {
    char buffer[64];
    uint8_t checksum = calculateChecksum(cmd);
    int len = snprintf(buffer, sizeof(buffer), "$%s*%02X\r\n", cmd, checksum);
    HAL_UART_Transmit(huart, (uint8_t *)buffer, len, HAL_MAX_DELAY);
}

// ESP32 (Receiver) - using ESP-IDF
int readCommand(char *out, size_t maxLen) {
    uint8_t data[128];
    int len = uart_read_bytes(UART_NUM_2, data, sizeof(data),
                              100 / portTICK_PERIOD_MS);
    if (len <= 0) return 0;
    data[len] = '\0';

    char *start = strchr((char *)data, '$');
    char *star  = strchr((char *)data, '*');
    if (!start || !star || star <= start) return 0;

    // Extract payload between '$' and '*'
    size_t payloadLen = star - start - 1;
    memcpy(out, start + 1, payloadLen);
    out[payloadLen] = '\0';

    // Verify checksum
    uint8_t rxChecksum = (uint8_t)strtol(star + 1, NULL, 16);
    uint8_t calcChecksum = calculateChecksum(out);
    if (rxChecksum != calcChecksum) return -1;  // Checksum error

    return (int)payloadLen;  // Valid command
}
```

---

### 6. Advanced UART Topics

#### 6.1 Circular Buffer Implementation

```cpp
// Ring buffer untuk non-blocking UART
#define BUFFER_SIZE 256

typedef struct {
    volatile uint8_t buffer[BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} RingBuffer;

RingBuffer rxBuffer = {0};

// Add byte to buffer (called from ISR)
void bufferPush(RingBuffer* buf, uint8_t data) {
    uint16_t next = (buf->head + 1) % BUFFER_SIZE;
    if (next != buf->tail) {  // Not full
        buf->buffer[buf->head] = data;
        buf->head = next;
    }
}

// Get byte from buffer (called from main)
int bufferPop(RingBuffer* buf) {
    if (buf->head == buf->tail) return -1;  // Empty
    uint8_t data = buf->buffer[buf->tail];
    buf->tail = (buf->tail + 1) % BUFFER_SIZE;
    return data;
}

// Available bytes
uint16_t bufferAvailable(RingBuffer* buf) {
    return (buf->head - buf->tail + BUFFER_SIZE) % BUFFER_SIZE;
}
```

#### 6.2 DMA-based UART (STM32)

```c
// DMA configuration untuk UART TX
void USART_DMA_Init(void) {
    // Enable DMA1 clock
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    
    // Configure DMA1 Channel 4 (USART1_TX)
    DMA1_Channel4->CCR = 0;
    DMA1_Channel4->CCR |= DMA_CCR_MINC |    // Memory increment
                         DMA_CCR_DIR |     // Memory to peripheral
                         DMA_CCR_TCIE;     // Transfer complete interrupt
    
    DMA1_Channel4->CPAR = (uint32_t)&USART1->DR;  // Peripheral address
    
    // Enable DMA for USART1 TX
    USART1->CR3 |= USART_CR3_DMAT;
}

// Send data using DMA
void USART_DMA_Send(const uint8_t* data, uint16_t length) {
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;  // Disable
    DMA1_Channel4->CMAR = (uint32_t)data;
    DMA1_Channel4->CNDTR = length;
    DMA1_Channel4->CCR |= DMA_CCR_EN;   // Enable
}
```

#### 6.3 Error Detection dan Handling

```c
// UART Error Flags STM32 (HAL-based)
void checkUARTErrors(UART_HandleTypeDef *huart) {
    uint32_t sr = huart->Instance->SR;
    
    if (sr & USART_SR_ORE) {
        // Overrun Error - data lost
        printf("Overrun!\n");
        volatile uint32_t dummy = huart->Instance->DR;  // Clear
    }
    
    if (sr & USART_SR_FE) {
        // Framing Error - wrong stop bit
        printf("Frame Error!\n");
    }
    
    if (sr & USART_SR_NE) {
        // Noise Error - noise detected
        printf("Noise Error!\n");
    }
    
    if (sr & USART_SR_PE) {
        // Parity Error - parity mismatch
        printf("Parity Error!\n");
    }
}

// XOR Checksum calculation
uint8_t calculateChecksum(const char* data) {
    uint8_t checksum = 0;
    while (*data) {
        checksum ^= *data++;
    }
    return checksum;
}
```

---

### 7. Perbandingan UART: STM32 vs ESP32

| Aspek | STM32F103C8T6 | ESP32 |
|-------|---------------|-------|
| **Jumlah UART** | 3 (USART1-3) | 3 (UART0-2) |
| **Max Baud Rate** | 4.5 Mbps | 5 Mbps |
| **Hardware FIFO** | No (single buffer) | 128 bytes |
| **DMA Support** | Yes | Yes |
| **TX Default Pins** | PA9, PA2, PB10 | GPIO1, GPIO10, GPIO17 |
| **RX Default Pins** | PA10, PA3, PB11 | GPIO3, GPIO9, GPIO16 |
| **Pin Remapping** | Limited (AFIO) | Full flexibility |
| **Interrupt Types** | RXNE, TXE, TC | Multiple events |
| **Flow Control** | CTS/RTS available | CTS/RTS available |

---

### 8. Best Practices UART

#### 8.1 Design Guidelines

1. **Gunakan Baud Rate Standar**
   - Stick dengan 9600, 115200, atau rates umum lainnya
   - Hindari custom rates yang sulit di-match

2. **Selalu Connect Ground**
   - GND adalah referensi voltage - WAJIB connected
   - Tanpa common ground, komunikasi tidak akan bekerja

3. **Buffer Management**
   - Gunakan circular buffer untuk RX
   - Process data di main loop, bukan ISR
   - Handle buffer overflow gracefully

4. **Error Handling**
   - Implementasi checksum atau CRC
   - Timeout untuk incomplete messages
   - Retry mechanism untuk critical data

#### 8.2 Common Mistakes

```c
// ❌ SALAH: Blocking di ISR
void USART1_IRQHandler(void) {
    char c = USART1->DR;
    printf("%c", c);       // BLOCKING!
    HAL_Delay(10);         // BLOCKING!
}

// ✓ BENAR: Non-blocking ISR
volatile bool dataReady = false;
volatile char receivedChar;

void USART1_IRQHandler(void) {
    receivedChar = USART1->DR;
    dataReady = true;  // Set flag only
}

// Dalam main loop
while (1) {
    if (dataReady) {
        dataReady = false;
        processChar(receivedChar);  // Process in main
    }
}
```

```c
// ❌ SALAH: Dynamic allocation di ISR
void USART1_IRQHandler(void) {
    char *cmd = malloc(64);  // Dynamic memory!
    // ... string operations in ISR
}

// ✓ BENAR: Use buffer dan process later
volatile char cmdBuffer[64];
volatile bool cmdComplete = false;

void USART1_IRQHandler(void) {
    static uint8_t idx = 0;
    char c = USART1->DR;
    
    if (c == '\n') {
        cmdBuffer[idx] = '\0';
        cmdComplete = true;
        idx = 0;
    } else if (idx < sizeof(cmdBuffer)-1) {
        cmdBuffer[idx++] = c;
    }
}
```

#### 8.3 Debugging Tips

1. **Gunakan Logic Analyzer**
   - Verify actual baud rate
   - Check frame timing
   - Identify bit errors

2. **Echo Test**
   - Send data, expect echo back
   - Verifikasi RX dan TX works

3. **Known Pattern Test**
   - Send "UUUUU" (0x55 = alternating bits)
   - Easy to see on oscilloscope

4. **Loopback Test**
   - Connect TX to RX on same device
   - Test software layer

---

### 9. Aplikasi Praktis

#### 9.1 Serial Command Parser

```c
// Struktur command parser
typedef struct {
    const char *command;
    void (*handler)(char *params);
} CommandEntry;

void cmdLED(char *params) {
    if (strcmp(params, "ON") == 0)
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
}

void cmdADC(char *params) {
    int pin = atoi(params);
    // Start ADC conversion and read value
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    uint32_t value = HAL_ADC_GetValue(&hadc1);
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "ADC%d=%lu\n", pin, value);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, HAL_MAX_DELAY);
}

CommandEntry commands[] = {
    {"LED", cmdLED},
    {"ADC", cmdADC},
    {NULL, NULL}
};

void processCommand(char *input) {
    char *cmd = strtok(input, " ");
    char *params = strtok(NULL, "");
    
    for (int i = 0; commands[i].command != NULL; i++) {
        if (strcmp(cmd, commands[i].command) == 0) {
            commands[i].handler(params);
            return;
        }
    }
    const char *msg = "Unknown command\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}
```

#### 9.2 Data Logging Protocol

```c
// Structured data packet
typedef struct __attribute__((packed)) {
    uint8_t header;      // 0xAA
    uint8_t type;        // Packet type
    uint16_t length;     // Data length
    uint8_t data[60];    // Payload
    uint16_t crc;        // CRC-16
} DataPacket;

// ESP-IDF: send packet via UART
void sendPacket(uint8_t type, uint8_t *data, uint16_t len) {
    DataPacket pkt;
    pkt.header = 0xAA;
    pkt.type = type;
    pkt.length = len;
    memcpy(pkt.data, data, len);
    pkt.crc = calculateCRC16(&pkt, sizeof(pkt) - 2);
    
    uart_write_bytes(UART_NUM_2, (const char *)&pkt, sizeof(pkt));
}

// STM32 HAL: send packet via UART
void sendPacket_HAL(UART_HandleTypeDef *huart, uint8_t type,
                    uint8_t *data, uint16_t len) {
    DataPacket pkt;
    pkt.header = 0xAA;
    pkt.type = type;
    pkt.length = len;
    memcpy(pkt.data, data, len);
    pkt.crc = calculateCRC16(&pkt, sizeof(pkt) - 2);
    
    HAL_UART_Transmit(huart, (uint8_t *)&pkt, sizeof(pkt), HAL_MAX_DELAY);
}
```

---

## 📖 Referensi

### Dokumentasi Resmi
1. **STM32F103C8 Reference Manual** (RM0008) - ST Microelectronics
   - Chapter 27: Universal synchronous asynchronous receiver transmitter (USART)
2. **ESP32 Technical Reference Manual** - Espressif Systems
   - Chapter 12: UART Controller
3. **ESP-IDF UART Documentation**
   - https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/uart.html

### Buku Referensi
1. *Mastering STM32* - Carmine Noviello (Chapter 8: USART)
2. *Serial Port Complete* - Jan Axelson
3. *The Embedded Rust Book* - UART Communication

### Standards
1. **RS-232** - EIA/TIA-232 Standard
2. **TTL Serial** - 3.3V/5V Logic Levels

