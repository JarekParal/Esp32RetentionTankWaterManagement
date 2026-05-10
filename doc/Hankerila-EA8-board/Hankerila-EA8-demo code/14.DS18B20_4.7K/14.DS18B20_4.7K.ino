/*EA8 DS18B20 CODE*/
/*A 4.7K-10K resistor needs to be connected between 5V and DATA*/
#include <DS18B20.h>
DS18B20 ds1(15);  //channel-1-DS18b20

DS18B20 ds2(16);  //channel-2-DS18b20

void setup()
{
  Serial.begin(115200);
  Serial.println(__FILE__);
}
/*DS18B20 temperature detect*/
void loop()
{
  Serial.print("Temperature1:");
  Serial.print(ds1.getTempC());
  Serial.print("℃/");
  delay(100); 
  Serial.print("Temperature2:");
  Serial.print(ds2.getTempC());
  Serial.println(" ℃/\n");
  delay(100);  
} 