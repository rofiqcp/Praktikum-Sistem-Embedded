# Referensi Modul 09: FreeRTOS Task Management

## 📚 Referensi Utama

### 1. Dokumentasi Resmi FreeRTOS
| Resource | Link | Deskripsi |
|----------|------|-----------|
| FreeRTOS Official | https://freertos.org | Website utama FreeRTOS |
| API Reference | https://freertos.org/a00106.html | Dokumentasi lengkap API |
| Task Functions | https://freertos.org/a00019.html | xTaskCreate, vTaskDelete, dll |
| Kernel Control | https://freertos.org/a00020.html | vTaskDelay, scheduler, dll |
| Best Practices | https://freertos.org/FreeRTOS-Coding-Standard-and-Style-Guide.html | Coding standards |

### 2. Buku Referensi
| Judul | Penulis | Tahun | ISBN |
|-------|---------|-------|------|
| **Mastering the FreeRTOS Real Time Kernel** | Richard Barry | 2016 | Free PDF |
| **Mastering STM32** | Carmine Noviello | 2018 | 978-1-291-81120-7 |
| **Using the FreeRTOS Real Time Kernel - A Practical Guide** | Richard Barry | 2010 | 978-1446169032 |
| **The Definitive Guide to ARM Cortex-M3 and Cortex-M4** | Joseph Yiu | 2014 | 978-0124080829 |
| **ESP32 Technical Reference Manual** | Espressif | 2023 | Online |

### 3. Dokumentasi Hardware

#### STM32F103C8T6
| Dokumen | Link |
|---------|------|
| Datasheet | https://www.st.com/resource/en/datasheet/stm32f103c8.pdf |
| Reference Manual | https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf |
| Programming Manual | https://www.st.com/resource/en/programming_manual/pm0075-stm32f10xxx-flash-memory-microcontrollers-stmicroelectronics.pdf |

#### ESP32
| Dokumen | Link |
|---------|------|
| Technical Reference | https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf |
| Datasheet | https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf |
| Hardware Design Guide | https://www.espressif.com/sites/default/files/documentation/esp32_hardware_design_guidelines_en.pdf |

---

## 🎥 Video Tutorial

### FreeRTOS Basics
1. **FreeRTOS on STM32** - Shawn Hymel (DigiKey)
   - https://www.youtube.com/watch?v=F321087yYy4
   - Part 1-12, sangat komprehensif

2. **Introduction to RTOS** - Shawn Hymel
   - https://www.youtube.com/playlist?list=PLEBQazB0HUyQ4hAPU1cJED6t3DU0h34bz
   - Playlist lengkap tentang konsep RTOS

3. **ESP32 FreeRTOS Tutorial** - Andreas Spiess
   - https://www.youtube.com/watch?v=WQGAs9MwXno
   - Fokus pada dual-core ESP32

4. **STM32 with FreeRTOS** - Controllers Tech
   - https://www.youtube.com/watch?v=Qc3qp1WU3UM
   - STM32CubeIDE setup

### Task Management Specific
1. **FreeRTOS Task Creation** 
   - https://www.youtube.com/watch?v=vHjxJ6m9RW4
   
2. **Task Priority and Scheduling**
   - https://www.youtube.com/watch?v=UXmyBn_gL_4

3. **Task Notifications**
   - https://www.youtube.com/watch?v=rM6Kn1BcfYI

---

## 📝 Artikel dan Tutorial Online

### FreeRTOS Concepts
| Judul | Sumber | Link |
|-------|--------|------|
| Getting Started with FreeRTOS | FreeRTOS.org | https://freertos.org/FreeRTOS-quick-start-guide.html |
| Task Creation Tutorial | FreeRTOS.org | https://freertos.org/a00125.html |
| Task Priorities | FreeRTOS.org | https://freertos.org/RTOS-task-priority.html |
| Stack Overflow Protection | FreeRTOS.org | https://freertos.org/Stacks-and-stack-overflow-checking.html |
| Debugging Tips | Memfault | https://interrupt.memfault.com/blog/freertos-debugging-tips |

### STM32 Specific
| Judul | Sumber | Link |
|-------|--------|------|
| STM32 FreeRTOS Guide | ST Wiki | https://wiki.st.com/stm32mcu/wiki/Introduction_to_FREERTOS |
| CMSIS-RTOS v2 | ARM | https://arm-software.github.io/CMSIS_5/RTOS2/html/index.html |
| HAL + FreeRTOS | ControllersTech | https://controllerstech.com/freertos-with-stm32/ |

### ESP32 Specific
| Judul | Sumber | Link |
|-------|--------|------|
| ESP-IDF FreeRTOS | Espressif Docs | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html |
| ESP32 Dual Core | RandomNerdTutorials | https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/ |
| ESP32 Tasks Tutorial | LastMinuteEngineers | https://lastminuteengineers.com/esp32-freertos-tutorial/ |

---

## 🔧 Tools dan Software

### Development Environment
| Tool | Fungsi | Link |
|------|--------|------|
| **PlatformIO** | IDE untuk embedded | https://platformio.org |
| **STM32CubeIDE** | Official ST IDE | https://www.st.com/en/development-tools/stm32cubeide.html |
| **Arduino IDE** | ESP32 development | https://www.arduino.cc/en/software |
| **VS Code** | Code editor | https://code.visualstudio.com |

### Debugging Tools
| Tool | Fungsi | Link |
|------|--------|------|
| **Tracealyzer** | RTOS visualization | https://percepio.com/tracealyzer/ |
| **SystemView** | RTOS analysis (free) | https://www.segger.com/products/development-tools/systemview/ |
| **OpenOCD** | Debug adapter | https://openocd.org |
| **ST-Link Utility** | STM32 programmer | https://www.st.com/en/development-tools/stsw-link004.html |

### Serial Monitor
| Tool | Platform | Link |
|------|----------|------|
| **PuTTY** | Windows | https://putty.org |
| **RealTerm** | Windows | https://realterm.sourceforge.io |
| **CoolTerm** | Cross-platform | https://freeware.the-meiers.org |
| **minicom** | Linux | `apt install minicom` |

---

## 📊 Cheat Sheets

### FreeRTOS API Quick Reference

#### Task Creation
```c
// Membuat task
BaseType_t xTaskCreate(
    TaskFunction_t pvTaskCode,      // Fungsi task
    const char* pcName,             // Nama task
    configSTACK_DEPTH_TYPE usStackDepth, // Stack size (words)
    void* pvParameters,             // Parameter
    UBaseType_t uxPriority,         // Prioritas (0 = lowest)
    TaskHandle_t* pxCreatedTask     // Handle
);

// ESP32: Membuat task di core spesifik
BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t pvTaskCode,
    const char* pcName,
    uint32_t usStackDepth,
    void* pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t* pxCreatedTask,
    BaseType_t xCoreID              // 0, 1, atau tskNO_AFFINITY
);
```

#### Task Control
```c
void vTaskDelay(TickType_t xTicksToDelay);
void vTaskDelayUntil(TickType_t* pxPreviousWakeTime, TickType_t xTimeIncrement);
void vTaskDelete(TaskHandle_t xTask);
void vTaskSuspend(TaskHandle_t xTask);
void vTaskResume(TaskHandle_t xTask);
```

#### Task Info
```c
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t xTask);
TaskHandle_t xTaskGetCurrentTaskHandle(void);
char* pcTaskGetName(TaskHandle_t xTask);
UBaseType_t uxTaskGetNumberOfTasks(void);
UBaseType_t uxTaskPriorityGet(TaskHandle_t xTask);
void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority);
```

#### Scheduler Control
```c
void vTaskStartScheduler(void);
void vTaskEndScheduler(void);
void vTaskSuspendAll(void);
BaseType_t xTaskResumeAll(void);
```

#### Time Functions
```c
TickType_t xTaskGetTickCount(void);
#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)(xTimeInMs * configTICK_RATE_HZ / 1000))
```

### Typical Stack Sizes
| Task Type | STM32 (words) | ESP32 (bytes) |
|-----------|---------------|---------------|
| Simple LED blink | 64-128 | 1024-2048 |
| Sensor reading | 128-256 | 2048-4096 |
| UART communication | 128-256 | 2048-4096 |
| WiFi/Network | N/A | 4096-8192 |
| Display update | 128-256 | 2048-4096 |

### Priority Guidelines
| Prioritas | Penggunaan |
|-----------|------------|
| 0 (Idle) | Idle task, statistik |
| 1 (Low) | Monitoring, logging |
| 2 (Normal) | Operasi reguler |
| 3 (High) | Time-critical I/O |
| 4+ (Highest) | Safety-critical, interrupt handlers |

---

## 🧪 Contoh Kode Referensi

### GitHub Repositories
| Repository | Deskripsi | Link |
|------------|-----------|------|
| FreeRTOS | Official FreeRTOS | https://github.com/FreeRTOS/FreeRTOS |
| STM32 FreeRTOS Examples | STM32Cube examples | https://github.com/STMicroelectronics/STM32CubeF1 |
| ESP32 Examples | Arduino-ESP32 | https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples |
| FreeRTOS Demo | Comprehensive demos | https://github.com/FreeRTOS/FreeRTOS/tree/main/FreeRTOS/Demo |

### Template Projects
| Template | Platform | Link |
|----------|----------|------|
| PlatformIO STM32 FreeRTOS | STM32 | https://github.com/platformio/platform-ststm32/tree/master/examples |
| ESP32 FreeRTOS Starter | ESP32 | https://github.com/espressif/esp-idf/tree/master/examples/system/freertos |

---

## 📱 Forum dan Komunitas

| Platform | Link | Fokus |
|----------|------|-------|
| **FreeRTOS Forum** | https://forums.freertos.org | Official support |
| **STM32 Community** | https://community.st.com | ST products |
| **ESP32 Forum** | https://esp32.com | Espressif products |
| **Reddit r/embedded** | https://reddit.com/r/embedded | General embedded |
| **Stack Overflow** | https://stackoverflow.com/questions/tagged/freertos | Q&A |
| **EEVblog Forum** | https://www.eevblog.com/forum/ | Electronics |

---

## 📄 Paper dan Jurnal

1. **"FreeRTOS: A Real-Time Operating System for Microcontrollers"**
   - Richard Barry, 2003
   
2. **"Analysis of Task Scheduling Algorithms in FreeRTOS"**
   - IEEE Conference on Embedded Systems

3. **"Comparative Study of FreeRTOS and Other RTOS"**
   - Journal of Embedded Systems

4. **"Real-Time Task Scheduling: A Survey"**
   - ACM Computing Surveys

---

## 🎓 Course Online

| Platform | Course | Link |
|----------|--------|------|
| **Udemy** | Mastering RTOS | https://www.udemy.com/course/mastering-rtos-hands-on-with-freertos-arduino-and-stm32fx/ |
| **Coursera** | Real-Time Systems | Search "real-time embedded systems" |
| **edX** | Embedded Systems | https://www.edx.org/learn/embedded-systems |
| **YouTube** | DigiKey RTOS Series | https://www.youtube.com/playlist?list=PLEBQazB0HUyQ4hAPU1cJED6t3DU0h34bz |

---

## ⚠️ Catatan Penting

1. **Selalu cek versi** - FreeRTOS terus update, pastikan dokumentasi sesuai versi yang digunakan
2. **Baca datasheet** - Pahami hardware sebelum programming
3. **Join komunitas** - Forum adalah sumber bantuan terbaik
4. **Practice** - Tidak ada shortcut, harus banyak coding!

---

*Terakhir diupdate: 2024*
