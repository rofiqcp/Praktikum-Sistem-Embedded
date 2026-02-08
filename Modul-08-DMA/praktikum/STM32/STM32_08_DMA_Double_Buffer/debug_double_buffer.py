#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
============================================================
Debug Script: STM32_08_DMA_Double_Buffer
============================================================
Script Python untuk parsing dan visualisasi data dari
program DMA Software Double Buffer (Ping-Pong) pada STM32.

Fitur:
  - Parsing output serial dengan tag [DATA]
  - Visualisasi ping-pong antara halaman A dan B
  - Grafik tegangan per halaman
  - Statistik pemrosesan (cycles, overrun)
  - Grafik throughput samples/sec

Penggunaan:
  python debug_double_buffer.py --port /dev/ttyUSB0
  python debug_double_buffer.py --file log.txt
============================================================
"""

import argparse
import re
import sys
import time
from collections import deque
from datetime import datetime

import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np


# ============================================================
# Pola Regex untuk Parsing Data Serial
# ============================================================
# Format: [DATA] PAGE=x AVG_MV=xx MIN_RAW=xx MAX_RAW=xx AVG_RAW=xx
#         VOLTAGE=xx CYCLES=xx HALF_CNT=xx FULL_CNT=xx TOTAL=xx OVERRUN=xx
PAGE_PATTERN = re.compile(
    r'\[DATA\]\s+PAGE=(\w)\s+AVG_MV=(\d+)\s+MIN_RAW=(\d+)\s+MAX_RAW=(\d+)\s+'
    r'AVG_RAW=(\d+)\s+VOLTAGE=(\d+)\s+CYCLES=(\d+)\s+'
    r'HALF_CNT=(\d+)\s+FULL_CNT=(\d+)\s+TOTAL=(\d+)\s+OVERRUN=(\d+)'
)

# Format: [DATA] SAMPLES_A val val val ...
# Format: [DATA] SAMPLES_B val val val ...
SAMPLES_PATTERN = re.compile(
    r'\[DATA\]\s+SAMPLES_([AB])\s+([\d\s.]+)'
)

# Format: [DATA] STATS TOTAL_PROC=xx PAGE_A=xx PAGE_B=xx OVERRUN=xx
#         AVG_CYC_A=xx AVG_CYC_B=xx SPS=xx UPTIME=xx
STATS_PATTERN = re.compile(
    r'\[DATA\]\s+STATS\s+TOTAL_PROC=(\d+)\s+PAGE_A=(\d+)\s+PAGE_B=(\d+)\s+'
    r'OVERRUN=(\d+)\s+AVG_CYC_A=(\d+)\s+AVG_CYC_B=(\d+)\s+'
    r'SPS=(\d+)\s+UPTIME=(\d+)'
)

MAX_DATA_POINTS = 300


class DoubleBufferParser:
    """Parser dan visualizer untuk data double buffer."""

    def __init__(self):
        """Inisialisasi buffer data."""
        self.timestamps = deque(maxlen=MAX_DATA_POINTS)

        # Data per halaman
        self.page_a_voltage = deque(maxlen=MAX_DATA_POINTS)
        self.page_b_voltage = deque(maxlen=MAX_DATA_POINTS)
        self.page_a_cycles = deque(maxlen=MAX_DATA_POINTS)
        self.page_b_cycles = deque(maxlen=MAX_DATA_POINTS)
        self.page_a_min = deque(maxlen=MAX_DATA_POINTS)
        self.page_a_max = deque(maxlen=MAX_DATA_POINTS)
        self.page_b_min = deque(maxlen=MAX_DATA_POINTS)
        self.page_b_max = deque(maxlen=MAX_DATA_POINTS)

        # Statistik
        self.overrun_history = deque(maxlen=MAX_DATA_POINTS)
        self.sps_history = deque(maxlen=MAX_DATA_POINTS)
        self.page_sequence = deque(maxlen=MAX_DATA_POINTS)  # A/B sequence

        # Sampel mentah terakhir
        self.last_samples_a = []
        self.last_samples_b = []

        self.total_pages = 0
        self.start_time = time.time()

    def parse_line(self, line):
        """Parse satu baris output serial."""
        line = line.strip()
        if not line:
            return None

        # Parse data halaman
        match = PAGE_PATTERN.search(line)
        if match:
            data = {
                'type': 'page',
                'page': match.group(1),
                'avg_mv': int(match.group(2)),
                'min_raw': int(match.group(3)),
                'max_raw': int(match.group(4)),
                'avg_raw': int(match.group(5)),
                'voltage': int(match.group(6)),
                'cycles': int(match.group(7)),
                'half_cnt': int(match.group(8)),
                'full_cnt': int(match.group(9)),
                'total': int(match.group(10)),
                'overrun': int(match.group(11)),
                'timestamp': time.time() - self.start_time
            }
            self._update_page_data(data)
            return data

        # Parse sampel mentah
        match = SAMPLES_PATTERN.search(line)
        if match:
            page = match.group(1)
            vals_str = match.group(2).replace('...', '').strip()
            vals = [int(v) for v in vals_str.split() if v.isdigit()]
            if page == 'A':
                self.last_samples_a = vals
            else:
                self.last_samples_b = vals
            return {'type': 'samples', 'page': page, 'values': vals}

        # Parse statistik
        match = STATS_PATTERN.search(line)
        if match:
            data = {
                'type': 'stats',
                'total_proc': int(match.group(1)),
                'page_a_count': int(match.group(2)),
                'page_b_count': int(match.group(3)),
                'overrun': int(match.group(4)),
                'avg_cyc_a': int(match.group(5)),
                'avg_cyc_b': int(match.group(6)),
                'sps': int(match.group(7)),
                'uptime': int(match.group(8))
            }
            self.sps_history.append(data['sps'])
            self.overrun_history.append(data['overrun'])
            return data

        return None

    def _update_page_data(self, data):
        """Update buffer internal dengan data halaman."""
        self.timestamps.append(data['timestamp'])
        self.total_pages += 1

        if data['page'] == 'A':
            self.page_a_voltage.append(data['avg_mv'])
            self.page_a_cycles.append(data['cycles'])
            self.page_a_min.append(data['min_raw'])
            self.page_a_max.append(data['max_raw'])
            self.page_sequence.append(0)
        else:
            self.page_b_voltage.append(data['avg_mv'])
            self.page_b_cycles.append(data['cycles'])
            self.page_b_min.append(data['min_raw'])
            self.page_b_max.append(data['max_raw'])
            self.page_sequence.append(1)

    def print_summary(self, data):
        """Cetak ringkasan ke konsol."""
        if data['type'] == 'page':
            print(f"  Page {data['page']}: {data['avg_mv']:4d} mV "
                  f"(raw: {data['avg_raw']}, "
                  f"min={data['min_raw']}, max={data['max_raw']}) "
                  f"[{data['cycles']} cycles]")
        elif data['type'] == 'stats':
            print(f"  [STATS] Total={data['total_proc']} "
                  f"A={data['page_a_count']} B={data['page_b_count']} "
                  f"Overrun={data['overrun']} "
                  f"SPS={data['sps']}")


def create_realtime_plot(parser):
    """Buat plot real-time untuk double buffer monitor."""
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('STM32 DMA Double Buffer (Ping-Pong) Monitor',
                 fontsize=14, fontweight='bold')
    plt.subplots_adjust(hspace=0.35, wspace=0.3)

    def update(frame):
        for ax in axes.flat:
            ax.clear()

        # --- Subplot 1: Tegangan per halaman ---
        ax1 = axes[0, 0]
        if parser.page_a_voltage:
            a_v = list(parser.page_a_voltage)
            a_idx = list(range(len(a_v)))
            ax1.plot(a_idx, a_v, 'b-o', linewidth=1.2, markersize=3,
                     label='Page A (First Half)', alpha=0.8)
        if parser.page_b_voltage:
            b_v = list(parser.page_b_voltage)
            b_idx = list(range(len(b_v)))
            ax1.plot(b_idx, b_v, 'r-s', linewidth=1.2, markersize=3,
                     label='Page B (Second Half)', alpha=0.8)
        ax1.set_xlabel('Sample Index')
        ax1.set_ylabel('Tegangan (mV)')
        ax1.set_title('Tegangan ADC per Halaman Buffer')
        ax1.legend(loc='upper right', fontsize=8)
        ax1.grid(True, alpha=0.3)
        ax1.set_ylim(-100, 3400)

        # --- Subplot 2: Ping-Pong Sequence ---
        ax2 = axes[0, 1]
        if parser.page_sequence:
            seq = list(parser.page_sequence)
            colors = ['#3498db' if s == 0 else '#e74c3c' for s in seq]
            ax2.bar(range(len(seq)), [1]*len(seq), color=colors, alpha=0.7)
            # Legend manual
            from matplotlib.patches import Patch
            legend_elements = [Patch(facecolor='#3498db', alpha=0.7, label='Page A'),
                               Patch(facecolor='#e74c3c', alpha=0.7, label='Page B')]
            ax2.legend(handles=legend_elements, loc='upper right', fontsize=8)
        ax2.set_xlabel('Urutan Pemrosesan')
        ax2.set_ylabel('Halaman')
        ax2.set_title('Pola Ping-Pong (A-B-A-B...)')
        ax2.set_yticks([0, 0.5, 1])
        ax2.set_yticklabels(['', '', ''])
        ax2.grid(True, alpha=0.3, axis='x')

        # --- Subplot 3: Processing Cycles ---
        ax3 = axes[1, 0]
        if parser.page_a_cycles:
            ac = list(parser.page_a_cycles)
            ax3.plot(ac, 'b-', linewidth=1.0, alpha=0.7, label='Page A')
        if parser.page_b_cycles:
            bc = list(parser.page_b_cycles)
            ax3.plot(bc, 'r-', linewidth=1.0, alpha=0.7, label='Page B')
        all_cycles = list(parser.page_a_cycles) + list(parser.page_b_cycles)
        if all_cycles:
            ax3.axhline(y=np.mean(all_cycles), color='orange', linestyle='--',
                        label=f'Rata-rata: {np.mean(all_cycles):.0f}')
        ax3.set_xlabel('Index')
        ax3.set_ylabel('CPU Cycles')
        ax3.set_title('Waktu Pemrosesan per Halaman')
        ax3.legend(fontsize=8)
        ax3.grid(True, alpha=0.3)

        # --- Subplot 4: Throughput & Overrun ---
        ax4 = axes[1, 1]
        if parser.sps_history:
            sps = list(parser.sps_history)
            ax4.bar(range(len(sps)), sps, color='#2ecc71', alpha=0.7,
                    label='Samples/sec')
            ax4.axhline(y=np.mean(sps), color='orange', linestyle='--',
                        label=f'Rata-rata: {np.mean(sps):.0f} SPS')
        ax4.set_xlabel('Interval')
        ax4.set_ylabel('Samples/sec')
        ax4.set_title('Throughput & Overrun')
        ax4.legend(fontsize=8)
        ax4.grid(True, alpha=0.3)

        # Tambahkan info overrun di pojok
        if parser.overrun_history:
            last_overrun = list(parser.overrun_history)[-1]
            ax4.text(0.98, 0.95, f'Overrun: {last_overrun}',
                     transform=ax4.transAxes, ha='right', va='top',
                     fontsize=10, color='red' if last_overrun > 0 else 'green',
                     bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

    return fig, update


def read_serial(port, baud, parser):
    """Baca data dari serial port."""
    import serial

    print(f"[INFO] Membuka serial port {port} @ {baud} baud...")
    ser = serial.Serial(port, baud, timeout=1)
    print(f"[INFO] Serial port terbuka. Menunggu data...\n")

    fig, update_func = create_realtime_plot(parser)
    ani = animation.FuncAnimation(fig, update_func, interval=500, cache_frame_data=False)
    plt.ion()
    plt.show()

    try:
        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='replace')
                data = parser.parse_line(line)
                if data:
                    parser.print_summary(data)
            plt.pause(0.01)
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")
    finally:
        ser.close()
        plt.ioff()
        plt.show()


def read_file(filepath, parser):
    """Baca dan parse data dari file log."""
    print(f"[INFO] Membaca file: {filepath}")

    with open(filepath, 'r') as f:
        for line in f:
            data = parser.parse_line(line)
            if data:
                parser.print_summary(data)

    print(f"\n[INFO] Total halaman diproses: {parser.total_pages}")

    if parser.total_pages == 0:
        print("[WARN] Tidak ada data [DATA] ditemukan.")
        return

    fig, update_func = create_realtime_plot(parser)
    update_func(0)
    plt.tight_layout()

    output_file = filepath.replace('.txt', '_plot.png').replace('.log', '_plot.png')
    if output_file == filepath:
        output_file = filepath + '_plot.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"[INFO] Grafik disimpan: {output_file}")
    plt.show()


def main():
    """Fungsi utama program."""
    arg_parser = argparse.ArgumentParser(
        description='Debug Parser untuk STM32 DMA Double Buffer',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Contoh penggunaan:
  %(prog)s --port /dev/ttyUSB0
  %(prog)s --port COM3 --baud 115200
  %(prog)s --file capture.log
        """
    )
    arg_parser.add_argument('--port', '-p', type=str,
                            help='Serial port (e.g., /dev/ttyUSB0, COM3)')
    arg_parser.add_argument('--baud', '-b', type=int, default=115200,
                            help='Baud rate (default: 115200)')
    arg_parser.add_argument('--file', '-f', type=str,
                            help='File log untuk di-parse')

    args = arg_parser.parse_args()

    if not args.port and not args.file:
        arg_parser.print_help()
        print("\n[ERROR] Harus menyediakan --port atau --file")
        sys.exit(1)

    parser = DoubleBufferParser()

    if args.file:
        read_file(args.file, parser)
    elif args.port:
        read_serial(args.port, args.baud, parser)


if __name__ == '__main__':
    main()
