"""
Custom PlatformIO build script for STM32CubeF1 Framework
========================================================
This script automatically includes ALL headers and compiles necessary sources from:
- HAL Drivers
- FreeRTOS (RTOS kernel)
- FatFs (File system)
- LwIP (TCP/IP stack)
- USB Device Library
- USB Host Library
- Utilities

Author: Auto-generated for STM32F103 Projects
"""

Import("env")
import os
import glob

# ============================================================================
# FRAMEWORK CONFIGURATION
# ============================================================================

platform = env.PioPlatform()
FRAMEWORK_DIR = platform.get_package_dir("framework-stm32cubef1")

# ============================================================================
# SECTION 1: COLLECT ALL INCLUDE PATHS
# ============================================================================

framework_includes = []

def add_include_path(base_path, relative_paths):
    """Helper function to add include paths if they exist"""
    for rel_path in relative_paths:
        full_path = os.path.join(base_path, rel_path)
        if os.path.exists(full_path):
            framework_includes.append(full_path)

# 1. HAL Driver includes
add_include_path(FRAMEWORK_DIR, [
    "Drivers/STM32F1xx_HAL_Driver/Inc",
    "Drivers/CMSIS/Device/ST/STM32F1xx/Include",
    "Drivers/CMSIS/Include",
])

# 2. FreeRTOS includes
freertos_base = os.path.join(FRAMEWORK_DIR, "Middlewares", "Third_Party", "FreeRTOS", "Source")
add_include_path(freertos_base, [
    "include",
    "portable/GCC/ARM_CM3",
])

# 3. FatFs (File System) includes
fatfs_base = os.path.join(FRAMEWORK_DIR, "Middlewares", "Third_Party", "FatFs")
add_include_path(fatfs_base, [
    "src",
])

# 4. LwIP (TCP/IP Stack) includes
lwip_base = os.path.join(FRAMEWORK_DIR, "Middlewares", "Third_Party", "LwIP", "src")
add_include_path(lwip_base, [
    "include",
    "include/lwip",
    "include/lwip/apps",
    "include/lwip/priv",
    "include/lwip/prot",
    "include/netif",
    "include/posix",
    "include/posix/sys",
])

# 5. USB Device Library includes
usb_device_base = os.path.join(FRAMEWORK_DIR, "Middlewares", "ST", "STM32_USB_Device_Library")
add_include_path(usb_device_base, [
    "Core/Inc",
    "Class/CDC/Inc",
    "Class/HID/Inc",
    "Class/MSC/Inc",
    "Class/AUDIO/Inc",
    "Class/DFU/Inc",
    "Class/CustomHID/Inc",
    "Class/Template/Inc",
])

# 6. USB Host Library includes
usb_host_base = os.path.join(FRAMEWORK_DIR, "Middlewares", "ST", "STM32_USB_Host_Library")
add_include_path(usb_host_base, [
    "Core/Inc",
    "Class/CDC/Inc",
    "Class/HID/Inc",
    "Class/MSC/Inc",
    "Class/AUDIO/Inc",
    "Class/MTP/Inc",
    "Class/Template/Inc",
])

# 7. Scan for any additional Inc/include directories in Utilities
utilities_base = os.path.join(FRAMEWORK_DIR, "Utilities")
if os.path.exists(utilities_base):
    for root, dirs, files in os.walk(utilities_base):
        if root.endswith("Inc") or root.endswith("include"):
            framework_includes.append(root)

# Add all include paths to build environment
env.Append(CPPPATH=framework_includes)

# ============================================================================
# SECTION 2: COMPILE FREERTOS SOURCE FILES
# ============================================================================

freertos_src_base = os.path.join(FRAMEWORK_DIR, "Middlewares", "Third_Party", "FreeRTOS", "Source")

# FreeRTOS core sources (required for RTOS operation)
freertos_sources = [
    "tasks.c",
    "queue.c",
    "list.c",
    "timers.c",
    "portable/GCC/ARM_CM3/port.c",
    "portable/MemMang/heap_4.c",
]

# Compile FreeRTOS sources
freertos_objs = []
for src in freertos_sources:
    src_path = os.path.join(freertos_src_base, src)
    if os.path.exists(src_path):
        obj_name = src.replace("/", "_").replace(".c", ".o")
        obj_path = os.path.join("$BUILD_DIR", "FreeRTOS_obj", obj_name)
        obj = env.Object(obj_path, src_path)
        freertos_objs.append(obj)

env.Append(PIOBUILDFILES=freertos_objs)

# ============================================================================
# SECTION 3: BUILD SUMMARY
# ============================================================================

print("=" * 80)
print(" " * 20 + "STM32CubeF1 Framework Configuration")
print("=" * 80)
print("\n INCLUDE PATHS ({} total):".format(len(framework_includes)))
print("-" * 80)
for i, path in enumerate(framework_includes, 1):
    rel_path = os.path.relpath(path, FRAMEWORK_DIR)
    print(f"  {i:2d}. {rel_path}")

print("\n" + "=" * 80)
print("  COMPILED SOURCES:")
print("-" * 80)
print(f"  FreeRTOS: {len(freertos_objs)} files")
for src in freertos_sources:
    if os.path.exists(os.path.join(freertos_src_base, src)):
        print(f"    - {src}")

print("\n" + "=" * 80)
print(f"Total: {len(framework_includes)} include paths configured")
print(f"FreeRTOS: {len(freertos_objs)} object files compiled")
print("=" * 80 + "\n")
