import subprocess
import json
import re
from pathlib import Path

def run_test(connections):
    print(f"Testing with {connections} connections...")
    cmd = [
        "python3", "tests/perf/http/run_gateway_http_wrk.py",
        "--label", f"limit_{connections}",
        "--connections", str(connections),
        "--threads", "8",
        "--duration", "10",
        "--url", "http://127.0.0.1:8080/ping",
        "--kill-existing"
    ]
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    
    m = re.search(r"\[OK\] results written to: (.+)", proc.stdout)
    if m:
        res_dir = Path(m.group(1).strip())
        metrics_f = res_dir / "metrics.json"
        if metrics_f.exists():
            with open(metrics_f) as f:
                return json.load(f)
    print(f"Failed to get results for {connections}:\n{proc.stdout}")
    return None

results = []
for c in [128, 512, 1024, 2048]:
    data = run_test(c)
    if data:
        results.append({
            "conn": c,
            "qps": data.get("requests_per_sec", 0),
            "lat": data.get("latency_avg_ms", 0),
            "cpu": data.get("cpu_usage_pct", 0),
            "err": sum(data.get("errors", {}).values()) if isinstance(data.get("errors"), dict) else 0
        })

print("\nEXTREME TEST RESULTS:")
print(f"{'Conns':<10} {'QPS':<15} {'Latency(ms)':<15} {'CPU(%)':<10} {'Errors':<10}")
for r in results:
    print(f"{r['conn']:<10} {r['qps']:<15.2f} {r['lat']:<15.2f} {r['cpu']:<10.2f} {r['err']:<10}")
