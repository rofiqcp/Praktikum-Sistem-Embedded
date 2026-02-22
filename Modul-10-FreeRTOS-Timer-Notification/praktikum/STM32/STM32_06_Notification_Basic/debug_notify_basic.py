#!/usr/bin/env python3
"""Debug script for STM32_06_Notification_Basic - button notification events."""
import serial, time, csv, sys, re
from datetime import datetime

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILE = 'notification_basic_log.csv'

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    print(f"[DEBUG] Connecting to {port} @ {BAUD_RATE}")

    events = []
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        with open(CSV_FILE, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'event', 'notify_count', 'isr_count', 'tick'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"  {line}")
                ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                m = re.search(r'\[NOTIFY\].*#(\d+).*ISR=(\d+).*Tick=(\d+)', line)
                if m:
                    events.append({'tick': int(m.group(3))})
                    writer.writerow([ts, 'NOTIFY', m.group(1), m.group(2), m.group(3)])
                    f.flush()

                    if len(events) >= 2:
                        gap = events[-1]['tick'] - events[-2]['tick']
                        print(f"    -> Gap since last: {gap}ms")

    except KeyboardInterrupt:
        print(f"\n=== Notification Basic Stats ===")
        print(f"Total notifications: {len(events)}")
        if len(events) >= 2:
            gaps = [events[i]['tick']-events[i-1]['tick'] for i in range(1, len(events))]
            print(f"Avg gap: {sum(gaps)/len(gaps):.0f}ms")
        print(f"Log: {CSV_FILE}")
    except serial.SerialException as e:
        print(f"[ERROR] {e}")

if __name__ == '__main__':
    main()
