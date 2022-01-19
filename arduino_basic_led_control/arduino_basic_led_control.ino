/*
https://lowvoltage.github.io/2017/07/09/Onboard-LEDs-NodeMCU-Got-Two
*/


/*
 * Ariketa:
 * -ESP012 bi LED ekartzen ditu (LED_BUILTIN eta ???). Ikertu ze PIN konfiguratu behar da bigarren LED hori pizteko.
 */

#define LED_PIN LED_BUILTIN
#define LED_PIN1 12

unsigned long previousMillis = 0; 
const long interval = 100;
unsigned long previousMillis1 = 0; 
const long interval1 = 100;

void setup() {
  pinMode(LED_PIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(LED_PIN1, OUTPUT);
}

void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }

  if (currentMillis - previousMillis1 >= interval1) {
    // save the last time you blinked the LED
    previousMillis1 = currentMillis;
    digitalWrite(LED_PIN1, !digitalRead(LED_PIN1));
  }
  
  /*digitalWrite(LED_PIN, LOW);   // Turn the LED on by making the voltage LOW
  delay(1000);                      // Wait for a second
  digitalWrite(LED_PIN, HIGH);  // Turn the LED off by making the voltage HIGH
  delay(2000);                   // Wait for two seconds
  */
}
