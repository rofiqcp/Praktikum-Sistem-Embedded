#!/usr/bin/env python3
"""
STM32_10_BLE_HM10 - BLE Data Logger & HM-10 AT Command Tool
=============================================================
Tools:
1. BLE data logger (serial monitor with parsing)
2. HM-10 AT command interactive tool
3. Connection status tracker
4. Data throughput analyzer

Usage:
  python debug_analysis.py --logger PORT      # BLE data logger
  python debug_analysis.py --at PORT          # Interactive AT command tool
  python debug_analysis.py --analyze          # Analyze BLE communication demo
"""

import time
import sys
import argparse
from datetime import datetime
from collections import deque

# HM-10 AT Command definitions
HM10_COMMANDS = {
    "AT":          ("Test connection",           "OK"),
    "AT+ADDR?":    ("Get MAC address",           "OK+ADDR:"),
    "AT+BAUD?":    ("Get baud rate",             "OK+Get:"),
    "AT+CHAR?":    ("Get characteristic UUID",   "OK+Get:"),
    "AT+CONN?":    ("Get connection status",     "OK+CONN"),
    "AT+DISC?":    ("Discover devices",          "OK+DISCS"),
    "AT+HELP?":    ("Get help",                  "OK+HELP"),
    "AT+MODE?":    ("Get work mode",             "OK+Get:"),
    "AT+NAME?":    ("Get device name",           "OK+NAME:"),
    "AT+NOTI?":    ("Get notification state",    "OK+Get:"),
    "AT+ROLE?":    ("Get role (0=periph,1=cent)","OK+Get:"),
    "AT+RSSI?":    ("Get signal strength",       "OK+RSSI:"),
    "AT+UUID?":    ("Get service UUID",          "OK+Get:"),
    "AT+VERR?":    ("Get firmware version",      "OK+Get:"),
}

HM10_SET_COMMANDS = {
    "AT+NAME":     "Set device name (e.g., AT+NAMEMyDevice)",
    "AT+ROLE":     "Set role: 0=Peripheral, 1=Central",
    "AT+BAUD":     "Set baud: 0=9600,1=19200,...,4=115200",
    "AT+MODE":     "Set mode: 0=data,1=remote,2=both",
    "AT+NOTI":     "Set notify: 0=off, 1=on",
    "AT+UUID":     "Set service UUID (e.g., AT+UUID0xFFE0)",
    "AT+CHAR":     "Set characteristic UUID",
    "AT+CONN":     "Connect to address (e.g., AT+CONNaabbccddee)",
}

class BLEDataLogger:
    """Log and analyze BLE data from HM-10 module."""

    def __init__(self):
        self.log_entries = []
        self.rx_count = 0
        self.tx_count = 0
        self.connection_events = []
        self.throughput_window = deque(maxlen=100)
        self.start_time = time.time()

    def log_event(self, direction, data, event_type="DATA"):
        """Log a BLE event."""
        entry = {
            "timestamp": datetime.now().strftime("%H:%M:%S.%f")[:-3],
            "elapsed": f"{time.time() - self.start_time:.3f}",
            "direction": direction,
            "type": event_type,
            "data": data,
            "length": len(data),
        }
        self.log_entries.append(entry)

        if direction == "RX":
            self.rx_count += 1
        else:
            self.tx_count += 1

        self.throughput_window.append((time.time(), len(data)))

        # Check for connection events
        if "OK+CONN" in data:
            self.connection_events.append(("CONNECTED", entry["timestamp"]))
        elif "OK+LOST" in data:
            self.connection_events.append(("DISCONNECTED", entry["timestamp"]))

        return entry

    def get_throughput(self, window_sec=10):
        """Calculate throughput over a time window."""
        now = time.time()
        total_bytes = sum(size for ts, size in self.throughput_window
                         if now - ts < window_sec)
        return total_bytes / window_sec if window_sec > 0 else 0

    def print_stats(self):
        """Print communication statistics."""
        elapsed = time.time() - self.start_time
        print(f"\n{'=' * 50}")
        print(f"  BLE Communication Statistics")
        print(f"{'=' * 50}")
        print(f"  Duration    : {elapsed:.1f} seconds")
        print(f"  RX Messages : {self.rx_count}")
        print(f"  TX Messages : {self.tx_count}")
        print(f"  Throughput  : {self.get_throughput():.1f} bytes/sec")
        print(f"  Connections : {len([e for e in self.connection_events if e[0] == 'CONNECTED'])}")
        print(f"  Disconnects : {len([e for e in self.connection_events if e[0] == 'DISCONNECTED'])}")
        if self.connection_events:
            print(f"  Last Event  : {self.connection_events[-1][0]} at {self.connection_events[-1][1]}")
        print(f"{'=' * 50}\n")

def run_logger(port_name="/dev/ttyUSB0", baudrate=115200):
    """Run BLE data logger on serial port."""
    try:
        import serial
    except ImportError:
        print("ERROR: pyserial not installed. Run: pip install pyserial")
        print("\nRunning demo mode...\n")
        run_logger_demo()
        return

    print(f"Opening {port_name} at {baudrate} baud...")
    try:
        ser = serial.Serial(port_name, baudrate, timeout=0.1)
    except serial.SerialException as e:
        print(f"ERROR: {e}")
        run_logger_demo()
        return

    logger = BLEDataLogger()
    print(f"BLE Data Logger active - Press Ctrl+C to stop\n")

    try:
        line_buf = ""
        while True:
            data = ser.read(128)
            if data:
                text = data.decode("utf-8", errors="replace")
                line_buf += text

                while "\r\n" in line_buf:
                    line, line_buf = line_buf.split("\r\n", 1)
                    if line.strip():
                        # Determine direction
                        if line.startswith("[BLE] TX") or line.startswith("[APP] Sent"):
                            entry = logger.log_event("TX", line, "AT_CMD")
                        elif line.startswith("[BLE] RX"):
                            entry = logger.log_event("RX", line, "AT_RESP")
                        elif line.startswith("[BLE] RX Data"):
                            entry = logger.log_event("RX", line, "BLE_DATA")
                        elif "CONNECTED" in line or "DISCONNECTED" in line:
                            entry = logger.log_event("--", line, "EVENT")
                        else:
                            entry = logger.log_event("--", line, "INFO")

                        print(f"[{entry['timestamp']}] {entry['direction']} "
                              f"({entry['type']}) {entry['data']}")

            time.sleep(0.01)

    except KeyboardInterrupt:
        logger.print_stats()
        ser.close()

def run_at_tool(port_name="/dev/ttyUSB0", baudrate=9600):
    """Interactive HM-10 AT command tool (direct to HM-10)."""
    try:
        import serial
    except ImportError:
        print("ERROR: pyserial not installed. Run: pip install pyserial")
        print("\nRunning demo mode...\n")
        run_at_demo()
        return

    print(f"Opening {port_name} at {baudrate} baud (direct HM-10 connection)...")
    try:
        ser = serial.Serial(port_name, baudrate, timeout=1)
    except serial.SerialException as e:
        print(f"ERROR: {e}")
        run_at_demo()
        return

    print("\nHM-10 AT Command Tool")
    print("Type 'help' for command list, 'quit' to exit\n")

    try:
        while True:
            cmd = input("HM-10> ").strip()
            if not cmd:
                continue
            if cmd.lower() == "quit":
                break
            if cmd.lower() == "help":
                print_at_help()
                continue
            if cmd.lower() == "scan":
                cmd = "AT+DISC?"
            if cmd.lower() == "status":
                cmd = "AT+CONN?"

            # Send command
            ser.write(cmd.encode())
            time.sleep(0.5)

            # Read response
            resp = ser.read(256).decode("utf-8", errors="replace")
            if resp:
                print(f"  Response: {resp}")
            else:
                print("  (no response)")

    except KeyboardInterrupt:
        pass
    finally:
        ser.close()

def print_at_help():
    """Print AT command help."""
    print("\n--- Query Commands ---")
    for cmd, (desc, expect) in HM10_COMMANDS.items():
        print(f"  {cmd:16s} {desc}")
    print("\n--- Set Commands ---")
    for cmd, desc in HM10_SET_COMMANDS.items():
        print(f"  {cmd:16s} {desc}")
    print("\n--- Shortcuts ---")
    print(f"  {'scan':16s} Discover nearby BLE devices")
    print(f"  {'status':16s} Check connection status")
    print(f"  {'help':16s} Show this help")
    print(f"  {'quit':16s} Exit tool")
    print()

def run_logger_demo():
    """Demo mode for BLE data logger."""
    logger = BLEDataLogger()
    print("=== BLE Data Logger Demo ===\n")

    events = [
        ("TX", "AT",                  "AT_CMD"),
        ("RX", "OK",                  "AT_RESP"),
        ("TX", "AT+NAMEstm32ble",     "AT_CMD"),
        ("RX", "OK+Set:stm32ble",     "AT_RESP"),
        ("TX", "AT+ROLE0",            "AT_CMD"),
        ("RX", "OK+Set:0",            "AT_RESP"),
        ("TX", "AT+ADDR?",            "AT_CMD"),
        ("RX", "OK+ADDR:AABBCCDDEE", "AT_RESP"),
        ("--", "OK+CONN",             "EVENT"),
        ("RX", "Hello from phone",    "BLE_DATA"),
        ("TX", "ACK:Hello from phone","BLE_DATA"),
        ("RX", "LED ON",              "BLE_DATA"),
        ("TX", "ACK:LED ON",          "BLE_DATA"),
        ("--", "OK+LOST",             "EVENT"),
    ]

    for direction, data, evt_type in events:
        entry = logger.log_event(direction, data, evt_type)
        print(f"[{entry['timestamp']}] {direction:2s} ({evt_type:8s}) {data}")
        time.sleep(0.2)

    logger.print_stats()

def run_at_demo():
    """Demo mode for AT command tool."""
    print("\n=== HM-10 AT Command Tool (Demo Mode) ===\n")

    simulated = [
        ("AT",               "OK"),
        ("AT+NAME?",         "OK+NAME:stm32ble"),
        ("AT+ADDR?",         "OK+ADDR:AABBCCDDEE"),
        ("AT+ROLE?",         "OK+Get:0"),
        ("AT+UUID?",         "OK+Get:0xFFE0"),
        ("AT+BAUD?",         "OK+Get:0 (9600)"),
        ("AT+CONN?",         "OK+CONNF (not connected)"),
        ("AT+NAMEMyBLE",     "OK+Set:MyBLE"),
        ("AT+DISC?",         "OK+DISCS\r\nOK+DIS0:112233445566\r\nOK+NAME:Device1\r\nOK+DISCE"),
    ]

    for cmd, response in simulated:
        print(f"HM-10> {cmd}")
        print(f"  Response: {response}")
        print()

def analyze_communication():
    """Analyze BLE communication patterns."""
    print("=" * 60)
    print("  BLE HM-10 Communication Analysis")
    print("=" * 60)

    print("\n--- AT Command Reference ---")
    print_at_help()

    print("--- Typical Init Sequence ---")
    init_seq = [
        ("AT",               "OK",                "Test connection"),
        ("AT+NAMEstm32ble",  "OK+Set:stm32ble",   "Set device name"),
        ("AT+ROLE0",         "OK+Set:0",           "Set peripheral role"),
        ("AT+UUID0xFFE0",    "OK+Set:0xFFE0",      "Set service UUID"),
        ("AT+ADDR?",         "OK+ADDR:AABBCC...",  "Get MAC address"),
    ]
    for cmd, resp, desc in init_seq:
        print(f"  TX: {cmd:24s} -> RX: {resp:24s} [{desc}]")

    print("\n--- Connection Flow ---")
    flow = [
        "1. HM-10 powers up in advertising mode",
        "2. Phone/central scans and finds 'stm32ble'",
        "3. Phone connects -> HM-10 sends 'OK+CONN' to STM32",
        "4. Data exchange via UART (transparent mode)",
        "5. Phone disconnects -> HM-10 sends 'OK+LOST' to STM32",
        "6. HM-10 returns to advertising mode",
    ]
    for step in flow:
        print(f"  {step}")

    print("\n--- Wiring Diagram ---")
    print("  STM32 PA2 (TX) -----> HM-10 RX")
    print("  STM32 PA3 (RX) <----- HM-10 TX")
    print("  3.3V           -----> HM-10 VCC")
    print("  GND            -----> HM-10 GND")
    print("  STM32 PC13     -----> LED (connection indicator)")
    print()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="BLE HM-10 Debug & Analysis Tool")
    parser.add_argument("--logger", type=str, metavar="PORT", nargs="?", const="/dev/ttyUSB0",
                        help="Run BLE data logger on serial port")
    parser.add_argument("--at", type=str, metavar="PORT", nargs="?", const="/dev/ttyUSB0",
                        help="Interactive AT command tool")
    parser.add_argument("--analyze", action="store_true", help="Analyze BLE communication")
    args = parser.parse_args()

    if args.logger:
        run_logger(args.logger)
    elif args.at:
        run_at_tool(args.at)
    elif args.analyze:
        analyze_communication()
    else:
        analyze_communication()
