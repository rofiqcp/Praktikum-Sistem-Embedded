#!/usr/bin/env python3
"""
===========================================================================
 Debug Script: ESP32 UART RX — Sender & Verifier
===========================================================================
 Sends test data via serial to ESP32_03_DMA_UART_RX,
 verifies reception via echo, and plots event statistics.

 Usage:
   python debug_uart_rx.py [--port /dev/ttyUSB0] [--baud 115200]
   python debug_uart_rx.py --file output.log  (parse-only mode)
===========================================================================
"""

import re
import sys
import argparse
import time
import threading

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_PLOT = True
except ImportError:
    HAS_PLOT = False


class UartRxDebugger:
    """Sends test data and monitors UART RX events."""

    def __init__(self, port, baud):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = False
        self.rx_lines = []
        self.stats = {
            'sent_bytes': 0,
            'echo_bytes': 0,
            'events_parsed': 0,
            'data_events': 0,
            'overflow_events': 0,
            'pattern_events': 0,
        }

    def connect(self):
        """Open serial connection."""
        if not HAS_SERIAL:
            print("ERROR: pyserial not installed. Run: pip install pyserial")
            sys.exit(1)
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=0.5)
            print(f"Connected to {self.port} at {self.baud} baud")
            return True
        except serial.SerialException as e:
            print(f"Serial error: {e}")
            return False

    def reader_thread(self):
        """Background thread to read serial output."""
        while self.running:
            try:
                raw = self.ser.readline()
                if raw:
                    line = raw.decode('utf-8', errors='replace').strip()
                    if line:
                        self.rx_lines.append(line)
                        print(f"[ESP32] {line}")

                        # Parse stats from output
                        if 'Data Events:' in line:
                            m = re.search(r'(\d+)', line.split(':')[-1])
                            if m:
                                self.stats['data_events'] = int(m.group(1))
                        elif 'FIFO Overflow:' in line:
                            m = re.search(r'(\d+)', line.split(':')[-1])
                            if m:
                                self.stats['overflow_events'] = int(m.group(1))
                        elif 'Pattern Match:' in line:
                            m = re.search(r'(\d+)', line.split(':')[-1])
                            if m:
                                self.stats['pattern_events'] = int(m.group(1))
            except Exception:
                pass

    def send_test_data(self):
        """Send various test data patterns."""
        print("\n--- Sending test data ---\n")

        # Test 1: Simple string
        test1 = b"Hello ESP32 UART RX!\r"
        print(f"[SEND] Test 1: {len(test1)} bytes — Simple string")
        self.ser.write(test1)
        self.stats['sent_bytes'] += len(test1)
        time.sleep(0.5)

        # Test 2: Repeated pattern
        test2 = b"ABCDEFGH" * 16 + b"\r"
        print(f"[SEND] Test 2: {len(test2)} bytes — Repeated pattern")
        self.ser.write(test2)
        self.stats['sent_bytes'] += len(test2)
        time.sleep(0.5)

        # Test 3: Binary data
        test3 = bytes(range(256)) + b"\r"
        print(f"[SEND] Test 3: {len(test3)} bytes — Binary 0x00-0xFF")
        self.ser.write(test3)
        self.stats['sent_bytes'] += len(test3)
        time.sleep(0.5)

        # Test 4: Large block
        test4 = b"X" * 1024 + b"\r"
        print(f"[SEND] Test 4: {len(test4)} bytes — Large block")
        self.ser.write(test4)
        self.stats['sent_bytes'] += len(test4)
        time.sleep(1)

        # Test 5: Rapid small messages
        print("[SEND] Test 5: 20 rapid small messages")
        for i in range(20):
            msg = f"MSG{i:03d}\r".encode()
            self.ser.write(msg)
            self.stats['sent_bytes'] += len(msg)
            time.sleep(0.05)

        time.sleep(1)
        print(f"\n[DONE] Total sent: {self.stats['sent_bytes']} bytes")

    def run(self, duration=30):
        """Run the debug session."""
        if not self.connect():
            return

        self.running = True
        reader = threading.Thread(target=self.reader_thread, daemon=True)
        reader.start()

        time.sleep(2)  # Wait for ESP32 to initialize
        self.send_test_data()

        print(f"\nMonitoring for {duration}s (waiting for stats)...")
        time.sleep(duration)

        self.running = False
        reader.join(timeout=2)
        self.ser.close()

        self.print_summary()
        self.create_charts()

    def print_summary(self):
        """Print test summary."""
        print("\n" + "=" * 50)
        print("  UART RX Debug Summary")
        print("=" * 50)
        print(f"  Bytes Sent:        {self.stats['sent_bytes']}")
        print(f"  Data Events:       {self.stats['data_events']}")
        print(f"  Overflow Events:   {self.stats['overflow_events']}")
        print(f"  Pattern Matches:   {self.stats['pattern_events']}")
        print("=" * 50)

    def create_charts(self):
        """Create event statistics charts."""
        if not HAS_PLOT:
            print("matplotlib not installed, skipping charts")
            return

        fig, axes = plt.subplots(1, 2, figsize=(12, 5))

        # Chart 1: Event breakdown
        ax = axes[0]
        events = ['Data', 'Overflow', 'Pattern', 'Other']
        counts = [
            self.stats['data_events'],
            self.stats['overflow_events'],
            self.stats['pattern_events'],
            0
        ]
        colors = ['#2ecc71', '#e74c3c', '#3498db', '#95a5a6']
        bars = ax.bar(events, counts, color=colors)
        ax.set_title('ESP32 UART RX — Event Distribution')
        ax.set_ylabel('Count')
        for bar, val in zip(bars, counts):
            if val > 0:
                ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height(),
                        str(val), ha='center', va='bottom')
        ax.grid(axis='y', alpha=0.3)

        # Chart 2: Data flow
        ax2 = axes[1]
        labels = ['Bytes Sent', 'Events Processed']
        values = [self.stats['sent_bytes'], self.stats['data_events']]
        ax2.bar(labels, values, color=['#3498db', '#2ecc71'])
        ax2.set_title('ESP32 UART RX — Data Flow')
        ax2.set_ylabel('Count')
        ax2.grid(axis='y', alpha=0.3)

        plt.tight_layout()
        plt.savefig('uart_rx_events.png', dpi=150)
        print("Chart saved: uart_rx_events.png")
        plt.show()


def parse_log_file(filepath):
    """Parse stats from a log file."""
    with open(filepath, 'r') as f:
        lines = f.readlines()

    stats = {}
    for line in lines:
        line = line.strip()
        if 'Total Bytes:' in line:
            m = re.search(r'(\d+)', line.split(':')[-1])
            if m:
                stats['total_bytes'] = int(m.group(1))
        elif 'Data Events:' in line:
            m = re.search(r'(\d+)', line.split(':')[-1])
            if m:
                stats['data_events'] = int(m.group(1))

    print(f"Parsed stats: {stats}")


def main():
    parser = argparse.ArgumentParser(description='ESP32 UART RX Debug Tool')
    parser.add_argument('--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--file', help='Parse log file (no serial)')
    parser.add_argument('--duration', type=int, default=30, help='Monitor duration (s)')
    args = parser.parse_args()

    if args.file:
        parse_log_file(args.file)
    else:
        debugger = UartRxDebugger(args.port, args.baud)
        debugger.run(args.duration)


if __name__ == '__main__':
    main()
