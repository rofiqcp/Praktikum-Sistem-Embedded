#!/usr/bin/env python3
"""
Debug Script: SPIFFS File System Operations Monitor
Modul 07 - SPI & Storage | Program 09

Fitur:
- Parse serial output dari operasi SPIFFS
- Track penggunaan storage (total/used/free)
- Visualisasi timeline operasi file
- Statistik operasi (write/read/append/delete)

Penggunaan:
    python debug_spiffs.py [PORT] [BAUDRATE]
    python debug_spiffs.py /dev/ttyUSB0 115200
"""

import sys
import re
import time
from datetime import datetime
from collections import defaultdict

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WARN] pyserial not installed. Install: pip install pyserial")

try:
    import matplotlib.pyplot as plt
    import matplotlib.dates as mdates
    from matplotlib.patches import FancyBboxPatch
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed. Install: pip install matplotlib")


class SPIFFSDebugger:
    """Parser dan visualizer untuk operasi SPIFFS."""

    def __init__(self):
        self.operations = []       # List of (timestamp, operation, details)
        self.storage_history = []  # List of (timestamp, total, used, free)
        self.file_sizes = {}       # filename -> size
        self.op_counts = defaultdict(int)
        self.op_times = defaultdict(list)  # operation -> [time_us]
        self.start_time = time.time()

    def parse_line(self, line):
        """Parse satu baris output serial."""
        timestamp = time.time() - self.start_time

        # Parse storage info
        total_match = re.search(r'Total:\s*(\d+)\s*bytes', line)
        used_match = re.search(r'Used\s*:\s*(\d+)\s*bytes', line)
        free_match = re.search(r'Free\s*:\s*(\d+)\s*bytes', line)

        if total_match and used_match:
            total = int(total_match.group(1))
            used = int(used_match.group(1))
            free = total - used
            self.storage_history.append((timestamp, total, used, free))
            print(f"  [STORAGE] Total={total}B, Used={used}B, Free={free}B ({used/total*100:.1f}%)")

        # Parse file operations
        if 'written successfully' in line.lower() or 'written:' in line.lower():
            fname = self._extract_filename(line)
            self.operations.append((timestamp, 'WRITE', fname))
            self.op_counts['write'] += 1
            print(f"  [OP] WRITE: {fname}")

        elif 'appended' in line.lower():
            fname = self._extract_filename(line)
            self.operations.append((timestamp, 'APPEND', fname))
            self.op_counts['append'] += 1
            print(f"  [OP] APPEND: {fname}")

        elif 'file size:' in line.lower() or 'read complete' in line.lower():
            fname = self._extract_filename(line)
            self.operations.append((timestamp, 'READ', fname))
            self.op_counts['read'] += 1

        elif 'deleted successfully' in line.lower():
            fname = self._extract_filename(line)
            self.operations.append((timestamp, 'DELETE', fname))
            self.op_counts['delete'] += 1
            print(f"  [OP] DELETE: {fname}")

        elif 'unregistered' in line.lower():
            self.operations.append((timestamp, 'UNMOUNT', 'SPIFFS'))
            self.op_counts['unmount'] += 1
            print(f"  [OP] UNMOUNT: SPIFFS")

        # Parse timing data
        time_match = re.search(r'time:\s*(\d+)\s*us', line.lower())
        if time_match:
            time_us = int(time_match.group(1))
            # Determine which op
            if 'write' in line.lower():
                self.op_times['write'].append(time_us)
            elif 'read' in line.lower():
                self.op_times['read'].append(time_us)

        # Parse file listing
        file_entry = re.match(r'.*?(\S+\.\w+)\s+(\d+)', line)
        if file_entry and 'bytes' not in line.lower() and 'total' not in line.lower():
            fname = file_entry.group(1)
            fsize = int(file_entry.group(2))
            self.file_sizes[fname] = fsize

    def _extract_filename(self, line):
        """Extract filename dari baris log."""
        match = re.search(r"['\"]?(/spiffs/\S+)['\"]?", line)
        if match:
            return match.group(1)
        match = re.search(r'(\w+\.\w+)', line)
        if match:
            return match.group(1)
        return "unknown"

    def print_summary(self):
        """Print ringkasan operasi."""
        print("\n" + "=" * 60)
        print("          SPIFFS Operations Summary")
        print("=" * 60)

        print(f"\nTotal operations: {sum(self.op_counts.values())}")
        for op, count in sorted(self.op_counts.items()):
            print(f"  {op.upper():>10s}: {count}")

        if self.op_times:
            print(f"\nTiming Statistics:")
            for op, times in self.op_times.items():
                if times:
                    avg_t = sum(times) / len(times)
                    print(f"  {op.upper():>10s}: avg={avg_t:.0f}us, "
                          f"min={min(times)}us, max={max(times)}us")

        if self.file_sizes:
            print(f"\nFile Inventory:")
            for fname, fsize in sorted(self.file_sizes.items()):
                print(f"  {fname:30s} {fsize:>8d} bytes")

        if self.storage_history:
            last = self.storage_history[-1]
            print(f"\nFinal Storage: Total={last[1]}B, Used={last[2]}B, "
                  f"Free={last[3]}B ({last[2]/last[1]*100:.1f}%)")

        print("=" * 60)

    def plot_results(self):
        """Generate visualisasi hasil."""
        if not HAS_MATPLOTLIB:
            print("[WARN] matplotlib not available, skipping plots")
            return

        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('SPIFFS File System Debug Analysis', fontsize=14, fontweight='bold')

        # Plot 1: Storage usage over time
        ax1 = axes[0][0]
        if self.storage_history:
            times = [s[0] for s in self.storage_history]
            used = [s[2] for s in self.storage_history]
            free = [s[3] for s in self.storage_history]
            ax1.fill_between(times, 0, used, alpha=0.6, color='coral', label='Used')
            ax1.fill_between(times, used, [u + f for u, f in zip(used, free)],
                             alpha=0.4, color='lightgreen', label='Free')
            ax1.set_ylabel('Bytes')
            ax1.legend()
        ax1.set_title('Storage Usage Over Time')
        ax1.set_xlabel('Time (s)')
        ax1.grid(True, alpha=0.3)

        # Plot 2: Operations timeline
        ax2 = axes[0][1]
        if self.operations:
            op_colors = {'WRITE': 'green', 'READ': 'blue', 'APPEND': 'orange',
                         'DELETE': 'red', 'UNMOUNT': 'gray'}
            op_y = {'WRITE': 4, 'READ': 3, 'APPEND': 2, 'DELETE': 1, 'UNMOUNT': 0}
            for ts, op, detail in self.operations:
                y = op_y.get(op, 0)
                color = op_colors.get(op, 'black')
                ax2.scatter(ts, y, c=color, s=80, zorder=5, edgecolors='black', linewidth=0.5)
                ax2.annotate(detail.split('/')[-1] if '/' in detail else detail,
                             (ts, y), textcoords="offset points", xytext=(5, 5),
                             fontsize=7, rotation=30)
            ax2.set_yticks(list(op_y.values()))
            ax2.set_yticklabels(list(op_y.keys()))
        ax2.set_title('File Operations Timeline')
        ax2.set_xlabel('Time (s)')
        ax2.grid(True, alpha=0.3)

        # Plot 3: Operation counts
        ax3 = axes[1][0]
        if self.op_counts:
            ops = list(self.op_counts.keys())
            counts = list(self.op_counts.values())
            colors = ['#2ecc71', '#3498db', '#e67e22', '#e74c3c', '#95a5a6']
            bars = ax3.bar(ops, counts, color=colors[:len(ops)], edgecolor='black')
            for bar, count in zip(bars, counts):
                ax3.text(bar.get_x() + bar.get_width() / 2., bar.get_height() + 0.1,
                         str(count), ha='center', va='bottom', fontweight='bold')
        ax3.set_title('Operation Counts')
        ax3.set_ylabel('Count')
        ax3.grid(True, alpha=0.3, axis='y')

        # Plot 4: File sizes
        ax4 = axes[1][1]
        if self.file_sizes:
            names = [n.split('/')[-1] if '/' in n else n for n in self.file_sizes.keys()]
            sizes = list(self.file_sizes.values())
            ax4.barh(names, sizes, color='#9b59b6', edgecolor='black')
            for i, (name, size) in enumerate(zip(names, sizes)):
                ax4.text(size + max(sizes) * 0.02, i, f'{size}B', va='center', fontsize=9)
        ax4.set_title('File Sizes')
        ax4.set_xlabel('Size (bytes)')
        ax4.grid(True, alpha=0.3, axis='x')

        plt.tight_layout()
        plt.savefig('spiffs_debug_analysis.png', dpi=150, bbox_inches='tight')
        print("[INFO] Plot saved to spiffs_debug_analysis.png")
        plt.show()


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    debugger = SPIFFSDebugger()

    if not HAS_SERIAL:
        print("[ERROR] pyserial is required. Install: pip install pyserial")
        print("[INFO] Running in demo mode with sample data...")
        # Demo data
        demo_lines = [
            "Total: 957314 bytes (934 KB)",
            "Used : 0 bytes (0 KB)",
            "Free : 957314 bytes (934 KB)",
            "File '/spiffs/test.txt' written successfully",
            "Write time: 1250 us",
            "File size: 128 bytes",
            "Read complete. Time: 320 us",
            "Appended line 1 to file",
            "Appended line 2 to file",
            "Binary file written: 5 records, time: 890 us",
            "Binary read complete. Time: 450 us",
            "test.txt                       256",
            "data.bin                       184",
            "File deleted successfully!",
            "Total: 957314 bytes (934 KB)",
            "Used : 256 bytes (0 KB)",
            "SPIFFS unregistered successfully",
        ]
        for line in demo_lines:
            debugger.parse_line(line)
            time.sleep(0.05)

        debugger.print_summary()
        debugger.plot_results()
        return

    print(f"[INFO] Connecting to {port} at {baud} baud...")
    print("[INFO] Press Ctrl+C to stop and show results\n")

    try:
        ser = serial.Serial(port, baud, timeout=1)
        while True:
            if ser.in_waiting > 0:
                raw = ser.readline()
                try:
                    line = raw.decode('utf-8', errors='replace').strip()
                except Exception:
                    continue
                if line:
                    print(f"[SERIAL] {line}")
                    debugger.parse_line(line)
    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user.")
    except serial.SerialException as e:
        print(f"[ERROR] Serial error: {e}")
    finally:
        debugger.print_summary()
        debugger.plot_results()


if __name__ == '__main__':
    main()
