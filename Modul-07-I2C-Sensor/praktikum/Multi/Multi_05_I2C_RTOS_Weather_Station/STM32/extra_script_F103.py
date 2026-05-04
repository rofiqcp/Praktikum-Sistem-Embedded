"""
Custom PlatformIO build script for STM32CubeF1 + FreeRTOS
Creates a static library from FreeRTOS sources and links it.
"""
Import("env")
import os

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

freertos_base = os.path.join(
    FRAMEWORK_DIR, "Middlewares", "Third_Party", "FreeRTOS", "Source"
)
add_include_path(freertos_base, [
    "include",
    "portable/GCC/ARM_CM3",
])

env.Append(CPPPATH=framework_includes)

# Build FreeRTOS as a static library
freertos_sources = [
    os.path.join(freertos_base, "tasks.c"),
    os.path.join(freertos_base, "queue.c"),
    os.path.join(freertos_base, "list.c"),
    os.path.join(freertos_base, "timers.c"),
    os.path.join(freertos_base, "portable/GCC/ARM_CM3/port.c"),
    os.path.join(freertos_base, "portable/MemMang/heap_4.c"),
]

# Create library from FreeRTOS sources
freertos_lib = env.Library(
    target=os.path.join("$BUILD_DIR", "libfreertos"),
    source=freertos_sources
)

# Add library to link stage
env.Append(LIBS=[freertos_lib])
