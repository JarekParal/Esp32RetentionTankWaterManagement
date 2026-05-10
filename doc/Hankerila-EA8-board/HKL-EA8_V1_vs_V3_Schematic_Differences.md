# HKL-EA8 Schematic Differences: V1 vs V3.0.02

Comparison of the two ESP32 Hankerila EA8 board revisions:

| Attribute | V1 | V3.0.02 |
|---|---|---|
| Schematic title | `HKL-EA8-V1` | `HKL-EA8-V3.0.02` |
| Drawing version | V1.0 | V1.0 |
| Created | 2025-03-04 | 2025-07-09 |
| Updated | 2025-03-04 | 2025-11-08 |
| Source folder | `Hankerila-EA8-demo code 2025-07-20/` | `Hankerila-EA8-demo code 2025-11-08/` |

Both revisions share the same core architecture (ESP32-WROOM-32-N4 MCU, LAN8720A Ethernet PHY, 8× relay outputs, 8× isolated NPN/PNP digital inputs, ADS1115 ADC, MCP4725 DAC, RS-485, IR Tx/Rx, buzzer, I²C expansion). The differences below are what actually changed between the two revisions.

---

## 1. Relay-output drive path — buffer stage added (functional change)

This is the most significant electrical change.

- **V1**: The `PCF8574T` (U24, addr `0x24`) outputs `OT1..OT8` drive the relay base resistors `R01..R08` (10 kΩ) directly, which switch transistors `Y1..Y8` controlling relays `K17..K24`.
- **V3**: Two hex-buffer ICs **U74** and **U75** are inserted between the PCF8574 outputs and the relay drivers. The PCF8574 outputs (`OT1..OT8`) now feed `U74/U75`, whose buffered outputs (`OUT1..OUT8`) drive the relay transistors. Two new decoupling caps `C70`, `C71` were added on the buffer rails.

Effect: stronger drive current and isolation between the I²C expander pin and the relay-driver base, improving reliability and protecting the PCF8574 from inductive feedback faults.

## 2. HMI port → HMI / One-Wire selectable port (functional change)

- **V1**: Header `P15` is a dedicated **HMI UART** port (5 V, GND, `HMI_TX`, `HMI_RX`) on `GPIO15`/`GPIO16`.
- **V3**: A new jumper header **H2** ("HMI & ONE-WIRE") with the comment *"Jumper cap sets up HMI and ONE-Wire switching"* lets the same `GPIO15`/`GPIO16` lines be repurposed as a **1-Wire bus** (e.g. for DS18B20 temperature sensors) instead of a UART HMI link. Pin-function table in V3 documents this as `HMI_RX (Wire01) / HMI_TX (Wire02)`.

Effect: V3 boards can natively connect 1-Wire sensors without sacrificing the HMI capability — selected by jumper position.

## 3. Pin-assignment reference table added (documentation)

V3 adds a complete **"EA8 PIN DEFINE" table** on the drawing listing every functional block, its GPIO pins, controller chip and I²C address. V1 only has a free-text functional description. The table in V3 specifies:

| Function | Pin / Address | Chip |
|---|---|---|
| Infrared emission | GPIO32 | — |
| Infrared reception | GPIO33 | — |
| RS485 | 485_RX = GPIO14, 485_TX = GPIO13 | — |
| Buzzer | GPIO12 | — |
| HMI / One-Wire (jumper selectable) | HMI_RX = GPIO15, HMI_TX = GPIO16 | — |
| I²C interface | SDA = GPIO4, SCL = GPIO5 | — |
| NPN / PNP inputs (1–8) | 0x26 (`100110`) | PCF8574 |
| Relay outputs (1–8) | 0x24 (`100100`) | PCF8574 |
| Current transformer | 0x48, AIN0/AIN1 differential | ADS1115 |
| ADC 0-10 V (AIN2) | 0x48 | ADS1115 |
| ADC 0-20 mA (AIN3) | 0x48 | ADS1115 |
| DAC 0-10 V | 0x60 | MCP4725 |
| PHY LAN8720 | MDC = GPIO23, MDIO = GPIO18, CLK = GPIO17 | LAN8720 |

V1 referenced these addresses only informally (e.g. inline comments `Address:100100--0X24 RELAY OUTPUT`).

## 4. Analog input labelling — AIN2 changed from 0-5 V to 0-10 V

- **V1**: Block labels read *"0-5V Analog Signal Acquisition"* and *"0-20mA Analog Signal Acquisition"*.
- **V3**: Pin-define table re-classifies **AIN2 as 0–10 V** (`ADC_0-10V (AIN2)`) and AIN3 as 0–20 mA. Schematic adds Chinese annotations (*"A2 主要测量 0-5V 电压信号"* / *"A3 主要测量 4-20MA 电流信号"*) clarifying the analog channels and showing differential detection on `AIN0/AIN1` for the current-transformer input.

Note: the on-schematic Chinese annotation still says 0–5 V while the pin table says 0–10 V — this looks like an in-progress documentation update on the V3 drawing. Verify against V3 hardware before relying on the 0–10 V range on AIN2.

## 5. Power supply and USB download circuits hidden in V3 (drawing change only)

V1 shows the full schematic for:

- **DC input stage**: 8–28 V wide-input buck (U66, inductor `L15`) → 5 V → 3.3 V LDO (U67/U68), with TVS, fuses and bulk caps (`C56`–`C65`).
- **USB-C download port**: `USB2` Type-C connector → CH340C (`U42`) UART bridge with the classic `EN` / `IO0` auto-reset transistor pair (`Q16`, `Q17`, SS8050) and pushbuttons `SW5`, `SW6`.

V3 explicitly states: *"NOTE: The schematic diagram conceals the design of the power circuit and the USB download interface."* These blocks are no longer drawn on the V3 schematic page (likely moved to a separate sheet or treated as IP).

This is a **documentation change**, not necessarily a hardware change — but it means anyone working from V3 cannot validate the power tree from this drawing alone.

## 6. Component-value annotations removed in V3 (drawing change)

- **V1**: Resistor / capacitor / inductor values shown next to each refdes (e.g. `R52 47K`, `C42 100uF/35V`, `R57 4.7K`, `L14 10uH`).
- **V3**: Only refdes are shown; component values have been stripped from the schematic and presumably moved to the BOM.

## 7. Connector footprint labels removed (drawing change)

- **V1**: Pluggable terminals show their footprint pitch on the drawing (e.g. `5.0-3P`, `3.81-9P`, `3.81-5P`, `2.54-4P`, `PJ325-3.5`).
- **V3**: Pitch annotations removed; only the connector refdes (`P3..P9`, `P11`, `P15`, `P16`) is shown.

## 8. Additional GPIO test/labels exposed on ESP32 in V3

V3 explicitly labels the input-only ADC pins **GPIO34, GPIO35, GPIO36, GPIO39** on the ESP32 module symbol (V1 left these unlabeled / unused on the drawing). No connection change — these are documentation labels that signal the pins are available for future use.

## 9. Relay part-number annotation removed (drawing change)

- **V1**: Relays `K17..K24` show full part number `JQC-3FF/12VDC-1ZS`.
- **V3**: Same relay symbol, part number annotation removed (now in BOM).

---

## Summary — what actually changed in hardware

If you are migrating firmware/PCB work from V1 to V3, only items **1, 2, 4** matter electrically:

1. **Relay drive path now buffered** through `U74`/`U75` — same logic polarity, but trace OUT signals through the new buffer stage before the relay driver.
2. **HMI port is now jumper-selectable to 1-Wire** via header `H2` — same GPIO15/16 pins, but firmware can now optionally talk DS18B20-style devices on this connector when the jumper is in the One-Wire position.
3. **Analog channel AIN2 is documented as 0–10 V** instead of 0–5 V (verify on hardware — the V3 drawing has a contradicting on-page note).

Items 3, 5, 6, 7, 8, 9 are drawing/documentation cleanups, not circuit changes.

## Source files

- V1: `Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo code/SCH_HKL-EA8-V1_2025-03-04.pdf`
- V3: `Esp32RetentionTankWaterManagement/doc/Hankerila-EA8-board/Hankerila-EA8-demo code/SCH_HKL-EA8-V3.0.02.pdf`
