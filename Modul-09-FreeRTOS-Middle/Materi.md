# Modul 09: FreeRTOS Middle - GPIO, Interrupt, Encoder, Serial, DAC, ADC, I2C, SPI

## Capaian Pembelajaran

Setelah menyelesaikan Modul 09, mahasiswa mampu:

1. Menjelaskan konsep FreeRTOS: task, scheduler, ISR, semaphore, mutex, queue, event group, dan software timer.
2. Mengimplementasikan GPIO tasks dengan delay non-blocking pada ESP32 dan STM32.
3. Menangani external interrupt dengan RTOS primitives (semaphore/queue dari ISR).
4. Membaca encoder 2-pin dengan interrupt RTOS tanpa blocking.
5. Mengimplementasikan komunikasi serial UART dengan queue RTOS.
6. Menggunakan DAC dan ADC dengan task RTOS dan sinkronisasi.
7. Mengintegrasikan I2C dan SPI dengan RTOS menggunakan mutex untuk shared bus.
8. Membangun sistem multi-task dengan komunikasi antar-MCU menggunakan RTOS.
9. Mengintegrasikan semua konsep dalam project RTOS Sensor Hub System.

---

## 1. Dasar FreeRTOS

FreeRTOS adalah real-time operating system kernel untuk mikrokontroler. Konsep utama:

- **Task**: fungsi yang dijalankan secara konkuren oleh scheduler.
- **Scheduler**: mengatur eksekusi task berdasarkan priority (preemptive).
- **Context switch**: menyimpan/memulihkan state task saat switch.

Struktur task:

```c
void vTaskFunction(void *pvParameters) {
    for (;;) {
        // task code
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

Prioritas task: 0 (idle) sampai configMAX_PRIORITIES-1. Higher number = higher priority.

---

## 2. Task Creation dan Scheduler

### ESP32 FreeRTOS (Native)

```c
xTaskCreate(
    vTaskFunction,    // task function
    "TaskName",        // task name
    2048,             // stack depth (words)
    NULL,             // parameters
    1,                // priority
    NULL               // task handle
);
vTaskStartScheduler(); // otomatis di ESP32
```

### STM32 (CMSIS-RTOS / STM32Cube)

```c
osThreadDef(LEDTask, vLEDTask, osPriorityNormal, 0, 128);
osThreadCreate(osThread(LEDTask), NULL);
osKernelStart(); // jika tidak otomatis
```

Task delay: `vTaskDelay()` (tick-based) atau `vTaskDelayUntil()` (fixed frequency).

---

## 3. ISR dan RTOS

Interrupt Service Routine (ISR) harus cepat dan non-blocking. RTOS menyediakan ISR-safe API:

- `xSemaphoreGiveFromISR()`
- `xQueueSendFromISR()`
- `xEventGroupSetBitsFromISR()`

Aturan penting:

- Jangan gunakan `vTaskDelay()` di ISR.
- Jangan gunakan `xQueueSend()` (tanpa FromISR) di ISR.
- Gunakan `portYIELD_FROM_ISR()` untuk request context switch setelah ISR.

```c
void EXTI0_IRQHandler(void) {
    xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}
```

---

## 4. Semaphore dan Mutex

### Binary Semaphore

Digunakan untuk signaling dari ISR ke task atau task-to-task notification.

```c
SemaphoreHandle_t xBinarySemaphore = xSemaphoreCreateBinary();
// Task menunggu:
if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) { /* proceed */ }
// ISR memberi:
xSemaphoreGiveFromISR(xBinarySemaphore, NULL);
```

### Mutex

Digunakan untuk proteksi shared resource (mutual exclusion). Mutex memiliki priority inheritance untuk mencegah priority inversion.

```c
SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();
// Akses resource:
if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    // akses shared resource (UART, I2C, etc.)
    xSemaphoreGive(xMutex);
}
```

Perbedaan:
- Semaphore: signaling, tidak dimiliki.
- Mutex: proteksi resource, dimiliki task yang mengambilnya.

---

## 5. Queue

Queue menyediakan komunikasi antar task (inter-task communication) atau ISR-to-task.

```c
QueueHandle_t xQueue = xQueueCreate(10, sizeof(int));

// Task pengirim:
int value = 123;
xQueueSend(xQueue, &value, portMAX_DELAY);

// Task penerima:
int received;
if (xQueueReceive(xQueue, &received, portMAX_DELAY) == pdPASS) {
    // process received value
}

// ISR pengirim:
xQueueSendFromISR(xQueue, &value, NULL);
```

Queue bisa berisi data bebas tipe (struct, int, char, dll).

---

## 6. Event Group

Event group memungkinkan sinkronisasi berdasarkan bitmask (multiple events).

```c
EventGroupHandle_t xEventGroup = xEventGroupCreate();

// Task set bit:
xEventGroupSetBits(xEventGroup, BIT_0);

// Task wait for any bit:
EventBits_t bits = xEventGroupWaitBits(
    xEventGroup,
    BIT_0 | BIT_1,
    pdTRUE,        // clear on exit
    pdFALSE,       // wait for any, not all
    portMAX_DELAY
);

// ISR set bit:
xEventGroupSetBitsFromISR(xEventGroup, BIT_0, NULL);
```

---

## 7. GPIO Tasks

GPIO dengan RTOS menggunakan task yang mengendalikan pin secara periodik.

### ESP32

```c
void vLEDTask(void *pvParameters) {
    gpio_pad_select_gpio(2);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
    for (;;) {
        gpio_set_level(2, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(2, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

### STM32

```c
void vLEDTask(void *argument) {
    for (;;) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        osDelay(500);
    }
}
```

Non-blocking: gunakan `vTaskDelay()` atau `osDelay()`, bukan `HAL_Delay()`.

---

## 8. External Interrupt dengan RTOS

External interrupt ditangani dengan semaphore dari ISR.

### STM32

```c
// Init: PA0 EXTI falling edge
// ISR:
void EXTI0_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

// Task:
void vButtonTask(void *argument) {
    for (;;) {
        if (xSemaphoreTake(xButtonSemaphore, portMAX_DELAY) == pdTRUE) {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        }
    }
}
```

### ESP32

```c
void IRAM_ATTR gpio_isr_handler(void *arg) {
    xSemaphoreGiveFromISR(xBinarySemaphore, NULL);
}

// Init:
gpio_install_isr_service(0);
gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, NULL);
```

---

## 9. Encoder 2-Pin Interrupt RTOS

Rotary encoder memiliki 2 pin output (A dan B) dengan phase 90°. Pembacaan dengan interrupt pada kedua pin.

Algoritma:
1. Interrupt pada rising/falling pin A.
2. Baca pin B untuk tentukan arah.
3. Jika B == HIGH saat A falling → CW; jika B == LOW → CCW.
4. Kirim event ke queue/sem sebagaii ISR.
5. Task proses queue dan update counter.

```c
// ISR Pin A:
void IRAM_ATTR encoder_isr(void *arg) {
    int b_state = gpio_get_level(ENCODER_B);
    int direction = b_state ? 1 : -1; // CW=1, CCW=-1
    xQueueSendFromISR(xEncoderQueue, &direction, NULL);
}
```

Debounce dilakukan di task level, bukan ISR (delay di task, bukan ISR).

---

## 10. Serial Communication RTOS

UART dengan RTOS menggunakan queue untuk RX dan TX.

### ESP32

```c
// Init UART dengan driver
uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};
uart_param_config(UART_NUM_0, &uart_config);
uart_driver_install(UART_NUM_0, 1024, 1024, 10, &uart_queue, 0);

// Task RX:
void vUARTRxTask(void *arg) {
    uart_event_t event;
    for (;;) {
        if (xQueueReceive(uart_queue, &event, portMAX_DELAY)) {
            if (event.type == UART_DATA) {
                uint8_t data[64];
                int len = uart_read_bytes(UART_NUM_0, data, 64, 20);
                // process data
            }
        }
    }
}
```

### STM32

```c
// RX interrupt, masukkan ke queue
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    xQueueSendFromISR(xUARTQueue, &rx_char, NULL);
    HAL_UART_Receive_IT(&huart2, &rx_char, 1);
}
```

---

## 11. DAC dengan RTOS

DAC menghasilkan sinyal analog. Task RTOS menulis nilai DAC periodik.

### ESP32

```c
void vDACTask(void *arg) {
    dac_output_enable(DAC_CHANNEL_1); // GPIO25
    const int lookup[100] = { /* sin wave values */ };
    int idx = 0;
    for (;;) {
        dac_output_voltage(DAC_CHANNEL_1, lookup[idx]);
        idx = (idx + 1) % 100;
        vTaskDelay(pdMS_TO_TICKS(1)); // 1ms → 1kHz
    }
}
```

### STM32

```c
void vDACTask(void *arg) {
    for (;;) {
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_value);
        osDelay(1);
    }
}
```

Gunakan lookup table untuk bentuk gelombang (sin, triangle, sawtooth).

---

## 12. ADC dengan RTOS

ADC membaca tegangan analog. Task RTOS baca ADC periodik dan kirim ke queue.

### ESP32

```c
void vADCTask(void *arg) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // GPIO34
    for (;;) {
        int adc_val = adc1_get_raw(ADC1_CHANNEL_6);
        float voltage = adc_val * 3.3 / 4095;
        xQueueSend(xADCQueue, &voltage, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### STM32

```c
void vADCTask(void *arg) {
    for (;;) {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 10);
        uint32_t adc_val = HAL_ADC_GetValue(&hadc1);
        xQueueSend(xADCQueue, &adc_val, portMAX_DELAY);
        osDelay(100);
    }
}
```

---

## 13. I2C dengan RTOS

I2C bus shared harus dilindungi dengan mutex agar tidak ada race condition.

### ESP32

```c
void vI2CTask(void *arg) {
    for (;;) {
        if (xSemaphoreTake(xI2CMutex, portMAX_DELAY) == pdTRUE) {
            // baca sensor I2C
            i2c_master_write_read_device(...);
            xSemaphoreGive(xI2CMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### STM32

```c
void vI2CTask(void *arg) {
    for (;;) {
        if (xSemaphoreTake(xI2CMutex, portMAX_DELAY) == pdTRUE) {
            HAL_I2C_Mem_Read(&hi2c1, addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
            xSemaphoreGive(xI2CMutex);
        }
        osDelay(1000);
    }
}
```

Mutex diambil sebelum akses bus, dilepas setelah selesai.

---

## 14. SPI dengan RTOS

SPI bus juga harus dilindungi dengan mutex jika diakses multiple task.

### ESP32

```c
void vSPITask(void *arg) {
    for (;;) {
        if (xSemaphoreTake(xSPIMutex, portMAX_DELAY) == pdTRUE) {
            spi_transaction_t t = {
                .length = 8,
                .tx_buffer = tx_data,
                .rx_buffer = rx_data
            };
            spi_device_transmit(spi_handle, &t);
            xSemaphoreGive(xSPIMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### STM32

```c
void vSPITask(void *arg) {
    for (;;) {
        if (xSemaphoreTake(xSPIMutex, portMAX_DELAY) == pdTRUE) {
            HAL_SPI_TransmitReceive(&hspi1, tx, rx, len, 100);
            xSemaphoreGive(xSPIMutex);
        }
        osDelay(1000);
    }
}
```

---

## 15. Struktur Praktikum Final Modul 09

Praktikum final berisi tepat **25 eksperimen**:

### 10 Eksperimen STM32 (FreeRTOS via CMSIS-RTOS/STM32Cube)

| No | Kode | Topik | FreeRTOS Primitive |
|---|---|---|---|
| 1 | STM32_01 | GPIO Task Blink | vTaskDelay/osDelay |
| 2 | STM32_02 | External Interrupt | Binary Semaphore |
| 3 | STM32_03 | Encoder 2-Pin Interrupt | Queue |
| 4 | STM32_04 | Serial Communication | Queue |
| 5 | STM32_05 | DAC dengan RTOS | vTaskDelay |
| 6 | STM32_06 | ADC dengan RTOS | Queue |
| 7 | STM32_07 | I2C dengan RTOS | Mutex |
| 8 | STM32_08 | SPI dengan RTOS | Mutex |
| 9 | STM32_09 | Multi Task GPIO-ADC-UART | EventGroup |
| 10 | STM32_10 | FreeRTOS Semaphore Mutex | Semaphore+Mutex |

### 10 Eksperimen ESP32 (FreeRTOS Native)

| No | Kode | Topik | FreeRTOS Primitive |
|---|---|---|---|
| 1 | ESP32_01 | GPIO Task Blink | vTaskDelay |
| 2 | ESP32_02 | External Interrupt | Binary Semaphore |
| 3 | ESP32_03 | Encoder 2-Pin Interrupt | Queue |
| 4 | ESP32_04 | Serial Communication | Queue/UART driver |
| 5 | ESP32_05 | DAC dengan RTOS | vTaskDelay |
| 6 | ESP32_06 | ADC dengan RTOS | Queue |
| 7 | ESP32_07 | I2C dengan RTOS | Mutex |
| 8 | ESP32_08 | SPI dengan RTOS | Mutex |
| 9 | ESP32_09 | Multi Task GPIO-ADC-UART | EventGroup |
| 10 | ESP32_10 | FreeRTOS Semaphore Mutex | Semaphore+Mutex |

### 5 Eksperimen Multi STM32-ESP32 (RTOS Communication)

| No | Kode | Topik | Fokus |
|---|---|---|---|
| 1 | MULTI_01 | RTOS GPIO Task Sync | Task sync via UART |
| 2 | MULTI_02 | RTOS ADC-DAC Communication | Data exchange |
| 3 | MULTI_03 | RTOS I2C Sensor Sharing | Shared sensor |
| 4 | MULTI_04 | RTOS SPI Data Exchange | Data exchange |
| 5 | MULTI_05 | RTOS Sensor Hub System | Project integration |

---

## 16. Troubleshooting Checklist

1. Pastikan `configTOTAL_HEAP_SIZE` cukup untuk task dan primitive.
2. Periksa stack overflow dengan `uxTaskGetStackHighWaterMark()`.
3. Jangan gunakan blocking function di ISR.
4. Pastikan ISR-safe API (FromISR) dipakai di ISR.
5. Gunakan mutex untuk shared resource (UART, I2C, SPI).
6. Periksa priority inversion jika pakai mutex.
7. Pastikan `vTaskDelay()` digunakan, bukan `HAL_Delay()` di task.
8. Periksa queue full/empty pada pengiriman/penerimaan.
9. Debug dengan serial print, namun protect UART dengan mutex.
10. Periksa context switch time jika performa kritis.

---

## 17. Best Practice Desain RTOS

Software:

- Gunakan meaningful task names.
- Pilih priority task dengan benar (higher number = higher priority).
- Gunakan stack size yang cukup (cek dengan `uxTaskGetStackHighWaterMark()`).
- Hindari busy-wait di task; gunakan delay atau blocking primitive.
- Gunakan queue untuk data transfer, semaphore untuk signaling.
- Protect shared resource dengan mutex, bukan critical section (supaya RTOS bisa schedule).
- Tangani error pada pengambilan semaphore/queue (timeout, dll).

Hardware:

- Pastikan interrupt line tidak konflik.
- Gunakan pull-up pada pin interrupt jika perlu.
- Decoupling capacitor dekat MCU dan sensor.
- Common ground untuk multi-MCU komunikasi.

---

## 18. Referensi

1. FreeRTOS Official Documentation — https://www.freertos.org/documentation/
2. Espressif ESP-IDF Programming Guide — FreeRTOS, GPIO, UART, I2C, SPI, ADC, DAC.
3. STM32 Reference Manual dan CMSIS-RTOS Documentation — FreeRTOS integration.
4. Datasheet STM32F103/STM32F4 — GPIO, EXTI, ADC, DAC, I2C, SPI.
5. Datasheet ESP32 — GPIO matrix, UART, I2C, SPI, ADC, DAC.
