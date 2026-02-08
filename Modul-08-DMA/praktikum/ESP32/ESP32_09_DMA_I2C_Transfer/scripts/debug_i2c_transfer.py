"""
==========================================================================
 ESP32 I2C Transfer - Debug & Visualization
==========================================================================
 Modul 08 - Program 09: Temperature/pressure plot, timing comparison
 
 Usage: python debug_i2c_transfer.py [COM_PORT]
==========================================================================
"""

import serial
import re
import time
import sys
from collections import deque
from datetime import datetime

# ---- Configuration ----
SERIAL_PORT = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
BAUD_RATE = 115200
MAX_POINTS = 300

# ---- Data Storage ----
timestamps = deque(maxlen=MAX_POINTS)
temperatures = deque(maxlen=MAX_POINTS)
pressures = deque(maxlen=MAX_POINTS)
read_times = deque(maxlen=MAX_POINTS)

timing_methods = {}

# ---- Regex Patterns ----
re_temp = re.compile(r'Temperature:\s+([\d.-]+)\s+°C')
re_press = re.compile(r'Pressure:\s+([\d.]+)\s+hPa')
re_read_time = re.compile(r'Read time:\s+(\d+)\s+us')
re_reading = re.compile(r'Reading #(\d+)')
re_single = re.compile(r'Single-reg.*?(\d+)\s+us.*?([\d.]+)\s+us')
re_burst = re.compile(r'Burst read.*?(\d+)\s+us.*?([\d.]+)\s+us')
re_method_avg = re.compile(r'Avg:\s+([\d.]+)\s+us/read')

def parse_line(line):
    """Parse serial data."""
    data = {}
    
    m = re_temp.search(line)
    if m:
        data['temperature'] = float(m.group(1))
    
    m = re_press.search(line)
    if m:
        data['pressure'] = float(m.group(1))
    
    m = re_read_time.search(line)
    if m:
        data['read_time'] = int(m.group(1))
    
    m = re_reading.search(line)
    if m:
        data['reading_num'] = int(m.group(1))
    
    # Timing comparison results
    if 'Single-reg' in line and 'us' in line:
        data['timing_single'] = True
    if 'Burst read' in line and 'us' in line:
        data['timing_burst'] = True
    
    m = re_method_avg.search(line)
    if m:
        data['method_avg'] = float(m.group(1))
    
    return data

def print_temp_chart(temps, width=50):
    """Print ASCII temperature chart."""
    if not temps:
        return
    
    data = list(temps)[-20:]  # Last 20 points
    if len(data) < 2:
        return
    
    min_t = min(data) - 1
    max_t = max(data) + 1
    t_range = max_t - min_t
    if t_range == 0:
        t_range = 1
    
    height = 10
    chart = [[' '] * len(data) for _ in range(height)]
    
    for x, val in enumerate(data):
        y = int((val - min_t) / t_range * (height - 1))
        y = min(max(y, 0), height - 1)
        chart[height - 1 - y][x] = '●'
    
    print(f"\n  Temperature Chart (°C)  [{min_t:.1f} - {max_t:.1f}]")
    print(f"  {'─' * (len(data) + 4)}")
    for row in chart:
        print(f"  │{''.join(row)}│")
    print(f"  {'─' * (len(data) + 4)}")

def print_dashboard(data_acc):
    """Print debug dashboard."""
    print("\033[2J\033[H")
    print("=" * 60)
    print("   ESP32 I2C Transfer (BMP280) - Debug Dashboard")
    print("=" * 60)
    
    # Current readings
    temp = data_acc.get('temperature', 0)
    press = data_acc.get('pressure', 0)
    rt = data_acc.get('read_time', 0)
    
    print(f"\n  🌡  Temperature: {temp:7.2f} °C")
    print(f"  📊 Pressure:    {press:7.2f} hPa")
    print(f"  ⏱  Read time:   {rt:7d} us")
    print(f"  📖 Readings:    {data_acc.get('reading_num', 0)}")
    
    # Temperature chart
    print_temp_chart(temperatures)
    
    # Pressure trend
    if len(pressures) > 5:
        recent = list(pressures)[-5:]
        trend = recent[-1] - recent[0]
        arrow = '↑' if trend > 0.1 else '↓' if trend < -0.1 else '→'
        print(f"\n  Pressure trend: {arrow} ({trend:+.2f} hPa)")
    
    # Timing comparison
    if timing_methods:
        print(f"\n  {'─' * 50}")
        print(f"  TIMING COMPARISON")
        print(f"  {'─' * 50}")
        for method, avg_us in timing_methods.items():
            bar_len = int(avg_us / 10)
            bar_len = min(bar_len, 40)
            print(f"  {method:15s}: [{'█' * bar_len}] {avg_us:.0f} us")
    
    # Read time history
    if len(read_times) > 5:
        recent_rt = list(read_times)[-10:]
        avg_rt = sum(recent_rt) / len(recent_rt)
        min_rt = min(recent_rt)
        max_rt = max(recent_rt)
        print(f"\n  Read Time Stats (last {len(recent_rt)}):")
        print(f"    Avg: {avg_rt:.0f} us  Min: {min_rt} us  Max: {max_rt} us")
    
    print(f"\n{'=' * 60}")
    print(f"  {datetime.now().strftime('%H:%M:%S')} | Points: {len(temperatures)}")

def main():
    """Main function."""
    print(f"Connecting to {SERIAL_PORT}...")
    
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print("Connected!\n")
    except serial.SerialException as e:
        print(f"Error: {e}\nRunning demo mode...\n")
        demo_mode()
        return
    
    data_acc = {}
    current_method = None
    
    try:
        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    parsed = parse_line(line)
                    data_acc.update(parsed)
                    
                    if 'temperature' in parsed:
                        temperatures.append(parsed['temperature'])
                        timestamps.append(time.time())
                    if 'pressure' in parsed:
                        pressures.append(parsed['pressure'])
                    if 'read_time' in parsed:
                        read_times.append(parsed['read_time'])
                    
                    # Track timing test methods
                    if 'Method 1' in line:
                        current_method = 'Single-reg'
                    elif 'Method 2' in line:
                        current_method = 'Burst'
                    elif 'Method 3' in line:
                        current_method = 'Periodic'
                    
                    if 'method_avg' in parsed and current_method:
                        timing_methods[current_method] = parsed['method_avg']
                    
                    if 'temperature' in parsed:
                        print_dashboard(data_acc)
    
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ser.close()

def demo_mode():
    """Demo with simulated data."""
    import math
    t = 0
    data_acc = {}
    
    timing_methods.update({
        'Single-reg': 850,
        'Burst': 250,
        'Periodic': 280,
    })
    
    try:
        while True:
            temp = 25.0 + 2.0 * math.sin(t * 0.1) + 0.3 * math.sin(t * 0.5)
            press = 1013.25 + 5.0 * math.cos(t * 0.05)
            
            temperatures.append(temp)
            pressures.append(press)
            read_times.append(250 + int(50 * math.sin(t * 0.3)))
            timestamps.append(time.time())
            
            data_acc.update({
                'temperature': temp,
                'pressure': press,
                'read_time': read_times[-1],
                'reading_num': int(t),
            })
            
            print_dashboard(data_acc)
            time.sleep(0.5)
            t += 1
    except KeyboardInterrupt:
        print("\nDemo stopped.")

if __name__ == '__main__':
    main()
