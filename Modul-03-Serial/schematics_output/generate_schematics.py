#!/usr/bin/env python3
"""
Modul 03: Serial UART - Schematic Generator using Schemdraw
Generate professional circuit schematics untuk 12 percobaan
"""

import schemdraw
import schemdraw.elements as elm
import os
from pathlib import Path

# Create output directory
OUTPUT_DIR = Path('schematics_output')
OUTPUT_DIR.mkdir(exist_ok=True)

def setup_drawing(title: str, width: float = 14, height: float = 10):
    """Setup drawing configuration"""
    d = schemdraw.Drawing()
    d.config(fontsize=10, font='sans-serif', unit=0.8)
    return d

# ============================================================================
# PERCOBAAN 01: UART Echo - Polling
# ============================================================================
def draw_p01_uart_echo():
    """P01: UART Echo - Polling Schematic"""
    print("Generating P01: UART Echo - Polling...")
    
    d = setup_drawing("P01: UART Echo - Polling")
    
    # Title
    d += elm.Label().at((0, 9)).label('P01: UART Echo - Polling')
    d += elm.Label().at((0, 8.5)).label('Baud: 115200 (8N1)')
    
    # ESP32 Block
    d += elm.Box().at((1, 5.5)).fill(True).label('ESP32\nDevKit V1')
    
    # GPIO labels
    d += elm.Label().at((0.3, 5.5)).label('GPIO1(TX)')
    d += elm.Label().at((0.3, 5)).label('GPIO3(RX)')
    
    # USB Connection
    d += elm.Line().at((4, 6.2)).right(2)
    d += elm.Gap().label(['+', 'GND']).ofst(-0.3)
    d += elm.Label().at((5, 6.5)).label('USB\n115200')
    
    # PC/Laptop
    d += elm.Box().at((7, 5.5)).fill(False).label('PC\nSerial Monitor')
    
    # Notes
    d += elm.Label().at((1, 2.5)).label('Polling: blocking read/write')
    d += elm.Label().at((1, 2)).label('Loss on fast input')
    d += elm.Label().at((1, 1.5)).label('Frame: ~87us @ 115200 baud')
    
    d.save(str(OUTPUT_DIR / 'P01_UART_Echo.svg'))
    print("  ✅ P01_UART_Echo.svg")

# ============================================================================
# All remaining percobaan functions - Simplified version
# ============================================================================

def draw_remaining():
    """Draw all remaining experiments in simplified form"""
    
    # P02
    print("Generating P02: UART Interrupt RX...")
    d = setup_drawing("P02")
    d += elm.Label().at((0, 9)).label('P02: UART Interrupt RX - Event Queue')
    d += elm.Label().at((1, 8)).label('Non-blocking communication')
    d += elm.Label().at((1, 7)).label('RX Interrupt → ISR → Event Queue → Task')
    d += elm.Box().at((2, 5)).fill(True).label('ESP32\nFreeRTOS\nUART')
    d += elm.Label().at((1, 2)).label('Events: UART_DATA, FIFO_OVF, FRAME_ERR')
    d.save(str(OUTPUT_DIR / 'P02_UART_Interrupt_RX.svg'))
    print("  ✅ P02_UART_Interrupt_RX.svg")
    
    # P03
    print("Generating P03: Ring Buffer...")
    d = setup_drawing("P03")
    d += elm.Label().at((0, 9)).label('P03: Ring Buffer (Circular Buffer)')
    d += elm.Box().at((2, 6)).fill(True).label('Ring Buffer')
    d += elm.Label().at((2, 5)).label('[0..N-1]')
    d += elm.Label().at((1, 3)).label('ISR: rb_push() → Task: rb_pop()')
    d += elm.Label().at((1, 1.5)).label('Ops: init, push, pop, is_full, overflow detect')
    d.save(str(OUTPUT_DIR / 'P03_Ring_Buffer.svg'))
    print("  ✅ P03_Ring_Buffer.svg")
    
    # P04
    print("Generating P04: Printf Redirect...")
    d = setup_drawing("P04")
    d += elm.Label().at((0, 9)).label('P04: Printf Redirect - Multiple UART')
    d += elm.Box().at((1, 6.5)).fill(True).label('STM32')
    d += elm.Arrow().at((3, 6.5)).right(1).label('USART1')
    d += elm.Label().at((5, 6.5)).label('Operator')
    d += elm.Label().at((1, 4)).label('USART1: PA9/PA10')
    d += elm.Label().at((1, 3.5)).label('USART2: PA2/PA3')
    d += elm.Label().at((1, 2)).label('Two debug outputs simultaneously')
    d.save(str(OUTPUT_DIR / 'P04_Printf_Redirect.svg'))
    print("  ✅ P04_Printf_Redirect.svg")
    
    # P05
    print("Generating P05: Command Parser...")
    d = setup_drawing("P05")
    d += elm.Label().at((0, 9)).label('P05: Command Parser')
    d += elm.Label().at((1, 8)).label('Input: "BUZZER ON"')
    d += elm.Arrow().at((1, 7.5)).right(1.5).label('Parse')
    d += elm.Label().at((3.5, 7.5)).label('Commands: BUZZER, LED, SAMPLE...')
    d += elm.Label().at((1, 5)).label('Device Control via serial commands')
    d += elm.Label().at((1, 3)).label('Response: OK or ERROR')
    d.save(str(OUTPUT_DIR / 'P05_Command_Parser.svg'))
    print("  ✅ P05_Command_Parser.svg")
    
    # P06
    print("Generating P06: JSON Protocol...")
    d = setup_drawing("P06")
    d += elm.Label().at((0, 9)).label('P06: JSON Protocol (cJSON)')
    d += elm.Label().at((1, 8)).label('Sensor Data → JSON Encoding')
    d += elm.Label().at((1, 7)).label('{node:1, temp:25.3, humidity:60}')
    d += elm.Arrow().at((1, 6)).right(1.5).label('UART TX')
    d += elm.Label().at((4, 6)).label('→ Gateway parse')
    d += elm.Label().at((1, 4)).label('cJSON encode/decode')
    d.save(str(OUTPUT_DIR / 'P06_JSON_Protocol.svg'))
    print("  ✅ P06_JSON_Protocol.svg")
    
    # P07
    print("Generating P07: Line Editor...")
    d = setup_drawing("P07")
    d += elm.Label().at((0, 9)).label('P07: Interactive Line Editor')
    d += elm.Label().at((1, 8)).label('Input keys: a-z, Backspace, Enter, ↑↓')
    d += elm.Label().at((1, 7)).label('Backspace echo: \\b \\x20 \\b')
    d += elm.Label().at((1, 6)).label('Arrow keys: recall command history')
    d += elm.Label().at((1, 4)).label('Line buffer + history storage (last 10)')
    d.save(str(OUTPUT_DIR / 'P07_Line_Editor.svg'))
    print("  ✅ P07_Line_Editor.svg")
    
    # P08
    print("Generating P08: Framing STX/ETX...")
    d = setup_drawing("P08")
    d += elm.Label().at((0, 9)).label('P08: Packet Framing STX/ETX + Byte Stuffing')
    d += elm.Label().at((1, 8)).label('Frame: [STX] [NodeID] [Type] [Len] [Data] [CRC] [ETX]')
    d += elm.Label().at((1, 7)).label('STX=0x02, ETX=0x03, ESC=0x1B')
    d += elm.Label().at((1, 5.5)).label('Byte Stuffing:')
    d += elm.Label().at((1.5, 5)).label('if (data==STX/ETX) → send ESC + data')
    d += elm.Label().at((1, 3)).label('TX: Stuff → Send | RX: De-stuff ← Receive')
    d.save(str(OUTPUT_DIR / 'P08_Framing_STX_ETX.svg'))
    print("  ✅ P08_Framing_STX_ETX.svg")
    
    # P09
    print("Generating P09: CRC-8 Checksum...")
    d = setup_drawing("P09")
    d += elm.Label().at((0, 9)).label('P09: CRC-8 Checksum Error Detection')
    d += elm.Label().at((1, 8)).label('TX: Calculate CRC8 (poly=0x07)')
    d += elm.Arrow().at((1, 7.5)).right(2).label('Frame + CRC')
    d += elm.Label().at((4, 7.5)).label('→ UART TX')
    d += elm.Arrow().at((1, 6.5)).right(2).label('RX: Recalc CRC')
    d += elm.Label().at((4, 6.5)).label('→ Compare')
    d += elm.Label().at((1, 4)).label('Match? ✓ Accept : ✗ Reject')
    d.save(str(OUTPUT_DIR / 'P09_CRC8_Checksum.svg'))
    print("  ✅ P09_CRC8_Checksum.svg")
    
    # P10
    print("Generating P10: Timeout Parser...")
    d = setup_drawing("P10")
    d += elm.Label().at((0, 9)).label('P10: Timeout-Based Parser (Unframed)')
    d += elm.Label().at((1, 8)).label('Collect bytes in buffer')
    d += elm.Arrow().at((1, 7.5)).right(1.5).label('Timeout 50ms')
    d += elm.Label().at((4, 7.5)).label('→ Packet complete')
    d += elm.Label().at((1, 6)).label('No STX/ETX needed')
    d += elm.Label().at((1, 4)).label('Example: "SENSOR_DATA 25.3 60"')
    d.save(str(OUTPUT_DIR / 'P10_Timeout_Parser.svg'))
    print("  ✅ P10_Timeout_Parser.svg")
    
    # P11
    print("Generating P11: Multi-UART Bridge...")
    d = setup_drawing("P11")
    d += elm.Label().at((0, 9)).label('P11: Multi-UART Bridge (3 UART)')
    d += elm.Label().at((1, 8)).label('STM32 Node #1:')
    d += elm.Arrow().at((1, 7.5)).right(1).label('UART TX')
    d += elm.Box().at((3, 7.5)).fill(False).label('ESP32\n3 UART')
    d += elm.Label().at((1, 6.5)).label('STM32 Node #2:')
    d += elm.Arrow().at((1, 6)).right(1).label('UART TX')
    d += elm.Label().at((6, 7)).label('↓ Parser')
    d += elm.Label().at((6, 5)).label('↓ JSON')
    d += elm.Arrow().at((1, 4)).right(7).label('PC Monitor')
    d.save(str(OUTPUT_DIR / 'P11_Multi_UART_Bridge.svg'))
    print("  ✅ P11_Multi_UART_Bridge.svg")
    
    # P12
    print("Generating P12: Error Statistics...")
    d = setup_drawing("P12")
    d += elm.Label().at((0, 9)).label('P12: Error Statistics & Health Monitoring')
    d += elm.Label().at((1, 8)).label('Track: Parity, Frame, Overrun, CRC errors')
    d += elm.Box().at((2, 6)).fill(True).label('Error\nCounter')
    d += elm.Arrow().at((2, 5)).down(1).label('Calculate health')
    d += elm.Label().at((1, 3)).label('Node#1: 98%')
    d += elm.Label().at((1, 2.5)).label('Node#2: 95%')
    d += elm.Label().at((1, 1.5)).label('Alert: CRC>10%, Overrun>5')
    d.save(str(OUTPUT_DIR / 'P12_Error_Statistics.svg'))
    print("  ✅ P12_Error_Statistics.svg")

# ============================================================================
# Main Execution
# ============================================================================
def main():
    """Generate all schematics"""
    print("\n" + "="*70)
    print("Schemdraw UART Schematic Generator - Modul 03")
    print("="*70 + "\n")
    
    try:
        draw_p01_uart_echo()
        draw_p02_interrupt_rx()
        draw_p03_ring_buffer()
        draw_p04_printf_redirect()
        draw_p05_command_parser()
        draw_p06_json_protocol()
        draw_p07_line_editor()
        draw_p08_framing_stx_etx()
        draw_p09_crc8_checksum()
        draw_p10_timeout_parser()
        draw_p11_multi_uart_bridge()
        draw_p12_error_statistics()
        draw_system_architecture()
        
        print("\n" + "="*70)
        print("✅ All schematics generated successfully!")
        print(f"📁 Output directory: {OUTPUT_DIR.resolve()}")
        print("="*70 + "\n")
        
        # List generated files
        svg_files = list(OUTPUT_DIR.glob('*.svg'))
        print(f"Generated {len(svg_files)} schematic files:")
        for i, f in enumerate(sorted(svg_files), 1):
            print(f"  {i:2d}. {f.name}")
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == '__main__':
    main()
