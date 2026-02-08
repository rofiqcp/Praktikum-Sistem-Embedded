#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
============================================================
Debug Script: STM32_05_DMA_ADC_Multi_Channel
============================================================
Script Python untuk parsing dan visualisasi data dari
program DMA ADC Multi Channel STM32.

Fitur:
  - Parsing output serial dengan tag [DATA]
  - Plot real-time tegangan kedua channel ADC
  - Grafik statistik (min, max, rata-rata)
  - Histogram distribusi nilai ADC
  - Support input dari serial port atau file log

Penggunaan:
  python debug_adc_multi.py --port /dev/ttyUSB0
  python debug_adc_multi.py --file log.txt
  python debug_adc_multi.py --port COM3 --baud 115200
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
# Format: [DATA] CH0_AVG=xxx CH0_MIN=xxx CH0_MAX=xxx CH0_RAW=xxx
#                CH1_AVG=xxx CH1_MIN=xxx CH1_MAX=xxx CH1_RAW=xxx
#                CYCLES=xxx CNT=xxx
DATA_PATTERN = re.compile(
    r'\[DATA\]\s+'
    r'CH0_AVG=(\d+)\s+CH0_MIN=(\d+)\s+CH0_MAX=(\d+)\s+CH0_RAW=(\d+)\s+'
    r'CH1_AVG=(\d+)\s+CH1_MIN=(\d+)\s+CH1_MAX=(\d+)\s+CH1_RAW=(\d+)\s+'
    r'CYCLES=(\d+)\s+CNT=(\d+)'
)

# Format: [DATA] SAMPLES val0:val1 val2:val3 ...
SAMPLES_PATTERN = re.compile(
    r'\[DATA\]\s+SAMPLES\s+([\d:\s]+)'
)

# Format: [DATA] STATS TRANSFERS=xxx SAMPLES_PER_SEC=xxx ...
STATS_PATTERN = re.compile(
    r'\[DATA\]\s+STATS\s+TRANSFERS=(\d+)\s+SAMPLES_PER_SEC=(\d+)\s+'
    r'DMA_COUNT=(\d+)\s+UPTIME=(\d+)'
)

# Ukuran maksimum data yang disimpan untuk plotting
MAX_DATA_POINTS = 200


class ADCMultiChannelParser:
    """Parser dan visualizer untuk data ADC multi channel."""

    def __init__(self):
        """Inisialisasi buffer data dan statistik."""
        # Buffer data untuk plotting (deque untuk efisiensi)
        self.ch0_avg = deque(maxlen=MAX_DATA_POINTS)
        self.ch1_avg = deque(maxlen=MAX_DATA_POINTS)
        self.ch0_min = deque(maxlen=MAX_DATA_POINTS)
        self.ch0_max = deque(maxlen=MAX_DATA_POINTS)
        self.ch1_min = deque(maxlen=MAX_DATA_POINTS)
        self.ch1_max = deque(maxlen=MAX_DATA_POINTS)
        self.timestamps = deque(maxlen=MAX_DATA_POINTS)
        self.cycles = deque(maxlen=MAX_DATA_POINTS)
        self.throughput = deque(maxlen=MAX_DATA_POINTS)

        # Sampel mentah terakhir
        self.last_samples_ch0 = []
        self.last_samples_ch1 = []

        # Statistik keseluruhan
        self.total_readings = 0
        self.start_time = time.time()

    def parse_line(self, line):
        """
        Parse satu baris output serial.
        
        Args:
            line: String baris dari serial output
            
        Returns:
            dict atau None: Data yang di-parse, atau None jika bukan data
        """
        line = line.strip()
        if not line:
            return None

        # Coba parse data utama ADC
        match = DATA_PATTERN.search(line)
        if match:
            data = {
                'type': 'adc_data',
                'ch0_avg': int(match.group(1)),
                'ch0_min': int(match.group(2)),
                'ch0_max': int(match.group(3)),
                'ch0_raw': int(match.group(4)),
                'ch1_avg': int(match.group(5)),
                'ch1_min': int(match.group(6)),
                'ch1_max': int(match.group(7)),
                'ch1_raw': int(match.group(8)),
                'cycles': int(match.group(9)),
                'count': int(match.group(10)),
                'timestamp': time.time() - self.start_time
            }
            self._update_data(data)
            return data

        # Coba parse data sampel mentah
        match = SAMPLES_PATTERN.search(line)
        if match:
            pairs = match.group(1).strip().split()
            ch0_vals, ch1_vals = [], []
            for pair in pairs:
                parts = pair.split(':')
                if len(parts) == 2:
                    ch0_vals.append(int(parts[0]))
                    ch1_vals.append(int(parts[1]))
            self.last_samples_ch0 = ch0_vals
            self.last_samples_ch1 = ch1_vals
            return {'type': 'samples', 'ch0': ch0_vals, 'ch1': ch1_vals}

        # Coba parse statistik
        match = STATS_PATTERN.search(line)
        if match:
            data = {
                'type': 'stats',
                'transfers': int(match.group(1)),
                'samples_per_sec': int(match.group(2)),
                'dma_count': int(match.group(3)),
                'uptime': int(match.group(4))
            }
            self.throughput.append(data['samples_per_sec'])
            return data

        return None

    def _update_data(self, data):
        """Update buffer data internal dengan data baru."""
        self.ch0_avg.append(data['ch0_avg'])
        self.ch1_avg.append(data['ch1_avg'])
        self.ch0_min.append(data['ch0_min'])
        self.ch0_max.append(data['ch0_max'])
        self.ch1_min.append(data['ch1_min'])
        self.ch1_max.append(data['ch1_max'])
        self.timestamps.append(data['timestamp'])
        self.cycles.append(data['cycles'])
        self.total_readings += 1

    def print_summary(self, data):
        """Cetak ringkasan data ke konsol."""
        if data['type'] == 'adc_data':
            print(f"  CH0: {data['ch0_avg']:4d} mV "
                  f"(min={data['ch0_min']}, max={data['ch0_max']}) "
                  f"raw={data['ch0_raw']}")
            print(f"  CH1: {data['ch1_avg']:4d} mV "
                  f"(min={data['ch1_min']}, max={data['ch1_max']}) "
                  f"raw={data['ch1_raw']}")
            print(f"  Cycles: {data['cycles']}, "
                  f"Count: {data['count']}")
        elif data['type'] == 'stats':
            print(f"  [STATS] Throughput: {data['samples_per_sec']} samples/s, "
                  f"Uptime: {data['uptime']}s")


def create_realtime_plot(parser):
    """
    Buat plot real-time dengan matplotlib animation.
    
    Args:
        parser: Instance ADCMultiChannelParser
        
    Returns:
        tuple: (fig, ani) untuk kontrol animasi
    """
    fig, axes = plt.subplots(2, 2, figsize=(14, 9))
    fig.suptitle('STM32 DMA ADC Multi-Channel Monitor', fontsize=14, fontweight='bold')
    plt.subplots_adjust(hspace=0.35, wspace=0.3)

    def update(frame):
        """Fungsi update untuk animasi matplotlib."""
        for ax in axes.flat:
            ax.clear()

        if len(parser.timestamps) < 2:
            return

        t = list(parser.timestamps)

        # --- Subplot 1: Tegangan kedua channel ---
        ax1 = axes[0, 0]
        ax1.plot(t, list(parser.ch0_avg), 'b-', linewidth=1.5, label='CH0 (PA0)')
        ax1.plot(t, list(parser.ch1_avg), 'r-', linewidth=1.5, label='CH1 (PA1)')
        ax1.fill_between(t, list(parser.ch0_min), list(parser.ch0_max),
                         alpha=0.2, color='blue', label='CH0 range')
        ax1.fill_between(t, list(parser.ch1_min), list(parser.ch1_max),
                         alpha=0.2, color='red', label='CH1 range')
        ax1.set_xlabel('Waktu (s)')
        ax1.set_ylabel('Tegangan (mV)')
        ax1.set_title('Tegangan ADC Real-Time')
        ax1.legend(loc='upper right', fontsize=8)
        ax1.grid(True, alpha=0.3)
        ax1.set_ylim(-100, 3400)

        # --- Subplot 2: Sampel mentah terakhir ---
        ax2 = axes[0, 1]
        if parser.last_samples_ch0:
            x_idx = range(len(parser.last_samples_ch0))
            ax2.bar([x - 0.2 for x in x_idx], parser.last_samples_ch0,
                    width=0.4, color='blue', alpha=0.7, label='CH0')
            ax2.bar([x + 0.2 for x in x_idx], parser.last_samples_ch1,
                    width=0.4, color='red', alpha=0.7, label='CH1')
        ax2.set_xlabel('Index Sampel')
        ax2.set_ylabel('Nilai ADC (raw)')
        ax2.set_title('Sampel Mentah Terakhir')
        ax2.legend(fontsize=8)
        ax2.grid(True, alpha=0.3)

        # --- Subplot 3: Cycle count pemrosesan ---
        ax3 = axes[1, 0]
        if parser.cycles:
            ax3.plot(t, list(parser.cycles), 'g-', linewidth=1.2)
            ax3.axhline(y=np.mean(list(parser.cycles)), color='orange',
                        linestyle='--', label=f'Rata-rata: {np.mean(list(parser.cycles)):.0f}')
        ax3.set_xlabel('Waktu (s)')
        ax3.set_ylabel('CPU Cycles')
        ax3.set_title('Waktu Pemrosesan (DWT Cycles)')
        ax3.legend(fontsize=8)
        ax3.grid(True, alpha=0.3)

        # --- Subplot 4: Throughput ---
        ax4 = axes[1, 1]
        if parser.throughput:
            tp = list(parser.throughput)
            ax4.bar(range(len(tp)), tp, color='purple', alpha=0.7)
            ax4.axhline(y=np.mean(tp), color='orange', linestyle='--',
                        label=f'Rata-rata: {np.mean(tp):.0f} sps')
        ax4.set_xlabel('Interval')
        ax4.set_ylabel('Samples/sec')
        ax4.set_title('Throughput DMA ADC')
        ax4.legend(fontsize=8)
        ax4.grid(True, alpha=0.3)

    return fig, update


def read_serial(port, baud, parser):
    """
    Baca data dari serial port secara real-time.
    
    Args:
        port: Nama serial port (e.g., /dev/ttyUSB0)
        baud: Baud rate
        parser: Instance ADCMultiChannelParser
    """
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
    """
    Baca dan parse data dari file log.
    
    Args:
        filepath: Path ke file log
        parser: Instance ADCMultiChannelParser
    """
    print(f"[INFO] Membaca file: {filepath}")

    with open(filepath, 'r') as f:
        for line in f:
            data = parser.parse_line(line)
            if data:
                parser.print_summary(data)

    print(f"\n[INFO] Total pembacaan: {parser.total_readings}")

    if parser.total_readings == 0:
        print("[WARN] Tidak ada data [DATA] ditemukan dalam file.")
        return

    # Buat plot statis dari semua data
    fig, update_func = create_realtime_plot(parser)
    update_func(0)
    plt.tight_layout()
    
    # Simpan grafik
    output_file = filepath.replace('.txt', '_plot.png').replace('.log', '_plot.png')
    if output_file == filepath:
        output_file = filepath + '_plot.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"[INFO] Grafik disimpan: {output_file}")
    plt.show()


def main():
    """Fungsi utama program."""
    arg_parser = argparse.ArgumentParser(
        description='Debug Parser untuk STM32 DMA ADC Multi-Channel',
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

    parser = ADCMultiChannelParser()

    if args.file:
        read_file(args.file, parser)
    elif args.port:
        read_serial(args.port, args.baud, parser)


if __name__ == '__main__':
    main()
