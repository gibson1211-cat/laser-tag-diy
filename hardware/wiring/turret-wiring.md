# Turret Wiring List (ESP32) - Recommended Pinout (June 2026)

**References**: PIR on H (Repeat) mode, multiple IR receivers combined on one GPIO, buck converter for stable 5V to servos, 1S LiPo + TP4056.

**Wire Gauges**:
- 22 AWG: Power/ground/high-current (LiPo, servos, PAM8403).
- 26-28 AWG or ribbon cable: Signals (GPIO, IR, feedback).

**Full Wiring Table**:

| Component                       | ESP32 Pin | Notes / Connection                                              | Wire Gauge          |
|---------------------------------|-----------|-----------------------------------------------------------------|---------------------|
| LiPo + (via TP4056 + Buck)      | VIN / 5V | Buck converter output for stable 5V to servos & amp             | 22 AWG             |
| GND (all)                       | GND      | Common ground                                                   | 22 AWG             |
| Parallax Feedback 360° Pan Servo| GPIO 13  | Signal + feedback line (if using feedback)                      | 22 AWG             |
| 180° Tilt Servo                 | GPIO 12  | PWM control (note: GPIO12 strapping pin - test boot behavior) | 22 AWG             |
| HC-SR501 PIR (H/Repeat mode)    | GPIO 14  | Motion trigger, set to Repeat/H mode                            | 26 AWG             |
| Combined IR Receivers (TSOP4838)| GPIO 27  | Multiple receivers wired in parallel                            | 26 AWG             |
| PAM8403 Speaker Amp             | GPIO 25  | Audio signal; +5V/GND from buck converter                       | 22 AWG (power)     |
| Optional: Status LED            | GPIO 32  | Hit confirmation or ready indicator                             | 26 AWG             |

**Power Notes**: Use buck converter from LiPo/TP4056 for stable 5V. Avoid powering servos directly from ESP32 5V pin.

**MacBook Tip**: After wiring, test with simple servo sweep sketch. Update your local turret-wiring.md with any changes.