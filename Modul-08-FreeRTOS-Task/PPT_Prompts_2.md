# Prompt untuk Pembuatan PPT - Bagian 2
## Modul 09: FreeRTOS Task Management

---

## Slide 16: Implementasi STM32

**Prompt:**
```
Create slide showing STM32 FreeRTOS setup:
- STM32CubeMX screenshot mockup showing FreeRTOS configuration
- FreeRTOSConfig.h key parameters in code box
- STM32F103 Blue Pill board image
- Steps: Enable FreeRTOS, Configure heap, Set priorities
Technical setup guide style, 16:9 format.
```

---

## Slide 17: STM32 Code Example

**Prompt:**
```
Create code slide with STM32 task creation example:
- Show main() function with task creation
- Highlight xTaskCreate() call with parameters
- Show vTaskStartScheduler()
- Include LED task and UART task code snippets
Syntax highlighting, code editor style, dark theme, 16:9 ratio.
```

---

## Slide 18: ESP32 Dual Core

**Prompt:**
```
Create diagram showing ESP32 dual core architecture:
- Two CPU cores: PRO_CPU (Core 0) and APP_CPU (Core 1)
- Tasks distributed across cores
- xTaskCreatePinnedToCore() API highlight
- Show WiFi/BT typically on Core 0, App tasks on Core 1
ESP32 chip diagram with core visualization, 16:9 format.
```

---

## Slide 19: ESP32 Code Example

**Prompt:**
```
Create code slide with ESP32 Arduino task creation:
- Show setup() with xTaskCreatePinnedToCore()
- Parameters: function, name, stack (bytes), params, priority, handle, core
- Two tasks pinned to different cores
- Serial output showing core ID
Arduino IDE style, syntax highlighting, 16:9 ratio.
```

---

## Slide 20: STM32 vs ESP32 Comparison

**Prompt:**
```
Create comparison table infographic:
Two columns: STM32F103 vs ESP32
Rows:
- Cores: 1 vs 2
- Speed: 72MHz vs 240MHz
- RAM: 20KB vs 520KB
- Stack Unit: Words vs Bytes
- Core Affinity: N/A vs Supported
- Tick Rate: 1000Hz vs 100Hz
Clean table design with icons, 16:9 format.
```

---

## Slide 21: Stack Overflow Detection

**Prompt:**
```
Create diagnostic slide showing stack overflow:
- Stack diagram showing overflow scenario
- configCHECK_FOR_STACK_OVERFLOW methods (1 and 2)
- vApplicationStackOverflowHook() callback
- Warning indicators and error message
Technical debug illustration, 16:9 ratio.
```

---

## Slide 22: Task Runtime Statistics

**Prompt:**
```
Create terminal output mockup showing:
- vTaskList() output: Name, State, Priority, Stack, Number
- vTaskGetRunTimeStats() output: Task, Abs Time, % Time
- Stack high water mark display
- Example task statistics table
Terminal/console style with green text on black, 16:9 format.
```

---

## Slide 23: Dynamic Task Pattern

**Prompt:**
```
Create flowchart showing dynamic task creation:
- Manager task creates/deletes workers
- Worker tasks with parameters (ID, interval, pin)
- Memory allocation/deallocation flow
- xTaskCreate() and vTaskDelete() points marked
Process flowchart style, 16:9 ratio.
```

---

## Slide 24: Rate Monotonic Scheduling

**Prompt:**
```
Create RMS concept diagram:
- Three tasks with different periods: 10ms, 50ms, 100ms
- Priority assignment: shorter period = higher priority
- Timeline showing execution pattern
- Formula: Utilization = Σ(Ci/Ti)
Technical scheduling diagram, 16:9 format.
```

---

## Slide 25: Idle Hook Power Saving

**Prompt:**
```
Create power management diagram:
- CPU power states: Active, Idle, Sleep
- vApplicationIdleHook() calling __WFI()
- Power consumption graph showing reduction
- Wake on interrupt illustration
Energy efficiency infographic, 16:9 ratio.
```

---

## Slide 26: Best Practices - Task Design

**Prompt:**
```
Create DO/DON'T comparison slide:
DO side (green checkmarks):
- Single responsibility tasks
- Clear, focused functions
- Proper blocking calls
DON'T side (red X):
- Monolithic super-tasks
- Busy wait loops
- Missing yields
Split screen design, 16:9 format.
```

---

## Slide 27: Best Practices - Memory

**Prompt:**
```
Create memory best practices infographic:
- Static vs Dynamic allocation comparison
- xTaskCreateStatic() for predictability
- Heap fragmentation warning
- Stack size guidelines
- Memory monitoring tips
Educational checklist style, 16:9 ratio.
```

---

## Slide 28: Best Practices - Priority

**Prompt:**
```
Create priority assignment guide:
- Priority pyramid with levels 0-5
- Recommended usage for each level
- Avoid priority inversion warning
- Priority inheritance mention for mutexes
Professional guide layout, 16:9 format.
```

---

## Slide 29: Common Mistakes

**Prompt:**
```
Create troubleshooting slide:
Four problem boxes with solutions:
1. Task not running → Check priority, increase it
2. Stack overflow → Increase stack size
3. System freeze → Add vTaskDelay
4. Memory exhausted → Reduce tasks/stack
Each with warning icon and solution checkmark
Problem-solution layout, 16:9 ratio.
```

---

## Slide 30: Lab Exercise

**Prompt:**
```
Create lab assignment slide:
- Exercise 1: Create 3 LED tasks with different priorities
- Exercise 2: Implement suspend/resume with buttons
- Exercise 3: Monitor stack usage and optimize
- Challenge: Port STM32 code to ESP32 dual-core
Include circuit diagram thumbnail, 16:9 format.
```

---

## Slide 31: Project Ideas

**Prompt:**
```
Create project inspiration slide with 4 project cards:
1. Multi-sensor Data Logger - icon: sensor
2. Traffic Light Controller - icon: traffic light
3. Home Automation Hub - icon: smart home
4. Industrial Monitor - icon: factory
Each with brief description and difficulty level
Modern card layout, 16:9 ratio.
```

---

## Slide 32: Summary

**Prompt:**
```
Create summary slide with key takeaways:
- RTOS enables concurrent task execution
- Tasks have states: Ready, Running, Blocked, Suspended
- Priority-based preemptive scheduling
- Always include blocking calls
- Monitor stack and CPU usage
- STM32 single-core, ESP32 dual-core
Clean bullet point layout with icons, 16:9 format.
```

---

## Slide 33: Q&A

**Prompt:**
```
Create Q&A slide:
- Large question mark icon in center
- "Questions?" text
- Contact/resource information
- FreeRTOS documentation link
- Next module preview: "Queue & Semaphore"
Minimalist design, blue theme, 16:9 ratio.
```

---

## Slide 34: References

**Prompt:**
```
Create references slide:
1. FreeRTOS.org - Official Documentation
2. Mastering the FreeRTOS Real Time Kernel - Richard Barry
3. STM32 FreeRTOS User Guide (UM1722)
4. ESP-IDF FreeRTOS Documentation
5. ARM Cortex-M Programming Guide
Academic reference style, 16:9 format.
```

---

## Catatan Penggunaan

1. Gunakan prompt di atas dengan AI image generator seperti Midjourney, DALL-E, atau Stable Diffusion
2. Sesuaikan warna dan style dengan branding institusi
3. Tambahkan logo dan informasi institusi pada setiap slide
4. Export dalam format 16:9 untuk kompatibilitas maksimal
5. Gunakan font yang konsisten: Montserrat untuk judul, Open Sans untuk body

