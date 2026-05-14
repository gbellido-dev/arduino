#include <Servo.h>
Servo esc;

const int ESC_PIN  = 4;

// PWM (ajusta si tu ESC usa otros)
const int THR_NEU  = 1500;   // neutro
const int THR_FWD  = THR_NEU + 100;   // tu mínimo adelante encontrado
const int THR_BRAKE= THR_NEU - 100;   // “toque” de freno para habilitar reversa
const int THR_REV  = THR_NEU - 150;   // reversa (suave). Baja más si necesitas más fuerza

// tiempos (ms)
const int T_FORWARD   = 1000;  // 1 s adelante
const int T_STOP      = 3000;  // 1 s parado
const int T_REVERSE   = 1000;  // 1 s atrás
const int T_BRAKETAP  = 300;   // toque de freno
const int T_NEUTRALTAP= 300;   // pausa en neutro entre freno y reversa

void setup() {
  Serial.begin(115200);
  esc.attach(ESC_PIN, 1000, 2000);

  // Armar en neutro
  esc.writeMicroseconds(THR_NEU);
  delay(2000);
  Serial.println("ESC armado. Iniciando ciclo...");
}

void goReverse(int duration_ms) {
  // Secuencia doble toque: freno -> neutro -> reversa
  esc.writeMicroseconds(THR_BRAKE);  // toque de freno
  delay(T_BRAKETAP);

  esc.writeMicroseconds(THR_NEU);    // pausa en neutro
  delay(T_NEUTRALTAP);

  esc.writeMicroseconds(THR_REV);    // ahora sí, reversa real
  delay(duration_ms);
}

void loop() {
  // 1) Adelante 1 s
  Serial.println("Adelante");
  esc.writeMicroseconds(THR_FWD);
  delay(T_FORWARD);

  // 2) Parado 1 s
  Serial.println("Parado");
  esc.writeMicroseconds(THR_NEU);
  delay(T_STOP);

  // 3) Reversa 1 s (con doble toque)
  Serial.println("Reversa (con doble toque)");
  goReverse(T_REVERSE);

  // 4) Parado 1 s
  Serial.println("Parado");
  esc.writeMicroseconds(THR_NEU);
  delay(T_STOP);
}
