# IoT Voice Communication System
**WiFi-Based Audio Streaming and Text Messaging Platform**

---

## System Overview

The IoT Voice Communication System enables real-time voice communication between a laptop and multiple ESP32 microcontroller boards over WiFi. The laptop runs a Python server that captures microphone audio and broadcasts it to all connected ESP32 devices. Text messages and alerts are exchanged reliably over TCP, while audio streams over UDP for low-latency transmission.

**Key Capabilities:**
- Real-time audio streaming from laptop microphone to multiple ESP32 speakers
- Bidirectional text messaging between server and devices
- Alert notifications via push button and buzzer
- Support for up to 8 simultaneous device connections
- Designed for IoT applications, home automation, and remote monitoring

---

## System Architecture

```
┌──────────────────────────┐
│    Laptop (Server)       │
│  - Python Script         │
│  - PyAudio (Microphone)  │
│  - TCP/UDP Networking    │
└────────────┬─────────────┘
             │
      ┌──────┴───────────────────┐
      │                          │
  UDP Audio (5001)         TCP Messages (5002)
      │                          │
      ▼                          ▼
 ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐
 │ ESP32-1 │  │ ESP32-2 │  │ ESP32-3 │  │ ESP32-4 │
 │ ESP-IDF │  │ ESP-IDF │  │ ESP-IDF │  │ ESP-IDF │
 │ FreeRTOS│  │FreeRTOS │  │FreeRTOS │  │FreeRTOS │
 └─────────┘  └─────────┘  └─────────┘  └─────────┘
    │            │            │            │
    ├─Speaker    ├─Speaker    ├─Speaker    ├─Speaker
    ├─Button     ├─Button     ├─Button     ├─Button
    └─Buzzer     └─Buzzer     └─Buzzer     └─Buzzer
```

---

## Communication Protocols

### Audio Stream (UDP - Port 5001)
- **Source:** Laptop microphone captured at 16 kHz, 16-bit mono via PyAudio
- **Packet Format:** Header + Sequence Number + Timestamp + Audio Data
- **Bandwidth:** ~256 kbps
- **Latency:** 20-80 ms (acceptable for voice)
- **Tolerance:** UDP allows up to 5% packet loss with graceful handling

### Text Messaging (TCP - Port 5002)
- **Format:** JSON-based messages
- **Types:** User messages, alerts, status updates, acknowledgments
- **Reliability:** TCP ensures all messages arrive intact
- **Latency:** 10-30 ms typical
- **Max Message:** 64 characters per message

---

## Hardware Components

**ESP32 Microcontroller:**
- Dual-core 240 MHz processor
- 520 KB RAM, 4-8 MB Flash
- WiFi 802.11n (2.4 GHz)
- 34 GPIO pins (DAC, ADC, PWM-capable)
- Power: 5V USB or 3.3V regulated input

**Peripherals per ESP32:**
- **Speaker/Amplifier:** Connected to GPIO 25 (DAC output)
- **Push Button:** GPIO 34 with pull-down resistor for user alerts
- **Buzzer:** GPIO 27 with transistor driver for incoming alerts
- **Display (Optional):** I2C LCD for message display

**Laptop/Server Requirements:**
- Python 3.8 or newer
- PyAudio library for microphone input
- WiFi connectivity to same network as ESP32 devices
- Minimum 4 GB RAM recommended

---

## Software Stack

### Server (Laptop)
- **Language:** Python 3.8+
- **Audio Capture:** PyAudio (reads from system microphone)
- **Networking:** Python socket library (UDP broadcast + TCP server)
- **Architecture:** Multi-threaded (audio capture, UDP broadcast, TCP messaging, alert handling)
- **Format:** JSON for all network communication

### Client (ESP32)
- **Framework:** Espressif ESP-IDF (IoT Development Framework)
- **OS:** FreeRTOS (real-time operating system)
- **Language:** C with FreeRTOS API
- **Task-Based:** Each functionality runs as independent FreeRTOS task
  - WiFi connection management
  - UDP audio reception and buffering
  - Audio playback via DAC
  - TCP message handling
  - Button monitoring
  - Buzzer control

---

## Setup Overview

### Server Setup (Python)
1. Install PyAudio library
2. Configure server parameters (ports, WiFi settings)
3. Run Python script to start microphone capture and broadcasting

### ESP32 Setup (ESP-IDF)
1. Configure WiFi credentials (SSID, password)
2. Set server IP address and ports
3. Define GPIO pin assignments for audio, button, buzzer
4. Build project using ESP-IDF build tools
5. Flash to ESP32 board via USB
6. Monitor serial output to verify connection

---

## Data Flow

1. **Audio Path:**
   - Laptop PyAudio captures microphone → Python creates UDP packets
   - UDP packets broadcast to all connected ESP32 devices
   - ESP32 receives packets → stores in circular buffer
   - Playback task reads from buffer → outputs to DAC → speaker

2. **Messaging Path:**
   - User/Button input on ESP32 → creates JSON message
   - Message sent via TCP to laptop server
   - Server receives → processes and stores message
   - Server can send messages back to ESP32 via TCP

3. **Alert Path:**
   - Button press on ESP32 → sends alert via TCP
   - Server receives alert → controls buzzer on same device
   - Or server receives command → controls buzzer on specific device

---

## Key Features

- **Low-Latency Audio:** UDP protocol minimizes delay (essential for voice communication)
- **Reliable Messaging:** TCP ensures all messages arrive and in correct order
- **Multi-Device:** Single server manages up to 8 simultaneous ESP32 connections
- **Circular Buffering:** Smooth audio playback with automatic synchronization
- **Simple Networking:** Standard WiFi network, no specialized infrastructure needed
- **Scalable Architecture:** Easy to add more devices or features

---

## Application Example

- **Child protection:** Platform for helping babysitters keep track children 

---

**Technology Summary:** Python (server) + ESP-IDF/FreeRTOS (clients) + WiFi UDP/TCP networking
