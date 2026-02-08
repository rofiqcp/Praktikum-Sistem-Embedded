"""
==========================================================================
 ESP32 DMA Linked List / Scatter-Gather - Debug & Visualization
==========================================================================
 Modul 08 - Program 12: Timing comparison, scatter-gather visualization,
                         packet assembly diagram
 
 Usage: python debug_linked_list.py [COM_PORT]
==========================================================================
"""

import serial
import re
import sys
from collections import deque
from datetime import datetime

# ---- Configuration ----
SERIAL_PORT = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
BAUD_RATE = 115200
MAX_POINTS = 100

# ---- Data Storage ----
sequential_times = deque(maxlen=MAX_POINTS)
concatenate_times = deque(maxlen=MAX_POINTS)
queued_times = deque(maxlen=MAX_POINTS)
packets = []

# ---- Regex Patterns ----
re_sequential = re.compile(r'Sequential.*?Avg:\s+([\d.]+)\s+us')
re_concatenate = re.compile(r'Concatenate.*?Avg:\s+([\d.]+)\s+us')
re_queued = re.compile(r'Queued.*?Avg:\s+([\d.]+)\s+us')
re_packet = re.compile(r'Packet #(\d+)')
re_crc = re.compile(r'CRC32:\s+0x([0-9A-Fa-f]+)\s+([✓✗])')
re_method_time = re.compile(r'(?:Total|Avg):\s+([\d.]+)\s+us')
re_packets_sent = re.compile(r'Packets sent:\s+(\d+)')
re_verify = re.compile(r'Verify errors:\s+(\d+)')

# Timing from summary table
re_seq_row = re.compile(r'Sequential\s+║\s+([\d.]+)')
re_con_row = re.compile(r'Concatenate\s+║\s+([\d.]+)')
re_que_row = re.compile(r'Queued\s+║\s+([\d.]+)')

def print_scatter_gather_diagram():
    """Print scatter-gather memory layout visualization."""
    print("""
  ╔═══════════════════════════════════════════════════╗
  ║         SCATTER-GATHER MEMORY LAYOUT              ║
  ╚═══════════════════════════════════════════════════╝

  Memory Address Space (non-contiguous):
  
  0x3FFB_1000 ┌──────────────┐
              │   Header     │  16 bytes
              │  (magic, seq,│
              │   lengths)   │
              └──────┬───────┘
                     │
  0x3FFB_2000 ┌──────┴───────────────┐
              │   Payload 1           │  128 bytes
              │  (sensor data)        │
              └──────┬────────────────┘
                     │
  0x3FFB_3000 ┌──────┴──────────────────────────┐
              │   Payload 2                      │  256 bytes
              │  (configuration data)            │
              └──────┬───────────────────────────┘
                     │
  0x3FFB_4000 ┌──────┴──┐
              │   CRC   │  4 bytes
              │  (CRC32)│
              └─────────┘

  Three Gather Methods:
  
  Method 1 (Sequential):     Method 2 (Concatenate):    Method 3 (Queued):
  ┌───┐ → SPI TX             ┌───┐                      ┌───┐ → queue
  │ H │                      │ H │─┐                     │ H │
  └───┘                      └───┘ │  ┌─────────────┐   └───┘
  ┌─────┐ → SPI TX           ┌─────┐│  │ Contiguous  │   ┌─────┐ → queue
  │ P1  │                    │ P1  │├→ │   Buffer    │   │ P1  │
  └─────┘                    └─────┘│  │ (memcpy)   │   └─────┘
  ┌─────────┐ → SPI TX       ┌─────┘│  └──────┬──────┘   ┌─────────┐ → queue
  │   P2    │                │ P2  │ │         │          │   P2    │
  └─────────┘                └─────┘ │     SPI DMA TX     └─────────┘
  ┌──┐ → SPI TX              ┌──┐   │                    ┌──┐ → queue
  │CRC│                      │CRC│──┘                    │CRC│
  └──┘                       └──┘                        └──┘
  (4 transactions)           (4 memcpy + 1 TX)           (batch execute)
""")

def print_timing_bars(seq_t, con_t, que_t):
    """Print timing comparison bars."""
    max_t = max(seq_t, con_t, que_t, 1)
    width = 40
    
    print(f"\n  TIMING COMPARISON")
    print(f"  {'─' * 55}")
    
    seq_w = int(seq_t * width / max_t)
    con_w = int(con_t * width / max_t)
    que_w = int(que_t * width / max_t)
    
    # Color: green for fastest
    times = [seq_t, con_t, que_t]
    min_t = min(t for t in times if t > 0)
    
    def color(t):
        if t == min_t:
            return '\033[92m'  # Green
        elif t > min_t * 1.5:
            return '\033[91m'  # Red
        return '\033[93m'  # Yellow
    
    print(f"  Sequential:  {color(seq_t)}[{'█' * seq_w}{'░' * (width - seq_w)}] "
          f"{seq_t:.1f} us\033[0m")
    print(f"  Concatenate: {color(con_t)}[{'█' * con_w}{'░' * (width - con_w)}] "
          f"{con_t:.1f} us\033[0m")
    print(f"  Queued:      {color(que_t)}[{'█' * que_w}{'░' * (width - que_w)}] "
          f"{que_t:.1f} us\033[0m")
    
    # Speedup
    base = seq_t if seq_t > 0 else 1
    print(f"\n  Speedup vs Sequential:")
    print(f"    Concatenate: {base / (con_t if con_t > 0 else 1):.2f}x")
    print(f"    Queued:      {base / (que_t if que_t > 0 else 1):.2f}x")

def print_dashboard(data_acc):
    """Print full dashboard."""
    print("\033[2J\033[H")
    print("=" * 60)
    print("   ESP32 Scatter-Gather DMA - Debug Dashboard")
    print("=" * 60)
    
    # Scatter-gather diagram
    print_scatter_gather_diagram()
    
    # Timing comparison
    seq = data_acc.get('sequential', 0)
    con = data_acc.get('concatenate', 0)
    que = data_acc.get('queued', 0)
    
    if seq > 0 or con > 0 or que > 0:
        print_timing_bars(seq, con, que)
    
    # Packet info
    ps = data_acc.get('packets_sent', 0)
    ve = data_acc.get('verify_errors', 0)
    
    print(f"\n  Packets Sent:   {ps}")
    print(f"  Verify Errors:  {ve}")
    integrity = "✓ ALL PASS" if ve == 0 else f"✗ {ve} FAILURES"
    color = '\033[92m' if ve == 0 else '\033[91m'
    print(f"  CRC Integrity:  {color}{integrity}\033[0m")
    
    # Recommendation
    if seq > 0 and con > 0 and que > 0:
        times = {'Sequential': seq, 'Concatenate': con, 'Queued': que}
        best = min(times, key=times.get)
        print(f"\n  ╔═══════════════════════════════════════╗")
        print(f"  ║  RECOMMENDED: {best:25s}  ║")
        print(f"  ╚═══════════════════════════════════════╝")
    
    print(f"\n{'=' * 60}")
    print(f"  {datetime.now().strftime('%H:%M:%S')}")

def main():
    """Main function."""
    print(f"Connecting to {SERIAL_PORT}...")
    
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print("Connected!\n")
    except serial.SerialException as e:
        print(f"Error: {e}\nRunning demo...\n")
        demo_mode()
        return
    
    data_acc = {}
    
    try:
        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(line)
                    
                    m = re_seq_row.search(line)
                    if m:
                        data_acc['sequential'] = float(m.group(1))
                    
                    m = re_con_row.search(line)
                    if m:
                        data_acc['concatenate'] = float(m.group(1))
                    
                    m = re_que_row.search(line)
                    if m:
                        data_acc['queued'] = float(m.group(1))
                    
                    m = re_packets_sent.search(line)
                    if m:
                        data_acc['packets_sent'] = int(m.group(1))
                    
                    m = re_verify.search(line)
                    if m:
                        data_acc['verify_errors'] = int(m.group(1))
                    
                    if 'benchmark complete' in line.lower() or \
                       'KESIMPULAN' in line:
                        print_dashboard(data_acc)
    
    except KeyboardInterrupt:
        print("\n")
        if data_acc:
            print_dashboard(data_acc)
    finally:
        ser.close()

def demo_mode():
    """Demo with sample data."""
    data_acc = {
        'sequential': 285.3,
        'concatenate': 180.7,
        'queued': 145.2,
        'packets_sent': 300,
        'verify_errors': 0,
    }
    
    print_dashboard(data_acc)
    input("\nPress Enter to exit...")

if __name__ == '__main__':
    main()
