# # 🔥 ESP32 Smart Filament Dryer Core

Cheap ESP32-based filament dryer with real-time web dashboard, dual-core tasking, and safety-focused control logic.

---

## 🚀 Features

- Real-time temperature & humidity monitoring
- Auto profiles (PLA, PETG, ABS, Nylon, etc.)
- Manual override control
- Safety systems:
  - Sensor timeout shutdown
  - Thermal cutoff (70°C)
- Live web dashboard with telemetry + graphs

---

## 🧠 System Architecture

### Dual-Core RTOS Design

- Core 0 → Control loop (sensor + relay + safety)
- Core 1 → Web server (UI + API)

Why this matters:
- Control loop stays deterministic
- Web requests cannot block heating logic
- System remains responsive under load

---

### Thin Client / Server Model

- ESP32 acts as backend server
- Browser acts as thick client
- REST endpoints:
  - `/data` → telemetry (JSON)
  - `/cmd` → control commands

---

## ⚙️ Control Strategy

### State Machine

System operates in:

- IDLE  
- AUTO  
- MANUAL ON  
- MANUAL OFF  

This avoids ambiguous states and keeps behavior predictable.

---

### Hysteresis Control (Relay Protection)

Instead of switching exactly at target temperature:

- Heater ON → below (target - hysteresis)
- Heater OFF → at/above target

Why:
- Reduces rapid relay toggling
- Extends relay life
- Stabilizes temperature band

---

### Safety Logic (Fail-Safe First)

- Sensor timeout → system shuts down
- Over-temperature → immediate cutoff
- Manual mode timeout → prevents runaway heating

System always defaults to **safe OFF state** on failure.

---

## 🧠 Real-Time Design Decisions

- FreeRTOS tasks pinned to separate cores
- Watchdog timer prevents system lockups
- Non-blocking timing using `millis()` (no delay-based control)
- Control loop runs independently of web stack

---

## 🧮 Memory Strategy

- No dynamic allocation in control loop
- Fixed-size buffers for JSON + state
- HTML stored in flash (PROGMEM)

Reason:
- Prevent heap fragmentation
- Ensure long-term stability

---

## ⚠️ What’s NOT implemented (yet)

Being explicit here:

- No PID control (only hysteresis)
- No relay minimum ON/OFF lockout timing
- No persistent logging
- No OTA updates

---

## ⚙️ Hardware

- ESP32-S3
- DHT11 *(low accuracy, slow response)*
- Relay module (active LOW)
- Heating bulb

---

## 📸 Demo

![UI](images/ui.png)  
![Hardware](images/hardware.jpg)

---

## 🛠️ Future Improvements

- Add relay minimum switching interval (anti-chatter lockout)
- PID-based temperature control
- Better sensor (SHT31 / BME280)
- Data logging
- OTA updates

---

## 💬 Feedback

Looking for feedback on:

- control strategy (PID vs hysteresis)
- relay protection strategies
- system safety improvements
- architecture decisions
