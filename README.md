# ArborFlow_ADS_CP
ArborFlow is a high-performance network security engine that uses Advanced Trees and Heaps in C to filter and schedule millions of packets in real-time.

## ArborFlow — Core Engine (Member 2: Gatekeeper)

## File Structure
```
ArborFlow/
└── core_engine/
    ├── Makefile
    ├── include/
    │   ├── bit_vector.h
    │   ├── veb_tree.h
    │   └── gatekeeper.h
    ├── src/
    │   ├── bit_vector.c
    │   ├── veb_tree.c
    │   └── gatekeeper.c
    └── tests/
        ├── test_gatekeeper.c
        ├── stress_test.c
        └── demo.c
```

## Running

Navigate to the core engine folder:
```bash
cd ArborFlow/core_engine
```

Run the test suite:
```bash
gcc -Wall -O2 -std=c11 -Iinclude -o gatekeeper_test src/bit_vector.c src/veb_tree.c src/gatekeeper.c tests/test_gatekeeper.c && ./gatekeeper_test
```

Run the stress test (1 million packets):
```bash
gcc -Wall -O2 -std=c11 -Iinclude -o stress_test src/bit_vector.c src/veb_tree.c src/gatekeeper.c tests/stress_test.c && ./stress_test
```

Run the demo:
```bash
gcc -Wall -O2 -std=c11 -Iinclude -o demo src/bit_vector.c src/veb_tree.c src/gatekeeper.c tests/demo.c && ./demo
```
