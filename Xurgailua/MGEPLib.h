/*****

  MGEPLib for CONTROLLINO MAXI

  Ver.: 1.0
  +StateMachine and Timers

  Ver.: 1.1
  +I2C LCD and HCSR04 sensor

  Ver.: 1.2
  +Encoder and axis

*****/

#ifndef HCSR04_h
#define HCSR04_h

class HCSR04
{
public:
	HCSR04(int out, int echo);			//initialisation class HCSR04 (trig pin , echo pin)
	HCSR04(int out, int echo[], int n); //initialisation class HCSR04 (trig pin , echo pin)
	~HCSR04();							//destructor
	float dist() const;					//return curent distance of element 0
	float dist(int n) const;			//return curent distance of element n

private:
	void init(int out, int echo[], int n); //for constructor
	int out;							   //out pin
	int *echo;							   //echo pin list
	int n;								   //number of el
};


void HCSR04::init(int out, int echo[], int n)
{
	this->out = out;
	this->echo = echo;
	this->n = n;
	pinMode(this->out, OUTPUT);
	for (int i = 0; i < n; i++)
		pinMode(this->echo[i], INPUT);
}
HCSR04::HCSR04(int out, int echo) { this->init(out, new int[1]{echo}, 1); }
HCSR04::HCSR04(int out, int echo[], int n) { this->init(out, echo, n); }
HCSR04::~HCSR04()
{
	~this->out;
	delete[] this->echo;
	~this->n;
}

///////////////////////////////////////////////////dist
float HCSR04::dist(int n) const
{
	digitalWrite(this->out, LOW);
	delayMicroseconds(2);
	digitalWrite(this->out, HIGH);
	delayMicroseconds(10);
	digitalWrite(this->out, LOW);
	noInterrupts();
	float d = pulseIn(this->echo[n], HIGH, 23529.4); // max sensor dist ~4m
	interrupts();
	return d / 58.8235;
}
float HCSR04::dist() const { return this->dist(0); }

#endif


#ifndef LiquidCrystal_I2C_h
#define LiquidCrystal_I2C_h

#include <inttypes.h>
#include "Print.h" 
#include <Wire.h>

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define En B00000100  // Enable bit
#define Rw B00000010  // Read/Write bit
#define Rs B00000001  // Register select bit

class LiquidCrystal_I2C : public Print {
public:
  LiquidCrystal_I2C(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows);
  void begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS );
  void clear();
  void home();
  void noDisplay();
  void display();
  void noBlink();
  void blink();
  void noCursor();
  void cursor();
  void scrollDisplayLeft();
  void scrollDisplayRight();
  void printLeft();
  void printRight();
  void leftToRight();
  void rightToLeft();
  void shiftIncrement();
  void shiftDecrement();
  void noBacklight();
  void backlight();
  void autoscroll();
  void noAutoscroll(); 
  void createChar(uint8_t, uint8_t[]);
  void createChar(uint8_t location, const char *charmap);
  // Example: 	const char bell[8] PROGMEM = {B00100,B01110,B01110,B01110,B11111,B00000,B00100,B00000};
  
  void setCursor(uint8_t, uint8_t); 
#if defined(ARDUINO) && ARDUINO >= 100
  virtual size_t write(uint8_t);
#else
  virtual void write(uint8_t);
#endif
  void command(uint8_t);
  void init();
  void oled_init();

////compatibility API function aliases
void blink_on();						// alias for blink()
void blink_off();       					// alias for noBlink()
void cursor_on();      	 					// alias for cursor()
void cursor_off();      					// alias for noCursor()
void setBacklight(uint8_t new_val);				// alias for backlight() and nobacklight()
void load_custom_character(uint8_t char_num, uint8_t *rows);	// alias for createChar()
void printstr(const char[]);

////Unsupported API functions (not implemented in this library)
uint8_t status();
void setContrast(uint8_t new_val);
uint8_t keypad();
void setDelay(int,int);
void on();
void off();
uint8_t init_bargraph(uint8_t graphtype);
void draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_col_end);
void draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_col_end);
	 

private:
  void init_priv();
  void send(uint8_t, uint8_t);
  void write4bits(uint8_t);
  void expanderWrite(uint8_t);
  void pulseEnable(uint8_t);
  uint8_t _Addr;
  uint8_t _displayfunction;
  uint8_t _displaycontrol;
  uint8_t _displaymode;
  uint8_t _numlines;
  bool _oled = false;
  uint8_t _cols;
  uint8_t _rows;
  uint8_t _backlightval;
};


#include <inttypes.h>
#if defined(ARDUINO) && ARDUINO >= 100

#include "Arduino.h"

#define printIIC(args)	Wire.write(args)
inline size_t LiquidCrystal_I2C::write(uint8_t value) {
	send(value, Rs);
	return 1;
}

#else
#include "WProgram.h"

#define printIIC(args)	Wire.send(args)
inline void LiquidCrystal_I2C::write(uint8_t value) {
	send(value, Rs);
}

#endif
#include "Wire.h"



// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

LiquidCrystal_I2C::LiquidCrystal_I2C(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows)
{
  _Addr = lcd_Addr;
  _cols = lcd_cols;
  _rows = lcd_rows;
  _backlightval = LCD_NOBACKLIGHT;
}

void LiquidCrystal_I2C::oled_init(){
  _oled = true;
	init_priv();
}

void LiquidCrystal_I2C::init(){
	init_priv();
}

void LiquidCrystal_I2C::init_priv()
{
	Wire.begin();
	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
	begin(_cols, _rows);  
}

void LiquidCrystal_I2C::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
	if (lines > 1) {
		_displayfunction |= LCD_2LINE;
	}
	_numlines = lines;

	// for some 1 line displays you can select a 10 pixel high font
	if ((dotsize != 0) && (lines == 1)) {
		_displayfunction |= LCD_5x10DOTS;
	}

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	delay(50); 
  
	// Now we pull both RS and R/W low to begin commands
	expanderWrite(_backlightval);	// reset expanderand turn backlight off (Bit 8 =1)
	delay(1000);

  	//put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46
	
	  // we start in 8bit mode, try to set 4 bit mode
   write4bits(0x03 << 4);
   delayMicroseconds(4500); // wait min 4.1ms
   
   // second try
   write4bits(0x03 << 4);
   delayMicroseconds(4500); // wait min 4.1ms
   
   // third go!
   write4bits(0x03 << 4); 
   delayMicroseconds(150);
   
   // finally, set to 4-bit interface
   write4bits(0x02 << 4); 


	// set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);  
	
	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();
	
	// clear it off
	clear();
	
	// Initialize to default text direction (for roman languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	
	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);
	
	home();
  
}

/********** high level commands, for the user! */
void LiquidCrystal_I2C::clear(){
	command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	delayMicroseconds(2000);  // this command takes a long time!
  if (_oled) setCursor(0,0);
}

void LiquidCrystal_I2C::home(){
	command(LCD_RETURNHOME);  // set cursor position to zero
	delayMicroseconds(2000);  // this command takes a long time!
}

void LiquidCrystal_I2C::setCursor(uint8_t col, uint8_t row){
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if ( row > _numlines ) {
		row = _numlines-1;    // we count rows starting w/0
	}
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void LiquidCrystal_I2C::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C::display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LiquidCrystal_I2C::noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C::cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LiquidCrystal_I2C::noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C::blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void LiquidCrystal_I2C::scrollDisplayLeft(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LiquidCrystal_I2C::scrollDisplayRight(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void LiquidCrystal_I2C::leftToRight(void) {
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void LiquidCrystal_I2C::rightToLeft(void) {
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void LiquidCrystal_I2C::autoscroll(void) {
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void LiquidCrystal_I2C::noAutoscroll(void) {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LiquidCrystal_I2C::createChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		write(charmap[i]);
	}
}

//createChar with PROGMEM input
void LiquidCrystal_I2C::createChar(uint8_t location, const char *charmap) {
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
	    	write(pgm_read_byte_near(charmap++));
	}
}

// Turn the (optional) backlight off/on
void LiquidCrystal_I2C::noBacklight(void) {
	_backlightval=LCD_NOBACKLIGHT;
	expanderWrite(0);
}

void LiquidCrystal_I2C::backlight(void) {
	_backlightval=LCD_BACKLIGHT;
	expanderWrite(0);
}



/*********** mid level commands, for sending data/cmds */

inline void LiquidCrystal_I2C::command(uint8_t value) {
	send(value, 0);
}


/************ low level data pushing commands **********/

// write either command or data
void LiquidCrystal_I2C::send(uint8_t value, uint8_t mode) {
	uint8_t highnib=value&0xf0;
	uint8_t lownib=(value<<4)&0xf0;
       write4bits((highnib)|mode);
	write4bits((lownib)|mode); 
}

void LiquidCrystal_I2C::write4bits(uint8_t value) {
	expanderWrite(value);
	pulseEnable(value);
}

void LiquidCrystal_I2C::expanderWrite(uint8_t _data){                                        
	Wire.beginTransmission(_Addr);
	printIIC((int)(_data) | _backlightval);
	Wire.endTransmission();   
}

void LiquidCrystal_I2C::pulseEnable(uint8_t _data){
	expanderWrite(_data | En);	// En high
	delayMicroseconds(1);		// enable pulse must be >450ns
	
	expanderWrite(_data & ~En);	// En low
	delayMicroseconds(50);		// commands need > 37us to settle
} 


// Alias functions

void LiquidCrystal_I2C::cursor_on(){
	cursor();
}

void LiquidCrystal_I2C::cursor_off(){
	noCursor();
}

void LiquidCrystal_I2C::blink_on(){
	blink();
}

void LiquidCrystal_I2C::blink_off(){
	noBlink();
}

void LiquidCrystal_I2C::load_custom_character(uint8_t char_num, uint8_t *rows){
		createChar(char_num, rows);
}

void LiquidCrystal_I2C::setBacklight(uint8_t new_val){
	if(new_val){
		backlight();		// turn backlight on
	}else{
		noBacklight();		// turn backlight off
	}
}

void LiquidCrystal_I2C::printstr(const char c[]){
	//This function is not identical to the function used for "real" I2C displays
	//it's here so the user sketch doesn't have to be changed 
	print(c);
}


// unsupported API functions
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void LiquidCrystal_I2C::off(){}
void LiquidCrystal_I2C::on(){}
void LiquidCrystal_I2C::setDelay (int cmdDelay,int charDelay) {}
uint8_t LiquidCrystal_I2C::status(){return 0;}
uint8_t LiquidCrystal_I2C::keypad (){return 0;}
uint8_t LiquidCrystal_I2C::init_bargraph(uint8_t graphtype){return 0;}
void LiquidCrystal_I2C::draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_col_end){}
void LiquidCrystal_I2C::draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_row_end){}
void LiquidCrystal_I2C::setContrast(uint8_t new_val){}
#pragma GCC diagnostic pop

#endif




#ifndef LinkedList_h
#define LinkedList_h

#include <stddef.h>

template<class T>
struct ListNode
{
	T data;
	ListNode<T> *next;
};

template <typename T>
class LinkedList{

protected:
	int _size;
	ListNode<T> *root;
	ListNode<T>	*last;

	// Helps "get" method, by saving last position
	ListNode<T> *lastNodeGot;
	int lastIndexGot;
	// isCached should be set to FALSE
	// everytime the list suffer changes
	bool isCached;

	ListNode<T>* getNode(int index);

	ListNode<T>* findEndOfSortedString(ListNode<T> *p, int (*cmp)(T &, T &));

public:
	LinkedList();
	LinkedList(int sizeIndex, T _t); //initiate list size and default value
	virtual ~LinkedList();

	/*
		Returns current size of LinkedList
	*/
	virtual int size();
	/*
		Adds a T object in the specified index;
		Unlink and link the LinkedList correcly;
		Increment _size
	*/
	virtual bool add(int index, T);
	/*
		Adds a T object in the end of the LinkedList;
		Increment _size;
	*/
	virtual bool add(T);
	/*
		Adds a T object in the start of the LinkedList;
		Increment _size;
	*/
	virtual bool unshift(T);
	/*
		Set the object at index, with T;
	*/
	virtual bool set(int index, T);
	/*
		Remove object at index;
		If index is not reachable, returns false;
		else, decrement _size
	*/
	virtual T remove(int index);
	/*
		Remove last object;
	*/
	virtual T pop();
	/*
		Remove first object;
	*/
	virtual T shift();
	/*
		Get the index'th element on the list;
		Return Element if accessible,
		else, return false;
	*/
	virtual T get(int index);

	/*
		Clear the entire array
	*/
	virtual void clear();

	/*
		Sort the list, given a comparison function
	*/
	virtual void sort(int (*cmp)(T &, T &));

		// add support to array brakets [] operator
	inline T& operator[](int index); 
	inline T& operator[](size_t& i) { return this->get(i); }
  	inline const T& operator[](const size_t& i) const { return this->get(i); }

};

// Initialize LinkedList with false values
template<typename T>
LinkedList<T>::LinkedList()
{
	root=NULL;
	last=NULL;
	_size=0;

	lastNodeGot = root;
	lastIndexGot = 0;
	isCached = false;
}

// Clear Nodes and free Memory
template<typename T>
LinkedList<T>::~LinkedList()
{
	ListNode<T>* tmp;
	while(root!=NULL)
	{
		tmp=root;
		root=root->next;
		delete tmp;
	}
	last = NULL;
	_size=0;
	isCached = false;
}

/*
	Actualy "logic" coding
*/

template<typename T>
ListNode<T>* LinkedList<T>::getNode(int index){

	int _pos = 0;
	ListNode<T>* current = root;

	// Check if the node trying to get is
	// immediatly AFTER the previous got one
	if(isCached && lastIndexGot <= index){
		_pos = lastIndexGot;
		current = lastNodeGot;
	}

	while(_pos < index && current){
		current = current->next;

		_pos++;
	}

	// Check if the object index got is the same as the required
	if(_pos == index){
		isCached = true;
		lastIndexGot = index;
		lastNodeGot = current;

		return current;
	}

	return NULL;
}

template<typename T>
int LinkedList<T>::size(){
	return _size;
}

template<typename T>
LinkedList<T>::LinkedList(int sizeIndex, T _t){
	for (int i = 0; i < sizeIndex; i++){
		add(_t);
	}
}

template<typename T>
bool LinkedList<T>::add(int index, T _t){

	if(index >= _size)
		return add(_t);

	if(index == 0)
		return unshift(_t);

	ListNode<T> *tmp = new ListNode<T>(),
				 *_prev = getNode(index-1);
	tmp->data = _t;
	tmp->next = _prev->next;
	_prev->next = tmp;

	_size++;
	isCached = false;

	return true;
}

template<typename T>
bool LinkedList<T>::add(T _t){

	ListNode<T> *tmp = new ListNode<T>();
	tmp->data = _t;
	tmp->next = NULL;
	
	if(root){
		// Already have elements inserted
		last->next = tmp;
		last = tmp;
	}else{
		// First element being inserted
		root = tmp;
		last = tmp;
	}

	_size++;
	isCached = false;

	return true;
}

template<typename T>
bool LinkedList<T>::unshift(T _t){

	if(_size == 0)
		return add(_t);

	ListNode<T> *tmp = new ListNode<T>();
	tmp->next = root;
	tmp->data = _t;
	root = tmp;
	
	_size++;
	isCached = false;
	
	return true;
}


template<typename T>
T& LinkedList<T>::operator[](int index) {
	return getNode(index)->data;
}

template<typename T>
bool LinkedList<T>::set(int index, T _t){
	// Check if index position is in bounds
	if(index < 0 || index >= _size)
		return false;

	getNode(index)->data = _t;
	return true;
}

template<typename T>
T LinkedList<T>::pop(){
	if(_size <= 0)
		return T();
	
	isCached = false;

	if(_size >= 2){
		ListNode<T> *tmp = getNode(_size - 2);
		T ret = tmp->next->data;
		delete(tmp->next);
		tmp->next = NULL;
		last = tmp;
		_size--;
		return ret;
	}else{
		// Only one element left on the list
		T ret = root->data;
		delete(root);
		root = NULL;
		last = NULL;
		_size = 0;
		return ret;
	}
}

template<typename T>
T LinkedList<T>::shift(){
	if(_size <= 0)
		return T();

	if(_size > 1){
		ListNode<T> *_next = root->next;
		T ret = root->data;
		delete(root);
		root = _next;
		_size --;
		isCached = false;

		return ret;
	}else{
		// Only one left, then pop()
		return pop();
	}

}

template<typename T>
T LinkedList<T>::remove(int index){
	if (index < 0 || index >= _size)
	{
		return T();
	}

	if(index == 0)
		return shift();
	
	if (index == _size-1)
	{
		return pop();
	}

	ListNode<T> *tmp = getNode(index - 1);
	ListNode<T> *toDelete = tmp->next;
	T ret = toDelete->data;
	tmp->next = tmp->next->next;
	delete(toDelete);
	_size--;
	isCached = false;
	return ret;
}


template<typename T>
T LinkedList<T>::get(int index){
	ListNode<T> *tmp = getNode(index);

	return (tmp ? tmp->data : T());
}

template<typename T>
void LinkedList<T>::clear(){
	while(size() > 0)
		shift();
}

template<typename T>
void LinkedList<T>::sort(int (*cmp)(T &, T &)){
	if(_size < 2) return; // trivial case;

	for(;;) {	

		ListNode<T> **joinPoint = &root;

		while(*joinPoint) {
			ListNode<T> *a = *joinPoint;
			ListNode<T> *a_end = findEndOfSortedString(a, cmp);
	
			if(!a_end->next	) {
				if(joinPoint == &root) {
					last = a_end;
					isCached = false;
					return;
				}
				else {
					break;
				}
			}

			ListNode<T> *b = a_end->next;
			ListNode<T> *b_end = findEndOfSortedString(b, cmp);

			ListNode<T> *tail = b_end->next;

			a_end->next = NULL;
			b_end->next = NULL;

			while(a && b) {
				if(cmp(a->data, b->data) <= 0) {
					*joinPoint = a;
					joinPoint = &a->next;
					a = a->next;	
				}
				else {
					*joinPoint = b;
					joinPoint = &b->next;
					b = b->next;	
				}
			}

			if(a) {
				*joinPoint = a;
				while(a->next) a = a->next;
				a->next = tail;
				joinPoint = &a->next;
			}
			else {
				*joinPoint = b;
				while(b->next) b = b->next;
				b->next = tail;
				joinPoint = &b->next;
			}
		}
	}
}

template<typename T>
ListNode<T>* LinkedList<T>::findEndOfSortedString(ListNode<T> *p, int (*cmp)(T &, T &)) {
	while(p->next && cmp(p->data, p->next->data) <= 0) {
		p = p->next;
	}
	
	return p;
}

#endif



#ifndef WebServer_h
#define WebServer_h


#include <Ethernet.h>

#define WS_REQUEST_BUF_LEN 25 // Adjust this to support longer URLs.
 
class WebServer;    // Forward declaration, needed for typedef below.

typedef void (*WsRequestHandler)(WebServer &w);
typedef enum {WS_TYPE_HTML, WS_TYPE_TEXT, WS_TYPE_JSON} WsContentType;

class WebServer {
  public:
    const char *verb = NULL;		 // A string that will contain the VERB (GET, POST)
    const char *url = NULL;          // The URL of the request, without any querystring.
	const char *querystring = NULL;  // The qyerystring, everything behind the questionmark.
	EthernetClient client;           // A reference to the EthernetClient object to work with.
	
	WebServer(EthernetClient &client);  
    ~WebServer();
    
	void serveUrl(const char* url, WsRequestHandler func, WsContentType contentType);
	void redirect(const char* url, const char* newurl);
	
  private:
    char _request[WS_REQUEST_BUF_LEN]; // A buffer to store the HTTP request.
    void disconnect();                  // Close the client connection.
	void throwError(const __FlashStringHelper *error); // HTTP error response.
};


WebServer::WebServer(EthernetClient &ec):client(ec){

  /* READ THE REQUEST */
  bool firstChar = true;         // Keep track of line beginnings to see the empty line marking end of header.
  int i = 0;                     // Number of characters read.
  char c = '\0';
  while (client.connected()) {   // While the TCP session is connected.
    if (client.available()) {    // Client data available to read?

      char c = client.read();    // Read 1 byte (character) from client
      if (i < WS_REQUEST_BUF_LEN) 
        _request[i] = c;         // Add the char to the global request buffer
      
      i++;                       // Increase the byte counter.

      if (c == '\n') {           // If this char is a end-of-line...
        if (firstChar) break;    // If it was the first char in a line, the line is empty. Break out.
        firstChar = true;        // Note that the next char will be the first in a line.
      }

      if (c != '\n' && c != '\r') // If the char is not a line ending character...
        firstChar = false;        // ...the next char is not going to be the first char of a line.

    } 
  } 
  
  /* PARSE THE REQUEST */
  int rlen = min(i, WS_REQUEST_BUF_LEN - 1); // Calculate the request length
  _request[rlen]='\0';			              // At least one string terminator in the buffer.
  verb = url = querystring = &_request[rlen]; // Reset all the strings to empty string.

  char *spc1=NULL, *spc2=NULL, *q=NULL;       // Initiate some marker variables.
  spc1 = strchr(_request,' ');                // Search for the space that separates the verb and the URL.
  if(spc1==NULL){
	  throwError(F("400 Bad Request"));
	  return;
  }
  verb = _request;                            
  spc1[0]='\0';								  // Terminate the verb string
  
  spc2 = strchr(spc1+1,' ');                  // Search for the space that after the URL
  if(spc2==NULL){
    throwError(F("414 Request-URI Too Long"));
	return;
  }
  url = spc1 + 1;             
  spc2[0] = '\0';                             // Terminate the URL string.
  
  q = strchr(spc1+1,'?');                     // Search for a question mark inside the URL
  if(q!=NULL){								  
	q[0]='\0';                                
	querystring = q + 1;
  }
  
  // Reject all HTTP verbs except GET 
  if (strcmp(verb, "GET")!=0) { 
    throwError(F("405 Method Not Allowed"));
    disconnect();
    return;
  }
}

void WebServer::serveUrl(const char* url, WsRequestHandler func, const WsContentType responseContentType=WS_TYPE_JSON){
  
  
  if(client){
	Serial.print("serveUrl");
	Serial.print(url);
  
    // Compare the url parameter with the url in the http request.
    if(strcmp(this->url,url)==0){ 

      // Write the response header
      client.println(F("HTTP/1.1 200 OK"));                 // Normal HTTP response.
      client.println(F("Connection: close"));               // Indicates to the client that the TCP connections will be closed and not reused.
      if(responseContentType==WS_TYPE_JSON)
        client.println(F("Content-Type: application/json; charset=utf-8"));// Content type is set to JSON
      else if(responseContentType==WS_TYPE_TEXT)
        client.println(F("Content-Type: text/plain; charset=utf-8"));      // Content type is set to plain text
      else
        client.println(F("Content-Type: text/html; charset=utf-8"));       // Content type is set to HTML (default)
      
      client.println();                                     // Empty line to indicate the end of the header.
  
      func(*this);         // Call the specified function and pass the current object as parameter.

      disconnect();          // Disconnect the TCP connection
    
    }
  }
}

void WebServer::redirect(const char* url, const char* newurl){
  if(client){

	// Compare the url parameter with the url in the http request.
    if(strcmp(this->url,url)==0){ 
	  client.println(F("HTTP/1.1 301 Moved Permanently"));  
	  client.println(F("Connection: close"));               
	  client.print(F("Location: "));
	  client.println(newurl);
	  client.println();
	  disconnect();
	}
  }
}

WebServer::~WebServer(){
  if(client){
	throwError(F("404 Not Found")); // If no URL has been served, throw a 404.
  }
}

void WebServer::disconnect(){
    client.flush(); // Flush the streams, make sure that the response has been delivered to the network.
    delay(1);       // Delay a little bit, shouldnt be needed if the flush() works.
    client.stop();  // Close the TCP Connection.
}

void WebServer::throwError(const __FlashStringHelper* error){
    client.print(F("HTTP/1.1 "));
	client.println(error);
    client.println(F("Connection: close"));                
    client.println(F("Content-Type: text/html"));
    client.println();
    client.print(F("<html><head><title>"));
	client.print(error);
	client.print(F("</title></head><body>"));
	client.print(error);
	client.print(F("</body></html>"));
    disconnect();
}

#endif


#ifndef TIMER_H
#define TIMER_H

#define TIMER_INDEFINITE -1
#define TIMER_UNLIMITED -1




class Timer{
	public:
	//Methods
	Timer();
	Timer(long _t);      //Constructor
	~Timer();            //Destructor
		
	void init();            //Initializations
	boolean done();         //Indicates time has elapsed
	boolean repeat(int times);
	boolean repeat(int times, long _t);
	boolean repeat();
	void repeatReset();
	boolean waiting();			// Indicates timer is started but not finished
	boolean started();			// Indicates timer has started
	void start();			//Starts a timer
	long stop();			//Stops a timer and returns elapsed time
	void restart();
	void reset();           //Resets timer to zero
	void set(long t);
	long get();
	boolean debounce(boolean signal);
	int repetitions = TIMER_UNLIMITED;
	
	private:

	typedef struct myTimer{
		long time;
		long last;
		boolean done;
		boolean started;
	};

	struct myTimer _timer;
	boolean _waiting;
};

//Default constructor
Timer::Timer(){
	this->_timer.time = 1000; //Default 1 second interval if not specified
}

Timer::Timer(long _t){
  this->_timer.time = _t;
}

//Default destructor
Timer::~Timer(){
  
}

//Initializations
void Timer::init(){
  this->_waiting = false;
}

/*
 * Repeats a timer x times
 * Useful to execute a task periodically.
 * Usage: 
 * if(timer.repeat(10)){
 * 	  do something 10 times, every second (default)
 * }
 */
boolean Timer::repeat(int times){
	if(times != TIMER_UNLIMITED){	
		// First repeat
		if(this->repetitions == TIMER_UNLIMITED){
			this->repetitions = times;			
		}
		// Stop
		if(this->repetitions == 0){
			return false;
		}
		
		if(this->repeat()){
			this->repetitions--;
			return true;
		}
		return false;
	}
	return this->repeat();
}

/*
 * Repeats a timer x times with a defined period
 * Useful to execute a task periodically.
 * Usage: 
 * if(timer.repeat(10,5000)){
 * 	  do something 10 times, every 5 seconds
 * }
 */
boolean Timer::repeat(int times, long _t){
	this->_timer.time = _t;
	return this->repeat(times);
}

/*
 * Repeats a timer indefinetely
 * Useful to execute a task periodically.
 * Usage: 
 * if(timer.repeat()){
 * 	  do something indefinetely, every second (default)
 * }
 */
boolean Timer::repeat(){
  if(this->done()){
    this->reset();
    return true;
  }  
	if(!this->_timer.started){
		this->_timer.last = millis();
		this->_timer.started = true;
    this->_waiting = true;
  }	
  return false;	
}

void Timer::repeatReset(){
	this->repetitions = -1;
}

/*
 * Checks if timer has finished
 * Returns true if it finished
 */
boolean Timer::done(){
  
  if(!this->_timer.started) return false;
  if( (millis()-this->_timer.last) >= this->_timer.time){
    this->_timer.done = true;
    this->_waiting = false;
    return true;
  }
  return false;
}

/*
 * Sets a timer preset
 */
void Timer::set(long t){
  this->_timer.time = t;
}

/*
 * Gets the timer preset
 */
long Timer::get(){
	return this->_timer.time;
}

/*
 * Returns the debounced value of signal
 * This is very useful to avoid "bouncing"
 * of electromechanical signals
 */
boolean Timer::debounce(boolean signal){
	if(this->done() && signal){
		this->start();
		return true;
	}
	return false;
}

/*
 * Resets a timer
 */
void Timer::reset(){
  this->stop();
  this->_timer.last = millis();
  this->_timer.done = false;
}

/*
 * Start a timer
 */
void Timer::start(){
	this->reset();
  this->_timer.started = true;
  this->_waiting = true;
}

/*
 * Stops a timer
 */
long Timer::stop(){
  this->_timer.started = false;
  this->_waiting = false;
  return millis()-this->_timer.last;
}

/*
 * Continues a stopped timer
 */
void Timer::restart(){
	if(!this->done()){
		this->_timer.started = true;
		this->_waiting = true;
	}
}

/*
 * Indicates if the timer is active
 * but has not yet finished.
 */
boolean Timer::waiting(){
  return (this->_timer.started && !this->done()) ? true : false;
}

boolean Timer::started(){
	return this->_timer.started;
}

#endif



#ifndef _STATE_H
#define _STATE_H

/*
 * Transition is a structure that holds the address of 
 * a function that evaluates whether or not not transition
 * from the current state and the number of the state to transition to
 */
struct Transition{
  bool (*conditionFunction)();
  int stateNumber;
};

/*
 * State represents a state in the statemachine. 
 * It consists mainly of the address of the function
 * that contains the state logic and a collection of transitions 
 * to other states.
 */
class State{
  public:
    State();
    ~State();

	void addTransition(bool (*c)(), State* s);
    void addTransition(bool (*c)(), int stateNumber);
    int evalTransitions();
    int execute();
    int setTransition(int index, int stateNumber);	//Can now dynamically set the transition
	
    // stateLogic is the pointer to the function
    // that represents the state logic
    void (*stateLogic)();
    LinkedList<struct Transition*> *transitions;
	int index;
};

State::State(){
  transitions = new LinkedList<struct Transition*>();
};

State::~State(){};

/*
 * Adds a transition structure to the list of transitions
 * for this state.
 * Params:
 * conditionFunction is the address of a function that will be evaluated
 * to determine if the transition occurs
 * state is the state to transition to
 */
void State::addTransition(bool (*conditionFunction)(), State* s){
  struct Transition* t = new Transition{conditionFunction,s->index};
  transitions->add(t);
}

/*
 * Adds a transition structure to the list of transitions
 * for this state.
 * Params:
 * conditionFunction is the address of a function that will be evaluated
 * to determine if the transition occurs
 * stateNumber is the number of the state to transition to
 */
void State::addTransition(bool (*conditionFunction)(), int stateNumber){
  struct Transition* t = new Transition{conditionFunction,stateNumber};
  transitions->add(t);
}

/*
 * Evals all transitions sequentially until one of them is true.
 * Returns:
 * The stateNumber of the transition that evaluates to true
 * -1 if none evaluate to true ===> Returning index now instead to avoid confusion between first run and no transitions
 */
int State::evalTransitions(){
  if(transitions->size() == 0) return index;
  bool result = false;
  
  for(int i=0;i<transitions->size();i++){
    result = transitions->get(i)->conditionFunction();
    if(result == true){
      return transitions->get(i)->stateNumber;
    }
  }
  return index;
}

/*
 * Execute runs the stateLogic and then evaluates
 * all available transitions. The transition that
 * returns true is returned.
 */
int State::execute(){
  stateLogic();
  return evalTransitions();
}

/*
 * Method to dynamically set a transition
 */
int State::setTransition(int index, int stateNo){
	if(transitions->size() == 0) return -1;
	transitions->get(index)->stateNumber = stateNo;
	return stateNo;
}


#endif


#ifndef _STATEMACHINE_H
#define _STATEMACHINE_H

class StateMachine
{
  public:
    // Methods
    
    StateMachine();
    ~StateMachine();
    void init();
    void run();

    // When a stated is added we pass the function that represents 
    // that state logic
    State* addState(void (*functionPointer)());
    State* transitionTo(State* s);
    int transitionTo(int i);
	
    // Attributes
    LinkedList<State*> *stateList;
	  bool executeOnce = true; 	//Indicates that a transition to a different state has occurred
    int currentState = -1;	//Indicates the current state number
};

StateMachine::StateMachine(){
  stateList = new LinkedList<State*>();
};

StateMachine::~StateMachine(){};

/*
 * Main execution of the machine occurs here in run
 * The current state is executed and it's transitions are evaluated
 * to determine the next state. 
 * 
 * By design, only one state is executed in one loop() cycle.
 */
void StateMachine::run(){
  //Serial.println("StateMachine::run()");
  // Early exit, no states are defined
  if(stateList->size() == 0) return;

  // Initial condition
  if(currentState == -1){
    currentState = 0;
  }
  
  // Execute state logic and return transitioned
  // to state number. Remember the current state then check
  // if it wasnt't changed in state logic. If it was, we 
  // should ignore predefined transitions.
  int initialState = currentState;
  int next = stateList->get(currentState)->execute();
  if(initialState == currentState){
    executeOnce = (currentState == next)?false:true;
    currentState = next;
  }
}

/*
 * Adds a state to the machine
 * It adds the state in sequential order.
 */
State* StateMachine::addState(void(*functionPointer)()){
  State* s = new State();
  s->stateLogic = functionPointer;
  stateList->add(s);
  s->index = stateList->size()-1;
  return s;
}

/*
 * Jump to a state
 * given by a pointer to that state.
 */
State* StateMachine::transitionTo(State* s){
  this->currentState = s->index;
  this->executeOnce = true;
  return s;
}

/*
 * Jump to a state
 * given by a state index number.
 */
int StateMachine::transitionTo(int i){
  if(i < stateList->size()){
	this->currentState = i;
	this->executeOnce = true;
	return i;
  }
  return currentState;
}

#endif

#ifndef ENCODER_H
#define ENCODER_H


/*
FORWARD (Avance Positivo)
Impulse 1 (A)  ___     ___     ___     ___
              |   |___|   |___|   |___|   |
Impulse 2 (B)      ___     ___     ___    
         _________|   |___|   |___|   |___|

Transiciones (A → cambia antes que B):
- A sube, luego B sube → +
- A baja, luego B baja → +

------------------------------------------------

REVERSE (Retroceso Negativo)
Impulse 1 (A)      ___     ___     ___    
         _________|   |___|   |___|   |___|
Impulse 2 (B)  ___     ___     ___     ___
              |   |___|   |___|   |___|   |

Transiciones (B → cambia antes que A):
- B sube, luego A sube → -
- B baja, luego A baja → -

------------------------------------------------

ESQUEMA DE CAMBIOS DE ESTADO
---------------------------------
Estado Anterior  |  Estado Nuevo  |  Acción
---------------------------------
    00           |      01        |  +1 (Forward)
    01           |      11        |  +1 (Forward)
    11           |      10        |  +1 (Forward)
    10           |      00        |  +1 (Forward)

    00           |      10        |  -1 (Reverse)
    10           |      11        |  -1 (Reverse)
    11           |      01        |  -1 (Reverse)
    01           |      00        |  -1 (Reverse)

---------------------------------
Leyenda:
00 -> A = 0, B = 0
01 -> A = 0, B = 1
10 -> A = 1, B = 0
11 -> A = 1, B = 1
*/

class Encoder{
	public:
	//Methods
	Encoder(uint8_t pinA, uint8_t pinB, int lag);
	~Encoder();            //Destructor
	void reset();           //Resets timer to zero
  void update();  
  long getCount();
  long countGreaterThan(long destination);
  long countLessThan(long destination);  
	uint8_t pinA;
  uint8_t pinB;
	
  private:
  int lag;
  long counter;
  uint8_t state;
};

//Default constructor
Encoder::Encoder(uint8_t pinA, uint8_t pinB, int lag){
	this->pinA = pinA;
  this->pinB = pinB;
  this->lag = lag;
  this->counter = -9999;
  this->state = (digitalRead(pinA) << 1) | digitalRead(pinB);
}

//Default destructor
Encoder::~Encoder(){
  
}

void Encoder::reset(void) {
  this->counter = 0;
}

long Encoder::getCount(void) {
  return (this->counter);
}

long Encoder::countGreaterThan(long destination) {
  if(this->counter + this->lag >= destination){
    return true;
  }
  return false;
}

long Encoder::countLessThan(long destination) {
  if(this->counter + this->lag <= destination){
    return true;
  }
  return false;
}

void Encoder::update(void) {
    uint8_t currentState = (digitalRead(this->pinA) << 1) | digitalRead(this->pinB);
    uint8_t transition = (this->state << 2) | currentState; // Estado previo + nuevo

    switch (transition) {
        case 0b0001: case 0b0111: case 0b1000: case 0b1110:
            this->counter++;  // Forward
            break;
        case 0b0010: case 0b0100: case 0b1011: case 0b1101:
            this->counter--;  // Reverse
            break;
    }

    this->state = currentState;  // Guardar el nuevo estado
}

#endif


#ifndef AXIS_H
#define AXIS_H

#define AXIS_UP 1
#define AXIS_DOWN -1
#define AXIS_LEFT 1
#define AXIS_RIGHT -1
#define AXIS_FORWARD 1
#define AXIS_BACKWARD -1

#define AXIS_POSITIVE 1  // Movimiento en la dirección positiva del eje
#define AXIS_NEGATIVE -1 // Movimiento en la dirección negativa del eje
#define AXIS_NONE 0

#define AXIS_START 1
#define AXIS_STOP 0

#define AXIS_ON 1
#define AXIS_OFF 0


class Axis{
	public:
	//Methods
  Axis(uint8_t pin);
	Axis(uint8_t pinA, uint8_t pinB);
	~Axis();            //Destructor
	void move(int type);           //Resets timer to zero 
	uint8_t pinA;
  uint8_t pinB;
	
  private:

};


Axis::Axis(uint8_t pin){
	this->pinA = pin;
  this->pinB = -1;
}

Axis::Axis(uint8_t pinA, uint8_t pinB){
	this->pinA = pinA;
  this->pinB = pinB;
}

//Default destructor
Axis::~Axis(){
  
}


void Axis::move(int type) {
  if (type == AXIS_NONE) {
      if (this->pinA > 0) digitalWrite(this->pinA, LOW);
      if (this->pinB > 0) digitalWrite(this->pinB, LOW);
  } else if (type == AXIS_POSITIVE) {
      if (this->pinA > 0) digitalWrite(this->pinA, HIGH);
      if (this->pinB > 0) digitalWrite(this->pinB, LOW);
  } else if (type == AXIS_NEGATIVE) {
      if (this->pinA > 0) digitalWrite(this->pinA, LOW);
      if (this->pinB > 0) digitalWrite(this->pinB, HIGH);
  }
}

#endif

#ifndef END_STOP_H
#define END_STOP_H

#define END_STOP_ON 1
#define END_STOP_OFF 0

#define END_STOP_MODE_NORMAL 0
#define END_STOP_MODE_INVERTED 1


class EndStop{
	public:
	//Methods
  EndStop(uint8_t pin, uint8_t mode);
	~EndStop();            //Destructor
	uint8_t getValue();
  uint8_t isPressed();
  void update();
	uint8_t pin;
  uint8_t mode;
	
  private:
  uint8_t value;
};


EndStop::EndStop(uint8_t pin, uint8_t mode){
	this->pin = pin;
  this->mode = mode;
}


//Default destructor
EndStop::~EndStop(){
  
}


uint8_t EndStop::getValue() {
  return this->value;
}

uint8_t EndStop::isPressed() {
  if(this->mode == END_STOP_MODE_NORMAL){
    this->value == 0?1:0;
  }else{
    this->value == 0?0:1;
  }
}

void EndStop::update() {
  this->value = digitalRead(this->pin);
}

#endif



#ifndef MGEPLib_h
#define MGEPLib_h

#define ARRAY_SIZE(X) (sizeof(X)/sizeof(*X))

extern void initialize();
extern void execute();

StateMachine machine;

#include <Arduino.h>
#include <Controllino.h>

unsigned long machineStatePreviousMillis = 0;
unsigned long jsonGenerationPreviousMillis = 0;
const long machineStateInterval = 1000/75; //50Hz
const long jsonGenerationInterval = 1000/1; //1Hz

EthernetServer server(80);

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(10, 1, 31, 177);
IPAddress dns(192, 168, 1, 1);   // Dirección IP del servidor DNS (opcional)
IPAddress gateway(10, 1, 31, 1);  // Dirección IP de la puerta de enlace (el router)
IPAddress subnet(255, 255, 255, 0); // Máscara de subred

char jsonArray[512];


void generateJson() {
  // String para almacenar la estructura JSON temporal
  String json = "{\n";

  // Leer y agregar el estado de los pines digitales (D0 a D21, sin relés compartidos)
  json += " \"digital\":\n {\n  ";
  for (int i = 0; i <= 21; i++) {
    json += "\"D" + String(i) + "\":" + String(digitalRead(i));
    if (i < 21) {
      json += ",";
      json += "\n  ";
    }else{
      json += "\n ";
    }
  }
  json += "},\n";

  // Leer y agregar el estado de los pines analógicos
  json += " \"analog\":\n {\n  ";
  for (int i = 0; i <= 11; i++) {
    json += "\"A" + String(i) + "\":" + String(analogRead(A0 + i));
    if (i < 11) {
      json += ",";
      json += "\n  ";
    }else{
      json += "\n ";
    }
  }
  json += "},\n";

  // Leer y agregar el estado de los relés
  json += " \"relay\":\n {\n  ";
  for (int i = 22; i <= 31; i++) {
    json += "\"R" + String(i - 21) + "\":" + String(digitalRead(i));  // Ajuste para mostrar Relay1, Relay2, etc.
    if (i < 31) {
      json += ",";
      json += "\n  ";
    }else{
      json += "\n ";
    }
  }
  json += "}\n";

  json += "}";

  
  // Convertir String a char[] y copiarlo a la memoria asignada
  json.toCharArray(jsonArray, json.length() + 1);
}

void setup() {

  Serial.begin(250000);
  Ethernet.begin(mac, ip, dns, gateway, subnet);
  server.begin();
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());
  initialize();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - machineStatePreviousMillis >= machineStateInterval) {
    machineStatePreviousMillis = currentMillis;
    machine.run();
  }

  currentMillis = millis();
  if (currentMillis - jsonGenerationPreviousMillis >= jsonGenerationInterval) {
    jsonGenerationPreviousMillis = currentMillis;
    //generateJson();
  }

  // Listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("New client!");
    WebServer w(client);                    // Read and parse the HTTP Request
    w.serveUrl("/ios",[](WebServer &w){
      w.client.println(jsonArray);
    });
    w.serveUrl("/",[](WebServer &w){
      w.client.println("{");
      w.client.print(" \"iosUrl\":\"http://");
      w.client.print(Ethernet.localIP());
      w.client.println("/ios\"");
      w.client.println("}");
    });
  }

  execute();
}

StateMachine CreateNewMachine() {
  machine = StateMachine();
  return machine;
}

#endif