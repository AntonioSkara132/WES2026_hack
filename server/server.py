import socket
import threading
import tkinter as tk
from tkinter.scrolledtext import ScrolledText
import struct
import pyaudio
import sys

#########################################
# Configuration
#########################################
# TCP Text Settings
TEXT_IP = '172.16.55.105'
TEXT_PORT = 8000
MSG_SIZE = 64

# UDP Audio Settings
AUDIO_IP = '172.16.55.10'
AUDIO_PORT = 3333
CHUNK = 512
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 44100

def recvn(sock, n):
    data = b""
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if not packet: return None
        data += packet
    return data

class CombinedServerUI:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("ESP32 Control Center (Text + Audio)")
        
        # --- UI Elements ---
        self.text_area = ScrolledText(self.root, wrap=tk.WORD, width=70, height=20, state='disabled')
        self.text_area.pack(padx=10, pady=10)

        controls_frame = tk.Frame(self.root)
        controls_frame.pack(fill=tk.X, padx=10, pady=5)

        self.entry = tk.Entry(controls_frame, width=40)
        self.entry.pack(side=tk.LEFT, padx=5)

        self.send_button = tk.Button(controls_frame, text="Send Text", command=self.send_text)
        self.send_button.pack(side=tk.LEFT, padx=5)

        self.mic_running = False
        self.mic_button = tk.Button(controls_frame, text="Start Mic Stream", 
                                   command=self.toggle_mic, bg="green", fg="white")
        self.mic_button.pack(side=tk.RIGHT, padx=5)

        # Networking State
        self.clients = []
        self.udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 1024 * 1024)

    def log(self, message):
        self.text_area.configure(state='normal')
        self.text_area.insert(tk.END, message + "\n")
        self.text_area.yview(tk.END)
        self.text_area.configure(state='disabled')

    # --- Text Logic (TCP) ---
    def send_text(self):
        message = self.entry.get()
        if message:
            # Add newline and pad with NULLs
            full_msg = message
            msg_bytes = full_msg.encode('ascii', errors='replace')[:MSG_SIZE]
            msg_bytes = msg_bytes.ljust(MSG_SIZE, b'\x00')

            for client in self.clients:
                try:
                    client.sendall(msg_bytes)
                except:
                    self.log("[ERROR] Failed to send to client")

            self.log(f"[SENT] {message}")
            self.entry.delete(0, tk.END)

    # --- Audio Logic (UDP) ---
    def toggle_mic(self):
        if not self.mic_running:
            self.mic_running = True
            self.mic_button.config(text="Stop Mic Stream", bg="red")
            threading.Thread(target=self.audio_stream_thread, daemon=True).start()
            self.log("[AUDIO] Mic Stream Started")
        else:
            self.mic_running = False
            self.mic_button.config(text="Start Mic Stream", bg="green")
            self.log("[AUDIO] Mic Stream Stopped")

    def audio_stream_thread(self):
        p = pyaudio.PyAudio()
        try:
            stream = p.open(format=FORMAT, channels=CHANNELS, rate=RATE,
                            input=True, frames_per_buffer=CHUNK)
            packet_id = 0
            
            while self.mic_running:
                data = stream.read(CHUNK, exception_on_overflow=False)
                # Pack Sequence ID (4 bytes) + Audio Data
                header = struct.pack(">I", packet_id)
                packet = header + data
                self.udp_sock.sendto(packet, (AUDIO_IP, AUDIO_PORT))
                packet_id += 1
                
        except Exception as e:
            self.log(f"[AUDIO ERROR] {e}")
        finally:
            stream.stop_stream()
            stream.close()
            p.terminate()

    # --- TCP Server Logic ---
    def handle_client(self, client_socket, client_address):
        try:
            while True:
                data = recvn(client_socket, MSG_SIZE)
                if not data: break
                
                # Strip null bytes and display
                raw_msg = data.decode('ascii', errors='replace')
  
                msg = raw_msg.split('\x00')[0]
                
                msg = msg.strip()
                self.log(f"{client_address}: {msg}")
        except Exception as e:
            self.log(f"[CONN ERROR] {client_address}: {e}")
        finally:
            client_socket.close()
            if client_socket in self.clients:
                self.clients.remove(client_socket)
            self.log(f"[DISCONNECTED] {client_address}")

    def tcp_server_thread(self):
        lsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        lsock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        lsock.bind((TEXT_IP, TEXT_PORT))
        lsock.listen(5)
        self.log(f"[SYSTEM] Text Server: {TEXT_IP}:{TEXT_PORT}")

        while True:
            client_sock, addr = lsock.accept()
            self.clients.append(client_sock)
            self.log(f"[NEW CONNECTION] {addr}")
            threading.Thread(target=self.handle_client, args=(client_sock, addr), daemon=True).start()

    def run(self):
        # Start the TCP server in the background
        threading.Thread(target=self.tcp_server_thread, daemon=True).start()
        self.root.mainloop()

if __name__ == "__main__":
    app = CombinedServerUI()
    app.run()
