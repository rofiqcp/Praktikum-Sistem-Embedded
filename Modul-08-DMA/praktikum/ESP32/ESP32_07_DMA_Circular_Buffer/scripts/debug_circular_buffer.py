"""
==========================================================================
 ESP32 DMA Circular Buffer - Debug & Visualization
==========================================================================
 Modul 08 - Program 07: Buffer fill level plot, processing rate chart,
                         overflow timeline
 
 Usage: python debug_circular_buffer.py [COM_PORT]
 Default: /dev/ttyUSB0
==========================================================================
"""

import serial
import re
import time
import sys
from collections import deque
from datetime import datetime

# ---- Configuration ----
SERIAL_PORT = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
BAUD_RATE = 115200
MAX_POINTS = 200

# ---- Data Storage ----
timestamps = deque(maxlen=MAX_POINTS)
fill_levels = deque(maxlen=MAX_POINTS)
write_rates = deque(maxlen=MAX_POINTS)
read_rates = deque(maxlen=MAX_POINTS)
overflow_counts = deque(maxlen=MAX_POINTS)
underflow_counts = deque(maxlen=MAX_POINTS)
max_fills = deque(maxlen=MAX_POINTS)

# ---- Regex Patterns ----
re_fill = re.compile(r'(\d+\.\d+)%')
re_write_rate = re.compile(r'Write rate:\s+([\d.]+)\s+sps')
re_read_rate = re.compile(r'Read rate:\s+([\d.]+)\s+sps')
re_overflow = re.compile(r'Overflows:\s+(\d+)')
re_underflow = re.compile(r'Underflows:\s+(\d+)')
re_max_fill = re.compile(r'Max fill:\s+(\d+)')
re_count = re.compile(r'Count:\s+(\d+)')
re_write_pos = re.compile(r'Write pos:\s+(\d+)')
re_read_pos = re.compile(r'Read pos:\s+(\d+)')
re_avg = re.compile(r'Avg:\s+([\d.]+)V')

def parse_line(line):
    """Parse a serial line and extract data."""
    data = {}
    
    m = re_fill.search(line)
    if m:
        data['fill_pct'] = float(m.group(1))
    
    m = re_write_rate.search(line)
    if m:
        data['write_rate'] = float(m.group(1))
    
    m = re_read_rate.search(line)
    if m:
        data['read_rate'] = float(m.group(1))
    
    m = re_overflow.search(line)
    if m:
        data['overflow'] = int(m.group(1))
    
    m = re_underflow.search(line)
    if m:
        data['underflow'] = int(m.group(1))
    
    m = re_max_fill.search(line)
    if m:
        data['max_fill'] = int(m.group(1))
    
    m = re_count.search(line)
    if m:
        data['count'] = int(m.group(1))
    
    m = re_write_pos.search(line)
    if m:
        data['write_pos'] = int(m.group(1))
    
    m = re_read_pos.search(line)
    if m:
        data['read_pos'] = int(m.group(1))
    
    m = re_avg.search(line)
    if m:
        data['avg_voltage'] = float(m.group(1))
    
    return data

def print_dashboard(data_acc):
    """Print a text-based dashboard."""
    print("\033[2J\033[H")  # Clear screen
    print("=" * 70)
    print("   ESP32 DMA Circular Buffer - Debug Dashboard")
    print("=" * 70)
    
    if 'fill_pct' in data_acc:
        pct = data_acc['fill_pct']
        bar_width = 50
        filled = int(pct * bar_width / 100)
        bar = '█' * filled + '░' * (bar_width - filled)
        color = '\033[92m' if pct < 50 else '\033[93m' if pct < 80 else '\033[91m'
        print(f"\n  Buffer Fill: {color}[{bar}] {pct:5.1f}%\033[0m")
    
    if 'write_rate' in data_acc and 'read_rate' in data_acc:
        wr = data_acc['write_rate']
        rr = data_acc['read_rate']
        ratio = wr / rr if rr > 0 else float('inf')
        print(f"\n  Write Rate:  {wr:10.1f} samples/sec")
        print(f"  Read Rate:   {rr:10.1f} samples/sec")
        print(f"  Ratio (W/R): {ratio:10.2f}x")
        if ratio > 1.0:
            print("  ⚠ Producer faster than consumer!")
    
    if 'overflow' in data_acc:
        print(f"\n  Overflows:   {data_acc.get('overflow', 0):8d}")
        print(f"  Underflows:  {data_acc.get('underflow', 0):8d}")
        print(f"  Max Fill:    {data_acc.get('max_fill', 0):8d}")
    
    if 'avg_voltage' in data_acc:
        print(f"\n  Avg Voltage: {data_acc['avg_voltage']:.3f} V")
    
    # Overflow timeline (last N events)
    if len(overflow_counts) > 1:
        print("\n  Overflow Timeline (last 10):")
        recent = list(overflow_counts)[-10:]
        prev = recent[0]
        for i, val in enumerate(recent[1:], 1):
            delta = val - prev
            marker = '🔴' if delta > 0 else '🟢'
            print(f"    {marker} +{delta} overflows")
            prev = val
    
    print("\n" + "=" * 70)
    print(f"  Timestamp: {datetime.now().strftime('%H:%M:%S.%f')[:-3]}")
    print(f"  Data points collected: {len(fill_levels)}")
    print("=" * 70)

def main():
    """Main debug loop."""
    print(f"Connecting to {SERIAL_PORT} at {BAUD_RATE} baud...")
    
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"Connected! Monitoring circular buffer...\n")
    except serial.SerialException as e:
        print(f"Error: {e}")
        print("Entering demo mode with simulated data...")
        demo_mode()
        return
    
    data_acc = {}
    
    try:
        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    parsed = parse_line(line)
                    data_acc.update(parsed)
                    
                    # When we get fill percentage, record a data point
                    if 'fill_pct' in parsed:
                        now = time.time()
                        timestamps.append(now)
                        fill_levels.append(parsed['fill_pct'])
                    
                    if 'write_rate' in parsed:
                        write_rates.append(parsed['write_rate'])
                    if 'read_rate' in parsed:
                        read_rates.append(parsed['read_rate'])
                    if 'overflow' in parsed:
                        overflow_counts.append(parsed['overflow'])
                    if 'underflow' in parsed:
                        underflow_counts.append(parsed['underflow'])
                    
                    # Update dashboard periodically
                    if 'fill_pct' in parsed:
                        print_dashboard(data_acc)
    
    except KeyboardInterrupt:
        print("\n\nStopped by user.")
        print_summary(data_acc)
    finally:
        ser.close()

def print_summary(data_acc):
    """Print final summary."""
    print("\n" + "=" * 70)
    print("  FINAL SUMMARY")
    print("=" * 70)
    if fill_levels:
        print(f"  Fill Level  - Min: {min(fill_levels):.1f}%  "
              f"Max: {max(fill_levels):.1f}%  "
              f"Avg: {sum(fill_levels)/len(fill_levels):.1f}%")
    if write_rates:
        print(f"  Write Rate  - Min: {min(write_rates):.0f}  "
              f"Max: {max(write_rates):.0f}  "
              f"Avg: {sum(write_rates)/len(write_rates):.0f} sps")
    if overflow_counts:
        print(f"  Total Overflows:  {max(overflow_counts)}")
    if underflow_counts:
        print(f"  Total Underflows: {max(underflow_counts)}")
    print("=" * 70)

def demo_mode():
    """Demo mode with simulated data."""
    import math
    print("\n--- DEMO MODE (no hardware) ---\n")
    
    t = 0
    overflow_total = 0
    
    try:
        while True:
            fill = 50 + 40 * math.sin(t * 0.1) + 5 * math.sin(t * 0.5)
            fill = max(0, min(100, fill))
            wr = 10000 + 500 * math.sin(t * 0.2)
            rr = 8000 + 300 * math.cos(t * 0.15)
            
            if fill > 90:
                overflow_total += int((fill - 90) * 10)
            
            data_acc = {
                'fill_pct': fill,
                'write_rate': wr,
                'read_rate': rr,
                'overflow': overflow_total,
                'underflow': 0,
                'max_fill': int(max(fill_levels) if fill_levels else fill),
                'avg_voltage': 1.65 + 0.5 * math.sin(t * 0.05),
            }
            
            fill_levels.append(fill)
            write_rates.append(wr)
            read_rates.append(rr)
            overflow_counts.append(overflow_total)
            
            print_dashboard(data_acc)
            time.sleep(0.5)
            t += 1
    
    except KeyboardInterrupt:
        print("\nDemo stopped.")

if __name__ == '__main__':
    main()
