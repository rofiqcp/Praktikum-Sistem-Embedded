#!/usr/bin/env python3
"""
==========================================================================
FILE        : debug_nvs.py
PROJECT     : ESP32_08_NVS_Key_Value
MODUL       : 07 - SPI & Storage
DESCRIPTION : Parse NVS operations from serial output, track key-value
              pairs, and display storage usage statistics.

USAGE:
    python3 debug_nvs.py [PORT] [BAUD]
    python3 debug_nvs.py /dev/ttyUSB0 115200

SERIAL FORMATS EXPECTED:
    NVS:BOOT_COUNT=<n>,STATUS=<status>
    NVS:DEVICE_NAME=<name>,STATUS=<status>
    NVS:WIFI_SSID=<ssid>,STATUS=<status>
    NVS:THRESHOLD=<val>,RAW=<raw>,STATUS=<status>
    NVS:BLOB=CAL,OFFSET=<f>,GAIN=<f>,CH=<n>,VALID=<n>,STATUS=<status>
    NVS:KEY=<name>,TYPE=<type>
    NVS:STATS,USED=<n>,FREE=<n>,TOTAL=<n>,NS=<n>
    NVS:ERASE_KEY=<key>,STATUS=<status>
    NVS:ERASE_ALL=<ns>,STATUS=<status>
==========================================================================
"""

import sys
import re
import time
import serial
import serial.tools.list_ports
from datetime import datetime
from collections import OrderedDict

# ======================== Configuration ========================
DEFAULT_PORT = "/dev/ttyUSB0"
DEFAULT_BAUD = 115200

# Regex patterns for parsing NVS output
PATTERNS = {
    'boot_count': re.compile(r'NVS:BOOT_COUNT=(-?\d+),STATUS=(\w+)'),
    'device_name': re.compile(r'NVS:DEVICE_NAME=(.+?),STATUS=(\w+)'),
    'wifi_ssid': re.compile(r'NVS:WIFI_SSID=(.+?),STATUS=(\w+)'),
    'threshold': re.compile(r'NVS:THRESHOLD=([\d.]+),RAW=(-?\d+),STATUS=(\w+)'),
    'blob': re.compile(r'NVS:BLOB=(\w+),OFFSET=([\d.]+),GAIN=([\d.]+),CH=(\d+),VALID=(\d+),STATUS=(\w+)'),
    'key_list': re.compile(r'NVS:KEY=(\w+),TYPE=(\w+)'),
    'stats': re.compile(r'NVS:STATS,USED=(\d+),FREE=(\d+),TOTAL=(\d+),NS=(\d+)'),
    'erase_key': re.compile(r'NVS:ERASE_KEY=(\w+),STATUS=(\w+)'),
    'erase_all': re.compile(r'NVS:ERASE_ALL=(\w+),STATUS=(\w+)'),
}


class NVSMonitor:
    """Monitor and display NVS operations from serial output."""

    def __init__(self, port, baud):
        self.port = port
        self.baud = baud
        self.serial_conn = None

        # Key-value tracking
        self.kv_store = OrderedDict()
        self.key_types = {}
        self.operations_log = []

        # Storage stats
        self.nvs_used = 0
        self.nvs_free = 0
        self.nvs_total = 0
        self.nvs_namespaces = 0

        # Boot tracking
        self.boot_counts = []

    def connect(self):
        """Connect to serial port."""
        try:
            self.serial_conn = serial.Serial(self.port, self.baud, timeout=1)
            print(f"[OK] Connected to {self.port} at {self.baud} baud")
            time.sleep(2)
            self.serial_conn.reset_input_buffer()
            return True
        except serial.SerialException as e:
            print(f"[ERROR] Cannot connect to {self.port}: {e}")
            ports = serial.tools.list_ports.comports()
            if ports:
                print("Available ports:")
                for p in ports:
                    print(f"  {p.device} - {p.description}")
            return False

    def parse_line(self, line):
        """Parse a serial line for NVS data."""
        line = line.strip()
        if not line.startswith("NVS:"):
            return

        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

        # Boot count
        m = PATTERNS['boot_count'].search(line)
        if m:
            count = int(m.group(1))
            status = m.group(2)
            self.kv_store['boot_count'] = count
            self.key_types['boot_count'] = 'INT32'
            self.boot_counts.append(count)
            self.log_op(timestamp, 'READ/WRITE', 'boot_count', str(count), status)
            return

        # Device name
        m = PATTERNS['device_name'].search(line)
        if m:
            name = m.group(1)
            status = m.group(2)
            self.kv_store['device_name'] = name
            self.key_types['device_name'] = 'STRING'
            self.log_op(timestamp, 'READ/WRITE', 'device_name', name, status)
            return

        # WiFi SSID
        m = PATTERNS['wifi_ssid'].search(line)
        if m:
            ssid = m.group(1)
            status = m.group(2)
            self.kv_store['wifi_ssid'] = ssid
            self.key_types['wifi_ssid'] = 'STRING'
            self.log_op(timestamp, 'READ/WRITE', 'wifi_ssid', ssid, status)
            return

        # Threshold
        m = PATTERNS['threshold'].search(line)
        if m:
            val = float(m.group(1))
            raw = int(m.group(2))
            status = m.group(3)
            self.kv_store['threshold'] = val
            self.key_types['threshold'] = 'INT32 (float*100)'
            self.log_op(timestamp, 'READ/WRITE', 'threshold', f"{val} (raw={raw})", status)
            return

        # Blob
        m = PATTERNS['blob'].search(line)
        if m:
            blob_info = {
                'type': m.group(1),
                'offset': float(m.group(2)),
                'gain': float(m.group(3)),
                'channel': int(m.group(4)),
                'valid': bool(int(m.group(5))),
            }
            status = m.group(6)
            self.kv_store['blob_data'] = blob_info
            self.key_types['blob_data'] = 'BLOB'
            self.log_op(timestamp, 'READ/WRITE', 'blob_data', str(blob_info), status)
            return

        # Key listing
        m = PATTERNS['key_list'].search(line)
        if m:
            key = m.group(1)
            ktype = m.group(2)
            self.key_types[key] = ktype
            self.log_op(timestamp, 'LIST', key, f"type={ktype}", 'OK')
            return

        # Stats
        m = PATTERNS['stats'].search(line)
        if m:
            self.nvs_used = int(m.group(1))
            self.nvs_free = int(m.group(2))
            self.nvs_total = int(m.group(3))
            self.nvs_namespaces = int(m.group(4))
            self.log_op(timestamp, 'STATS', '-',
                        f"used={self.nvs_used} free={self.nvs_free}", 'OK')
            return

        # Erase key
        m = PATTERNS['erase_key'].search(line)
        if m:
            key = m.group(1)
            status = m.group(2)
            if key in self.kv_store:
                del self.kv_store[key]
            self.log_op(timestamp, 'ERASE', key, '-', status)
            return

        # Erase all
        m = PATTERNS['erase_all'].search(line)
        if m:
            ns = m.group(1)
            status = m.group(2)
            self.kv_store.clear()
            self.log_op(timestamp, 'ERASE_ALL', ns, '-', status)
            return

    def log_op(self, timestamp, op_type, key, value, status):
        """Log an NVS operation."""
        entry = {
            'time': timestamp,
            'op': op_type,
            'key': key,
            'value': value,
            'status': status,
        }
        self.operations_log.append(entry)
        status_icon = "✓" if status in ("OK", "ESP_OK") else "✗"
        print(f"  [{timestamp}] {status_icon} {op_type:<12} {key:<16} = {value}")

    def display_summary(self):
        """Display current NVS state summary."""
        print("\n" + "=" * 60)
        print(" NVS Key-Value Store Summary")
        print("=" * 60)

        if self.kv_store:
            print(f"\n{'Key':<18} {'Type':<16} {'Value'}")
            print("-" * 60)
            for key, value in self.kv_store.items():
                ktype = self.key_types.get(key, 'UNKNOWN')
                val_str = str(value)
                if len(val_str) > 40:
                    val_str = val_str[:37] + "..."
                print(f"{key:<18} {ktype:<16} {val_str}")
        else:
            print("\n  (No keys stored)")

        print(f"\n--- Storage Statistics ---")
        if self.nvs_total > 0:
            usage_pct = (self.nvs_used / self.nvs_total) * 100
            bar_len = 30
            filled = int(bar_len * self.nvs_used / self.nvs_total)
            bar = "█" * filled + "░" * (bar_len - filled)
            print(f"  [{bar}] {usage_pct:.1f}%")
            print(f"  Used: {self.nvs_used}  Free: {self.nvs_free}  Total: {self.nvs_total}")
            print(f"  Namespaces: {self.nvs_namespaces}")
        else:
            print("  (No stats received yet)")

        if self.boot_counts:
            print(f"\n--- Boot History ---")
            print(f"  Boot counts recorded: {self.boot_counts}")
            print(f"  Latest boot: #{self.boot_counts[-1]}")

        print(f"\n--- Operations Log ({len(self.operations_log)} total) ---")
        for entry in self.operations_log[-10:]:  # Show last 10
            status_icon = "✓" if entry['status'] in ("OK", "ESP_OK") else "✗"
            print(f"  [{entry['time']}] {status_icon} {entry['op']:<12} "
                  f"{entry['key']:<16} {entry['value']}")

    def run(self):
        """Main entry point: connect and monitor."""
        if not self.connect():
            return

        print("\n[INFO] Monitoring NVS operations... (Ctrl+C to stop)\n")
        print(f"{'Time':<16} {'Op':<12} {'Key':<16} {'Value'}")
        print("-" * 60)

        try:
            while True:
                if self.serial_conn.in_waiting:
                    try:
                        line = self.serial_conn.readline().decode('utf-8', errors='ignore')
                        self.parse_line(line)
                    except (serial.SerialException, UnicodeDecodeError):
                        pass
                else:
                    time.sleep(0.01)
        except KeyboardInterrupt:
            print("\n\n[INFO] Stopped by user.")
            self.display_summary()
        finally:
            if self.serial_conn:
                self.serial_conn.close()
                print("[OK] Serial connection closed.")


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_BAUD

    print("=" * 50)
    print(" NVS Key-Value Storage Monitor")
    print(f" Port: {port}  Baud: {baud}")
    print("=" * 50)

    monitor = NVSMonitor(port, baud)
    monitor.run()


if __name__ == "__main__":
    main()
