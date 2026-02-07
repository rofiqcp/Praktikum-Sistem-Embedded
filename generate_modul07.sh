#!/bin/bash
# ============================================================================
# MODUL 07 - SPI & Storage - ESP32 + STM32 Program Generator
# ============================================================================
BASE="/root/otomasi/Praktikum-Sistem-Embedded/Modul-07-SPI-Storage/praktikum"

# Create directory structure for all 12 ESP32 + 12 STM32 programs
ESP32_PROGRAMS=(
  "ESP32_01_SPI_Loopback"
  "ESP32_02_SPI_OLED_SSD1306"
  "ESP32_03_SPI_Flash_W25Q32"
  "ESP32_04_SPI_SD_Card"
  "ESP32_05_SPI_MCP3208_ADC"
  "ESP32_06_SPI_DAC_MCP4921"
  "ESP32_07_SPI_Multi_Slave"
  "ESP32_08_NVS_Key_Value"
  "ESP32_09_SPIFFS_File_System"
  "ESP32_10_SPI_Speed_Benchmark"
  "ESP32_11_SPI_Interrupt_Mode"
  "ESP32_12_Storage_Data_Logger"
)

STM32_PROGRAMS=(
  "STM32_01_SPI_Loopback"
  "STM32_02_SPI_OLED_SSD1306"
  "STM32_03_SPI_Flash_W25Q32"
  "STM32_04_SPI_SD_Card"
  "STM32_05_SPI_MCP3208_ADC"
  "STM32_06_SPI_DAC_MCP4921"
  "STM32_07_SPI_Multi_Slave"
  "STM32_08_Flash_Read_Write"
  "STM32_09_Flash_Key_Value"
  "STM32_10_SPI_Speed_Benchmark"
  "STM32_11_SPI_Interrupt_Mode"
  "STM32_12_Storage_Data_Logger"
)

for prog in "${ESP32_PROGRAMS[@]}"; do
  mkdir -p "$BASE/ESP32/$prog/src"
  mkdir -p "$BASE/ESP32/$prog/include"
done

for prog in "${STM32_PROGRAMS[@]}"; do
  mkdir -p "$BASE/STM32/$prog/src"
  mkdir -p "$BASE/STM32/$prog/include"
done

echo "Directory structure created for Modul 07"
