# HKL-EA8 Demo Code — Examples Glossary

Quick reference to every Arduino sketch and configuration file shipped in
[Hankerila-EA8-demo code/](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/).
Each entry lists which on-board peripheral it exercises, the libraries used, the I²C addresses / GPIOs touched, and what the sketch actually does.

The board uses a fixed I²C bus on **GPIO4 (SDA) / GPIO5 (SCL)** and a fixed UART for **RS-485 on GPIO14 (RX) / GPIO13 (TX)**. Most examples reuse those pins.

---

## Quick map (peripheral → example)

| Peripheral / feature | Example folder |
|---|---|
| Relay outputs (PCF8574 @ 0x24) | [1_PCF8574_output](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/1_PCF8574_output/) |
| Digital inputs (PCF8574 @ 0x26) | [2_PCF8574_Input](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/2_PCF8574_Input/), [2_Wire_PCF8574_Input](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/2_Wire_PCF8574_Input/) |
| Inputs → drive relays | [2_Wire_PCF8574_Input_trigger_output](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/2_Wire_PCF8574_Input_trigger_output/) |
| 0–10 V DAC output (MCP4725 @ 0x60) | [3_DAC0_10V](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/3_DAC0_10V/), [3_DAC0_10V_0-4095](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/3_DAC0_10V_0-4095/) |
| 0–5 V / 0–20 mA ADC (ADS1115 @ 0x48) | [4_ANALOG_0_5V](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/4_ANALOG_0_5V/) |
| Current transformer (SCT-013 differential) | [05_SCT013_30A_1V_SENSOR](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/05_SCT013_30A_1V_SENSOR/) |
| RS-485 UART | [06_RS485_RE_SEND](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/06_RS485_RE_SEND/), [15_MODBUS_POWER_METER](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/15_MODBUS_POWER_METER/) |
| Ethernet (LAN8720) | [07_Ethernet_communication](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/07_Ethernet_communication/), [16.Ethernet_WEB_control](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/16.Ethernet_WEB_control/) |
| Buzzer (GPIO12) | [08_BEEP](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/08_BEEP/) |
| IR transmitter (GPIO32) | [09_IR_SendDemo](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/09_IR_SendDemo/) |
| IR receiver (GPIO33) | [12.IR_receive_code](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/12.IR_receive_code/) |
| Temperature/humidity (SHT31 @ 0x44) | [10_SHT31_SENSOR_DETECT](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/10_SHT31_SENSOR_DETECT/) |
| HMI UART on GPIO15/16 | [11_HMI](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/11_HMI/) |
| WiFi web control of relays | [13.Web_control](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/13.Web_control/) |
| 1-Wire / DS18B20 (GPIO15, GPIO16) | [14.DS18B20_4.7K](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/14.DS18B20_4.7K/) |
| WiFi connect / RSSI | [WIFI](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/WIFI/) |
| ESPHome / Home Assistant integration | [0.esphome](Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo%20code/0.esphome/) |

---

## Detailed entries

### `0.esphome/Hankerila-EA8.yaml`
**Purpose:** Full ESPHome configuration that exposes the board to Home Assistant.
**What it covers:** Ethernet (LAN8720 on GPIO23/18/17, static IP), I²C bus on GPIO4/5, MCP4725 DAC at `0x60` mapped as a "monochromatic light" output, and (in the rest of the file) the PCF8574 expanders for relay/input mapping. Use this as the no-code path to wire the board into Home Assistant.

### `1_PCF8574_output/`
**Purpose:** Drive the 8 on-board relays (Y1..Y8 / K17..K24) via the output PCF8574 expander at I²C address `0x24`.
**Library:** Renzo Mischianti's `PCF8574`.
**What it does:** In `setup()` turns each P0..P7 HIGH one-by-one (300 ms apart). In `loop()` toggles only `P0` between HIGH and LOW — i.e. blinks relay 1 forever (the rest of the per-channel toggles are commented out, ready to uncomment).

### `2_PCF8574_Input/`
**Purpose:** Read the 8 isolated digital inputs (X1..X8) via the input PCF8574 expander at `0x26`.
**Library:** `PCF8574` (Mischianti).
**What it does:** Configures P0..P7 as INPUT and prints `KEYn PRESSED` to Serial whenever an input goes LOW (active-low through the optocouplers).

### `2_Wire_PCF8574_Input/`
Two sketches packaged together:
- **`2_Wire_PCF8574_Input.ino`** — same job as `2_PCF8574_Input` but reads the byte directly with the raw `Wire` API (`Wire.requestFrom(0x26, 1)`) and decodes specific bit patterns (`0xFE..0x7F`) to detect each key. Demonstrates the bare-metal I²C path without using the PCF8574 wrapper.
- **`WEB_Temperature_Monitor/`** — appears bundled here as a misplaced copy of the DS18B20 web monitor. See entry under §14.

### `2_Wire_PCF8574_Input_trigger_output/`
**Purpose:** Wire the 8 inputs (`0x26`) directly to the 8 relays (`0x24`) so each input drives the matching relay.
**What it does:** Reads the input expander byte, then for each bit `n` writes the matching relay LOW (energised) when input `n` is LOW, HIGH otherwise. Effectively a software-emulated 8-channel pass-through.

### `3_DAC0_10V/`
**Purpose:** 0–10 V analog output via MCP4725 DAC (I²C `0x60`) followed by the on-board op-amp gain-of-2 stage.
**What it does:** Sweeps the DAC from 0 mV → 5000 mV in 100 mV steps and back, every 200 ms. The comment notes that the circuit's ×2 amplifier means the requested 0–5 V input becomes 0–10 V at the terminal — so a `voltage_mV` of 100 produces 0.2 V at the output.

### `3_DAC0_10V_0-4095/`
**Purpose:** Same hardware, but addressed using raw 12-bit codes (0..4095) instead of millivolts.
**What it does:** Sweeps `val` from 0 to 4095 in steps of 41 (~0.1 V per step). Useful when you want full-scale resolution without the millivolt mapping.

### `4_ANALOG_0_5V/`
**Purpose:** Read the two single-ended analog inputs through the ADS1115 (I²C `0x48`).
**Library:** `ADS1X15`.
**What it does:** Measures channel A2 (0–5 V voltage input) and A3 (0–20 mA current input — converted to mA via a 200 Ω shunt). Prints raw counts plus computed Volts/mA at 1 Hz.

### `05_SCT013_30A_1V_SENSOR/`
**Purpose:** Read a clamp-on AC current transformer (SCT-013-030, 30 A → 1 V) on the differential input AIN0/AIN1 of the ADS1115.
**What it does:** `ADS.readADC_Differential_0_1()` returns the raw differential count, which is converted to volts and then to amps using the 30 A / 1 V calibration constant. The single-ended A2/A3 reads are commented out but left in for reference.

### `06_RS485_RE_SEND/`
**Purpose:** Bare-bones RS-485 echo / send demo on `Serial1` (GPIO14 RX, GPIO13 TX) at 115200 baud.
**What it does:** Prints a welcome banner on the bus at boot, then echoes back any bytes received on RS-485. Useful sanity check that the SP3485 transceiver and DE/RE direction control are working.

### `07_Ethernet_communication/`
**Purpose:** Bring up the LAN8720 Ethernet PHY and run a UDP echo server.
**Pins:** MDC=GPIO23, MDIO=GPIO18, REF_CLK=GPIO17 (OUT), no power-enable pin.
**What it does:** Static IP `192.168.50.200`, listens on UDP port 4196, prints any received packet with sender IP/port and replies `Received: <payload>`.

### `08_BEEP/`
**Purpose:** Hello-world for the on-board buzzer on GPIO12.
**What it does:** 1 Hz square-wave on GPIO12 — beep on, beep off, every second.

### `09_IR_SendDemo/`
**Purpose:** Drive the IR-emitter LED on GPIO32.
**Library:** `IRremote`.
**What it does:** When `GPIO0` (the BOOT button) is pressed, sends two NEC raw codes (`0xF807FF00`, `0xF906FF00`) at 38 kHz carrier, 1 second apart.

### `10_SHT31_SENSOR_DETECT/`
**Purpose:** Read an SHT31 / SHT3x temperature & humidity sensor over the on-board I²C extension port.
**Library:** `Adafruit_SHT31` (sensor address `0x44`).
**What it does:** Prints temperature in °C and °F, plus relative humidity, every 1–2 seconds.

### `11_HMI/`
**Purpose:** Receive line-based commands from a serial HMI display on `Serial1` (GPIO15 RX, GPIO16 TX, 9600 baud) and toggle relays accordingly.
**What it does:** Strings `relay01`..`relay08` arriving over the HMI link toggle the matching PCF8574 relay output (`0x24`). All relays are initialised LOW. Note: V3 boards can also repurpose these pins as a 1-Wire bus via the H2 jumper (see V1 vs V3 schematic comparison doc).

### `12.IR_receive_code/`
**Purpose:** Decode IR remote signals from the on-board IR receiver (GPIO33).
**Library:** `IRremote`.
**What it does:** Prints the address, command and raw 32-bit code of every IR frame received. Use this to discover codes you later play back with `09_IR_SendDemo`.

### `13.Web_control/`
**Purpose:** Connect to WiFi and serve a tiny HTML control panel that toggles the 8 relays.
**WiFi:** Hard-coded SSID `HANKER` / password `a12345678` — change before flashing.
**What it does:** Starts an `WebServer` on port 80. The root page renders a 2-row button table (ON1..ON8 / OFF1..OFF8) that fires `GET /SW?LED=onN` or `offN`; the handler maps those to `pcf8574_re.digitalWrite(...)` on the relay expander.

### `14.DS18B20_4.7K/`
DS18B20 temperature-sensor examples that use the HMI port (now jumper-selectable to 1-Wire on V3 boards). All require a 4.7 kΩ–10 kΩ pull-up between data and 5 V.

- **`14.DS18B20_4.7K.ino`** — Two single sensors, one on GPIO15 (`ds1`) and one on GPIO16 (`ds2`); prints both temperatures continuously. Uses the lightweight `DS18B20` library.
- **`DS18b20_Address/DS18b20_Address.ino`** — **Address scanner**. Walks the 1-Wire bus on GPIO15 with `OneWire::search()` and prints every detected ROM code in hex, flagging anything whose family code isn't `0x28`. Use this once to harvest the 8-byte addresses you'll paste into the multi-sensor sketches.
- **`Multiple_DS18B20s/Multiple_DS18B20s.ino`** — Reads 5 specific sensors by hard-coded ROM address from a single GPIO15 bus, sets 12-bit resolution, and prints all 5 values every 2 s. (Bundled also as `Multiple_DS18B20s.zip`.)
- **`10pcs_ds18b20/10pcs_ds18b20.ino`** — Same as above but for 10 sensors. Includes a `checkSensorsConnection()` helper that flags missing devices at startup. Comments are in Chinese.
- **`WEB_Temperature_Monitor/WEB_Temperature_Monitor.ino`** — Connects to WiFi (`HANKER` / `a12345678`), serves a web page that displays the live temperatures from 5 DS18B20s and tracks per-sensor change flags so the page can highlight movement.
- **`WEB_10pcs_ds18b20/WEB_10pcs_ds18b20.ino`** — Same web-monitor pattern scaled to 10 sensors with last-reported-value tracking; resolution dropped to 10-bit for faster polling.

### `15_MODBUS_POWER_METER/`
**Purpose:** Talk Modbus RTU over the RS-485 port to a power meter at slave address `203` (0xCB).
**Library:** `ModbusMaster` plus `SoftwareSerial` (note: uses `SoftwareSerial(14, 13)` even though hardware UART works too — this is what the supplier shipped).
**What it does:** Reads two 16-bit holding registers starting at `0x40`. Heavy comments dissect the actual on-wire bytes (`CB 03 00 40 00 02 D4 75` request → `CB 03 04 30 39 26 94 54 FD` response) which makes this a useful Modbus tutorial as well.

### `16.Ethernet_WEB_control/`
**Purpose:** The `13.Web_control` HTML relay panel, but served over **Ethernet** instead of WiFi.
**Important:** Header comment warns to stick to **arduino-esp32 v2.0.17** for the board package — newer cores break compilation of the legacy `ETH.begin(...)` signature.
**What it does:** Static IP `192.168.50.200`, web server on port 80, same `/SW?LED=onN/offN` API toggling the 8 relays via PCF8574 at `0x24`.

### `WIFI/`
**Purpose:** WiFi connect smoke test.
**What it does:** Joins SSID `HANKER` / password `a12345678` (5-second timeout), then in `loop()` prints the current RSSI every 2 s and labels the strength as excellent / good / fair / poor.

---

## Conventions used across all examples

- **I²C addresses (fixed by hardware):**
  - `0x24` — PCF8574 relay-output expander
  - `0x26` — PCF8574 digital-input expander
  - `0x44` — SHT3x temperature/humidity (extension port)
  - `0x48` — ADS1115 ADC (analog inputs + CT)
  - `0x60` — MCP4725 DAC (0–10 V output)
- **Default I²C pins:** `Wire.begin(4, 5)` (SDA, SCL).
- **Default RS-485 pins:** `Serial1.begin(baud, SERIAL_8N1, 14, 13)` (RX, TX).
- **Default Ethernet pins:** `MDC=23, MDIO=18, CLK=GPIO17_OUT, ETH_PHY_LAN8720, addr=0`.
- **Buzzer:** GPIO12. **IR TX:** GPIO32. **IR RX:** GPIO33.
- **HMI / 1-Wire port:** GPIO15 (RX / data1), GPIO16 (TX / data2). On V3 boards the H2 jumper picks between the two roles.
- **WiFi credentials in demos:** SSID `HANKER`, password `a12345678` — change before deploying.

## Notable findings / gotchas

Things worth knowing before you flash any of these sketches:

- **Hardcoded WiFi credentials.** `13.Web_control`, `WIFI`, and the `WEB_*` DS18B20 sketches all ship with `ssid = "HANKER"` / `password = "a12345678"`. Change these before flashing — they will not connect to your network as-is.
- **arduino-esp32 v2.0.17 only for `16.Ethernet_WEB_control`.** The header comment explicitly warns that newer ESP32 board-package versions break compilation of the legacy `ETH.begin(addr, power, mdc, mdio, type, clk_mode)` signature used here. If you upgrade the core, the Ethernet demos must be ported to the new `ETH.begin(...)` API.
- **Misplaced sketch.** `2_Wire_PCF8574_Input/WEB_Temperature_Monitor/` is actually a DS18B20 web demo, not a digital-input demo. It looks like a copy-paste leftover — the equivalent file in `14.DS18B20_4.7K/WEB_Temperature_Monitor/` is the canonical version.
- **`15_MODBUS_POWER_METER` uses SoftwareSerial on RS-485.** The sketch declares `SoftwareSerial BTserial(14, 13)` even though `Serial1` (the hardware UART already wired to the SP3485 transceiver on those exact pins) would be more reliable, especially at higher baud rates. If you adapt this for production, swap to `Serial1`.
- **DS18B20 multi-sensor sketches need real ROM addresses.** `Multiple_DS18B20s.ino`, `10pcs_ds18b20.ino`, and the two `WEB_*` variants ship with example ROM codes that **will not match your devices** — run `DS18b20_Address.ino` first to harvest your actual addresses, then paste them into `sensorsAddress[]`.
- **DS18B20 pull-up is mandatory.** A 4.7 kΩ–10 kΩ resistor between the DS18B20 DATA line and 5 V is required (folder name "14.DS18B20_4.7K" is a hint). Without it the bus is unreliable or completely silent.
- **HMI port and 1-Wire share GPIO15/16.** The `11_HMI` and `14.DS18B20_*` sketches use the same pins — you cannot run both at once. On V3 boards the H2 jumper selects which role the connector plays.
- **`09_IR_SendDemo` uses GPIO0 as a trigger input.** GPIO0 is also the BOOT-mode strap; pressing it during boot puts the ESP32 into download mode. The demo only reads GPIO0 after `setup()` completes, so it works in practice, but be aware of the dual role.
- **`13.Web_control` (and `16.Ethernet_WEB_control`) are unauthenticated.** Anyone who can reach the IP can flip relays. Don't expose them outside a trusted LAN without adding auth.
- **DAC range vs schematic note.** `3_DAC0_10V` maps `0–5000 mV` to DAC code `0–4095`; the on-board op-amp doubles this to 0–10 V at the terminal. Note the V3 schematic comparison flagged a documentation contradiction on AIN2's range (0–5 V vs 0–10 V) — verify against your hardware before trusting either label.
- **ADS1115 gain is set to `0` (±6.144 V) in `4_ANALOG_0_5V` and `05_SCT013_*`.** That's the safest range but reduces resolution. If your signal is bounded ≤ 4.096 V you can switch to gain `1` for ~2× better LSB. The SCT-013 differential signal in particular benefits from a higher gain.
- **ESPHome YAML credentials are placeholders.** `0.esphome/Hankerila-EA8.yaml` ships with example API/OTA keys. Regenerate these (and the static IP) before adding the device to your Home Assistant instance.

## Source files

- Folder: [doc/Hankerila-EA8-board/Hankerila-EA8-demo code/](.)
- Companion docs:
  - [HKL-EA8 Product Introduction Document.pdf](./HKL-EA8%20%20Product%20Introduction%20Document.pdf)
  - [HKL-EA8_V1_vs_V3_Schematic_Differences.md](./HKL-EA8_V1_vs_V3_Schematic_Differences.md)
