"""
debug_analysis.py - AT Command Interactive Terminal for ESP-01
=============================================================
Connects to STM32 UART1 debug port and provides an interactive
AT command terminal for testing ESP-01 communication.

Usage:
    python debug_analysis.py [PORT] [BAUDRATE]
    python debug_analysis.py COM3 115200
    python debug_analysis.py /dev/ttyUSB0
"""

import sys
import time
import threading

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[!] pyserial not installed. Run: pip install pyserial")
    sys.exit(1)

DEFAULT_PORT = "/dev/ttyUSB0"
DEFAULT_BAUD = 115200

# AT commands reference
AT_COMMANDS = {
    "1": ("AT",         "Test AT connection"),
    "2": ("AT+GMR",     "Get firmware version"),
    "3": ("AT+RST",     "Reset module"),
    "4": ("AT+CWMODE?", "Query WiFi mode"),
    "5": ("AT+CWLAP",   "List available APs"),
    "6": ("AT+CIFSR",   "Get IP address"),
    "7": ("AT+CWMODE=1","Set Station mode"),
    "8": ("AT+CWMODE=2","Set SoftAP mode"),
}


def list_serial_ports():
    """List available serial ports."""
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("[!] No serial ports found")
        return
    print("\nAvailable serial ports:")
    print("-" * 50)
    for p in ports:
        print(f"  {p.device:20s} - {p.description}")
    print()


def reader_thread(ser, running_event):
    """Background thread to read serial data."""
    while running_event.is_set():
        try:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                text = data.decode("utf-8", errors="replace")
                sys.stdout.write(text)
                sys.stdout.flush()
            else:
                time.sleep(0.01)
        except (serial.SerialException, OSError):
            break


def show_menu():
    """Show AT command menu."""
    print("\n" + "=" * 50)
    print("  ESP-01 AT Command Interactive Terminal")
    print("=" * 50)
    print("\nQuick Commands:")
    for key, (cmd, desc) in AT_COMMANDS.items():
        print(f"  [{key}] {cmd:20s} - {desc}")
    print(f"\n  [c] Custom AT command")
    print(f"  [m] Show this menu")
    print(f"  [q] Quit")
    print("-" * 50)


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_BAUD

    list_serial_ports()

    print(f"[*] Connecting to {port} at {baud} baud...")
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
    except serial.SerialException as e:
        print(f"[!] Cannot open {port}: {e}")
        sys.exit(1)

    print(f"[+] Connected to {port}")

    running = threading.Event()
    running.set()
    rx_thread = threading.Thread(target=reader_thread, args=(ser, running), daemon=True)
    rx_thread.start()

    show_menu()

    try:
        while True:
            try:
                choice = input("\n> ").strip()
            except EOFError:
                break

            if choice == "q":
                break
            elif choice == "m":
                show_menu()
            elif choice == "c":
                cmd = input("Enter AT command: ").strip()
                if cmd:
                    if not cmd.endswith("\r\n"):
                        cmd += "\r\n"
                    print(f"[TX] {cmd.strip()}")
                    ser.write(cmd.encode())
                    time.sleep(1)
            elif choice in AT_COMMANDS:
                cmd_str, desc = AT_COMMANDS[choice]
                print(f"[TX] {cmd_str} ({desc})")
                ser.write((cmd_str + "\r\n").encode())
                wait_time = 5 if "RST" in cmd_str else 2
                time.sleep(wait_time)
            else:
                # Treat as raw AT command
                if choice:
                    if not choice.endswith("\r\n"):
                        choice += "\r\n"
                    print(f"[TX] {choice.strip()}")
                    ser.write(choice.encode())
                    time.sleep(1)
    except KeyboardInterrupt:
        print("\n[*] Interrupted")

    running.clear()
    rx_thread.join(timeout=1)
    ser.close()
    print("[*] Disconnected")


if __name__ == "__main__":
    main()
