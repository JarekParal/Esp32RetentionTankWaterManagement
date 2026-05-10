//hankerila official store----www.aliexpress.com/store/1103673574
/*EA8 GPIO 15 GPIO16 Serial port communication*/
// Install EspSoftwareSerial library by Dirk Kaar,Peter
#include <SoftwareSerial.h>
#include<PCF8574.h>
PCF8574 pcf8574(0x24,4,5);

void setup() {
  Serial.begin(9600); 
  Serial1.begin(9600,SERIAL_8N1,15,16);  
  pcf8574.pinMode(P0, OUTPUT); 
  pcf8574.pinMode(P1, OUTPUT); 
  pcf8574.pinMode(P2, OUTPUT); 
  pcf8574.pinMode(P3, OUTPUT);
  pcf8574.pinMode(P4, OUTPUT); 
  pcf8574.pinMode(P5, OUTPUT); 
  pcf8574.pinMode(P6, OUTPUT); 
  pcf8574.pinMode(P7, OUTPUT); 
  pcf8574.begin();
  pcf8574.digitalWrite(P0,LOW);
  pcf8574.digitalWrite(P1,LOW);
  pcf8574.digitalWrite(P2,LOW);
  pcf8574.digitalWrite(P3,LOW);
  pcf8574.digitalWrite(P4,LOW);
  pcf8574.digitalWrite(P5,LOW);
  pcf8574.digitalWrite(P6,LOW);
  pcf8574.digitalWrite(P7,LOW);
}

void loop() {
  if (Serial1.available()) {
    // Read the received data into a string variable
    String receivedData = Serial1.readStringUntil('\n'); 
    Serial.println(receivedData);
    // Check whether the received string is "relay01"
    if (receivedData == "relay01") {
      int currentState = pcf8574.digitalRead(P0);
      pcf8574.digitalWrite(P0,currentState == LOW? HIGH : LOW);  
    }
    // Check whether the received string is "relay02"
    if (receivedData == "relay02") {
      int currentState = pcf8574.digitalRead(P1);
      pcf8574.digitalWrite(P1,currentState == LOW? HIGH : LOW);  
    }
    if (receivedData == "relay03") {
      int currentState = pcf8574.digitalRead(P2);
      pcf8574.digitalWrite(P2,currentState == LOW? HIGH : LOW);  
    }
    if (receivedData == "relay04") {
      int currentState = pcf8574.digitalRead(P3);
      pcf8574.digitalWrite(P3,currentState == LOW? HIGH : LOW);  
    }
    if (receivedData == "relay05") {
      int currentState = pcf8574.digitalRead(P4);
      pcf8574.digitalWrite(P4,currentState == LOW? HIGH : LOW);  
    }
    if (receivedData == "relay06") {
      int currentState = pcf8574.digitalRead(P5);
      pcf8574.digitalWrite(P5,currentState == LOW? HIGH : LOW);  
    }
    if (receivedData == "relay07") {
      int currentState = pcf8574.digitalRead(P6);
      pcf8574.digitalWrite(P6,currentState == LOW? HIGH : LOW);  
    }
    if (receivedData == "relay08") {
      int currentState = pcf8574.digitalRead(P7);
      pcf8574.digitalWrite(P7,currentState == LOW? HIGH : LOW);  
    }
  }

}