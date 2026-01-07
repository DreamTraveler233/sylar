#!/usr/bin/env python3

import argparse
import datetime as _dt
import json
import os
import re
import signal
import socket
import subprocess
import sys
import time
from pathlib import Path


def _run(cmd: list[str], *, cwd: str | None = None, timeout: float | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, cwd=cwd, timeout=timeout, check=False, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def _wait_port(host: str, port: int, timeout_s: float) -> bool:
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.2):
                return True
        except OSError:
            time.sleep(0.1)
    return False


def _read_proc_status_kb(pid: int) -> dict[str, int]:
    # returns keys like VmRSS/VmHWM in kB if present
    path = Path(f"/proc/{pid}/status")
    out: dict[str, int] = {}
    try:
        for line in path.read_text().splitlines():
            if line.startswith("VmRSS:") or line.startswith("VmHWM:"):
                parts = line.split()
                if len(parts) >= 2 and parts[1].isdigit():
                    out[parts[0].rstrip(":")] = int(parts[1])
            elif line.startswith("Threads:"):
                parts = line.split()
                if len(parts) >= 2 and parts[1].isdigit():
                    out["threads"] = int(parts[1])
    except FileNotFoundError:
        pass
    return out


def _get_proc_cpu_jiffies(pid: int) -> tuple[int, int] | None:
    # returns (utime, stime) in jiffies
    try:
        stat_line = Path(f"/proc/{pid}/stat").read_text().split()
        return int(stat_line[13]), int(stat_line[14])
    except (FileNotFoundError, IndexError):
        return None


_WRK_REQS_RE = re.compile(r"^Requests/sec:\s+([0-9.]+)", re.MULTILINE)
_WRK_LAT_RE = re.compile(r"^\s*Latency\s+([0-9.]+)([a-zA-Z]+)\s+([0-9.]+)([a-zA-Z]+)", re.MULTILINE)
_WRK_DIST_RE = re.compile(r"^\s*([0-9]+)%\s+([0-9.]+)([a-zA-Z]+)\s*$", re.MULTILINE)
_WRK_ERR_RE = re.compile(
    r"Socket errors:\s+connect\s+(\d+),\s+read\s+(\d+),\s+write\s+(\d+),\s+timeout\s+(\d+)", re.MULTILINE
)
_WRK_TRANS_RE = re.compile(r"^Transfer/sec:\s+([0-9.]+)([a-zA-Z/]+)", re.MULTILINE)


def _to_ms(value: float, unit: str) -> float:
    unit = unit.lower()
    if unit in ("ms",):
        return value
    if unit in ("s", "sec", "secs"):
        return value * 1000.0
    if unit in ("us",):
        return value / 1000.0
    if unit in ("ns",):
        return value / 1_000_000.0
    return value


def parse_wrk(output: str) -> dict:
    metrics: dict = {}

    m = _WRK_REQS_RE.search(output)
    if m:
        metrics["requests_per_sec"] = float(m.group(1))

    m = _WRK_TRANS_RE.search(output)
    if m:
        metrics["transfer_per_sec"] = f"{m.group(1)}{m.group(2)}"

    # example: "Latency   1.23ms   0.50ms ..." => avg + stdev
    m = _WRK_LAT_RE.search(output)
    if m:
        avg = _to_ms(float(m.group(1)), m.group(2))
        stdev = _to_ms(float(m.group(3)), m.group(4))
        metrics["latency_avg_ms"] = avg
        metrics["latency_stdev_ms"] = stdev

    # Socket errors
    m = _WRK_ERR_RE.search(output)
    if m:
        metrics["errors"] = {
            "connect": int(m.group(1)),
            "read": int(m.group(2)),
            "write": int(m.group(3)),
            "timeout": int(m.group(4)),
        }
    else:
        metrics["errors"] = {"connect": 0, "read": 0, "write": 0, "timeout": 0}

    # Total error count for easy summary
    metrics["error_count"] = sum(metrics["errors"].values())

    # distribution block (requires --latency)
    dist: dict[str, float] = {}
    for pct, val, unit in _WRK_DIST_RE.findall(output):
        dist[pct] = _to_ms(float(val), unit)
    if dist:
        metrics["latency_distribution_ms"] = dist

    return metrics


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--label", required=True, help="Results label, e.g. baseline/mempool")
    ap.add_argument("--binary", default="./bin/gateway_http", help="Path to gateway_http binary")
    ap.add_argument(
        "--conf",
        default=None,
        help=(
            "Config directory passed to -c. If omitted, let the binary use its default config path. "
            "Note: this project resolves relative paths against the executable directory (bin/), not the current working directory."
        ),
    )
    ap.add_argument("--url", default="http://127.0.0.1:8080/_/status", help="Benchmark URL")
    ap.add_argument("--threads", type=int, default=4)
    ap.add_argument("--connections", type=int, default=64)
    ap.add_argument("--duration", type=int, default=10)
    ap.add_argument("--warmup", type=int, default=3)
    ap.add_argument("--mem-sample-ms", type=int, default=200)
    ap.add_argument("--results-dir", default="tests/perf/results")
    ap.add_argument("--kill-existing", action="store_true", help="Try to stop existing gateway_http before run")
    ap.add_argument("--startup-timeout", type=float, default=10.0)

    args = ap.parse_args()

    repo_root = Path(__file__).resolve().parents[3]
    results_root = (repo_root / args.results_dir).resolve()
    ts = _dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    out_dir = results_root / args.label / ts
    out_dir.mkdir(parents=True, exist_ok=True)

    server_log = out_dir / "server.log"
    wrk_txt = out_dir / "wrk.txt"
    metrics_json = out_dir / "metrics.json"

    if args.kill_existing:
        # Be conservative: kill only the exact process name to avoid accidental matches.
        _run(["pkill", "-x", "gateway_http"], cwd=str(repo_root))
        time.sleep(0.5)

    # Start server in foreground (-s). Keep stdout/stderr for diagnostics.
    server_cmd = [args.binary]
    if args.conf:
        server_cmd += ["-c", args.conf]
    server_cmd += ["-s"]
    with server_log.open("w") as logf:
        proc = subprocess.Popen(server_cmd, cwd=str(repo_root), stdout=logf, stderr=subprocess.STDOUT, text=True)

    try:
        # Wait for port
        u = args.url
        host = "127.0.0.1"
        port = 8080
        try:
            # crude parse
            hostport = u.split("//", 1)[1].split("/", 1)[0]
            if ":" in hostport:
                host, p = hostport.rsplit(":", 1)
                port = int(p)
            else:
                host = hostport
                port = 80
        except Exception:
            pass

        if not _wait_port(host, port, args.startup_timeout):
            rc = proc.poll()
            tail = ""
            try:
                txt = server_log.read_text(errors="replace")
                tail = "\n".join(txt.splitlines()[-80:])
            except Exception:
                pass
            if rc is not None:
                raise RuntimeError(
                    f"server exited early (rc={rc}) before listening on {host}:{port}. "
                    f"See {server_log}.\n--- server.log tail ---\n{tail}\n--- end ---"
                )
            raise RuntimeError(
                f"server not listening on {host}:{port} within timeout. See {server_log}.\n"
                f"--- server.log tail ---\n{tail}\n--- end ---"
            )

        def run_wrk(seconds: int) -> str:
            cmd = [
                "wrk",
                "-t",
                str(args.threads),
                "-c",
                str(args.connections),
                "-d",
                f"{seconds}s",
                "--latency",
                args.url,
            ]
            cp = _run(cmd, cwd=str(repo_root), timeout=seconds + 30)
            return cp.stdout

        # Warmup
        if args.warmup > 0:
            _ = run_wrk(args.warmup)

        # Measure + memory sampling
        sample_interval = max(0.05, args.mem_sample_ms / 1000.0)
        start_t = time.time()
        start_cpu = _get_proc_cpu_jiffies(proc.pid)

        wrk_proc = subprocess.Popen(
            [
                "wrk",
                "-t",
                str(args.threads),
                "-c",
                str(args.connections),
                "-d",
                f"{args.duration}s",
                "--latency",
                args.url,
            ],
            cwd=str(repo_root),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )

        max_rss_kb = 0
        max_hwm_kb = 0
        max_threads = 0
        while wrk_proc.poll() is None:
            st = _read_proc_status_kb(proc.pid)
            if "VmRSS" in st:
                max_rss_kb = max(max_rss_kb, st["VmRSS"])
            if "VmHWM" in st:
                max_hwm_kb = max(max_hwm_kb, st["VmHWM"])
            if "threads" in st:
                max_threads = max(max_threads, st["threads"])
            time.sleep(sample_interval)

        out = (wrk_proc.stdout.read() if wrk_proc.stdout else "")
        elapsed = time.time() - start_t
        end_cpu = _get_proc_cpu_jiffies(proc.pid)

        wrk_txt.write_text(out)

        metrics = parse_wrk(out)

        # CPU Usage calculation
        if start_cpu and end_cpu and elapsed > 0:
            # Try to get CLK_TCK, fallback to 100 which is common on Linux
            clk_tck = 100
            try:
                # os.sysconf_names might have it as 'SC_CLK_TCK'
                import os
                if hasattr(os, "sysconf"):
                    # Use string name if the constant is not directly on os module
                    clk_tck = os.sysconf("SC_CLK_TCK")
            except (ValueError, KeyError, AttributeError):
                pass
            
            u_delta = end_cpu[0] - start_cpu[0]
            s_delta = end_cpu[1] - start_cpu[1]
            total_delta = u_delta + s_delta
            # %usage = jiffies / (seconds * clk_tck)
            metrics["cpu_usage_pct"] = (total_delta / (elapsed * clk_tck)) * 100.0
            metrics["cpu_user_pct"] = (u_delta / (elapsed * clk_tck)) * 100.0
            metrics["cpu_sys_pct"] = (s_delta / (elapsed * clk_tck)) * 100.0

        metrics.update(
            {
                "label": args.label,
                "timestamp": ts,
                "url": args.url,
                "threads": args.threads,
                "connections": args.connections,
                "duration_s": args.duration,
                "warmup_s": args.warmup,
                "elapsed_s": elapsed,
                "server_pid": proc.pid,
                "max_rss_kb": max_rss_kb,
                "max_hwm_kb": max_hwm_kb,
                "max_threads": max_threads,
            }
        )

        # best-effort git info
        git = _run(["git", "rev-parse", "HEAD"], cwd=str(repo_root))
        if git.returncode == 0:
            metrics["git_head"] = git.stdout.strip()

        metrics_json.write_text(json.dumps(metrics, ensure_ascii=False, indent=2))

        print(f"[OK] results written to: {out_dir}")
        print(json.dumps(metrics, ensure_ascii=False, indent=2))
        return 0

    finally:
        # Stop server
        try:
            proc.send_signal(signal.SIGTERM)
            proc.wait(timeout=5)
        except Exception:
            try:
                proc.kill()
            except Exception:
                pass


if __name__ == "__main__":
    raise SystemExit(main())
