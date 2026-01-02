# High-Frequency Performance Benchmarks

This Load Balancer was stress-tested using `wrk` to evaluate its performance under heavy concurrent load. The results demonstrate that a custom `kqueue` implementation can provide superior throughput compared to general-purpose servers.

### 📊 Benchmark Summary (Latest Run)
* **Peak Throughput:** 40,037.72 requests/sec
* **Total Requests:** 1,204,698 (in 30.09 seconds)
* **Data Transferred:** 228.63 MB
* **Success Rate:** 99.967% (Only 394 read errors out of 1.2M+ requests)

---

### ⚙️ Environment & Configuration
Testing was performed on local loopback to measure the raw efficiency of the event loop and thread management.

| Variable | Value |
| :--- | :--- |
| **Tool** | `wrk` (Industry Standard) |
| **Hardware** | MacBook Pro M1, 8GB RAM |
| **Concurrency** | 400 Simultaneous Connections |
| **Threads** | 12 (Utilizing M1 Performance/Efficiency cores) |
| **Target** | `http://localhost:8080/` |
| **Protocol Support** | Dual-Stack IPv4 (127.0.0.1) & IPv6 (::1) | 

---
