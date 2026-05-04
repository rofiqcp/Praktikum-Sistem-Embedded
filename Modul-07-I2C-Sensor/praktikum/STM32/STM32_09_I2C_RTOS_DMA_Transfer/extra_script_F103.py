"""
Custom PlatformIO build script for STM32CubeF1 + FreeRTOS
Adds FreeRTOS include paths and compiles FreeRTOS kernel sources.
"""
import os
Import("env")

FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-stm32cubef1")

if not FRAMEWORK_DIR:
    print("Warning: framework-stm32cubef1 not found, skipping FreeRTOS setup")
else:
    framework_includes = []

    def add_include_path(base_path, relative_paths):
        for rel_path in relative_paths:
            full_path = os.path.join(base_path, rel_path)
            if os.path.exists(full_path):
                framework_includes.append(full_path)
                print("Added include path: {}".format(full_path))

    add_include_path(FRAMEWORK_DIR, [
        "Drivers/STM32F1xx_HAL_Driver/Inc",
        "Drivers/CMSIS/Device/ST/STM32F1xx/Include",
        "Drivers/CMSIS/Include",
    ])

    freertos_base = os.path.join(
        FRAMEWORK_DIR, "Middlewares", "Third_Party", "FreeRTOS", "Source"
    )
    print("FreeRTOS base path: {}".format(freertos_base))
    print("FreeRTOS base exists: {}".format(os.path.exists(freertos_base)))

    add_include_path(freertos_base, [
        "include",
        "portable/GCC/ARM_CM3",
    ])

    env.Append(CPPPATH=framework_includes)

    freertos_sources = [
        "tasks.c", "queue.c", "list.c", "timers.c",
        "portable/GCC/ARM_CM3/port.c",
        "portable/MemMang/heap_4.c",
    ]

    # Build FreeRTOS sources
    freertos_objs = []
    for src in freertos_sources:
        src_path = os.path.join(freertos_base, src)
        if os.path.exists(src_path):
            print("Compiling FreeRTOS source: {}".format(src_path))
            obj_name = src.replace("/", "_").replace(".c", ".o")
            obj_path = os.path.join("$BUILD_DIR", "FreeRTOS_obj", obj_name)
            obj = env.Object(obj_path, src_path)
            freertos_objs.append(obj)
        else:
            print("Warning: FreeRTOS source not found: {}".format(src_path))

    # Create a static library from FreeRTOS objects
    if freertos_objs:
        freertos_lib = env.StaticLibrary(
            os.path.join("$BUILD_DIR", "FreeRTOS_obj", "libfreertos"),
            freertos_objs
        )
        env.Append(LIBS=[freertos_lib])
        print("FreeRTOS library created and added to link")
    else:
        print("Warning: No FreeRTOS objects to link")
