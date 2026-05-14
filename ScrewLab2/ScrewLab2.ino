// ================== Intervalo ==================
const unsigned long PUBLISH_INTERVAL_MS = 1000;

// ================== ADC ==================
const float ADC_FULL_SCALE_V = 10.0f;   // 0–10 V
const float ADC_COUNTS      = 4095.0f;  // 12 bits

unsigned long lastPub = 0;


// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(AI0_0, INPUT);
  pinMode(AI0_1, INPUT);

}

// ================== LOOP ==================
void loop() {

  // Publicación periódica
  unsigned long now = millis();
  if (now - lastPub >= PUBLISH_INTERVAL_MS) {
    lastPub = now;

    // Lecturas (asumiendo AI0_0 y AI0_1 son pines analógicos válidos)
    int raw0 = analogRead(AI0_0);
    int raw1 = analogRead(AI0_1);
    float voltage0 = (raw0 / ADC_COUNTS) * ADC_FULL_SCALE_V;
    float voltage1 = (raw1 / ADC_COUNTS) * ADC_FULL_SCALE_V;

    // JSON plano
    char payload[120];
    snprintf(payload, sizeof(payload),
             "{\"voltage0\":%.5f,\"voltage1\":%.5f}",
             voltage0, voltage1);

    Serial.print("@MSG:");
    Serial.println(payload);
  }
}
