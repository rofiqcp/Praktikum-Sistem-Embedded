#!/usr/bin/env python3
"""
Generator script for Modul 10, 11, 12 - FreeRTOS Practicum
Creates 72 C programs (ESP32 + STM32) + 72 Python debug/analysis scripts
"""

import os

BASE = "/root/otomasi/Praktikum-Sistem-Embedded"

# ============================================================================
# TEMPLATES
# ============================================================================

PLATFORMIO_ESP32 = """[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = espidf
monitor_speed = 115200
"""

PLATFORMIO_STM32 = """[env:bluepill_f103c8]
platform = ststm32
board = bluepill_f103c8
framework = stm32cube
extra_scripts = pre:extra_script_F103.py
build_flags = 
    -D USE_HAL_DRIVER
    -D STM32F103xB
    -D HSE_VALUE=8000000
    -I src
    -I include
    -Wno-unused-variable
    -Wno-unused-function
"""

EXTRA_SCRIPT = r'''"""
Custom PlatformIO build script for STM32CubeF1 Framework
"""
Import("env")
import os, glob

platform = env.PioPlatform()
FRAMEWORK_DIR = platform.get_package_dir("framework-stm32cubef1")

framework_includes = []

def add_include_path(base_path, relative_paths):
    for rel_path in relative_paths:
        full_path = os.path.join(base_path, rel_path)
        if os.path.exists(full_path):
            framework_includes.append(full_path)

add_include_path(FRAMEWORK_DIR, [
    "Drivers/STM32F1xx_HAL_Driver/Inc",
    "Drivers/CMSIS/Device/ST/STM32F1xx/Include",
    "Drivers/CMSIS/Include",
])

freertos_base = os.path.join(FRAMEWORK_DIR, "Middlewares", "Third_Party", "FreeRTOS", "Source")
add_include_path(freertos_base, ["include", "portable/GCC/ARM_CM3"])
env.Append(CPPPATH=framework_includes)

freertos_sources = ["tasks.c", "queue.c", "list.c", "timers.c",
                    "portable/GCC/ARM_CM3/port.c", "portable/MemMang/heap_4.c"]
freertos_objs = []
for src in freertos_sources:
    src_path = os.path.join(freertos_base, src)
    if os.path.exists(src_path):
        obj_name = src.replace("/", "_").replace(".c", ".o")
        obj_path = os.path.join("$BUILD_DIR", "FreeRTOS_obj", obj_name)
        obj = env.Object(obj_path, src_path)
        freertos_objs.append(obj)
env.Append(PIOBUILDFILES=freertos_objs)
print(f"[FreeRTOS] Compiled {len(freertos_objs)} source files")
'''


def freertos_config_h(title, desc, use_mutexes=1, use_recursive=0, use_counting=0, 
                      use_timers=0, use_task_notifications=1, check_stack=2,
                      malloc_hook=1, total_heap="15 * 1024", use_queue_sets=0,
                      include_delete=0, include_suspend=0, extra_defines="",
                      use_stream_buffers=0, use_event_groups=0):
    return f'''#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f1xx.h"

/* {title} - {desc} */

#define configUSE_PREEMPTION                    1
#define configCPU_CLOCK_HZ                      (SystemCoreClock)
#define configTICK_RATE_HZ                      ((TickType_t)1000)
#define configMAX_PRIORITIES                    8
#define configMINIMAL_STACK_SIZE                ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                   ((size_t)({total_heap}))
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1

#define configUSE_MUTEXES                       {use_mutexes}
#define configUSE_RECURSIVE_MUTEXES             {use_recursive}
#define configUSE_COUNTING_SEMAPHORES           {use_counting}
#define configUSE_QUEUE_SETS                    {use_queue_sets}
#define configUSE_TASK_NOTIFICATIONS            {use_task_notifications}

#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1

#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_MALLOC_FAILED_HOOK            {malloc_hook}
#define configCHECK_FOR_STACK_OVERFLOW          {check_stack}

#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

#define configUSE_TIMERS                        {use_timers}
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            (configMINIMAL_STACK_SIZE * 2)

#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS 4
#endif
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY         (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define xPortPendSVHandler  PendSV_Handler
#define vPortSVCHandler     SVC_Handler

#define INCLUDE_vTaskPrioritySet        1
#define INCLUDE_uxTaskPriorityGet       1
#define INCLUDE_vTaskDelete             {include_delete}
#define INCLUDE_vTaskSuspend            {include_suspend}
#define INCLUDE_vTaskDelayUntil         1
#define INCLUDE_vTaskDelay              1
#define INCLUDE_xTaskGetSchedulerState  1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_eTaskGetState           1
#define INCLUDE_xTaskResumeFromISR      0

{extra_defines}

#define LED_GPIO_PORT   GPIOC
#define LED_GPIO_PIN    GPIO_PIN_13
#define LED_ACTIVE_LOW  1
#define DEBUG_UART_INSTANCE  USART1
#define DEBUG_UART_BAUDRATE  115200
#define DEBUG_UART_TX_PORT   GPIOA
#define DEBUG_UART_TX_PIN    GPIO_PIN_9
#define DEBUG_UART_RX_PORT   GPIOA
#define DEBUG_UART_RX_PIN    GPIO_PIN_10

#endif /* FREERTOS_CONFIG_H */
'''

# ============================================================================
# PYTHON DEBUG SCRIPT TEMPLATE
# ============================================================================

def python_debug_script(exercise_name, description, analysis_type="general"):
    """Generate Python serial debug & analysis script"""
    
    extra_imports = ""
    extra_analysis = ""
    
    if "queue" in analysis_type.lower() or "producer" in analysis_type.lower():
        extra_analysis = '''
    def analyze_queue_metrics(self):
        """Analyze queue send/receive patterns"""
        sends = [l for l in self.data_lines if 'send' in l.lower() or 'enqueue' in l.lower()]
        recvs = [l for l in self.data_lines if 'recv' in l.lower() or 'dequeue' in l.lower() or 'receive' in l.lower()]
        fulls = [l for l in self.data_lines if 'full' in l.lower()]
        
        print(f"\\n{'='*60}")
        print(f"  QUEUE ANALYSIS")
        print(f"{'='*60}")
        print(f"  Total Send operations  : {len(sends)}")
        print(f"  Total Receive operations: {len(recvs)}")
        print(f"  Queue Full events      : {len(fulls)}")
        if len(sends) > 0:
            print(f"  Success rate           : {len(recvs)/len(sends)*100:.1f}%")
        print(f"{'='*60}")
'''
    elif "semaphore" in analysis_type.lower() or "mutex" in analysis_type.lower():
        extra_analysis = '''
    def analyze_sync_metrics(self):
        """Analyze semaphore/mutex patterns"""
        takes = [l for l in self.data_lines if 'take' in l.lower() or 'lock' in l.lower() or 'acquire' in l.lower()]
        gives = [l for l in self.data_lines if 'give' in l.lower() or 'unlock' in l.lower() or 'release' in l.lower()]
        blocks = [l for l in self.data_lines if 'block' in l.lower() or 'wait' in l.lower()]
        
        print(f"\\n{'='*60}")
        print(f"  SYNCHRONIZATION ANALYSIS")
        print(f"{'='*60}")
        print(f"  Total Take/Lock   : {len(takes)}")
        print(f"  Total Give/Unlock : {len(gives)}")
        print(f"  Blocking events   : {len(blocks)}")
        if len(takes) > 0 and len(gives) > 0:
            print(f"  Balance (give-take): {len(gives) - len(takes)}")
        print(f"{'='*60}")
'''
    elif "timer" in analysis_type.lower():
        extra_analysis = '''
    def analyze_timer_metrics(self):
        """Analyze timer callback patterns"""
        callbacks = [l for l in self.data_lines if 'callback' in l.lower() or 'timer' in l.lower() or 'expired' in l.lower()]
        
        print(f"\\n{'='*60}")
        print(f"  TIMER ANALYSIS")
        print(f"{'='*60}")
        print(f"  Timer callbacks fired: {len(callbacks)}")
        
        # Try to extract timing
        timestamps = []
        for l in callbacks:
            for word in l.split():
                try:
                    val = int(word)
                    if val > 100:  # likely a tick count
                        timestamps.append(val)
                except ValueError:
                    pass
        
        if len(timestamps) >= 2:
            intervals = [timestamps[i+1] - timestamps[i] for i in range(len(timestamps)-1)]
            avg_interval = sum(intervals) / len(intervals)
            print(f"  Avg callback interval: {avg_interval:.1f} ticks")
            print(f"  Min interval         : {min(intervals)} ticks")
            print(f"  Max interval         : {max(intervals)} ticks")
        print(f"{'='*60}")
'''
    elif "notification" in analysis_type.lower():
        extra_analysis = '''
    def analyze_notification_metrics(self):
        """Analyze task notification patterns"""
        notifies = [l for l in self.data_lines if 'notify' in l.lower() or 'notification' in l.lower()]
        waits = [l for l in self.data_lines if 'wait' in l.lower() or 'take' in l.lower()]
        
        print(f"\\n{'='*60}")
        print(f"  NOTIFICATION ANALYSIS")
        print(f"{'='*60}")
        print(f"  Notifications sent   : {len(notifies)}")
        print(f"  Wait/Take operations : {len(waits)}")
        print(f"{'='*60}")
'''
    elif "memory" in analysis_type.lower() or "heap" in analysis_type.lower():
        extra_analysis = '''
    def analyze_memory_metrics(self):
        """Analyze memory/heap usage patterns"""
        allocs = [l for l in self.data_lines if 'alloc' in l.lower() or 'malloc' in l.lower()]
        frees = [l for l in self.data_lines if 'free' in l.lower()]
        heap_vals = []
        
        for l in self.data_lines:
            if 'heap' in l.lower() or 'free' in l.lower():
                for word in l.split():
                    try:
                        val = int(word)
                        if val > 100:
                            heap_vals.append(val)
                    except ValueError:
                        pass
        
        print(f"\\n{'='*60}")
        print(f"  MEMORY ANALYSIS")
        print(f"{'='*60}")
        print(f"  Allocation events : {len(allocs)}")
        print(f"  Free events       : {len(frees)}")
        if heap_vals:
            print(f"  Max heap reported : {max(heap_vals)} bytes")
            print(f"  Min heap reported : {min(heap_vals)} bytes")
            print(f"  Heap delta        : {max(heap_vals) - min(heap_vals)} bytes")
        print(f"{'='*60}")
'''
    else:
        extra_analysis = '''
    def analyze_general_metrics(self):
        """General analysis of serial output"""
        errors = [l for l in self.data_lines if 'error' in l.lower() or 'fail' in l.lower()]
        warnings = [l for l in self.data_lines if 'warn' in l.lower()]
        
        print(f"\\n{'='*60}")
        print(f"  GENERAL ANALYSIS")
        print(f"{'='*60}")
        print(f"  Total lines captured : {len(self.data_lines)}")
        print(f"  Error lines          : {len(errors)}")
        print(f"  Warning lines        : {len(warnings)}")
        print(f"{'='*60}")
'''

    return f'''#!/usr/bin/env python3
"""
==========================================================
 DEBUG & ANALYSIS SCRIPT: {exercise_name}
 {description}
==========================================================
 Tool untuk monitoring serial output dari ESP32/STM32,
 parsing data, dan analisa performa FreeRTOS.

 Penggunaan:
   python3 debug_{exercise_name.lower()}.py [PORT] [BAUDRATE]
   
 Contoh:
   python3 debug_{exercise_name.lower()}.py /dev/ttyUSB0 115200
   python3 debug_{exercise_name.lower()}.py COM3 115200
==========================================================
"""

import serial
import serial.tools.list_ports
import sys
import time
import signal
import json
import csv
from datetime import datetime
from collections import deque

class SerialDebugger:
    """Serial port debugger and data analyzer for FreeRTOS exercises"""
    
    def __init__(self, port=None, baudrate=115200, timeout=1):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial_conn = None
        self.data_lines = []
        self.running = True
        self.start_time = None
        self.log_file = f"log_{{exercise_name.lower()}}_{{datetime.now().strftime(\'%Y%m%d_%H%M%S\')}}.csv"
        
        signal.signal(signal.SIGINT, self._signal_handler)
    
    def _signal_handler(self, sig, frame):
        print("\\n\\n[INFO] Stopping capture (Ctrl+C)...")
        self.running = False
    
    @staticmethod
    def list_ports():
        """List all available serial ports"""
        ports = serial.tools.list_ports.comports()
        print("\\n Available Serial Ports:")
        print("-" * 50)
        for p in ports:
            print(f"  {{p.device}} - {{p.description}}")
        if not ports:
            print("  (No serial ports found)")
        print("-" * 50)
        return ports
    
    def connect(self):
        """Connect to serial port"""
        if not self.port:
            ports = self.list_ports()
            if ports:
                self.port = ports[0].device
                print(f"[AUTO] Using port: {{self.port}}")
            else:
                print("[ERROR] No serial ports available!")
                return False
        
        try:
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE
            )
            print(f"[OK] Connected to {{self.port}} @ {{self.baudrate}} baud")
            self.start_time = time.time()
            return True
        except serial.SerialException as e:
            print(f"[ERROR] Cannot open {{self.port}}: {{e}}")
            return False
    
    def capture(self, duration=30):
        """Capture serial data for specified duration"""
        print(f"\\n[CAPTURE] Recording for {{duration}}s... (Ctrl+C to stop early)")
        print("=" * 70)
        
        end_time = time.time() + duration
        line_count = 0
        
        with open(self.log_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_ms', 'raw_data'])
            
            while self.running and time.time() < end_time:
                try:
                    if self.serial_conn and self.serial_conn.in_waiting:
                        raw = self.serial_conn.readline()
                        try:
                            line = raw.decode('utf-8', errors='replace').strip()
                        except:
                            line = str(raw)
                        
                        if line:
                            elapsed = int((time.time() - self.start_time) * 1000)
                            timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                            
                            self.data_lines.append(line)
                            writer.writerow([timestamp, elapsed, line])
                            
                            line_count += 1
                            # Color-code output
                            if 'error' in line.lower() or 'fail' in line.lower():
                                print(f"  [{{timestamp}}] \\033[91m{{line}}\\033[0m")
                            elif 'warn' in line.lower():
                                print(f"  [{{timestamp}}] \\033[93m{{line}}\\033[0m")
                            elif 'ok' in line.lower() or 'success' in line.lower():
                                print(f"  [{{timestamp}}] \\033[92m{{line}}\\033[0m")
                            else:
                                print(f"  [{{timestamp}}] {{line}}")
                except Exception as e:
                    print(f"[WARN] Read error: {{e}}")
        
        print("=" * 70)
        print(f"[DONE] Captured {{line_count}} lines in {{duration}}s")
        print(f"[SAVE] Log saved to: {{self.log_file}}")
    
    def extract_numeric_values(self, keyword):
        """Extract numeric values from lines containing keyword"""
        values = []
        for line in self.data_lines:
            if keyword.lower() in line.lower():
                for word in line.replace(',', ' ').replace(':', ' ').split():
                    try:
                        values.append(float(word))
                    except ValueError:
                        pass
        return values
{extra_analysis}
    
    def print_summary(self):
        """Print capture summary"""
        print(f"\\n{'='*60}")
        print(f"  CAPTURE SUMMARY - {exercise_name}")
        print(f"{'='*60}")
        print(f"  Port          : {{self.port}}")
        print(f"  Baudrate      : {{self.baudrate}}")
        print(f"  Total lines   : {{len(self.data_lines)}}")
        print(f"  Log file      : {{self.log_file}}")
        
        # Count unique patterns
        patterns = {{}}
        for line in self.data_lines:
            key = line.split(':')[0].strip() if ':' in line else line[:30]
            patterns[key] = patterns.get(key, 0) + 1
        
        print(f"\\n  Top message patterns:")
        for pat, count in sorted(patterns.items(), key=lambda x: -x[1])[:10]:
            print(f"    {{count:4d}}x | {{pat[:50]}}")
        print(f"{'='*60}")
    
    def close(self):
        """Close serial connection"""
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
            print("[OK] Serial port closed")


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else None
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
    duration = int(sys.argv[3]) if len(sys.argv) > 3 else 30
    
    print(f"\\n{'='*60}")
    print(f"  {exercise_name} - Debug & Analysis Tool")
    print(f"  {description}")
    print(f"{'='*60}")
    
    dbg = SerialDebugger(port=port, baudrate=baudrate)
    
    if not dbg.connect():
        print("\\n[TIP] Jalankan tanpa argumen untuk auto-detect port")
        print("[TIP] Atau: python3 {{sys.argv[0]}} /dev/ttyUSB0 115200 30")
        return
    
    try:
        dbg.capture(duration=duration)
        dbg.print_summary()
        # Run analysis
        for method_name in dir(dbg):
            if method_name.startswith('analyze_'):
                getattr(dbg, method_name)()
    finally:
        dbg.close()


if __name__ == "__main__":
    main()
'''


# ============================================================================
# MODUL 10: Queue & Semaphore — ESP32 Programs
# ============================================================================

M10_ESP32 = {
"ESP32_01_Queue_Basic": {
"main": r'''/*
 * ===========================================================================
 * MODUL 10 - Percobaan 01: Queue Basic
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Mengirim integer via queue — producer-consumer pattern dasar.
 * Producer task mengirim counter ke queue, consumer task menerima & mencetak.
 * Hardware: Serial Monitor + 1x LED (GPIO2)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN     GPIO_NUM_2
#define QUEUE_LEN   5

static const char *TAG = "QUEUE_BASIC";
static QueueHandle_t xQueue;

static void producer_task(void *pvParam)
{
    int counter = 0;
    while (1) {
        counter++;
        if (xQueueSend(xQueue, &counter, pdMS_TO_TICKS(100)) == pdPASS) {
            ESP_LOGI(TAG, "[PRODUCER] Sent: %d", counter);
        } else {
            ESP_LOGW(TAG, "[PRODUCER] Queue FULL, cannot send %d", counter);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void consumer_task(void *pvParam)
{
    int received;
    while (1) {
        if (xQueueReceive(xQueue, &received, pdMS_TO_TICKS(1000)) == pdPASS) {
            ESP_LOGI(TAG, "[CONSUMER] Received: %d", received);
            gpio_set_level(LED_PIN, received % 2);  /* Toggle LED */
        } else {
            ESP_LOGW(TAG, "[CONSUMER] Timeout - no data in queue");
        }
    }
}

void app_main(void)
{
    /* Configure LED */
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "=== MODUL 10: Queue Basic - Producer Consumer ===");
    ESP_LOGI(TAG, "Queue length: %d, Item size: %d bytes", QUEUE_LEN, sizeof(int));

    /* Create queue */
    xQueue = xQueueCreate(QUEUE_LEN, sizeof(int));
    if (xQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue!");
        return;
    }

    /* Create tasks */
    xTaskCreate(producer_task, "Producer", 2048, NULL, 2, NULL);
    xTaskCreate(consumer_task, "Consumer", 2048, NULL, 1, NULL);

    ESP_LOGI(TAG, "Tasks created. Scheduler running...");
}
''',
"py_type": "queue"
},

"ESP32_02_Queue_Struct": {
"main": r'''/*
 * ===========================================================================
 * MODUL 10 - Percobaan 02: Queue Struct
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Mengirim struct (sensor data) via queue — type-safe messaging.
 * Demonstrasi pengiriman data kompleks antar task.
 * Hardware: Serial Monitor
 * ===========================================================================
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_random.h"

#define QUEUE_LEN   10

static const char *TAG = "QUEUE_STRUCT";

typedef enum {
    SENSOR_TEMPERATURE,
    SENSOR_HUMIDITY,
    SENSOR_PRESSURE
} sensor_type_t;

typedef struct {
    sensor_type_t type;
    float value;
    uint32_t timestamp;
    uint8_t sensor_id;
} sensor_data_t;

static QueueHandle_t xSensorQueue;

static const char* sensor_name(sensor_type_t t) {
    switch(t) {
        case SENSOR_TEMPERATURE: return "TEMP";
        case SENSOR_HUMIDITY:    return "HUMI";
        case SENSOR_PRESSURE:    return "PRES";
        default: return "UNKN";
    }
}

static void sensor_task(void *pvParam)
{
    uint8_t id = (uint8_t)(uintptr_t)pvParam;
    sensor_data_t data;
    
    while (1) {
        data.sensor_id = id;
        data.timestamp = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        
        switch (id % 3) {
            case 0:
                data.type = SENSOR_TEMPERATURE;
                data.value = 20.0f + (float)(esp_random() % 150) / 10.0f;
                break;
            case 1:
                data.type = SENSOR_HUMIDITY;
                data.value = 40.0f + (float)(esp_random() % 400) / 10.0f;
                break;
            case 2:
                data.type = SENSOR_PRESSURE;
                data.value = 1000.0f + (float)(esp_random() % 500) / 10.0f;
                break;
        }
        
        if (xQueueSend(xSensorQueue, &data, pdMS_TO_TICKS(100)) == pdPASS) {
            ESP_LOGI(TAG, "[SENSOR_%d] Sent: %s = %.1f @%lums",
                     id, sensor_name(data.type), data.value, (unsigned long)data.timestamp);
        } else {
            ESP_LOGW(TAG, "[SENSOR_%d] Queue FULL!", id);
        }
        vTaskDelay(pdMS_TO_TICKS(300 + (id * 200)));
    }
}

static void processor_task(void *pvParam)
{
    sensor_data_t data;
    uint32_t count = 0;
    
    while (1) {
        if (xQueueReceive(xSensorQueue, &data, pdMS_TO_TICKS(2000)) == pdPASS) {
            count++;
            ESP_LOGI(TAG, "[PROC] #%lu Sensor_%d %s=%.1f t=%lums",
                     (unsigned long)count, data.sensor_id,
                     sensor_name(data.type), data.value, (unsigned long)data.timestamp);
        } else {
            ESP_LOGW(TAG, "[PROC] No data for 2s!");
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== MODUL 10: Queue Struct - Sensor Data ===");
    ESP_LOGI(TAG, "Struct size: %d bytes", sizeof(sensor_data_t));
    
    xSensorQueue = xQueueCreate(QUEUE_LEN, sizeof(sensor_data_t));
    if (xSensorQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue!");
        return;
    }
    
    xTaskCreate(sensor_task, "Sensor0", 2048, (void*)0, 2, NULL);
    xTaskCreate(sensor_task, "Sensor1", 2048, (void*)1, 2, NULL);
    xTaskCreate(sensor_task, "Sensor2", 2048, (void*)2, 2, NULL);
    xTaskCreate(processor_task, "Processor", 2048, NULL, 3, NULL);
}
''',
"py_type": "queue"
},

"ESP32_03_Queue_Multiple": {
"main": r'''/*
 * ===========================================================================
 * MODUL 10 - Percobaan 03: Queue Multiple
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Menggunakan multiple queues untuk routing pesan ke handler berbeda.
 * Hardware: Serial Monitor + 2x LED (GPIO2, GPIO4)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"

#define LED1_PIN    GPIO_NUM_2
#define LED2_PIN    GPIO_NUM_4

static const char *TAG = "QUEUE_MULTI";
static QueueHandle_t xCommandQueue;
static QueueHandle_t xStatusQueue;

typedef struct {
    uint8_t cmd_id;
    uint16_t param;
} command_t;

typedef struct {
    uint8_t cmd_id;
    uint8_t status;  /* 0=OK, 1=ERROR */
    uint32_t tick;
} status_t;

static void commander_task(void *pvParam)
{
    command_t cmd;
    uint8_t id = 0;
    while (1) {
        cmd.cmd_id = id++;
        cmd.param = esp_random() % 1000;
        
        if (xQueueSend(xCommandQueue, &cmd, pdMS_TO_TICKS(100)) == pdPASS) {
            ESP_LOGI(TAG, "[CMD] Sent cmd #%d param=%d", cmd.cmd_id, cmd.param);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void executor_task(void *pvParam)
{
    command_t cmd;
    status_t sts;
    while (1) {
        if (xQueueReceive(xCommandQueue, &cmd, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "[EXEC] Processing cmd #%d param=%d", cmd.cmd_id, cmd.param);
            
            /* Toggle LEDs based on command */
            gpio_set_level(LED1_PIN, cmd.param % 2);
            gpio_set_level(LED2_PIN, (cmd.param / 2) % 2);
            
            vTaskDelay(pdMS_TO_TICKS(200)); /* Simulate work */
            
            /* Send status back */
            sts.cmd_id = cmd.cmd_id;
            sts.status = (cmd.param > 500) ? 1 : 0;
            sts.tick = xTaskGetTickCount();
            xQueueSend(xStatusQueue, &sts, pdMS_TO_TICKS(100));
        }
    }
}

static void monitor_task(void *pvParam)
{
    status_t sts;
    while (1) {
        if (xQueueReceive(xStatusQueue, &sts, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "[MON] Cmd #%d %s @tick=%lu",
                     sts.cmd_id,
                     sts.status == 0 ? "OK" : "ERROR",
                     (unsigned long)sts.tick);
        }
    }
}

void app_main(void)
{
    gpio_reset_pin(LED1_PIN); gpio_set_direction(LED1_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED2_PIN); gpio_set_direction(LED2_PIN, GPIO_MODE_OUTPUT);
    
    ESP_LOGI(TAG, "=== MODUL 10: Multiple Queues ===");
    
    xCommandQueue = xQueueCreate(5, sizeof(command_t));
    xStatusQueue  = xQueueCreate(5, sizeof(status_t));
    
    xTaskCreate(commander_task, "Commander", 2048, NULL, 2, NULL);
    xTaskCreate(executor_task,  "Executor",  2048, NULL, 3, NULL);
    xTaskCreate(monitor_task,   "Monitor",   2048, NULL, 1, NULL);
}
''',
"py_type": "queue"
},

"ESP32_04_Queue_ISR": {
"main": r'''/*
 * ===========================================================================
 * MODUL 10 - Percobaan 04: Queue ISR
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * xQueueSendFromISR() — button interrupt sends event via queue to task.
 * Hardware: 1x Push button (GPIO0) + 1x LED (GPIO2)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BTN_PIN     GPIO_NUM_0
#define LED_PIN     GPIO_NUM_2
#define QUEUE_LEN   10

static const char *TAG = "QUEUE_ISR";
static QueueHandle_t xButtonQueue;

typedef struct {
    uint32_t tick;
    int level;
} button_event_t;

static void IRAM_ATTR button_isr(void *arg)
{
    button_event_t evt;
    evt.tick = xTaskGetTickCountFromISR();
    evt.level = gpio_get_level(BTN_PIN);
    
    BaseType_t xHigherPrioWoken = pdFALSE;
    xQueueSendFromISR(xButtonQueue, &evt, &xHigherPrioWoken);
    if (xHigherPrioWoken) {
        portYIELD_FROM_ISR();
    }
}

static void led_handler_task(void *pvParam)
{
    button_event_t evt;
    int led_state = 0;
    uint32_t press_count = 0;
    
    while (1) {
        if (xQueueReceive(xButtonQueue, &evt, portMAX_DELAY) == pdPASS) {
            if (evt.level == 0) {  /* Button pressed (active low) */
                press_count++;
                led_state = !led_state;
                gpio_set_level(LED_PIN, led_state);
                ESP_LOGI(TAG, "[ISR→TASK] Button #%lu @tick=%lu LED=%s",
                         (unsigned long)press_count, (unsigned long)evt.tick,
                         led_state ? "ON" : "OFF");
            }
        }
    }
}

void app_main(void)
{
    /* LED setup */
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    /* Button setup with interrupt */
    gpio_reset_pin(BTN_PIN);
    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BTN_PIN, GPIO_INTR_ANYEDGE);
    
    ESP_LOGI(TAG, "=== MODUL 10: Queue from ISR ===");
    
    xButtonQueue = xQueueCreate(QUEUE_LEN, sizeof(button_event_t));
    
    xTaskCreate(led_handler_task, "LEDHandler", 2048, NULL, 3, NULL);
    
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_PIN, button_isr, NULL);
    
    ESP_LOGI(TAG, "Press BOOT button (GPIO0) to toggle LED");
}
''',
"py_type": "queue"
},

"ESP32_05_Queue_Set": {
"main": r'''/*
 * ===========================================================================
 * MODUL 10 - Percobaan 05: Queue Set
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * xQueueCreateSet() — wait on multiple queues simultaneously.
 * Hardware: Serial Monitor
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "QUEUE_SET";

static QueueHandle_t xTempQueue;
static QueueHandle_t xHumiQueue;
static QueueSetHandle_t xQueueSet;

static void temp_sender(void *pvParam)
{
    float temp;
    while (1) {
        temp = 20.0f + (float)(esp_random() % 200) / 10.0f;
        xQueueSend(xTempQueue, &temp, pdMS_TO_TICKS(100));
        ESP_LOGI(TAG, "[TEMP_TX] Sent: %.1f°C", temp);
        vTaskDelay(pdMS_TO_TICKS(700));
    }
}

static void humi_sender(void *pvParam)
{
    float humi;
    while (1) {
        humi = 30.0f + (float)(esp_random() % 500) / 10.0f;
        xQueueSend(xHumiQueue, &humi, pdMS_TO_TICKS(100));
        ESP_LOGI(TAG, "[HUMI_TX] Sent: %.1f%%", humi);
        vTaskDelay(pdMS_TO_TICKS(1100));
    }
}

static void mux_receiver(void *pvParam)
{
    QueueSetMemberHandle_t xActiveMember;
    float value;
    
    while (1) {
        xActiveMember = xQueueSelectFromSet(xQueueSet, portMAX_DELAY);
        
        if (xActiveMember == xTempQueue) {
            xQueueReceive(xTempQueue, &value, 0);
            ESP_LOGI(TAG, "[MUX] Temperature: %.1f°C", value);
        } else if (xActiveMember == xHumiQueue) {
            xQueueReceive(xHumiQueue, &value, 0);
            ESP_LOGI(TAG, "[MUX] Humidity: %.1f%%", value);
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== MODUL 10: Queue Set (Select/Multiplex) ===");
    
    xTempQueue = xQueueCreate(5, sizeof(float));
    xHumiQueue = xQueueCreate(5, sizeof(float));
    
    /* Queue set must be large enough: sum of all queue lengths */
    xQueueSet = xQueueCreateSet(5 + 5);
    xQueueAddToSet(xTempQueue, xQueueSet);
    xQueueAddToSet(xHumiQueue, xQueueSet);
    
    xTaskCreate(temp_sender, "TempTX", 2048, NULL, 2, NULL);
    xTaskCreate(humi_sender, "HumiTX", 2048, NULL, 2, NULL);
    xTaskCreate(mux_receiver, "MuxRX", 2048, NULL, 3, NULL);
}
''',
"py_type": "queue"
},

"ESP32_06_Binary_Semaphore": {
"main": r'''/*
 * ===========================================================================
 * MODUL 10 - Percobaan 06: Binary Semaphore
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Binary semaphore: ISR gives → task takes. Event signaling pattern.
 * Hardware: 1x Push button (GPIO0) + 1x LED (GPIO2)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BTN_PIN     GPIO_NUM_0
#define LED_PIN     GPIO_NUM_2

static const char *TAG = "BIN_SEMA";
static SemaphoreHandle_t xBinarySem;

static void IRAM_ATTR button_isr(void *arg)
{
    BaseType_t xHigherPrioWoken = pdFALSE;
    xSemaphoreGiveFromISR(xBinarySem, &xHigherPrioWoken);
    if (xHigherPrioWoken) portYIELD_FROM_ISR();
}

static void led_task(void *pvParam)
{
    int state = 0;
    uint32_t count = 0;
    
    while (1) {
        /* Block until ISR signals */
        if (xSemaphoreTake(xBinarySem, portMAX_DELAY) == pdTRUE) {
            count++;
            state = !state;
            gpio_set_level(LED_PIN, state);
            ESP_LOGI(TAG, "[TASK] Button event #%lu — LED %s",
                     (unsigned long)count, state ? "ON" : "OFF");
        }
    }
}

void app_main(void)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    gpio_reset_pin(BTN_PIN);
    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BTN_PIN, GPIO_INTR_NEGEDGE);
    
    ESP_LOGI(TAG, "=== MODUL 10: Binary Semaphore (ISR → Task) ===");
    
    xBinarySem = xSemaphoreCreateBinary();
    
    xTaskCreate(led_task, "LEDTask", 2048, NULL, 3, NULL);
    
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_PIN, button_isr, NULL);
    
    ESP_LOGI(TAG, "Press BOOT button to signal semaphore");
}
''',
"py_type": "semaphore"
},

"ESP32_07_Counting_Semaphore": {
"main": r'''/*
 * ===========================================================================
 * MODUL 10 - Percobaan 07: Counting Semaphore
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Resource pool management: 3 resources shared by 5 tasks.
 * Parking lot analogy — counting semaphore limits concurrent access.
 * Hardware: Serial Monitor + 3x LED (GPIO2, GPIO4, GPIO5)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"

#define NUM_RESOURCES   3
#define NUM_WORKERS     5

static const gpio_num_t led_pins[3] = {GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_5};
static const char *TAG = "COUNT_SEM";
static SemaphoreHandle_t xResourceSem;

static void worker_task(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    
    while (1) {
        ESP_LOGI(TAG, "[W%d] Waiting for resource... (avail=%lu)",
                 id, (unsigned long)uxSemaphoreGetCount(xResourceSem));
        
        if (xSemaphoreTake(xResourceSem, portMAX_DELAY) == pdTRUE) {
            UBaseType_t avail = uxSemaphoreGetCount(xResourceSem);
            int res_idx = NUM_RESOURCES - 1 - (int)avail;
            if (res_idx < 0) res_idx = 0;
            if (res_idx >= NUM_RESOURCES) res_idx = NUM_RESOURCES - 1;
            
            gpio_set_level(led_pins[res_idx], 1);
            ESP_LOGI(TAG, "[W%d] ACQUIRED resource (LED%d ON, avail=%lu)",
                     id, res_idx, (unsigned long)avail);
            
            /* Use resource */
            vTaskDelay(pdMS_TO_TICKS(1000 + (esp_random() % 2000)));
            
            gpio_set_level(led_pins[res_idx], 0);
            xSemaphoreGive(xResourceSem);
            ESP_LOGI(TAG, "[W%d] RELEASED resource (LED%d OFF)", id, res_idx);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    for (int i = 0; i < NUM_RESOURCES; i++) {
        gpio_reset_pin(led_pins[i]);
        gpio_set_direction(led_pins[i], GPIO_MODE_OUTPUT);
    }
    
    ESP_LOGI(TAG, "=== MODUL 10: Counting Semaphore ===");
    ESP_LOGI(TAG, "%d resources, %d workers", NUM_RESOURCES, NUM_WORKERS);
    
    xResourceSem = xSemaphoreCreateCounting(NUM_RESOURCES, NUM_RESOURCES);
    
    for (int i = 0; i < NUM_WORKERS; i++) {
        char name[16];
        snprintf(name, sizeof(name), "Worker%d", i);
        xTaskCreate(worker_task, name, 2048, (void*)(uintptr_t)i, 2, NULL);
    }
}
''',
"py_type": "semaphore"
},

"ESP32_08_Mutex_Shared_Resource": {
"main": r'''/*
 * ===========================================================================
 * MODUL 10 - Percobaan 08: Mutex Shared Resource
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Melindungi shared variable dengan mutex.
 * Demonstrasi: TANPA mutex (race condition) vs DENGAN mutex (aman).
 * Hardware: Serial Monitor
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "MUTEX";
static SemaphoreHandle_t xMutex;

/* Shared resource */
static volatile int shared_counter = 0;
static volatile int unsafe_counter = 0;

static void safe_increment_task(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    for (int i = 0; i < 1000; i++) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            int temp = shared_counter;
            temp++;
            /* Small delay to increase chance of race condition */
            for (volatile int j = 0; j < 10; j++) {}
            shared_counter = temp;
            xSemaphoreGive(xMutex);
        }
    }
    ESP_LOGI(TAG, "[SAFE_%d] Finished 1000 increments. Counter=%d", id, shared_counter);
    vTaskDelete(NULL);
}

static void unsafe_increment_task(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    for (int i = 0; i < 1000; i++) {
        int temp = unsafe_counter;
        temp++;
        for (volatile int j = 0; j < 10; j++) {}
        unsafe_counter = temp;
    }
    ESP_LOGI(TAG, "[UNSAFE_%d] Finished 1000 increments. Counter=%d", id, unsafe_counter);
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== MODUL 10: Mutex — Shared Resource Protection ===");
    
    xMutex = xSemaphoreCreateMutex();
    
    /* Phase 1: UNSAFE — no mutex */
    ESP_LOGW(TAG, "--- Phase 1: WITHOUT MUTEX (expect race condition) ---");
    unsafe_counter = 0;
    
    TaskHandle_t h1, h2;
    xTaskCreate(unsafe_increment_task, "Unsafe1", 2048, (void*)1, 2, &h1);
    xTaskCreate(unsafe_increment_task, "Unsafe2", 2048, (void*)2, 2, &h2);
    
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGW(TAG, "UNSAFE result: %d (expected 2000)", unsafe_counter);
    
    /* Phase 2: SAFE — with mutex */
    ESP_LOGI(TAG, "--- Phase 2: WITH MUTEX (correct result) ---");
    shared_counter = 0;
    
    xTaskCreate(safe_increment_task, "Safe1", 2048, (void*)1, 2, NULL);
    xTaskCreate(safe_increment_task, "Safe2", 2048, (void*)2, 2, NULL);
    
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(TAG, "SAFE result: %d (expected 2000)", shared_counter);
    
    ESP_LOGI(TAG, "=== COMPARISON: unsafe=%d vs safe=%d ===",
             unsafe_counter, shared_counter);
}
''',
"py_type": "mutex"
},

"ESP32_09_Mutex_Priority_Inversion": {
"main": r'''/*
 * ===========================================================================
 * MODUL 10 - Percobaan 09: Mutex Priority Inversion
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Demo: Priority inversion problem & fix with priority inheritance.
 * Low priority holds mutex → high priority blocked → medium runs first!
 * Mutex has priority inheritance built-in on ESP32.
 * Hardware: Serial Monitor + 3x LED (GPIO2, GPIO4, GPIO5)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_LOW     GPIO_NUM_2
#define LED_MED     GPIO_NUM_4
#define LED_HIGH    GPIO_NUM_5

static const char *TAG = "PRI_INV";
static SemaphoreHandle_t xMutex;

static void busy_wait_ms(int ms)
{
    TickType_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(ms)) {
        taskYIELD();
    }
}

/* LOW priority task — holds mutex for a long time */
static void low_task(void *pvParam)
{
    while (1) {
        ESP_LOGI(TAG, "[LOW  prio=1] Taking mutex...");
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            gpio_set_level(LED_LOW, 1);
            ESP_LOGW(TAG, "[LOW  prio=1] GOT mutex — working 3s...");
            busy_wait_ms(3000);
            gpio_set_level(LED_LOW, 0);
            ESP_LOGW(TAG, "[LOW  prio=1] Releasing mutex");
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* MEDIUM priority task — no mutex, just CPU-bound */
static void medium_task(void *pvParam)
{
    vTaskDelay(pdMS_TO_TICKS(500)); /* Start slightly after low */
    while (1) {
        gpio_set_level(LED_MED, 1);
        ESP_LOGI(TAG, "[MED  prio=2] Running (no mutex needed)...");
        busy_wait_ms(2000);
        gpio_set_level(LED_MED, 0);
        ESP_LOGI(TAG, "[MED  prio=2] Done");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* HIGH priority task — needs mutex */
static void high_task(void *pvParam)
{
    vTaskDelay(pdMS_TO_TICKS(1000)); /* Start after low has mutex */
    while (1) {
        ESP_LOGE(TAG, "[HIGH prio=3] Need mutex! Taking...");
        TickType_t t0 = xTaskGetTickCount();
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            TickType_t waited = xTaskGetTickCount() - t0;
            gpio_set_level(LED_HIGH, 1);
            ESP_LOGE(TAG, "[HIGH prio=3] GOT mutex after %lu ms (priority inheritance!)",
                     (unsigned long)(waited * portTICK_PERIOD_MS));
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level(LED_HIGH, 0);
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    gpio_reset_pin(LED_LOW);  gpio_set_direction(LED_LOW, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED_MED);  gpio_set_direction(LED_MED, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED_HIGH); gpio_set_direction(LED_HIGH, GPIO_MODE_OUTPUT);
    
    ESP_LOGI(TAG, "=== MODUL 10: Priority Inversion Demo ===");
    ESP_LOGI(TAG, "Mutex includes priority inheritance (ESP32 default)");
    
    xMutex = xSemaphoreCreateMutex();
    
    xTaskCreate(low_task,    "Low",    2048, NULL, 1, NULL);
    xTaskCreate(medium_task, "Medium", 2048, NULL, 2, NULL);
    xTaskCreate(high_task,   "High",   2048, NULL, 3, NULL);
}
''',
"py_type": "mutex"
},

"ESP32_10_Recursive_Mutex": {
"main": r'''/*
 * ===========================================================================
 * MODUL 10 - Percobaan 10: Recursive Mutex
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Recursive mutex: same task can take the mutex multiple times.
 * Useful for nested function calls that each need the lock.
 * Hardware: Serial Monitor
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "REC_MUTEX";
static SemaphoreHandle_t xRecMutex;
static int shared_resource = 0;

/* Nested function that also needs the lock */
static void update_resource_inner(int delta)
{
    ESP_LOGI(TAG, "  [inner] Taking recursive mutex (2nd time)...");
    if (xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY) == pdTRUE) {
        shared_resource += delta;
        ESP_LOGI(TAG, "  [inner] Resource = %d (added %d)", shared_resource, delta);
        xSemaphoreGiveRecursive(xRecMutex);
        ESP_LOGI(TAG, "  [inner] Released (still held by outer)");
    }
}

/* Outer function that calls inner */
static void update_resource_outer(int value)
{
    ESP_LOGI(TAG, "[outer] Taking recursive mutex (1st time)...");
    if (xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "[outer] Got mutex. Current resource = %d", shared_resource);
        
        /* Call nested function — it will take mutex again */
        update_resource_inner(value);
        update_resource_inner(value * 2); /* Another nested call */
        
        ESP_LOGI(TAG, "[outer] After nested calls, resource = %d", shared_resource);
        xSemaphoreGiveRecursive(xRecMutex);
        ESP_LOGI(TAG, "[outer] Released mutex completely");
    }
}

static void task_func(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    int iter = 0;
    while (1) {
        iter++;
        ESP_LOGI(TAG, "\n=== Task %d, Iteration %d ===", id, iter);
        update_resource_outer(id * 10);
        vTaskDelay(pdMS_TO_TICKS(2000 + (id * 500)));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== MODUL 10: Recursive Mutex ===");
    ESP_LOGI(TAG, "Same task can lock mutex multiple times in nested calls");
    
    xRecMutex = xSemaphoreCreateRecursiveMutex();
    
    xTaskCreate(task_func, "Task1", 2048, (void*)1, 2, NULL);
    xTaskCreate(task_func, "Task2", 2048, (void*)2, 2, NULL);
}
''',
"py_type": "mutex"
},

"ESP32_11_Producer_Consumer": {
"main": r'''/*
 * ===========================================================================
 * MODUL 10 - Percobaan 11: Producer Consumer
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Classic producer-consumer with bounded buffer (queue).
 * Multiple producers, single consumer, rate monitoring.
 * Hardware: Serial Monitor + 1x LED (GPIO2)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"

#define LED_PIN         GPIO_NUM_2
#define BUFFER_SIZE     8
#define NUM_PRODUCERS   3

static const char *TAG = "PROD_CONS";
static QueueHandle_t xBuffer;

typedef struct {
    uint8_t producer_id;
    uint32_t seq;
    float data;
} item_t;

static void producer_task(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    item_t item;
    uint32_t seq = 0;
    
    while (1) {
        item.producer_id = id;
        item.seq = seq++;
        item.data = (float)(esp_random() % 10000) / 100.0f;
        
        ESP_LOGI(TAG, "[P%d] Producing item #%lu (%.2f) queue=%lu/%d",
                 id, (unsigned long)item.seq, item.data,
                 (unsigned long)(BUFFER_SIZE - uxQueueSpacesAvailable(xBuffer)),
                 BUFFER_SIZE);
        
        if (xQueueSend(xBuffer, &item, pdMS_TO_TICKS(2000)) == pdPASS) {
            ESP_LOGI(TAG, "[P%d] Item enqueued OK", id);
        } else {
            ESP_LOGW(TAG, "[P%d] BUFFER FULL! Item dropped!", id);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500 + (esp_random() % 1000)));
    }
}

static void consumer_task(void *pvParam)
{
    item_t item;
    uint32_t total = 0;
    
    while (1) {
        if (xQueueReceive(xBuffer, &item, portMAX_DELAY) == pdPASS) {
            total++;
            gpio_set_level(LED_PIN, total % 2);
            
            ESP_LOGI(TAG, "[CONSUMER] Item from P%d seq#%lu data=%.2f (total=%lu, pending=%lu)",
                     item.producer_id, (unsigned long)item.seq, item.data,
                     (unsigned long)total,
                     (unsigned long)(BUFFER_SIZE - uxQueueSpacesAvailable(xBuffer)));
            
            /* Simulate processing time */
            vTaskDelay(pdMS_TO_TICKS(200 + (esp_random() % 300)));
        }
    }
}

void app_main(void)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    ESP_LOGI(TAG, "=== MODUL 10: Producer-Consumer Pattern ===");
    ESP_LOGI(TAG, "%d producers, 1 consumer, buffer=%d", NUM_PRODUCERS, BUFFER_SIZE);
    
    xBuffer = xQueueCreate(BUFFER_SIZE, sizeof(item_t));
    
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        char name[16];
        snprintf(name, sizeof(name), "Producer%d", i);
        xTaskCreate(producer_task, name, 2048, (void*)(uintptr_t)i, 2, NULL);
    }
    xTaskCreate(consumer_task, "Consumer", 2048, NULL, 3, NULL);
}
''',
"py_type": "queue"
},

"ESP32_12_Reader_Writer": {
"main": r'''/*
 * ===========================================================================
 * MODUL 10 - Percobaan 12: Reader-Writer Lock
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Multiple readers can read concurrently, but writer needs exclusive access.
 * Implemented with a mutex + counting semaphore.
 * Hardware: Serial Monitor
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_random.h"

#define NUM_READERS 4
#define NUM_WRITERS 2

static const char *TAG = "RW_LOCK";

static SemaphoreHandle_t xWriteMutex;    /* Exclusive write access */
static SemaphoreHandle_t xReaderMutex;   /* Protect reader_count */
static int reader_count = 0;

/* Shared data */
static int shared_data = 0;
static int write_count = 0;

static void reader_task(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    int local_data;
    
    while (1) {
        /* Reader entry */
        xSemaphoreTake(xReaderMutex, portMAX_DELAY);
        reader_count++;
        if (reader_count == 1) {
            xSemaphoreTake(xWriteMutex, portMAX_DELAY); /* First reader blocks writers */
        }
        xSemaphoreGive(xReaderMutex);
        
        /* Critical section — READ */
        local_data = shared_data;
        ESP_LOGI(TAG, "[R%d] Read: %d (readers=%d)", id, local_data, reader_count);
        vTaskDelay(pdMS_TO_TICKS(200)); /* Simulate reading time */
        
        /* Reader exit */
        xSemaphoreTake(xReaderMutex, portMAX_DELAY);
        reader_count--;
        if (reader_count == 0) {
            xSemaphoreGive(xWriteMutex); /* Last reader unblocks writers */
        }
        xSemaphoreGive(xReaderMutex);
        
        vTaskDelay(pdMS_TO_TICKS(500 + (esp_random() % 1000)));
    }
}

static void writer_task(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    
    while (1) {
        xSemaphoreTake(xWriteMutex, portMAX_DELAY);
        
        /* Critical section — WRITE */
        write_count++;
        shared_data = write_count * 100 + id;
        ESP_LOGW(TAG, "[W%d] WRITE: %d (exclusive access)", id, shared_data);
        vTaskDelay(pdMS_TO_TICKS(500)); /* Simulate write time */
        
        xSemaphoreGive(xWriteMutex);
        
        vTaskDelay(pdMS_TO_TICKS(2000 + (esp_random() % 2000)));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== MODUL 10: Reader-Writer Lock ===");
    ESP_LOGI(TAG, "%d readers, %d writers", NUM_READERS, NUM_WRITERS);
    
    xWriteMutex  = xSemaphoreCreateMutex();
    xReaderMutex = xSemaphoreCreateMutex();
    
    for (int i = 0; i < NUM_READERS; i++) {
        char n[16]; snprintf(n, sizeof(n), "Reader%d", i);
        xTaskCreate(reader_task, n, 2048, (void*)(uintptr_t)i, 2, NULL);
    }
    for (int i = 0; i < NUM_WRITERS; i++) {
        char n[16]; snprintf(n, sizeof(n), "Writer%d", i);
        xTaskCreate(writer_task, n, 2048, (void*)(uintptr_t)i, 3, NULL);
    }
}
''',
"py_type": "mutex"
}
}

# Due to the massive size, I'll define the rest of the programs inline
# Continue with STM32 Modul 10, then ESP32/STM32 for Modul 11 and 12

def create_project(base_path, name, platform, main_c, freertos_cfg=None):
    """Create a PlatformIO project directory"""
    proj_dir = os.path.join(base_path, name)
    src_dir = os.path.join(proj_dir, "src")
    os.makedirs(src_dir, exist_ok=True)
    
    # platformio.ini
    with open(os.path.join(proj_dir, "platformio.ini"), 'w') as f:
        f.write(PLATFORMIO_ESP32 if platform == "ESP32" else PLATFORMIO_STM32)
    
    # main.c
    with open(os.path.join(src_dir, "main.c"), 'w') as f:
        f.write(main_c)
    
    # STM32 extras
    if platform == "STM32":
        inc_dir = os.path.join(proj_dir, "include")
        os.makedirs(inc_dir, exist_ok=True)
        with open(os.path.join(proj_dir, "extra_script_F103.py"), 'w') as f:
            f.write(EXTRA_SCRIPT)
        if freertos_cfg:
            with open(os.path.join(inc_dir, "FreeRTOSConfig.h"), 'w') as f:
                f.write(freertos_cfg)
    
    return proj_dir

def create_python_script(proj_dir, name, desc, analysis_type):
    """Create Python debug/analysis script"""
    script = python_debug_script(name, desc, analysis_type)
    py_file = os.path.join(proj_dir, f"debug_{name.lower()}.py")
    with open(py_file, 'w') as f:
        f.write(script)


# ============================================================================
# Execute generation
# ============================================================================

if __name__ == "__main__":
    print("=" * 70)
    print("  Generating Modul 10 ESP32 programs...")
    print("=" * 70)
    
    m10_esp32_base = os.path.join(BASE, "Modul-10-FreeRTOS-Queue-Semaphore/praktikum/ESP32")
    
    for name, data in M10_ESP32.items():
        proj = create_project(m10_esp32_base, name, "ESP32", data["main"])
        create_python_script(proj, name, f"Modul 10 - {name}", data["py_type"])
        print(f"  ✅ {name}")
    
    print(f"\n  Generated {len(M10_ESP32)} ESP32 programs for Modul 10")
    print("  Remaining programs will be generated by subsequent scripts.")
