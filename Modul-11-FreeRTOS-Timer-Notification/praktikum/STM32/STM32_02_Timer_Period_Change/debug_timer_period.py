#!/usr/bin/env python3
"""Debug script for STM32_02_Timer_Period_Change - monitors period changes."""
import serial, time, csv, sys, re
from collections import defaultdict
from datetime import datetime

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILE = 'timer_period_change_log.csv'

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    print(f"[DEBUG] Connecting to {port} @ {BAUD_RATE}")
    stats = {'periods': defaultdict(int), 'total_blinks': 0, 'presses': 0}

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        with open(CSV_FILE, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'event', 'period_ms', 'count', 'tick'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"  {line}")
                ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                m = re.search(r'\[BLINK\].*#(\d+).*Period=(\d+)ms.*Tick=(\d+)', line)
                if m:
                    cnt, period, tick = int(m.group(1)), int(m.group(2)), int(m.group(3))
                    stats['total_blinks'] = cnt
                    stats['periods'][period] += 1
                    writer.writerow([ts, 'BLINK', period, cnt, tick])
                    f.flush()

                m2 = re.search(r'\[PERIOD\].*Changed to (\d+)ms.*press #(\d+)', line)
                if m2:
                    period, presses = int(m2.group(1)), int(m2.group(2))
                    stats['presses'] = presses
                    writer.writerow([ts, 'PERIOD_CHANGE', period, presses, ''])
                    f.flush()

    except KeyboardInterrupt:
        print(f"\n=== Period Change Stats ===")
        print(f"Total blinks: {stats['total_blinks']}")
        print(f"Button presses: {stats['presses']}")
        for p, c in sorted(stats['periods'].items()):
            print(f"  {p}ms period: {c} blinks")
        print(f"Log: {CSV_FILE}")
    except serial.SerialException as e:
        print(f"[ERROR] {e}")

if __name__ == '__main__':
    main()
