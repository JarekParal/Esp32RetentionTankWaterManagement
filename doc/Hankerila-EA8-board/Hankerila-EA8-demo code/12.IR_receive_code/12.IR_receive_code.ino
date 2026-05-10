//hankerila official store----www.aliexpress.com/store/1103673574
/*EA8 infrared receiver program*/
#include <IRremote.h>
IRrecv irrecv_33(33);// set ir_receive pin IO33

void setup() {

  Serial.begin(9600);
  irrecv_33.enableIRIn();
}

void loop() {
    if (irrecv_33.decode()) {
      if(irrecv_33.decodedIRData.decodedRawData!=(0)){
        Serial.print("irCode address: ");            
        Serial.println(irrecv_33.decodedIRData.address,HEX); 
        Serial.print("irCode command: ");            
        Serial.println(irrecv_33.decodedIRData.command,HEX);
        Serial.print("irCode decodedRawData: ");            
        Serial.println(irrecv_33.decodedIRData.decodedRawData,HEX);
      }
      IrReceiver.resume();
    }
}
