//hankerila official store----www.aliexpress.com/store/1103673574
/*EA8 DAC output 0-10V*/
#include <Wire.h>

// Define the I2C address of MCP4725. 
//const int MCP4725_ADDR1 = 0x60; 
const int MCP4725_ADDR1 = 0x60; 

// This function is used to convert the input value (ranging from 0 to 4095) into appropriate data and then send it to MCP4725.
// The input parameter 'voltage_mV' represents the voltage value in millivolts that needs to be converted and sent.
void setVoltage(int voltage_mV) {
    // Map the input voltage value (in the range of 0 - 5000 mV) to a value within the range of 0 - 4095.
    // This mapping is likely to adjust the input voltage range to fit the requirements of the MCP4725 device.
    int val = map(voltage_mV, 0, 5000, 0, 4095);
    Wire.beginTransmission(MCP4725_ADDR1);
    // Write the most significant byte of the converted value to the device.
    // The '& 0xFF' operation is used to ensure that only the lower 8 bits are considered.
    Wire.write((val >> 8) & 0xFF);
    // Write the least significant byte of the converted value to the device.
    Wire.write(val & 0xFF);
    // End the I2C transmission to complete sending the data to the device.
    Wire.endTransmission();
}

void setup() {

    Serial.begin(115200); 
    Serial.println(__FILE__);
    Wire.begin(4, 5); //GPIO4-SDA GPIO5-SCL
}

void loop() {
    // This is an example loop that gradually increases the voltage from 0V to 5V.
    // The voltage is incremented by 0.1V (represented as 100 mV ) each time.
    //Because we output 0-10V, amplified by a factor of 2, the DAC output is 0.2V when the increment is 100mv
    for (int voltage_mV = 0; voltage_mV <= 5000; voltage_mV += 100) {   
        setVoltage(voltage_mV);
        Serial.print("Set voltage to: ");
        Serial.print(voltage_mV);
        Serial.println(" mV");
        delay(200); 
    }
    for (int voltage_mV = 5000; voltage_mV >=0; voltage_mV -= 100) {   
        setVoltage(voltage_mV);
        Serial.print("Set voltage to: ");
        Serial.print(voltage_mV);
        Serial.println(" mV");
        delay(200); 
    }
    delay(2000);
}
   