#include <Arduino.h>
#include <Wire.h>
#include "PCF8574.h"

// I2C pins definition
const int SDA_PIN = 4;
const int SCL_PIN = 5;

// PCF8574 device addresses
PCF8574 pcf8574(0x26, SDA_PIN, SCL_PIN);  // Input module
PCF8574 pcf8574_re(0x24, SDA_PIN, SCL_PIN); // Relay control module

// Input pins (PCF8574 P0-P7)
const int INPUT_1_PIN = 0;
const int INPUT_2_PIN = 1;
const int INPUT_3_PIN = 2;
const int INPUT_4_PIN = 3;
const int INPUT_5_PIN = 4;
const int INPUT_6_PIN = 5;
const int INPUT_7_PIN = 6;
const int INPUT_8_PIN = 7;

// Relay output pins (PCF8574_re P0-P7)
const int RELAY_1_PIN = 0;
const int RELAY_2_PIN = 1;
const int RELAY_3_PIN = 2;
const int RELAY_4_PIN = 3;
const int RELAY_5_PIN = 4;
const int RELAY_6_PIN = 5;
const int RELAY_7_PIN = 6;
const int RELAY_8_PIN = 7;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    Serial.println(__FILE__);  //Print file storage path
    while (!Serial);
    
    // Initialize I2C communication
    Wire.begin(SDA_PIN, SCL_PIN);
    Serial.println("Wire library initialized.");

    // Configure all pins of the input module as INPUT
    for (int i = 0; i < 8; i++) {
        pcf8574.pinMode(i, INPUT);
    }
    
    // Configure all pins of the relay module as OUTPUT
    for (int i = 0; i < 8; i++) {
        pcf8574_re.pinMode(i, OUTPUT);
    }
    
    // Initialize PCF8574 devices
    if (pcf8574.begin()) {
        Serial.println("PCF8574 input module initialized successfully.");
    } else {
        Serial.println("Failed to initialize PCF8574 input module.");
    }
    
    if (pcf8574_re.begin()) {
        Serial.println("PCF8574 relay control module initialized successfully.");
    } else {
        Serial.println("Failed to initialize PCF8574 relay control module.");
    }
    
}

void loop() {
    // Request input data from PCF8574
    Wire.requestFrom(0x26, 1);
    if (Wire.available()) {
        // Read input byte
        byte inputData = Wire.read();
        Serial.print("Input Data: 0x");
        Serial.println(inputData, HEX);
        
        // ============== INPUT 1 CONTROL ==============
        if (!(inputData & (1 << INPUT_1_PIN))) {
            pcf8574_re.digitalWrite(RELAY_1_PIN, LOW);
            Serial.println("INPUT_1 is LOW --> Relay 1 turned ON");
        } else {
            pcf8574_re.digitalWrite(RELAY_1_PIN, HIGH);
            //Serial.println("INPUT_1 is HIGH --> Relay 1 turned OFF");
        }
        // ============== INPUT 2 CONTROL ==============
        if (!(inputData & (1 << INPUT_2_PIN))) {
            pcf8574_re.digitalWrite(RELAY_2_PIN, LOW);
            Serial.println("INPUT_2 is LOW --> Relay 2 turned ON");
        } else {

            pcf8574_re.digitalWrite(RELAY_2_PIN, HIGH);
           // Serial.println("INPUT_2 is HIGH --> Relay 2 turned OFF");
        }
        
        // ============== INPUT 3 CONTROL ==============
        if (!(inputData & (1 << INPUT_3_PIN))) {
            pcf8574_re.digitalWrite(RELAY_3_PIN, LOW);
            Serial.println("INPUT_3 is LOW --> Relay 3 turned ON");
        } else {
            pcf8574_re.digitalWrite(RELAY_3_PIN, HIGH);
           // Serial.println("INPUT_3 is HIGH --> Relay 3 turned OFF");
        }
        
        // ============== INPUT 4 CONTROL ==============
        if (!(inputData & (1 << INPUT_4_PIN))) {
            pcf8574_re.digitalWrite(RELAY_4_PIN, LOW);
            Serial.println("INPUT_4 is LOW --> Relay 4 turned ON");
        } else {
            pcf8574_re.digitalWrite(RELAY_4_PIN, HIGH);
           // Serial.println("INPUT_4 is HIGH --> Relay 4 turned OFF");
        }
        
        // ============== INPUT 5 CONTROL ==============
        if (!(inputData & (1 << INPUT_5_PIN))) {
            pcf8574_re.digitalWrite(RELAY_5_PIN, LOW);
            Serial.println("INPUT_5 is LOW --> Relay 5 turned ON");
        } else {
            pcf8574_re.digitalWrite(RELAY_5_PIN, HIGH);
            //Serial.println("INPUT_5 is HIGH --> Relay 5 turned OFF");
        }
        
        // ============== INPUT 6 CONTROL ==============
        if (!(inputData & (1 << INPUT_6_PIN))) {
            pcf8574_re.digitalWrite(RELAY_6_PIN, LOW);
            Serial.println("INPUT_6 is LOW --> Relay 6 turned ON");
        } else {
            pcf8574_re.digitalWrite(RELAY_6_PIN, HIGH);
            //Serial.println("INPUT_6 is HIGH --> Relay 6 turned OFF");
        }
        
        // ============== INPUT 7 CONTROL ==============
        if (!(inputData & (1 << INPUT_7_PIN))) {
            pcf8574_re.digitalWrite(RELAY_7_PIN, LOW);
            Serial.println("INPUT_7 is LOW --> Relay 7 turned ON");
        } else {
            pcf8574_re.digitalWrite(RELAY_7_PIN, HIGH);
            //Serial.println("INPUT_7 is HIGH --> Relay 7 turned OFF");
        }
        
        // ============== INPUT 8 CONTROL ==============
        if (!(inputData & (1 << INPUT_8_PIN))) {
            pcf8574_re.digitalWrite(RELAY_8_PIN, LOW);
            Serial.println("INPUT_8 is LOW --> Relay 8 turned ON");
        } else {
            pcf8574_re.digitalWrite(RELAY_8_PIN, HIGH);
            //Serial.println("INPUT_8 is HIGH --> Relay 8 turned OFF");
        }
        
        // Add a small delay to avoid frequent I2C communication
        delay(100);
    }
}    