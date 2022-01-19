/*****************************************************/
/*******   VARIABLE AND FUNCTION DEFINITION   ********/
/*****************************************************/

#define LINE_BUF_SIZE 64   //Maximum input string length
#define ARG_BUF_SIZE 16     //Maximum argument string length
#define MAX_NUM_ARGS 8      //Maximum number of arguments
 
int LEDpin = LED_BUILTIN;
int blink_cycles = 10;      //How many times the LED will blink
bool error_flag = false;
 
char line[LINE_BUF_SIZE];
char args[MAX_NUM_ARGS][ARG_BUF_SIZE];

 
//Function declarations
int cmd_help();
int cmd_led();
int cmd_math();
int cmd_exit();

void cli_init();
void my_cli();

void read_line();
void parse_line();
int execute();

void help_help();
void help_led();
void help_math();
void help_exit();

 
//List of command names
const char *commands_str[] = {
    "help",
    "led",
	"math",
    "exit"
};
 
//List of LED sub command names
const char *led_args[] = {
    "on",
    "off",
    "blink"
};

//List of MATH sub command names
const char *math_args[] = {
    "+",
    "-",
    "*",
	"/"
};
 
int num_commands = sizeof(commands_str) / sizeof(char *);




/*****************************************************/
/***********   ARDUINO MINIMAL FUNCTIONS  ************/
/*****************************************************/
 
void setup() {
    Serial.begin(115200);
    pinMode(LEDpin, OUTPUT);
 
    cli_init();
}
 
void loop() {
    my_cli();
}


/*****************************************************/
/***************   MENU TOP FUNCTIONS  ***************/
/*****************************************************/


void cli_init(){
    Serial.println("Welcome to this simple Arduino command line interface (CLI).");
    Serial.println("Type \"help\" to see a list of commands.");
}
 
 
void my_cli(){
    Serial.print("> ");
 
    read_line();
    if(!error_flag){
        parse_line();
    }
    if(!error_flag){
        execute();
    }
 
    memset(line, 0, LINE_BUF_SIZE);
    memset(args, 0, sizeof(args[0][0]) * MAX_NUM_ARGS * ARG_BUF_SIZE);
 
    error_flag = false;
}


void read_line(){
    String line_string;
 
    while(!Serial.available());
 
    if(Serial.available()){
        line_string = Serial.readStringUntil('\n');
        if(line_string.length() < LINE_BUF_SIZE){
          line_string.toCharArray(line, LINE_BUF_SIZE);
          Serial.println(line_string);
        }
        else{
          Serial.println("Input string too long.");
          error_flag = true;
        }
    }
}
 
void parse_line(){
    char *argument;
    int counter = 0;
 
    argument = strtok(line, " ");
 
    while((argument != NULL)){
        if(counter < MAX_NUM_ARGS){
            if(strlen(argument) < ARG_BUF_SIZE){
                strcpy(args[counter],argument);
                argument = strtok(NULL, " ");
                counter++;
            }
            else{
                Serial.println("Input string too long.");
                error_flag = true;
                break;
            }
        }
        else{
            break;
        }
    }
}
 
int execute(){  

  if(strcmp(args[0], commands_str[0]) == 0){
    return cmd_help();
  }else if(strcmp(args[0], commands_str[1]) == 0){
    return cmd_led();
  }else if(strcmp(args[0], commands_str[2]) == 0){
    return cmd_math();
  }else if(strcmp(args[0], commands_str[3]) == 0){
    return cmd_exit();
  }
 
    Serial.println("Invalid command. Type \"help\" for more.");
    return 0;
}



/*****************************************************/
/****************   HELP CMD FUNCTIONS  **************/
/*****************************************************/

int cmd_help(){
    if(args[1] == NULL){
        help_help();
    }
    else if(strcmp(args[1], commands_str[0]) == 0){
        help_help();
    }
    else if(strcmp(args[1], commands_str[1]) == 0){
        help_led();
    }
    else if(strcmp(args[1], commands_str[2]) == 0){
        help_math();
    }    
    else if(strcmp(args[1], commands_str[3]) == 0){
        help_exit();
    }
    else{
        help_help();
    }
}
 
void help_help(){
    Serial.println("The following commands are available:");
 
    for(int i=0; i<num_commands; i++){
        Serial.print("  ");
        Serial.println(commands_str[i]);
    }
    Serial.println("");
    Serial.println("You can for instance type \"help led\" for more info on the LED command.");
}
 
void help_led(){
    Serial.print("Control the on-board LED, either on, off or blinking ");
    Serial.print(blink_cycles);
    Serial.print(" times:");
    Serial.println("  led on");
    Serial.println("  led off");
    Serial.println("  led blink hz");
    Serial.println("    where \"hz\" is the blink frequency in Hz.");
}

void help_math(){
    Serial.print("It makes calculos like a calculator (+,-,*,/)");
    Serial.println(" Samples:");
    Serial.println("  math 3 + 2");
    Serial.println("  math 3 - 2");
	  Serial.println("  math 3 * 2");
	  Serial.println("  math 3 / 2");
}
 
void help_exit(){
    Serial.println("This will exit the CLI. To restart the CLI, restart the program.");
}



/*****************************************************/
/****************   LED CMD FUNCTIONS  ***************/
/*****************************************************/
int cmd_led(){
    if(strcmp(args[1], led_args[0]) == 0){
        Serial.println("Turning on the LED.");
        digitalWrite(LEDpin, HIGH);
    }
    else if(strcmp(args[1], led_args[1]) == 0){
        Serial.println("Turning off the LED.");
        digitalWrite(LEDpin, LOW);
    }
    else if(strcmp(args[1], led_args[2]) == 0){
        if(atoi(args[2]) > 0){
            Serial.print("Blinking the LED ");
            Serial.print(blink_cycles);
            Serial.print(" times at ");
            Serial.print(args[2]);
            Serial.println(" Hz.");
             
            int delay_ms = (int)round(1000.0/atoi(args[2])/2);
             
            for(int i=0; i<blink_cycles; i++){
                digitalWrite(LEDpin, HIGH);
                delay(delay_ms);
                digitalWrite(LEDpin, LOW);
                delay(delay_ms);
            }
        }
        else{
            Serial.println("Invalid frequency.");
        }
    }
    else{
        Serial.println("Invalid command. Type \"help led\" to see how to use the LED command.");
    }
}


/*****************************************************/
/****************   MATH CMD FUNCTIONS  ***************/
/*****************************************************/
int cmd_math(){
	bool valid = true;
	int result = 0;
	
	Serial.print("Calculating ");
	Serial.print(args[1]);
	Serial.print(" ");
	Serial.print(args[2]);
	Serial.print(" ");
	Serial.print(args[3]);
	Serial.print(" = ");
	
    if(strcmp(args[2], math_args[0]) == 0){ //+
		  result = atoi(args[1]) + atoi(args[3]);
    }
    else if(strcmp(args[2], math_args[1]) == 0){ //-
		  result = atoi(args[1]) - atoi(args[3]);
    }
    else if(strcmp(args[2], math_args[2]) == 0){ //*
		  result = atoi(args[1]) * atoi(args[3]);
    }
	else if(strcmp(args[2], math_args[3]) == 0){ //*
		  result = atoi(args[1]) / atoi(args[3]);
    }
    else{
		Serial.println("ERROR");
        Serial.println("Invalid command. Type \"help math\" to see how to use the LED command.");
		valid = false;
    }
	
	if(valid){
		delay(500);
		Serial.println(result);
	}
}
 
 
/*****************************************************/
/****************   EXIT CMD FUNCTIONS  **************/
/*****************************************************/
int cmd_exit(){
    Serial.println("Exiting CLI.");
 
    while(1);
}
