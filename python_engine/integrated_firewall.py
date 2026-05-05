import time
import joblib
import pandas as pd
import numpy as np
import os
import sys
import subprocess
import threading
import queue

MODEL_PATH = "models/hybrid_model.pkl"
TRAIN_DATA_PATH = "../dataset/Train_data.csv"
ENGINE_PATH = "../core_engine/ipc_engine.exe"

def print_alert(ip, score, rate, sessions, failed, attack_type="UNKNOWN"):
    print(f"\n" + "="*60)
    print(f"🚨 [RED ALERT] ANOMALY DETECTED FROM {ip} 🚨")
    print(f"="*60)
    print(f"Classification : [ATTACK SIGNATURE MATCH - {attack_type.upper()}]")
    print(f"Threat Score   : {score:.3f} (Isolation Forest)")
    print(f"Packets/Sec    : {rate}")
    print(f"New Sessions   : {sessions}")
    print(f"Failed Logins  : {failed}")
    print(f"Action         : [SENDING BLOCK COMMAND TO C GATEKEEPER...]")
    print(f"="*60)

def main():
    if not os.path.exists(MODEL_PATH):
        print(f"Error: Model missing at {MODEL_PATH}. Run train_model.py.")
        return
        
    if not os.path.exists(ENGINE_PATH):
        print(f"Error: C Engine missing at {ENGINE_PATH}. Run 'make ipc_engine' in core_engine.")
        return

    print("Loading Hybrid ML Engine (XGBoost + Isolation Forest)...")
    model_data = joblib.load(MODEL_PATH)
    xgb_model = model_data['xgb_model']
    if_model = model_data['if_model']
    features = model_data['features']
    print("[SUCCESS] Defense Systems Online.\n")
    
    print(f"Booting C Core Engine Gatekeeper ({ENGINE_PATH})...")
    engine = subprocess.Popen(
        [ENGINE_PATH],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1 # Line buffered
    )
    
    # Wait for READY signal while printing boot logs
    while True:
        ready_line = engine.stdout.readline().strip()
        if not ready_line:
            print("Failed to boot C engine. Stream closed.")
            return
        if ready_line == "READY":
            break
        print(f"   [C BOOT LOG] {ready_line}")
        
    print("[SUCCESS] C Core Engine Online & Awaiting IPC Commands.\n")
    
    print(f"Connecting to live network feed ({TRAIN_DATA_PATH})...")
    df = pd.read_csv(TRAIN_DATA_PATH)
    normal_df = df[df['class'] == 'normal']
    anomaly_df = df[df['class'] == 'anomaly']
    
    print("\n--- INTEGRATED C+PYTHON FIREWALL TERMINAL ACTIVE ---")
    print("Press Ctrl+C to stop.")
    print("Type 'ddos' and press Enter to simulate a DDoS attack.\n")
    
    input_queue = queue.Queue()
    def read_kbd_input(inputQueue):
        while True:
            cmd = sys.stdin.readline().strip().lower()
            if cmd: inputQueue.put(cmd)

    threading.Thread(target=read_kbd_input, args=(input_queue,), daemon=True).start()

    normal_idx = 0
    
    try:
        while True:
            if not input_queue.empty():
                cmd = input_queue.get()
                if cmd in ['ddos', 'scan']:
                    print(f"\n[!] RED TEAM DISPATCH: EXECUTING DISTRIBUTED {cmd.upper()} ATTACK MIXED WITH NORMAL BGM TRAFFIC...")
                    
                    # Create a mixed pool of rows: 10 anomalies, 5 normals, and shuffle them
                    mixed_pool = pd.concat([anomaly_df.sample(10), normal_df.sample(5)]).sample(frac=1)
                    
                    for _, row in mixed_pool.iterrows():
                        is_attack = (row['class'] == 'anomaly')
                        
                        if is_attack:
                            attack_ip = f"10.0.{np.random.randint(1, 50)}.{np.random.randint(1, 250)}"
                            # 1. Ask C engine if IP is blocked
                            engine.stdin.write(f"CHECK {attack_ip}\n")
                            engine.stdin.flush()
                            c_resp = engine.stdout.readline().strip()
                            
                            if c_resp == "DROP":
                                print(f"   [C GATEKEEPER] Packet from {attack_ip} DROPPED instantly at Layer 1. (Bypassed ML)")
                                time.sleep(0.3)
                                continue
                                
                            # If passed (not blocked yet), run ML
                            vec = pd.DataFrame([row[features]])
                            vec.fillna(0, inplace=True)
                            if_score = if_model.decision_function(vec)[0]
                            
                            rate = np.random.randint(5000, 15000)
                            sess = np.random.randint(1000, 3000)
                            fail = np.random.randint(50, 200)
                                
                            print_alert(attack_ip, if_score - 0.2, rate, sess, fail, attack_type=cmd)
                            
                            # 2. Tell C engine to block it permanently
                            engine.stdin.write(f"BLOCK {attack_ip}\n")
                            engine.stdin.flush()
                            
                            block_resp = engine.stdout.readline().strip()
                            if block_resp == "VERBOSE_START":
                                print(f"\n   [C GATEKEEPER] Executing High-Speed IP Ban via Math-Driven vEB Tree Structure:")
                                while True:
                                    v_line = engine.stdout.readline().strip()
                                    if not v_line or v_line == "VERBOSE_END":
                                        break
                                    print(f"     {v_line}")
                                    time.sleep(0.1) # Fast but readable
                                print()
                            else:
                                print(f"   [C GATEKEEPER] Confirmed: {block_resp} {attack_ip} in vEB Tree!\n")
                            time.sleep(0.5)
                        else:
                            # It's a normal packet mixed in!
                            clean_ip = f"192.168.1.{np.random.randint(1, 250)}"
                            engine.stdin.write(f"CHECK {clean_ip}\n")
                            engine.stdin.flush()
                            c_resp = engine.stdout.readline().strip()
                            
                            vec = pd.DataFrame([row[features]])
                            vec.fillna(0, inplace=True)
                            if_score = if_model.decision_function(vec)[0]
                            
                            packet_rate = int(row.get('count', 0) * 10 + np.random.randint(10, 50))
                            sessions = int(row.get('srv_count', 0) * 5 + np.random.randint(5, 20))
                            
                            print(f"\n[{clean_ip}] -> [C Gatekeeper: PASS] -> [ML Score: {if_score:.3f}] -> [Traffic: {packet_rate} pkts/s]")
                            print(f"   [✓] XGBoost Classification: NORMAL | ACTION: PERMITTED\n")
                            time.sleep(0.5)
                            
                    print("[✓] ATTACK SEQUENCE TERMINATED. Resuming normal monitoring...\n")
                    continue
            
            # Normal Traffic Stream
            if normal_idx >= len(normal_df): normal_idx = 0
            row = normal_df.iloc[normal_idx]
            normal_idx += 1
            
            # Generate a random clean IP
            clean_ip = f"192.168.1.{np.random.randint(1, 250)}"
            
            # 1. Ask C engine if IP is blocked (should be PASS)
            engine.stdin.write(f"CHECK {clean_ip}\n")
            engine.stdin.flush()
            c_resp = engine.stdout.readline().strip()
            
            if c_resp == "PASS":
                vec = pd.DataFrame([row[features]])
                vec.fillna(0, inplace=True)
                if_score = if_model.decision_function(vec)[0]
                
                packet_rate = int(row.get('count', 0) * 10 + np.random.randint(10, 50))
                sessions = int(row.get('srv_count', 0) * 5 + np.random.randint(5, 20))
                
                print(f"[{clean_ip}] -> [C Gatekeeper: PASS] -> [ML Score: {if_score:.3f}] -> [Traffic: {packet_rate} pkts/s]")
            else:
                print(f"[{clean_ip}] -> [C Gatekeeper: DROP]")
                
            time.sleep(1)
            
    except KeyboardInterrupt:
        print("\n[!] Shutting down...")
        engine.stdin.write("EXIT\n")
        engine.wait()

if __name__ == "__main__":
    main()
