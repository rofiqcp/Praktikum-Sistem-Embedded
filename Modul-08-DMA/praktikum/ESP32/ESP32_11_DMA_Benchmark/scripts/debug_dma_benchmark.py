"""
==========================================================================
 ESP32 DMA Benchmark - Debug & Visualization
==========================================================================
 Modul 08 - Program 11: Multi-panel benchmark comparison, heatmap,
                         recommendation chart
 
 Usage: python debug_dma_benchmark.py [COM_PORT]
==========================================================================
"""

import serial
import re
import sys
from collections import defaultdict
from datetime import datetime

# ---- Configuration ----
SERIAL_PORT = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
BAUD_RATE = 115200

# ---- Data Storage ----
results = []

# ---- Regex Patterns ----
re_result = re.compile(
    r'(memcpy|loop|SPI DMA|SPI CPU|UART buf|UART bbb|ADC poll|ADC DMA)\s+'
    r'(\d+)\s+(?:bytes|samp):\s+([\d.]+)\s+us/?(?:op|total),?\s+([\d.]+)\s+(?:MB/s|sps)'
)
re_summary_row = re.compile(
    r'║\s+(\w+)\s+║\s+(\S+)\s+║\s+(\d+)\s+║\s+([\d.]+)\s+║\s+([\d.]+)\s+║.*?([\d.]+)%'
)
re_speedup = re.compile(r'(\w+)\s+([\d.]+)x faster')

class BenchResult:
    def __init__(self, test, method, size, time_us, throughput, cpu_pct=0):
        self.test = test
        self.method = method
        self.size = size
        self.time_us = time_us
        self.throughput = throughput
        self.cpu_pct = cpu_pct

def parse_line(line):
    """Parse benchmark output line."""
    m = re_summary_row.search(line)
    if m:
        return BenchResult(
            test=m.group(1),
            method=m.group(2),
            size=int(m.group(3)),
            time_us=float(m.group(4)),
            throughput=float(m.group(5)),
            cpu_pct=float(m.group(6))
        )
    return None

def print_bar_chart(data, title, unit="", width=40):
    """Print horizontal bar chart."""
    if not data:
        return
    
    print(f"\n  {title}")
    print(f"  {'─' * (width + 30)}")
    
    max_val = max(v for _, v in data)
    if max_val == 0:
        max_val = 1
    
    for label, value in data:
        bar_len = int(value * width / max_val)
        bar = '█' * bar_len + '░' * (width - bar_len)
        print(f"  {label:20s} │{bar}│ {value:.1f} {unit}")

def print_heatmap(results_by_test):
    """Print a text-based heatmap."""
    print(f"\n  THROUGHPUT HEATMAP (MB/s)")
    print(f"  {'─' * 60}")
    
    sizes = sorted(set(r.size for r in results))
    methods = sorted(set(r.method for r in results))
    
    if not sizes or not methods:
        print("  (no data)")
        return
    
    # Header
    print(f"  {'Method':<15}", end="")
    for s in sizes:
        print(f"  {s:>6}", end="")
    print()
    print(f"  {'─' * 15}", end="")
    for s in sizes:
        print(f"  {'─' * 6}", end="")
    print()
    
    for method in methods:
        print(f"  {method:<15}", end="")
        for size in sizes:
            # Find matching result
            matching = [r for r in results if r.method == method and r.size == size]
            if matching:
                tp = matching[0].throughput
                # Color based on throughput
                if tp > 100:
                    color = '\033[92m'  # Green
                elif tp > 10:
                    color = '\033[93m'  # Yellow
                else:
                    color = '\033[91m'  # Red
                print(f"  {color}{tp:6.1f}\033[0m", end="")
            else:
                print(f"  {'─':>6}", end="")
        print()

def print_recommendation():
    """Print recommendation chart."""
    print(f"\n  {'═' * 55}")
    print(f"  RECOMMENDATION CHART")
    print(f"  {'═' * 55}")
    
    recommendations = [
        ("SPI > 64 bytes", "DMA", "████████████████████", "AUTO"),
        ("SPI ≤ 64 bytes", "Either", "██████████░░░░░░░░░░", "OK w/o DMA"),
        ("ADC continuous", "DMA", "████████████████████", "REQUIRED"),
        ("ADC single-shot", "Polling", "████░░░░░░░░░░░░░░░░", "Simple use"),
        ("UART bulk", "Buffered", "████████████████░░░░", "ALWAYS"),
        ("Memory copy", "memcpy()", "████████████████████", "Optimized"),
    ]
    
    print(f"  {'Use Case':<20} {'Rec.':<10} {'Benefit':<22} {'Notes'}")
    print(f"  {'─' * 20} {'─' * 10} {'─' * 22} {'─' * 12}")
    for use, rec, bar, note in recommendations:
        print(f"  {use:<20} {rec:<10} {bar} {note}")

def print_dashboard():
    """Print comprehensive dashboard."""
    print("\033[2J\033[H")
    print("=" * 65)
    print("   ESP32 DMA Benchmark - Analysis Dashboard")
    print("=" * 65)
    
    if not results:
        print("\n  Waiting for benchmark data...\n")
        return
    
    # Group by test
    by_test = defaultdict(list)
    for r in results:
        by_test[r.test].append(r)
    
    # Throughput comparison
    throughput_data = [(f"{r.test}/{r.method} {r.size}B", r.throughput)
                       for r in results if r.throughput > 0]
    if throughput_data:
        # Show top 10
        throughput_data.sort(key=lambda x: x[1], reverse=True)
        print_bar_chart(throughput_data[:10], "TOP 10 THROUGHPUT", "MB/s")
    
    # CPU usage comparison
    cpu_data = [(f"{r.test}/{r.method}", r.cpu_pct)
                for r in results if r.cpu_pct > 0]
    if cpu_data:
        # Deduplicate
        seen = set()
        unique_cpu = []
        for label, pct in cpu_data:
            if label not in seen:
                seen.add(label)
                unique_cpu.append((label, pct))
        print_bar_chart(unique_cpu, "CPU USAGE (%)", "%")
    
    # Heatmap
    print_heatmap(results)
    
    # Recommendations
    print_recommendation()
    
    print(f"\n{'=' * 65}")
    print(f"  Total tests: {len(results)}")
    print(f"  {datetime.now().strftime('%H:%M:%S')}")

def main():
    """Main function."""
    print(f"Connecting to {SERIAL_PORT}...")
    
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print("Connected! Waiting for benchmark...\n")
    except serial.SerialException as e:
        print(f"Error: {e}\nRunning demo...\n")
        demo_mode()
        return
    
    try:
        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    result = parse_line(line)
                    if result:
                        results.append(result)
                    
                    # Check for summary section end
                    if 'Benchmark complete' in line or 'RECOMMENDATIONS' in line:
                        print_dashboard()
                    
                    # Also print raw line
                    print(line)
    
    except KeyboardInterrupt:
        print("\n")
        print_dashboard()
    finally:
        ser.close()

def demo_mode():
    """Demo with sample data."""
    global results
    
    demo_data = [
        BenchResult("MemCopy", "memcpy", 64, 0.5, 122.0, 100),
        BenchResult("MemCopy", "for-loop", 64, 2.1, 29.0, 100),
        BenchResult("MemCopy", "memcpy", 1024, 3.2, 305.0, 100),
        BenchResult("MemCopy", "for-loop", 1024, 18.5, 52.8, 100),
        BenchResult("MemCopy", "memcpy", 16384, 28.0, 558.0, 100),
        BenchResult("MemCopy", "for-loop", 16384, 285.0, 54.8, 100),
        BenchResult("SPI", "DMA", 64, 15.2, 4.0, 50),
        BenchResult("SPI", "DMA", 1024, 42.5, 23.0, 15),
        BenchResult("SPI", "DMA", 16384, 180.0, 86.8, 10),
        BenchResult("SPI", "no-DMA", 64, 18.5, 3.3, 100),
        BenchResult("UART", "buffered", 64, 700, 0.087, 40),
        BenchResult("UART", "byte-by-byte", 64, 2800, 0.022, 95),
        BenchResult("ADC", "polling", 128, 2500, 0.049, 100),
        BenchResult("ADC", "DMA-cont", 128, 800, 0.153, 20),
    ]
    
    results = demo_data
    print_dashboard()
    
    input("\nPress Enter to exit...")

if __name__ == '__main__':
    main()
