#include "Arduino.h"
#include "PCF8574.h"

// Set the I2C address
PCF8574 pcf8574_1(0x26, 4, 5);  // Create a PCF8574 object with address 0x22 and specified pins

unsigned long timeElapsed;

void setup() {
    Serial.begin(115200);  // Start serial communication at 115200 baud rate
    delay(1000);  // Delay for 1000 milliseconds

    //pcf8574.pinMode(P0, OUTPUT);
    pcf8574_1.pinMode(P0, INPUT);  // Set pin P0 of pcf8574_1 as input
    pcf8574_1.pinMode(P1, INPUT);  // Set pin P1 of pcf8574_1 as input
    pcf8574_1.pinMode(P2, INPUT);  // Set pin P2 of pcf8574_1 as input
    pcf8574_1.pinMode(P3, INPUT);  // Set pin P3 of pcf8574_1 as input
    pcf8574_1.pinMode(P4, INPUT);  // Set pin P4 of pcf8574_1 as input
    pcf8574_1.pinMode(P5, INPUT);  // Set pin P5 of pcf8574_1 as input
    pcf8574_1.pinMode(P6, INPUT);  // Set pin P6 of pcf8574_1 as input
    pcf8574_1.pinMode(P7, INPUT);  // Set pin P7 of pcf8574_1 as input

    pcf8574_1.begin();  // Initialize pcf8574_1

}

void loop() {
    uint8_t val1 = pcf8574_1.digitalRead(P0);  // Read digital value from pin P0 of pcf8574_1
    uint8_t val2 = pcf8574_1.digitalRead(P1);  // Read digital value from pin P1 of pcf8574_1
    uint8_t val3 = pcf8574_1.digitalRead(P2);  // Read digital value from pin P2 of pcf8574_1
    uint8_t val4 = pcf8574_1.digitalRead(P3);  // Read digital value from pin P3 of pcf8574_1
    uint8_t val5 = pcf8574_1.digitalRead(P4);  // Read digital value from pin P4 of pcf8574_1
    uint8_t val6 = pcf8574_1.digitalRead(P5);  // Read digital value from pin P5 of pcf8574_1
    uint8_t val7 = pcf8574_1.digitalRead(P6);  // Read digital value from pin P6 of pcf8574_1
    uint8_t val8 = pcf8574_1.digitalRead(P7);  // Read digital value from pin P7 of pcf8574_1

    if (val1 == LOW) Serial.println("KEY1 PRESSED");  // If the value on pin P0 of pcf8574_1 is low, print "KEY1 PRESSED"
    if (val2 == LOW) Serial.println("KEY2 PRESSED");  // Similar for other pins and corresponding messages
    if (val3 == LOW) Serial.println("KEY3 PRESSED");
    if (val4 == LOW) Serial.println("KEY4 PRESSED");
    if (val5 == LOW) Serial.println("KEY5 PRESSED");
    if (val6 == LOW) Serial.println("KEY6 PRESSED");
    if (val7 == LOW) Serial.println("KEY7 PRESSED");
    if (val8 == LOW) Serial.println("KEY8 PRESSED");

   delay(300);  // Delay for 300 milliseconds
}