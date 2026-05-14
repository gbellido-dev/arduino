// ====== CONFIG ======
#define PIN_OUT 11
//#define OUTPUT_ACTIVE_HIGH   // descomenta si tu SSR/relé se activa con HIGH

const unsigned long PERIOD_MS = 5000;  // 5 segundos por ciclo
const float DUTY_PCT = 30.0;           // % de tiempo encendido

// ====== RUNTIME ======
unsigned long cycleStart = 0;
unsigned long onTime_ms = (unsigned long)(PERIOD_MS * (DUTY_PCT / 100.0f));

inline void writeOutput(bool on) {
#ifdef OUTPUT_ACTIVE_HIGH
  digitalWrite(PIN_OUT, on ? HIGH : LOW);
#else
  digitalWrite(PIN_OUT, on ? HIGH : LOW); // ajusta si tu hardware es invertido
#endif
}

void setup() {
  pinMode(PIN_OUT, OUTPUT);
  writeOutput(false);
  cycleStart = millis();
}

void loop() {
  unsigned long now = millis();
  unsigned long elapsed = now - cycleStart;
  if (elapsed >= PERIOD_MS) {
    cycleStart = now;
    elapsed = 0;
  }
  writeOutput(elapsed < onTime_ms);
}
