/*This is code that has been tested for normal reading function.
Written by myp on 2024-01-17*/
#include <ModbusMaster.h>
#include <SoftwareSerial.h>

// Instantiate a ModbusMaster object.
ModbusMaster node;
// Define SoftwareSerial for RX and TX pins.
SoftwareSerial BTserial(14, 13);

// Variables to store temperature and humidity.
uint16_t temperature = 0;
uint16_t humidity = 0;

void setup()
{
    // Initialize serial communication at 9600 baud rate.
    Serial.begin(9600);
    BTserial.begin(9600);

    // Set the Modbus slave ID and serial port.
    node.begin(203, BTserial); // Set the address of the slave meter to 203.
}

// A boolean variable for state.
bool state = true;

void loop()
{
    // Create a receiving buffer.
    uint8_t result;
    // Set the starting address for reading registers.
    uint16_t sum_add = 64;
    /*For example, if the voltage data storage registers of the meter are 0x64 and 0X65, then the starting address is 0x64, and the length of the registers is 2 (0x64 and 0x65).*/
    // Set the length of registers to be read.
    uint16_t sum_leng = 2;
    // Read holding registers.
    result = node.readHoldingRegisters(sum_add, sum_leng);
    //send  = CB 03 00 40 00 02 D4 75
    /*CB is 203 which is the address of the meter.
      03 is the function code.
      00 40 is the starting address of the register.
      00 02 is the length of registers to be read.
      D4 75 is CRC.*/
    //read =  CB 03 04 30 39 26 94 54 FD
    /*Using sscom, after sending the above command on the PCB, immediately send the above command.
      CB: The starting address of the register to be read.
      03: Function code.
      04: Since the length of registers to be read is 2, it is 4 bytes.
      30 39 26 94: These are the 4-byte data.
      54 FD: CRC check. When sending through SSCOM, you can select modbus crc16 to be automatically added.*/


    //send  = 0A 03 00 01 00 02 94 B0
    //send  = 0A 03 00 01 00 06 95 73   leng=6
    //read =   0A 03 04 30 39 26 94 84 31
    //read =   0A 03 0C 30 39 26 94 20 21 22 23 24 25 26 27 87 06  leng=6



    if (result == node.ku8MBSuccess)
    {
        // Get temperature and humidity from the response buffer.
        temperature = node.getResponseBuffer(0);
        humidity = node.getResponseBuffer(1);
        // Extract high and low bytes of temperature.
        uint16_t i = highByte(temperature);
        uint16_t j = lowByte(temperature);
        // Extract high and low bytes of humidity.
        uint16_t a = highByte(humidity);
        uint16_t b = lowByte(humidity);
        // Print temperature and humidity.
        Serial.printf("温度是%x.%x\n", i, j);
        Serial.printf("湿度是%x.%x\n", a, b);
        //Serial.println(humidity);
    }
    else
    {
        // Print error message if reading fails.
        Serial.println("error");
    }


    delay(1000);
}