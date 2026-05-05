# ArborFlow - Network Security Engine

## Overview

ArborFlow is a real-time network packet processing and firewall engine built in C. It reads packet data from a CSV dataset, checks against a blacklist of 24,339 blocked IPs, and processes packets based on priority.

---

## Project Structure

```
ArborFlow/
├── data/
│   ├── test_packets.csv    ← Demo dataset (291 packets)
│   └── blacklist.csv       ← 24,339 blocked IPs
├── core_engine/
│   ├── main_csv.c          ← Entry point
│   ├── arborflow_csv.exe   ← Compiled binary
│   ├── Makefile
│   ├── include/            ← Header files
│   ├── src/                ← Source code
│   │   ├── gatekeeper.c    ← Firewall logic
│   │   ├── ip_trie.c       ← O(1) IP lookup
│   │   ├── bit_vector.c   ← Bit storage
│   │   ├── capture.c      ← Packet parsing
│   │   └── concurrent_q.c  ← Lock-free queue
│   └── scheduler/          ← Priority heap
└── docs/                  ← Documentation
```

---

## Components

| Component | File | Purpose |
|-----------|------|---------|
| **Gatekeeper** | `gatekeeper.c` | Firewall - blocks blacklisted IPs |
| **IP Trie** | `ip_trie.c` | O(1) IP lookup using 4-level tree |
| **BitVector** | `bit_vector.c` | Efficient 256-bit storage |
| **Capture** | `capture.c` | Packet header parsing |
| **Concurrent Queue** | `concurrent_q.c` | Lock-free SPSC queue |
| **Scheduler** | `scheduler.c` | Priority heap (DNS=9, HTTP=8, Other=5) |

---

## How to Run

### Prerequisites
- GCC compiler
- Windows (with MSYS2) or Linux

### Build & Run
```bash
cd core_engine
gcc -Wall -O2 -std=c11 -Iinclude -Ischeduler -o arborflow_csv \
    src/bit_vector.c src/ip_trie.c src/gatekeeper.c scheduler/scheduler.c main_csv.c

./arborflow_csv
```

---

## Sample Output

```
[Gatekeeper] Initialized. Static IP Trie engine ready.
[Gatekeeper] Loaded 24339 new IPs from ../data/blacklist.csv
[SUCCESS] Loaded 291 packets from ../data/test_packets.csv

🚀 ArborFlow Running...

[PROCESS] 192.168.1.45 -> 8.8.8.8 Proto:UDP Port:53 Size:64 Priority:9
[DROP] 103.154.231.122 blocked by Gatekeeper
[PROCESS] 192.168.1.67 -> 142.250.185.206 Proto:TCP Port:443 Size:1500 Priority:8
...
```

---

## Data Files

| File | Description |
|------|-------------|
| `test_packets.csv` | 291 test packets (random mix of allowed/blocked) |
| `blacklist.csv` | 24,339 blocked IP addresses |

---

## Priority Levels

| Traffic Type | Port | Priority |
|--------------|------|----------|
| DNS | 53 | 9 (highest) |
| HTTP/HTTPS | 80/443 | 8 |
| Other | any | 5 (default) |

---

## Academic Features

- **O(1) IP Lookup**: 4-level trie + BitVector
- **Lock-Free Queue**: Explicit memory ordering (acquire/release)
- **Priority Scheduling**: Max heap implementation

---

## License

Academic Project