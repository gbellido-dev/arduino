// this example is public domain. enjoy!
// https://learn.adafruit.com/thermocouple/

#include "max6675.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>


#define DHTPIN 11
#define DHTTYPE    DHT22


int thermoDO_d0 = 10; //azul
int thermoCS_d0 = 9; //verde
int thermoCLK_d0 = 8; //amarillo*/

int thermoDO_d1 = 5; //azul
int thermoCS_d1 = 4; //verde
int thermoCLK_d1 = 6; //amarillo

MAX6675 thermocouple_d0(thermoCLK_d0, thermoCS_d0, thermoDO_d0);
MAX6675 thermocouple_d1(thermoCLK_d1, thermoCS_d1, thermoDO_d1);

DHT_Unified dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();
  delay(500);
}

void loop() {
  // basic readout test, just print the current temp

    sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    //Serial.print(F("Temperature: "));
    //Serial.print(event.temperature);
    //Serial.println(F("°C"));
  }
  
   Serial.print(event.temperature);
   Serial.print(","); 
   Serial.print(thermocouple_d0.readCelsius());
   Serial.print(","); 
   Serial.println(thermocouple_d1.readCelsius());
   // For the MAX6675 to update, you must delay AT LEAST 250ms between reads!



   delay(1000);
}
