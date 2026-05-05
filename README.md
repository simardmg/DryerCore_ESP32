# 🔥 ESP32 Smart Dryer Core

ESP32-based filament dryer with real-time web dashboard, dual-core tasking, and safety-focused control logic.

---

## 🚀 Features

- Real-time temperature & humidity monitoring
- Auto profiles (PLA, PETG, ABS, Nylon)
- Manual ON/OFF override
- Safety systems:
  - Sensor timeout shutdown
  - Thermal cutoff (70°C)
- Live web dashboard with real-time graphs

---

## 🧠 System Architecture

### Dual-Core RTOS Design

- Core 0 → Control loop (sensor + relay + safety)
- Core 1 → Web server (UI + API)

This separation ensures:
- Control loop is not blocked by network requests
- Stable timing for heater control
- Responsive UI

---

### Thin Client / Server Model

- ESP32 acts as thin backend server
- Browser UI is a thick client
- REST endpoints:
  - `/data` → telemetry (JSON)
  - `/cmd` → control actions

---

## ⚙️ Control Logic

### State Machine

System operates in:

- IDLE  
- AUTO  
- MANUAL ON  
- MANUAL OFF  

---

### Hysteresis-Based Control

- Heater ON → below (target - hysteresis)
- Heater OFF → at/above target

Why:
- Reduces relay chatter
- Improves stability
- Extends relay life

---

## 🛡️ Safety Design

- Sensor timeout → system shuts down
- Over-temperature → immediate cutoff
- Manual mode timeout → prevents runaway heating

System always defaults to **safe OFF state**.

---

## 🧠 Real-Time Design Choices

- FreeRTOS tasks pinned to separate cores
- Watchdog timer for reliability
- Non-blocking control using `millis()`
- Control loop isolated from web server

---

## 🧮 Memory Strategy

- No dynamic allocation in control loop
- Fixed-size buffers
- HTML UI stored in flash (PROGMEM)

---

## 📸 Demo

### UI

![UI Main](images/UI.png)
![UI Graph](images/UI1.png)
![UI Graph 2](images/UI2.png)
![UI Graph 3](images/UI3.png)

### Hardware

![Hardware](images/hardware.png)

---

## ⚙️ Hardware

- ESP32-S3
- DHT11 sensor *(low accuracy — upgrade recommended)*
- Relay module (active LOW)
- Heating bulb

---

## ⚠️ Limitations

- DHT11 has low accuracy and slow response
- No PID control (uses hysteresis)
- No relay minimum switching delay
- No data logging
- No OTA updates

---

## 🛠️ Future Improvements

- Add relay switching delay (anti-chatter protection)
- PID temperature control
- Better sensor (SHT31 / BME280)
- Logging system
- OTA updates

---

## 💬 Feedback

Looking for feedback on:

- control logic
- relay protection strategies
- system safety
- architecture decisions
