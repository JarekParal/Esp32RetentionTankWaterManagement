//hankerila official store----www.aliexpress.com/store/1103673574
/*EA8 DAC output 0-10V*/
/*0-4095 corresponds to 0-5V and outputs 0-10V through the amplifier*/
#include <Wire.h>

// Define the I2C address of MCP4725. 
const int MCP4725_ADDR1 = 0x60; 

// This function is used to send the input value (ranging from 0 to 4095) to MCP4725.
// The input parameter 'val' represents the value that needs to be sent.
void setVoltage(int val) {
    Wire.beginTransmission(MCP4725_ADDR1);
    // Write the most significant byte of the value to the device.
    // The '& 0xFF' operation is used to ensure that only the lower 8 bits are considered.
    Wire.write((val >> 8) & 0xFF);
    // Write the least significant byte of the value to the device.
    Wire.write(val & 0xFF);
    // End the I2C transmission to complete sending the data to the device.
    Wire.endTransmission();
}

void setup() {
    Serial.begin(115200); 
    Wire.begin(4, 5); //GPIO4-SDA GPIO5-SCL
}

void loop() {
    // This is an example loop that gradually increases the value from 0 to 4095.
    // The value is incremented by 41 each time, which approximately corresponds to an increment of 0.1V.
    for (int val = 0; val <= 4095; val += 41) {   
        setVoltage(val);
        Serial.print("Set value to: ");
        Serial.println(val);
        delay(200); 
    }
}