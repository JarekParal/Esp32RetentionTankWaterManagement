# HKL-EA8 Intelligent Control Board — Product Introduction

> Markdown extraction of [HKL-EA8 Product Introduction Document.pdf](HKL-EA8%20%20Product%20Introduction%20Document.pdf), the vendor-supplied datasheet from HANKERILA. Content is preserved verbatim from the source; only formatting has been adapted to markdown. Refer to the original PDF for the diagrams and product photos.
>
> Vendor: HANKERILA — Official Store: [aliexpress.com/store/1103673574](https://www.aliexpress.com/store/1103673574)

---

## I. Product Overview

The HKL-EA8 intelligent controller is specifically designed for diverse control and data interaction scenarios. It integrates a rich array of interfaces and communication functions, supports wide voltage input of DC12V or DC24V, and boasts powerful signal processing capabilities and flexible networking communication methods. This makes it widely applicable in fields such as industrial automation, smart home, and environmental monitoring, providing users with efficient and stable intelligent control solutions.

---

## II. Core Functions and Technical Parameters

### (I) Power Input

> *This power supply supports DC input ranging from 12V to 24V, offering flexibility for various applications. It provides stable and reliable power, ensuring smooth operation of connected devices.*

| Project | Parameter Details |
|---|---|
| Input Voltage | DC12V or DC24V |
| Power Management Chip | Built-in high-efficiency power management chip |
| Protection Functions | Overvoltage, undervoltage, and overcurrent protection |
| Application Advantages | Compatible with various power supply devices, reducing costs and complexity; multiple protection features enhance device reliability and service life |

### (II) Digital Input

> *This circuit board has 8 input channels, compatible with both NPN and PNP sensors. Different-colored lights indicate various states.*

| Project | Parameter Details |
|---|---|
| Number of Ports | 8 universal input ports supporting NPN or PNP |
| Control chip | PCF8574 (Address `0x26`) — SDA(GPIO4) SCL(GPIO5) |
| Status Indication | Each port is equipped with an independent indicator light to display the signal input status in real-time |
| Electrical Characteristics | High input impedance, strong anti-interference ability |
| Application Scenarios | Connecting switch-type sensors such as limit switches and proximity switches, used for detecting the status of industrial equipment and home security sensing |

### (III) Relay Output

| Project | Parameter Details |
|---|---|
| Number of Ports | 8 relay output ports |
| Control chip | PCF8574 (Address `0x24`) — SDA(GPIO4) SCL(GPIO5) |
| Contact Capacity | 10A 277VAC or 12A 125VAC |
| Technical Features | Adopts magnetic latching technology, low power consumption, fast response speed, high reliability, and long service life |
| Application Scenarios | Driving high-power loads such as motors and solenoid valves, realizing start-stop control of industrial equipment and switching control of home appliances |

> Relay part: **JQC-3FF / 12VDC-1ZS(551)** — 10A 277VAC, 12A 125VAC.

### (IV) Analog Input

This product's analog input utilizes the **ADS1115** high-precision 16-bit ADC chip, which offers excellent conversion accuracy and stability, meeting the requirements of various complex signal acquisition scenarios.

#### 1. AIN0 – AIN1 (Differential Input Pair for Power Current Detection)

- **Parameters:** Leveraging the differential input characteristics of the ADS1115 chip, it can effectively suppress common-mode interference, improving the accuracy and stability of signal acquisition. It supports a differential voltage input range of ± [X] V (subject to actual configuration), suitable for high-precision power current detection scenarios.
- **Working Principle:** AIN0 and AIN1 form a differential input pair, connecting to the secondary output of the current transformer. By detecting the voltage difference across the current transformer, the magnitude of the power current is accurately calculated. After the ADS1115 chip performs AD conversion on the differential signal, the data is transmitted to the main control unit for real-time monitoring of the power current.
- **Application Scenarios:** In industrial equipment and smart home systems, it is used to monitor the current of power lines, enabling timely detection of abnormal situations such as overloads and short circuits, achieving overload protection and fault warning. It can also be applied to energy consumption analysis, helping users optimize energy usage strategies and reduce operating costs.

#### 2. ADC1 (0 – 5V Signal Detection, AIN2 Interface)

- **Parameters:** Based on the ADS1115 chip, it realizes 16-bit high-precision AD conversion with a maximum sampling rate of 860 SPS, input impedance greater than 10 MΩ, and measurement accuracy of ±0.1% FS. It supports 0 – 5V voltage signal input, and the built-in signal conditioning circuit can filter and amplify the input signal to ensure the accuracy of the collected data.
- **Working Principle:** The AIN2 port receives the external 0 – 5V voltage signal, and the ADS1115 chip converts it into a digital signal, which is transmitted to the main control unit for processing and analysis via the I²C communication protocol.
- **Application Scenarios:** Suitable for connecting voltage-type sensors such as pressure sensors and temperature sensors (through voltage output modules). In environmental monitoring systems, it can accurately collect the voltage signals output by temperature and humidity sensors to obtain real-time environmental parameter changes. In industrial automation production lines, it is used to monitor the voltage status of equipment, ensuring stable production operations.

#### 3. ADC2 (0 – 20mA Signal Detection, AIN3 Interface)

- **Parameters:** Also based on the 16-bit high-precision conversion capability of the ADS1115 chip, it is optimized for 4 – 20mA current signal input. A built-in precision sampling resistor converts the current signal into a voltage signal, and it has a disconnection detection function to monitor the signal transmission status in real-time.
- **Working Principle:** The external 4 – 20mA current signal is input through the AIN3 interface. After being converted into a voltage signal using a 0.1% high-precision sampling resistor R60 (200 Ω), the ADS1115 chip performs AD conversion, and the converted digital signal is uploaded to the main control unit via I²C communication, realizing the digital acquisition of current signals.
- **Application Scenarios:** Commonly used for connecting various current-type sensors such as flow sensors and level sensors. In industrial pipeline monitoring, it can obtain real-time current signal data of flow and level, providing accurate basis for production scheduling. In energy management systems, it monitors the working current of equipment for energy consumption statistics and analysis.

### (V) Analog Output

The analog output of this product employs the **MCP4725** chip, a 12-bit high-precision digital-to-analog converter that enables stable and accurate 0 – 10V signal output.

| Project | Parameter Details |
|---|---|
| Output Port | DAC1 (0 – 10V output) |
| Conversion Chip | 12-bit DAC conversion chip |
| Output Accuracy | ±0.5% FS |
| Load Capacity | 40 – 60 mA |
| Application Scenarios | Controlling analog-adjustable devices such as frequency converters and electric control valves to achieve precise industrial automation control |

> The MCP4725 is supplied with 0–5 V and the LM358 amplification factor `Au = 1 + Rf/Ri` is amplified by a factor of 2 to produce the 0–10 V output.

### (VI) Communication Interfaces

#### 1. RS485 Communication Interface

| Project | Parameter Details |
|---|---|
| Supported Protocol | Modbus RTU protocol |
| Communication Rate | Up to 115200 bps |
| Communication Distance | Up to 1200 meters (under standard conditions) |
| Special Features | Automatic flow control function to suppress signal reflection and interference |
| Application Scenarios | Networking with devices such as PLCs and intelligent instruments for data interaction in industrial fieldbus |

> **Example power-meter parameters (Modbus RTU):** baud rate 9600 bps, 8 data bits, 1 stop bit, EVEN parity, function 0x03 (read).
>
> | Register address | Data | IEEE-754 | Function |
> |---|---|---|---|
> | 0x00, 0x01 | Power consumption (kWh) | Float | Read |
> | 0x64, 0x65 | Voltage (V) | Float | Read |
> | 0x6A, 0x6B | Current (A) | Float | Read |
> | 0x76, 0x77 | Power (kW) | Float | Read |
> | 0x8E, 0x8F | Power Factor | Float | Read |
> | 0x90, 0x91 | Frequency (Hz) | Float | Read |

#### 2. Ethernet Communication

| Project | Parameter Details |
|---|---|
| Interface Type | 10/100Mbps auto-negotiation Ethernet interface |
| Supported Protocol | TCP/IP protocol |
| Special Features | Built-in network wake-up function, supporting remote configuration and monitoring |
| Application Scenarios | Connecting devices to local area networks or the Internet for remote control of smart homes and cloud data interaction in industrial IoT |

#### 3. WIFI Communication

| Project | Parameter Details |
|---|---|
| Frequency Band | 2.4 GHz band |
| Standard | Complies with IEEE 802.11 b/g/n standards |
| Application Scenarios | Wireless connection of devices for local wireless control of smart homes and mobile device control |

### (VII) Special Function Interfaces

#### 1. SHT3X Sensor Interface

| Project | Parameter Details |
|---|---|
| Communication Protocol | I²C communication protocol |
| Communication Modes | Supports standard mode (100 kHz) and fast mode (400 kHz) |
| Application Scenarios | Connecting SHT3X temperature and humidity sensors for temperature and humidity data collection in environmental monitoring, warehousing logistics, and smart homes |

#### 2. HMI Interface

| Project | Parameter Details |
|---|---|
| Supported Devices | Various HMI devices such as touch screens and liquid crystal displays |
| Communication Modes | Customizable communication protocols, supporting serial communication |
| Application Scenarios | Providing a local human-machine interaction interface for parameter setting, status viewing, and device control, enhancing operational convenience |

#### 3. Infrared Emission and Reception Interface

| Project | Parameter Details |
|---|---|
| Emission Interface | Supports 38 kHz carrier frequency, emission distance up to 10 meters (in an unobstructed environment) |
| Reception Interface | High sensitivity, capable of receiving standard infrared remote control signals |
| Application Scenarios | Controlling infrared devices, realizing infrared sensing control at home, such as remote control of home appliances and automatic lighting upon human body induction |

#### 4. IIC Communication Interface

| Project | Parameter Details |
|---|---|
| Number of Ports | 2 groups of I²C communication interfaces |
| Communication Modes | Supports standard mode (100 kHz) and fast mode (400 kHz) |
| Special Features | Capable of connecting multiple I²C devices, with automatic arbitration and conflict detection functions |
| Application Scenarios | Expanding I²C devices such as EEPROM storage chips and barometric pressure sensors to enrich device functions and application scenarios |

> Compatible HANKERILA add-on boards that can be connected via I²C: **ADI16**, **DO8**, **A6**.

---

## III. Application Scenarios

### (I) Industrial Automation Field

In industrial automation production lines, the EA8 can connect various sensors through digital input ports to monitor the operating status of production equipment in real-time. Relay output ports are used to control the actuators like motors and valves, realizing automated production processes. Through RS485 and Ethernet communication interfaces, it interacts with devices such as PLCs and upper computers, constructing a complete industrial automation control system to improve production efficiency and product quality.

### (II) Smart Home Field

In smart home systems, the product supports multiple communication methods. Users can remotely control home appliances through mobile phone APPs, realizing functions such as lighting adjustment, curtain opening/closing, and air conditioning control. Digital input ports can detect door/window status and human infrared signals, enabling smart security and automated scene control. The analog output port can adjust the brightness of smart dimming devices, creating a comfortable home environment.

### (III) Environmental Monitoring Field

In environmental monitoring projects, the SHT3X sensor interface and analog input ports are used to collect environmental parameters such as temperature, humidity, air pressure, and air quality in real-time. Utilizing Ethernet or WIFI communication interfaces, the data is uploaded to cloud servers for remote real-time monitoring and data analysis. Meanwhile, according to changes in environmental parameters, relay output and analog output ports can automatically control ventilation, dehumidification, cooling, and other equipment to maintain environmental comfort and safety.

### (IV) Energy Management Field

In energy management systems, the current detection signal interface can monitor the power consumption of equipment in real-time. Combined with analog input and communication interfaces, it realizes the collection, analysis, and statistics of energy consumption data. Relay output ports control the start and stop of power equipment, achieving energy-saving optimization and intelligent control, reducing energy consumption and operating costs.

---

## IV. Product Advantages

1. **High Integration:** Integrates multiple input/output interfaces and communication functions, meeting diverse control and data interaction requirements, reducing external expansion devices, and lowering system costs and complexity.
2. **High Reliability:** Adopts high-quality components and advanced circuit designs, equipped with complete protection mechanisms, adaptable to harsh working environments, ensuring long-term stable operation of the device.
3. **Flexible Expandability:** Abundant interfaces and communication protocol support facilitate users to expand functions and devices according to actual needs, meeting the personalized requirements of different application scenarios.
4. **Convenient and Easy to Use:** Provides a user-friendly human-machine interface and comprehensive development tools, supports multiple communication protocols and programming languages, making it convenient for users to develop and debug, reducing development difficulty and cycle.

---

## V. Technical Support and Services

We offer comprehensive technical support and high-quality after-sales services, including product manuals, sample codes, and other materials, as well as online technical consultation, remote debugging, on-site maintenance, and other services. If you have any questions or needs, please feel free to contact our technical support team. We will wholeheartedly serve you.

- **WhatsApp:** +86 13575789565
- **Email:** 1208toglobal@gmail.com
- **Store:** [aliexpress.com/store/1103673574](https://www.aliexpress.com/store/1103673574)
