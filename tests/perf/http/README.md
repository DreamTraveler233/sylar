# HTTP 性能测试

使用 `wrk` 对 `gateway_http` 的指定路径进行压测，输出 QPS、延迟分位、以及压测期间的 RSS 峰值。

## 前置

- 已构建 `gateway_http`：`./scripts/build.sh`
- 本机已安装 `wrk`（当前环境为 `/usr/local/bin/wrk`）

## 示例

对 `/_/status` 压测（建议先从小压力开始）：

```bash
python3 tests/perf/http/run_gateway_http_wrk.py \
  --label mempool \
  --url http://127.0.0.1:8080/_/status \
  --threads 4 --connections 64 \
  --warmup 3 --duration 10
```

对比旧版本二进制：

```bash
python3 tests/perf/http/run_gateway_http_wrk.py \
  --label baseline \
  --binary /path/to/old/bin/gateway_http \
  --conf /path/to/old/bin/config \
  --url http://127.0.0.1:8080/_/status \
  --threads 4 --connections 64 \
  --warmup 3 --duration 10
```

> 注意：本项目对 `-c` 的相对路径解析是“相对可执行文件所在目录（bin/）”，不是相对你运行脚本时的工作目录。
> 因此一般不需要显式传 `--conf`（让程序使用默认的 `bin/config` 即可）。

## 一键对比（不改仓库配置）

脚本会复制一份临时配置目录，将 `mempool.enable` 分别设置为 0/1 跑两次，然后输出对比结果，并把两次 run 的结果与 `compare.json` 一起落盘。

```bash
python3 tests/perf/http/compare_gateway_http_wrk.py \
  --url http://127.0.0.1:8080/_/status \
  --threads 4 --connections 64 \
  --warmup 3 --duration 10
```

> 提示：为了让“内存池减少堆分配”的收益更明显，建议选一个业务开销较低的接口（如果后续需要，我可以加一个轻量的 `/_/ping` servlet 仅用于压测）。
