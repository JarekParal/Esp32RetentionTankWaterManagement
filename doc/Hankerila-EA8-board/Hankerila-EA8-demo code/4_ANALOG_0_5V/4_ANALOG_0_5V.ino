//hankerila official store----www.aliexpress.com/store/1103673574
// EA8 measure analog input use pin A2 A3
/*install  ADS1X15 library*/
#include "ADS1X15.h"
ADS1115 ADS(0x48);
const float shuntResistor = 200.0; 

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

  int16_t val_1 = ADS.readADC(2);  // measure PIN A2
  int16_t val_2 = ADS.readADC(3);  // measure PIN A3
  float g= ADS.getMaxVoltage();
  Serial.print("g="); Serial.println(g); 

  float f = ADS.toVoltage(1);  //  voltage factor MaxVoltage/32767
  Serial.print("f="); Serial.println(f,5); 
  float  Voltage_A1 = (float)val_1 *f;
  float  Voltage_A2 = (float)val_2 *f;
  float CurrentA2 = Voltage_A2 / shuntResistor * 1000; 

  if(val_1 >0){
    Serial.print("Analog01: "); 
    Serial.print(val_1);
    Serial.print("\t");
    Serial.print(Voltage_A1, 2); 
    Serial.println("V");
    Serial.println();
  }
  if(val_2 >0){
    Serial.print("Analog02: "); 
    Serial.print(val_2);
    Serial.print("\t");
    Serial.print(CurrentA2, 2); 
    Serial.println("mA");
    Serial.println();
  }

  delay(1000);
}


