#!/usr/bin/env python3
"""
Debug script for STM32_01_Software_Timer_Basic
Monitors one-shot and auto-reload timer events via serial.
"""

import serial
import time
import csv
import sys
import re
from collections import defaultdict
from datetime import datetime

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILE = 'timer_basic_log.csv'

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    print(f"[DEBUG] Connecting to {port} @ {BAUD_RATE} baud")

    stats = defaultdict(lambda: {'count': 0, 'ticks': []})

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()
        print("[DEBUG] Connected. Logging timer events...\n")

        with open(CSV_FILE, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['timestamp', 'timer_type', 'count', 'tick', 'raw_line'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue

                print(f"  {line}")

                # Parse timer events
                m_oneshot = re.search(r'\[ONE-SHOT\].*Count=(\d+).*Tick=(\d+)', line)
                m_auto = re.search(r'\[AUTO-RELOAD\].*Count=(\d+).*Tick=(\d+)', line)

                ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                if m_oneshot:
                    count, tick = int(m_oneshot.group(1)), int(m_oneshot.group(2))
                    stats['one-shot']['count'] = count
                    stats['one-shot']['ticks'].append(tick)
                    writer.writerow([ts, 'ONE-SHOT', count, tick, line])
                    csvfile.flush()

                elif m_auto:
                    count, tick = int(m_auto.group(1)), int(m_auto.group(2))
                    stats['auto-reload']['count'] = count
                    stats['auto-reload']['ticks'].append(tick)
                    writer.writerow([ts, 'AUTO-RELOAD', count, tick, line])
                    csvfile.flush()

                    # Calculate interval
                    ticks = stats['auto-reload']['ticks']
                    if len(ticks) >= 2:
                        interval = ticks[-1] - ticks[-2]
                        print(f"    -> Auto-reload interval: {interval} ms")

    except KeyboardInterrupt:
        print("\n\n=== Timer Basic Statistics ===")
        for timer_type, data in stats.items():
            print(f"  {timer_type}: {data['count']} fires")
            if len(data['ticks']) >= 2:
                intervals = [data['ticks'][i] - data['ticks'][i-1]
                           for i in range(1, len(data['ticks']))]
                avg = sum(intervals) / len(intervals)
                print(f"    Avg interval: {avg:.1f} ms")
                print(f"    Min: {min(intervals)} ms, Max: {max(intervals)} ms")
        print(f"\nLog saved to {CSV_FILE}")
    except serial.SerialException as e:
        print(f"[ERROR] Serial: {e}")

if __name__ == '__main__':
    main()
