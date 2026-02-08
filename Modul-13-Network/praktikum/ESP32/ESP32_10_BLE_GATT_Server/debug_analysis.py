#!/usr/bin/env python3
"""
============================================================================
BLE GATT Client Test Tool
============================================================================
Deskripsi:
  Script Python untuk menguji BLE GATT Server pada ESP32.
  Melakukan connect, read characteristic, write LED control,
  dan subscribe notifications.

Dependensi:
  pip install bleak matplotlib

Penggunaan:
  python debug_analysis.py                  # Auto-scan and connect
  python debug_analysis.py --address XX:XX  # Connect to specific device
  python debug_analysis.py --test           # Full automated test
  python debug_analysis.py --plot           # Plot notification data

Author: Praktikum Sistem Embedded
============================================================================
"""

import asyncio
import argparse
import sys
import time
from datetime import datetime

try:
    from bleak import BleakScanner, BleakClient
    HAS_BLEAK = True
except ImportError:
    HAS_BLEAK = False
    print("[WARNING] 'bleak' library not installed. Install with: pip install bleak")

try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

# ============================================================================
# Konfigurasi UUIDs (harus match dengan ESP32 GATT Server)
# ============================================================================
TARGET_DEVICE_NAME = "ESP32_GATT"

# ESP-IDF uses 16-bit UUIDs wrapped in Bluetooth Base UUID
# 0000XXXX-0000-1000-8000-00805f9b34fb
SERVICE_UUID    = "000000ff-0000-1000-8000-00805f9b34fb"
CHAR_READ_UUID  = "0000ff01-0000-1000-8000-00805f9b34fb"
CHAR_WRITE_UUID = "0000ff02-0000-1000-8000-00805f9b34fb"
CHAR_NOTIFY_UUID = "0000ff03-0000-1000-8000-00805f9b34fb"

DATA_LOG_FILE = "gatt_data_log.csv"

# Storage for notification data
notification_data = []
notification_timestamps = []


def notification_handler(sender, data):
    """Callback untuk menerima notification dari GATT Server."""
    value = data[0] if data else 0
    timestamp = time.time()
    notification_data.append(value)
    notification_timestamps.append(timestamp)
    print(f"  [NOTIFY] Char handle {sender}: value = {value} (0x{value:02X})")


async def scan_for_device():
    """Scan untuk menemukan ESP32 GATT Server."""
    print(f"\n[INFO] Scanning for device: {TARGET_DEVICE_NAME}...")
    devices = await BleakScanner.discover(timeout=5.0)

    for device in devices:
        if device.name and TARGET_DEVICE_NAME in device.name:
            print(f"[FOUND] {device.name} - {device.address} (RSSI: {device.rssi})")
            return device.address

    print(f"[ERROR] Device '{TARGET_DEVICE_NAME}' not found!")
    return None


async def read_characteristic(client, uuid, name):
    """Read a GATT characteristic."""
    try:
        data = await client.read_gatt_char(uuid)
        value = data[0] if data else 0
        print(f"  [READ]  {name}: value = {value} (raw: {data.hex()})")
        return value
    except Exception as e:
        print(f"  [ERROR] Read {name} failed: {e}")
        return None


async def write_characteristic(client, uuid, value, name):
    """Write to a GATT characteristic."""
    try:
        await client.write_gatt_char(uuid, bytes([value]))
        print(f"  [WRITE] {name}: sent value = {value}")
        return True
    except Exception as e:
        print(f"  [ERROR] Write {name} failed: {e}")
        return False


async def connect_and_test(address):
    """Connect ke GATT Server dan lakukan operasi test."""
    print(f"\n{'='*60}")
    print(f"  BLE GATT Client Test")
    print(f"  Connecting to: {address}")
    print(f"{'='*60}\n")

    async with BleakClient(address) as client:
        if not client.is_connected:
            print("[ERROR] Failed to connect!")
            return

        print(f"[OK] Connected to {address}")

        # List services
        print(f"\n--- Available Services ---")
        for service in client.services:
            print(f"  Service: {service.uuid}")
            for char in service.characteristics:
                props = ", ".join(char.properties)
                print(f"    Char: {char.uuid} [{props}]")

        # Test 1: Read sensor value
        print(f"\n--- Test 1: Read Sensor Value ---")
        sensor_val = await read_characteristic(client, CHAR_READ_UUID, "Sensor")

        # Test 2: Write LED control
        print(f"\n--- Test 2: LED Control via Write ---")
        print("  Turning LED ON...")
        await write_characteristic(client, CHAR_WRITE_UUID, 1, "LED")
        await asyncio.sleep(2)
        print("  Turning LED OFF...")
        await write_characteristic(client, CHAR_WRITE_UUID, 0, "LED")
        await asyncio.sleep(1)

        # Test 3: Subscribe to notifications
        print(f"\n--- Test 3: Notifications (listening 15s) ---")
        notification_data.clear()
        notification_timestamps.clear()

        try:
            await client.start_notify(CHAR_NOTIFY_UUID, notification_handler)
            print("  Notifications enabled. Listening...")

            for i in range(15):
                await asyncio.sleep(1)
                print(f"  [{i+1}/15s] Received {len(notification_data)} notifications so far")

            await client.stop_notify(CHAR_NOTIFY_UUID)
            print("  Notifications stopped.")
        except Exception as e:
            print(f"  [WARN] Notification test: {e}")

        # Test 4: LED blink pattern via write
        print(f"\n--- Test 4: LED Blink Pattern ---")
        for i in range(5):
            await write_characteristic(client, CHAR_WRITE_UUID, 1, "LED")
            await asyncio.sleep(0.3)
            await write_characteristic(client, CHAR_WRITE_UUID, 0, "LED")
            await asyncio.sleep(0.3)
        print("  Blink pattern complete!")

        # Summary
        print(f"\n{'='*60}")
        print(f"  Test Summary")
        print(f"{'='*60}")
        print(f"  Sensor read     : {'PASS' if sensor_val is not None else 'FAIL'}")
        print(f"  LED write       : PASS")
        print(f"  Notifications   : {len(notification_data)} received")
        if notification_data:
            print(f"  Notify min/max  : {min(notification_data)}/{max(notification_data)}")
            print(f"  Notify average  : {sum(notification_data)/len(notification_data):.1f}")

        # Save data
        if notification_data:
            with open(DATA_LOG_FILE, 'w') as f:
                f.write("index,timestamp,value\n")
                start = notification_timestamps[0]
                for i, (ts, val) in enumerate(zip(notification_timestamps, notification_data)):
                    f.write(f"{i},{ts-start:.3f},{val}\n")
            print(f"  Data saved to   : {DATA_LOG_FILE}")


def plot_notification_data():
    """Plot notification data dari file log."""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib required. Install: pip install matplotlib numpy")
        return

    try:
        data = np.genfromtxt(DATA_LOG_FILE, delimiter=',', skip_header=1)
        indices = data[:, 0]
        timestamps = data[:, 1]
        values = data[:, 2]
    except Exception as e:
        print(f"[ERROR] Cannot load {DATA_LOG_FILE}: {e}")
        return

    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle(f"BLE GATT Notification Data - {TARGET_DEVICE_NAME}", fontsize=13)

    # Plot 1: Values over time
    axes[0].plot(timestamps, values, 'b-o', markersize=4, label='Sensor Value')
    axes[0].axhline(y=np.mean(values), color='r', linestyle='--',
                    label=f'Mean: {np.mean(values):.1f}')
    axes[0].set_xlabel('Time (seconds)')
    axes[0].set_ylabel('Sensor Value')
    axes[0].set_title('Notification Values Over Time')
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)

    # Plot 2: Histogram
    axes[1].hist(values, bins=15, color='green', alpha=0.7, edgecolor='black')
    axes[1].set_xlabel('Value')
    axes[1].set_ylabel('Frequency')
    axes[1].set_title('Value Distribution')
    axes[1].grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('gatt_notification_analysis.png', dpi=150, bbox_inches='tight')
    print("[INFO] Plot saved to gatt_notification_analysis.png")
    plt.show()


def main():
    parser = argparse.ArgumentParser(description="BLE GATT Client Test Tool")
    parser.add_argument('--address', type=str, help='BLE device address (XX:XX:XX:XX:XX:XX)')
    parser.add_argument('--test', action='store_true', help='Run full automated test')
    parser.add_argument('--plot', action='store_true', help='Plot notification data from log')
    parser.add_argument('--target', type=str, default=TARGET_DEVICE_NAME, help='Target device name')
    args = parser.parse_args()

    if args.plot:
        plot_notification_data()
        return

    if not HAS_BLEAK:
        print("[ERROR] 'bleak' library required. Install: pip install bleak")
        sys.exit(1)

    global TARGET_DEVICE_NAME
    TARGET_DEVICE_NAME = args.target

    address = args.address
    if not address:
        address = asyncio.run(scan_for_device())
        if not address:
            sys.exit(1)

    asyncio.run(connect_and_test(address))

    if notification_data and HAS_MATPLOTLIB:
        plot_notification_data()


if __name__ == "__main__":
    main()
