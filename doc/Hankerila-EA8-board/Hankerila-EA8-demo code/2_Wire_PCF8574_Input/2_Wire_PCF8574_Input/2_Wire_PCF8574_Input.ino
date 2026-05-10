#include <Arduino.h>
#include <Wire.h>
#include "PCF8574.h"

// I2C address of PCF8574
const byte PCF8574_ADDRESS = 0x26;

// Define SDA and SCL pins
const int SDA_PIN = 4;
const int SCL_PIN = 5;

// Create a PCF8574 object
PCF8574 pcf8574_in(PCF8574_ADDRESS,SDA_PIN,SCL_PIN);



void setup() {
    // Initialize serial communication at a baud rate of 115200
    Serial.begin(115200);
    // Wait for the serial port to initialize
    while (!Serial);
    // Initialize the Wire library and specify the SDA and SCL pins
    Wire.begin(SDA_PIN, SCL_PIN);
    Serial.println("Wire library initialized.");

    // Initialize the 8 pins of PCF8574 as input mode
    for (int i = 0; i < 8; i++) {
        pcf8574_in.pinMode(i, INPUT);
    }
    // Initialize pcf8574_in
    if (pcf8574_in.begin()) {
        Serial.println("pcf8574_in initialized successfully.");
    } else {
        Serial.println("Failed to initialize pcf8574_in.");
    }

}

void loop() {
    // Request 1 byte of data from pcf8574_in
    Wire.requestFrom(PCF8574_ADDRESS, 1);
    if (Wire.available()) {
        // Read the received byte of data
        byte inputData = Wire.read();
        // Check if the first key is pressed
        if(inputData==0xFE){
          Serial.println("KEY 1 PRESSED");
        }
        // Check if the second key is pressed
        if(inputData==0xFD){
          Serial.println("KEY 2 PRESSED");
        }
        // Check if the third key is pressed
        if(inputData==0xFB){
          Serial.println("KEY 3 PRESSED");
        }
        // Check if the fourth key is pressed
        if(inputData==0xF7){
          Serial.println("KEY 4 PRESSED");
        }
        // Check if the fifth key is pressed
        if(inputData==0xEF){
          Serial.println("KEY 5 PRESSED");
        }
        // Check if the sixth key is pressed
        if(inputData==0xDF){
          Serial.println("KEY 6 PRESSED");
        }
        // Check if the seventh key is pressed
        if(inputData==0xBF){
          Serial.println("KEY 7 PRESSED");
        }
        // Check if the eighth key is pressed
        if(inputData==0x7F){
          Serial.println("KEY 8 PRESSED");
        }
    }
}