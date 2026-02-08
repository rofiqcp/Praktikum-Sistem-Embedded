#!/usr/bin/env python3
"""Debug script for STM32_10_Event_Group_Basic - sensor event tracking."""
import serial, time, csv, sys, re
from collections import defaultdict
from datetime import datetime

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILE = 'event_group_basic_log.csv'

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    print(f"[DEBUG] Connecting to {port} @ {BAUD_RATE}")

    sensor_counts = defaultdict(int)
    all_ready = []
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        with open(CSV_FILE, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'event', 'sensor', 'bits', 'tick'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"  {line}")
                ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                m_s = re.search(r'\[(\w+)\] Ready!.*Group=0x(\w+).*Tick=(\d+)', line)
                m_c = re.search(r'\[COLLECTOR\].*ALL.*#(\d+).*bits=0x(\w+).*Tick=(\d+)', line)

                if m_s:
                    sensor_counts[m_s.group(1)] += 1
                    writer.writerow([ts, 'SENSOR_READY', m_s.group(1), m_s.group(2), m_s.group(3)])
                    f.flush()
                elif m_c:
                    all_ready.append(int(m_c.group(3)))
                    writer.writerow([ts, 'ALL_READY', '', m_c.group(2), m_c.group(3)])
                    f.flush()
                    print(f"    *** ALL SENSORS READY ***")

    except KeyboardInterrupt:
        print(f"\n=== Event Group Stats ===")
        for s, c in sensor_counts.items():
            print(f"  {s}: {c} readings")
        print(f"All-ready events: {len(all_ready)}")
        if len(all_ready) >= 2:
            intervals = [all_ready[i]-all_ready[i-1] for i in range(1, len(all_ready))]
            print(f"Avg all-ready interval: {sum(intervals)/len(intervals):.0f}ms")
        print(f"Log: {CSV_FILE}")
    except serial.SerialException as e:
        print(f"[ERROR] {e}")

if __name__ == '__main__':
    main()
