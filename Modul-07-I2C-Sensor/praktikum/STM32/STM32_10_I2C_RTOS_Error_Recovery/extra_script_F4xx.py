"""
Custom PlatformIO build script for STM32CubeF4 + FreeRTOS
Adds FreeRTOS include paths, sets FPU flags, and creates symlinks to FreeRTOS sources.
"""
Import("env")
import os

FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-stm32cubef4")

if not FRAMEWORK_DIR:
    print("Warning: framework-stm32cubef4 not found, skipping FreeRTOS setup")
else:
    # FPU flags for Cortex-M4F
    fpu_flags = ["-mfpu=fpv4-sp-d16", "-mfloat-abi=hard"]
    env.Append(ASFLAGS=fpu_flags, CCFLAGS=fpu_flags, LINKFLAGS=fpu_flags)
    for flag_list_name in ("ASFLAGS", "CCFLAGS", "LINKFLAGS"):
        flag_list = env.get(flag_list_name, [])
        env.Replace(**{flag_list_name: [f for f in flag_list if f != "-mfloat-abi=soft"]})

    FREERTOS_BASE = os.path.join(FRAMEWORK_DIR, "Middlewares", "Third_Party", "FreeRTOS", "Source")

    # Add include paths
    env.Append(CPPPATH=[
        os.path.join(FRAMEWORK_DIR, "Drivers", "STM32F4xx_HAL_Driver", "Inc"),
        os.path.join(FRAMEWORK_DIR, "Drivers", "CMSIS", "Device", "ST", "STM32F4xx", "Include"),
        os.path.join(FRAMEWORK_DIR, "Drivers", "CMSIS", "Include"),
        os.path.join(FREERTOS_BASE, "include"),
        os.path.join(FREERTOS_BASE, "portable", "GCC", "ARM_CM4F"),
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
        ("port.c", os.path.join(FREERTOS_BASE, "portable", "GCC", "ARM_CM4F", "port.c")),
        ("heap_4.c", os.path.join(FREERTOS_BASE, "portable", "MemMang", "heap_4.c")),
    ]

    for name, src_path in freertos_sources:
        if os.path.exists(src_path):
            dst_path = os.path.join(src_dir, name)
            if os.path.exists(dst_path) or os.path.islink(dst_path):
                os.remove(dst_path)
            os.symlink(src_path, dst_path)
            print("Created symlink: {} -> {}".format(dst_path, src_path))
