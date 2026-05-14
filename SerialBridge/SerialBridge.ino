void setup() {
  Serial.begin(115200);          // Comunicación con el PC
  Serial1.begin(38400, SERIAL_8N1, 18, 19); // Comunicación con MKS SERVO42C (RX1, TX1)
}

void loop() {
  // Si recibimos algo desde el PC (Serial0), lo enviamos a Serial1
  if (Serial.available()) {
    byte incoming = Serial.read();
    Serial1.write(incoming);
  }

  // Si recibimos algo desde Serial1 (MKS), lo enviamos al PC (Serial0)
  if (Serial1.available()) {
    byte incoming = Serial1.read();
    Serial.write(incoming);
  }
}
