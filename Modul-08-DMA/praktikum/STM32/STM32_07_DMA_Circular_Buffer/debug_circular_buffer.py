#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
============================================================
Debug Script: STM32_07_DMA_Circular_Buffer
============================================================
Script Python untuk parsing dan visualisasi data dari
program DMA Circular Buffer UART pada STM32.

Fitur:
  - Parsing output serial dengan tag [DATA]
  - Grafik real-time buffer usage (head/tail pointer)
  - Throughput bytes/sec
  - Visualisasi circular buffer state
  - Interactive mode: kirim data dan lihat respons echo

Penggunaan:
  python debug_circular_buffer.py --port /dev/ttyUSB0
  python debug_circular_buffer.py --file log.txt
  python debug_circular_buffer.py --port COM3 --interactive
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
# Format: [DATA] RX_LEN=xx TOTAL_BYTES=xx MSG_COUNT=xx HEAD=xx TAIL=xx AVAIL=xx
RX_DATA_PATTERN = re.compile(
    r'\[DATA\]\s+RX_LEN=(\d+)\s+TOTAL_BYTES=(\d+)\s+MSG_COUNT=(\d+)\s+'
    r'HEAD=(\d+)\s+TAIL=(\d+)\s+AVAIL=(\d+)'
)

# Format: [DATA] HEX xx xx xx ...
HEX_PATTERN = re.compile(
    r'\[DATA\]\s+HEX\s+((?:[0-9A-Fa-f]{2}\s?)+)'
)

# Format: [DATA] STATS TOTAL_BYTES=xx TOTAL_MSG=xx HEAD=xx TAIL=xx AVAIL=xx USAGE=xx% OVERRUN=xx UPTIME=xx
STATS_PATTERN = re.compile(
    r'\[DATA\]\s+STATS\s+TOTAL_BYTES=(\d+)\s+TOTAL_MSG=(\d+)\s+'
    r'HEAD=(\d+)\s+TAIL=(\d+)\s+AVAIL=(\d+)\s+USAGE=(\d+)%\s+'
    r'OVERRUN=(\d+)\s+UPTIME=(\d+)'
)

# Echo response
ECHO_PATTERN = re.compile(r'\[ECHO\]\s+(.*)')

MAX_DATA_POINTS = 300
BUFFER_SIZE = 256  # Harus sesuai dengan RX_BUFFER_SIZE di config.h


class CircularBufferParser:
    """Parser dan visualizer untuk data circular buffer."""

    def __init__(self):
        """Inisialisasi buffer data."""
        self.timestamps = deque(maxlen=MAX_DATA_POINTS)
        self.head_pos = deque(maxlen=MAX_DATA_POINTS)
        self.tail_pos = deque(maxlen=MAX_DATA_POINTS)
        self.available = deque(maxlen=MAX_DATA_POINTS)
        self.usage_pct = deque(maxlen=MAX_DATA_POINTS)
        self.rx_lengths = deque(maxlen=MAX_DATA_POINTS)
        self.total_bytes_history = deque(maxlen=MAX_DATA_POINTS)
        self.throughput = deque(maxlen=MAX_DATA_POINTS)

        self.last_total_bytes = 0
        self.last_throughput_time = time.time()
        self.total_messages = 0
        self.echo_messages = []
        self.start_time = time.time()

    def parse_line(self, line):
        """Parse satu baris output serial."""
        line = line.strip()
        if not line:
            return None

        # Parse RX data event
        match = RX_DATA_PATTERN.search(line)
        if match:
            data = {
                'type': 'rx_data',
                'rx_len': int(match.group(1)),
                'total_bytes': int(match.group(2)),
                'msg_count': int(match.group(3)),
                'head': int(match.group(4)),
                'tail': int(match.group(5)),
                'avail': int(match.group(6)),
                'timestamp': time.time() - self.start_time
            }
            self._update_rx_data(data)
            return data

        # Parse stats
        match = STATS_PATTERN.search(line)
        if match:
            data = {
                'type': 'stats',
                'total_bytes': int(match.group(1)),
                'total_msg': int(match.group(2)),
                'head': int(match.group(3)),
                'tail': int(match.group(4)),
                'avail': int(match.group(5)),
                'usage': int(match.group(6)),
                'overrun': int(match.group(7)),
                'uptime': int(match.group(8))
            }
            self._update_stats(data)
            return data

        # Parse hex dump
        match = HEX_PATTERN.search(line)
        if match:
            hex_str = match.group(1).strip()
            hex_bytes = [int(h, 16) for h in hex_str.split()]
            return {'type': 'hex', 'bytes': hex_bytes}

        # Parse echo
        match = ECHO_PATTERN.search(line)
        if match:
            msg = match.group(1)
            self.echo_messages.append(msg)
            return {'type': 'echo', 'message': msg}

        return None

    def _update_rx_data(self, data):
        """Update buffer data dengan event RX."""
        self.timestamps.append(data['timestamp'])
        self.head_pos.append(data['head'])
        self.tail_pos.append(data['tail'])
        self.available.append(data['avail'])
        self.rx_lengths.append(data['rx_len'])
        self.total_bytes_history.append(data['total_bytes'])
        self.total_messages = data['msg_count']

        # Hitung throughput
        now = time.time()
        dt = now - self.last_throughput_time
        if dt >= 1.0:
            bps = (data['total_bytes'] - self.last_total_bytes) / dt
            self.throughput.append(bps)
            self.last_total_bytes = data['total_bytes']
            self.last_throughput_time = now

    def _update_stats(self, data):
        """Update dari stats event."""
        t = time.time() - self.start_time
        self.timestamps.append(t)
        self.head_pos.append(data['head'])
        self.tail_pos.append(data['tail'])
        self.available.append(data['avail'])
        self.usage_pct.append(data['usage'])

    def print_summary(self, data):
        """Cetak ringkasan ke konsol."""
        if data['type'] == 'rx_data':
            print(f"  RX: {data['rx_len']} bytes | "
                  f"Total: {data['total_bytes']} | "
                  f"Head={data['head']} Tail={data['tail']} "
                  f"Avail={data['avail']}")
        elif data['type'] == 'stats':
            print(f"  [STATS] Bytes={data['total_bytes']} "
                  f"Msg={data['total_msg']} "
                  f"Usage={data['usage']}% "
                  f"Overrun={data['overrun']}")
        elif data['type'] == 'echo':
            print(f"  Echo: {data['message']}")


def create_realtime_plot(parser):
    """Buat plot real-time untuk circular buffer monitor."""
    fig, axes = plt.subplots(2, 2, figsize=(14, 9))
    fig.suptitle('STM32 DMA Circular Buffer Monitor', fontsize=14, fontweight='bold')
    plt.subplots_adjust(hspace=0.35, wspace=0.3)

    def update(frame):
        for ax in axes.flat:
            ax.clear()

        if len(parser.timestamps) < 2:
            return

        t = list(parser.timestamps)

        # --- Subplot 1: Head/Tail Position ---
        ax1 = axes[0, 0]
        ax1.plot(t, list(parser.head_pos), 'b-', linewidth=1.5, label='Head (DMA write)')
        ax1.plot(t, list(parser.tail_pos), 'r-', linewidth=1.5, label='Tail (SW read)')
        ax1.axhline(y=BUFFER_SIZE, color='gray', linestyle='--', alpha=0.5,
                     label=f'Buffer size ({BUFFER_SIZE})')
        ax1.set_xlabel('Waktu (s)')
        ax1.set_ylabel('Posisi (byte)')
        ax1.set_title('Head/Tail Pointer Position')
        ax1.legend(loc='upper right', fontsize=8)
        ax1.grid(True, alpha=0.3)
        ax1.set_ylim(-10, BUFFER_SIZE + 20)

        # --- Subplot 2: Available Data ---
        ax2 = axes[0, 1]
        if parser.available:
            avail = list(parser.available)
            ax2.fill_between(t[:len(avail)], avail, alpha=0.4, color='green')
            ax2.plot(t[:len(avail)], avail, 'g-', linewidth=1.2)
        ax2.set_xlabel('Waktu (s)')
        ax2.set_ylabel('Bytes Tersedia')
        ax2.set_title('Data Tersedia di Buffer')
        ax2.grid(True, alpha=0.3)
        ax2.set_ylim(0, BUFFER_SIZE)

        # --- Subplot 3: RX Lengths per message ---
        ax3 = axes[1, 0]
        if parser.rx_lengths:
            rl = list(parser.rx_lengths)
            ax3.bar(range(len(rl)), rl, color='purple', alpha=0.7)
            ax3.axhline(y=np.mean(rl), color='orange', linestyle='--',
                        label=f'Rata-rata: {np.mean(rl):.1f}B')
        ax3.set_xlabel('Message Index')
        ax3.set_ylabel('Panjang (bytes)')
        ax3.set_title('Panjang Data per Penerimaan')
        ax3.legend(fontsize=8)
        ax3.grid(True, alpha=0.3)

        # --- Subplot 4: Throughput ---
        ax4 = axes[1, 1]
        if parser.throughput:
            tp = list(parser.throughput)
            ax4.plot(tp, 'm-o', linewidth=1.5, markersize=4)
            ax4.axhline(y=np.mean(tp), color='orange', linestyle='--',
                        label=f'Rata-rata: {np.mean(tp):.0f} B/s')
        ax4.set_xlabel('Interval')
        ax4.set_ylabel('Throughput (bytes/s)')
        ax4.set_title('Throughput Penerimaan')
        ax4.legend(fontsize=8)
        ax4.grid(True, alpha=0.3)

    return fig, update


def read_serial(port, baud, parser, interactive=False):
    """Baca data dari serial port."""
    import serial
    import threading

    print(f"[INFO] Membuka serial port {port} @ {baud} baud...")
    ser = serial.Serial(port, baud, timeout=0.1)
    print(f"[INFO] Serial port terbuka.")

    if interactive:
        print("[INFO] Mode interaktif: ketik pesan dan tekan Enter untuk mengirim.")
        print("[INFO] Ketik 'quit' untuk keluar.\n")

        def input_thread():
            while True:
                try:
                    msg = input()
                    if msg.lower() == 'quit':
                        break
                    ser.write((msg + '\r\n').encode())
                    print(f"  >> Sent: {msg}")
                except (EOFError, KeyboardInterrupt):
                    break

        t = threading.Thread(target=input_thread, daemon=True)
        t.start()

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

    print(f"\n[INFO] Total pesan: {parser.total_messages}")

    if len(parser.timestamps) == 0:
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
        description='Debug Parser untuk STM32 DMA Circular Buffer UART',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Contoh penggunaan:
  %(prog)s --port /dev/ttyUSB0
  %(prog)s --port COM3 --interactive
  %(prog)s --file capture.log
        """
    )
    arg_parser.add_argument('--port', '-p', type=str,
                            help='Serial port (e.g., /dev/ttyUSB0, COM3)')
    arg_parser.add_argument('--baud', '-b', type=int, default=115200,
                            help='Baud rate (default: 115200)')
    arg_parser.add_argument('--file', '-f', type=str,
                            help='File log untuk di-parse')
    arg_parser.add_argument('--interactive', '-i', action='store_true',
                            help='Mode interaktif (kirim dan terima data)')

    args = arg_parser.parse_args()

    if not args.port and not args.file:
        arg_parser.print_help()
        print("\n[ERROR] Harus menyediakan --port atau --file")
        sys.exit(1)

    parser = CircularBufferParser()

    if args.file:
        read_file(args.file, parser)
    elif args.port:
        read_serial(args.port, args.baud, parser, args.interactive)


if __name__ == '__main__':
    main()
