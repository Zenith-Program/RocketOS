import socket
import struct
import time

# -----------------------------
# Configuration
# -----------------------------
SIMULINK_IP = '127.0.0.1'     # Assuming Simulink is on the same machine
SIMULINK_PORT = 25000         # Port Simulink is listening on
LOCAL_PORT = 25001            # Port Python listens on for Simulink response

SEND_FLOATS = [1.23, 4.56, 7.89]  # Example float data
FLOAT_FORMAT = '<3d'             # Format: 3 little-endian 32-bit floats

# -----------------------------
# Set up sending socket
# -----------------------------
send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# -----------------------------
# Set up receiving socket
# -----------------------------
recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
recv_sock.bind(('0.0.0.0', LOCAL_PORT))
recv_sock.settimeout(2.0)  # 2 second timeout

# -----------------------------
# Pack and send float data
# -----------------------------
payload = struct.pack(FLOAT_FORMAT, *SEND_FLOATS)
send_sock.sendto(payload, (SIMULINK_IP, SIMULINK_PORT))
print(f"Sent to Simulink: {SEND_FLOATS}")

# -----------------------------
# Wait for response
# -----------------------------
try:
    data, addr = recv_sock.recvfrom(1024)
    if len(data) % 8 != 0:
        raise ValueError("Received data size is not a multiple of 8 (float size)")
    num_floats = len(data) // 8
    response_floats = struct.unpack('<' + 'd'*num_floats, data)
    print(f"Received from Simulink: {response_floats}")
except socket.timeout:
    print("No response received from Simulink.")