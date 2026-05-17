# HW modification notes for this project

## Buzzer -> replaced with input interrupt for [PCF8574 I/O expander](./datasheets/PCF8574_datasheet.pdf)

- R27 (10k pull-up), R47 (2k) and R106 (10k pull-down) removed to allow connecting the INT (pin 13) from PCF8574 to IO12 (pin 14) on [ESP32-WROOM-32 (page 8)](./datasheets/esp32-wroom-32_datasheet_en.pdf)
- added direct connection between INT output on PCF8574 and ESP32-WROOM IO12 (without any resistor on the way -> internal pull-up has to be activated)
- removed the Beeper functionality (forever - we don't want to use it anymore)

## Infrared Transmitter/Receiver -> replace with Ultrasonic (HC-SR04) 

- removed transistor U24, infra LED U26, infra receiver IR1, R50 (2k -> replaced 200R), R51 (10k pull-down)
- keep the R52 (47k pull-up)
- connector "schema"
    - 1 - 3V3
    - 2 - GND
    - 3 - IO33 - echo/receiver - direct connection to ESP with R52 (47k) pull-up
    - 4 - IO32 - trigger/transmitter - replace R50 2k with 200R 
