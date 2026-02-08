#!/usr/bin/env python3
"""
debug_memory_allocation.py
==========================
Parses serial output from STM32_02_Memory_Allocation.
Visualises memory blocks, allocation patterns, and fragmentation.

Usage:
    python debug_memory_allocation.py --port /dev/ttyUSB0
    python debug_memory_allocation.py --port COM3 --plot
    python debug_memory_allocation.py --port /dev/ttyUSB0 --csv mem_log.csv
"""

import argparse
import csv
import re
import sys
import time
from datetime import datetime
from collections import OrderedDict

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("ERROR: pyserial not installed. Run: pip install pyserial")
    sys.exit(1)

try:
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    from matplotlib.collections import PatchCollection
    HAS_MATPLOTLIB = True
except ImportError:
    print("WARNING: matplotlib not installed. Plotting disabled.")
    HAS_MATPLOTLIB = False


# ── Regex patterns ─────────────────────────────────────────────────────────
RE_HEAP_STATUS = re.compile(
    r'\[(\w[\w-]*)\]\s+Free=(\d+)\s*\|\s*Used=(\d+)\s*\|\s*MinEver=(\d+)'
)
RE_ALLOC = re.compile(
    r'\[([\w-]+)\]\s+Alloc\s+(\d+):\s*(\d+)\s*bytes\s*@\s*(0x[0-9A-Fa-f]+)\s*\|\s*Free=(\d+)'
)
RE_FREE = re.compile(
    r'\[([\w-]+)\]\s+Free\s+(\d+):\s*(\d+)\s*bytes\s*@\s*(0x[0-9A-Fa-f]+)'
)
RE_FRAG_BLOCK = re.compile(
    r'\[FRAG\]\s+Block\s+(\d+)\s*@\s*(0x[0-9A-Fa-f]+)\s*\|\s*Free=(\d+)'
)
RE_FRAG_FREE = re.compile(
    r'\[FRAG\]\s+Free block\s+(\d+)\s*@\s*(0x[0-9A-Fa-f]+)'
)
RE_LARGE_ALLOC = re.compile(
    r'\[FRAG\]\s+(?:Large alloc (SUCCESS|FAILED))'
)


class MemoryBlock:
    """Represents a single allocated memory block."""
    def __init__(self, idx, size, addr, alloc_time, tag):
        self.idx = idx
        self.size = size
        self.addr = addr
        self.alloc_time = alloc_time
        self.free_time = None
        self.tag = tag


class MemoryTracker:
    """Tracks all allocation/free events and heap status."""

    def __init__(self, csv_path="memory_allocation_log.csv"):
        self.csv_path = csv_path
        self.blocks = OrderedDict()  # addr -> MemoryBlock
        self.events = []             # chronological event list
        self.heap_snapshots = []     # (time, free, used, min_ever, tag)
        self.frag_results = []

        self.start_time = time.time()
        self._init_csv()

    def _init_csv(self):
        with open(self.csv_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                'timestamp', 'elapsed_s', 'event', 'tag', 'index',
                'size_bytes', 'address', 'free_heap', 'detail'
            ])
        print(f"[CSV] Logging to {self.csv_path}")

    def _log(self, event, tag, idx, size, addr, free_heap, detail=''):
        elapsed = time.time() - self.start_time
        now_str = datetime.now().strftime('%H:%M:%S.%f')[:-3]
        with open(self.csv_path, 'a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                now_str, f"{elapsed:.2f}", event, tag, idx,
                size, addr, free_heap, detail
            ])

    def process_line(self, line):
        """Parse serial line and track memory."""
        elapsed = time.time() - self.start_time

        # ── Heap status ──
        m = RE_HEAP_STATUS.search(line)
        if m:
            tag = m.group(1)
            free_h, used_h, min_ev = int(m.group(2)), int(m.group(3)), int(m.group(4))
            self.heap_snapshots.append((elapsed, free_h, used_h, min_ev, tag))
            self._log('HEAP', tag, '', '', '', free_h, f"used={used_h} min={min_ev}")
            return

        # ── Allocation event ──
        m = RE_ALLOC.search(line)
        if m:
            tag, idx, size, addr, free_h = m.group(1), int(m.group(2)), int(m.group(3)), m.group(4), int(m.group(5))
            blk = MemoryBlock(idx, size, addr, elapsed, tag)
            self.blocks[addr] = blk
            self.events.append(('ALLOC', elapsed, tag, idx, size, addr, free_h))
            self._log('ALLOC', tag, idx, size, addr, free_h)
            return

        # ── Free event ──
        m = RE_FREE.search(line)
        if m:
            tag, idx, size, addr = m.group(1), int(m.group(2)), int(m.group(3)), m.group(4)
            if addr in self.blocks:
                self.blocks[addr].free_time = elapsed
            self.events.append(('FREE', elapsed, tag, idx, size, addr, 0))
            self._log('FREE', tag, idx, size, addr, '')
            return

        # ── Fragmentation test results ──
        m = RE_LARGE_ALLOC.search(line)
        if m:
            result = m.group(1)
            self.frag_results.append((elapsed, result))
            self._log('FRAG', 'FRAG', '', '', '', '', f"large_alloc={result}")
            return

    def print_summary(self):
        """Print a text summary of memory tracking."""
        print("\n" + "=" * 60)
        print("  Memory Allocation Analysis Summary")
        print("=" * 60)

        total_allocs = sum(1 for e in self.events if e[0] == 'ALLOC')
        total_frees = sum(1 for e in self.events if e[0] == 'FREE')
        total_alloc_bytes = sum(e[4] for e in self.events if e[0] == 'ALLOC')
        total_free_bytes = sum(e[4] for e in self.events if e[0] == 'FREE')

        print(f"  Total allocations : {total_allocs}")
        print(f"  Total frees       : {total_frees}")
        print(f"  Bytes allocated   : {total_alloc_bytes}")
        print(f"  Bytes freed       : {total_free_bytes}")
        print(f"  Heap snapshots    : {len(self.heap_snapshots)}")
        print(f"  Frag test results : {len(self.frag_results)}")

        if self.heap_snapshots:
            free_vals = [s[1] for s in self.heap_snapshots]
            print(f"  Min free heap     : {min(free_vals)} bytes")
            print(f"  Max free heap     : {max(free_vals)} bytes")

        # Allocation sizes by tag
        tags = {}
        for e in self.events:
            if e[0] == 'ALLOC':
                tag = e[2]
                if tag not in tags:
                    tags[tag] = {'count': 0, 'bytes': 0, 'sizes': []}
                tags[tag]['count'] += 1
                tags[tag]['bytes'] += e[4]
                tags[tag]['sizes'].append(e[4])

        if tags:
            print("\n  By allocation pattern:")
            for tag, info in tags.items():
                sizes = info['sizes']
                print(f"    [{tag}] {info['count']} allocs, "
                      f"{info['bytes']} bytes total, "
                      f"sizes: {min(sizes)}-{max(sizes)}")

        print("=" * 60)

    def plot_summary(self):
        """Generate visualisation plots."""
        if not HAS_MATPLOTLIB:
            print("[PLOT] matplotlib not available.")
            return

        fig, axes = plt.subplots(3, 1, figsize=(14, 12))
        fig.suptitle('Memory Allocation Analysis – STM32_02', fontsize=14)

        # Plot 1: Heap free over time
        ax = axes[0]
        if self.heap_snapshots:
            t = [s[0] for s in self.heap_snapshots]
            free = [s[1] for s in self.heap_snapshots]
            used = [s[2] for s in self.heap_snapshots]
            ax.fill_between(t, 0, used, alpha=0.4, color='red', label='Used')
            ax.fill_between(t, used, [u + f for u, f in zip(used, free)],
                            alpha=0.4, color='green', label='Free')
            ax.set_ylabel('Bytes')
            ax.set_title('Heap Usage Over Time')
            ax.legend()
        ax.grid(True, alpha=0.3)

        # Plot 2: Allocation / free events as scatter
        ax = axes[1]
        alloc_t = [e[1] for e in self.events if e[0] == 'ALLOC']
        alloc_s = [e[4] for e in self.events if e[0] == 'ALLOC']
        free_t = [e[1] for e in self.events if e[0] == 'FREE']
        free_s = [e[4] for e in self.events if e[0] == 'FREE']
        if alloc_t:
            ax.scatter(alloc_t, alloc_s, c='red', marker='^', s=60,
                       label='Alloc', alpha=0.7, zorder=5)
        if free_t:
            ax.scatter(free_t, free_s, c='green', marker='v', s=60,
                       label='Free', alpha=0.7, zorder=5)
        ax.set_ylabel('Block Size (bytes)')
        ax.set_title('Allocation & Free Events')
        ax.legend()
        ax.grid(True, alpha=0.3)

        # Plot 3: Memory block lifetime (Gantt-like)
        ax = axes[2]
        blocks_with_life = [(addr, blk) for addr, blk in self.blocks.items()
                            if blk.free_time is not None]
        colors = {'SEQ': '#e74c3c', 'SEQ-FWD': '#3498db', 'RND': '#2ecc71',
                  'FRAG': '#f39c12'}
        y = 0
        for addr, blk in blocks_with_life:
            c = colors.get(blk.tag, '#95a5a6')
            ax.barh(y, blk.free_time - blk.alloc_time,
                    left=blk.alloc_time, height=0.6,
                    color=c, alpha=0.7, edgecolor='black', linewidth=0.5)
            ax.text(blk.alloc_time, y, f" {blk.size}B",
                    va='center', fontsize=7)
            y += 1

        ax.set_ylabel('Block #')
        ax.set_xlabel('Time (s)')
        ax.set_title('Block Lifetime (alloc → free)')
        # Legend
        legend_patches = [mpatches.Patch(color=c, label=t) for t, c in colors.items()]
        ax.legend(handles=legend_patches, loc='upper right', fontsize=8)
        ax.grid(True, alpha=0.3, axis='x')

        plt.tight_layout()
        png_path = self.csv_path.replace('.csv', '.png')
        plt.savefig(png_path, dpi=150)
        print(f"[PLOT] Saved to {png_path}")
        plt.show()


def auto_detect_port():
    ports = serial.tools.list_ports.comports()
    for p in ports:
        desc = (p.description or '').lower()
        if any(k in desc for k in ['usb', 'uart', 'serial', 'ch340', 'cp210', 'ftdi']):
            return p.device
    if ports:
        return ports[0].device
    return None


def main():
    parser = argparse.ArgumentParser(description='FreeRTOS Memory Allocation Debug Tool')
    parser.add_argument('--port', type=str, default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--csv', type=str, default='memory_allocation_log.csv',
                        help='CSV log file')
    parser.add_argument('--duration', type=int, default=0,
                        help='Run duration in seconds (0=forever)')
    parser.add_argument('--plot', action='store_true', help='Show plot after capture')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    if port is None:
        print("ERROR: No serial port found. Use --port to specify.")
        sys.exit(1)

    tracker = MemoryTracker(csv_path=args.csv)

    print(f"[SERIAL] Opening {port} @ {args.baud} baud")
    try:
        ser = serial.Serial(port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"ERROR: Cannot open {port}: {e}")
        sys.exit(1)

    print("[SERIAL] Connected. Press Ctrl+C to stop.\n")
    start = time.time()

    try:
        while True:
            if args.duration > 0 and (time.time() - start) > args.duration:
                print(f"\n[INFO] Duration {args.duration}s reached.")
                break

            raw = ser.readline()
            if not raw:
                continue

            try:
                line = raw.decode('utf-8', errors='replace').strip()
            except Exception:
                continue

            if line:
                print(line)
                tracker.process_line(line)

    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user.")
    finally:
        ser.close()

    tracker.print_summary()

    if args.plot:
        tracker.plot_summary()


if __name__ == '__main__':
    main()
