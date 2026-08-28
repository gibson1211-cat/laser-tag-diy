# laser-tag-diy

DIY laser tag for Tyler (8) and Emmit (6). ESP32 guns + up to four motorized turrets. IR only. Home Assistant over MQTT.

Kid-safe: infrared (not visible lasers), turret does **not** track faces (no `vision/event`), no loud night audio.

Repo wiring (`hardware/wiring/`) is the pin source of truth. This README restates those maps and how to flash.

## This week

- House Wi-Fi **ORBI75** (visible). `Arena_Net` (hidden VLAN) is later — do not flash hidden-SSID firmware this week.
- MQTT broker: **192.168.20.249:1883** (HA mosquitto). Topics: `tag/event`, `tag/match`, `ha/cmd`, `ha/state`. Not `192.168.1.30`, not `home/laser_gun/*`.
- ArduinoOTA port **3232**. Hostnames: `laser-gun-1`, `laser-turret-1` … `laser-turret-4`.
- Gun 1 live identity: Wi-Fi MAC `78:1c:3c:cb:3d:34` (last seen `192.168.20.228`). Turret 1 MAC `28:05:a5:70:48:bc`.

## Layout

```
hardware/wiring/gun-wiring.md
hardware/wiring/turret-wiring.md
software/common/tag_net.*          shared Wi-Fi / OTA / MQTT
software/common/secrets.example.h
software/guns/gun.ino              + copies of tag_net.* and secrets.example.h
software/turret/turret.ino         TURRET_ID 1..4
```

Arduino IDE compiles files in the sketch folder. Keep `tag_net.*` copies in `guns/` and `turret/` in sync with `software/common/`.

## Secrets

```bash
cp software/guns/secrets.example.h software/guns/secrets.h
cp software/turret/secrets.example.h software/turret/secrets.h
# edit WIFI_PASSWORD and MQTT_PASSWORD locally
```

`secrets.h` is gitignored. Never commit it. Never `Serial.print` passwords, SSIDs-with-PSK, or MQTT creds.

## Pinout (repo maps — do not “fix” on the bench without updating git)

### Gun (ESP32) — `hardware/wiring/gun-wiring.md`

| Component | ESP32 pin | Notes |
|-----------|-----------|--------|
| LiPo + (via TP4056) | VIN / 5V | 22 AWG |
| GND | GND | Common ground |
| IR LEDs (4× via 2N2222) | **GPIO 26** | Transistor base |
| TSOP4838 IR receiver(s) | **GPIO 27** | Combined in parallel |
| PAM8403 amp | **GPIO 25** | Quiet at night in firmware |
| Hit LEDs | **GPIO 32** | PWM |
| Status LED / buzzer | **GPIO 33** | MQTT-connected = on |
| Test trigger (sketch only) | **GPIO 0** | BOOT to GND. Not in the wiring table. |

Power: 1S LiPo + TP4056. 22 AWG power, 26–28 AWG signals.

### Turret (ESP32) — `hardware/wiring/turret-wiring.md`

| Component | ESP32 pin | Notes |
|-----------|-----------|--------|
| LiPo + TP4056 + buck | VIN / 5V | Buck feeds servos & amp. Not ESP32 5V pin. |
| GND | GND | Common ground |
| Parallax 360° pan | **GPIO 13** | Signal |
| 180° tilt | **GPIO 12** | Strapping pin — see below |
| HC-SR501 PIR | **GPIO 14** | H / Repeat mode |
| Combined TSOP4838 | **GPIO 27** | Parallel receivers |
| PAM8403 | **GPIO 25** | |
| Status LED | **GPIO 32** | |

Four units: set `#define TURRET_ID N` (1–4) in `turret.ino` (or `-DTURRET_ID=N`) so hostname is `laser-turret-N`.

### Pin-change recommendations (not applied this week)

Firmware **matches the repo tables**. If a board will not boot or a gun will not fire from a real trigger, consider these later and change git + wiring together:

1. **GPIO 12 tilt (turret)** is a strapping pin (MTDI / flash voltage). If the servo line is high at reset, the ESP32 can fail to boot. If that happens: unplug tilt during USB boot, then attach. Future move: **GPIO 16 or 17**.
2. **GPIO 14 PIR** is also a strapping pin. A HIGH PIR at reset can confuse boot. Unplug PIR if boot loops. Future move: **GPIO 15**.
3. **GPIO 0 trigger (gun)** is BOOT. Holding the trigger at reset enters download mode. Fine for bench. A real trigger button should move to **GPIO 4** (not strapping) and be added to `gun-wiring.md`.

## Family-safe behavior

- IR LEDs only (38 kHz carrier for TSOP4838). No visible laser diode in firmware.
- Turret hunts on **PIR** + match play. It never subscribes to `vision/event` and does not track faces.
- Audio is muted when `tag/match` is `mode=off` / `state=stopped`, when `ha/state` says `input_boolean.laser_tag_play=off`, or between **20:00–08:00 America/New_York** (NTP).
- Local IR fire/hit still works if Wi-Fi or MQTT is down.

## MQTT (Tag v0)

Broker `192.168.20.249:1883`. JSON UTF-8.

| Topic | Direction | Use |
|-------|-----------|-----|
| `tag/event` | device → HA | `fire` / `hit` (not retained) |
| `tag/match` | HA → device | subscribe; mute / stop hunt when off |
| `ha/cmd` | subscribe | play on/off helpers |
| `ha/state` | subscribe | `input_boolean.laser_tag_play` |

IR has no shooter ID yet, so hits publish `"gun_id":"unknown"` and `"target_id":"<this device>"`.

## Libraries

Arduino IDE / arduino-cli, board **ESP32 Dev Module** (`esp32:esp32:esp32`).

- `PubSubClient` (Nick O'Leary)
- `ESP32Servo` (turret only)
- Built-in: `WiFi`, `ArduinoOTA`

```bash
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32
arduino-cli lib install "PubSubClient" "ESP32Servo"
```

Copy `secrets.example.h` → `secrets.h` in the sketch folder **before** compile.

## Identify the USB serial (do this every time)

On ubuntu-server there are two CP2102s. **Gun 1 is USB bus 7-1 → `/dev/ttyUSB0` today.** Turret adapter is the other CP2102 (`/dev/ttyUSB1`, bus 3-1). Confirm before any write:

```bash
source /opt/src/esp-venv/bin/activate
# Map port → USB bus
python3 - <<'PY'
import os, glob
for tty in glob.glob("/sys/class/tty/ttyUSB*/device"):
    name = os.path.basename(os.path.dirname(tty))
    real = os.path.realpath(tty)
    print(name, real)
PY
# Read MAC — must be 78:1c:3c:cb:3d:34 before flashing gun 1
esptool.py --port /dev/ttyUSB0 --chip esp32 read-mac
```

**Do not USB-flash the turret** from this box unless Mike is at the bench and has asked. First turret image is USB; later updates are OTA.

## USB flash (esptool) — gun 1 only, after MAC check

Compile (example using arduino-cli; output names vary slightly by core version):

```bash
cd /opt/src/laser-tag-diy
arduino-cli compile --fqbn esp32:esp32:esp32 \
  --output-dir /tmp/laser-gun-build \
  software/guns
```

Then, **only if** `read-mac` printed `78:1c:3c:cb:3d:34` and the port is still bus **7-1** `/dev/ttyUSB0`:

```bash
source /opt/src/esp-venv/bin/activate
# Merge-bin from arduino-cli is easiest:
ls /tmp/laser-gun-build
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 \
  --before default-reset --after hard-reset \
  write-flash --flash-mode dio --flash-freq 80m --flash-size detect \
  0x0 /tmp/laser-gun-build/gun.ino.merged.bin
```

If there is no `merged.bin`, use the classic layout from the same folder:

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 write-flash -z \
  0x1000  /tmp/laser-gun-build/gun.ino.bootloader.bin \
  0x8000  /tmp/laser-gun-build/gun.ino.partitions.bin \
  0x10000 /tmp/laser-gun-build/gun.ino.bin
```

Or: `arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 software/guns`

Confirm over serial (`115200`) that it prints `host=laser-gun-1` and joins ORBI75. Then disconnect USB; later flashes are OTA.

Turret USB (Mike, on the bench — once per unit, after wiring):

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 --build-property compiler.cpp.extra_flags=-DTURRET_ID=1 \
  --output-dir /tmp/laser-turret-1-build software/turret
# Repeat TURRET_ID=2..4 for the other three boards.
# Identify MAC 28:05:a5:70:48:bc for turret 1 before write-flash.
```

## OTA after the first USB image (preferred)

Needs Wi-Fi on ORBI75. Default port **3232**. `espota.py` ships with the Arduino-ESP32 package (and with esptool installs as a cousin; Arduino’s copy is the one we want):

```bash
# Gun 1 (IP last seen 192.168.20.228 — confirm DHCP if it moved)
python3 ~/.arduino15/packages/esp32/hardware/esp32/*/tools/espota.py \
  -i 192.168.20.228 -p 3232 -r -f /tmp/laser-gun-build/gun.ino.bin

# Turret N once it has joined Wi-Fi (hostname laser-turret-N)
python3 ~/.arduino15/packages/esp32/hardware/esp32/*/tools/espota.py \
  -i laser-turret-1.local -p 3232 -r -f /tmp/laser-turret-1-build/turret.ino.bin
```

If mDNS fails, use the DHCP IP from the Orbi / HA. Optional `OTA_PASSWORD` in `secrets.h` then pass `-a`.

## Weekend turret wiring (bench wires unplugged)

Firmware will not move servos or count hits until the harness matches the table. Do this on the bench **before** USB-flashing turrets:

1. **Common GND** from ESP32, buck, servos, PIR, TSOP, PAM8403.
2. **Buck 5V** to pan, tilt, and PAM8403 VCC. Do **not** power servos from the ESP32 5V pin.
3. ESP32 VIN from TP4056 / 1S LiPo (or USB only for first flash).
4. Pan signal → GPIO 13. Tilt signal → GPIO 12 (unplug tilt while resetting if it will not boot).
5. PIR OUT → GPIO 14. Set jumper to **H (Repeat)**. 5V and GND on the PIR.
6. All TSOP4838 OUT lines tied together → GPIO 27. VS / GND on each receiver.
7. PAM8403 DIN → GPIO 25. Volume low for first test. Firmware already mutes at night.
8. Optional status LED → GPIO 32 + resistor to GND.
9. No camera, no face tracker, no USB gadget on the turret ESP except the CP2102 for the first flash.
10. Label boards turret-1 … turret-4. Flash `TURRET_ID` to match the label. Turret 1 MAC should be `28:05:a5:70:48:bc`.

After wiring: USB flash **once** (MAC check), then OTA. IR between a gun and a turret should work even with Wi-Fi down (serial `HIT DETECTED`).
