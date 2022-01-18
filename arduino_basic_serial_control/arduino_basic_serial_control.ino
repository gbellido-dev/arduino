int a;
int b;
void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);
}

void loop() {
	Serial.println("Enter the First number");
	while(!Serial.available()); // wait till the user has entered something
	a = Serial.parseInt(); // treat what the user has entered as an Integer and read the whole number
	
	Serial.println("Enter the second number");
	while(!Serial.available());
	b = Serial.parseInt();
	
	Serial.println(a+b); // Print the sum
}
