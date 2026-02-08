#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
============================================================
Debug Script: STM32_06_DMA_SPI_Transfer
============================================================
Script Python untuk parsing dan visualisasi data perbandingan
kecepatan SPI Polling vs SPI DMA pada STM32.

Fitur:
  - Parsing output serial dengan tag [DATA]
  - Grafik perbandingan waktu polling vs DMA
  - Grafik throughput (KB/s)
  - Grafik speedup factor per ukuran transfer
  - Support input dari serial port atau file log

Penggunaan:
  python debug_spi_transfer.py --port /dev/ttyUSB0
  python debug_spi_transfer.py --file log.txt
============================================================
"""

import argparse
import re
import sys
import time
from collections import defaultdict
from datetime import datetime

import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np


# ============================================================
# Pola Regex untuk Parsing Data Serial
# ============================================================
# Format: [DATA] SIZE=xxx POLL_CYC=xxx DMA_CYC=xxx POLL_US=xxx DMA_US=xxx SPEEDUP=xxx MATCH=x
SPEED_PATTERN = re.compile(
    r'\[DATA\]\s+SIZE=(\d+)\s+POLL_CYC=(\d+)\s+DMA_CYC=(\d+)\s+'
    r'POLL_US=([\d.]+)\s+DMA_US=([\d.]+)\s+SPEEDUP=([\d.]+)\s+MATCH=(\d+)'
)

# Format: [DATA] THROUGHPUT SIZE=xxx POLL_KBPS=xxx DMA_KBPS=xxx
THROUGHPUT_PATTERN = re.compile(
    r'\[DATA\]\s+THROUGHPUT\s+SIZE=(\d+)\s+POLL_KBPS=([\d.]+)\s+DMA_KBPS=([\d.]+)'
)

# Format: [DATA] SUMMARY ROUND=xxx
SUMMARY_PATTERN = re.compile(
    r'\[DATA\]\s+SUMMARY\s+ROUND=(\d+)'
)


class SPITransferParser:
    """Parser dan visualizer untuk data SPI transfer."""

    def __init__(self):
        """Inisialisasi buffer data."""
        # Data per ukuran transfer
        self.speed_data = defaultdict(list)     # size -> list of dicts
        self.throughput_data = defaultdict(list) # size -> list of dicts
        self.rounds = []
        self.total_tests = 0

    def parse_line(self, line):
        """
        Parse satu baris output serial.
        
        Args:
            line: String baris dari serial output
            
        Returns:
            dict atau None
        """
        line = line.strip()
        if not line:
            return None

        # Parse data kecepatan
        match = SPEED_PATTERN.search(line)
        if match:
            data = {
                'type': 'speed',
                'size': int(match.group(1)),
                'poll_cycles': int(match.group(2)),
                'dma_cycles': int(match.group(3)),
                'poll_us': float(match.group(4)),
                'dma_us': float(match.group(5)),
                'speedup': float(match.group(6)),
                'match': int(match.group(7))
            }
            self.speed_data[data['size']].append(data)
            self.total_tests += 1
            return data

        # Parse data throughput
        match = THROUGHPUT_PATTERN.search(line)
        if match:
            data = {
                'type': 'throughput',
                'size': int(match.group(1)),
                'poll_kbps': float(match.group(2)),
                'dma_kbps': float(match.group(3))
            }
            self.throughput_data[data['size']].append(data)
            return data

        # Parse ringkasan ronde
        match = SUMMARY_PATTERN.search(line)
        if match:
            data = {
                'type': 'summary',
                'round': int(match.group(1))
            }
            self.rounds.append(data['round'])
            return data

        return None

    def print_summary(self, data):
        """Cetak ringkasan data ke konsol."""
        if data['type'] == 'speed':
            status = "OK" if data['match'] else "FAIL"
            print(f"  Size={data['size']:4d}B  "
                  f"Poll={data['poll_us']:8.1f}us  "
                  f"DMA={data['dma_us']:8.1f}us  "
                  f"Speedup={data['speedup']:.2f}x  [{status}]")
        elif data['type'] == 'throughput':
            print(f"  Size={data['size']:4d}B  "
                  f"Poll={data['poll_kbps']:.1f} KB/s  "
                  f"DMA={data['dma_kbps']:.1f} KB/s")
        elif data['type'] == 'summary':
            print(f"\n  === Ronde #{data['round']} selesai ===")


def create_plot(parser):
    """
    Buat plot perbandingan SPI polling vs DMA.
    
    Args:
        parser: Instance SPITransferParser
    """
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('STM32 SPI DMA vs Polling - Performance Comparison',
                 fontsize=14, fontweight='bold')
    plt.subplots_adjust(hspace=0.35, wspace=0.3)

    sizes = sorted(parser.speed_data.keys())
    if not sizes:
        print("[WARN] Tidak ada data speed untuk diplot.")
        return fig

    # Hitung rata-rata per ukuran
    avg_poll_us = []
    avg_dma_us = []
    avg_speedup = []
    avg_poll_kbps = []
    avg_dma_kbps = []

    for size in sizes:
        entries = parser.speed_data[size]
        avg_poll_us.append(np.mean([e['poll_us'] for e in entries]))
        avg_dma_us.append(np.mean([e['dma_us'] for e in entries]))
        avg_speedup.append(np.mean([e['speedup'] for e in entries]))

        if size in parser.throughput_data:
            tp = parser.throughput_data[size]
            avg_poll_kbps.append(np.mean([e['poll_kbps'] for e in tp]))
            avg_dma_kbps.append(np.mean([e['dma_kbps'] for e in tp]))
        else:
            avg_poll_kbps.append(0)
            avg_dma_kbps.append(0)

    x = np.arange(len(sizes))
    width = 0.35
    size_labels = [f"{s}B" for s in sizes]

    # --- Subplot 1: Waktu transfer (us) ---
    ax1 = axes[0, 0]
    bars1 = ax1.bar(x - width/2, avg_poll_us, width, label='Polling', color='#e74c3c', alpha=0.8)
    bars2 = ax1.bar(x + width/2, avg_dma_us, width, label='DMA', color='#2ecc71', alpha=0.8)
    ax1.set_xlabel('Ukuran Transfer')
    ax1.set_ylabel('Waktu (μs)')
    ax1.set_title('Waktu Transfer: Polling vs DMA')
    ax1.set_xticks(x)
    ax1.set_xticklabels(size_labels)
    ax1.legend()
    ax1.grid(True, alpha=0.3, axis='y')

    # --- Subplot 2: Speedup factor ---
    ax2 = axes[0, 1]
    colors = ['#3498db' if s >= 1.0 else '#e74c3c' for s in avg_speedup]
    bars = ax2.bar(x, avg_speedup, color=colors, alpha=0.8)
    ax2.axhline(y=1.0, color='gray', linestyle='--', linewidth=1, label='Break-even')
    ax2.set_xlabel('Ukuran Transfer')
    ax2.set_ylabel('Speedup (x)')
    ax2.set_title('DMA Speedup Factor')
    ax2.set_xticks(x)
    ax2.set_xticklabels(size_labels)
    ax2.legend()
    ax2.grid(True, alpha=0.3, axis='y')
    # Tambahkan label angka di atas bar
    for bar, val in zip(bars, avg_speedup):
        ax2.text(bar.get_x() + bar.get_width()/2., bar.get_height() + 0.02,
                 f'{val:.2f}x', ha='center', va='bottom', fontsize=9)

    # --- Subplot 3: Throughput (KB/s) ---
    ax3 = axes[1, 0]
    if avg_poll_kbps:
        ax3.bar(x - width/2, avg_poll_kbps, width, label='Polling', color='#e74c3c', alpha=0.8)
        ax3.bar(x + width/2, avg_dma_kbps, width, label='DMA', color='#2ecc71', alpha=0.8)
    ax3.set_xlabel('Ukuran Transfer')
    ax3.set_ylabel('Throughput (KB/s)')
    ax3.set_title('Throughput Perbandingan')
    ax3.set_xticks(x)
    ax3.set_xticklabels(size_labels)
    ax3.legend()
    ax3.grid(True, alpha=0.3, axis='y')

    # --- Subplot 4: CPU Cycles ---
    ax4 = axes[1, 1]
    avg_poll_cyc = [np.mean([e['poll_cycles'] for e in parser.speed_data[s]]) for s in sizes]
    avg_dma_cyc = [np.mean([e['dma_cycles'] for e in parser.speed_data[s]]) for s in sizes]
    ax4.plot(sizes, avg_poll_cyc, 'r-o', linewidth=2, markersize=8, label='Polling')
    ax4.plot(sizes, avg_dma_cyc, 'g-s', linewidth=2, markersize=8, label='DMA')
    ax4.set_xlabel('Ukuran Transfer (bytes)')
    ax4.set_ylabel('CPU Cycles')
    ax4.set_title('CPU Cycles vs Ukuran Data')
    ax4.legend()
    ax4.grid(True, alpha=0.3)

    return fig


def read_serial(port, baud, parser):
    """Baca data dari serial port."""
    import serial

    print(f"[INFO] Membuka serial port {port} @ {baud} baud...")
    ser = serial.Serial(port, baud, timeout=1)
    print(f"[INFO] Serial port terbuka. Menunggu data...\n")

    try:
        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='replace')
                data = parser.parse_line(line)
                if data:
                    parser.print_summary(data)
                    # Update plot setelah setiap ronde
                    if data['type'] == 'summary':
                        fig = create_plot(parser)
                        plt.draw()
                        plt.pause(0.1)
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")
    finally:
        ser.close()
        if parser.total_tests > 0:
            fig = create_plot(parser)
            plt.tight_layout()
            plt.show()


def read_file(filepath, parser):
    """Baca dan parse data dari file log."""
    print(f"[INFO] Membaca file: {filepath}")

    with open(filepath, 'r') as f:
        for line in f:
            data = parser.parse_line(line)
            if data:
                parser.print_summary(data)

    print(f"\n[INFO] Total test: {parser.total_tests}")

    if parser.total_tests == 0:
        print("[WARN] Tidak ada data [DATA] ditemukan.")
        return

    fig = create_plot(parser)
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
        description='Debug Parser untuk STM32 DMA SPI Transfer',
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

    parser = SPITransferParser()

    if args.file:
        read_file(args.file, parser)
    elif args.port:
        read_serial(args.port, args.baud, parser)


if __name__ == '__main__':
    main()
