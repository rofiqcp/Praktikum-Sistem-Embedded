#!/bin/bash
# Generate all 12 STM32 programs for Modul 12: FreeRTOS Memory & Advanced
BASE="/root/otomasi/Praktikum-Sistem-Embedded/Modul-12-FreeRTOS-Memory-Advanced/praktikum/STM32"

PROGRAMS=(
  "STM32_01_Heap_Monitor"
  "STM32_02_Memory_Allocation"
  "STM32_03_Stack_Overflow_Detect"
  "STM32_04_Static_Allocation"
  "STM32_05_Memory_Pool"
  "STM32_06_Stream_Buffer"
  "STM32_07_Message_Buffer"
  "STM32_08_Critical_Section"
  "STM32_09_Heap_Fragmentation"
  "STM32_10_PSRAM_External_RAM"
  "STM32_11_Memory_Leak_Detection"
  "STM32_12_System_Dashboard"
)

for prog in "${PROGRAMS[@]}"; do
  DIR="$BASE/$prog"
  mkdir -p "$DIR/src" "$DIR/include"
  echo "Created: $DIR"
done

echo "All directories created."
