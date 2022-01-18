void setup(){
  Serial.begin(115200);
}
  
void loop(){
	double result = 0;
  char str[30];
  
	for(int m=0; m < 1000000; m+=10000){
		Serial.print("Calculating pi number with m terms(");
		Serial.print(m);
		Serial.print(") = ");
		result = pi(m); //calculating pi number with m terms!!
    sprintf(str, "%.6f", result);
    Serial.println(str);
	}
	
}

double pi(int m){
  double result=4;
  for (int x=1; x<m; x++){
    result*=(2*x);
    result/=(2*x+1);
    result*=(2*x+2);
    result/=(2*x+1);
    yield();
  }
  return result;
}
