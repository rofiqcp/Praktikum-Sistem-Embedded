"""
debug_analysis.py - TCP Server + Serial Monitor for ESP-01
==========================================================
Runs a simple TCP echo server and simultaneously monitors
STM32 debug output via serial port.

Usage:
    python debug_analysis.py [SERIAL_PORT] [BAUDRATE] [TCP_PORT]
    python debug_analysis.py /dev/ttyUSB0 115200 8080
"""

import sys
import time
import socket
import threading
from datetime import datetime

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[!] pyserial not installed. Run: pip install pyserial")
    sys.exit(1)

DEFAULT_SERIAL_PORT = "/dev/ttyUSB0"
DEFAULT_BAUD = 115200
DEFAULT_TCP_PORT = 8080


class TCPServer:
    """Simple TCP echo server for testing STM32 TCP client."""

    def __init__(self, port):
        self.port = port
        self.running = False
        self.connections = 0
        self.messages_rx = 0
        self.server_socket = None

    def start(self):
        """Start the TCP server in background."""
        self.running = True
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.settimeout(1.0)
        self.server_socket.bind(("0.0.0.0", self.port))
        self.server_socket.listen(5)

        thread = threading.Thread(target=self._accept_loop, daemon=True)
        thread.start()
        print(f"[TCP Server] Listening on 0.0.0.0:{self.port}")

    def _accept_loop(self):
        """Accept incoming connections."""
        while self.running:
            try:
                client, addr = self.server_socket.accept()
                self.connections += 1
                ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                print(f"\n[TCP Server][{ts}] Connection #{self.connections} from {addr}")
                handler = threading.Thread(
                    target=self._handle_client, args=(client, addr), daemon=True
                )
                handler.start()
            except socket.timeout:
                continue
            except OSError:
                break

    def _handle_client(self, client, addr):
        """Handle a single client connection."""
        client.settimeout(10.0)
        try:
            data = client.recv(1024)
            if data:
                self.messages_rx += 1
                ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                text = data.decode("utf-8", errors="replace").strip()
                print(f"[TCP Server][{ts}] RX from {addr}: {text}")

                # Echo response
                response = f"Echo: {text} [Server Time: {ts}]\r\n"
                client.send(response.encode())
                print(f"[TCP Server][{ts}] TX to {addr}: {response.strip()}")
        except socket.timeout:
            print(f"[TCP Server] Client {addr} timed out")
        except Exception as e:
            print(f"[TCP Server] Error with {addr}: {e}")
        finally:
            client.close()

    def stop(self):
        """Stop the TCP server."""
        self.running = False
        if self.server_socket:
            self.server_socket.close()

    def print_stats(self):
        """Print server statistics."""
        print(f"\n[TCP Server Stats]")
        print(f"  Port        : {self.port}")
        print(f"  Connections : {self.connections}")
        print(f"  Messages RX : {self.messages_rx}")


def serial_reader(ser, running_event):
    """Read and display serial data from STM32."""
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


def main():
    serial_port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SERIAL_PORT
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_BAUD
    tcp_port = int(sys.argv[3]) if len(sys.argv) > 3 else DEFAULT_TCP_PORT

    # List serial ports
    ports = serial.tools.list_ports.comports()
    if ports:
        print("Available serial ports:")
        for p in ports:
            print(f"  {p.device} - {p.description}")

    # Start TCP server
    tcp_server = TCPServer(tcp_port)
    tcp_server.start()

    # Open serial port
    print(f"\n[*] Opening serial port {serial_port} at {baud} baud...")
    try:
        ser = serial.Serial(serial_port, baud, timeout=0.1)
    except serial.SerialException as e:
        print(f"[!] Cannot open {serial_port}: {e}")
        print("[*] TCP server still running. Press Ctrl+C to stop.")
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            tcp_server.stop()
            return

    running = threading.Event()
    running.set()
    rx = threading.Thread(target=serial_reader, args=(ser, running), daemon=True)
    rx.start()

    print("[+] Monitoring. Press 's' for stats, 'q' to quit.\n")

    try:
        while True:
            try:
                cmd = input().strip().lower()
            except EOFError:
                break
            if cmd == "q":
                break
            elif cmd == "s":
                tcp_server.print_stats()
    except KeyboardInterrupt:
        print("\n[*] Interrupted")

    running.clear()
    rx.join(timeout=1)
    ser.close()
    tcp_server.stop()
    tcp_server.print_stats()
    print("[*] Done")


if __name__ == "__main__":
    main()
