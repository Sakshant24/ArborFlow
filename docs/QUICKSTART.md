# Quick Start Guide - ArborFlow

## Overview
ArborFlow is a C-based network firewall engine that processes packets from a CSV dataset and filters them against a blacklist.

---

## Prerequisites
- GCC compiler
- Windows (MSYS2) or Linux

---

## Build & Run

### Step 1: Navigate to project
```bash
cd core_engine
```

### Step 2: Compile
```bash
gcc -Wall -O2 -std=c11 -Iinclude -Ischeduler -o arborflow_csv \
    src/bit_vector.c src/ip_trie.c src/gatekeeper.c scheduler/scheduler.c main_csv.c
```

### Step 3: Run
```bash
./arborflow_csv
```

---

## Expected Output

```
============================================================
   ArborFlow - C Network Processing Engine
============================================================

[1] Loading packets from dataset...
[SUCCESS] Loaded 291 packets from ../data/test_packets.csv

[2] Initializing Gatekeeper...
[3] Loading blacklist...
[SUCCESS] Loaded 24339 blacklisted IPs

[4] Initializing Scheduler...

============================================================
   🚀 ArborFlow Running... (Press Ctrl+C to stop)
============================================================

[PROCESS] 192.168.1.45 -> 8.8.8.8 Proto:UDP Port:53 Size:64 Priority:9
[DROP] 103.154.231.122 blocked by Gatekeeper
[PROCESS] 192.168.1.67 -> 142.250.185.206 Proto:TCP Port:443 Size:1500 Priority:8
...
```

---

## Data Files

| File | Description |
|------|-------------|
| `data/test_packets.csv` | 291 test packets |
| `data/blacklist.csv` | 24,339 blocked IPs |

---

## Components

1. **Gatekeeper** - Firewall that blocks IPs in blacklist
2. **IP Trie** - O(1) IP lookup using 4-level tree
3. **BitVector** - Efficient bit storage
4. **Concurrent Queue** - Lock-free SPSC queue
5. **Scheduler** - Priority heap (DNS=9, HTTP=8, Other=5)

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Binary not found | Compile using gcc command above |
| Missing header | Use `-Iinclude -Ischeduler` flags |
| Link errors | Make sure all .c files are included |

---

## Project Structure

```
ArborFlow/
├── data/
│   ├── test_packets.csv
│   └── blacklist.csv
├── core_engine/
│   ├── main_csv.c
│   ├── arborflow_csv.exe
│   ├── include/
│   ├── src/
│   └── scheduler/
└── docs/
```