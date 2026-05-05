**ArborFlow**

*High-Performance Network Processing Engine*

# **Project Overview**
ArborFlow is a real-time network processing and security engine built in C. It integrates network-level packet capture via libpcap with Bit Vectors, van Emde Boas Trees, Concurrent Queues, and Heaps to process live traffic with high performance.

ArborFlow simulates a mini firewall and traffic scheduler system. It captures real network packets from the system interface, filters malicious or suspicious IP addresses using advanced data structures, schedules packets based on priority using a heap, and processes them in real-time.


# **What is a Network Packet?**
When two computers communicate over a network, they do not send data as one continuous stream. Instead, data is broken up into small, fixed-size chunks called packets. Each packet travels independently across the network and is reassembled at the destination.

Think of it like sending a large book through the mail. Instead of mailing the entire book as one package, you tear out each chapter, put each one in a separate envelope with a label that says which book it belongs to, what chapter it is, and where it is going. The recipient collects all the envelopes and reassembles the book in the correct order.

Every packet contains two things:

- Headers — metadata describing where the packet came from, where it is going, what type of data it carries, and how to handle it.
- Payload — the actual data being sent (a fragment of a file, a web page, a DNS query, and so on).

## **Why Packets and Not Streams?**
Networks are shared infrastructure. If one computer sent a continuous stream of data, it would monopolize the entire link and block every other user. By breaking data into packets, the network can interleave traffic from many sources simultaneously. A router can send one packet from you, one packet from your neighbor, one packet from a server in Frankfurt — all on the same wire, thousands of times per second.

Packets also allow for error recovery. If one packet is lost or corrupted in transit, only that packet needs to be retransmitted, not the entire file.


# **How a Packet is Structured — The Layer Model**
A network packet is not a flat block of bytes. It is a series of nested envelopes, each added by a different layer of the networking stack. This design is called encapsulation. When you send data, each layer of software wraps the data from the layer above it in its own header, adding its own metadata. When the packet arrives, each layer unwraps its envelope and passes the contents up.

This layered model is defined by the OSI (Open Systems Interconnection) model and in practice by the TCP/IP model. ArborFlow works primarily with layers 2, 3, and 4.

*[ packet\_layer.png — insert here ]*

*Figure 1 — Packet layer encapsulation (Layers 2, 3, 4, 7)*
## **Layer 2 — Ethernet Frame**
This is the outermost envelope. It is added by the network interface card (NIC) on your machine and is only relevant on the local network segment. Once a packet crosses a router, the Ethernet frame is stripped and a new one is added for the next segment.

An Ethernet frame header is 14 bytes:

- 6 bytes — destination MAC address (the hardware address of the next hop)
- 6 bytes — source MAC address (your machine's hardware address)
- 2 bytes — EtherType field (tells the receiver what protocol is inside: 0x0800 = IPv4, 0x0806 = ARP, 0x86DD = IPv6)

In ArborFlow, the very first thing packet\_handler does is read this 2-byte EtherType field. If it is not 0x0800 (IPv4), the packet is discarded immediately. ARP, IPv6, and other protocols are ignored because the rest of the parsing code assumes IPv4.

## **Layer 3 — IP Packet**
Inside the Ethernet frame sits the IP packet. This is where network-level routing happens. The IP header contains the information routers need to forward the packet across the internet toward its destination.

An IPv4 header is at minimum 20 bytes:

- 4 bits — version (always 4 for IPv4)
- 4 bits — header length (IHL), measured in 32-bit words; multiply by 4 to get bytes
- 1 byte — DSCP/ECN (quality of service and congestion flags)
- 2 bytes — total length of the IP packet including payload
- 2 bytes — identification (for reassembling fragmented packets)
- 3 bytes — flags and fragment offset
- 1 byte — TTL (time to live; decremented by each router, dropped at zero)
- 1 byte — protocol number (6 = TCP, 17 = UDP, 1 = ICMP)
- 2 bytes — header checksum
- 4 bytes — source IP address
- 4 bytes — destination IP address

ArborFlow extracts the source IP, destination IP, and protocol number from this header. The IHL field is critical — because IP headers can have optional extra fields, the parser must multiply IHL by 4 to know exactly where the IP header ends and the next layer begins.

## **Layer 4 — TCP or UDP Segment**
Inside the IP packet sits either a TCP segment or a UDP datagram, depending on the protocol number in the IP header.

### **TCP (Transmission Control Protocol)**
TCP is a connection-oriented protocol. It guarantees delivery, ordering, and error checking. A TCP header is 20 bytes minimum:

- 2 bytes — source port number
- 2 bytes — destination port number
- 4 bytes — sequence number (position of this data in the stream)
- 4 bytes — acknowledgment number (confirms receipt up to this point)
- 1 byte — data offset (how long the TCP header is, in 32-bit words)
- 1 byte — flags (SYN, ACK, FIN, RST, PSH, URG)
- 2 bytes — window size (flow control)
- 2 bytes — checksum
- 2 bytes — urgent pointer

ArborFlow reads the destination port from the TCP header to assign priority. Port 80 (HTTP) and port 443 (HTTPS) receive priority 8.

### **UDP (User Datagram Protocol)**
UDP is connectionless and lightweight. There is no handshake, no guaranteed delivery, and no ordering. A UDP header is exactly 8 bytes:

- 2 bytes — source port
- 2 bytes — destination port
- 2 bytes — length
- 2 bytes — checksum

ArborFlow reads the destination port from the UDP header to check for DNS traffic. Port 53 (DNS) receives the highest priority of 9, because DNS lookups block all further network activity until resolved.

## **Layer 7 — Application Payload**
Everything above Layer 4 is the application payload — the actual content being communicated. This could be an HTTP request for a web page, the bytes of an image file, a DNS query asking for an IP address, an email, a video stream chunk, or anything else. ArborFlow does not parse this layer. It stops at Layer 4, extracts the port numbers for priority assignment, and treats the rest as opaque data.


# **How C Reads a Packet — Bytes and Pointers**
When libpcap captures a packet and calls packet\_handler, it passes a pointer to the raw bytes of that packet in memory. In C, this is typed as const u\_char \*, which is simply a pointer to the first byte of a contiguous block of memory.

There is no high-level object, no automatic parsing, no named fields. C sees the packet as nothing more than a sequence of bytes at a memory address. The programmer is responsible for knowing the structure of each header and manually navigating to the right byte offset to read each field.

## **The Pointer Arithmetic Approach**
The key technique is casting a raw byte pointer to a struct pointer. The C standard defines exactly how structs are laid out in memory. By pointing a struct pointer at the right offset within the raw packet bytes, the struct fields map directly onto the packet bytes — no copying, no parsing loop. The struct is just a lens that imposes named fields onto raw memory.

For example, to read the IP header, ArborFlow jumps past the 14-byte Ethernet header and casts the resulting pointer to struct ip. The field ip\_hl then reads bits 4-7 of the byte at that offset, ip\_p reads the byte at offset 9, ip\_src reads bytes 12-15, and so on — all by the struct layout, not by manual indexing.

To reach the TCP or UDP header, the code must jump past the IP header. Since IP headers can have variable-length options, the jump distance is not always 20 bytes — it is ip\_hl multiplied by 4. This is why the IHL field exists: to tell you where the payload begins.

## **Byte Order — Big Endian and Little Endian**
Multi-byte integers can be stored in two ways. In little-endian systems (x86/x64, which is most modern hardware), the least significant byte is stored first. In big-endian systems (network byte order), the most significant byte is stored first.

Networks always use big-endian byte order. x86 CPUs use little-endian. This means that a 16-bit port number like 80 (0x0050) arrives on the wire as the bytes 0x00, 0x50 in that order — correct. But if you read those two bytes directly as a little-endian uint16\_t, the CPU interprets them as 0x5000, which is 20480 — completely wrong.

This is why every multi-byte field read from a packet header must be converted before use. The functions ntohs (network to host short, 16-bit) and ntohl (network to host long, 32-bit) perform this byte swap. Skipping them means every port comparison and IP address comparison silently produces wrong results.

Single-byte fields like the protocol number do not need conversion — there is only one byte, so byte order is irrelevant.

## **Why u\_char and Not char or int?**
Network packet bytes have values from 0 to 255. The plain char type in C is signed on most platforms, meaning it holds -128 to +127. A byte value of 200 would be interpreted as -56, breaking all comparisons and arithmetic.

u\_char (unsigned char) holds 0 to 255 and maps directly to raw byte values. It is also 1 byte wide, which means pointer arithmetic on u\_char \* advances exactly one byte at a time. This is essential for the offset calculations used to navigate between headers. Using int would move 4 bytes per step, destroying all the offset math.


# **What is libpcap?**
libpcap (Packet Capture library) is a C library that provides a portable interface for capturing network packets directly from a network interface. It sits between your application and the operating system's network stack, giving you access to raw packets before the OS hands them to individual sockets.

Without libpcap, writing a packet capture tool would require writing different, platform-specific code for Linux, macOS, and BSD. libpcap abstracts all of this into a single API. Tools like Wireshark, tcpdump, and Snort are all built on libpcap.

## **How libpcap Works**
libpcap puts the network interface into promiscuous mode, which causes the NIC to pass all packets it sees on the wire to the OS, not just those addressed to the machine. The OS kernel then copies these packets into a ring buffer in kernel memory.

libpcap's pcap\_loop function reads from this kernel buffer in a tight loop. For each packet, it calls the callback function you registered — in ArborFlow, this is packet\_handler. The callback receives a pointer directly into libpcap's internal buffer; the data is not copied again. This is why you must not store the packet pointer beyond the duration of the callback — libpcap will overwrite that buffer with the next packet.

## **BPF Filters**
libpcap uses the Berkeley Packet Filter (BPF) system to filter packets in the kernel before they are ever copied to userspace. BPF is a small bytecode language that runs inside the kernel. You write a filter expression in a human-readable format (such as 'ip' or 'tcp port 443'), libpcap compiles it to BPF bytecode with pcap\_compile, and installs it with pcap\_setfilter.

After the filter is installed, only matching packets are copied from the kernel ring buffer to your application. Packets that do not match are discarded entirely in kernel space — they never consume userspace CPU time. This is a significant performance advantage when you only care about a subset of traffic.

ArborFlow installs the filter 'ip', which discards ARP, IPv6, VLAN, and all other non-IPv4 traffic before packet\_handler is ever called.

## **The Capture Thread**
pcap\_loop blocks the thread it runs on. It never returns until pcap\_breakloop is called from another thread. This is why ArborFlow runs pcap\_loop inside a dedicated POSIX thread created with pthread\_create. The main thread continues running the rest of the system — the scheduler, the gatekeeper, the queue consumer — while the capture thread feeds packets in concurrently.

When capture\_stop is called, it sets a flag, calls pcap\_breakloop to unblock pcap\_loop, and then calls pthread\_join to wait for the capture thread to finish before freeing any resources. This ordering prevents the capture thread from accessing memory that has already been freed.


# **Architecture Flow**

|**Stage**|**Component**|**Purpose**|
| :- | :- | :- |
|1|Capture Engine (libpcap)|Captures raw packets from the network interface|
|2|Packet Parser|Extracts IP addresses, protocol, port numbers, and size|
|3|Concurrent Queue|Lock-free buffer between capture thread and processing thread|
|4|Gatekeeper|Filters malicious IPs using Bit Vector and vEB Tree|
|5|Scheduler (Max Heap)|Orders packets by priority for processing|
|6|Process / Output|Handles the packet — logs, forwards, or drops|


# **Core Data Structures**

|**Module**|**Data Structure**|**Complexity**|
| :- | :- | :- |
|Capture Engine|Networking (libpcap), OS threads|—|
|Concurrent Queue|Lock-Free SPSC Ring Buffer|O(1) enqueue and dequeue|
|Gatekeeper — Layer 1|Bit Vector|O(1) prefix lookup|
|Gatekeeper — Layer 2|van Emde Boas Tree|O(log log U) exact IP match|
|Scheduler|Max Heap (Priority Queue)|O(log n) insert, O(1) peek|
|Session Manager|Splay Tree (optional)|O(log n) amortized|


# **Priority Assignment**
Each captured packet is assigned a numeric priority before being enqueued. The scheduler uses this value to determine processing order.

|**Traffic Type**|**Condition**|**Priority**|
| :- | :- | :- |
|DNS|UDP, destination port 53|9 — Very High|
|HTTP / HTTPS|TCP, destination port 80 or 443|8 — High|
|All other traffic|Any other protocol or port|5 — Normal|

DNS receives the highest priority because unresolved DNS queries block all dependent network activity. A single stalled DNS lookup can delay every subsequent connection that depends on it.


# **Gatekeeper — Firewall Logic**
The Gatekeeper module sits between the concurrent queue and the scheduler. Every packet dequeued from the capture queue passes through two filtering layers before it is admitted to the scheduler.

## **Layer 1 — Bit Vector (O(1) prefix filtering)**
A bit vector is an array of bits indexed by value. For IP filtering, each bit corresponds to a possible value in an IP address field (a subnet prefix, for example). Checking whether a prefix is blocked requires only a single memory read and a bitwise test — O(1) regardless of how many prefixes are blocked.

This layer acts as a fast pre-filter. If a packet's source IP matches a blocked prefix, it is dropped immediately without consulting the more expensive vEB Tree.

## **Layer 2 — van Emde Boas Tree (O(log log U) exact matching)**
The van Emde Boas Tree provides exact IP address lookup. Unlike a hash table, it supports predecessor and successor queries in O(log log U) time, where U is the universe size (2^32 for IPv4). It is particularly well-suited to dense sets of blocked IPs where spatial locality matters.

Only packets that pass the Bit Vector layer reach the vEB Tree check. If the exact source IP is found in the blocked set, the packet is dropped. Otherwise it is admitted to the scheduler.


# **Project Structure**
- core\_engine/
  - Makefile — build rules
  - main.c — entry point, wires all components together
  - include/ — all header files (.h)
  - src/ — all source implementations (.c)
  - scheduler/ — packet struct, heap scheduler
  - tests/ — gatekeeper tests, capture demo


# **Building and Running**
ArborFlow requires Linux or WSL because it uses libpcap, POSIX threads (pthread), and Linux network headers (netinet/ip.h, unistd.h) that are not available natively on Windows.

## **Step 1 — Navigate**
cd ArborFlow/core\_engine
## **Step 2 — Build**
make clean

make
## **Step 3 — Run**
sudo ./arborflow eth0

Root privileges (sudo) are required because opening a network interface for raw packet capture is a privileged operation. Replace eth0 with your actual interface name, which you can find by running ip link or checking the device list printed at startup.


# **Sample Output**
[PROCESS] 91.189.91.83 -> 172.19.231.46   Priority:5  Size:1494

[PROCESS] 172.19.231.46 -> 91.189.91.83   Priority:8  Size:86

Each line represents one packet that passed the Gatekeeper and was processed by the scheduler. The source and destination IP addresses are real addresses captured from live traffic. Priority is assigned dynamically based on the protocol and port. Size is the total packet length in bytes including all headers.


# **Team Contributions**

|**Component**|**Contributor Focus**|
| :- | :- |
|Capture Engine|Networking, libpcap integration, OS threading|
|Gatekeeper|Advanced Trees — Bit Vector and van Emde Boas|
|Scheduler|Heap implementation, priority queue|
|Concurrent Queue|Lock-free data structures, atomic operations|
|ML / Visualization|Python (optional extension)|


# **Future Enhancements**
- Machine learning-based anomaly detection for identifying unusual traffic patterns
- Web dashboard for real-time visualization of packet flow and drop rates
- Advanced QoS rules with per-flow rate limiting
- Distributed packet processing across multiple capture threads


# **Conclusion**
ArborFlow demonstrates how advanced data structures and systems programming combine to build a real-world, high-performance network engine. Every component — from the lock-free queue that decouples capture from processing, to the two-layer gatekeeper that filters malicious traffic without a mutex in the hot path, to the max heap that ensures high-priority packets are always served first — is chosen because its algorithmic properties match the constraints of real-time network processing.
ArborFlow — High-Performance Network Processing Engine  |  Page 
