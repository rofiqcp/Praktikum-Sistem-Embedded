"""
debug_analysis.py - WiFi Connection Monitor for ESP-01
======================================================
Monitors STM32 debug output and tracks WiFi connection status,
signal strength, and reconnection events.

Usage:
    python debug_analysis.py [PORT] [BAUDRATE]
    python debug_analysis.py /dev/ttyUSB0 115200
"""

import sys
import time
import re
import threading
from datetime import datetime
from collections import defaultdict

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[!] pyserial not installed. Run: pip install pyserial")
    sys.exit(1)

DEFAULT_PORT = "/dev/ttyUSB0"
DEFAULT_BAUD = 115200


class WiFiMonitor:
    """Tracks WiFi connection state and statistics."""

    def __init__(self):
        self.connected = False
        self.ip_address = "N/A"
        self.ssid = "N/A"
        self.connect_count = 0
        self.disconnect_count = 0
        self.last_connect_time = None
        self.events = []
        self.at_responses = defaultdict(int)

    def parse_line(self, line):
        """Parse a debug output line for WiFi status info."""
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

        # Detect WiFi connected
        if "WiFi connected" in line or "WIFI GOT IP" in line:
            self.connected = True
            self.connect_count += 1
            self.last_connect_time = time.time()
            self.events.append((timestamp, "CONNECTED"))

        # Detect WiFi disconnected
        if "disconnected" in line.lower() or "No AP" in line:
            self.connected = False
            self.disconnect_count += 1
            self.events.append((timestamp, "DISCONNECTED"))

        # Parse IP address from CIFSR response
        ip_match = re.search(r'STAIP,"(\d+\.\d+\.\d+\.\d+)"', line)
        if ip_match:
            self.ip_address = ip_match.group(1)

        # Parse SSID from CWJAP response
        ssid_match = re.search(r'CWJAP:"([^"]+)"', line)
        if ssid_match:
            self.ssid = ssid_match.group(1)

        # Count AT responses
        if "[OK]" in line:
            self.at_responses["OK"] += 1
        elif "[ERR]" in line:
            self.at_responses["ERROR"] += 1
        elif "[TMO]" in line:
            self.at_responses["TIMEOUT"] += 1

    def print_status(self):
        """Print current WiFi status summary."""
        print("\n" + "=" * 55)
        print("  WiFi Connection Monitor - Status")
        print("=" * 55)
        status = "CONNECTED" if self.connected else "DISCONNECTED"
        print(f"  Status     : {status}")
        print(f"  SSID       : {self.ssid}")
        print(f"  IP Address : {self.ip_address}")
        print(f"  Connects   : {self.connect_count}")
        print(f"  Disconnects: {self.disconnect_count}")
        print(f"  AT OK      : {self.at_responses['OK']}")
        print(f"  AT ERROR   : {self.at_responses['ERROR']}")
        print(f"  AT TIMEOUT : {self.at_responses['TIMEOUT']}")
        if self.events:
            print(f"\n  Last 5 Events:")
            for ts, evt in self.events[-5:]:
                print(f"    [{ts}] {evt}")
        print("=" * 55)


def reader_thread(ser, monitor, running_event):
    """Read serial data and parse WiFi status."""
    line_buf = ""
    while running_event.is_set():
        try:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                text = data.decode("utf-8", errors="replace")
                sys.stdout.write(text)
                sys.stdout.flush()

                line_buf += text
                while "\n" in line_buf:
                    line, line_buf = line_buf.split("\n", 1)
                    monitor.parse_line(line)
            else:
                time.sleep(0.01)
        except (serial.SerialException, OSError):
            break


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_BAUD

    # List ports
    ports = serial.tools.list_ports.comports()
    if ports:
        print("Available ports:")
        for p in ports:
            print(f"  {p.device} - {p.description}")

    print(f"\n[*] Connecting to {port} at {baud} baud...")
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
    except serial.SerialException as e:
        print(f"[!] Cannot open {port}: {e}")
        sys.exit(1)

    monitor = WiFiMonitor()
    running = threading.Event()
    running.set()
    rx = threading.Thread(target=reader_thread, args=(ser, monitor, running), daemon=True)
    rx.start()

    print("[+] Monitoring WiFi status. Press 's' for status, 'q' to quit.\n")

    try:
        while True:
            try:
                cmd = input().strip().lower()
            except EOFError:
                break
            if cmd == "q":
                break
            elif cmd == "s":
                monitor.print_status()
    except KeyboardInterrupt:
        print("\n[*] Interrupted")

    running.clear()
    rx.join(timeout=1)
    ser.close()

    monitor.print_status()
    print("[*] Done")


if __name__ == "__main__":
    main()
