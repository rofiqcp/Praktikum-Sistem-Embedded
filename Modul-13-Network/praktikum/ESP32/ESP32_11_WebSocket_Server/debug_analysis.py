#!/usr/bin/env python3
"""
============================================================================
WebSocket Client Test & Data Analysis Tool
============================================================================
Deskripsi:
  Script Python untuk menguji WebSocket server pada ESP32.
  Mengirim commands (LED on/off, status), menerima sensor data
  broadcast, dan memvisualisasikan data real-time.

Dependensi:
  pip install websockets matplotlib numpy

Penggunaan:
  python debug_analysis.py --ip 192.168.1.100           # Interactive mode
  python debug_analysis.py --ip 192.168.1.100 --test    # Automated test
  python debug_analysis.py --ip 192.168.1.100 --monitor # Monitor data
  python debug_analysis.py --plot                        # Plot saved data

Author: Praktikum Sistem Embedded
============================================================================
"""

import asyncio
import argparse
import json
import time
import sys
from datetime import datetime

try:
    import websockets
    HAS_WEBSOCKETS = True
except ImportError:
    HAS_WEBSOCKETS = False
    print("[WARNING] 'websockets' library not installed. Install: pip install websockets")

try:
    import matplotlib.pyplot as plt
    import numpy as np
    from matplotlib.animation import FuncAnimation
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARNING] 'matplotlib/numpy' not installed. Plot features disabled.")

# ============================================================================
# Konfigurasi
# ============================================================================
WS_PORT = 80
DATA_LOG_FILE = "ws_sensor_log.csv"

# Data storage untuk monitoring
sensor_data = {
    'timestamps': [],
    'temp': [],
    'hum': [],
    'heap': [],
    'ids': [],
}


async def interactive_client(ws_url):
    """Mode interaktif - kirim perintah manual ke ESP32."""
    print(f"\n{'='*60}")
    print(f"  WebSocket Interactive Client")
    print(f"  Server: {ws_url}")
    print(f"{'='*60}")
    print("Commands: led_on, led_off, status, ping, quit")
    print("Or type any message to echo.\n")

    async with websockets.connect(ws_url) as websocket:
        # Task untuk menerima pesan
        async def receiver():
            try:
                async for message in websocket:
                    try:
                        data = json.loads(message)
                        msg_type = data.get('type', 'unknown')
                        if msg_type == 'sensor':
                            print(f"  [SENSOR] id={data['id']} temp={data['temp']}°C "
                                  f"hum={data['hum']}% heap={data['heap']}")
                        elif msg_type == 'welcome':
                            print(f"  [WELCOME] {data['msg']}")
                        elif msg_type == 'pong':
                            print(f"  [PONG] time={data['time']}ms")
                        elif msg_type == 'status':
                            print(f"  [STATUS] uptime={data['uptime']}s "
                                  f"clients={data['clients']} heap={data['heap']}")
                        else:
                            print(f"  [RX] {message}")
                    except json.JSONDecodeError:
                        print(f"  [RX] {message}")
            except websockets.exceptions.ConnectionClosed:
                print("[INFO] Connection closed by server")

        # Start receiver di background
        recv_task = asyncio.create_task(receiver())

        # Sender loop
        try:
            while True:
                cmd = await asyncio.get_event_loop().run_in_executor(
                    None, lambda: input(">> "))
                if cmd.lower() == 'quit':
                    break
                await websocket.send(cmd)
                print(f"  [TX] {cmd}")
        except (EOFError, KeyboardInterrupt):
            pass

        recv_task.cancel()
        print("\n[INFO] Disconnected.")


async def automated_test(ws_url):
    """Test otomatis semua fitur WebSocket server."""
    print(f"\n{'='*60}")
    print(f"  WebSocket Automated Test Suite")
    print(f"  Server: {ws_url}")
    print(f"{'='*60}\n")

    results = {}

    try:
        async with websockets.connect(ws_url, open_timeout=5) as websocket:
            print("[OK] Connected to WebSocket server")

            # Test 1: Receive welcome message
            print("\n--- Test 1: Welcome Message ---")
            try:
                msg = await asyncio.wait_for(websocket.recv(), timeout=3.0)
                data = json.loads(msg)
                results['welcome'] = data.get('type') == 'welcome'
                print(f"  Received: {msg}")
                print(f"  Result: {'PASS' if results['welcome'] else 'FAIL'}")
            except asyncio.TimeoutError:
                results['welcome'] = False
                print("  Result: FAIL (timeout)")

            # Test 2: Ping/Pong
            print("\n--- Test 2: Ping/Pong ---")
            t_start = time.time()
            await websocket.send("ping")
            msg = await asyncio.wait_for(websocket.recv(), timeout=3.0)
            rtt = (time.time() - t_start) * 1000
            data = json.loads(msg)
            results['ping'] = data.get('type') == 'pong'
            print(f"  Response: {msg}")
            print(f"  RTT: {rtt:.1f}ms")
            print(f"  Result: {'PASS' if results['ping'] else 'FAIL'}")

            # Test 3: LED Control
            print("\n--- Test 3: LED Control ---")
            await websocket.send("led_on")
            msg = await asyncio.wait_for(websocket.recv(), timeout=3.0)
            data = json.loads(msg)
            led_on_ok = data.get('status') == 'ON'

            await asyncio.sleep(1)
            await websocket.send("led_off")
            msg = await asyncio.wait_for(websocket.recv(), timeout=3.0)
            data = json.loads(msg)
            led_off_ok = data.get('status') == 'OFF'

            results['led'] = led_on_ok and led_off_ok
            print(f"  LED ON: {'OK' if led_on_ok else 'FAIL'}")
            print(f"  LED OFF: {'OK' if led_off_ok else 'FAIL'}")
            print(f"  Result: {'PASS' if results['led'] else 'FAIL'}")

            # Test 4: Status query
            print("\n--- Test 4: Status Query ---")
            await websocket.send("status")
            msg = await asyncio.wait_for(websocket.recv(), timeout=3.0)
            data = json.loads(msg)
            results['status'] = data.get('type') == 'status'
            print(f"  Response: {msg}")
            print(f"  Result: {'PASS' if results['status'] else 'FAIL'}")

            # Test 5: Echo
            print("\n--- Test 5: Echo Test ---")
            test_msg = "Hello ESP32!"
            await websocket.send(test_msg)
            msg = await asyncio.wait_for(websocket.recv(), timeout=3.0)
            data = json.loads(msg)
            results['echo'] = data.get('data') == test_msg
            print(f"  Sent: {test_msg}")
            print(f"  Received: {msg}")
            print(f"  Result: {'PASS' if results['echo'] else 'FAIL'}")

            # Test 6: Receive broadcast sensor data
            print("\n--- Test 6: Sensor Broadcast (waiting 10s) ---")
            sensor_count = 0
            try:
                end_time = time.time() + 10
                while time.time() < end_time:
                    msg = await asyncio.wait_for(websocket.recv(), timeout=5.0)
                    data = json.loads(msg)
                    if data.get('type') == 'sensor':
                        sensor_count += 1
                        print(f"  Sensor #{data['id']}: temp={data['temp']}°C hum={data['hum']}%")
            except asyncio.TimeoutError:
                pass
            results['broadcast'] = sensor_count > 0
            print(f"  Received {sensor_count} sensor broadcasts")
            print(f"  Result: {'PASS' if results['broadcast'] else 'FAIL'}")

    except Exception as e:
        print(f"[ERROR] Connection failed: {e}")
        return

    # Summary
    print(f"\n{'='*60}")
    print(f"  Test Results Summary")
    print(f"{'='*60}")
    passed = sum(1 for v in results.values() if v)
    total = len(results)
    for test_name, result in results.items():
        print(f"  {test_name:<15}: {'PASS ✓' if result else 'FAIL ✗'}")
    print(f"\n  Total: {passed}/{total} passed")
    print(f"  Score: {passed/total*100:.0f}%")


async def monitor_data(ws_url, duration=60):
    """Monitor dan log sensor data dari ESP32."""
    print(f"\n{'='*60}")
    print(f"  WebSocket Sensor Data Monitor")
    print(f"  Duration: {duration}s | Press Ctrl+C to stop")
    print(f"{'='*60}\n")

    # Reset data
    for key in sensor_data:
        sensor_data[key].clear()

    with open(DATA_LOG_FILE, 'w') as f:
        f.write("timestamp,id,temp,hum,heap\n")

    start_time = time.time()

    try:
        async with websockets.connect(ws_url) as websocket:
            # Drain welcome message
            await asyncio.wait_for(websocket.recv(), timeout=3.0)

            while (time.time() - start_time) < duration:
                try:
                    msg = await asyncio.wait_for(websocket.recv(), timeout=5.0)
                    data = json.loads(msg)

                    if data.get('type') == 'sensor':
                        elapsed = time.time() - start_time
                        sensor_data['timestamps'].append(elapsed)
                        sensor_data['temp'].append(data['temp'])
                        sensor_data['hum'].append(data['hum'])
                        sensor_data['heap'].append(data['heap'])
                        sensor_data['ids'].append(data['id'])

                        print(f"[{elapsed:>6.1f}s] #{data['id']:>4d} | "
                              f"Temp: {data['temp']:>2d}°C | "
                              f"Hum: {data['hum']:>2d}% | "
                              f"Heap: {data['heap']:>6d}")

                        with open(DATA_LOG_FILE, 'a') as f:
                            f.write(f"{elapsed:.2f},{data['id']},{data['temp']},"
                                    f"{data['hum']},{data['heap']}\n")

                except asyncio.TimeoutError:
                    print(f"[{time.time()-start_time:.1f}s] Waiting for data...")

    except KeyboardInterrupt:
        print("\n[INFO] Monitoring stopped by user.")
    except Exception as e:
        print(f"[ERROR] {e}")

    # Statistics
    if sensor_data['temp']:
        print(f"\n{'='*60}")
        print(f"  Monitoring Statistics")
        print(f"{'='*60}")
        print(f"  Samples    : {len(sensor_data['temp'])}")
        print(f"  Temp range : {min(sensor_data['temp'])}-{max(sensor_data['temp'])}°C")
        print(f"  Hum range  : {min(sensor_data['hum'])}-{max(sensor_data['hum'])}%")
        print(f"  Heap range : {min(sensor_data['heap'])}-{max(sensor_data['heap'])} bytes")
        print(f"  Data saved : {DATA_LOG_FILE}")


def plot_sensor_data():
    """Plot sensor data dari file log."""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib required. Install: pip install matplotlib numpy")
        return

    try:
        data = np.genfromtxt(DATA_LOG_FILE, delimiter=',', skip_header=1)
        timestamps = data[:, 0]
        temp = data[:, 2]
        hum = data[:, 3]
        heap = data[:, 4]
    except Exception as e:
        print(f"[ERROR] Cannot load {DATA_LOG_FILE}: {e}")
        return

    fig, axes = plt.subplots(3, 1, figsize=(12, 10), sharex=True)
    fig.suptitle("ESP32 WebSocket Sensor Data", fontsize=14, fontweight='bold')

    # Temperature
    axes[0].plot(timestamps, temp, 'r-o', markersize=3, label='Temperature')
    axes[0].axhline(y=np.mean(temp), color='darkred', linestyle='--',
                    label=f'Mean: {np.mean(temp):.1f}°C')
    axes[0].set_ylabel('Temperature (°C)')
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)

    # Humidity
    axes[1].plot(timestamps, hum, 'b-o', markersize=3, label='Humidity')
    axes[1].axhline(y=np.mean(hum), color='darkblue', linestyle='--',
                    label=f'Mean: {np.mean(hum):.1f}%')
    axes[1].set_ylabel('Humidity (%)')
    axes[1].legend()
    axes[1].grid(True, alpha=0.3)

    # Free Heap
    axes[2].plot(timestamps, heap / 1024, 'g-o', markersize=3, label='Free Heap')
    axes[2].set_ylabel('Free Heap (KB)')
    axes[2].set_xlabel('Time (seconds)')
    axes[2].legend()
    axes[2].grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('ws_sensor_analysis.png', dpi=150, bbox_inches='tight')
    print("[INFO] Plot saved to ws_sensor_analysis.png")
    plt.show()


def main():
    parser = argparse.ArgumentParser(description="WebSocket Client Test & Analysis Tool")
    parser.add_argument('--ip', type=str, help='ESP32 IP address')
    parser.add_argument('--port', type=int, default=WS_PORT, help='WebSocket port')
    parser.add_argument('--test', action='store_true', help='Run automated test suite')
    parser.add_argument('--monitor', action='store_true', help='Monitor sensor data')
    parser.add_argument('--plot', action='store_true', help='Plot saved sensor data')
    parser.add_argument('--duration', type=int, default=60, help='Monitor duration (seconds)')
    args = parser.parse_args()

    if args.plot:
        plot_sensor_data()
        return

    if not args.ip:
        print("[ERROR] ESP32 IP address required. Use: --ip 192.168.x.x")
        sys.exit(1)

    if not HAS_WEBSOCKETS:
        print("[ERROR] 'websockets' library required. Install: pip install websockets")
        sys.exit(1)

    ws_url = f"ws://{args.ip}:{args.port}/ws"

    if args.test:
        asyncio.run(automated_test(ws_url))
    elif args.monitor:
        asyncio.run(monitor_data(ws_url, args.duration))
        if sensor_data['temp'] and HAS_MATPLOTLIB:
            plot_sensor_data()
    else:
        asyncio.run(interactive_client(ws_url))


if __name__ == "__main__":
    main()
