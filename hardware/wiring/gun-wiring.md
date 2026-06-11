# Gun Wiring List (ESP32) - Recommended Pinout (June 2026)

**References**: 1S LiPo + TP4056, combined IR receivers on one GPIO where possible.

**Wire Gauges**:
- 22 AWG for power/ground/high-current (LiPo, PAM8403 amp, LEDs).
- 26-28 AWG or ribbon cable for signals (GPIO, IR receivers).

**Full Wiring Table**:

| Component                  | ESP32 Pin | Notes / Connection                                      | Wire Gauge          |
|----------------------------|-----------|---------------------------------------------------------|---------------------|
| LiPo + (via TP4056)        | VIN / 5V | Power input; use TP4056 for charging                   | 22 AWG             |
| GND (all components)       | GND      | Common ground for everything                            | 22 AWG             |
| IR LEDs (4x via 2N2222)    | GPIO 26  | Transistor base; collector to +5V, LEDs to collector   | 22 AWG (power side), 26 AWG (signal) |
| TSOP4838 IR Receiver(s)    | GPIO 27  | Combined multiple receivers in parallel                 | 26 AWG             |
| PAM8403 Amp (audio out)    | GPIO 25  | Digital audio signal; +5V/GND from regulated supply     | 22 AWG (power)     |
| Hit Indicator LEDs         | GPIO 32  | PWM for brightness control                              | 26 AWG             |
| Optional: Status LED / Buzzer | GPIO 33 | Spare for future hit feedback                           | 26 AWG             |

**Power Notes**: 1S LiPo + TP4056. Add buck converter if expanding for stable 5V to amp/servos. Test voltages before connecting to ESP32.

**MacBook Tip**: Use Arduino IDE or VS Code + PlatformIO for uploading. Copy this table into your local gun-wiring.md.