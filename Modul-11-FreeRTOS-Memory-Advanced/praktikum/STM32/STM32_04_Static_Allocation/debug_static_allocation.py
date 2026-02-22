#!/usr/bin/env python3
"""
debug_static_allocation.py
==========================
Verifies that no dynamic allocation occurs after init for STM32_04_Static_Allocation.
Monitors producer/consumer throughput, queue status, and heap invariance.

Usage:
    python debug_static_allocation.py --port /dev/ttyUSB0
    python debug_static_allocation.py --port COM3 --plot
    python debug_static_allocation.py --port /dev/ttyUSB0 --csv static_log.csv
"""

import argparse
import csv
import re
import sys
import time
from datetime import datetime
from collections import deque

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("ERROR: pyserial not installed. Run: pip install pyserial")
    sys.exit(1)

try:
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    print("WARNING: matplotlib not installed. Plotting disabled.")
    HAS_MATPLOTLIB = False


# ── Regex patterns ─────────────────────────────────────────────────────────
RE_STATUS_COUNTS = re.compile(
    r'\[STATUS\]\s+Cycle=(\d+)\s*\|\s*Produced=(\d+)\s*\|\s*Consumed=(\d+)'
)
RE_STATUS_HEAP = re.compile(
    r'\[STATUS\]\s+Free heap:\s*(\d+)\s*\|\s*MinEver:\s*(\d+)\s*\|\s*AfterInit:\s*(\d+)'
)
RE_STATUS_DYNAMIC = re.compile(
    r'\[STATUS\]\s+(OK: No dynamic allocation after init|WARNING: Dynamic allocation detected!(?:\s*Lost\s*(\d+)\s*bytes)?)'
)
RE_STATUS_WATERMARKS = re.compile(
    r'\[STATUS\]\s+Watermarks:\s*Producer=(\d+)\s*Consumer=(\d+)\s*Status=(\d+)\s*LED=(\d+)'
)
RE_STATUS_QUEUE = re.compile(
    r'\[STATUS\]\s+Queue:\s*(\d+)/(\d+)\s*items\s*\|\s*Free=(\d+)\s*slots'
)
RE_PRODUCER = re.compile(
    r'\[PRODUCER\]\s+Sent item #(\d+):\s*value=(\d+),\s*tick=(\d+)'
)
RE_CONSUMER = re.compile(
    r'\[CONSUMER\]\s+Recv item #(\d+):\s*value=(\d+),\s*latency=(\d+)\s*ms'
)
RE_INIT_HEAP = re.compile(
    r'\[INIT\]\s+Free heap after all static init:\s*(\d+)\s*bytes'
)


class StaticAllocMonitor:
    """Monitors static allocation demo for dynamic alloc violations."""

    def __init__(self, csv_path="static_allocation_log.csv", max_points=500):
        self.csv_path = csv_path
        self.max_points = max_points

        # Heap tracking
        self.heap_after_init = None
        self.heap_timestamps = deque(maxlen=max_points)
        self.heap_free = deque(maxlen=max_points)
        self.heap_min_ever = deque(maxlen=max_points)
        self.dynamic_violations = []

        # Task watermarks
        self.watermark_timestamps = deque(maxlen=max_points)
        self.watermark_producer = deque(maxlen=max_points)
        self.watermark_consumer = deque(maxlen=max_points)
        self.watermark_status = deque(maxlen=max_points)
        self.watermark_led = deque(maxlen=max_points)

        # Queue status
        self.queue_timestamps = deque(maxlen=max_points)
        self.queue_items = deque(maxlen=max_points)
        self.queue_capacity = 8

        # Producer / Consumer throughput
        self.produced_timestamps = deque(maxlen=max_points)
        self.produced_counts = deque(maxlen=max_points)
        self.consumed_timestamps = deque(maxlen=max_points)
        self.consumed_counts = deque(maxlen=max_points)

        # Latency
        self.latencies = deque(maxlen=max_points)
        self.latency_timestamps = deque(maxlen=max_points)

        self.start_time = time.time()
        self._init_csv()

    def _init_csv(self):
        with open(self.csv_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                'timestamp', 'elapsed_s', 'event', 'cycle', 'produced',
                'consumed', 'free_heap', 'min_ever', 'after_init',
                'dynamic_ok', 'wm_producer', 'wm_consumer', 'wm_status',
                'wm_led', 'queue_items', 'queue_cap', 'latency_ms', 'detail'
            ])
        print(f"[CSV] Logging to {self.csv_path}")

    def _log(self, **kwargs):
        elapsed = time.time() - self.start_time
        now_str = datetime.now().strftime('%H:%M:%S.%f')[:-3]
        row = [
            now_str, f"{elapsed:.2f}",
            kwargs.get('event', ''),
            kwargs.get('cycle', ''),
            kwargs.get('produced', ''),
            kwargs.get('consumed', ''),
            kwargs.get('free_heap', ''),
            kwargs.get('min_ever', ''),
            kwargs.get('after_init', ''),
            kwargs.get('dynamic_ok', ''),
            kwargs.get('wm_producer', ''),
            kwargs.get('wm_consumer', ''),
            kwargs.get('wm_status', ''),
            kwargs.get('wm_led', ''),
            kwargs.get('queue_items', ''),
            kwargs.get('queue_cap', ''),
            kwargs.get('latency_ms', ''),
            kwargs.get('detail', ''),
        ]
        with open(self.csv_path, 'a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(row)

    def process_line(self, line):
        """Parse a serial line."""
        elapsed = time.time() - self.start_time

        # ── Init heap ──
        m = RE_INIT_HEAP.search(line)
        if m:
            self.heap_after_init = int(m.group(1))
            self._log(event='INIT_HEAP', after_init=self.heap_after_init)
            print(f"  >>> Heap after init: {self.heap_after_init} bytes")
            return

        # ── Status counts ──
        m = RE_STATUS_COUNTS.search(line)
        if m:
            cycle, produced, consumed = int(m.group(1)), int(m.group(2)), int(m.group(3))
            self.produced_timestamps.append(elapsed)
            self.produced_counts.append(produced)
            self.consumed_timestamps.append(elapsed)
            self.consumed_counts.append(consumed)
            self._log(event='STATUS', cycle=cycle, produced=produced, consumed=consumed)
            return

        # ── Status heap ──
        m = RE_STATUS_HEAP.search(line)
        if m:
            free_h, min_ev, after_init = int(m.group(1)), int(m.group(2)), int(m.group(3))
            self.heap_timestamps.append(elapsed)
            self.heap_free.append(free_h)
            self.heap_min_ever.append(min_ev)
            self._log(event='HEAP', free_heap=free_h, min_ever=min_ev, after_init=after_init)
            return

        # ── Dynamic alloc check ──
        m = RE_STATUS_DYNAMIC.search(line)
        if m:
            ok = 'OK' in m.group(1)
            self._log(event='DYNAMIC_CHECK', dynamic_ok='OK' if ok else 'VIOLATION')
            if not ok:
                lost = m.group(2)
                self.dynamic_violations.append((elapsed, lost))
                print(f"\n  !!! DYNAMIC ALLOCATION VIOLATION at {elapsed:.2f}s !!!")
                if lost:
                    print(f"  !!! Lost {lost} bytes !!!\n")
            return

        # ── Watermarks ──
        m = RE_STATUS_WATERMARKS.search(line)
        if m:
            wp, wc, ws, wl = int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4))
            self.watermark_timestamps.append(elapsed)
            self.watermark_producer.append(wp)
            self.watermark_consumer.append(wc)
            self.watermark_status.append(ws)
            self.watermark_led.append(wl)
            self._log(event='WATERMARKS', wm_producer=wp, wm_consumer=wc,
                      wm_status=ws, wm_led=wl)
            return

        # ── Queue status ──
        m = RE_STATUS_QUEUE.search(line)
        if m:
            items, cap, free_slots = int(m.group(1)), int(m.group(2)), int(m.group(3))
            self.queue_timestamps.append(elapsed)
            self.queue_items.append(items)
            self.queue_capacity = cap
            self._log(event='QUEUE', queue_items=items, queue_cap=cap)
            return

        # ── Consumer latency ──
        m = RE_CONSUMER.search(line)
        if m:
            latency = int(m.group(3))
            self.latency_timestamps.append(elapsed)
            self.latencies.append(latency)
            self._log(event='CONSUMER', latency_ms=latency,
                      detail=f"item={m.group(1)} value={m.group(2)}")
            return

        # ── Producer event ──
        m = RE_PRODUCER.search(line)
        if m:
            self._log(event='PRODUCER',
                      detail=f"item={m.group(1)} value={m.group(2)} tick={m.group(3)}")
            return

    def print_summary(self):
        """Print a text summary."""
        print("\n" + "=" * 60)
        print("  Static Allocation Verification Summary")
        print("=" * 60)

        print(f"\n  Heap after init    : {self.heap_after_init or 'N/A'} bytes")
        if self.heap_free:
            print(f"  Current free heap  : {list(self.heap_free)[-1]} bytes")
            print(f"  Min ever free      : {min(self.heap_min_ever)} bytes")
            if self.heap_after_init:
                diff = self.heap_after_init - list(self.heap_free)[-1]
                if diff == 0:
                    print(f"  Dynamic alloc check: PASS (no change)")
                else:
                    print(f"  Dynamic alloc check: FAIL (lost {diff} bytes)")

        print(f"\n  Dynamic violations : {len(self.dynamic_violations)}")
        for t, lost in self.dynamic_violations:
            print(f"    [{t:.2f}s] Lost {lost} bytes")

        if self.produced_counts:
            print(f"\n  Items produced     : {list(self.produced_counts)[-1]}")
        if self.consumed_counts:
            print(f"  Items consumed     : {list(self.consumed_counts)[-1]}")

        if self.latencies:
            lats = list(self.latencies)
            print(f"\n  Latency stats:")
            print(f"    Min   : {min(lats)} ms")
            print(f"    Max   : {max(lats)} ms")
            print(f"    Avg   : {sum(lats) / len(lats):.1f} ms")

        if self.watermark_producer:
            print(f"\n  Stack watermarks (last reading):")
            print(f"    Producer : {list(self.watermark_producer)[-1]} words")
            print(f"    Consumer : {list(self.watermark_consumer)[-1]} words")
            print(f"    Status   : {list(self.watermark_status)[-1]} words")
            print(f"    LED      : {list(self.watermark_led)[-1]} words")

        verdict = "PASS" if len(self.dynamic_violations) == 0 else "FAIL"
        print(f"\n  === VERDICT: {verdict} ===")
        print("=" * 60)

    def plot_summary(self):
        """Generate visualisation."""
        if not HAS_MATPLOTLIB:
            return

        fig, axes = plt.subplots(4, 1, figsize=(14, 16))
        fig.suptitle('Static Allocation Verification – STM32_04', fontsize=14)

        # Plot 1: Heap free over time (should be constant)
        ax = axes[0]
        if self.heap_timestamps:
            t = list(self.heap_timestamps)
            free = list(self.heap_free)
            ax.plot(t, free, 'g-o', markersize=3, label='Free heap')
            if self.heap_after_init:
                ax.axhline(y=self.heap_after_init, color='blue',
                           linestyle='--', linewidth=2,
                           label=f'After init ({self.heap_after_init})')
            # Mark violations
            for vt, lost in self.dynamic_violations:
                ax.axvline(x=vt, color='red', linewidth=2, alpha=0.7)
        ax.set_ylabel('Bytes')
        ax.set_title('Heap Free (should remain constant)')
        ax.legend()
        ax.grid(True, alpha=0.3)

        # Plot 2: Stack watermarks
        ax = axes[1]
        if self.watermark_timestamps:
            t = list(self.watermark_timestamps)
            ax.plot(t, list(self.watermark_producer), '-o', markersize=3,
                    label='Producer')
            ax.plot(t, list(self.watermark_consumer), '-s', markersize=3,
                    label='Consumer')
            ax.plot(t, list(self.watermark_status), '-^', markersize=3,
                    label='Status')
            ax.plot(t, list(self.watermark_led), '-d', markersize=3,
                    label='LED')
        ax.set_ylabel('Watermark (words)')
        ax.set_title('Task Stack Watermarks')
        ax.legend()
        ax.grid(True, alpha=0.3)

        # Plot 3: Queue utilisation
        ax = axes[2]
        if self.queue_timestamps:
            t = list(self.queue_timestamps)
            items = list(self.queue_items)
            ax.fill_between(t, 0, items, alpha=0.5, color='#3498db')
            ax.plot(t, items, 'b-o', markersize=3, label='Items in queue')
            ax.axhline(y=self.queue_capacity, color='red', linestyle='--',
                       label=f'Capacity ({self.queue_capacity})')
        ax.set_ylabel('Items')
        ax.set_title('Queue Utilisation')
        ax.legend()
        ax.grid(True, alpha=0.3)

        # Plot 4: Consumer latency
        ax = axes[3]
        if self.latency_timestamps:
            t = list(self.latency_timestamps)
            lats = list(self.latencies)
            ax.plot(t, lats, 'r-o', markersize=3)
            avg_lat = sum(lats) / len(lats)
            ax.axhline(y=avg_lat, color='blue', linestyle='--',
                       label=f'Average ({avg_lat:.1f} ms)')
        ax.set_ylabel('Latency (ms)')
        ax.set_xlabel('Time (s)')
        ax.set_title('Producer → Consumer Latency')
        ax.legend()
        ax.grid(True, alpha=0.3)

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
    parser = argparse.ArgumentParser(
        description='FreeRTOS Static Allocation Verification Tool')
    parser.add_argument('--port', type=str, default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--csv', type=str, default='static_allocation_log.csv',
                        help='CSV log file')
    parser.add_argument('--duration', type=int, default=0,
                        help='Run duration in seconds (0=forever)')
    parser.add_argument('--plot', action='store_true',
                        help='Show plot after capture')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    if port is None:
        print("ERROR: No serial port found. Use --port to specify.")
        sys.exit(1)

    monitor = StaticAllocMonitor(csv_path=args.csv)

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
                monitor.process_line(line)

    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user.")
    finally:
        ser.close()

    monitor.print_summary()

    if args.plot:
        monitor.plot_summary()


if __name__ == '__main__':
    main()
