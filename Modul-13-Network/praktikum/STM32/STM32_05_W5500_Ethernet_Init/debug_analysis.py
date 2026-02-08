#!/usr/bin/env python3
"""
Debug Analysis - STM32_05_W5500_Ethernet_Init
Network configuration display and SPI traffic analysis for W5500 initialization.

Usage:
    python3 debug_analysis.py [--port /dev/ttyUSB0] [--baud 115200]
"""

import serial
import time
import sys
import re
import argparse
from datetime import datetime


def parse_args():
    parser = argparse.ArgumentParser(description="W5500 Ethernet Init - Debug Analysis")
    parser.add_argument("--port", default="/dev/ttyUSB0", help="Serial port")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate")
    parser.add_argument("--timeout", type=float, default=30.0, help="Capture duration (seconds)")
    return parser.parse_args()


class W5500InitAnalyzer:
    """Analyze W5500 initialization and network config from UART output."""

    def __init__(self):
        self.lines = []
        self.version = None
        self.mac = None
        self.ip = None
        self.subnet = None
        self.gateway = None
        self.phy_status = []
        self.link_states = []
        self.errors = []

    def parse_line(self, line):
        """Parse a single UART output line."""
        self.lines.append(line)

        # W5500 Version
        m = re.search(r"W5500 Version:\s*0x([0-9A-Fa-f]+)", line)
        if m:
            self.version = int(m.group(1), 16)

        # MAC address
        m = re.search(r"MAC:\s*([0-9A-Fa-f:]+)", line)
        if m:
            self.mac = m.group(1)

        # IP address
        m = re.search(r"IP:\s*(\d+\.\d+\.\d+\.\d+)", line)
        if m:
            self.ip = m.group(1)

        # Subnet
        m = re.search(r"Sub:\s*(\d+\.\d+\.\d+\.\d+)", line)
        if m:
            self.subnet = m.group(1)

        # Gateway
        m = re.search(r"GW:\s*(\d+\.\d+\.\d+\.\d+)", line)
        if m:
            self.gateway = m.group(1)

        # PHY Config
        m = re.search(r"PHY.*?0x([0-9A-Fa-f]+)", line)
        if m:
            self.phy_status.append(int(m.group(1), 16))

        # Link status
        m = re.search(r"Link[=:]\s*(\w+)", line)
        if m:
            self.link_states.append(m.group(1))

        # Errors
        if "!!!" in line or "error" in line.lower() or "fail" in line.lower():
            self.errors.append(line.strip())

    def print_report(self):
        """Print comprehensive analysis report."""
        print("\n" + "=" * 60)
        print("  W5500 Ethernet Init - Debug Analysis Report")
        print("=" * 60)
        print(f"  Timestamp: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"  Total lines captured: {len(self.lines)}")

        # Chip verification
        print("\n--- Chip Verification ---")
        if self.version is not None:
            expected = 0x04
            status = "OK" if self.version == expected else "MISMATCH"
            print(f"  Version: 0x{self.version:02X} (expected 0x{expected:02X}) [{status}]")
        else:
            print("  Version: NOT DETECTED")

        # Network config
        print("\n--- Network Configuration ---")
        print(f"  MAC Address : {self.mac or 'NOT SET'}")
        print(f"  IP Address  : {self.ip or 'NOT SET'}")
        print(f"  Subnet Mask : {self.subnet or 'NOT SET'}")
        print(f"  Gateway     : {self.gateway or 'NOT SET'}")

        # Validate IP
        if self.ip:
            parts = self.ip.split('.')
            if all(0 <= int(p) <= 255 for p in parts):
                print(f"  IP Valid    : YES")
            else:
                print(f"  IP Valid    : NO (out of range)")

        # PHY Status
        print("\n--- PHY Status History ---")
        if self.phy_status:
            for i, phy in enumerate(self.phy_status):
                link = "UP" if (phy & 0x01) else "DOWN"
                speed = "100Mbps" if (phy & 0x02) else "10Mbps"
                duplex = "Full" if (phy & 0x04) else "Half"
                print(f"  [{i}] PHY=0x{phy:02X} Link={link} Speed={speed} Duplex={duplex}")
        else:
            print("  No PHY readings captured")

        # Link stability
        print("\n--- Link Stability ---")
        if self.link_states:
            up_count = sum(1 for s in self.link_states if s == "UP")
            total = len(self.link_states)
            pct = (up_count / total) * 100
            print(f"  Readings: {total}  UP: {up_count}  DOWN: {total - up_count}")
            print(f"  Uptime: {pct:.1f}%")
        else:
            print("  No link readings captured")

        # SPI Communication
        print("\n--- SPI Communication ---")
        spi_ops = sum(1 for l in self.lines if "Write" in l or "Read" in l or "SPI" in l)
        print(f"  SPI operations detected: {spi_ops}")
        print(f"  Interface: SPI1 (SCK=PA5, MISO=PA6, MOSI=PA7, CS=PA4)")
        print(f"  Clock: 9 MHz (72MHz / 8)")

        # Errors
        print("\n--- Errors ---")
        if self.errors:
            for err in self.errors:
                print(f"  [!] {err}")
        else:
            print("  No errors detected")

        print("\n" + "=" * 60)


def main():
    args = parse_args()
    analyzer = W5500InitAnalyzer()

    print(f"W5500 Ethernet Init Debug Analysis")
    print(f"Port: {args.port}  Baud: {args.baud}  Duration: {args.timeout}s")
    print(f"Waiting for data...\n")

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        start = time.time()

        while (time.time() - start) < args.timeout:
            if ser.in_waiting:
                try:
                    line = ser.readline().decode("utf-8", errors="replace").strip()
                    if line:
                        ts = time.time() - start
                        print(f"[{ts:7.2f}s] {line}")
                        analyzer.parse_line(line)
                except Exception as e:
                    print(f"  [decode error: {e}]")
            else:
                time.sleep(0.05)

        ser.close()

    except serial.SerialException as e:
        print(f"Serial error: {e}")
        print("Running in demo mode with sample data...\n")
        demo_lines = [
            "=== STM32 W5500 Ethernet Init ===",
            "SPI1: SCK=PA5 MISO=PA6 MOSI=PA7 CS=PA4",
            "W5500 Version: 0x04 (expected 0x04)",
            "W5500 Network configured:",
            "  MAC: DE:AD:BE:EF:01:05",
            "  IP:  192.168.1.100",
            "  Sub: 255.255.255.0",
            "  GW:  192.168.1.1",
            "Read-back IP: 192.168.1.100",
            "PHY Config: 0x07",
            "  Link:   UP",
            "  Speed:  100Mbps",
            "[Cycle 1] Status Check:",
            "  PHY=0x07 Link=UP",
        ]
        for line in demo_lines:
            print(f"  [demo] {line}")
            analyzer.parse_line(line)

    analyzer.print_report()


if __name__ == "__main__":
    main()
