//hankerila official store----www.aliexpress.com/store/1103673574
/*EA8 infrared emission program, cyclic transmission of fixed infrared signals*/
#include <IRremote.h>
IRsend irsend_32(32);      // set ir_send pin IO32
uint8_t sRepeats = 0;

void setup() {

  Serial.begin(9600);
  pinMode(0,INPUT);
 
  IrSender.begin(32); // Specify send pin and enable feedback LED at default feedback LED pin
  IrSender.enableIROut(38);
}

void loop() {

        if(digitalRead(0)==LOW){
        irsend_32.sendNECRaw(0xF807FF00, sRepeats);   //  0xF807FF00  is the raw_code  
        delay(1000);
        irsend_32.sendNECRaw(0xF906FF00, sRepeats);  //   0xF906FF00 is the raw_code  
        delay(1000);
        Serial.println("IR SEND IS DONE");
  }

        
}
