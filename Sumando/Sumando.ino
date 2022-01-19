void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.flush();
}

void loop() {
  // put your main code here, to run repeatedly:

  char c;
  char d;
  char a;

  while (Serial.available() == 0);
  c = Serial.read();
  Serial.println(c);
  
  while (Serial.available() == 0);
  d = Serial.read();
  Serial.println(d);
  a = c + d;

  Serial.println(a);

}
