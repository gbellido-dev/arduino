// Include the Servo library 
#include <Servo.h> 
#include <Esp.h>
int deg = 0;
// Declare the Servo pin 
int servoPin = D5; 
// Create a servo object 
Servo Servo1; 
void setup() { 
   ESP.wdtDisable();
   // We need to attach the servo to the used pin number 
   Servo1.attach(servoPin); 
   //ESP.wdtEnable(1000);
}
void loop(){ 
   // Make servo go to 0 degrees 
   Servo1.write(deg);
   for(int i=0;i<20;i++){
      yield();
      delay(100);
      //ESP.wdtFeed();
   }
   deg++; if(deg == 180) deg = 0;
}
