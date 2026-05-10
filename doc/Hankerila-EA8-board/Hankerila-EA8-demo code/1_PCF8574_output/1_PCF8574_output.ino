#include "Arduino.h"
/*install PCF8574 library by Renzo Mischianti*/
#include "PCF8574.h"


/*SDA pin GPIO4*/
/*SCL pin GPIO5*/
#define SDA 4
#define SCL 5
/*Create the PCF8574 instance*/
/*Set i2c address*/
PCF8574 pcf8574(0x24,SDA,SCL);



void setup()
{
	Serial.begin(115200);
  Serial.println(__FILE__);  //Print file storage path

	// Set pinMode to OUTPUT
	pcf8574.pinMode(P0, OUTPUT);
  pcf8574.pinMode(P1, OUTPUT);
  pcf8574.pinMode(P2, OUTPUT);
  pcf8574.pinMode(P3, OUTPUT);
  pcf8574.pinMode(P4, OUTPUT);
  pcf8574.pinMode(P5, OUTPUT);
  pcf8574.pinMode(P6, OUTPUT);
  pcf8574.pinMode(P7, OUTPUT);
  //Initialize PCF8574
  pcf8574.begin();
  delay(300);
	pcf8574.digitalWrite(P0, HIGH);
  delay(300);
  pcf8574.digitalWrite(P1, HIGH);
  delay(300);
  pcf8574.digitalWrite(P2, HIGH);
  delay(300);
  pcf8574.digitalWrite(P3, HIGH);
  delay(300);
  pcf8574.digitalWrite(P4, HIGH);
  delay(300);
  pcf8574.digitalWrite(P5, HIGH);
  delay(300);
  pcf8574.digitalWrite(P6, HIGH);
  delay(300);
  pcf8574.digitalWrite(P7, HIGH);
  delay(300);/*
  	pcf8574.digitalWrite(P0, LOW);
  delay(300);
  pcf8574.digitalWrite(P1, LOW);
  delay(300);
  pcf8574.digitalWrite(P2, LOW);
  delay(300);
  pcf8574.digitalWrite(P3, LOW);
  delay(300);
  pcf8574.digitalWrite(P4, LOW);
  delay(300);
  pcf8574.digitalWrite(P5, LOW);
  delay(300);
  pcf8574.digitalWrite(P6, LOW);
  delay(300);
  pcf8574.digitalWrite(P7, LOW);
  delay(300);*/
}

void loop()
{
  pcf8574.digitalWrite(P0, HIGH);
  delay(300);
  /*pcf8574.digitalWrite(P1, HIGH);
  delay(300);
  pcf8574.digitalWrite(P2, HIGH);
  delay(300);
  pcf8574.digitalWrite(P3, HIGH);
  delay(300);
  pcf8574.digitalWrite(P4, HIGH);
  delay(300);
  pcf8574.digitalWrite(P5, HIGH);
  delay(300);
  pcf8574.digitalWrite(P6, HIGH);
  delay(300);
  pcf8574.digitalWrite(P7, HIGH);
  delay(300);*/

	pcf8574.digitalWrite(P0, LOW);
  delay(300);
 /* pcf8574.digitalWrite(P1, LOW);
  delay(300);
  pcf8574.digitalWrite(P2, LOW);
  delay(300);
  pcf8574.digitalWrite(P3, LOW);
  delay(300);
  pcf8574.digitalWrite(P4, LOW);
  delay(300);
  pcf8574.digitalWrite(P5, LOW);
  delay(300);
  pcf8574.digitalWrite(P6, LOW);
  delay(300);
  pcf8574.digitalWrite(P7, LOW);
  delay(300);*/
  
	
}
