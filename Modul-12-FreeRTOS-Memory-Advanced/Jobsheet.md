# Jobsheet Modul 12: FreeRTOS Memory Management & Advanced Features

## 🎯 Tujuan Praktikum
1.  Mahasiswa mampu memonitor penggunaan memori (**Heap** dan **Stack**) pada sistem FreeRTOS.
2.  Mahasiswa mampu mendeteksi dan menangani *Stack Overflow* dan kegagalan alokasi memori (*Malloc Failed*).
3.  Mahasiswa mampu menggunakan **Event Groups** untuk sinkronisasi kompleks antar task.
4.  Mahasiswa mampu menggunakan **Stream Buffer** dan **Message Buffer** untuk transfer data performa tinggi.
5.  Mahasiswa memahami konsep **Static Memory Allocation** pada FreeRTOS.

## ⚠️ Peringatan
- Modul ini menggunakan manipulasi memori intensif. Reset hardware mungkin sering diperlukan jika terjadi *Hard Fault*.
- Pastikan Serial Monitor terhubung pada baudrate **9600** atau **115200** sesuai kode.

---

## 🛠️ Percobaan 1: Monitoring Heap & Stack High Water Mark
**Tujuan:** Memantau sisa memori heap dan sisa stack terendah ("High Water Mark") dari sebuah task untuk mencegah overflow.

### Kode Program (Main.cpp)
```cpp
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>

TaskHandle_t Task1Handle = NULL;
TaskHandle_t Task2Handle = NULL;

void Task1(void *pvParameters);
void Task2(void *pvParameters);

void setup() {
  Serial.begin(115200);
  xTaskCreate(Task1, "Task1", 1024, NULL, 1, &Task1Handle); // Stack 1024 words/bytes
  xTaskCreate(Task2, "Task2", 1024, NULL, 1, &Task2Handle);
}

void loop() {
  // Empty. FreeRTOS scheduler runs tasks.
}

void Task1(void *pvParameters) {
  char buffer[50]; // Local variable consumes stack
  for (;;) {
    // Simulasi penggunaan stack
    sprintf(buffer, "Task 1 Running");
    Serial.println(buffer);
    
    // Cek sisa stack
    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    Serial.print("Task 1 Stack High Water Mark: ");
    Serial.print(stackHighWaterMark); 
    Serial.println(" words/bytes");

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void Task2(void *pvParameters) {
  for (;;) {
    // Cek Heap
    size_t freeHeap = xPortGetFreeHeapSize();
    size_t minEverFreeHeap = xPortGetMinimumEverFreeHeapSize();
    
    Serial.print("Total Free Heap: ");
    Serial.print(freeHeap);
    Serial.print(" | Min Ever Free: ");
    Serial.println(minEverFreeHeap);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
```

### ❓ Analisis
1.  Perhatikan nilai `Stack High Water Mark`. Nilai ini menunjukkan sisa stack *terkecil* yang pernah terjadi. Semakin mendekati 0, semakin berbahaya.
2.  Apa yang terjadi jika Anda memperbesar ukuran array local `buffer` di `Task1`?

---

## 🛠️ Percobaan 2: Stack Overflow Detection
**Tujuan:** Mengaktifkan mekanisme pengecekan stack overflow bawaan FreeRTOS.
**Konfigurasi:** Pastikan `configCHECK_FOR_STACK_OVERFLOW` diatur ke 1 atau 2 pada `FreeRTOSConfig.h`. (Pada Arduino ESP32 biasanya sudah default 2).

### Kode Program
```cpp
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>

TaskHandle_t BadTaskHandle = NULL;

// Hook function yang dipanggil otomatis saat overflow
// Definisi ini mungkin perlu extern "C" di C++ tergantung platform
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  Serial.print("STACK OVERFLOW detected in task: ");
  Serial.println(pcTaskName);
  while(1); // Trap forever
}

void BadTask(void *pvParameters) {
  uint8_t bigArray[500]; // Array besar untuk menjebol stack (misal stack size cuma 1024 bytes)
  
  // Isi array untuk memaksa penggunaan memori
  memset(bigArray, 0xAA, sizeof(bigArray));
  
  // Rekursif function call untuk menghabiskan stack
  // Atau just allocating local variables
  
  for (;;) {
    Serial.println("Bad Task running...");
    vTaskDelay(100);
  }
}

void setup() {
  Serial.begin(115200);
  // Buat task dengan stack SANGAT KECIL (misal 512 bytes / 128 words)
  xTaskCreate(BadTask, "BadTask", 1024, NULL, 1, &BadTaskHandle); 
}

void loop() {}
```

### ❓ Analisis
1.  Apakah pesan "STACK OVERFLOW" muncul di Serial Monitor?
2.  Jika tidak, kurangi stack size pada `xTaskCreate` sampai error muncul.

---

## 🛠️ Percobaan 3: Event Groups (Multiple Flags)
**Tujuan:** Menggabungkan beberapa flag kondisi (misal: "WiFi Connect" DAN "MQTT Connect") menjadi satu Event Group.

### Kode Program
```cpp
#include <Arduino.h>
#include <FreeRTOS.h>
#include <event_groups.h>

// Definisi Bit
#define WIFI_CONNECTED_BIT  (1 << 0) // 001
#define MQTT_CONNECTED_BIT  (1 << 1) // 010
#define ALL_SYNC_BITS       (WIFI_CONNECTED_BIT | MQTT_CONNECTED_BIT)

EventGroupHandle_t xEventGroup;

void TaskWiFi(void *pvParameters) {
  for (;;) {
    Serial.println("Connecting WiFi...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    Serial.println("WiFi Connected!");
    xEventGroupSetBits(xEventGroup, WIFI_CONNECTED_BIT);
    vTaskSuspend(NULL); // Run once
  }
}

void TaskMQTT(void *pvParameters) {
  for (;;) {
    Serial.println("Connecting MQTT...");
    vTaskDelay(pdMS_TO_TICKS(3000)); // Simulasi takes longer
    Serial.println("MQTT Connected!");
    xEventGroupSetBits(xEventGroup, MQTT_CONNECTED_BIT);
    vTaskSuspend(NULL);
  }
}

void TaskMainApp(void *pvParameters) {
  for (;;) {
    Serial.println("App waiting for WiFi & MQTT...");
    
    // Wait for BOTH bits to be set
     EventBits_t uxBits = xEventGroupWaitBits(
            xEventGroup,
            ALL_SYNC_BITS,       // Bits to wait for
            pdTRUE,              // Clear bits on exit? YES
            pdTRUE,              // Wait for ALL bits? YES (AND logic)
            portMAX_DELAY);      // Wait forever

    if ((uxBits & ALL_SYNC_BITS) == ALL_SYNC_BITS) {
      Serial.println(">>> SYSTEM READY! Starting Application Loop.");
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  Serial.begin(115200);
  xEventGroup = xEventGroupCreate();
  
  xTaskCreate(TaskWiFi, "WiFi", 2048, NULL, 1, NULL);
  xTaskCreate(TaskMQTT, "MQTT", 2048, NULL, 1, NULL);
  xTaskCreate(TaskMainApp, "App", 2048, NULL, 1, NULL);
}

void loop() {}
```

---

## 🛠️ Percobaan 4: Stream Buffer (ISR to Task Data Stream)
**Tujuan:** Mengirim data byte-stream dari ISR ke Task dengan *overhead* lebih rendah daripada Queue. Cocok untuk data sensor cepat atau UART.

### Kode Program
```cpp
#include <Arduino.h>
#include <FreeRTOS.h>
#include <stream_buffer.h>

StreamBufferHandle_t xStreamBuffer;
const size_t xStreamBufferSizeBytes = 100;
const size_t xTriggerLevel = 10; // Task bangun jika ada 10 bytes

// Timer/ISR Simulator
HardwareTimer *MyTim; // STM32
// Untuk ESP32 gunakan simple task simulator agar mudah

void TaskConsumer(void *pvParameters) {
  uint8_t rxBuffer[20];
  for (;;) {
    // Block until at least xTriggerLevel bytes are available
    size_t bytesRead = xStreamBufferReceive(xStreamBuffer, rxBuffer, sizeof(rxBuffer), portMAX_DELAY);
    
    if (bytesRead > 0) {
      Serial.print("Received chunk: ");
      for(int i=0; i<bytesRead; i++) {
        Serial.print(rxBuffer[i]);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}

void TaskProducer(void *pvParameters) {
  uint8_t dataCounter = 0;
  for (;;) {
    // Kirim 1 byte setiap 100ms (Simulasi incoming data stream)
    xStreamBufferSend(xStreamBuffer, &dataCounter, 1, 0);
    dataCounter++;
    vTaskDelay(pdMS_TO_TICKS(100)); // 100ms interval
    
    // Note: Karena trigger level 10, TaskConsumer akan print setiap 10 * 100ms = 1 detik
  }
}

void setup() {
  Serial.begin(115200);
  xStreamBuffer = xStreamBufferCreate(xStreamBufferSizeBytes, xTriggerLevel);
  
  xTaskCreate(TaskConsumer, "Cons", 2048, NULL, 1, NULL);
  xTaskCreate(TaskProducer, "Prod", 2048, NULL, 1, NULL);
}

void loop() {}
```

---

## 🛠️ Percobaan 5: Static Memory Allocation
**Tujuan:** Membuat Task tanpa menggunakan Heap (Malloc), melainkan menggunakan buffer global yang dialokasikan saat compile-time. Lebih aman untuk *Safety Critical System*.
**Konfigurasi:** Membutuhkan `configSUPPORT_STATIC_ALLOCATION` = 1 di FreeRTOSConfig.h.

### Kode Program
```cpp
#include <Arduino.h>
#include <FreeRTOS.h>

// 1. Definisikan Stack dan TCB secara Global (Static)
#define STACK_SIZE 200
StaticTask_t xTaskBuffer;
StackType_t xStack[STACK_SIZE];

void TaskStatic(void *pvParameters) {
  for (;;) {
    Serial.println("I am a Static Task!");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  Serial.begin(115200);
  
  // 2. Create Static Task
  TaskHandle_t xHandle = xTaskCreateStatic(
      TaskStatic,       // Function
      "StaticTask",     // Name
      STACK_SIZE,       // Stack Size (count)
      NULL,             // Parameter
      1,                // Priority
      xStack,           // Pointer to Stack Buffer
      &xTaskBuffer      // Pointer to TCB
  );
  
  if (xHandle == NULL) {
    Serial.println("Failed to create static task");
  }
}

void loop() {}
```

