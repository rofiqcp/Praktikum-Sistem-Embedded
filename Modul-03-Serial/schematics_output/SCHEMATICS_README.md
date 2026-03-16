# Modul-03: Serial UART - Schematic Collection

**Generated:** All 13 circuit diagrams using Schemdraw library (SVG format)  
**Location:** `schematics_output/` directory  
**Script:** `generate_schematics_v2.py` (Simplified Schemdraw approach)

## Schematic Files

### System Overview
- **P00_System_Architecture.svg** - Complete UART System Architecture
  - Sensor Nodes (STM32 #1, #2)
  - Gateway (ESP32 Multi-UART Hub)
  - PC Monitor Terminal

### Experiment Schematics (P01-P12)

**Phase 1: Fundamentals**
1. **P01_LED_Switch.svg** - UART Echo - Polling
   - Baud: 115200 (8N1)
   - GPIO1(TX) ↔ GPIO3(RX) ↔ USB

2. **P02_UART_Interrupt_RX.svg** - Interrupt-Based RX
   - FreeRTOS Event Queue
   - ISR Handler Processing
   - Non-blocking communication

3. **P03_Ring_Buffer.svg** - Ring Buffer (Circular Buffer)
   - ISR ↔ Main Task Communication
   - Fixed-size array structure
   - Push/Pop operations

4. **P04_Printf_Redirect.svg** - Printf to Multiple UART
   - STM32 USART1 (Operator)
   - STM32 USART2 (Debug PC)
   - Concurrent dual output

**Phase 2: Protocol & Parsing**
5. **P05_Command_Parser.svg** - Command Parser
   - Device control via serial
   - Example: "BUZZER ON"
   - Response: OK/ERROR

6. **P06_JSON_Protocol.svg** - JSON Data Serialization
   - cJSON library
   - Sensor data encoding
   - Gateway parsing

7. **P07_Line_Editor.svg** - Interactive Line Editor
   - Key handling (printable, backspace, enter, arrows)
   - Command history buffer (last 10)

8. **P08_Framing_STX_ETX.svg** - Packet Framing
   - Frame: [STX|NodeID|Type|Len|Data|CRC|ETX]
   - Byte stuffing for special characters
   - TX stuffing ↔ RX de-stuffing

**Phase 3: Reliability**
9. **P09_CRC8_Checksum.svg** - Error Detection
   - CRC-8 polynomial (0x07)
   - TX: Calculate → Send
   - RX: Recalculate → Compare

10. **P10_Timeout_Parser.svg** - Timeout-Based Parser
    - Unframed protocol
    - 50ms silence detection
    - No STX/ETX needed

**Phase 4: Integration**
11. **P11_Multi_UART_Bridge.svg** - Multi-UART Gateway
    - 3 simultaneous UART inputs
    - STM32 Node #1 (Temperature)
    - STM32 Node #2 (Humidity)
    - JSON aggregation → PC

12. **P12_Error_Statistics.svg** - Network Health Monitoring
    - Error counter (Parity, Frame, Overrun, CRC)
    - Health % = (Total RX - Errors) / Total RX
    - Alert thresholds

## Usage

View schematics in any SVG viewer or web browser:
```bash
# Open SVG files
firefox schematics_output/P01_LED_Switch.svg
# or
start schematics_output/P01_LED_Switch.svg  # Windows
```

## Features

✅ **13 Complete Schematics** - All experiments documented  
✅ **SVG Format** - Vector graphics, scalable, publication-ready  
✅ **Clear Labels** - Easy-to-read text descriptions  
✅ **Connection Flow** - Data flow and signal paths  
✅ **Component Details** - Baud rates, pin assignments, protocols  

## Hardware Reference

**GPIO Pin Assignments (from schematics):**
- ESP32: GPIO1(TX), GPIO3(RX)
- STM32: USART1(PA9/PA10), USART2(PA2/PA3)

**Communication Specs:**
- Baud Rate: 115200
- Data Bits: 8
- Parity: None
- Stop Bits: 1
- Format: 8N1

## Notes

- All schematics generated using Schemdraw v0.22
- Special characters replaced with ASCII equivalents for XML compatibility
- Each schematic includes protocol descriptions and signal flow
- Suitable for educational documentation and presentation materials
