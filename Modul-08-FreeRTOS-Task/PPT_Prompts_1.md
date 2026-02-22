# Prompt untuk Pembuatan PPT - Bagian 1
## Modul 09: FreeRTOS Task Management

### Instruksi Umum untuk AI Image Generator

Gunakan prompt berikut untuk membuat slide presentasi yang profesional dan edukatif.

---

## Slide 1: Judul

**Prompt:**
```
Create a professional presentation title slide with dark blue gradient background. 
Title: "FreeRTOS Task Management" in large white bold text.
Subtitle: "Real-Time Operating System untuk Embedded Systems"
Include subtle microcontroller circuit pattern overlay.
Modern minimalist tech design with STM32 and ESP32 chip icons.
16:9 aspect ratio, corporate style.
```

---

## Slide 2: Agenda

**Prompt:**
```
Create an agenda slide with 6 numbered items in a clean vertical layout:
1. Pengenalan RTOS
2. Konsep Task
3. Task States & Lifecycle
4. Task Priority & Scheduling
5. Implementasi STM32 & ESP32
6. Best Practices

Use blue and white color scheme, include small relevant icons for each item.
Professional presentation style, 16:9 ratio.
```

---

## Slide 3: Apa itu RTOS?

**Prompt:**
```
Educational infographic comparing Bare-Metal vs RTOS architecture.
Left side: Simple sequential loop diagram labeled "Bare-Metal (Super Loop)"
Right side: Multi-task parallel execution diagram labeled "RTOS"
Show advantages: Deterministic timing, Preemptive scheduling, Low latency
Use blue tones, technical diagram style, clear labels.
16:9 presentation slide format.
```

---

## Slide 4: FreeRTOS Overview

**Prompt:**
```
Create an infographic showing FreeRTOS features:
- Logo: FreeRTOS with green checkmark
- Key stats: <10KB ROM, <1KB RAM, 35+ architectures
- Feature boxes: Tasks, Queues, Semaphores, Mutexes, Timers
- Supported platforms: ARM Cortex-M, ESP32, RISC-V icons
Blue gradient background, modern tech style, 16:9 ratio.
```

---

## Slide 5: FreeRTOS Architecture

**Prompt:**
```
Create layered architecture diagram showing:
Top layer: "Application Tasks" (user code)
Second: "FreeRTOS API" (API functions)  
Third: "FreeRTOS Kernel" (scheduler, memory)
Fourth: "Port Layer" (hardware specific)
Bottom: "Hardware (MCU)" with STM32 and ESP32 icons
Use different shades of blue for each layer, arrows showing interaction.
Professional technical diagram, 16:9 format.
```

---

## Slide 6: Apa itu Task?

**Prompt:**
```
Create educational diagram showing task anatomy:
- Task Control Block (TCB) box with: State, Priority, Stack Pointer, Name
- Task Stack box with: Local Variables, Parameters, Return Addresses, CPU Registers
- Arrow connecting TCB to Stack
- Visual representation of independent execution unit
Clean technical illustration, blue color scheme, 16:9 ratio.
```

---

## Slide 7: Task States

**Prompt:**
```
Create state machine diagram for FreeRTOS task states:
- Four states: READY (green), RUNNING (blue), BLOCKED (yellow), SUSPENDED (red)
- Arrows showing transitions with labels:
  - Ready → Running (Scheduler picks)
  - Running → Blocked (vTaskDelay)
  - Running → Suspended (vTaskSuspend)
  - Blocked → Ready (Event/Timeout)
  - Suspended → Ready (vTaskResume)
Clean flowchart style, colored boxes, clear arrows with labels.
16:9 presentation format.
```

---

## Slide 8: Task Priority

**Prompt:**
```
Create priority hierarchy diagram:
- Vertical stack showing priority levels 0-5
- Level 5: "Critical/ISR" (red, top)
- Level 4: "Real-time" (orange)
- Level 3: "Time-sensitive" (yellow)
- Level 2: "Normal" (green)
- Level 1: "Background" (light blue)
- Level 0: "Idle" (gray, bottom)
Arrow pointing up labeled "Higher Priority"
Clean infographic style, 16:9 ratio.
```

---

## Slide 9: Scheduler Algorithm

**Prompt:**
```
Create timeline diagram showing preemptive scheduling:
- Three horizontal bars representing Task A (Pri 3), Task B (Pri 2), Task C (Pri 1)
- Show how high priority task preempts lower priority
- Time ticks marked on horizontal axis
- Color coding: running (solid), ready (striped), blocked (dotted)
Technical timeline visualization, clear legend, 16:9 format.
```

---

## Slide 10: Context Switch

**Prompt:**
```
Create diagram showing context switch mechanism:
- Two task boxes: "Current Task" and "New Task"
- Steps in middle:
  1. Save Context (push registers)
  2. Update TCB (save SP)
  3. Select Next Task (scheduler)
  4. Load Context (load SP)
  5. Restore Context (pop registers)
- Arrows showing flow between tasks
Technical process diagram, numbered steps, 16:9 ratio.
```

---

## Slide 11: xTaskCreate API

**Prompt:**
```
Create code documentation slide showing xTaskCreate function:
- Function signature with parameter boxes
- Parameters: pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask
- Return value explanation: pdPASS or error
- Visual code snippet styling with syntax highlighting colors
Clean technical documentation style, 16:9 format.
```

---

## Slide 12: Task Function Template

**Prompt:**
```
Create code template visualization:
- Show task function structure:
  void vTaskFunction(void *pvParameters)
  {
      // Initialization
      for(;;)
      {
          // Task logic
          vTaskDelay(pdMS_TO_TICKS(100));
      }
  }
- Highlight key parts: initialization, infinite loop, delay
Code editor style with line numbers, syntax highlighting, 16:9 ratio.
```

---

## Slide 13: Stack Size Calculation

**Prompt:**
```
Create infographic showing stack calculation:
- Formula: Stack = (LocalVars + CallDepth×32 + Context) × 1.3
- Components breakdown with sizes
- Recommended minimums:
  - Simple task: 128 words
  - Printf task: 256 words
  - Float task: 256+ words
- Visual stack diagram showing layers
Educational diagram, blue theme, 16:9 format.
```

---

## Slide 14: Delay APIs Comparison

**Prompt:**
```
Create comparison diagram:
Left: vTaskDelay() - "Relative Delay"
- Timeline showing delay from current point
Right: vTaskDelayUntil() - "Absolute Delay"  
- Timeline showing precise periodic execution
Include timing diagrams with tick marks
Show drift vs no-drift comparison
Technical timing diagram, 16:9 ratio.
```

---

## Slide 15: Task Control APIs

**Prompt:**
```
Create API reference card with icons:
- vTaskSuspend() - Pause icon
- vTaskResume() - Play icon
- vTaskPrioritySet() - Up/down arrow icon
- vTaskDelete() - X icon
- uxTaskGetStackHighWaterMark() - Water level icon
Each with brief description and usage
Clean API documentation style, 16:9 format.
```

