import threading
import queue
import socket
import serial
import struct
import time
import sys

# === CONFIGURATION ===
SERIAL_PORT = 'COM5'            # Your Teensy COM port
BAUDRATE = 115200
SIMULINK_IP = '127.0.0.1'
SIMULINK_RECV_PORT = 25001      # Simulink → Python
SIMULINK_SEND_PORT = 25000      # Python → Simulink
FLOAT_COUNT_RX = 1              # Number of doubles Simulink sends
FLOAT_COUNT_TX = 2              # Number of doubles Teensy sends
# ======================

# === SHARED QUEUES ===
hil_to_simulink = queue.Queue()
simulink_to_teensy = queue.Queue()
user_commands = queue.Queue()

# === EXIT EVENT ===
stop_event = threading.Event()

# === SERIAL SETUP ===
ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=0.1)

# === UDP SOCKETS ===
udp_recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp_recv_sock.bind(('0.0.0.0', SIMULINK_RECV_PORT))
udp_recv_sock.settimeout(0.1)

udp_send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


# === THREAD FUNCTIONS ===

def serial_read_thread():
    # """Reads from Teensy and routes HIL or shell responses"""
    buffer = ""
    while not stop_event.is_set():
        try:
            data = ser.read(ser.in_waiting or 1).decode('utf-8', errors='ignore')
            buffer += data
            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)
                line = line.strip()
                if line.startswith('#'):
                    try:
                        nums = list(map(float, line[1:].strip().split()))
                        if len(nums) == FLOAT_COUNT_TX:
                            hil_to_simulink.put(nums)
                        else:
                            print(f"[WARN] Malformed HIL packet: {line}")
                    except ValueError:
                        print(f"[WARN] Could not parse HIL packet: {line}")
                else:
                    print(f"[Teensy] {line}")
        except Exception as e:
            print(f"[ERROR] Serial read: {e}")
            stop_event.set()


def serial_write_thread():
    # """Sends user commands and HIL updates to Teensy"""
    while not stop_event.is_set():
        try:
            # Prefer HIL data if available
            try:
                hil_packet = simulink_to_teensy.get_nowait()
                msg = '#' + ' '.join(f'{x:.10g}' for x in hil_packet) + '\n'
                ser.write(msg.encode('utf-8'))
            except queue.Empty:
                pass

            # Also handle user input
            try:
                cmd = user_commands.get_nowait()
                if cmd.startswith('>'):
                    ser.write((cmd.strip() + '\n').encode('utf-8'))
            except queue.Empty:
                pass

        except Exception as e:
            print(f"[ERROR] Serial write: {e}")
            stop_event.set()


def simulink_receive_thread():
    # """Receives UDP packets from Simulink and pushes to Teensy"""
    while not stop_event.is_set():
        try:
            data, _ = udp_recv_sock.recvfrom(8 * FLOAT_COUNT_RX)
            if len(data) != 8 * FLOAT_COUNT_RX:
                print(f"[WARN] Wrong data size from Simulink: {len(data)}")
                continue
            floats = struct.unpack(f'<{FLOAT_COUNT_RX}d', data)
            simulink_to_teensy.put(floats)
        except socket.timeout:
            continue
        except Exception as e:
            print(f"[ERROR] Simulink receive: {e}")
            stop_event.set()


def simulink_send_thread():
    # """Sends latest HIL data to Simulink"""
    while not stop_event.is_set():
        try:
            data = hil_to_simulink.get(timeout=0.1)
            payload = struct.pack(f'<{FLOAT_COUNT_TX}d', *data)
            udp_send_sock.sendto(payload, (SIMULINK_IP, SIMULINK_SEND_PORT))
        except queue.Empty:
            continue
        except Exception as e:
            print(f"[ERROR] Simulink send: {e}")
            stop_event.set()


def user_input_thread():
    # """Accepts user input while everything runs"""
    while not stop_event.is_set():
        try:
            cmd = input()
            if(cmd.strip() == "@quit"):
                stop_event.set()
            else:
                user_commands.put(cmd)
        except Exception as e:
            print(f"[ERROR] Input thread: {e}")
            stop_event.set()


# === START THREADS ===
threads = [
    threading.Thread(target=serial_read_thread, daemon=True),
    threading.Thread(target=serial_write_thread, daemon=True),
    threading.Thread(target=simulink_receive_thread, daemon=True),
    threading.Thread(target=simulink_send_thread, daemon=True),
    threading.Thread(target=user_input_thread, daemon=True),
]
print("Simulation starting up...")

for t in threads:
    t.start()

print("Simulation startup complete. Type commands here. Use @quit to end the simulation.")

# Keep main thread alive durring simulation
try:
    while not stop_event.is_set():
        time.sleep(0.1)
except KeyboardInterrupt:
    print("Keyboard interrupt detected.")
    stop_event.set()

#Shut down the program
print("Simulation Shutting Down...")
for t in threads:
    t.join()
ser.close()
udp_send_sock.close()
udp_recv_sock.close()
print("Simulation Shut Down Complete")
sys.exit(0)