#!/usr/bin/env python3
"""Debug script for STM32_07_Notification_Value - command value tracking."""
import serial, time, csv, sys, re
from collections import Counter
from datetime import datetime

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILE = 'notification_value_log.csv'

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    print(f"[DEBUG] Connecting to {port} @ {BAUD_RATE}")
    cmd_counter = Counter()

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        with open(CSV_FILE, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'direction', 'cmd_hex', 'cmd_name', 'count'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"  {line}")
                ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                m_send = re.search(r'\[SEND\].*0x(\w+) \((\w+)\).*#(\d+)', line)
                m_recv = re.search(r'\[RECV\].*0x(\w+) \((\w+)\).*#(\d+)', line)

                if m_send:
                    writer.writerow([ts, 'SEND', m_send.group(1), m_send.group(2), m_send.group(3)])
                    f.flush()
                elif m_recv:
                    cmd_counter[m_recv.group(2)] += 1
                    writer.writerow([ts, 'RECV', m_recv.group(1), m_recv.group(2), m_recv.group(3)])
                    f.flush()

    except KeyboardInterrupt:
        print(f"\n=== Command Stats ===")
        for cmd, cnt in cmd_counter.most_common():
            print(f"  {cmd}: {cnt}")
        print(f"Log: {CSV_FILE}")
    except serial.SerialException as e:
        print(f"[ERROR] {e}")

if __name__ == '__main__':
    main()
