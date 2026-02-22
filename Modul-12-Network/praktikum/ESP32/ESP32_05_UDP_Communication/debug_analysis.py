#!/usr/bin/env python3
"""
============================================================================
ESP32_05_UDP_Communication - Debug & Analysis Tool
Modul 13 - Network & IoT
============================================================================

Script ini menyediakan:
1. UDP Test Client  - Mengirim datagram ke ESP32 dan menerima balasan
2. UDP Test Server  - Mendengarkan broadcast dari ESP32
3. Packet Analysis  - Analisis statistik paket UDP (latency, loss, dll.)
4. Visualisasi      - Grafik matplotlib untuk analisis traffic

Cara Pakai:
  python debug_analysis.py --mode client --ip 192.168.1.100 --port 3333
  python debug_analysis.py --mode server --port 3334
  python debug_analysis.py --mode analyze --logfile udp_log.json

============================================================================
"""

import socket
import time
import json
import argparse
import threading
import sys
from datetime import datetime

# Opsional: matplotlib untuk visualisasi
try:
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib tidak tersedia. Install: pip install matplotlib")


class UDPTestClient:
    """UDP Client untuk menguji komunikasi dengan ESP32"""

    def __init__(self, target_ip, target_port=3333, timeout=5):
        self.target_ip = target_ip
        self.target_port = target_port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(timeout)
        self.stats = {
            "sent": 0, "received": 0, "lost": 0,
            "latencies_ms": [], "timestamps": []
        }
        print(f"[CLIENT] Target: {target_ip}:{target_port}")

    def send_and_receive(self, message):
        """Kirim pesan dan tunggu balasan (echo), ukur latency"""
        t_start = time.time()
        try:
            self.sock.sendto(message.encode(), (self.target_ip, self.target_port))
            self.stats["sent"] += 1
            print(f"[TX] -> {self.target_ip}:{self.target_port} : {message}")

            data, addr = self.sock.recvfrom(1024)
            t_end = time.time()
            latency_ms = (t_end - t_start) * 1000

            self.stats["received"] += 1
            self.stats["latencies_ms"].append(latency_ms)
            self.stats["timestamps"].append(datetime.now().isoformat())

            print(f"[RX] <- {addr[0]}:{addr[1]} : {data.decode()} "
                  f"(latency: {latency_ms:.2f} ms)")
            return data.decode(), latency_ms

        except socket.timeout:
            self.stats["lost"] += 1
            print(f"[TIMEOUT] Tidak ada balasan dari {self.target_ip}")
            return None, None

    def run_test(self, count=10, interval=1.0):
        """Jalankan test pengiriman berulang"""
        print(f"\n{'='*60}")
        print(f"  UDP Test: {count} paket, interval {interval}s")
        print(f"{'='*60}\n")

        for i in range(1, count + 1):
            msg = f"Test packet #{i} from Python client"
            self.send_and_receive(msg)
            if i < count:
                time.sleep(interval)

        self.print_stats()
        return self.stats

    def print_stats(self):
        """Tampilkan statistik pengujian"""
        total = self.stats["sent"]
        recv = self.stats["received"]
        lost = self.stats["lost"]
        loss_pct = (lost / total * 100) if total > 0 else 0

        print(f"\n{'='*60}")
        print(f"  UDP Test Statistics")
        print(f"{'='*60}")
        print(f"  Sent: {total} | Received: {recv} | Lost: {lost} ({loss_pct:.1f}%)")

        if self.stats["latencies_ms"]:
            lats = self.stats["latencies_ms"]
            print(f"  Latency - Min: {min(lats):.2f} ms | "
                  f"Max: {max(lats):.2f} ms | "
                  f"Avg: {sum(lats)/len(lats):.2f} ms")
        print(f"{'='*60}\n")

    def close(self):
        self.sock.close()


class UDPBroadcastListener:
    """Mendengarkan broadcast UDP dari ESP32"""

    def __init__(self, port=3334, timeout=60):
        self.port = port
        self.timeout = timeout
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(('', port))
        self.sock.settimeout(10)
        self.received_packets = []
        print(f"[SERVER] Listening for broadcasts on port {port}...")

    def listen(self, duration=60):
        """Dengarkan broadcast selama durasi tertentu (detik)"""
        start = time.time()
        print(f"[SERVER] Listening for {duration} seconds...\n")

        while (time.time() - start) < duration:
            try:
                data, addr = self.sock.recvfrom(1024)
                timestamp = datetime.now().isoformat()
                packet_info = {
                    "timestamp": timestamp,
                    "source_ip": addr[0],
                    "source_port": addr[1],
                    "data": data.decode(errors='replace'),
                    "size": len(data)
                }
                self.received_packets.append(packet_info)

                print(f"[BROADCAST] {addr[0]}:{addr[1]} -> {data.decode(errors='replace')}")

                # Coba parse JSON jika formatnya JSON
                try:
                    json_data = json.loads(data.decode())
                    print(f"           Parsed: seq={json_data.get('seq','?')}, "
                          f"heap={json_data.get('heap','?')}")
                except json.JSONDecodeError:
                    pass

            except socket.timeout:
                elapsed = int(time.time() - start)
                print(f"[SERVER] Waiting... ({elapsed}/{duration}s)")

        print(f"\n[SERVER] Selesai. Total paket diterima: {len(self.received_packets)}")
        return self.received_packets

    def close(self):
        self.sock.close()


def analyze_and_plot(stats, packets=None):
    """Analisis data dan buat visualisasi dengan matplotlib"""
    if not HAS_MATPLOTLIB:
        print("[SKIP] Visualisasi memerlukan matplotlib. Install: pip install matplotlib")
        return

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle("ESP32 UDP Communication Analysis", fontsize=14, fontweight='bold')

    # Plot 1: Latency over time
    ax1 = axes[0][0]
    if stats and stats["latencies_ms"]:
        lats = stats["latencies_ms"]
        ax1.plot(range(1, len(lats) + 1), lats, 'b-o', markersize=4, label="Latency")
        avg_lat = sum(lats) / len(lats)
        ax1.axhline(y=avg_lat, color='r', linestyle='--', label=f"Average: {avg_lat:.1f} ms")
        ax1.set_xlabel("Packet Number")
        ax1.set_ylabel("Latency (ms)")
        ax1.set_title("UDP Round-Trip Latency")
        ax1.legend()
        ax1.grid(True, alpha=0.3)
    else:
        ax1.text(0.5, 0.5, "No latency data", ha='center', va='center')
        ax1.set_title("UDP Round-Trip Latency")

    # Plot 2: Packet loss pie chart
    ax2 = axes[0][1]
    if stats:
        labels = ['Received', 'Lost']
        sizes = [stats["received"], stats["lost"]]
        colors = ['#2ecc71', '#e74c3c']
        if sum(sizes) > 0:
            ax2.pie(sizes, labels=labels, colors=colors, autopct='%1.1f%%',
                    startangle=90, textprops={'fontsize': 11})
        ax2.set_title(f"Packet Delivery (Total: {stats['sent']})")
    else:
        ax2.text(0.5, 0.5, "No stats data", ha='center', va='center')

    # Plot 3: Heap memory from broadcast (jika ada)
    ax3 = axes[1][0]
    if packets:
        heap_values = []
        seq_values = []
        for pkt in packets:
            try:
                data = json.loads(pkt["data"])
                if "heap" in data and "seq" in data:
                    heap_values.append(data["heap"])
                    seq_values.append(data["seq"])
            except (json.JSONDecodeError, KeyError):
                pass
        if heap_values:
            ax3.plot(seq_values, [h / 1024 for h in heap_values], 'g-s', markersize=4)
            ax3.set_xlabel("Sequence Number")
            ax3.set_ylabel("Free Heap (KB)")
            ax3.set_title("ESP32 Heap Memory (from Broadcast)")
            ax3.grid(True, alpha=0.3)
        else:
            ax3.text(0.5, 0.5, "No heap data in broadcasts", ha='center', va='center')
    else:
        ax3.text(0.5, 0.5, "No broadcast data", ha='center', va='center')
        ax3.set_title("ESP32 Heap Memory")

    # Plot 4: Latency histogram
    ax4 = axes[1][1]
    if stats and stats["latencies_ms"]:
        ax4.hist(stats["latencies_ms"], bins=15, color='#3498db', edgecolor='black', alpha=0.7)
        ax4.set_xlabel("Latency (ms)")
        ax4.set_ylabel("Frequency")
        ax4.set_title("Latency Distribution")
        ax4.grid(True, alpha=0.3)
    else:
        ax4.text(0.5, 0.5, "No latency data", ha='center', va='center')
        ax4.set_title("Latency Distribution")

    plt.tight_layout()
    plt.savefig("udp_analysis.png", dpi=150)
    print("[PLOT] Grafik disimpan: udp_analysis.png")
    plt.show()


def main():
    parser = argparse.ArgumentParser(
        description="ESP32 UDP Communication - Debug & Analysis Tool")
    parser.add_argument("--mode", choices=["client", "server", "analyze"],
                        default="client", help="Mode: client/server/analyze")
    parser.add_argument("--ip", default="192.168.1.100",
                        help="Target IP address (untuk mode client)")
    parser.add_argument("--port", type=int, default=3333,
                        help="UDP port number")
    parser.add_argument("--count", type=int, default=20,
                        help="Jumlah paket test (mode client)")
    parser.add_argument("--interval", type=float, default=1.0,
                        help="Interval antar paket (detik)")
    parser.add_argument("--duration", type=int, default=60,
                        help="Durasi listen (detik, mode server)")
    parser.add_argument("--logfile", default=None,
                        help="File JSON untuk menyimpan/membaca log")
    args = parser.parse_args()

    if args.mode == "client":
        client = UDPTestClient(args.ip, args.port)
        stats = client.run_test(count=args.count, interval=args.interval)
        client.close()

        # Simpan log
        logfile = args.logfile or "udp_client_log.json"
        with open(logfile, 'w') as f:
            json.dump(stats, f, indent=2)
        print(f"[LOG] Stats disimpan ke {logfile}")

        analyze_and_plot(stats)

    elif args.mode == "server":
        listener = UDPBroadcastListener(port=args.port)
        packets = listener.listen(duration=args.duration)
        listener.close()

        logfile = args.logfile or "udp_broadcast_log.json"
        with open(logfile, 'w') as f:
            json.dump(packets, f, indent=2)
        print(f"[LOG] Broadcast data disimpan ke {logfile}")

        analyze_and_plot(None, packets)

    elif args.mode == "analyze":
        if not args.logfile:
            print("[ERROR] Mode analyze memerlukan --logfile")
            sys.exit(1)
        with open(args.logfile, 'r') as f:
            data = json.load(f)

        if isinstance(data, dict) and "latencies_ms" in data:
            analyze_and_plot(data)
        elif isinstance(data, list):
            analyze_and_plot(None, data)
        else:
            print("[ERROR] Format logfile tidak dikenali")


if __name__ == "__main__":
    main()
