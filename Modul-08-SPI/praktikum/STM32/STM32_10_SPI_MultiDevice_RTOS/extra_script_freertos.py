Import("env")
# extra_script_freertos.py: FreeRTOS include paths only.
# Source compilation is handled by extra_script_F103.py / extra_script_F4xx.py.
board = env["BOARD"]
print("extra_script_freertos.py: board = " + board)
# No source files added here to avoid duplicates.
