#!/usr/bin/env python3
"""
============================================================================
Debug & Analysis Tool - ESP32_04_TCP_Client_Server
============================================================================
Modul 13 - Network & IoT | Praktikum Sistem Embedded

Tool ini berfungsi ganda:
  1. Serial Monitor Parser - parse output serial ESP32
  2. TCP Client Tester    - connect ke TCP server ESP32 dan kirim data

PENGGUNAAN:
  # Mode 1: Monitor serial output
  python3 debug_analysis.py monitor --port /dev/ttyUSB0

  # Mode 2: TCP client - test koneksi ke ESP32 server
  python3 debug_analysis.py tcp --host 192.168.1.100 --tcp-port 8080

  # Mode 3: Interactive TCP client
  python3 debug_analysis.py tcp --host 192.168.1.100 --tcp-port 8080 --interactive

  # Mode 4: Parse dari file
  python3 debug_analysis.py monitor --file output.txt

DEPENDENSI:
  pip install pyserial matplotlib
============================================================================
"""

import argparse
import re
import sys
import time
import socket
import threading
from datetime import datetime

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.use('Agg')
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


class TCPAnalyzer:
    """Parser dan analyzer untuk output TCP Server/Client ESP32."""

    def __init__(self):
        self.connections = []       # List of connection events
        self.messages = []          # List of received/sent messages
        self.bytes_received = 0
        self.bytes_sent = 0
        self.active_clients = 0
        self.errors = []

        # Regex patterns
        self.client_connect = re.compile(
            r'Client connected from\s*([\d.]+):(\d+)')
        self.client_disconnect = re.compile(r'Client disconnected')
        self.received = re.compile(r'Received\s+(\d+)\s+bytes:\s*"(.+?)"')
        self.echo_sent = re.compile(r'Echo sent back\s*\((\d+)\s*bytes\)')
        self.error_pattern = re.compile(r'error|gagal|failed', re.IGNORECASE)
        self.heap_pattern = re.compile(r'heap:\s*(\d+)\s*bytes')

    def parse_line(self, line):
        """Parse output serial."""
        line = line.strip()
        timestamp = datetime.now().strftime("%H:%M:%S")

        # Client connected
        conn_match = self.client_connect.search(line)
        if conn_match:
            ip, port = conn_match.group(1), conn_match.group(2)
            self.connections.append({
                'time': timestamp, 'type': 'CONNECT',
                'ip': ip, 'port': port
            })
            self.active_clients += 1
            return "CONNECT"

        # Client disconnected
        if self.client_disconnect.search(line):
            self.active_clients = max(0, self.active_clients - 1)
            self.connections.append({
                'time': timestamp, 'type': 'DISCONNECT',
                'ip': '', 'port': ''
            })
            return "DISCONNECT"

        # Data received
        recv_match = self.received.search(line)
        if recv_match:
            nbytes = int(recv_match.group(1))
            msg = recv_match.group(2)
            self.bytes_received += nbytes
            self.messages.append({
                'time': timestamp, 'dir': 'RX',
                'bytes': nbytes, 'data': msg
            })
            return "RECEIVED"

        # Echo sent
        echo_match = self.echo_sent.search(line)
        if echo_match:
            nbytes = int(echo_match.group(1))
            self.bytes_sent += nbytes
            return "SENT"

        # Errors
        if self.error_pattern.search(line):
            self.errors.append({'time': timestamp, 'msg': line})

        return None

    def print_summary(self):
        """Tampilkan ringkasan TCP communication."""
        print("\n" + "=" * 60)
        print("  TCP SERVER - COMMUNICATION SUMMARY")
        print("=" * 60)
        print(f"  Active Clients  : {self.active_clients}")
        print(f"  Total Connects  : {sum(1 for c in self.connections if c['type']=='CONNECT')}")
        print(f"  Total Disconnects: {sum(1 for c in self.connections if c['type']=='DISCONNECT')}")
        print(f"  Bytes Received  : {self.bytes_received:,}")
        print(f"  Bytes Sent      : {self.bytes_sent:,}")
        print(f"  Messages        : {len(self.messages)}")
        print(f"  Errors          : {len(self.errors)}")

        if self.messages:
            print(f"\n  Recent Messages (last 10):")
            for msg in self.messages[-10:]:
                arrow = "◄─" if msg['dir'] == 'RX' else "─►"
                print(f"    [{msg['time']}] {arrow} {msg['bytes']}B: {msg['data']}")

        if self.errors:
            print(f"\n  Errors (last 5):")
            for err in self.errors[-5:]:
                print(f"    [{err['time']}] ✗ {err['msg'][:60]}")
        print("=" * 60)

    def plot_traffic(self, filename="tcp_traffic.png"):
        """Plot traffic statistics."""
        if not HAS_MATPLOTLIB or not self.messages:
            return

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

        # Message sizes over time
        times = list(range(len(self.messages)))
        sizes = [m['bytes'] for m in self.messages]
        colors = ['blue' if m['dir'] == 'RX' else 'green' for m in self.messages]
        ax1.bar(times, sizes, color=colors, alpha=0.7)
        ax1.set_xlabel('Message Number')
        ax1.set_ylabel('Size (bytes)')
        ax1.set_title('TCP Message Sizes (Blue=RX, Green=TX)')
        ax1.grid(True, alpha=0.3)

        # Connection events timeline
        connect_times = [i for i, c in enumerate(self.connections) if c['type'] == 'CONNECT']
        disconnect_times = [i for i, c in enumerate(self.connections) if c['type'] == 'DISCONNECT']
        ax2.eventplot([connect_times], colors=['green'], lineoffsets=1,
                      linelengths=0.5, label='Connect')
        ax2.eventplot([disconnect_times], colors=['red'], lineoffsets=0.5,
                      linelengths=0.5, label='Disconnect')
        ax2.set_xlabel('Event Number')
        ax2.set_title('Connection Events')
        ax2.legend()
        ax2.set_yticks([])

        plt.tight_layout()
        plt.savefig(filename, dpi=150)
        print(f"[OK] Traffic chart saved: {filename}")
        plt.close()


class TCPClientTester:
    """TCP Client untuk testing koneksi ke ESP32 server."""

    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = None
        self.connected = False

    def connect(self):
        """Connect ke TCP server."""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(5.0)
            print(f"[INFO] Connecting to {self.host}:{self.port}...")
            self.sock.connect((self.host, self.port))
            self.connected = True
            print(f"[OK] Connected to {self.host}:{self.port}")
            return True
        except socket.error as e:
            print(f"[ERROR] Connection failed: {e}")
            return False

    def send_message(self, message):
        """Kirim pesan dan terima response (echo)."""
        if not self.connected:
            print("[ERROR] Not connected!")
            return None
        try:
            self.sock.sendall(message.encode('utf-8'))
            print(f"[TX] Sent: \"{message}\" ({len(message)} bytes)")

            response = self.sock.recv(1024)
            response_str = response.decode('utf-8', errors='replace')
            print(f"[RX] Echo: \"{response_str}\" ({len(response)} bytes)")

            # Verify echo
            if response_str == message:
                print("[OK] Echo verified ✓")
            else:
                print("[WARN] Echo mismatch! ✗")
            return response_str
        except socket.error as e:
            print(f"[ERROR] Communication error: {e}")
            return None

    def run_automated_test(self):
        """Jalankan serangkaian test otomatis."""
        print("\n" + "=" * 50)
        print("  TCP CLIENT - AUTOMATED TEST")
        print("=" * 50)

        if not self.connect():
            return

        test_messages = [
            "Hello ESP32!",
            "Test 123",
            "Praktikum Modul 13",
            "A" * 100,             # Pesan panjang
            "Special: !@#$%^&*()",
            "",                     # Pesan kosong
        ]

        passed = 0
        failed = 0

        for i, msg in enumerate(test_messages):
            print(f"\n--- Test {i+1}/{len(test_messages)} ---")
            if not msg:
                print("[SKIP] Empty message")
                continue
            result = self.send_message(msg)
            if result == msg:
                passed += 1
            else:
                failed += 1
            time.sleep(0.5)

        print(f"\n{'='*50}")
        print(f"  TEST RESULTS: {passed} passed, {failed} failed")
        print(f"{'='*50}")

        self.close()

    def run_interactive(self):
        """Mode interaktif - user ketik pesan manual."""
        if not self.connect():
            return

        print("\n[INFO] Interactive mode. Type message and press Enter.")
        print("[INFO] Type 'quit' to exit.\n")

        try:
            while True:
                msg = input("Send > ")
                if msg.lower() == 'quit':
                    break
                if msg:
                    self.send_message(msg)
        except (KeyboardInterrupt, EOFError):
            print("\n[INFO] Exiting interactive mode.")

        self.close()

    def close(self):
        """Tutup koneksi."""
        if self.sock:
            self.sock.close()
            self.connected = False
            print("[INFO] Connection closed.")


def monitor_serial(port, baudrate, analyzer):
    """Monitor serial dari ESP32."""
    if not HAS_SERIAL:
        print("[ERROR] pyserial required.")
        sys.exit(1)

    print(f"[INFO] Connecting to {port} @ {baudrate}...")
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"[OK] Monitoring... (Ctrl+C to stop)\n")

        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='replace')
                print(line, end='')
                result = analyzer.parse_line(line)
                if result in ("CONNECT", "DISCONNECT"):
                    analyzer.print_summary()

    except serial.SerialException as e:
        print(f"[ERROR] Serial: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Stopped.")
        analyzer.print_summary()
        analyzer.plot_traffic()


def parse_from_file(filepath, analyzer):
    """Parse dari file."""
    with open(filepath, 'r') as f:
        for line in f:
            analyzer.parse_line(line)
    analyzer.print_summary()
    analyzer.plot_traffic()


def main():
    parser = argparse.ArgumentParser(
        description='ESP32 TCP Client/Server - Debug & Analysis Tool')
    subparsers = parser.add_subparsers(dest='mode', help='Operation mode')

    # Monitor mode
    mon_parser = subparsers.add_parser('monitor', help='Monitor serial output')
    mon_parser.add_argument('--port', default='/dev/ttyUSB0')
    mon_parser.add_argument('--baud', type=int, default=115200)
    mon_parser.add_argument('--file', type=str, default=None)

    # TCP client mode
    tcp_parser = subparsers.add_parser('tcp', help='TCP client tester')
    tcp_parser.add_argument('--host', required=True, help='ESP32 IP address')
    tcp_parser.add_argument('--tcp-port', type=int, default=8080)
    tcp_parser.add_argument('--interactive', action='store_true',
                            help='Interactive mode')

    args = parser.parse_args()

    if args.mode == 'monitor':
        analyzer = TCPAnalyzer()
        if args.file:
            parse_from_file(args.file, analyzer)
        else:
            monitor_serial(args.port, args.baud, analyzer)
    elif args.mode == 'tcp':
        tester = TCPClientTester(args.host, args.tcp_port)
        if args.interactive:
            tester.run_interactive()
        else:
            tester.run_automated_test()
    else:
        parser.print_help()


if __name__ == '__main__':
    main()
