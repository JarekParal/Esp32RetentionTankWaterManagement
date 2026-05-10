//hankerila official store----www.aliexpress.com/store/1103673574
/*EA8 code of RS485 */
void setup() {

  Serial1.begin(115200,SERIAL_8N1,14,13);  //  IO14   485RX   IO13 485TX
  Serial1.println("WELCOME TO USE HKL-EA8"); // RS485 port output the string

}

void loop() {

  /*if Serial1(RS485) receive data ,print it*/
  while(Serial1.available()>0)
   {
    Serial1.print((char)Serial1.read());//Read rs485 receive data  and print it
   }
  delay(200);
}
