# 性能测试（perf）

本目录用于对 XinYu-IM 后端组件进行可重复、量化的性能测试。通过统一的测试脚本与环境，分析系统的负载能力、响应延迟及资源利用率。

## 1. 核心评估指标

性能测试不应只看单一数字，而应关注以下**指标集合**的平衡：

### 黄金三角（核心质量）
*   **吞吐量 (Throughput)**: 关注 `requests_per_sec` (QPS/RPS)。衡量单位时间内系统处理的工作量。
*   **响应延迟 (Latency)**: 重点关注 **P95/P99** 尾部延迟及平均延迟。P99 代表了 99% 用户的真实体验，反映了系统的稳定性。
*   **错误率 (Error Rate)**: 压测期间失败请求（如 HTTP 5xx, 超时等）的比例。**任何 QPS 数据的有效性都必须建立在错误率接近 0% 的前提下。**

### 系统资源（瓶颈定位）
*   **内存占用 (RSS/HWM)**: 重点观察物理内存占用。通过内存池（Mempool）优化后，应对比内存小幅上涨与延迟大幅下降之间的权衡（Trade-off）。
*   **CPU 利用率**: 观察系统态（%sy）与用户态（%us）占比。内存池优化通常能通过减少系统调用显著降低 `%sy`。

---

## 2. 性能测试方法论

建议采用以下步骤进行科学测试：

### 第一阶段：基线测试 (Baseline Test)
在无负载或极低负载下运行，确定系统在理想状态下的性能参数。本项目中，即为禁用内存池且配置较低并发连接数的 run。

### 第二阶段：阶梯压力测试 (Step-up Test)
1.  固定线程数（Threads），逐步增加连接数（Connections），例如从 16 -> 64 -> 256 -> 1024。
2.  观察吞吐量曲线。当 QPS 不再随连接数增加而增长，或 P99 出现陡增、错误率上升时，该点即为系统的**性能拐点**。

### 第三阶段：对比测试 (Comparative Test)
*   **场景**：验证某一特性（如 Mempool）的优化效果。
*   **工具**：手动切换配置或使用 [compare_gateway_http_wrk.py](http/compare_gateway_http_wrk.py)。
*   **关键点**：确保两次测试的环境（硬件、网络、数据库状态）完全一致。

### 第四阶段：稳定性/压力测试 (Soak Test)
延长持续时间（如 `--duration 3600`），观察系统在长时间高负荷运行下内存是否存在泄漏（RSS 是否持续攀升）以及是否存在累积碎片导致的延迟漂移。

---

## 3. 快速操作步骤

本框架支持“一键对比分析”和“手动阶梯测试”。

### 3.1 一键对比测试（推荐）
自动运行“基线”和“内存池”两个场景并产出对比报表，无需手动修改配置文件。
```bash
# 进入仓库根目录
python3 tests/perf/http/compare_gateway_http_wrk.py \
  --url http://127.0.0.1:8080/_/status \
  --threads 4 --connections 64 \
  --warmup 3 --duration 30
```

### 3.2 手动阶梯压力测试
用于寻找系统瓶颈拐点。
1.  **修改配置**：在 `bin/config/gateway_http/system.yaml` 中设置 `mempool.enable: 1`。
2.  **运行压测**：逐渐加大 `--connections`。
```bash
python3 tests/perf/http/run_gateway_http_wrk.py --label p256 --connections 256 --duration 60
python3 tests/perf/http/run_gateway_http_wrk.py --label p512 --connections 512 --duration 60
```

---

## 4. 结果目录说明

结果输出到 [tests/perf/results/](results/)，结构如下：
*   `wrk.txt`：wrk 工具的原始输出（查看详细延迟分布）。
*   `metrics.json`：解析后的结构化指标，便于自动化分析。
*   `server.log`：压测期间被测服务的控制台日志及错误信息。
*   `compare.json`：对比脚本生成的汇总收益报告。

---

## 4. 模块指南

*   **HTTP 接口**: 见 [tests/perf/http/README.md](http/README.md)。
*   **WebSocket**: （待添加）建议使用专门的并发客户端模拟工具进行长连接压力测试。

