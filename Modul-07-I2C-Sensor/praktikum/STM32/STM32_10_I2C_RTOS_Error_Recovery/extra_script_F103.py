"""
Custom PlatformIO build script for STM32CubeF1 + FreeRTOS
Adds FreeRTOS include paths and creates symlinks to FreeRTOS sources in src directory.
"""
Import("env")
import os

platform = env.PioPlatform()
FRAMEWORK_DIR = platform.get_package_dir("framework-stm32cubef1")
FREERTOS_BASE = os.path.join(FRAMEWORK_DIR, "Middlewares", "Third_Party", "FreeRTOS", "Source")

# Add include paths
env.Append(CPPPATH=[
    os.path.join(FRAMEWORK_DIR, "Drivers", "STM32F1xx_HAL_Driver", "Inc"),
    os.path.join(FRAMEWORK_DIR, "Drivers", "CMSIS", "Device", "ST", "STM32F1xx", "Include"),
    os.path.join(FRAMEWORK_DIR, "Drivers", "CMSIS", "Include"),
    os.path.join(FREERTOS_BASE, "include"),
    os.path.join(FREERTOS_BASE, "portable", "GCC", "ARM_CM3"),
])

# Create symlinks to FreeRTOS sources in src directory
src_dir = os.path.join(env.subst("$PROJECT_DIR"), "src")
freertos_sources = [
    ("tasks.c", os.path.join(FREERTOS_BASE, "tasks.c")),
    ("queue.c", os.path.join(FREERTOS_BASE, "queue.c")),
    ("list.c", os.path.join(FREERTOS_BASE, "list.c")),
    ("timers.c", os.path.join(FREERTOS_BASE, "timers.c")),
    ("event_groups.c", os.path.join(FREERTOS_BASE, "event_groups.c")),
    ("stream_buffer.c", os.path.join(FREERTOS_BASE, "stream_buffer.c")),
    ("port.c", os.path.join(FREERTOS_BASE, "portable", "GCC", "ARM_CM3", "port.c")),
    ("heap_4.c", os.path.join(FREERTOS_BASE, "portable", "MemMang", "heap_4.c")),
]

for name, src_path in freertos_sources:
    if os.path.exists(src_path):
        dst_path = os.path.join(src_dir, name)
        if os.path.exists(dst_path) or os.path.islink(dst_path):
            os.remove(dst_path)
        os.symlink(src_path, dst_path)
        print("Created symlink: {} -> {}".format(dst_path, src_path))
