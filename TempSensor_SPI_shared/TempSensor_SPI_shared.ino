// this example is public domain. enjoy!
// https://learn.adafruit.com/thermocouple/

#include "max6675.h"

int thermoDO_d0 = 12; //azul
int thermoCS_d0 = 10; //verde
int thermoCLK_d0 = 13; //amarillo*/


int thermoDO_d1 = 12; //azul
int thermoCS_d1 = 9; //verde
int thermoCLK_d1 = 13; //amarillo

MAX6675 thermocouple_d0(thermoCLK_d0, thermoCS_d0, thermoDO_d0);
MAX6675 thermocouple_d1(thermoCLK_d1, thermoCS_d1, thermoDO_d1);

void setup() {
  Serial.begin(9600);
  delay(500);
}

void loop() {
  // basic readout test, just print the current temp
  
   Serial.print("C (d0)= "); 
   Serial.println(thermocouple_d0.readCelsius());
   Serial.print("C (d1)= "); 
   Serial.println(thermocouple_d1.readCelsius());
   // For the MAX6675 to update, you must delay AT LEAST 250ms between reads!
   delay(1000);
}
