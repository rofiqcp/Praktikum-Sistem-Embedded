"""
Custom PlatformIO build script for STM32CubeF4 + FreeRTOS
Creates a static library from FreeRTOS sources and links it.
"""
Import("env")
import os

board = env.BoardConfig()
mcu = board.get("build.mcu", "stm32f411ceu6")

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

    framework_includes = []

    def add_include_path(base_path, relative_paths):
        for rel_path in relative_paths:
            full_path = os.path.join(base_path, rel_path)
            if os.path.exists(full_path):
                framework_includes.append(full_path)

    add_include_path(FRAMEWORK_DIR, [
        "Drivers/STM32F4xx_HAL_Driver/Inc",
        "Drivers/CMSIS/Device/ST/STM32F4xx/Include",
        "Drivers/CMSIS/Include",
    ])

    freertos_base = os.path.join(
        FRAMEWORK_DIR, "Middlewares", "Third_Party", "FreeRTOS", "Source"
    )
    add_include_path(freertos_base, [
        "include",
        "portable/GCC/ARM_CM4F",
    ])

    env.Append(CPPPATH=framework_includes)

    # Build FreeRTOS as a static library
    freertos_sources = [
        os.path.join(freertos_base, "tasks.c"),
        os.path.join(freertos_base, "queue.c"),
        os.path.join(freertos_base, "list.c"),
        os.path.join(freertos_base, "timers.c"),
        os.path.join(freertos_base, "portable/GCC/ARM_CM4F/port.c"),
        os.path.join(freertos_base, "portable/MemMang/heap_4.c"),
    ]

    # Create library from FreeRTOS sources
    freertos_lib = env.Library(
        target=os.path.join("$BUILD_DIR", "libfreertos"),
        source=freertos_sources
    )

    # Add library to link stage
    env.Append(LIBS=[freertos_lib])