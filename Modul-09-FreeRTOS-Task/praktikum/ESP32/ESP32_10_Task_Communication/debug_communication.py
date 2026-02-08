#!/usr/bin/env python3
"""
============================================================================
Debug Parser & Visualizer - ESP32 Task Communication (Race Condition)
============================================================================
Mem-parse output serial [DATA] dari program ESP32_10_Task_Communication
dan menampilkan grafik race condition & data corruption.

Fitur:
  - Grafik corruption rate over time
  - Breakdown jenis korupsi (pie chart)
  - Timeline event korupsi
  - Throughput producer/consumer
  - Counter mismatch distribution

Penggunaan:
  python debug_communication.py --port /dev/ttyUSB0
  python debug_communication.py --file output.log
============================================================================
"""

import argparse
import sys
import re
import time
import threading
from collections import deque, defaultdict

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    from matplotlib.gridspec import GridSpec
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


class CommunicationParser:
    """Parser untuk data serial ESP32 Task Communication"""

    def __init__(self, max_samples=300):
        self.max_samples = max_samples
        # Corruption stats
        self.total_reads = 0
        self.total_corruptions = 0
        self.corruption_rate = 0.0
        self.corruption_history = deque(maxlen=max_samples)
        self.rate_history = deque(maxlen=max_samples)
        self.timestamps = deque(maxlen=max_samples)
        # Corruption breakdown
        self.counter_mismatch = 0
        self.magic_start_corrupt = 0
        self.magic_end_corrupt = 0
        self.checksum_error = 0
        self.sequence_error = 0
        # Corruption events
        self.corruption_events = deque(maxlen=200)
        # Throughput
        self.producer_count = 0
        self.consumer_count = 0
        self.producer_history = deque(maxlen=max_samples)
        self.consumer_history = deque(maxlen=max_samples)
        # Fast producer/consumer
        self.fast_corruptions = 0
        self.fast_reads = 0
        self.fast_rate = 0.0
        # Counter mismatch values
        self.mismatch_deltas = deque(maxlen=100)
        # Heap
        self.heap_free = 0
        self.heap_min = 0
        # Counters
        self.total_lines = 0
        self.data_lines = 0
        self.start_time = time.time()

    def parse_line(self, line):
        """Parse satu baris output serial"""
        self.total_lines += 1
        line = line.strip()

        if not line.startswith("[DATA]"):
            return

        self.data_lines += 1
        content = line[6:]
        elapsed = time.time() - self.start_time

        # RACE_STATS
        m = re.match(r'RACE_STATS,reads=(\d+),corruptions=(\d+),rate=([\d.]+)', content)
        if m:
            self.total_reads = int(m.group(1))
            self.total_corruptions = int(m.group(2))
            self.corruption_rate = float(m.group(3))
            self.corruption_history.append(self.total_corruptions)
            self.rate_history.append(self.corruption_rate)
            self.timestamps.append(elapsed)
            return

        # DETAIL
        m = re.match(r'DETAIL,counter_mismatch=(\d+),magic_start=(\d+),'
                     r'magic_end=(\d+),checksum=(\d+),sequence=(\d+)', content)
        if m:
            self.counter_mismatch = int(m.group(1))
            self.magic_start_corrupt = int(m.group(2))
            self.magic_end_corrupt = int(m.group(3))
            self.checksum_error = int(m.group(4))
            self.sequence_error = int(m.group(5))
            return

        # CORRUPT event
        m = re.match(r'CORRUPT,type=(\w+)', content)
        if m:
            corr_type = m.group(1)
            self.corruption_events.append((elapsed, corr_type))
            if corr_type == 'counter_mismatch':
                m2 = re.search(r'counter=(\d+),copy=(\d+)', content)
                if m2:
                    delta = abs(int(m2.group(1)) - int(m2.group(2)))
                    self.mismatch_deltas.append(delta)
            return

        # CONSUMER
        m = re.match(r'CONSUMER,reads=(\d+),seq=(\d+),corruptions=(\d+),rate=([\d.]+)', content)
        if m:
            self.consumer_count = int(m.group(1))
            self.consumer_history.append(self.consumer_count)
            return

        # PRODUCER
        m = re.match(r'PRODUCER,seq=(\d+),count=(\d+)', content)
        if m:
            self.producer_count = int(m.group(2))
            self.producer_history.append(self.producer_count)
            return

        # THROUGHPUT
        m = re.match(r'THROUGHPUT,producer=(\d+),consumer=(\d+),uptime=(\d+)', content)
        if m:
            self.producer_count = int(m.group(1))
            self.consumer_count = int(m.group(2))
            return

        # FAST_CONS
        m = re.match(r'FAST_CONS,reads=(\d+),corruptions=(\d+),rate=([\d.]+)', content)
        if m:
            self.fast_reads = int(m.group(1))
            self.fast_corruptions = int(m.group(2))
            self.fast_rate = float(m.group(3))
            return

        # HEAP
        m = re.match(r'HEAP,free=(\d+),min=(\d+)', content)
        if m:
            self.heap_free = int(m.group(1))
            self.heap_min = int(m.group(2))
            return


class CommunicationVisualizer:
    """Visualisasi race condition dan data corruption"""

    def __init__(self, parser):
        self.parser = parser
        self.fig = plt.figure(figsize=(16, 10))
        self.fig.suptitle('ESP32 FreeRTOS - Race Condition & Data Corruption Monitor',
                          fontsize=14, fontweight='bold', color='darkred')
        gs = GridSpec(3, 3, figure=self.fig, hspace=0.45, wspace=0.35)

        self.ax_rate = self.fig.add_subplot(gs[0, :2])
        self.ax_pie = self.fig.add_subplot(gs[0, 2])
        self.ax_corr = self.fig.add_subplot(gs[1, 0])
        self.ax_throughput = self.fig.add_subplot(gs[1, 1])
        self.ax_info = self.fig.add_subplot(gs[1, 2])
        self.ax_info.axis('off')
        self.ax_events = self.fig.add_subplot(gs[2, :2])
        self.ax_events.axis('off')
        self.ax_delta = self.fig.add_subplot(gs[2, 2])

    def update(self, frame):
        p = self.parser

        # 1. Corruption rate over time
        self.ax_rate.clear()
        self.ax_rate.set_title('Corruption Rate Over Time (%)')
        self.ax_rate.set_xlabel('Waktu (s)')
        self.ax_rate.set_ylabel('Rate (%)')
        if p.timestamps and p.rate_history:
            ts = list(p.timestamps)
            rates = list(p.rate_history)
            self.ax_rate.plot(ts[:len(rates)], rates, 'r-', linewidth=2, label='Corruption Rate')
            self.ax_rate.fill_between(ts[:len(rates)], rates, alpha=0.2, color='red')
        self.ax_rate.grid(True, alpha=0.3)
        self.ax_rate.legend(fontsize=8)

        # 2. Corruption breakdown pie
        self.ax_pie.clear()
        self.ax_pie.set_title('Jenis Korupsi')
        types = {
            'Counter\nMismatch': p.counter_mismatch,
            'Magic\nStart': p.magic_start_corrupt,
            'Magic\nEnd': p.magic_end_corrupt,
            'Checksum': p.checksum_error,
            'Sequence': p.sequence_error
        }
        non_zero = {k: v for k, v in types.items() if v > 0}
        if non_zero:
            colors = ['#ff6b6b', '#ffd93d', '#4ecdc4', '#6c5ce7', '#fd79a8']
            self.ax_pie.pie(non_zero.values(), labels=non_zero.keys(),
                           colors=colors[:len(non_zero)], autopct='%1.0f%%',
                           textprops={'fontsize': 8})
        else:
            self.ax_pie.text(0.5, 0.5, 'Belum ada\nkorupsi', ha='center',
                            va='center', fontsize=12, color='green')

        # 3. Corruption count bar
        self.ax_corr.clear()
        self.ax_corr.set_title('Jumlah Korupsi per Jenis')
        cats = ['Cntr\nMis', 'Magic\nS', 'Magic\nE', 'Chksum', 'Seq']
        vals = [p.counter_mismatch, p.magic_start_corrupt,
                p.magic_end_corrupt, p.checksum_error, p.sequence_error]
        colors = ['#ff6b6b', '#ffd93d', '#4ecdc4', '#6c5ce7', '#fd79a8']
        bars = self.ax_corr.bar(cats, vals, color=colors)
        for bar, val in zip(bars, vals):
            if val > 0:
                self.ax_corr.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                                 str(val), ha='center', va='bottom', fontsize=8)
        self.ax_corr.tick_params(axis='x', labelsize=7)

        # 4. Throughput
        self.ax_throughput.clear()
        self.ax_throughput.set_title('Throughput')
        cats = ['Producer', 'Consumer', 'Fast\nCorrupt']
        vals = [p.producer_count, p.consumer_count, p.fast_corruptions]
        colors = ['#4ecdc4', '#6c5ce7', '#ff6b6b']
        bars = self.ax_throughput.bar(cats, vals, color=colors)
        for bar, val in zip(bars, vals):
            self.ax_throughput.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                                   f'{val:,}', ha='center', va='bottom', fontsize=8)

        # 5. Info panel
        self.ax_info.clear()
        self.ax_info.axis('off')
        info = (
            f"═══ RACE CONDITION ═══\n"
            f"Total Reads: {p.total_reads:,}\n"
            f"Corruptions: {p.total_corruptions:,}\n"
            f"Rate: {p.corruption_rate:.4f}%\n"
            f"\n═══ FAST CHANNEL ═══\n"
            f"Reads: {p.fast_reads:,}\n"
            f"Corruptions: {p.fast_corruptions:,}\n"
            f"Rate: {p.fast_rate:.2f}%\n"
            f"\n═══ HEAP ═══\n"
            f"Free: {p.heap_free:,} B\n"
            f"\nData lines: {p.data_lines}"
        )
        self.ax_info.text(0.05, 0.95, info, transform=self.ax_info.transAxes,
                         fontsize=9, verticalalignment='top', fontfamily='monospace',
                         bbox=dict(boxstyle='round', facecolor='mistyrose', alpha=0.8))

        # 6. Event log
        self.ax_events.clear()
        self.ax_events.axis('off')
        self.ax_events.set_title('Corruption Events (terbaru)', fontsize=10, loc='left')
        events_text = "Time(s)    Type               \n" + "─" * 35 + "\n"
        recent = list(p.corruption_events)[-12:]
        for t, ctype in recent:
            events_text += f"{t:8.1f}s  ⚠ {ctype}\n"
        if not recent:
            events_text += "  (belum ada korupsi terdeteksi)\n"
        self.ax_events.text(0.02, 0.95, events_text, transform=self.ax_events.transAxes,
                           fontsize=8, verticalalignment='top', fontfamily='monospace',
                           bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

        # 7. Mismatch delta histogram
        self.ax_delta.clear()
        self.ax_delta.set_title('Counter Mismatch Delta')
        if p.mismatch_deltas:
            self.ax_delta.hist(list(p.mismatch_deltas), bins=20, color='#ff6b6b', alpha=0.7)
            self.ax_delta.set_xlabel('Delta')
            self.ax_delta.set_ylabel('Frekuensi')
        else:
            self.ax_delta.text(0.5, 0.5, 'N/A', ha='center', va='center')
        self.ax_delta.grid(True, alpha=0.3)

        return []

    def run(self):
        ani = animation.FuncAnimation(self.fig, self.update, interval=1000, blit=False)
        plt.tight_layout()
        plt.show()


def read_serial(port, baudrate, parser):
    if not HAS_SERIAL:
        print("ERROR: pip install pyserial")
        sys.exit(1)
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Terhubung ke {port} @ {baudrate}")
        while True:
            line = ser.readline().decode('utf-8', errors='ignore')
            if line:
                parser.parse_line(line)
                if '[DATA]CORRUPT' in line or '[DATA]RACE' in line:
                    print(f"  {line.strip()}")
    except serial.SerialException as e:
        print(f"Error serial: {e}")
    except KeyboardInterrupt:
        print("\nDihentikan.")


def read_file(filepath, parser):
    try:
        with open(filepath, 'r') as f:
            for line in f:
                parser.parse_line(line)
        print(f"Selesai: {parser.data_lines} data, {parser.total_corruptions} korupsi")
    except FileNotFoundError:
        print(f"File tidak ditemukan: {filepath}")
        sys.exit(1)


def main():
    ap = argparse.ArgumentParser(description='ESP32 Race Condition Visualizer')
    ap.add_argument('--port', '-p', help='Serial port')
    ap.add_argument('--baud', '-b', type=int, default=115200)
    ap.add_argument('--file', '-f', help='File log')
    ap.add_argument('--no-gui', action='store_true')
    args = ap.parse_args()

    data_parser = CommunicationParser()

    if args.file:
        read_file(args.file, data_parser)
        if not args.no_gui and HAS_MATPLOTLIB:
            viz = CommunicationVisualizer(data_parser)
            viz.update(0)
            plt.tight_layout()
            plt.show()
        else:
            print(f"\nReads: {data_parser.total_reads}, Corruptions: {data_parser.total_corruptions}")
            print(f"Rate: {data_parser.corruption_rate:.4f}%")
    elif args.port:
        if not args.no_gui and HAS_MATPLOTLIB:
            thread = threading.Thread(target=read_serial,
                                     args=(args.port, args.baud, data_parser),
                                     daemon=True)
            thread.start()
            viz = CommunicationVisualizer(data_parser)
            viz.run()
        else:
            read_serial(args.port, args.baud, data_parser)
    else:
        print("Gunakan --port atau --file")
        print("  python debug_communication.py --port /dev/ttyUSB0")


if __name__ == '__main__':
    main()
