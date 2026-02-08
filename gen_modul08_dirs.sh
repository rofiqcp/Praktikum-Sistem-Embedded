BASE="Modul-08-DMA/praktikum"
ESP32_PROGS=(
  "ESP32_01_DMA_Memory_to_Memory"
  "ESP32_02_DMA_UART_TX"
  "ESP32_03_DMA_UART_RX"
  "ESP32_04_DMA_ADC_Continuous"
  "ESP32_05_DMA_ADC_Multi_Channel"
  "ESP32_06_DMA_SPI_Transfer"
  "ESP32_07_DMA_Circular_Buffer"
  "ESP32_08_DMA_Double_Buffer"
  "ESP32_09_DMA_I2C_Transfer"
  "ESP32_10_DMA_DAC_Waveform"
  "ESP32_11_DMA_Benchmark"
  "ESP32_12_DMA_Linked_List"
)
STM32_PROGS=(
  "STM32_01_DMA_Memory_to_Memory"
  "STM32_02_DMA_UART_TX"
  "STM32_03_DMA_UART_RX"
  "STM32_04_DMA_ADC_Continuous"
  "STM32_05_DMA_ADC_Multi_Channel"
  "STM32_06_DMA_SPI_Transfer"
  "STM32_07_DMA_Circular_Buffer"
  "STM32_08_DMA_Double_Buffer"
  "STM32_09_DMA_I2C_Transfer"
  "STM32_10_DMA_DAC_Waveform"
  "STM32_11_DMA_Benchmark"
  "STM32_12_DMA_Linked_List"
)
for p in "${ESP32_PROGS[@]}"; do mkdir -p "$BASE/ESP32/$p/src" "$BASE/ESP32/$p/include"; done
for p in "${STM32_PROGS[@]}"; do mkdir -p "$BASE/STM32/$p/src" "$BASE/STM32/$p/include"; done
echo "Modul 08 directories created"
