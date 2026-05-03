#!/usr/bin/env python3
"""
Debug Flash Key-Value Store - STM32_09_Flash_Key_Value
Parse KV operations, track entries, and visualize flash page usage.

Usage:
    python debug_flash_kv.py /dev/ttyUSB0 115200
    python debug_flash_kv.py  (uses saved log file)
"""

import sys
import re
import time
import os
from collections import OrderedDict
from datetime import datetime

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    from matplotlib.gridspec import GridSpec
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed. Install with: pip install matplotlib")


class KVEntry:
    """Represents a single key-value entry."""
    def __init__(self, key, value, vtype, timestamp=None):
        self.key = key
        self.value = value
        self.vtype = vtype
        self.timestamp = timestamp or time.time()
        self.deleted = False


class FlashKVDebugger:
    """Parse and visualize flash key-value store operations."""

    def __init__(self):
        self.entries = OrderedDict()
        self.operations = []  # (timestamp, op_type, key, value)
        self.flash_writes = 0
        self.compactions = 0
        self.page_usage = {"active": 0, "max_per_page": 12}
        self.errors = []
        self.raw_lines = []
        self.active_page = 0x0800F800
        self.backup_page = 0x0800FC00

    def parse_line(self, line):
        """Parse a single log line."""
        line = line.strip()
        if not line:
            return
        self.raw_lines.append(line)
        ts = time.time()

        # KV SET int
        m = re.match(r"\[KV\] SET int '(\w+)' = (-?\d+)", line)
        if m:
            key, val = m.group(1), int(m.group(2))
            self.entries[key] = KVEntry(key, val, "INT", ts)
            self.operations.append((ts, "SET_INT", key, val))
            self.flash_writes += 1
            return

        # KV SET float
        m = re.match(r"\[KV\] SET float '(\w+)' = (-?[\d.]+)", line)
        if m:
            key, val = m.group(1), float(m.group(2))
            self.entries[key] = KVEntry(key, val, "FLOAT", ts)
            self.operations.append((ts, "SET_FLOAT", key, val))
            self.flash_writes += 1
            return

        # KV SET string
        m = re.match(r'\[KV\] SET string \'(\w+)\' = "(.+)"', line)
        if m:
            key, val = m.group(1), m.group(2)
            self.entries[key] = KVEntry(key, val, "STRING", ts)
            self.operations.append((ts, "SET_STRING", key, val))
            self.flash_writes += 1
            return

        # KV GET
        m = re.match(r"\[KV\] GET (\w+) '(\w+)' = (.+)", line)
        if m:
            vtype, key, val = m.group(1), m.group(2), m.group(3)
            self.operations.append((ts, f"GET_{vtype.upper()}", key, val))
            return

        # KV DELETE
        m = re.match(r"\[KV\] DELETE '(\w+)'", line)
        if m:
            key = m.group(1)
            if key in self.entries:
                self.entries[key].deleted = True
            self.operations.append((ts, "DELETE", key, None))
            self.flash_writes += 1
            return

        # Compaction
        if "Compaction done" in line:
            self.compactions += 1
            self.operations.append((ts, "COMPACT", "", None))
            return

        # Page usage
        m = re.match(r"\[KV\] Flash writes\s*:\s*(\d+)\s*/\s*(\d+)", line)
        if m:
            self.page_usage["active"] = int(m.group(1))
            self.page_usage["max_per_page"] = int(m.group(2))
            return

        # Errors
        if "ERROR" in line:
            self.errors.append((ts, line))

    def parse_stream(self, port, baudrate=115200):
        """Read from serial port and parse in real-time."""
        if not HAS_SERIAL:
            print("[ERROR] pyserial not installed. Install with: pip install pyserial")
            return

        print(f"[INFO] Connecting to {port} at {baudrate} baud...")
        ser = serial.Serial(port, baudrate, timeout=1)
        print("[INFO] Connected. Press Ctrl+C to stop.\n")

        try:
            while True:
                line = ser.readline().decode('utf-8', errors='replace')
                if line:
                    print(line, end='')
                    self.parse_line(line)
        except KeyboardInterrupt:
            print("\n[INFO] Stopped by user.")
        finally:
            ser.close()

    def parse_file(self, filename):
        """Parse a saved log file."""
        with open(filename, 'r') as f:
            for line in f:
                self.parse_line(line)
        print(f"[INFO] Parsed {len(self.raw_lines)} lines from {filename}")

    def print_summary(self):
        """Print text summary of KV store state."""
        print("\n" + "=" * 60)
        print("  Flash Key-Value Store Debug Summary")
        print("=" * 60)

        print(f"\n  Active entries  : {sum(1 for e in self.entries.values() if not e.deleted)}")
        print(f"  Deleted entries : {sum(1 for e in self.entries.values() if e.deleted)}")
        print(f"  Flash writes    : {self.flash_writes}")
        print(f"  Compactions     : {self.compactions}")
        print(f"  Errors          : {len(self.errors)}")
        print(f"  Total ops       : {len(self.operations)}")

        print(f"\n  Page usage      : {self.page_usage['active']}/{self.page_usage['max_per_page']}")

        print("\n  Current Entries:")
        print(f"  {'Key':<16} {'Type':<8} {'Value':<30} {'Status'}")
        print(f"  {'-'*16} {'-'*8} {'-'*30} {'-'*8}")
        for key, entry in self.entries.items():
            status = "DELETED" if entry.deleted else "ACTIVE"
            val_str = str(entry.value)[:28]
            print(f"  {key:<16} {entry.vtype:<8} {val_str:<30} {status}")

        if self.errors:
            print("\n  Errors:")
            for ts, err in self.errors:
                print(f"    {err}")

    def plot(self):
        """Generate visualization plots."""
        if not HAS_MATPLOTLIB:
            print("[WARN] Cannot plot without matplotlib")
            return

        fig = plt.figure(figsize=(16, 10))
        fig.suptitle("STM32 Flash Key-Value Store Analysis", fontsize=14, fontweight='bold')
        gs = GridSpec(2, 3, figure=fig, hspace=0.35, wspace=0.3)

        # 1. Flash page usage gauge
        ax1 = fig.add_subplot(gs[0, 0])
        used = self.page_usage.get("active", 0)
        total = self.page_usage.get("max_per_page", 12)
        colors = ['#4CAF50' if i < used else '#E0E0E0' for i in range(total)]
        bars = ax1.barh(range(total), [1]*total, color=colors, edgecolor='#333', linewidth=0.5)
        ax1.set_yticks(range(total))
        ax1.set_yticklabels([f"Slot {i}" for i in range(total)], fontsize=7)
        ax1.set_title(f"Flash Page Usage ({used}/{total})", fontsize=10)
        ax1.set_xlim(0, 1.2)
        ax1.set_xticks([])

        # 2. Operation timeline
        ax2 = fig.add_subplot(gs[0, 1:])
        if self.operations:
            op_types = [op[1] for op in self.operations]
            op_colors_map = {
                "SET_INT": "#2196F3", "SET_FLOAT": "#FF9800", "SET_STRING": "#4CAF50",
                "GET_INT": "#90CAF9", "GET_FLOAT": "#FFCC80", "GET_STRING": "#A5D6A7",
                "DELETE": "#F44336", "COMPACT": "#9C27B0"
            }
            x_vals = range(len(op_types))
            colors = [op_colors_map.get(t, "#999") for t in op_types]
            ax2.bar(x_vals, [1]*len(op_types), color=colors, width=0.8)
            ax2.set_title("Operation Timeline", fontsize=10)
            ax2.set_xlabel("Operation #")
            ax2.set_yticks([])

            handles = [mpatches.Patch(color=c, label=k) for k, c in op_colors_map.items()
                       if k in set(op_types)]
            ax2.legend(handles=handles, loc='upper right', fontsize=7, ncol=2)

        # 3. Entry types distribution
        ax3 = fig.add_subplot(gs[1, 0])
        type_counts = {}
        for e in self.entries.values():
            if not e.deleted:
                type_counts[e.vtype] = type_counts.get(e.vtype, 0) + 1
        if type_counts:
            labels = list(type_counts.keys())
            sizes = list(type_counts.values())
            colors_pie = ['#2196F3', '#FF9800', '#4CAF50', '#9C27B0']
            ax3.pie(sizes, labels=labels, colors=colors_pie[:len(labels)],
                    autopct='%1.0f%%', startangle=90)
        ax3.set_title("Entry Types", fontsize=10)

        # 4. Write activity over time
        ax4 = fig.add_subplot(gs[1, 1])
        write_ops = [op for op in self.operations if op[1].startswith("SET")]
        if write_ops:
            # Group by batches of 5
            batch_size = max(1, len(write_ops) // 20)
            batches = [len(write_ops[i:i+batch_size]) for i in range(0, len(write_ops), batch_size)]
            ax4.bar(range(len(batches)), batches, color='#2196F3', alpha=0.7)
            ax4.set_xlabel("Time Window")
            ax4.set_ylabel("Writes")
        ax4.set_title("Write Activity", fontsize=10)

        # 5. Compaction and wear info
        ax5 = fig.add_subplot(gs[1, 2])
        stats = {
            'Writes': self.flash_writes,
            'Compactions': self.compactions,
            'Active': sum(1 for e in self.entries.values() if not e.deleted),
            'Deleted': sum(1 for e in self.entries.values() if e.deleted),
            'Errors': len(self.errors)
        }
        bars = ax5.barh(list(stats.keys()), list(stats.values()),
                        color=['#2196F3', '#9C27B0', '#4CAF50', '#FF9800', '#F44336'])
        ax5.set_title("Store Statistics", fontsize=10)
        for bar, val in zip(bars, stats.values()):
            ax5.text(bar.get_width() + 0.3, bar.get_y() + bar.get_height()/2,
                     str(val), va='center', fontsize=9)

        plt.savefig("flash_kv_debug.png", dpi=150, bbox_inches='tight')
        print("[INFO] Plot saved to flash_kv_debug.png")
        plt.show()


def generate_sample_data():
    """Generate sample log data for testing."""
    return """[KV] Initializing key-value store...
[KV] Active page: 0x0800F800
[KV] Backup page: 0x0800FC00
[KV] Entry size : 84 bytes
[KV] Entries/page: 12
[KV] Found 0 valid entries, write offset=0
[KV] SET int 'boot_count' = 1
[KV] SET int 'last_boot_ms' = 15
[KV] SET string 'dev_name' = "BluePill-01"
[KV] SET string 'wifi_ssid' = "LabIoT_5G"
[KV] SET string 'wifi_pass' = "embedded2025"
[KV] GET string 'dev_name' = "BluePill-01"
[KV] GET string 'wifi_ssid' = "LabIoT_5G"
[KV] SET float 'cal_offset' = -2.3500
[KV] SET float 'cal_gain' = 1.0247
[KV] SET int 'adc_ref_mv' = 3300
[KV] SET float 'cal_temp' = 25.0000
[KV] DELETE 'wifi_pass'
[KV] Flash writes   : 10 / 12
[KV] SET int 'counter' = 0
[KV] SET int 'counter' = 1
[KV] Active page full, compacting...
[KV] Compaction done. New active page: 0x0800FC00, entries: 8
"""


if __name__ == "__main__":
    debugger = FlashKVDebugger()

    if len(sys.argv) >= 2:
        port_or_file = sys.argv[1]
        if os.path.isfile(port_or_file):
            debugger.parse_file(port_or_file)
        elif HAS_SERIAL:
            baudrate = int(sys.argv[2]) if len(sys.argv) >= 3 else 115200
            debugger.parse_stream(port_or_file, baudrate)
        else:
            print(f"[ERROR] Cannot open {port_or_file}")
            sys.exit(1)
    else:
        print("[INFO] No input specified. Using sample data for demo.")
        for line in generate_sample_data().strip().split('\n'):
            debugger.parse_line(line)

    debugger.print_summary()

    if HAS_MATPLOTLIB:
        debugger.plot()
