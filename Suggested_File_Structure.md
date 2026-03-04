## Suggested overall File Structure
```
ArborFlow/
├── Makefile                   # Compiles the C engine (Members 1, 2, 3)
├── run_system.sh              # Script to start C engine, Python backend, and Web UI
│
├── core_engine/               # THE C SYSTEM (Members 1, 2, 3)
│   ├── include/               # Header files (.h)
│   │   ├── capture.h          # Member 1: libpcap & thread structures
│   │   ├── concurrent_q.h     # Member 1: Lock-free queue for passing packets
│   │   ├── bit_vector.h       # Member 2: O(1) Blacklist
│   │   ├── veb_tree.h         # Member 2: IP Deep Inspection
│   │   ├── splay_tree.h       # Member 3: Active Session tracking
│   │   └── fib_heap.h         # Member 3: QoS Packet Priority Queue
│   │
│   └── src/                   # Implementation files (.c)
│       ├── main_engine.c      # Ties Members 1, 2, and 3 together
│       ├── capture.c          # Member 1
│       ├── concurrent_q.c     # Member 1
│       ├── bit_vector.c       # Member 2
│       ├── veb_tree.c         # Member 2
│       ├── splay_tree.c       # Member 3
│       └── fib_heap.c         # Member 3
│
├── ml_backend/                # THE INTELLIGENCE (Member 4)
│   ├── requirements.txt       # Python dependencies (scikit-learn, etc.)
│   ├── bridge.py              # Listens to Unix Sockets from the C engine
│   ├── train_model.py         # Script to train the Isolation Forest
│   └── models/
│       └── anomaly_model.pkl  # Saved, pre-trained ML model
│
├── web_dashboard/             # THE VISUALS (Member 5)
│   ├── package.json           # Node.js/React dependencies
│   ├── server.js              # Express server to host the WebSockets
│   └── public/                # Frontend files
│       ├── index.html         # Main dashboard HTML
│       ├── style.css          # CSS styling
│       └── app.js             # JS to update live graphs
│
└── hardware_alert/            # THE PHYSICAL NODE (Member 5)
    └── esp32_alert/
        └── esp32_alert.ino    # Arduino code for the ESP32 to trigger lights/buzzer
```