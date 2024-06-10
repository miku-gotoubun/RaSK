#include <SoftwareSerial.h>
SoftwareSerial mySerial(16,17); //tweliteのUARTピン番号RXTX
int i=0;

void setup() {
  // put your setup code here, to run once:
  mySerial.begin(115200);
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(i);
  mySerial.println(i);
  i++;

}
