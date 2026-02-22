#!/usr/bin/env python3
"""
==========================================================
 DEBUG & ANALYSIS SCRIPT: ESP32_08_Mutex_Shared_Resource
 Modul 10 - ESP32_08_Mutex_Shared_Resource
==========================================================
 Tool untuk monitoring serial output dari ESP32/STM32,
 parsing data, dan analisa performa FreeRTOS.

 Penggunaan:
   python3 debug_esp32_08_mutex_shared_resource.py [PORT] [BAUDRATE]
   
 Contoh:
   python3 debug_esp32_08_mutex_shared_resource.py /dev/ttyUSB0 115200
   python3 debug_esp32_08_mutex_shared_resource.py COM3 115200
==========================================================
"""

import serial
import serial.tools.list_ports
import sys
import time
import signal
import json
import csv
from datetime import datetime
from collections import deque

class SerialDebugger:
    """Serial port debugger and data analyzer for FreeRTOS exercises"""
    
    def __init__(self, port=None, baudrate=115200, timeout=1):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial_conn = None
        self.data_lines = []
        self.running = True
        self.start_time = None
        self.log_file = f"log_{exercise_name.lower()}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        
        signal.signal(signal.SIGINT, self._signal_handler)
    
    def _signal_handler(self, sig, frame):
        print("\n\n[INFO] Stopping capture (Ctrl+C)...")
        self.running = False
    
    @staticmethod
    def list_ports():
        """List all available serial ports"""
        ports = serial.tools.list_ports.comports()
        print("\n Available Serial Ports:")
        print("-" * 50)
        for p in ports:
            print(f"  {p.device} - {p.description}")
        if not ports:
            print("  (No serial ports found)")
        print("-" * 50)
        return ports
    
    def connect(self):
        """Connect to serial port"""
        if not self.port:
            ports = self.list_ports()
            if ports:
                self.port = ports[0].device
                print(f"[AUTO] Using port: {self.port}")
            else:
                print("[ERROR] No serial ports available!")
                return False
        
        try:
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE
            )
            print(f"[OK] Connected to {self.port} @ {self.baudrate} baud")
            self.start_time = time.time()
            return True
        except serial.SerialException as e:
            print(f"[ERROR] Cannot open {self.port}: {e}")
            return False
    
    def capture(self, duration=30):
        """Capture serial data for specified duration"""
        print(f"\n[CAPTURE] Recording for {duration}s... (Ctrl+C to stop early)")
        print("=" * 70)
        
        end_time = time.time() + duration
        line_count = 0
        
        with open(self.log_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_ms', 'raw_data'])
            
            while self.running and time.time() < end_time:
                try:
                    if self.serial_conn and self.serial_conn.in_waiting:
                        raw = self.serial_conn.readline()
                        try:
                            line = raw.decode('utf-8', errors='replace').strip()
                        except:
                            line = str(raw)
                        
                        if line:
                            elapsed = int((time.time() - self.start_time) * 1000)
                            timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                            
                            self.data_lines.append(line)
                            writer.writerow([timestamp, elapsed, line])
                            
                            line_count += 1
                            # Color-code output
                            if 'error' in line.lower() or 'fail' in line.lower():
                                print(f"  [{timestamp}] \033[91m{line}\033[0m")
                            elif 'warn' in line.lower():
                                print(f"  [{timestamp}] \033[93m{line}\033[0m")
                            elif 'ok' in line.lower() or 'success' in line.lower():
                                print(f"  [{timestamp}] \033[92m{line}\033[0m")
                            else:
                                print(f"  [{timestamp}] {line}")
                except Exception as e:
                    print(f"[WARN] Read error: {e}")
        
        print("=" * 70)
        print(f"[DONE] Captured {line_count} lines in {duration}s")
        print(f"[SAVE] Log saved to: {self.log_file}")
    
    def extract_numeric_values(self, keyword):
        """Extract numeric values from lines containing keyword"""
        values = []
        for line in self.data_lines:
            if keyword.lower() in line.lower():
                for word in line.replace(',', ' ').replace(':', ' ').split():
                    try:
                        values.append(float(word))
                    except ValueError:
                        pass
        return values

    def analyze_sync_metrics(self):
        """Analyze semaphore/mutex patterns"""
        takes = [l for l in self.data_lines if 'take' in l.lower() or 'lock' in l.lower() or 'acquire' in l.lower()]
        gives = [l for l in self.data_lines if 'give' in l.lower() or 'unlock' in l.lower() or 'release' in l.lower()]
        blocks = [l for l in self.data_lines if 'block' in l.lower() or 'wait' in l.lower()]
        
        print(f"\n{'='*60}")
        print(f"  SYNCHRONIZATION ANALYSIS")
        print(f"{'='*60}")
        print(f"  Total Take/Lock   : {len(takes)}")
        print(f"  Total Give/Unlock : {len(gives)}")
        print(f"  Blocking events   : {len(blocks)}")
        if len(takes) > 0 and len(gives) > 0:
            print(f"  Balance (give-take): {len(gives) - len(takes)}")
        print(f"{'='*60}")

    
    def print_summary(self):
        """Print capture summary"""
        print(f"\n============================================================")
        print(f"  CAPTURE SUMMARY - ESP32_08_Mutex_Shared_Resource")
        print(f"============================================================")
        print(f"  Port          : {self.port}")
        print(f"  Baudrate      : {self.baudrate}")
        print(f"  Total lines   : {len(self.data_lines)}")
        print(f"  Log file      : {self.log_file}")
        
        # Count unique patterns
        patterns = {}
        for line in self.data_lines:
            key = line.split(':')[0].strip() if ':' in line else line[:30]
            patterns[key] = patterns.get(key, 0) + 1
        
        print(f"\n  Top message patterns:")
        for pat, count in sorted(patterns.items(), key=lambda x: -x[1])[:10]:
            print(f"    {count:4d}x | {pat[:50]}")
        print(f"============================================================")
    
    def close(self):
        """Close serial connection"""
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
            print("[OK] Serial port closed")


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else None
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
    duration = int(sys.argv[3]) if len(sys.argv) > 3 else 30
    
    print(f"\n============================================================")
    print(f"  ESP32_08_Mutex_Shared_Resource - Debug & Analysis Tool")
    print(f"  Modul 10 - ESP32_08_Mutex_Shared_Resource")
    print(f"============================================================")
    
    dbg = SerialDebugger(port=port, baudrate=baudrate)
    
    if not dbg.connect():
        print("\n[TIP] Jalankan tanpa argumen untuk auto-detect port")
        print("[TIP] Atau: python3 {sys.argv[0]} /dev/ttyUSB0 115200 30")
        return
    
    try:
        dbg.capture(duration=duration)
        dbg.print_summary()
        # Run analysis
        for method_name in dir(dbg):
            if method_name.startswith('analyze_'):
                getattr(dbg, method_name)()
    finally:
        dbg.close()


if __name__ == "__main__":
    main()
