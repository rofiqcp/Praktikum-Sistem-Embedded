#!/usr/bin/env python3
"""
============================================================================
ESP32_06_HTTP_Server - Debug & Analysis Tool
Modul 13 - Network & IoT
============================================================================

Script ini menyediakan:
1. API Endpoint Tester  - Test semua REST API endpoint ESP32
2. Load Test            - Kirim banyak request untuk uji performa
3. LED Control CLI      - Kontrol LED dari command line
4. Visualisasi          - Response time, throughput chart

Cara Pakai:
  python debug_analysis.py --ip 192.168.1.100 --mode test
  python debug_analysis.py --ip 192.168.1.100 --mode led --state on
  python debug_analysis.py --ip 192.168.1.100 --mode loadtest --count 100

============================================================================
"""

import time
import json
import argparse
import sys
from datetime import datetime

# HTTP requests
try:
    import requests
    HAS_REQUESTS = True
except ImportError:
    HAS_REQUESTS = False
    # Fallback ke urllib
    import urllib.request
    import urllib.error

# Matplotlib opsional
try:
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


class ESP32HttpTester:
    """Tester untuk ESP32 HTTP Server REST API"""

    def __init__(self, base_url):
        self.base_url = base_url.rstrip('/')
        self.results = []
        print(f"[TESTER] Target: {self.base_url}")

    def _get(self, path):
        """HTTP GET request dengan timing"""
        url = f"{self.base_url}{path}"
        t_start = time.time()
        try:
            if HAS_REQUESTS:
                resp = requests.get(url, timeout=10)
                elapsed = (time.time() - t_start) * 1000
                return {
                    "url": url, "method": "GET", "status": resp.status_code,
                    "body": resp.text, "elapsed_ms": elapsed, "ok": resp.ok
                }
            else:
                req = urllib.request.Request(url)
                with urllib.request.urlopen(req, timeout=10) as resp:
                    body = resp.read().decode()
                    elapsed = (time.time() - t_start) * 1000
                    return {
                        "url": url, "method": "GET", "status": resp.status,
                        "body": body, "elapsed_ms": elapsed, "ok": resp.status == 200
                    }
        except Exception as e:
            elapsed = (time.time() - t_start) * 1000
            return {
                "url": url, "method": "GET", "status": 0,
                "body": str(e), "elapsed_ms": elapsed, "ok": False
            }

    def _post(self, path, data):
        """HTTP POST request dengan timing"""
        url = f"{self.base_url}{path}"
        t_start = time.time()
        try:
            if HAS_REQUESTS:
                resp = requests.post(url, json=data, timeout=10)
                elapsed = (time.time() - t_start) * 1000
                return {
                    "url": url, "method": "POST", "status": resp.status_code,
                    "body": resp.text, "elapsed_ms": elapsed, "ok": resp.ok
                }
            else:
                payload = json.dumps(data).encode()
                req = urllib.request.Request(url, data=payload,
                    headers={"Content-Type": "application/json"})
                with urllib.request.urlopen(req, timeout=10) as resp:
                    body = resp.read().decode()
                    elapsed = (time.time() - t_start) * 1000
                    return {
                        "url": url, "method": "POST", "status": resp.status,
                        "body": body, "elapsed_ms": elapsed, "ok": resp.status == 200
                    }
        except Exception as e:
            elapsed = (time.time() - t_start) * 1000
            return {
                "url": url, "method": "POST", "status": 0,
                "body": str(e), "elapsed_ms": elapsed, "ok": False
            }

    def test_all_endpoints(self):
        """Test semua API endpoint dan tampilkan hasil"""
        print(f"\n{'='*65}")
        print(f"  ESP32 HTTP Server - API Endpoint Test")
        print(f"{'='*65}\n")

        tests = [
            ("GET /", "Root HTML Page"),
            ("GET /api/status", "System Status JSON"),
            ("POST /api/led ON", "LED Control - Turn ON"),
            ("GET /api/status", "Verify LED ON"),
            ("POST /api/led OFF", "LED Control - Turn OFF"),
            ("GET /api/status", "Verify LED OFF"),
        ]

        results = []
        for test_name, desc in tests:
            print(f"  [{desc}]")
            if test_name == "GET /":
                result = self._get("/")
                # Cek apakah response berisi HTML
                has_html = "<html>" in result["body"].lower() if result["ok"] else False
                print(f"    Status: {result['status']} | "
                      f"HTML: {'Yes' if has_html else 'No'} | "
                      f"Size: {len(result['body'])} bytes | "
                      f"Time: {result['elapsed_ms']:.1f} ms")

            elif test_name == "GET /api/status":
                result = self._get("/api/status")
                if result["ok"]:
                    try:
                        data = json.loads(result["body"])
                        print(f"    Status: {result['status']} | Time: {result['elapsed_ms']:.1f} ms")
                        print(f"    Heap: {data.get('free_heap', '?')} bytes | "
                              f"Uptime: {data.get('uptime_sec', '?')}s | "
                              f"LED: {data.get('led_state', '?')}")
                    except json.JSONDecodeError:
                        print(f"    Status: {result['status']} (JSON parse error)")
                else:
                    print(f"    FAILED: {result['body']}")

            elif "POST" in test_name:
                state = "ON" in test_name
                result = self._post("/api/led", {"state": state})
                print(f"    Status: {result['status']} | "
                      f"Response: {result['body']} | "
                      f"Time: {result['elapsed_ms']:.1f} ms")

            result["test"] = desc
            results.append(result)
            print()

        # Ringkasan
        passed = sum(1 for r in results if r["ok"])
        total = len(results)
        print(f"{'='*65}")
        print(f"  Results: {passed}/{total} passed")
        print(f"{'='*65}\n")

        self.results = results
        return results

    def load_test(self, endpoint="/api/status", count=50, delay=0.1):
        """Load test: kirim banyak request untuk ukur performa"""
        print(f"\n{'='*65}")
        print(f"  Load Test: {count} requests to {endpoint}")
        print(f"{'='*65}\n")

        latencies = []
        errors = 0
        heap_values = []

        for i in range(1, count + 1):
            result = self._get(endpoint)
            latencies.append(result["elapsed_ms"])

            if result["ok"]:
                try:
                    data = json.loads(result["body"])
                    if "free_heap" in data:
                        heap_values.append(data["free_heap"])
                except (json.JSONDecodeError, KeyError):
                    pass
            else:
                errors += 1

            if i % 10 == 0:
                print(f"  Progress: {i}/{count} | "
                      f"Avg: {sum(latencies)/len(latencies):.1f} ms | "
                      f"Errors: {errors}")

            time.sleep(delay)

        # Statistik
        print(f"\n{'='*65}")
        print(f"  Load Test Results")
        print(f"{'='*65}")
        print(f"  Total Requests : {count}")
        print(f"  Success        : {count - errors}")
        print(f"  Errors         : {errors}")
        print(f"  Avg Latency    : {sum(latencies)/len(latencies):.2f} ms")
        print(f"  Min Latency    : {min(latencies):.2f} ms")
        print(f"  Max Latency    : {max(latencies):.2f} ms")
        if heap_values:
            print(f"  Heap Start     : {heap_values[0]} bytes")
            print(f"  Heap End       : {heap_values[-1]} bytes")
            print(f"  Heap Delta     : {heap_values[-1] - heap_values[0]} bytes")
        print(f"{'='*65}\n")

        stats = {"latencies": latencies, "errors": errors,
                 "heap_values": heap_values, "count": count}

        # Simpan log
        with open("http_loadtest_log.json", 'w') as f:
            json.dump(stats, f, indent=2)
        print("[LOG] Saved to http_loadtest_log.json")

        if HAS_MATPLOTLIB:
            self._plot_loadtest(stats)

        return stats

    def _plot_loadtest(self, stats):
        """Visualisasi hasil load test"""
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle("ESP32 HTTP Server - Load Test Analysis", fontsize=14, fontweight='bold')

        # Plot 1: Latency over time
        ax1 = axes[0][0]
        lats = stats["latencies"]
        ax1.plot(range(1, len(lats) + 1), lats, 'b-', alpha=0.7, linewidth=0.8)
        avg = sum(lats) / len(lats)
        ax1.axhline(y=avg, color='r', linestyle='--', label=f"Avg: {avg:.1f} ms")
        ax1.set_xlabel("Request #")
        ax1.set_ylabel("Response Time (ms)")
        ax1.set_title("Response Time per Request")
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        # Plot 2: Latency histogram
        ax2 = axes[0][1]
        ax2.hist(lats, bins=20, color='#3498db', edgecolor='black', alpha=0.7)
        ax2.set_xlabel("Response Time (ms)")
        ax2.set_ylabel("Frequency")
        ax2.set_title("Response Time Distribution")
        ax2.grid(True, alpha=0.3)

        # Plot 3: Heap memory
        ax3 = axes[1][0]
        if stats["heap_values"]:
            hv = [h / 1024 for h in stats["heap_values"]]
            ax3.plot(range(1, len(hv) + 1), hv, 'g-o', markersize=2)
            ax3.set_xlabel("Request #")
            ax3.set_ylabel("Free Heap (KB)")
            ax3.set_title("Heap Memory During Load Test")
            ax3.grid(True, alpha=0.3)
        else:
            ax3.text(0.5, 0.5, "No heap data", ha='center', va='center')

        # Plot 4: Throughput (requests/second, sliding window)
        ax4 = axes[1][1]
        window = 10
        if len(lats) >= window:
            throughput = []
            for i in range(len(lats) - window + 1):
                window_time = sum(lats[i:i+window]) / 1000  # ms -> s
                throughput.append(window / window_time if window_time > 0 else 0)
            ax4.plot(range(window, len(lats) + 1), throughput, 'purple', linewidth=1.2)
            ax4.set_xlabel("Request #")
            ax4.set_ylabel("Requests/sec")
            ax4.set_title(f"Throughput (window={window})")
            ax4.grid(True, alpha=0.3)
        else:
            ax4.text(0.5, 0.5, "Not enough data", ha='center', va='center')

        plt.tight_layout()
        plt.savefig("http_loadtest_analysis.png", dpi=150)
        print("[PLOT] Saved: http_loadtest_analysis.png")
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description="ESP32 HTTP Server - Debug & Analysis Tool")
    parser.add_argument("--ip", default="192.168.1.100",
                        help="ESP32 IP address")
    parser.add_argument("--port", type=int, default=80,
                        help="HTTP port (default 80)")
    parser.add_argument("--mode", choices=["test", "led", "loadtest"],
                        default="test", help="Mode: test/led/loadtest")
    parser.add_argument("--state", choices=["on", "off"], default="on",
                        help="LED state (mode=led)")
    parser.add_argument("--count", type=int, default=50,
                        help="Request count (mode=loadtest)")
    args = parser.parse_args()

    base_url = f"http://{args.ip}:{args.port}"
    tester = ESP32HttpTester(base_url)

    if args.mode == "test":
        tester.test_all_endpoints()

    elif args.mode == "led":
        state = args.state == "on"
        result = tester._post("/api/led", {"state": state})
        print(f"LED {'ON' if state else 'OFF'}: {result['body']} "
              f"({result['elapsed_ms']:.1f} ms)")

    elif args.mode == "loadtest":
        tester.load_test(count=args.count)


if __name__ == "__main__":
    main()
