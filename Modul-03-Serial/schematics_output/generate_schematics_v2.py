#!/usr/bin/env python3
"""
Modul 03: Serial UART - Schematic Generator using Schemdraw (Simplified)
Generate professional circuit schematics untuk 12 percobaan
"""

import schemdraw
import schemdraw.elements as elm
import os
from pathlib import Path

# Create output directory
OUTPUT_DIR = Path('schematics_output')
OUTPUT_DIR.mkdir(exist_ok=True)

def create_schematic(title: str, content_lines: list) -> None:
    """Create a simple schematic with text labels"""
    d = schemdraw.Drawing()
    d.config(fontsize=9, font='sans-serif', unit=0.8)
    
    y_pos = 9
    for line in content_lines:
        d += elm.Label().at((0.5, y_pos)).label(line)
        y_pos -= 0.7
    
    return d

def create_all_schematics():
    """Generate all 13 schematics"""
    
    schematics = {
        'P00_System_Architecture.svg': [
            'Modul 03: Complete Serial UART System Architecture',
            '',
            'Sensor Nodes (Left):',
            '  STM32 Node #1 (DHT22 Temp/Humidity)',
            '  STM32 Node #2 (MQ-135 Air Quality)',
            '',
            'Gateway (Center):',
            '  ESP32 DevKit V1 with 3 UART ports',
            '  UART0: PC Terminal (115200)',
            '  UART1: Node #1 Connection',
            '  UART2: Node #2 Connection',
            '',
            'PC Monitor (Right):',
            '  Serial Monitor 115200 baud',
            '',
            'Data Flow: Sensor to STX/ETX Frame to Gateway to JSON to Monitor'
        ],
        'P01_LED_Switch.svg': [
            'P01: UART Echo - Polling',
            'Baud Rate: 115200 (8N1)',
            '',
            'ESP32 Serial Connection:',
            '  GPIO1 (TX) to USB TX',
            '  GPIO3 (RX) from USB RX',
            '  GND to USB GND',
            '',
            'Operation:',
            '  Read UART RX byte to Echo to TX',
            '  Polling: while(1) uart_read_bytes()'
        ],
        'P02_UART_Interrupt_RX.svg': [
            'P02: UART Interrupt RX - Event Queue',
            'Non-blocking Communication',
            '',
            'Interrupt Handler:',
            '  UART RX ISR triggered on data',
            '  then xQueueSendFromISR()',
            '',
            'Main Task:',
            '  xQueueReceive() blocks for data',
            '  uart_write_bytes() echoes response',
            '',
            'Events: UART_DATA, FIFO_OVF, FRAME_ERR, PARITY_ERR'
        ],
        'P03_Ring_Buffer.svg': [
            'P03: Ring Buffer (Circular Buffer)',
            'ISR and Main Task Communication',
            '',
            'Ring Buffer Structure:',
            '  data[N]: Fixed-size array',
            '  head (write pointer, ISR)',
            '  tail (read pointer, task)',
            '  count: Bytes stored',
            '',
            'Operations:',
            '  rb_push(byte) - ISR adds byte',
            '  rb_pop() - Task retrieves byte',
            '  Overflow detection and handling'
        ],
        'P04_Printf_Redirect.svg': [
            'P04: Printf Redirect - Multiple UART',
            '',
            'STM32F103C8T6 (Blue Pill):',
            '  USART1: PA9(TX) PA10(RX)',
            '  USART2: PA2(TX) PA3(RX)',
            '',
            'Usage:',
            '  printf() to USART1 (Operator)',
            '  debug_log() to USART2 (Debug PC)',
            '',
            'Each UART: Separate USB-TTL converter'
        ],
        'P05_Command_Parser.svg': [
            'P05: Command Parser - Device Control',
            '',
            'Input: "BUZZER ON\\n"',
            '  ↓ strtok() parse',
            '',
            'Decision Tree:',
            '  "BUZZER" to GPIO control',
            '  "LED" to LED toggle',
            '  "SAMPLE" to Sensor read',
            '',
            'Response: "OK" or "ERROR"'
        ],
        'P06_JSON_Protocol.svg': [
            'P06: JSON Protocol (cJSON)',
            'Sensor Data Serialization',
            '',
            'Sensor Node:',
            '  Read: Temperature, Humidity, Air Quality',
            '  Encode: cJSON_CreateObject()',
            '',
            'JSON Frame:',
            '  {"node":1, "temp":25.3, "humidity":60}',
            '',
            'Gateway: Decode with cJSON_Parse()'
        ],
        'P07_Line_Editor.svg': [
            'P07: Interactive Line Editor',
            'Command Input with History',
            '',
            'Key Handling:',
            '  Printable: Insert into buffer',
            '  Backspace: Delete with echo',
            '  Enter: Process line',
            '  ↑↓ Arrows: Recall history',
            '',
            'History: Store last 10 commands'
        ],
        'P08_Framing_STX_ETX.svg': [
            'P08: Packet Framing STX/ETX + Byte Stuffing',
            '',
            'Frame Format:',
            '  [STX:0x02] [NodeID] [Type] [Len] [Data] [CRC] [ETX:0x03]',
            '',
            'Byte Stuffing Rule:',
            '  If data contains STX/ETX/ESC:',
            '    then Send ESC (0x1B) plus data byte',
            '',
            'TX: Stuff data before send',
            'RX: De-stuff data after receive'
        ],
        'P09_CRC8_Checksum.svg': [
            'P09: CRC-8 Checksum Error Detection',
            '',
            'TX (Sender):',
            '  1. Payload data (NodeID, Temp, etc)',
            '  2. Calculate CRC-8 (poly=0x07)',
            '  3. Send: [Frame] + CRC + [ETX]',
            '',
            'RX (Receiver):',
            '  1. Receive frame + CRC',
            '  2. Recalculate CRC-8 on payload',
            '  3. Compare: Match? Accept : Reject'
        ],
        'P10_Timeout_Parser.svg': [
            'P10: Timeout-Based Parser (Unframed)',
            'Packet Delimiter by Silence',
            '',
            'Operation:',
            '  1. Collect bytes in buffer',
            '  2. Start timer on first byte',
            '  3. Restart timer each new byte',
            '  4. Timeout 50ms then Packet complete',
            '',
            'Advantage: No STX/ETX needed',
            'Example: "SENSOR_DATA 25.3 60"'
        ],
        'P11_Multi_UART_Bridge.svg': [
            'P11: Multi-UART Bridge Gateway',
            'Sensor Data Aggregation',
            '',
            'Input (STM32 Nodes):',
            '  UART1: Node #1 (Temperature)',
            '  UART2: Node #2 (Humidity)',
            '',
            'Processing:',
            '  Parse STX/ETX frames',
            '  Validate CRC-8 checks',
            '  Aggregate sensor data',
            '',
            'Output: JSON to PC Terminal'
        ],
        'P12_Error_Statistics.svg': [
            'P12: Error Statistics and Network Health',
            'Reliability Monitoring',
            '',
            'Track Error Types:',
            '  - Parity Errors',
            '  - Frame Errors',
            '  - UART Overrun',
            '  - CRC Mismatches',
            '',
            'Health % = (Total RX - Errors) / Total RX',
            'Alert: CRC greater than 10%, Overrun greater than 5%'
        ]
    }
    
    print("\n" + "="*70)
    print("Schemdraw UART Schematic Generator - Modul 03 (Simplified)")
    print("="*70 + "\n")
    
    for filename, content in schematics.items():
        try:
            print(f"Generating {filename}...")
            d = create_schematic(filename, content)
            output_path = OUTPUT_DIR / filename
            d.save(str(output_path))
            print(f"  ✅ {filename}")
        except Exception as e:
            print(f"  ❌ Error: {e}")
    
    print("\n" + "="*70)
    print(f"✅ Complete! Generated {len(schematics)} schematics in '{OUTPUT_DIR}/'")
    print("="*70 + "\n")

if __name__ == '__main__':
    create_all_schematics()
