#!/usr/bin/env python3
"""Debug script for STM32_04_Timer_Debounce - analyzes bounce filtering."""
import serial, time, csv, sys, re
from datetime import datetime

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILE = 'timer_debounce_log.csv'

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
            writer.writerow(['timestamp', 'press_num', 'raw_irqs', 'debounced', 'ratio'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"  {line}")
                ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                m = re.search(r'\[DEBOUNCE\].*#(\d+).*Raw IRQs=(\d+).*Filtered=(\d+).*Ratio=([\d.]+)', line)
                if m:
                    press, raw, filt, ratio = m.group(1), m.group(2), m.group(3), m.group(4)
                    events.append({'raw': int(raw), 'filt': int(filt)})
                    writer.writerow([ts, press, raw, filt, ratio])
                    f.flush()
                    print(f"    -> Bounce ratio: {ratio}:1")

    except KeyboardInterrupt:
        print(f"\n=== Debounce Analysis ===")
        if events:
            last = events[-1]
            print(f"Total raw interrupts: {last['raw']}")
            print(f"Total valid presses: {last['filt']}")
            if last['filt'] > 0:
                print(f"Overall ratio: {last['raw']/last['filt']:.1f}:1")
                print(f"Bounce events filtered: {last['raw'] - last['filt']}")
        print(f"Log: {CSV_FILE}")
    except serial.SerialException as e:
        print(f"[ERROR] {e}")

if __name__ == '__main__':
    main()
