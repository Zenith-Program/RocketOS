import threading
import queue
import socket
import serial
import struct
import time
import sys
import os

CONFIG_FILE_NAME = 'HILBridge.cfg'
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CONFIG_FILE = os.path.join(SCRIPT_DIR, CONFIG_FILE_NAME)

# === DEFAULT CONFIG MAP ===
default_config = {
    "Target_COM_Port": 0,
    "Baud_Rate": 115200,
    "Bridge_UDP_Port": 25001,
    "Simulink_UDP_Port": 25000,
    "Values_Sent_Simulink_To_Target": 1,
    "Values_Sent_Target_To_Simulink": 1
}

# === PARSE CONFIG FILE OR CREATE DEFAULT ONE ===
def load_config(config_file):
    config = default_config.copy()

    if not os.path.exists(config_file):
        print(f"[INFO] Config file not found. Creating default at '{config_file}'.")
        with open(config_file, 'w') as f:
            for key, value in config.items():
                f.write(f"{key} = {value}\n")
    else:
        print(f"[INFO] Loading config from '{config_file}'.")
        with open(config_file, 'r') as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                if '=' in line:
                    key, value = map(str.strip, line.split('=', 1))
                    if key in config:
                        try:
                            config[key] = int(value)
                        except ValueError:
                            print(f"[WARNING] Invalid value for {key}, using default: {config[key]}")
                    else:
                        print(f"[WARNING] Ignoring unknown config value: {key}")

    return config

# === CONFIGURATION ===
config = load_config(CONFIG_FILE)
SERIAL_PORT         = f'COM{config["Target_COM_Port"]}'                
BAUDRATE            = config["Baud_Rate"]                  
SIMULINK_IP         = '127.0.0.1'           
BRIDGE_PORT         = config["Bridge_UDP_Port"]                 
SIMULINK_PORT       = config["Simulink_UDP_Port"]               
SIM_TO_TARGET_COUNT = config["Values_Sent_Simulink_To_Target"]             
TARGET_TO_SIM_COUNT = config["Values_Sent_Target_To_Simulink"]             
# ======================

# === SHARED VARIABLES ===
target_to_simulink_lock = threading.Lock()
target_to_simulink = None
target_to_simulink_new = False
simulink_to_target_lock = threading.Lock()
simulink_to_target = None
simulink_to_target_new = False
user_commands = queue.Queue()

# === EXIT EVENT ===
stop_event = threading.Event()

# === SERIAL SETUP ===
try:
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=0.1)
except Exception as e:
    print(f"[ERROR] Serial Port: {e}")
    stop_event.set()


# === UDP SOCKETS ===
if not stop_event.is_set():
    udp_recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_recv_sock.bind(('0.0.0.0', BRIDGE_PORT))
    udp_recv_sock.settimeout(0.1)

    udp_send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


# === THREAD FUNCTIONS ===

def serial_read_thread():
    # Reads from Target and routes HIL or shell responses
    buffer = ""
    while not stop_event.is_set():
        try:
            data = ser.read(ser.in_waiting or 1).decode('utf-8', errors='ignore')
            buffer += data
            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)
                stripedline = line.strip()
                if stripedline.startswith('#'):
                    try:
                        nums = list(map(float, stripedline[1:].strip().split()))
                        if len(nums) == TARGET_TO_SIM_COUNT:
                            with target_to_simulink_lock:
                                global target_to_simulink
                                global target_to_simulink_new
                                target_to_simulink = nums
                                target_to_simulink_new = True
                        else:
                            print(f"[ERROR] Size mismatch in HIL packet from target: {stripedline}, expected {TARGET_TO_SIM_COUNT}, actual was {len(nums)}")
                            stop_event.set()
                    except ValueError:
                        print(f"[ERROR] Could not parse HIL packet from target: {stripedline}")
                        stop_event.set()
                else:
                    print(line)
        except Exception as e:
            print(f"[ERROR] Serial read: {e}")
            stop_event.set()

def serial_write_thread():
    # Sends user commands and HIL updates to Target
    while not stop_event.is_set():
        try:
            # Send HIL data to target if available
            with simulink_to_target_lock:
                global simulink_to_target_new
                global simulink_to_target
                hil_packet = simulink_to_target
                newData = simulink_to_target_new
                simulink_to_target_new = False
            if(hil_packet and newData):
                msg = '#' + ' '.join(f'{x:.10g}' for x in hil_packet) + '\r'
                ser.write(msg.encode('utf-8'))

            # Send user commands to target
            try:
                cmd = user_commands.get_nowait()
                if cmd.startswith('>'):
                    ser.write((cmd.strip() + '\r').encode('utf-8'))
            except queue.Empty:
                pass

        except Exception as e:
            print(f"[ERROR] Serial write: {e}")
            stop_event.set()


def simulink_receive_thread():
    # Receives UDP packets from Simulink and pushes to Target
    while not stop_event.is_set():
        try:
            data, _ = udp_recv_sock.recvfrom(8 * SIM_TO_TARGET_COUNT)
            if len(data) != 8 * SIM_TO_TARGET_COUNT:
                print(f"[ERROR] Wrong data size from Simulink: {len(data)}, expected {SIM_TO_TARGET_COUNT}, actual was {len(data)/8}")
                stop_event.set()
                continue
            floats = struct.unpack(f'<{SIM_TO_TARGET_COUNT}d', data)
            with simulink_to_target_lock:
                global simulink_to_target
                global simulink_to_target_new
                simulink_to_target = floats
                simulink_to_target_new = True
        except socket.timeout:
            continue
        except Exception as e:
            print(f"[ERROR] Simulink receive: {e}")
            stop_event.set()


def simulink_send_thread():
    # Sends latest HIL data to Simulink
    while not stop_event.is_set():
        try:
            with target_to_simulink_lock:
                global target_to_simulink_new
                global target_to_simulink
                data = target_to_simulink
                newData = target_to_simulink_new
                target_to_simulink = None
                target_to_simulink_new = False
            if(data and newData):
                payload = struct.pack(f'<{TARGET_TO_SIM_COUNT}d', *data)
                udp_send_sock.sendto(payload, (SIMULINK_IP, SIMULINK_PORT))
        except queue.Empty:
            continue
        except Exception as e:
            print(f"[ERROR] Simulink send: {e}")
            stop_event.set()


def user_input_thread():
    # Accepts user input while everything runs
    while not stop_event.is_set():
        try:
            cmd = input()
            cmd = cmd.strip()
            if(cmd == "quit" or cmd == "exit"):
                stop_event.set()
            else:
                user_commands.put(cmd)
        except Exception as e:
            print(f"[ERROR] Input thread: {e}")
            stop_event.set()

# === START THREADS ===
if not stop_event.is_set():
    threads = [
        threading.Thread(target=serial_read_thread, daemon=True),
        threading.Thread(target=serial_write_thread, daemon=True),
        threading.Thread(target=simulink_receive_thread, daemon=True),
        threading.Thread(target=simulink_send_thread, daemon=True),
        threading.Thread(target=user_input_thread, daemon=True),
    ]
    print("[INFO] Simulation bridge starting up...")

    for t in threads:
        t.start()

    print("[INFO] Simulation bridge startup complete. Type commands here. Use 'quit' or 'exit' to end the simulation bridge.")

    # Keep main thread alive durring simulation
    try:
        while not stop_event.is_set():
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("[INFO] Keyboard interrupt detected.")
        stop_event.set()

    #Shut down the program
    print("[INFO] Simulation bridge shutting down...")
    for t in threads:
        t.join()
    ser.close()
    udp_send_sock.close()
    udp_recv_sock.close()

print("[INFO] Simulation bridge shut down complete.")
sys.exit(0)