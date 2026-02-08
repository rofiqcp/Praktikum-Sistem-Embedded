#!/usr/bin/env python3
"""
Comprehensive fix script for ALL project structure issues:
1. Add missing CMakeLists.txt for ESP32 projects (root + src/)
2. Add missing include/ directories and config.h files
3. Ensure consistent structure across all 342 projects
"""
import os
import glob
import re

BASE = "/root/otomasi/Praktikum-Sistem-Embedded"

# ============================================================================
# TEMPLATES
# ============================================================================

def cmake_root(project_name):
    return f"""cmake_minimum_required(VERSION 3.16)
include($ENV{{IDF_PATH}}/tools/cmake/project.cmake)
project({project_name})
"""

CMAKE_SRC = """idf_component_register(
    SRC_DIRS "."
    INCLUDE_DIRS "."
)
"""

# ============================================================================
# STM32 config.h templates  
# ============================================================================

def stm32_config_h_basic(module_name, project_name, pins_block=""):
    """Generate config.h for non-FreeRTOS STM32 projects."""
    return f"""#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================================
 * {project_name}
 * {module_name}
 * Platform: STM32 (Blue Pill F103 / Black Pill F401/F411)
 * ============================================================================ */

/* --- Board Detection & HAL Include --- */
#if defined(STM32F103xB)
    #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
    #include "stm32f4xx_hal.h"
#else
    #error "Unsupported STM32 target. Define STM32F103xB, STM32F401xC, or STM32F411xE"
#endif

/* --- LED Pin (On-board) --- */
#if defined(STM32F103xB)
    #define LED_PORT        GPIOC
    #define LED_PIN         GPIO_PIN_13
    #define LED_ACTIVE_LOW  1
    #define LED_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()
#else
    #define LED_PORT        GPIOC
    #define LED_PIN         GPIO_PIN_13
    #define LED_ACTIVE_LOW  1
    #define LED_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()
#endif

{pins_block}
/* --- Timing --- */
#define BLINK_DELAY_MS  500

/* --- UART for printf --- */
#define PRINTF_UART     USART1

/* --- Function Prototypes --- */
void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */
"""

def stm32_config_h_freertos(module_name, project_name, pins_block=""):
    """Generate config.h for FreeRTOS STM32 projects."""
    return f"""#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================================
 * {project_name}
 * {module_name}
 * Platform: STM32 (Blue Pill F103 / Black Pill F401/F411) + FreeRTOS
 * ============================================================================ */

/* --- Board Detection & HAL Include --- */
#if defined(STM32F103xB)
    #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
    #include "stm32f4xx_hal.h"
#else
    #error "Unsupported STM32 target"
#endif

/* --- LED Pins --- */
#if defined(STM32F103xB)
    #define LED_PORT        GPIOC
    #define LED_PIN         GPIO_PIN_13
    #define LED_ACTIVE_LOW  1
    #define LED_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()
#else
    #define LED_PORT        GPIOC
    #define LED_PIN         GPIO_PIN_13
    #define LED_ACTIVE_LOW  1
    #define LED_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()
#endif

{pins_block}
/* --- Task Parameters --- */
#define TASK_DEFAULT_STACK  256
#define TASK_DEFAULT_PRIO   2

/* --- Timing --- */
#define BLINK_DELAY_MS  500

/* --- UART for printf --- */
#define PRINTF_UART     USART1

/* --- Function Prototypes --- */
void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */
"""

# FreeRTOSConfig.h template for STM32
FREERTOS_CONFIG_H = """#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ============================================================================
 * FreeRTOS Kernel Configuration for STM32
 * Compatible with: STM32F103 (Blue Pill) / STM32F401 / STM32F411 (Black Pill)
 * ============================================================================ */

/* --- Scheduler --- */
#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0
#define configUSE_TICKLESS_IDLE                  0

/* --- Clock --- */
#define configCPU_CLOCK_HZ                      (SystemCoreClock)
#define configTICK_RATE_HZ                      ((TickType_t)1000)

/* --- Memory --- */
#define configMINIMAL_STACK_SIZE                ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                   ((size_t)(10 * 1024))
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0

/* --- Priority --- */
#define configMAX_PRIORITIES                    7
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3

/* --- Hooks --- */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_MALLOC_FAILED_HOOK            1
#define configCHECK_FOR_STACK_OVERFLOW          2

/* --- Queue & Semaphore --- */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                     1

/* --- Software Timer --- */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               2
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            (configMINIMAL_STACK_SIZE * 2)

/* --- Memory allocation --- */
#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* --- Debug & Trace --- */
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1
#define configGENERATE_RUN_TIME_STATS           0

/* --- Co-routines (disabled) --- */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES          2

/* --- API Includes --- */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           0
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1

/* --- NVIC / Interrupt --- */
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS 4
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY     5
#define configKERNEL_INTERRUPT_PRIORITY        (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY   (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* --- Handler Mapping --- */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler

/* --- Assert --- */
#define configASSERT(x) if((x)==0) { taskDISABLE_INTERRUPTS(); for(;;); }

extern uint32_t SystemCoreClock;

#endif /* FREERTOS_CONFIG_H */
"""

FREERTOS_MODULES = {9, 10, 11, 12, 13, 14}

stats = {
    "cmake_root_created": 0,
    "cmake_src_created": 0,
    "include_dir_created": 0,
    "config_h_created": 0,
    "freertosconfig_created": 0,
}

def get_module_number(path):
    """Extract module number from path."""
    match = re.search(r'Modul-(\d+)', path)
    return int(match.group(1)) if match else 0

def get_project_name(path):
    """Extract project directory name."""
    parts = path.rstrip('/').split('/')
    for p in reversed(parts):
        if p.startswith("ESP32_") or p.startswith("STM32_"):
            return p
    return "project"

def get_module_name(path):
    """Extract module directory name."""
    for part in path.split('/'):
        if part.startswith("Modul-"):
            return part
    return "Unknown"

def fix_esp32_project(project_dir):
    """Fix missing CMakeLists.txt files for ESP32 projects."""
    project_name = get_project_name(project_dir)
    
    # Root CMakeLists.txt
    cmake_root_path = os.path.join(project_dir, "CMakeLists.txt")
    if not os.path.exists(cmake_root_path):
        with open(cmake_root_path, 'w') as f:
            f.write(cmake_root(project_name))
        stats["cmake_root_created"] += 1
    
    # src/CMakeLists.txt
    src_dir = os.path.join(project_dir, "src")
    cmake_src_path = os.path.join(src_dir, "CMakeLists.txt")
    if os.path.isdir(src_dir) and not os.path.exists(cmake_src_path):
        with open(cmake_src_path, 'w') as f:
            f.write(CMAKE_SRC)
        stats["cmake_src_created"] += 1

def fix_stm32_project(project_dir):
    """Fix missing include/ directory and headers for STM32 projects."""
    mod_num = get_module_number(project_dir)
    project_name = get_project_name(project_dir)
    module_name = get_module_name(project_dir)
    needs_freertos = mod_num in FREERTOS_MODULES
    
    include_dir = os.path.join(project_dir, "include")
    if not os.path.isdir(include_dir):
        os.makedirs(include_dir, exist_ok=True)
        stats["include_dir_created"] += 1
    
    # config.h
    config_path = os.path.join(include_dir, "config.h")
    if not os.path.exists(config_path):
        if needs_freertos:
            content = stm32_config_h_freertos(module_name, project_name)
        else:
            content = stm32_config_h_basic(module_name, project_name)
        with open(config_path, 'w') as f:
            f.write(content)
        stats["config_h_created"] += 1
    
    # FreeRTOSConfig.h
    if needs_freertos:
        frc_path = os.path.join(include_dir, "FreeRTOSConfig.h")
        if not os.path.exists(frc_path):
            with open(frc_path, 'w') as f:
                f.write(FREERTOS_CONFIG_H)
            stats["freertosconfig_created"] += 1


def main():
    print("=" * 60)
    print("🔧 Fixing ALL project structure issues")
    print("=" * 60)
    
    # Fix ESP32 projects
    esp32_dirs = sorted(glob.glob(os.path.join(BASE, "*/praktikum/ESP32/*/")))
    for d in esp32_dirs:
        if os.path.isdir(d):
            fix_esp32_project(d)
    
    # Fix STM32 projects
    stm32_dirs = sorted(glob.glob(os.path.join(BASE, "*/praktikum/STM32/*/")))
    for d in stm32_dirs:
        if os.path.isdir(d):
            fix_stm32_project(d)
    
    print(f"\n📊 Results:")
    print(f"   CMakeLists.txt (root) created: {stats['cmake_root_created']}")
    print(f"   CMakeLists.txt (src/) created: {stats['cmake_src_created']}")
    print(f"   include/ dirs created:         {stats['include_dir_created']}")
    print(f"   config.h created:              {stats['config_h_created']}")
    print(f"   FreeRTOSConfig.h created:      {stats['freertosconfig_created']}")
    print("\n✅ Done!")

if __name__ == "__main__":
    main()
