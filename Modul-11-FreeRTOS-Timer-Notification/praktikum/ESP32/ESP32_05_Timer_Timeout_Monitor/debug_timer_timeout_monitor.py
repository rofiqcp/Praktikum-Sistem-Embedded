#!/usr/bin/env python3
"""
Debug Script: ESP32_05_Timer_Timeout_Monitor
==============================================
Monitors heartbeat/timeout behavior. Can optionally send heartbeat
commands automatically. Tracks timeout events and response times.

Usage:
    python debug_timer_timeout_monitor.py [--port /dev/ttyUSB0] [--baud 115200] [--duration 60]
    python debug_timer_timeout_monitor.py --auto-heartbeat 3  # Send heartbeat every 3s
"""

import serial
import serial.tools.list_ports
import re
import sys
import argparse
import time
import csv
import threading
from datetime import datetime

def auto_detect_port():
    ports = serial.tools.list_ports.comports()
    for p in ports:
        if any(x in (p.vid or 0, p.pid or 0) for x in [0x10C4, 0x1A86, 0x0403]):
            return p.device
        if any(x in p.description.lower() for x in ['cp210', 'ch340', 'ftdi', 'usb']):
            return p.device
    return ports[0].device if ports else '/dev/ttyUSB0'

def main():
    parser = argparse.ArgumentParser(description='Timer Timeout Monitor Debug')
    parser.add_argument('--port', default=None, help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--duration', type=int, default=0, help='Duration (0=unlimited)')
    parser.add_argument('--output', default=None, help='CSV output file')
    parser.add_argument('--auto-heartbeat', type=float, default=0,
                       help='Auto-send heartbeat interval (seconds, 0=disabled)')
    args = parser.parse_args()

    port = args.port or auto_detect_port()
    output_file = args.output or f"timeout_monitor_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"{'='*60}")
    print(f"  Timer Timeout Monitor Debug")
    print(f"{'='*60}")
    print(f"  Port: {port} | Baud: {args.baud}")
    print(f"  Auto-heartbeat: {'every %.1fs' % args.auto_heartbeat if args.auto_heartbeat > 0 else 'disabled'}")
    print(f"{'='*60}")

    stats = {
        'lines': 0, 'heartbeats': 0, 'timeouts': 0,
        'warnings': 0, 'state_changes': 0,
    }
    state_history = []
    running = True

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
        print(f"\n[INFO] Connected. (Ctrl+C to stop)\n")

        # Auto-heartbeat thread
        def heartbeat_sender():
            while running:
                if args.auto_heartbeat > 0:
                    time.sleep(args.auto_heartbeat)
                    if running:
                        try:
                            ser.write(b'hb\n')
                            print(f"\033[95m[AUTO] Sent heartbeat\033[0m")
                        except:
                            pass
                else:
                    time.sleep(1)

        if args.auto_heartbeat > 0:
            hb_thread = threading.Thread(target=heartbeat_sender, daemon=True)
            hb_thread.start()

        start_time = time.time()

        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'elapsed_s', 'event', 'heartbeats', 'timeouts',
                           'state', 'ms_since_hb', 'raw'])

            while True:
                if args.duration > 0 and (time.time() - start_time) > args.duration:
                    break

                if ser.in_waiting > 0:
                    try:
                        line = ser.readline().decode('utf-8', errors='replace').strip()
                        if not line: continue
                        stats['lines'] += 1
                        elapsed = time.time() - start_time
                        ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                        event = ''
                        state = ''
                        ms_since = ''

                        if 'HEARTBEAT' in line and 'timer_reset' in line:
                            event = 'HEARTBEAT'
                            stats['heartbeats'] += 1
                            state = 'HEALTHY'

                        elif 'TIMEOUT' in line and 'no_heartbeat' in line:
                            event = 'TIMEOUT'
                            stats['timeouts'] += 1
                            state = 'TIMEOUT'

                        elif 'WARNING' in line:
                            event = 'WARNING'
                            stats['warnings'] += 1

                        elif 'STATUS' in line:
                            event = 'STATUS'
                            m = re.search(r'state=(\w+)', line)
                            if m: state = m.group(1)
                            m = re.search(r'ms_since_last_hb=(\d+)', line)
                            if m: ms_since = m.group(1)

                        if state and (not state_history or state_history[-1][1] != state):
                            state_history.append((elapsed, state))
                            stats['state_changes'] += 1

                        color = '\033[0m'
                        if event == 'HEARTBEAT': color = '\033[92m'
                        elif event == 'TIMEOUT': color = '\033[91m'
                        elif event == 'WARNING': color = '\033[93m'

                        hb_count = str(stats['heartbeats'])
                        to_count = str(stats['timeouts'])

                        print(f"{color}[{ts}] [{elapsed:7.2f}s] {line}\033[0m")

                        writer.writerow([ts, f'{elapsed:.3f}', event, hb_count, to_count,
                                       state, ms_since, line])

                    except UnicodeDecodeError:
                        pass

    except serial.SerialException as e:
        print(f"\n[ERROR] Serial: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print(f"\n[INFO] Interrupted")
    finally:
        running = False
        if 'ser' in locals(): ser.close()

    print(f"\n{'='*60}")
    print(f"  SUMMARY - Timeout Monitor")
    print(f"{'='*60}")
    print(f"  Lines: {stats['lines']}")
    print(f"  Heartbeats received: {stats['heartbeats']}")
    print(f"  Timeouts occurred:   {stats['timeouts']}")
    print(f"  Warning blinks:      {stats['warnings']}")
    print(f"  State changes:       {stats['state_changes']}")

    if state_history:
        print(f"\n  State transition history:")
        for t, s in state_history:
            print(f"    [{t:7.2f}s] -> {s}")

    print(f"\n  CSV: {output_file}")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()
