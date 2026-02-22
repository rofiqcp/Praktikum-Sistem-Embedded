#!/usr/bin/env python3
"""
debug_stream_buffer.py - Stream Buffer Debug & Analysis Tool

Parses sent/received byte counts from STM32_06_Stream_Buffer.
Shows throughput analysis, buffer utilization, and trigger level behavior.
Logs data to CSV for post-analysis.

Usage:
    python debug_stream_buffer.py [--port /dev/ttyUSB0] [--baud 115200] [--csv log.csv]
"""

import serial
import re
import sys
import argparse
import csv
import time
from datetime import datetime
from collections import deque

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not available, plotting disabled")


class StreamBufferMonitor:
    def __init__(self, port, baud, csv_file=None):
        self.port = port
        self.baud = baud
        self.csv_file = csv_file
        self.csv_writer = None
        self.csv_fh = None

        # Data storage
        self.timestamps = deque(maxlen=500)
        self.bytes_sent_history = deque(maxlen=500)
        self.bytes_recv_history = deque(maxlen=500)
        self.throughput_history = deque(maxlen=500)
        self.buf_space_history = deque(maxlen=500)

        # Event tracking
        self.send_events = deque(maxlen=300)
        self.recv_events = deque(maxlen=300)
        self.burst_events = deque(maxlen=50)

        # Counters
        self.total_bytes_sent = 0
        self.total_bytes_recv = 0
        self.send_ops = 0
        self.recv_ops = 0
        self.send_fails = 0
        self.recv_timeouts = 0
        self.throughput = 0
        self.start_time = time.time()

        # Regex patterns
        self.re_send = re.compile(
            r'\[SEND\] #(\d+) len=(\d+) sent=(\d+) buf_space=(\d+)')
        self.re_send_fail = re.compile(r'\[SEND\] #(\d+) FAILED')
        self.re_recv = re.compile(
            r'\[RECV\] #(\d+) bytes=(\d+) data=\[(.+?)\]')
        self.re_recv_buf = re.compile(
            r'\[RECV\] buf_used=(\d+) trigger=(\d+)')
        self.re_recv_timeout = re.compile(r'\[RECV\] Timeout')
        self.re_burst_start = re.compile(r'\[BURST\] === Burst #(\d+)')
        self.re_burst_end = re.compile(r'\[BURST\] Burst #(\d+) complete')
        self.re_stats_sent = re.compile(r'\[STATS\] Bytes sent\s*:\s*(\d+)')
        self.re_stats_recv = re.compile(r'\[STATS\] Bytes recv\s*:\s*(\d+)')
        self.re_stats_send_ops = re.compile(r'\[STATS\] Send ops\s*:\s*(\d+)')
        self.re_stats_recv_ops = re.compile(r'\[STATS\] Recv ops\s*:\s*(\d+)')
        self.re_stats_send_fail = re.compile(r'\[STATS\] Send fails\s*:\s*(\d+)')
        self.re_stats_throughput = re.compile(r'\[STATS\] Throughput\s*:\s*(\d+)')
        self.re_stats_space = re.compile(r'\[STATS\] Buf space\s*:\s*(\d+)')

    def init_csv(self):
        if self.csv_file:
            self.csv_fh = open(self.csv_file, 'w', newline='')
            self.csv_writer = csv.writer(self.csv_fh)
            self.csv_writer.writerow([
                'timestamp', 'elapsed_s', 'event_type', 'seq_num',
                'data_len', 'bytes_transferred', 'buf_space',
                'total_sent', 'total_recv', 'throughput_bps', 'data_content'
            ])

    def log_csv(self, event_type, seq=0, data_len=0, transferred=0,
                buf_space=0, data=''):
        if self.csv_writer:
            elapsed = time.time() - self.start_time
            self.csv_writer.writerow([
                datetime.now().isoformat(), f'{elapsed:.3f}',
                event_type, seq, data_len, transferred, buf_space,
                self.total_bytes_sent, self.total_bytes_recv,
                self.throughput, data
            ])
            self.csv_fh.flush()

    def parse_line(self, line):
        elapsed = time.time() - self.start_time

        # Send event
        m = self.re_send.search(line)
        if m:
            seq = int(m.group(1))
            data_len = int(m.group(2))
            sent = int(m.group(3))
            space = int(m.group(4))

            self.send_events.append((elapsed, seq, data_len, sent, space))
            self.timestamps.append(elapsed)
            self.buf_space_history.append(space)

            self.log_csv('send', seq, data_len, sent, space)
            print(f"  [{elapsed:8.2f}s] TX #{seq}: {sent}/{data_len} bytes, "
                  f"buf_space={space}")
            return

        # Send fail
        m = self.re_send_fail.search(line)
        if m:
            seq = int(m.group(1))
            self.send_fails += 1
            print(f"  [{elapsed:8.2f}s] *** TX #{seq} FAILED - buffer full ***")
            self.log_csv('send_fail', seq)
            return

        # Receive event
        m = self.re_recv.search(line)
        if m:
            seq = int(m.group(1))
            nbytes = int(m.group(2))
            data = m.group(3)

            self.recv_events.append((elapsed, seq, nbytes, data))
            self.log_csv('recv', seq, nbytes, nbytes, 0, data)
            print(f"  [{elapsed:8.2f}s] RX #{seq}: {nbytes} bytes = [{data}]")
            return

        # Receive buffer info
        m = self.re_recv_buf.search(line)
        if m:
            buf_used = int(m.group(1))
            trigger = int(m.group(2))
            print(f"           buf_used={buf_used}, trigger_level={trigger}")
            return

        # Receive timeout
        m = self.re_recv_timeout.search(line)
        if m:
            self.recv_timeouts += 1
            print(f"  [{elapsed:8.2f}s] RX timeout (trigger not met)")
            self.log_csv('recv_timeout')
            return

        # Burst events
        m = self.re_burst_start.search(line)
        if m:
            burst_num = int(m.group(1))
            self.burst_events.append((elapsed, burst_num, 'start'))
            print(f"  [{elapsed:8.2f}s] >>> BURST #{burst_num} START <<<")
            self.log_csv('burst_start', burst_num)
            return

        m = self.re_burst_end.search(line)
        if m:
            burst_num = int(m.group(1))
            self.burst_events.append((elapsed, burst_num, 'end'))
            print(f"  [{elapsed:8.2f}s] >>> BURST #{burst_num} END <<<")
            self.log_csv('burst_end', burst_num)
            return

        # Stats
        m = self.re_stats_sent.search(line)
        if m:
            self.total_bytes_sent = int(m.group(1))
            self.bytes_sent_history.append(self.total_bytes_sent)

        m = self.re_stats_recv.search(line)
        if m:
            self.total_bytes_recv = int(m.group(1))
            self.bytes_recv_history.append(self.total_bytes_recv)

        m = self.re_stats_send_ops.search(line)
        if m:
            self.send_ops = int(m.group(1))

        m = self.re_stats_recv_ops.search(line)
        if m:
            self.recv_ops = int(m.group(1))

        m = self.re_stats_throughput.search(line)
        if m:
            self.throughput = int(m.group(1))
            self.throughput_history.append(self.throughput)
            self.timestamps.append(elapsed)

        m = self.re_stats_space.search(line)
        if m:
            space = int(m.group(1))
            self.buf_space_history.append(space)
            self.log_csv('stats')

    def print_summary(self):
        elapsed = time.time() - self.start_time
        avg_throughput = self.total_bytes_recv / elapsed if elapsed > 0 else 0

        print(f"\n{'='*50}")
        print(f"  STREAM BUFFER MONITOR SUMMARY")
        print(f"{'='*50}")
        print(f"  Duration       : {elapsed:.1f} seconds")
        print(f"  Total TX bytes : {self.total_bytes_sent}")
        print(f"  Total RX bytes : {self.total_bytes_recv}")
        print(f"  Send ops       : {self.send_ops}")
        print(f"  Recv ops       : {self.recv_ops}")
        print(f"  Send failures  : {self.send_fails}")
        print(f"  Recv timeouts  : {self.recv_timeouts}")
        print(f"  Avg throughput : {avg_throughput:.1f} bytes/sec")
        if self.send_events:
            avg_send = sum(e[3] for e in self.send_events) / len(self.send_events)
            print(f"  Avg send size  : {avg_send:.1f} bytes")
        if self.recv_events:
            avg_recv = sum(e[2] for e in self.recv_events) / len(self.recv_events)
            print(f"  Avg recv size  : {avg_recv:.1f} bytes")
        print(f"{'='*50}\n")

    def run_realtime(self):
        self.init_csv()
        print(f"\n[*] Connecting to {self.port} @ {self.baud} baud...")

        try:
            ser = serial.Serial(self.port, self.baud, timeout=1)
        except serial.SerialException as e:
            print(f"[ERROR] Cannot open {self.port}: {e}")
            sys.exit(1)

        print(f"[*] Connected. Monitoring stream buffer...")
        print(f"[*] Press Ctrl+C to stop\n")

        try:
            while True:
                raw = ser.readline()
                if raw:
                    try:
                        line = raw.decode('utf-8', errors='replace').strip()
                    except Exception:
                        continue
                    if line:
                        self.parse_line(line)
        except KeyboardInterrupt:
            print("\n[*] Stopped by user")
        finally:
            self.print_summary()
            ser.close()
            if self.csv_fh:
                self.csv_fh.close()
                print(f"[*] CSV saved to: {self.csv_file}")

    def plot_results(self):
        if not HAS_MATPLOTLIB:
            print("[WARN] Cannot plot - matplotlib not installed")
            return

        if not self.send_events and not self.recv_events:
            print("[WARN] No data to plot")
            return

        fig, axes = plt.subplots(3, 1, figsize=(12, 9), sharex=True)
        fig.suptitle('STM32 Stream Buffer Monitor', fontsize=14,
                     fontweight='bold')

        # Send/Recv event sizes over time
        if self.send_events:
            se_t = [e[0] for e in self.send_events]
            se_bytes = [e[3] for e in self.send_events]
            axes[0].bar(se_t, se_bytes, width=0.3, alpha=0.6, color='blue',
                        label='Sent')

        if self.recv_events:
            re_t = [e[0] for e in self.recv_events]
            re_bytes = [e[2] for e in self.recv_events]
            axes[0].bar(re_t, re_bytes, width=0.3, alpha=0.6, color='green',
                        label='Received')

        # Mark burst events
        for be in self.burst_events:
            if be[2] == 'start':
                axes[0].axvline(x=be[0], color='red', linestyle='--',
                               alpha=0.5)
                axes[0].annotate(f'Burst#{be[1]}', xy=(be[0], 0),
                                fontsize=7, color='red')

        axes[0].set_ylabel('Bytes')
        axes[0].set_title('Send/Receive Events')
        axes[0].legend(loc='upper right')
        axes[0].grid(True, alpha=0.3)

        # Buffer space over time
        if self.buf_space_history:
            bs_t = list(self.timestamps)[:len(self.buf_space_history)]
            axes[1].plot(bs_t, list(self.buf_space_history), 'orange',
                         linewidth=1.5)
            axes[1].fill_between(bs_t, list(self.buf_space_history),
                                alpha=0.2, color='orange')
            axes[1].axhline(y=256, color='gray', linestyle='--', alpha=0.5,
                           label='Buffer size')
            axes[1].axhline(y=16, color='red', linestyle='--', alpha=0.5,
                           label='Trigger level')
            axes[1].set_ylabel('Bytes')
            axes[1].set_title('Buffer Space Available')
            axes[1].legend(loc='upper right')
            axes[1].grid(True, alpha=0.3)

        # Throughput
        if self.throughput_history:
            tp_t = list(self.timestamps)[:len(self.throughput_history)]
            axes[2].plot(tp_t, list(self.throughput_history), 'm-',
                         linewidth=1.5, marker='o', markersize=4)
            axes[2].fill_between(tp_t, list(self.throughput_history),
                                alpha=0.2, color='magenta')
            axes[2].set_ylabel('Bytes/sec')
            axes[2].set_title('Throughput')
            axes[2].grid(True, alpha=0.3)

        axes[2].set_xlabel('Time (seconds)')
        plt.tight_layout()
        plt.savefig('stream_buffer_analysis.png', dpi=150, bbox_inches='tight')
        print("[*] Plot saved to: stream_buffer_analysis.png")
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description='STM32 Stream Buffer Debug Monitor')
    parser.add_argument('--port', '-p', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('--baud', '-b', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('--csv', '-c', default='stream_buffer_log.csv',
                        help='CSV output file')
    parser.add_argument('--plot', action='store_true',
                        help='Show plot after monitoring')
    args = parser.parse_args()

    monitor = StreamBufferMonitor(args.port, args.baud, args.csv)
    monitor.run_realtime()

    if args.plot:
        monitor.plot_results()


if __name__ == '__main__':
    main()
