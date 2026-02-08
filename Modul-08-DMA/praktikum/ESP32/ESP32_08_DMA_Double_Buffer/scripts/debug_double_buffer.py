"""
==========================================================================
 ESP32 DMA Double Buffer - Debug & Visualization
==========================================================================
 Modul 08 - Program 08: Timing diagram of buffer swaps, throughput 
                         comparison, latency chart
 
 Usage: python debug_double_buffer.py [COM_PORT]
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
collection_times = deque(maxlen=MAX_POINTS)
processing_times = deque(maxlen=MAX_POINTS)
overlap_pcts = deque(maxlen=MAX_POINTS)
swap_counts = deque(maxlen=MAX_POINTS)
sb_throughputs = deque(maxlen=MAX_POINTS)
db_throughputs = deque(maxlen=MAX_POINTS)
rms_values = deque(maxlen=MAX_POINTS)
timestamps = deque(maxlen=MAX_POINTS)

# ---- Regex Patterns ----
re_collection = re.compile(r'Avg Collection Time:\s+([\d.]+)\s+us')
re_processing = re.compile(r'Avg Processing Time:\s+([\d.]+)\s+us')
re_overlap = re.compile(r'Overlap Percentage:\s+([\d.]+)\s+%')
re_swaps = re.compile(r'Buffer Swaps:\s+(\d+)')
re_sb_tp = re.compile(r'Single Buffer:\s+([\d.]+)\s+samples')
re_db_tp = re.compile(r'Double Buffer:\s+([\d.]+)\s+samples')
re_improve = re.compile(r'Improvement:\s+([+-]?[\d.]+)\s+%')
re_rms = re.compile(r'RMS:\s+([\d.]+)')
re_peak = re.compile(r'Peak:\s+([\d.]+)\s+V')
re_active = re.compile(r'Active Buffer:\s+(\w)')

def parse_line(line):
    """Parse serial output line."""
    data = {}
    
    for name, pattern in [
        ('collection_time', re_collection),
        ('processing_time', re_processing),
        ('overlap_pct', re_overlap),
        ('swap_count', re_swaps),
        ('sb_throughput', re_sb_tp),
        ('db_throughput', re_db_tp),
        ('improvement', re_improve),
        ('rms', re_rms),
        ('peak', re_peak),
    ]:
        m = pattern.search(line)
        if m:
            data[name] = float(m.group(1))
    
    m = re_active.search(line)
    if m:
        data['active_buffer'] = m.group(1)
    
    return data

def print_timing_diagram(ct, pt):
    """Print a text-based timing diagram."""
    if ct <= 0 or pt <= 0:
        return
    
    max_t = max(ct, pt)
    width = 50
    cw = int(ct * width / max_t)
    pw = int(pt * width / max_t)
    
    print(f"\n  {'─' * 60}")
    print(f"  TIMING DIAGRAM (us)")
    print(f"  {'─' * 60}")
    
    # Cycle N
    print(f"  Buf A  Collect: [{'█' * cw}{' ' * (width - cw)}] {ct:.0f}us")
    print(f"  Buf B  Process: [{' ' * 3}{'▓' * pw}{' ' * (width - pw - 3)}] {pt:.0f}us")
    print(f"  {'─' * 60}")
    
    # Cycle N+1  
    print(f"  Buf B  Collect: [{' ' * 3}{'█' * cw}{' ' * (width - cw - 3)}] {ct:.0f}us")
    print(f"  Buf A  Process: [{' ' * 6}{'▓' * pw}{' ' * (width - pw - 6)}] {pt:.0f}us")
    print(f"  {'─' * 60}")
    
    overlap = min(ct, pt) / (ct + pt) * 100
    print(f"  Overlap: {overlap:.1f}% | Sequential: {ct + pt:.0f}us | "
          f"Pipelined: {max_t:.0f}us")

def print_dashboard(data_acc):
    """Print comprehensive dashboard."""
    print("\033[2J\033[H")
    print("=" * 65)
    print("   ESP32 Double Buffer (Ping-Pong) - Debug Dashboard")
    print("=" * 65)
    
    ab = data_acc.get('active_buffer', '?')
    swaps = data_acc.get('swap_count', 0)
    print(f"\n  Active Buffer: {ab}  |  Swaps: {swaps:.0f}")
    
    ct = data_acc.get('collection_time', 0)
    pt = data_acc.get('processing_time', 0)
    
    if ct > 0 and pt > 0:
        print_timing_diagram(ct, pt)
    
    # Throughput comparison
    sb = data_acc.get('sb_throughput', 0)
    db_t = data_acc.get('db_throughput', 0)
    imp = data_acc.get('improvement', 0)
    
    print(f"\n  {'─' * 60}")
    print(f"  THROUGHPUT COMPARISON")
    print(f"  {'─' * 60}")
    
    max_tp = max(sb, db_t, 1)
    bar_w = 40
    sb_w = int(sb * bar_w / max_tp) if max_tp > 0 else 0
    db_w = int(db_t * bar_w / max_tp) if max_tp > 0 else 0
    
    print(f"  Single: [{'█' * sb_w}{' ' * (bar_w - sb_w)}] {sb:.0f} sps")
    print(f"  Double: [{'█' * db_w}{' ' * (bar_w - db_w)}] {db_t:.0f} sps")
    color = '\033[92m' if imp > 0 else '\033[91m'
    print(f"  Improvement: {color}{imp:+.1f}%\033[0m")
    
    # Processing results
    if 'rms' in data_acc:
        print(f"\n  RMS: {data_acc['rms']:.1f}  |  Peak: {data_acc.get('peak', 0):.3f}V")
    
    # Latency chart (history)
    if len(collection_times) > 5:
        print(f"\n  {'─' * 60}")
        print(f"  LATENCY TREND (last 10)")
        print(f"  {'─' * 60}")
        recent_ct = list(collection_times)[-10:]
        recent_pt = list(processing_times)[-10:]
        for i, (c, p) in enumerate(zip(recent_ct, recent_pt)):
            ratio = c / p if p > 0 else 0
            print(f"    [{i+1:2d}] C:{c:7.0f}us  P:{p:7.0f}us  "
                  f"ratio:{ratio:.2f}")
    
    print(f"\n{'=' * 65}")
    print(f"  {datetime.now().strftime('%H:%M:%S')} | "
          f"Points: {len(collection_times)}")

def main():
    """Main debug loop."""
    print(f"Connecting to {SERIAL_PORT}...")
    
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print("Connected!\n")
    except serial.SerialException as e:
        print(f"Error: {e}")
        print("Running demo mode...\n")
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
                    
                    if 'collection_time' in parsed:
                        collection_times.append(parsed['collection_time'])
                    if 'processing_time' in parsed:
                        processing_times.append(parsed['processing_time'])
                    if 'overlap_pct' in parsed:
                        overlap_pcts.append(parsed['overlap_pct'])
                        timestamps.append(time.time())
                        print_dashboard(data_acc)
                    if 'sb_throughput' in parsed:
                        sb_throughputs.append(parsed['sb_throughput'])
                    if 'db_throughput' in parsed:
                        db_throughputs.append(parsed['db_throughput'])
    
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ser.close()

def demo_mode():
    """Simulated demo mode."""
    import math
    t = 0
    data_acc = {}
    
    try:
        while True:
            ct = 800 + 100 * math.sin(t * 0.2)
            pt = 1200 + 150 * math.cos(t * 0.15)
            
            data_acc.update({
                'active_buffer': 'A' if int(t) % 2 == 0 else 'B',
                'swap_count': t * 5,
                'collection_time': ct,
                'processing_time': pt,
                'overlap_pct': min(ct, pt) / (ct + pt) * 100,
                'sb_throughput': 5000 + 200 * math.sin(t * 0.1),
                'db_throughput': 8000 + 300 * math.cos(t * 0.12),
                'improvement': 60 + 5 * math.sin(t * 0.08),
                'rms': 1200 + 100 * math.sin(t * 0.3),
                'peak': 3.1 + 0.2 * math.sin(t * 0.25),
            })
            
            collection_times.append(ct)
            processing_times.append(pt)
            overlap_pcts.append(data_acc['overlap_pct'])
            
            print_dashboard(data_acc)
            time.sleep(0.8)
            t += 1
    except KeyboardInterrupt:
        print("\nDemo stopped.")

if __name__ == '__main__':
    main()
