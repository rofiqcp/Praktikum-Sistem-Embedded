"""
debug_analysis.py - HTTP Response Parser & Serial Monitor
==========================================================
Monitors STM32 debug output and parses HTTP GET responses,
tracking request statistics and response analysis.

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


class HTTPAnalyzer:
    """Analyzes HTTP responses captured from STM32 debug output."""

    def __init__(self):
        self.request_count = 0
        self.status_codes = defaultdict(int)
        self.response_times = []
        self.errors = []
        self.last_request_time = None
        self.last_status = None
        self.last_body_preview = ""
        self.total_bytes_rx = 0

    def parse_line(self, line):
        """Parse a debug output line for HTTP info."""
        # Track request start
        if "HTTP GET Request #" in line:
            self.request_count += 1
            self.last_request_time = time.time()

        # Parse status code
        status_match = re.search(r'Status Code:\s*(\d+)', line)
        if status_match:
            code = int(status_match.group(1))
            self.status_codes[code] += 1
            self.last_status = code
            if self.last_request_time:
                elapsed = time.time() - self.last_request_time
                self.response_times.append(elapsed)

        # Parse data length
        len_match = re.search(r'Data length:\s*(\d+)', line)
        if len_match:
            self.total_bytes_rx += int(len_match.group(1))

        # Track errors
        if "[ERR]" in line or "[TMO]" in line or "[FATAL]" in line:
            ts = datetime.now().strftime("%H:%M:%S")
            self.errors.append((ts, line.strip()))

        # Track body preview
        if "Body (" in line:
            self.last_body_preview = ""
        elif self.last_body_preview is not None and line.strip():
            if "--- End HTTP" not in line:
                self.last_body_preview += line

    def print_report(self):
        """Print HTTP analysis report."""
        print("\n" + "=" * 60)
        print("  HTTP GET Analysis Report")
        print("=" * 60)
        print(f"  Total Requests  : {self.request_count}")
        print(f"  Total Bytes RX  : {self.total_bytes_rx}")
        print(f"  Last Status     : {self.last_status}")

        if self.status_codes:
            print(f"\n  Status Code Distribution:")
            for code, count in sorted(self.status_codes.items()):
                bar = "#" * min(count, 30)
                print(f"    {code}: {count:4d} {bar}")

        if self.response_times:
            avg_time = sum(self.response_times) / len(self.response_times)
            min_time = min(self.response_times)
            max_time = max(self.response_times)
            print(f"\n  Response Times:")
            print(f"    Average : {avg_time:.2f}s")
            print(f"    Min     : {min_time:.2f}s")
            print(f"    Max     : {max_time:.2f}s")

        if self.errors:
            print(f"\n  Recent Errors ({len(self.errors)} total):")
            for ts, err in self.errors[-5:]:
                print(f"    [{ts}] {err[:60]}")

        # Success rate
        total_responses = sum(self.status_codes.values())
        if total_responses > 0:
            success = self.status_codes.get(200, 0)
            rate = (success / total_responses) * 100
            print(f"\n  Success Rate: {rate:.1f}% ({success}/{total_responses})")

        print("=" * 60)


def serial_reader(ser, analyzer, running_event):
    """Read serial data and feed to analyzer."""
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
                    analyzer.parse_line(line)
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
        print("Available serial ports:")
        for p in ports:
            print(f"  {p.device} - {p.description}")

    print(f"\n[*] Connecting to {port} at {baud} baud...")
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
    except serial.SerialException as e:
        print(f"[!] Cannot open {port}: {e}")
        sys.exit(1)

    analyzer = HTTPAnalyzer()
    running = threading.Event()
    running.set()
    rx = threading.Thread(target=serial_reader, args=(ser, analyzer, running), daemon=True)
    rx.start()

    print("[+] HTTP Monitor active. 'r' for report, 'q' to quit.\n")

    try:
        while True:
            try:
                cmd = input().strip().lower()
            except EOFError:
                break
            if cmd == "q":
                break
            elif cmd == "r":
                analyzer.print_report()
    except KeyboardInterrupt:
        print("\n[*] Interrupted")

    running.clear()
    rx.join(timeout=1)
    ser.close()

    analyzer.print_report()
    print("[*] Done")


if __name__ == "__main__":
    main()
