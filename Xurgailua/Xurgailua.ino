#include "MGEPLib.h"


#define KONPRESOREA_PIN CONTROLLINO_D6
#define XURGAILUA_PIN CONTROLLINO_D7

#define X_ZERO_PIN CONTROLLINO_A4
#define Z_ZERO_PIN CONTROLLINO_A3
#define W_ZERO_PIN CONTROLLINO_A5

#define ENCODER_X_A CONTROLLINO_A8
#define ENCODER_X_B CONTROLLINO_A9

#define ENCODER_W_A CONTROLLINO_A8
#define ENCODER_W_B CONTROLLINO_A9

#define ENCODER_Z_A CONTROLLINO_A8
#define ENCODER_Z_B CONTROLLINO_A9

StateMachine automata = CreateNewMachine();

Timer timerS1 = Timer(5000);

Encoder encoderX = Encoder(ENCODER_X_A, ENCODER_X_B, 0);
Encoder encoderW = Encoder(ENCODER_W_A, ENCODER_W_B, 0);
Encoder encoderZ = Encoder(ENCODER_Z_A, ENCODER_Z_B, 40);

EndStop finalX = EndStop(X_ZERO_PIN, END_STOP_MODE_INVERTED);
EndStop finalW = EndStop(W_ZERO_PIN, END_STOP_MODE_INVERTED);
EndStop finalZ = EndStop(Z_ZERO_PIN, END_STOP_MODE_INVERTED);

Axis ejeX = Axis(CONTROLLINO_D2, CONTROLLINO_D3);
Axis ejeW = Axis(CONTROLLINO_D4, CONTROLLINO_D5);
Axis ejeZ = Axis(CONTROLLINO_D0, CONTROLLINO_D1);
Axis ejeCompresor = Axis(KONPRESOREA_PIN);
Axis ejeValvula = Axis(XURGAILUA_PIN);

int x = -1, z = -1, w = -1;

unsigned long lastMillis = 0;


void initialize(){

  pinMode(CONTROLLINO_D0, OUTPUT);
  pinMode(CONTROLLINO_D1, OUTPUT);
  pinMode(CONTROLLINO_D2, OUTPUT);
  pinMode(CONTROLLINO_D3, OUTPUT);
  pinMode(CONTROLLINO_D4, OUTPUT);
  pinMode(CONTROLLINO_D5, OUTPUT);
  pinMode(CONTROLLINO_D6, OUTPUT);
  pinMode(CONTROLLINO_D7, OUTPUT);

  pinMode(X_ZERO_PIN, INPUT);
  pinMode(Z_ZERO_PIN, INPUT);
  pinMode(W_ZERO_PIN, INPUT);

  pinMode(ENCODER_Z_A, INPUT);
  pinMode(ENCODER_Z_B, INPUT);

  ejeX.move(AXIS_NONE);
  ejeW.move(AXIS_NONE);
  ejeZ.move(AXIS_NONE);
  ejeCompresor.move(AXIS_OFF);
  ejeValvula.move(AXIS_OFF);


  /*digitalWrite(CONTROLLINO_D0, LOW);
  digitalWrite(CONTROLLINO_D1, LOW);
  digitalWrite(CONTROLLINO_D2, LOW);
  digitalWrite(CONTROLLINO_D3, LOW);
  digitalWrite(CONTROLLINO_D4, LOW);
  digitalWrite(CONTROLLINO_D5, LOW);
  digitalWrite(CONTROLLINO_D6, LOW);
  digitalWrite(CONTROLLINO_D7, LOW);*/

  State* S1 = automata.addState(state1);
  State* S2 = automata.addState(state2);
  State* S3 = automata.addState(state3);

  S1->addTransition(transitionS1S2, S2);
  S2->addTransition(transitionS2S3, S3);
  S3->addTransition(transitionS3S1, S1);
}




void execute(){
  /*x = digitalRead(X_ZERO_PIN);
  z = digitalRead(Z_ZERO_PIN);
  w = digitalRead(W_ZERO_PIN);*/
  finalX.update();
  finalW.update();
  finalZ.update();

  if(finalX.getValue() == 0){
    encoderX.reset();
  }
  
  if(finalW.getValue() == 0){
    encoderW.reset();
  }
  
  if(finalZ.getValue() == 0){
    encoderZ.reset();
  }

  //updateEncoderZ();
  encoderX.update();
  encoderW.update();
  encoderZ.update();


  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis >= 2050) {
    lastMillis = currentMillis;
    
    Serial.println("Positions");
    Serial.print("| X: ");Serial.println(encoderX.getCount());
    Serial.print("| W: ");Serial.println(encoderW.getCount());
    Serial.print("| Z: ");Serial.println(encoderZ.getCount());
  }
}


void state1(){
  if(machine.executeOnce){
    Serial.println("State 1");
    /*moveX(X_BACKWARD);
    moveZ(Z_UP);
    moveW(W_RIGHT);
    konpresorea(STOP);
    */
    ejeX.move(AXIS_BACKWARD);
    ejeZ.move(AXIS_UP);
    ejeW.move(AXIS_RIGHT);
    
  }

  if(x == 0){
    //moveX(STOP);
    ejeX.move(AXIS_NONE);
  }

  if(z == 0){
    //moveZ(STOP);
    ejeZ.move(AXIS_NONE);
  }

  if(w == 0){
    //moveW(STOP);
    ejeW.move(AXIS_NONE);
  }

}

bool transitionS1S2(){
    if(x == 0 && z == 0 && w == 0){
      return true;
    }else{
      return false;
    }
}


void state2(){
  if(machine.executeOnce){
    Serial.println("State 2");
    //timerS1.start();
    
    //moveX(X_FORWARD);
    /*xurgatu(STOP);
    konpresorea(START);
    moveZ(Z_DOWN);*/
    //moveW(W_LEFT);
    ejeValvula.move(AXIS_OFF);
    ejeCompresor.move(AXIS_ON);
    ejeZ.move(AXIS_DOWN);
  }
}

bool transitionS2S3(){
  //Serial.print("->>>>");Serial.println(position);
  return encoderZ.countGreaterThan(2800);
}

void state3(){
  if(machine.executeOnce){
    Serial.println("State 3");
    //moveZ(STOP);
    ejeZ.move(AXIS_NONE);
    //xurgatu(START);
    ejeValvula.move(AXIS_ON);
    timerS1.start();
    
    //moveX(X_FORWARD);
    
    //moveW(W_LEFT);
  }
}

bool transitionS3S1(){
  return timerS1.done();
}


