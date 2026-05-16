# HW modification notes for this project

## BEEP -> reused for input interrupt for [PCF8574 I/O expander](./datasheets/PCF8574_datasheet.pdf)

- R47 (2k) and R106 (10k) removed to allow connecting the INT (pin 13) from PCF8574 to IO12 (pin 14) on [ESP32-WROOM-32 (page 8)](./datasheets/esp32-wroom-32_datasheet_en.pdf)
- add direct connection between INT output on PCF8574 and ESP32-WROOM IO12 
- remove the Beeper functionality (forever)