# Quick Start Guide - CSV Packet Testing

## Quick Summary

You now have a working **CSV-based packet generator and processor** for ArborFlow. Instead of capturing live packets from your network, the system reads pre-generated packets from `packets.csv`.

## Files Created/Modified

### New Files
- **`packets.csv`** - 20,000 pre-generated packets
- **`generate_packets.py`** - Python script to regenerate packets
- **`core_engine/test_csv.c`** - Test program for CSV mode
- **`CSV_IMPLEMENTATION_SUMMARY.md`** - Detailed documentation

### Modified Files
- **`core_engine/include/capture.h`** - Added CSV mode structs
- **`core_engine/src/capture.c`** - Added CSV reading functions
- **`core_engine/Makefile`** - Added test_csv build target
- **`core_engine/include/bit_vector.h`** - Fixed bv_destroy declaration
- **`core_engine/src/bit_vector.c`** - Implemented bv_destroy

## How to Use

### 1. Regenerate packets.csv (if needed)
```bash
cd /Users/prathmesh/ArborFlow_ADS_CP
python3 generate_packets.py
```

Expected output:
```
✅ Success! Created /Users/prathmesh/ArborFlow_ADS_CP/packets.csv
   File size: 557.14 KB
   Total packets: 20000
   - Default (5): 7957
   - HTTP/HTTPS (8): 7048
   - DNS (9): 4995
```

### 2. Compile the project
```bash
cd core_engine
make clean
make           # Builds arborflow (live capture mode)
make test_csv  # Builds test_csv (CSV mode)
```

### 3. Run CSV test
```bash
./test_csv ../packets.csv <delay_ms> <src_ip_hex>
```

**Examples:**
```bash
# Default: 10ms delay, source 127.0.0.1
./test_csv ../packets.csv

# 5ms delay, source IP 192.168.1.1
./test_csv ../packets.csv 5 0xC0A80101

# 0ms delay (as fast as possible), source 10.0.0.1
./test_csv ../packets.csv 0 0x0A000001

# 50ms delay (slow simulation)
./test_csv ../packets.csv 50 0xFFFFFFFF
```

### 4. Original live capture still works
```bash
./arborflow en0
# or list available devices:
./arborflow
```

## CSV File Format

`packets.csv` has this structure:

```
dest_ip,protocol,port,size,priority
117.3.140.34,17,2098,1148,5
103.154.231.122,17,53,434,9
142.44.233.169,6,80,356,8
```

**Columns:**
- **dest_ip**: Destination IP address (5,000 from blacklist + 15,000 random)
- **protocol**: 6 (TCP) or 17 (UDP)
- **port**: Network port number
  - 80 = HTTP
  - 443 = HTTPS
  - 53 = DNS
  - Others = random
- **size**: Packet size in bytes (64-1500)
- **priority**: Processing priority
  - 9 = DNS (highest)
  - 8 = HTTP/HTTPS
  - 5 = Default (lowest)

## What Happens During Test

1. **CSV Read**: Loads packets from CSV file line by line
2. **Delay**: Waits `delay_ms` milliseconds between packets
3. **Enqueue**: Adds packet to concurrent queue
4. **Dequeue**: Main loop reads from queue
5. **Filter**: Checks destination IP against blacklist
   - If blacklisted: **🚫 BLOCKED**
   - If not blacklisted: **✅ PASSED**
6. **Schedule**: Inserts into priority heap
7. **Process**: Extracts from heap by priority (highest first)

## Example Test Output

```
[1] ✅ PASSED: 192.168.1.1 -> 117.3.140.34 | Proto:17 Size:1148 Priority:5
    ▶ PROCESSING: 192.168.1.1 -> 117.3.140.34 | Priority:5 Size:1148 (from heap)

[2] ✅ PASSED: 192.168.1.1 -> 103.154.231.122 | Proto:17 Size:434 Priority:9
    ▶ PROCESSING: 192.168.1.1 -> 103.154.231.122 | Priority:9 Size:434 (from heap)

[3] 🚫 BLOCKED: 192.168.1.1 -> 1.13.22.203 | Proto:17 Size:512 Priority:5
    (This destination IP is in blacklist)
```

## Statistics Summary

After the test completes, you'll see:
```
📈 Statistics:
   Total packets processed: 21
   Packets blocked (blacklisted): 0
   Packets scheduled: 21
   Elapsed time: 0 seconds
   Avg packets/sec: 21.0
```

## Key Features

### Reproducible
- Same packets every run
- Fixed random seed (42)
- Deterministic IP/protocol distribution

### Configurable
- Adjust delay per test: `delay_ms` parameter
- Change source IP: `src_ip_hex` parameter
- Regenerate with new rules: modify `generate_packets.py`

### Realistic
- 5,000 blacklisted + 15,000 normal packets
- Proper protocol mix (50% TCP, 50% UDP)
- Correct priority assignments:
  - HTTP/HTTPS = 8
  - DNS = 9
  - Others = 5

### Verifiable
- Shows which packets blocked vs passed
- Displays priority-based processing
- Tracks statistics

## Troubleshooting

### Issue: "Cannot open packets.csv"
```bash
cd /Users/prathmesh/ArborFlow_ADS_CP
python3 generate_packets.py
# Then try test again
```

### Issue: "Queue full, dropping packets"
- Increase delay_ms to give more processing time
- Or increase Q_SIZE in concurrent_q.h and recompile

### Issue: "0 packets processed"
- Make sure csv_file path is correct
- Default path is `../packets.csv` (relative to core_engine/)

### Issue: All packets blocked
- This would mean source_ip is in blacklist
- Use a different source IP: `0x0A000001` (10.0.0.1)

## Data Verification

Check if packets.csv is valid:
```bash
# Count rows
wc -l packets.csv  # Should be 20001 (header + 20000 data)

# View sample
head -5 packets.csv
tail -5 packets.csv

# Check format
grep "^[0-9]" packets.csv | head -3
```

## Architecture

```
┌─────────────────────────────────────────────────────┐
│ packets.csv (20,000 pre-generated packets)          │
└──────────────────┬──────────────────────────────────┘
                   │
                   ├─> csv_capture_thread_func()
                   │   ├─ Read CSV line
                   │   ├─ Parse dest_ip, protocol, port, size, priority
                   │   ├─ Create Packet struct
                   │   ├─ Sleep delay_ms
                   │   └─ Enqueue to concurrent queue
                   │
                   ▼
        ┌──────────────────────┐
        │ Concurrent Queue     │ (1024 capacity)
        │ (SPSC, atomic ops)   │
        └──────┬───────────────┘
               │
               ├─> Main Loop Dequeue
               │
               ├─> [Gatekeeper Filter]
               │   ├─ Check if dest_ip in blacklist
               │   ├─ Block if yes
               │   └─ Pass if no
               │
               ├─> [Scheduler Insert]
               │   ├─ Insert by priority
               │   └─ Maintain heap property
               │
               ├─> [Scheduler Extract]
               │   ├─ Get highest priority packet
               │   └─ Process
               │
               ▼
          Statistics & Output
```

## Next Steps

1. **Generate packets** (already done)
   ```bash
   python3 generate_packets.py
   ```

2. **Compile** (already done)
   ```bash
   make test_csv
   ```

3. **Test with different scenarios**
   ```bash
   ./test_csv ../packets.csv 0    # No delay
   ./test_csv ../packets.csv 100  # 100ms delay
   ./test_csv ../packets.csv 1000 # 1 second delay
   ```

4. **Modify CSV generation** (if needed)
   - Edit `generate_packets.py` to change:
     - Blacklist count (currently 5000)
     - Non-blacklist count (currently 15000)
     - Protocol distributions
     - Port assignments
     - Packet sizes

## Summary

✅ **Done:**
- CSV packet generator
- CSV reader in C
- Concurrent queue integration
- Firewall filtering
- Priority scheduling
- Test program with statistics

🚀 **Ready to Use:**
- `./test_csv ../packets.csv 10 0xC0A80101`
- Replace live capture with reproducible CSV testing
- Simulate various network conditions with different delays
