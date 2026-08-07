#!/usr/bin/env python3
import socket
import json
import time
import os
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

# 256-bit Pre-Shared Key matching the C++ Gateway
KEY = bytes([
    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
    0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
    0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
])

IP_ADDRESS = "127.0.0.1"
PORT = 8080

def send_secure_telemetry():
    aesgcm = AESGCM(KEY)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    print(f"Sending AES-256-GCM Encrypted Telemetry to {IP_ADDRESS}:{PORT}...")

    try:
        while True:
            # 1. Prepare raw payload
            payload = {
                "client_id": "amr_robot_01",
                "battery": 91.2,
                "temperature": 38.5,
                "velocity_x": 0.85
            }
            plaintext = json.dumps(payload).encode('utf-8')

            # 2. Generate 12-byte IV (Nonce)
            iv = os.urandom(12)

            # 3. Encrypt (Returns Ciphertext + 16-byte Auth Tag concatenated)
            encrypted_data = aesgcm.encrypt(iv, plaintext, None)
            ciphertext = encrypted_data[:-16]
            tag = encrypted_data[-16:]

            # 4. Binary Wire Format: [12B IV][16B Tag][Ciphertext]
            wire_packet = iv + tag + ciphertext
            sock.sendto(wire_packet, (IP_ADDRESS, PORT))

            print(f"[SENT] {len(wire_packet)} B encrypted frame | IV: {iv.hex()[:8]}...")
            time.sleep(1.0)

    except KeyboardInterrupt:
        print("\nStopping client.")
    finally:
        sock.close()

if __name__ == "__main__":
    send_secure_telemetry()