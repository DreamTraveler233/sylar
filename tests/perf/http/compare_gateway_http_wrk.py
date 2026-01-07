#!/usr/bin/env python3

import argparse
import datetime as _dt
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

def _run(cmd: list[str], *, cwd: str | None = None, timeout: float | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(
        cmd,
        cwd=cwd,
        timeout=timeout,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

def _load_metrics(path: Path) -> dict:
    f = path / "metrics.json"
    if not f.exists():
        return {}
    return json.loads(f.read_text(encoding="utf-8"))

def _pct(old, new):
    if old == 0:
        return 0.0
    return ((new - old) / old) * 100.0

def _patch_mempool_enable(system_yaml: Path, enable: int) -> None:
    txt = system_yaml.read_text(encoding="utf-8", errors="replace")
    m = re.search(r"(?ms)^mempool:\s*\n(?P<indent>[ \t]+)enable:\s*([01])\s*$", txt)
    if m:
        indent = m.group("indent")
        txt = re.sub(
            r"(?ms)^mempool:\s*\n(?P<indent>[ \t]+)enable:\s*([01])\s*$",
            f"mempool:\n{indent}enable: {int(enable)}",
            txt,
        )
        system_yaml.write_text(txt, encoding="utf-8")
        return
    lines = txt.splitlines()
    out = []
    inserted = False
    for line in lines:
        out.append(line)
        if not inserted and re.match(r"^\s*pid_file\s*:\s*.+$", line):
            out.append("")
            out.append("# 内存池开关（用于 IO 热路径 buffer 复用）")
            out.append("mempool:")
            out.append(f"    enable: {int(enable)}")
            inserted = True
    if not inserted:
        out.append("")
        out.append("# 内存池开关（用于 IO 热路径 buffer 复用）")
        out.append("mempool:")
        out.append(f"    enable: {int(enable)}")
    system_yaml.write_text("\n".join(out) + "\n", encoding="utf-8")

def _make_temp_conf(src_conf_dir: Path, dst_dir: Path, enable: int) -> Path:
    if dst_dir.exists():
        shutil.rmtree(dst_dir)
    shutil.copytree(src_conf_dir, dst_dir)
    system_yaml = dst_dir / "system.yaml"
    if not system_yaml.exists():
        raise RuntimeError(f"missing {system_yaml}")
    _patch_mempool_enable(system_yaml, enable)
    return dst_dir

def _run_one(
    *,
    repo_root: Path,
    runner: Path,
    label: str,
    binary: str,
    conf_dir: Path,
    url: str,
    threads: int,
    connections: int,
    warmup: int,
    duration: int,
    startup_timeout: float,
    results_dir: str,
) -> Path:
    # Use standard run script
    cmd = [
        sys.executable,
        str(runner),
        "--label", label,
        "--binary", binary,
        "--conf", str(conf_dir),
        "--url", url,
        "--threads", str(threads),
        "--connections", str(connections),
        "--warmup", str(warmup),
        "--duration", str(duration),
        "--startup-timeout", str(startup_timeout),
        "--results-dir", results_dir,
        "--kill-existing",
    ]
    print(f"Running: {' '.join(cmd)}")
    proc = _run(cmd, cwd=str(repo_root))
    if proc.returncode != 0:
        print(f"FAILED: {label}\n{proc.stdout}")
        sys.exit(1)
    
    m = _OK_RE.search(proc.stdout)
    if not m:
        print(f"FAILED to find results dir in output: {proc.stdout}")
        sys.exit(1)
    return Path(m.group(1))

_OK_RE = re.compile(r"^\[OK\] results written to: (.+)$", re.MULTILINE)

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", default="./bin/gateway_http")
    ap.add_argument("--src-conf", default="bin/config/gateway_http")
    ap.add_argument("--url", default="http://127.0.0.1:8080/ping")
    ap.add_argument("--threads", type=int, default=4)
    ap.add_argument("--connections", type=int, default=64)
    ap.add_argument("--warmup", type=int, default=3)
    ap.add_argument("--duration", type=int, default=10)
    ap.add_argument("--startup-timeout", type=float, default=15.0)
    ap.add_argument("--results-dir", default="tests/perf/results")
    ap.add_argument("--compare-label", default="compare")

    args = ap.parse_args()
    repo_root = Path(__file__).resolve().parents[3]
    runner = repo_root / "tests/perf/http/run_gateway_http_wrk.py"
    
    src_conf_dir = (repo_root / args.src_conf).resolve()
    ts = _dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    compare_root = (repo_root / args.results_dir / args.compare_label / ts).resolve()
    compare_root.mkdir(parents=True, exist_ok=True)

    conf_baseline = _make_temp_conf(src_conf_dir, compare_root / "conf_baseline", 0)
    conf_mempool = _make_temp_conf(src_conf_dir, compare_root / "conf_mempool", 1)

    out_base = _run_one(repo_root=repo_root, runner=runner, label="baseline", 
                        binary=args.binary, conf_dir=conf_baseline, url=args.url,
                        threads=args.threads, connections=args.connections, 
                        warmup=args.warmup, duration=args.duration, 
                        startup_timeout=args.startup_timeout, 
                        results_dir=str(Path(args.results_dir) / args.compare_label / ts))

    out_mp = _run_one(repo_root=repo_root, runner=runner, label="mempool", 
                       binary=args.binary, conf_dir=conf_mempool, url=args.url,
                       threads=args.threads, connections=args.connections, 
                       warmup=args.warmup, duration=args.duration, 
                       startup_timeout=args.startup_timeout, 
                       results_dir=str(Path(args.results_dir) / args.compare_label / ts))

    m_base = _load_metrics(out_base)
    m_mp = _load_metrics(out_mp)

    def get_val(m, key, default=0.0):
        v = m.get(key, default)
        return float(v) if v is not None else default

    def get_p99(m):
        return get_val(m.get("latency_distribution_ms", {}), "99", 0.0)

    def get_errors(m):
        e = m.get("errors", {})
        if isinstance(e, dict):
            return sum(e.values())
        return 0

    metrics = [
        ("Throughput (QPS)", "requests_per_sec", "{:.2f}"),
        ("Latency Avg (ms)", "latency_avg_ms", "{:.2f}"),
        ("Latency P99 (ms)", "p99", "{:.2f}"),
        ("CPU Usage (%)", "cpu_usage_pct", "{:.2f}"),
        ("MAX RSS (KB)", "max_rss_kb", "{:.0f}"),
        ("Total Errors", "errors", "{:.0f}"),
    ]

    print("\n" + "="*80)
    print(f" PERFORMANCE COMPARISON: baseline vs mempool")
    print(f" URL: {args.url} | Concurrency: {args.connections} | Duration: {args.duration}s")
    print("-" * 80)
    print(f"{'Metric':<25} {'Baseline':>15} {'MemPool':>15} {'Delta (%)':>15}")
    print("-" * 80)

    comparison_data = {}
    for label, key, fmt in metrics:
        v_base = get_p99(m_base) if key == "p99" else (get_errors(m_base) if key == "errors" else get_val(m_base, key))
        v_mp = get_p99(m_mp) if key == "p99" else (get_errors(m_mp) if key == "errors" else get_val(m_mp, key))
        
        delta = _pct(v_base, v_mp)
        
        print(f"{label:<25} {fmt.format(v_base):>15} {fmt.format(v_mp):>15} {delta:>+14.2f}%")
        comparison_data[key] = {"baseline": v_base, "mempool": v_mp, "delta_pct": delta}

    summary = {
        "args": vars(args),
        "timestamp": ts,
        "baseline": m_base,
        "mempool": m_mp,
        "comparison": comparison_data
    }
    
    (compare_root / "compare_report.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print("-" * 80)
    print(f"Results saved to: {compare_root}")
    print("=" * 80 + "\n")

    return 0

if __name__ == "__main__":
    raise SystemExit(main())
