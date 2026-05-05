#!/usr/bin/env python3
"""
Generate packets.csv from blacklist.csv for ArborFlow testing.

Process:
1. Read blacklist.csv and cut to 10k entries
2. Take 5k random entries from those 10k
3. Generate 15k random IPs NOT in blacklist
4. For each IP, generate protocol, port, size, and priority
5. Save as packets.csv

CSV Format: dest_ip, protocol, port, size, priority
- protocol: 6 (TCP) or 17 (UDP)
- port: 80/443 (HTTP/HTTPS if TCP), 53 (DNS if UDP), random otherwise
- size: random 64-1500 bytes
- priority: 8 (HTTP/HTTPS), 9 (DNS), 5 (default)
"""

import csv
import random
import ipaddress
import os

# Set seed for reproducibility
random.seed(42)

BLACKLIST_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../data/blacklist.csv")
OUTPUT_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../data/packets.csv")

# Protocol constants
TCP = 6
UDP = 17

# Port constants
HTTP = 80
HTTPS = 443
DNS = 53

def read_blacklist(limit=None):
    """Read blacklist IPs, optionally limit to N entries."""
    ips = []
    try:
        with open(BLACKLIST_FILE, 'r') as f:
            for line in f:
                ip = line.strip()
                if ip:
                    ips.append(ip)
                    if limit and len(ips) >= limit:
                        break
    except FileNotFoundError:
        print(f"Error: {BLACKLIST_FILE} not found")
        return []
    return ips

def generate_random_ip():
    """Generate a random IP address."""
    return str(ipaddress.IPv4Address(random.randint(0, 2**32 - 1)))

def generate_non_blacklist_ips(blacklist_set, count=15000):
    """Generate IPs that are NOT in blacklist."""
    generated = []
    attempts = 0
    max_attempts = count * 10
    
    while len(generated) < count and attempts < max_attempts:
        ip = generate_random_ip()
        if ip not in blacklist_set:
            generated.append(ip)
        attempts += 1
    
    if len(generated) < count:
        print(f"Warning: Only generated {len(generated)}/{count} non-blacklist IPs")
    
    return generated

def get_priority_and_port(protocol):
    """
    Determine protocol, port, and priority.
    
    Priority rules (from capture.c):
    - HTTP/HTTPS (TCP port 80/443): priority 8
    - DNS (UDP port 53): priority 9
    - Default: priority 5
    """
    if protocol == TCP:
        # 70% HTTP, 30% HTTPS, rest random TCP ports
        rand = random.random()
        if rand < 0.35:
            return 80, 8  # HTTP
        elif rand < 0.7:
            return 443, 8  # HTTPS
        else:
            return random.randint(1024, 65535), 5  # Random TCP port
    else:  # UDP
        # 50% DNS, 50% random UDP ports
        if random.random() < 0.5:
            return 53, 9  # DNS
        else:
            return random.randint(1024, 65535), 5  # Random UDP port

def generate_packets_csv():
    """Generate packets.csv with 20k entries (5k blacklist + 15k non-blacklist)."""
    
    print("📖 Reading blacklist.csv...")
    blacklist_all = read_blacklist(limit=10000)
    print(f"   Loaded {len(blacklist_all)} IPs from blacklist (limited to 10k)")
    
    # Convert to set for O(1) lookup
    blacklist_set = set(blacklist_all)
    
    print("🎲 Selecting 5k random IPs from blacklist...")
    blacklist_selected = random.sample(blacklist_all, 5000)
    print(f"   Selected {len(blacklist_selected)} IPs")
    
    print("🎲 Generating 15k non-blacklist IPs...")
    non_blacklist = generate_non_blacklist_ips(blacklist_set, count=15000)
    print(f"   Generated {len(non_blacklist)} IPs")
    
    # Combine all IPs
    all_ips = blacklist_selected + non_blacklist
    print(f"📊 Total packets: {len(all_ips)}")
    
    # Generate packets
    print("📝 Generating packet data...")
    packets = []
    for ip in all_ips:
        # Randomize protocol
        protocol = random.choice([TCP, UDP])
        port, priority = get_priority_and_port(protocol)
        size = random.randint(64, 1500)
        
        packets.append({
            'dest_ip': ip,
            'protocol': protocol,
            'port': port,
            'size': size,
            'priority': priority
        })
    
    # Write to CSV
    print(f"💾 Writing to {OUTPUT_FILE}...")
    try:
        with open(OUTPUT_FILE, 'w', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=['dest_ip', 'protocol', 'port', 'size', 'priority'])
            writer.writeheader()
            writer.writerows(packets)
        
        # Verify
        file_size = os.path.getsize(OUTPUT_FILE)
        print(f"✅ Success! Created {OUTPUT_FILE}")
        print(f"   File size: {file_size / 1024:.2f} KB")
        print(f"   Total packets: {len(packets)}")
        print(f"\n📊 Packet Statistics:")
        
        # Count priority distribution
        priority_5 = sum(1 for p in packets if p['priority'] == 5)
        priority_8 = sum(1 for p in packets if p['priority'] == 8)
        priority_9 = sum(1 for p in packets if p['priority'] == 9)
        
        blacklist_count = sum(1 for p in packets if p['dest_ip'] in blacklist_selected)
        
        print(f"   - Default (5): {priority_5}")
        print(f"   - HTTP/HTTPS (8): {priority_8}")
        print(f"   - DNS (9): {priority_9}")
        print(f"   - Blacklisted IPs: {blacklist_count}")
        print(f"   - Non-blacklisted IPs: {len(packets) - blacklist_count}")
        
    except Exception as e:
        print(f"❌ Error writing CSV: {e}")
        return False
    
    return True

if __name__ == '__main__':
    success = generate_packets_csv()
    exit(0 if success else 1)
