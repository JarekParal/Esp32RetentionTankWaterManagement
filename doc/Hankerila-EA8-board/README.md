# Hankerila HKL-EA8 — Board Documentation

This folder is a local mirror of the vendor-supplied documentation, schematics and Arduino demo code for the **Hankerila HKL-EA8 intelligent control board**, used as the hardware target for this project's water-management controller.

## Sources

All material here was downloaded from the official vendor repository:

- **Upstream repository:** [github.com/hankerila-MA/HKL-EA8](https://github.com/hankerila-MA/HKL-EA8/)
- **Vendor store:** [aliexpress.com/store/1103673574](https://www.aliexpress.com/store/1103673574)

| Local file/folder | Upstream commit |
|---|---|
| [Hankerila-EA8-demo code/](Hankerila-EA8-demo%20code/) (Arduino sketches, ESPHome YAML, both schematic PDFs) | [`f925c75` (latest)](https://github.com/hankerila-MA/HKL-EA8/commit/f925c7510eed2ef331a57997307f9a52c4ec74f8) |
| [SCH_HKL-EA8-V1_2025-03-04.pdf](Hankerila-EA8-demo%20code/SCH_HKL-EA8-V1_2025-03-04.pdf) (V1 schematic) | introduced in [`c7aa384`](https://github.com/hankerila-MA/HKL-EA8/commit/c7aa38471f9270c9b2381e8686e271d8b19c93ef) |
| [SCH_HKL-EA8-V3.0.02.pdf](Hankerila-EA8-demo%20code/SCH_HKL-EA8-V3.0.02.pdf) (V3.0.02 schematic) | included in [`f925c75`](https://github.com/hankerila-MA/HKL-EA8/commit/f925c7510eed2ef331a57997307f9a52c4ec74f8) |
| [HKL-EA8 Product Introduction Document.pdf](HKL-EA8%20%20Product%20Introduction%20Document.pdf) | introduced in [`c7aa384`](https://github.com/hankerila-MA/HKL-EA8/commit/c7aa38471f9270c9b2381e8686e271d8b19c93ef) |

## What's in this folder

| File | Purpose |
|---|---|
| [HKL-EA8 Product Introduction Document.pdf](HKL-EA8%20%20Product%20Introduction%20Document.pdf) | Vendor datasheet — interfaces, electrical specs, pinouts, application notes (original PDF, includes diagrams and product photos). |
| [HKL-EA8_Product_Introduction.md](HKL-EA8_Product_Introduction.md) | Markdown extraction of the vendor datasheet — same text, searchable / diff-able, no images. |
| [HKL-EA8_V1_vs_V3_Schematic_Differences.md](HKL-EA8_V1_vs_V3_Schematic_Differences.md) | **Companion doc** — diff between the V1 and V3 schematics, written for this project. |
| [HKL-EA8_Demo_Examples_Glossary.md](HKL-EA8_Demo_Examples_Glossary.md) | **Companion doc** — one-paragraph summary of every Arduino example sketch shipped by the vendor, with gotchas. |
| [Hankerila-EA8-demo code/](Hankerila-EA8-demo%20code/) | Vendor demo code: Arduino sketches per peripheral, ESPHome YAML for Home Assistant, and both schematic PDFs (V1 and V3.0.02). |

## Board at a glance

ESP32-WROOM-32-N4 based industrial I/O controller with wired Ethernet:

| Block | Details |
|---|---|
| MCU | ESP32-WROOM-32-N4 (16 MB flash on V3.0.02 silkscreen) |
| Power | 12–24 V DC wide input; over-/under-voltage and over-current protection |
| Digital inputs | 8× isolated, NPN **or** PNP via PCF8574 @ I²C `0x26` (`SDA=GPIO4`, `SCL=GPIO5`) |
| Relay outputs | 8× JQC-3FF, 10 A @ 277 VAC / 12 A @ 125 VAC, via PCF8574 @ I²C `0x24` |
| Analog inputs | ADS1115 @ I²C `0x48`: AIN0/AIN1 differential CT, AIN2 0–5 V (V1) / 0–10 V (V3 docs), AIN3 0–20 mA |
| Analog output | MCP4725 @ I²C `0x60` → LM358 ×2 → 0–10 V @ 40–60 mA |
| Ethernet | LAN8720 PHY (`MDC=GPIO23`, `MDIO=GPIO18`, `CLK=GPIO17_OUT`), 10/100 Mbps |
| WiFi | 2.4 GHz, IEEE 802.11 b/g/n |
| RS-485 | Modbus RTU capable, up to 115200 bps, ~1200 m range (`RX=GPIO14`, `TX=GPIO13`) |
| HMI / 1-Wire | UART on `GPIO15`/`GPIO16`; on **V3** boards a jumper (H2) repurposes the connector for 1-Wire (e.g. DS18B20) |
| IR | Emitter on GPIO32 (38 kHz, ≈10 m), receiver on GPIO33 |
| Buzzer | GPIO12 |
| I²C expansion | 5 V and 12 V expansion headers (SHT3x sensor port, ADI16/DO8/A6 add-on boards) |

For the full pin / address map see the **EA8 PIN DEFINE** table on the V3.0.02 schematic.

## Where to look next

- New to the board? Skim [HKL-EA8 Product Introduction Document.pdf](HKL-EA8%20%20Product%20Introduction%20Document.pdf) first, then page through [HKL-EA8_Demo_Examples_Glossary.md](HKL-EA8_Demo_Examples_Glossary.md) to find the demo that matches the peripheral you need.
- Deciding between V1 and V3 hardware (or porting firmware between them)? See [HKL-EA8_V1_vs_V3_Schematic_Differences.md](HKL-EA8_V1_vs_V3_Schematic_Differences.md). The electrically meaningful changes are: relay outputs are now buffered through `U74`/`U75`; the HMI port is now jumper-selectable to 1-Wire; AIN2 is documented as 0–10 V (with a contradiction on the drawing — verify on hardware).
- Want Home Assistant integration with no firmware work? Start from [Hankerila-EA8-demo code/0.esphome/Hankerila-EA8.yaml](Hankerila-EA8-demo%20code/0.esphome/Hankerila-EA8.yaml).

> **Note:** The contents of this folder are vendor material, kept here unmodified for reference. Only [HKL-EA8_V1_vs_V3_Schematic_Differences.md](HKL-EA8_V1_vs_V3_Schematic_Differences.md) and [HKL-EA8_Demo_Examples_Glossary.md](HKL-EA8_Demo_Examples_Glossary.md) were authored for this project.
