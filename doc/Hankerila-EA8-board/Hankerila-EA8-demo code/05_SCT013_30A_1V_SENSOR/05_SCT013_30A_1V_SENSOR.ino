//hankerila official store----www.aliexpress.com/store/1103673574
/*HANKERILA HKL-EA8*/
/*INSTALL ADS1X15 library*/
/*A0-A1 measure SCT013 SENSOR
  A2 measure 0-5V sensor
  A3 measure 0-20ma sensor*/
#include "ADS1X15.h"
ADS1115 ADS(0x48);
//const float shuntResistor = 200.0; 
// Use SCT013 30A/1V 
const float SCT013_VOLTAGE_PER_AMPERE = 1.0 / 30.0;
void setup() 
{
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.println("Welcome Use HANKERILA EA8");

  Wire.begin(4,5);
  ADS.begin();
}


void loop() 
{
  ADS.setGain(0);  //Set gain  Default MaxVoltage 6.14V
  // Read the SCT013 sensor connection channel (A0-A1 differential mode)
  int16_t diffVal = ADS.readADC_Differential_0_1();  
 // int16_t val_3 = ADS.readADC(2);  // measure PIN A2 0-5v
 // int16_t val_4 = ADS.readADC(3);  // measure PIN A3 0-20ma
  

  float f = ADS.toVoltage(1);  //  voltage factor MaxVoltage/32767
  float g= ADS.getMaxVoltage();
  // Converts the read digital value to the voltage value
  float voltage = ADS.toVoltage(diffVal);
  // Convert voltage value to current value according to SCT013 parameter
  float current = voltage / SCT013_VOLTAGE_PER_AMPERE;

  //float  Voltage_A2 = (float)val_3 *f;

  //float  Voltage_A3 = (float)val_4 *f;
 // float CurrentA3 = Voltage_A3 / shuntResistor * 1000; 

  if(voltage >0){
    Serial.print("SCT013 Voltage: "); Serial.print(voltage, 3); Serial.println(" V");
    Serial.print("SCT013 Current: "); Serial.print(current, 3); Serial.println(" A");
  }
  /*
//A2 measure 0-5V
 if(val_3 >=0){
    Serial.print("Analog01: "); 
    Serial.print(val_3);
    Serial.print("\t");
    Serial.print(Voltage_A2); 
    Serial.println("V");
  }
  //A3 measure 0-20mA
 if(val_4 >=0){
    Serial.print("Analog02: "); 
    Serial.print(val_4);
    Serial.print("\t");
    Serial.print(CurrentA3); 
    Serial.println("mA");
  } */
  Serial.println();
  delay(500);
}


