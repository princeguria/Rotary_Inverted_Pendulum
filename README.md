# Rotary Inverted Pendulum

This repository contains the source code and documentation for a **Rotary Inverted Pendulum**.
The system balances a free-swinging pendulum using a **NEMA 17 stepper motor** driven by a
**DRV8825** and a **360 PPR optical rotary encoder**, all controlled by a **PID loop on an Arduino Uno**.

The project features:
- A **PID controller** that turns the pendulum angle error into a stepper speed
- **Direct port reads + hardware interrupts** for the quadrature encoder
- A **non-blocking step-pulse generator**
- A **safety cut-off** that puts the driver to sleep when the pendulum falls


#  Features

- **PID Stabilization** — Kp / Ki / Kd on the angle error, output used directly as step rate (steps/s).
- **Interrupt-Driven Encoder** — x4 quadrature decoding (1440 counts/rev, 0.25° per count) using direct `PIND` port reads on pins 2 and 3.
- **Non-Blocking Stepper Control** — STEP pulses are generated from `micros()` timing, so the PID loop is never blocked.
- **Safety Cutoff** — Driver is put to SLEEP and PID memory is reset if the pendulum leaves the 120°–205° window, so the motor and driver don't overheat.
- **Serial Debug** — Live angle and error at 115200 baud.

> Swing-up is **not** implemented: the pendulum is lifted by hand into the balance window.

---

# Hardware Requirements

| # | Component | Part | Description | Key specs |
|---|-----------|------|-------------|-----------|
| 1 | Stepper Motor | US-17HS4401S | NEMA 17, bipolar stepper | Holding torque ~44 N·cm, 1.8° step angle |
| 2 | Stepper Driver | DRV8825 | Stepper motor driver | Microstepping up to 1/32, 2.2 A max per coil |
| 3 | Rotary Encoder | E38S6G5-360B-G24N | Incremental optical encoder | 360 PPR, A/B channels, 38 mm diameter |
| 4 | Microcontroller | Arduino Uno | Microcontroller board | 16 MHz, 5 V logic, 14 digital I/O pins |
| 5 | Pendulum Rod | Custom | Physical rod | Length 15 cm, mass 21 g, breadth 2.4 cm |
| 6 | Pendulum Bob | Custom | Point mass | Mass 11 g, 13.9 cm from pivot |
| 7 | Power Supply | 12 V adapter / battery | DC supply | 12 V, 1.5 A |


#  Wiring & Connections (Arduino Uno)

## **1. Stepper Motor Driver (DRV8825)**

| Driver Pin | Arduino Pin | Description |
|-----------|-------------|-------------|
| STEP      | Pin 5       | Step pulse signal |
| DIR       | Pin 6       | Direction signal |
| SLP (SLEEP) | Pin 8     | Sleep control (active high = awake) |
| RST (RESET) | Pin 9     | Reset control (must be high to run) |
| VMOT      | Ext. 12 V + | Motor power (with 100 µF capacitor to GND) |
| GND (motor) | Ext. 12 V − | Motor power ground |
| GND (logic) | Arduino GND | Logic ground (**common ground required**) |
| 1A/1B, 2A/2B | —        | Stepper motor coils (A2/A1/B1/B2 on the board) |

The DRV8825 has **no logic-supply pin** (unlike the A4988's VDD), so only GND is shared with the Arduino.
Leave the **EN** pin unconnected (it is pulled low = enabled). Leave **M0/M1/M2** unconnected for full-step
or tie them for microstepping (see the DRV8825 table); firmware uses the step rate as-is, so a different microstep
setting changes how fast the arm moves for the same PID output.

## **2. Rotary Encoder (E38S6G5-360B-G24N)**

> Pins 2 and 3 are **PD2 / PD3 (INT0 / INT1)** on the ATmega328P, which is why the firmware can read them with `PIND`.

| Encoder Wire | Arduino Pin | Notes |
|--------------|-------------|-------|
| Phase A (Green) | Pin 2 | Interrupt pin — 4.7 kΩ pull-up to 5 V |
| Phase B (White) | Pin 3 | Interrupt pin — 4.7 kΩ pull-up to 5 V |
| VCC (Red)       | 5 V   | Encoder power |
| GND (Black)     | GND   | Encoder ground |

Wire colours are typical for this encoder family. Check them against your datasheet, and make sure
the encoder is powered from a voltage it supports and that its outputs never exceed 5 V on the Uno pins.

---



#  Installation & Usage

### **1. Upload the Firmware**
1. Open `firmware/rotary_pendulum_pid/rotary_pendulum_pid.ino` in the Arduino IDE.
2. Select **Board: Arduino Uno** and the correct port.
3. Click **Upload**. No external libraries are needed.

### **2. Set the Driver Current**
Adjust the DRV8825 Vref before running the motor (`I = 2 × Vref` on Pololu-style boards) according to your motor's rated current.

### **3. Run**
1. **Power on with the pendulum hanging straight down.** Encoder counts start at 0 on boot, so down = 0° and upright = 180°.
2. Open the Serial Monitor at **115200 baud**.
3. Slowly raise the pendulum to upright. The driver wakes up when the angle enters **120°–205°** and PID takes over.
4. If it falls outside that window the driver sleeps and the monitor prints `FALLEN! Ang: …`.

> The angle is folded with `abs()`, so always raise the pendulum in the same direction.

Serial output while balancing:
```
Ang: 179.75 | Err: -0.25
```

---

# Controller

```
error  = angle - TARGET_ANGLE        (degrees, TARGET_ANGLE = 180)
pid    = Kp*error + Ki*∫error dt + Kd*d(error)/dt
speed  = pid                         (steps per second; sign → DIR pin)
```

| Parameter | Default |
|-----------|---------|
| `Kp` | 35.0 |
| `Ki` | 0.015 |
| `Kd` | 0.1 |
| `TARGET_ANGLE` | 180.0 |
| `SPEED_LIMIT` | 40000 steps/s (clamp) |
| Safety window | 120° – 205° |


#  Known Limitations

- **No swing-up** — the pendulum must be lifted by hand.
- **Step rate is limited by loop time** — one step is sent per `loop()` pass, so `SPEED_LIMIT` is a clamp, not a guarantee.
- **Angle uses `abs()`** — it does not distinguish which side of the bottom the pendulum is on.
- **Encoder zero is set at power-up** — always boot with the pendulum hanging down.

---

#  Roadmap

- Energy-based swing-up
- State-feedback (LQR) balancing
- Fixed-rate control loop with acceleration limiting

---

# 📜 License

MIT — see [LICENSE](LICENSE).
