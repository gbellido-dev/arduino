#include "HX711.h"

HX711 balanza;

// Pines HX711: DT, SCK
#define DT 12
#define SCK 13

void setup() {
  Serial.begin(9600);
  balanza.begin(DT, SCK);
  balanza.set_scale(2280.f);  // <-- este número lo debes calibrar
  balanza.tare();             // pone a cero la balanza
}

void loop() {
  Serial.print("Peso: ");
  Serial.print(balanza.get_units(), 2);  // lee y convierte a "unidades"
  Serial.println(" kg");                // o la unidad que definas
  delay(500);
}
