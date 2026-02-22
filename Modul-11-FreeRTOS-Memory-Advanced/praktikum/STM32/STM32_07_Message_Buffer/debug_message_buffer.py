#!/usr/bin/env python3
"""
debug_message_buffer.py - Message Buffer Debug & Analysis Tool

Parses discrete messages from STM32_07_Message_Buffer.
Categorizes messages by type (CMD, DATA, STATUS).
Shows message distribution, timing, and throughput.
Logs data to CSV for post-analysis.

Usage:
    python debug_message_buffer.py [--port /dev/ttyUSB0] [--baud 115200] [--csv log.csv]
"""

import serial
import re
import sys
import argparse
import csv
import time
from datetime import datetime
from collections import deque, Counter

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not available, plotting disabled")


class MessageBufferMonitor:
    def __init__(self, port, baud, csv_file=None):
        self.port = port
        self.baud = baud
        self.csv_file = csv_file
        self.csv_writer = None
        self.csv_fh = None

        # Event storage
        self.timestamps = deque(maxlen=500)
        self.tx_events = deque(maxlen=300)
        self.rx_events = deque(maxlen=300)
        self.msg_sizes = deque(maxlen=300)

        # Counters per type (TX)
        self.tx_cmd = 0
        self.tx_data = 0
        self.tx_status = 0

        # Counters per type (RX)
        self.rx_cmd = 0
        self.rx_data = 0
        self.rx_status = 0
        self.rx_unknown = 0

        self.total_bytes_sent = 0
        self.total_bytes_recv = 0
        self.rx_timeouts = 0
        self.start_time = time.time()

        # Type-specific data tracking
        self.sensor_data = deque(maxlen=200)
        self.battery_data = deque(maxlen=100)
        self.rssi_data = deque(maxlen=100)

        # Regex patterns - TX
        self.re_cmd_tx = re.compile(
            r'\[CMD_TX\] #(\d+) cmd=0x([0-9A-Fa-f]+) param=(\d+) size=(\d+)')
        self.re_dat_tx = re.compile(
            r'\[DAT_TX\] #(\d+) sensor=(\d+) ts=(\d+) v=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\] size=(\d+)')
        self.re_sta_tx = re.compile(
            r'\[STA_TX\] #(\d+) node=(\d+) bat=(\d+)mV rssi=(-?\d+)dBm \[(.+?)\] size=(\d+)')

        # Regex patterns - RX
        self.re_msg_rx_cmd = re.compile(
            r'\[MSG_RX\] #(\d+) type=CMD cmd=0x([0-9A-Fa-f]+) param=(\d+) len=(\d+)')
        self.re_msg_rx_data = re.compile(
            r'\[MSG_RX\] #(\d+) type=DATA sensor=(\d+) ts=(\d+) v=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\] len=(\d+)')
        self.re_msg_rx_status = re.compile(
            r'\[MSG_RX\] #(\d+) type=STATUS node=(\d+) bat=(\d+)mV rssi=(-?\d+) \[(.+?)\] len=(\d+)')
        self.re_msg_rx_unknown = re.compile(
            r'\[MSG_RX\] #(\d+) type=UNKNOWN')
        self.re_msg_rx_timeout = re.compile(r'\[MSG_RX\] Timeout')

        # Stats
        self.re_stats_cmd_sent = re.compile(r'\[STATS\] Commands\s*:\s*(\d+)')
        self.re_stats_data_sent = re.compile(r'\[STATS\] Data\s*:\s*(\d+)')
        self.re_stats_status_sent = re.compile(r'\[STATS\] Status\s*:\s*(\d+)')
        self.re_stats_total_bytes = re.compile(r'\[STATS\] Total bytes\s*:\s*(\d+)')
        self.re_stats_unknown = re.compile(r'\[STATS\] Unknown\s*:\s*(\d+)')

    def init_csv(self):
        if self.csv_file:
            self.csv_fh = open(self.csv_file, 'w', newline='')
            self.csv_writer = csv.writer(self.csv_fh)
            self.csv_writer.writerow([
                'timestamp', 'elapsed_s', 'direction', 'msg_type', 'seq_num',
                'msg_size', 'detail1', 'detail2', 'detail3', 'detail4'
            ])

    def log_csv(self, direction, msg_type, seq=0, size=0,
                d1='', d2='', d3='', d4=''):
        if self.csv_writer:
            elapsed = time.time() - self.start_time
            self.csv_writer.writerow([
                datetime.now().isoformat(), f'{elapsed:.3f}',
                direction, msg_type, seq, size, d1, d2, d3, d4
            ])
            self.csv_fh.flush()

    def parse_line(self, line):
        elapsed = time.time() - self.start_time

        # ---- TX events ----
        m = self.re_cmd_tx.search(line)
        if m:
            seq = int(m.group(1))
            cmd = m.group(2)
            param = int(m.group(3))
            size = int(m.group(4))
            self.tx_cmd += 1
            self.tx_events.append((elapsed, 'CMD', seq, size))
            self.timestamps.append(elapsed)
            self.msg_sizes.append(('TX_CMD', size))
            self.log_csv('TX', 'CMD', seq, size, f'0x{cmd}', param)
            print(f"  [{elapsed:8.2f}s] TX CMD #{seq}: cmd=0x{cmd} "
                  f"param={param} [{size}B]")
            return

        m = self.re_dat_tx.search(line)
        if m:
            seq = int(m.group(1))
            sensor = int(m.group(2))
            ts = int(m.group(3))
            v = [int(m.group(i)) for i in range(4, 8)]
            size = int(m.group(8))
            self.tx_data += 1
            self.tx_events.append((elapsed, 'DATA', seq, size))
            self.timestamps.append(elapsed)
            self.msg_sizes.append(('TX_DATA', size))
            self.log_csv('TX', 'DATA', seq, size, sensor, ts,
                         f'{v[0]},{v[1]}', f'{v[2]},{v[3]}')
            print(f"  [{elapsed:8.2f}s] TX DATA #{seq}: sensor={sensor} "
                  f"v={v} [{size}B]")
            return

        m = self.re_sta_tx.search(line)
        if m:
            seq = int(m.group(1))
            node = int(m.group(2))
            bat = int(m.group(3))
            rssi = int(m.group(4))
            desc = m.group(5)
            size = int(m.group(6))
            self.tx_status += 1
            self.tx_events.append((elapsed, 'STATUS', seq, size))
            self.timestamps.append(elapsed)
            self.msg_sizes.append(('TX_STATUS', size))
            self.log_csv('TX', 'STATUS', seq, size, node, bat, rssi, desc)
            print(f"  [{elapsed:8.2f}s] TX STATUS #{seq}: node={node} "
                  f"bat={bat}mV rssi={rssi}dBm [{size}B]")
            return

        # ---- RX events ----
        m = self.re_msg_rx_cmd.search(line)
        if m:
            seq = int(m.group(1))
            cmd = m.group(2)
            param = int(m.group(3))
            size = int(m.group(4))
            self.rx_cmd += 1
            self.rx_events.append((elapsed, 'CMD', seq, size))
            self.timestamps.append(elapsed)
            self.log_csv('RX', 'CMD', seq, size, f'0x{cmd}', param)
            print(f"  [{elapsed:8.2f}s] RX CMD #{seq}: cmd=0x{cmd} "
                  f"param={param} [{size}B]")
            return

        m = self.re_msg_rx_data.search(line)
        if m:
            seq = int(m.group(1))
            sensor = int(m.group(2))
            ts = int(m.group(3))
            v = [int(m.group(i)) for i in range(4, 8)]
            size = int(m.group(8))
            self.rx_data += 1
            self.rx_events.append((elapsed, 'DATA', seq, size))
            self.sensor_data.append((elapsed, sensor, v))
            self.timestamps.append(elapsed)
            self.log_csv('RX', 'DATA', seq, size, sensor, ts,
                         f'{v[0]},{v[1]}', f'{v[2]},{v[3]}')
            print(f"  [{elapsed:8.2f}s] RX DATA #{seq}: sensor={sensor} "
                  f"v={v} [{size}B]")
            return

        m = self.re_msg_rx_status.search(line)
        if m:
            seq = int(m.group(1))
            node = int(m.group(2))
            bat = int(m.group(3))
            rssi = int(m.group(4))
            desc = m.group(5)
            size = int(m.group(6))
            self.rx_status += 1
            self.rx_events.append((elapsed, 'STATUS', seq, size))
            self.battery_data.append((elapsed, node, bat))
            self.rssi_data.append((elapsed, node, rssi))
            self.timestamps.append(elapsed)
            self.log_csv('RX', 'STATUS', seq, size, node, bat, rssi, desc)
            print(f"  [{elapsed:8.2f}s] RX STATUS #{seq}: node={node} "
                  f"bat={bat}mV rssi={rssi}dBm [{size}B]")
            return

        m = self.re_msg_rx_unknown.search(line)
        if m:
            self.rx_unknown += 1
            print(f"  [{elapsed:8.2f}s] RX UNKNOWN message")
            return

        m = self.re_msg_rx_timeout.search(line)
        if m:
            self.rx_timeouts += 1
            print(f"  [{elapsed:8.2f}s] RX timeout")
            return

    def print_summary(self):
        elapsed = time.time() - self.start_time
        total_tx = self.tx_cmd + self.tx_data + self.tx_status
        total_rx = self.rx_cmd + self.rx_data + self.rx_status

        print(f"\n{'='*55}")
        print(f"  MESSAGE BUFFER MONITOR SUMMARY")
        print(f"{'='*55}")
        print(f"  Duration       : {elapsed:.1f} seconds")
        print(f"  --- Sent ---")
        print(f"  Commands       : {self.tx_cmd}")
        print(f"  Data           : {self.tx_data}")
        print(f"  Status         : {self.tx_status}")
        print(f"  Total TX msgs  : {total_tx}")
        print(f"  --- Received ---")
        print(f"  Commands       : {self.rx_cmd}")
        print(f"  Data           : {self.rx_data}")
        print(f"  Status         : {self.rx_status}")
        print(f"  Unknown        : {self.rx_unknown}")
        print(f"  Total RX msgs  : {total_rx}")
        print(f"  RX timeouts    : {self.rx_timeouts}")
        if total_tx > 0:
            loss = total_tx - total_rx
            print(f"  Message loss   : {loss} ({loss*100/total_tx:.1f}%)")
        print(f"{'='*55}\n")

    def run_realtime(self):
        self.init_csv()
        print(f"\n[*] Connecting to {self.port} @ {self.baud} baud...")

        try:
            ser = serial.Serial(self.port, self.baud, timeout=1)
        except serial.SerialException as e:
            print(f"[ERROR] Cannot open {self.port}: {e}")
            sys.exit(1)

        print(f"[*] Connected. Monitoring message buffer...")
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

        if not self.tx_events and not self.rx_events:
            print("[WARN] No data to plot")
            return

        fig, axes = plt.subplots(2, 2, figsize=(14, 9))
        fig.suptitle('STM32 Message Buffer Monitor', fontsize=14,
                     fontweight='bold')

        # Message type distribution (pie chart)
        labels = ['CMD', 'DATA', 'STATUS']
        tx_sizes = [self.tx_cmd, self.tx_data, self.tx_status]
        rx_sizes = [self.rx_cmd, self.rx_data, self.rx_status]

        if sum(tx_sizes) > 0:
            axes[0, 0].pie(tx_sizes, labels=labels, autopct='%1.1f%%',
                          colors=['#ff9999', '#66b3ff', '#99ff99'])
            axes[0, 0].set_title('TX Message Distribution')
        else:
            axes[0, 0].text(0.5, 0.5, 'No TX data', ha='center', va='center')

        if sum(rx_sizes) > 0:
            axes[0, 1].pie(rx_sizes, labels=labels, autopct='%1.1f%%',
                          colors=['#ff9999', '#66b3ff', '#99ff99'])
            axes[0, 1].set_title('RX Message Distribution')
        else:
            axes[0, 1].text(0.5, 0.5, 'No RX data', ha='center', va='center')

        # Message timeline
        type_colors = {'CMD': 'red', 'DATA': 'blue', 'STATUS': 'green'}

        if self.rx_events:
            for msg_type in ['CMD', 'DATA', 'STATUS']:
                events = [(e[0], e[3]) for e in self.rx_events
                          if e[1] == msg_type]
                if events:
                    t = [e[0] for e in events]
                    s = [e[1] for e in events]
                    axes[1, 0].scatter(t, s, c=type_colors[msg_type],
                                      label=msg_type, s=30, alpha=0.7)

            axes[1, 0].set_xlabel('Time (seconds)')
            axes[1, 0].set_ylabel('Message Size (bytes)')
            axes[1, 0].set_title('RX Messages Over Time')
            axes[1, 0].legend()
            axes[1, 0].grid(True, alpha=0.3)

        # Sensor data values if available
        if self.sensor_data:
            sd_t = [d[0] for d in self.sensor_data]
            sd_v0 = [d[2][0] for d in self.sensor_data]
            sd_v1 = [d[2][1] for d in self.sensor_data]
            axes[1, 1].plot(sd_t, sd_v0, 'b-', label='Value[0]', alpha=0.7)
            axes[1, 1].plot(sd_t, sd_v1, 'r-', label='Value[1]', alpha=0.7)
            axes[1, 1].set_xlabel('Time (seconds)')
            axes[1, 1].set_ylabel('Sensor Value')
            axes[1, 1].set_title('Sensor Data Values')
            axes[1, 1].legend()
            axes[1, 1].grid(True, alpha=0.3)
        else:
            axes[1, 1].text(0.5, 0.5, 'No sensor data', ha='center',
                           va='center')

        plt.tight_layout()
        plt.savefig('message_buffer_analysis.png', dpi=150,
                    bbox_inches='tight')
        print("[*] Plot saved to: message_buffer_analysis.png")
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description='STM32 Message Buffer Debug Monitor')
    parser.add_argument('--port', '-p', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('--baud', '-b', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('--csv', '-c', default='message_buffer_log.csv',
                        help='CSV output file')
    parser.add_argument('--plot', action='store_true',
                        help='Show plot after monitoring')
    args = parser.parse_args()

    monitor = MessageBufferMonitor(args.port, args.baud, args.csv)
    monitor.run_realtime()

    if args.plot:
        monitor.plot_results()


if __name__ == '__main__':
    main()
