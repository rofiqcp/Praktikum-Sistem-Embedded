Import("env")

# Debug output
print("Extra script running for board: " + env["BOARD"])

# Get the current board
board = env["BOARD"]

# Add the correct FreeRTOS portable file based on the board
if "f103" in board.lower():
    # ARM_CM3 for STM32F1
    port_c = "/home/otomasi/.platformio/packages/framework-stm32cubef1/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c"
    port_include = "/home/otomasi/.platformio/packages/framework-stm32cubef1/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3"
else:
    # ARM_CM4F for STM32F4
    port_c = "/home/otomasi/.platformio/packages/framework-stm32cubef4/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c"
    port_include = "/home/otomasi/.platformio/packages/framework-stm32cubef4/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F"

# Add include path for portmacro.h
env.Prepend(CPPPATH=[port_include])

# Add project include path FIRST to ensure project FreeRTOSConfig.h is used
env.Prepend(CPPPATH=["include"])

# Add port.c to the build - use env.BuildSources()
# In SCons/PlatformIO, we can use env.Program() or modify SOURCES
env.Append(SOURCES=[port_c])
print("Added source: " + port_c)
