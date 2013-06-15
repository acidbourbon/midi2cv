

#define F_CPU 12e6
 
#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>

#define NOTE_ON 0b10010000
#define NOTE_OFF 0b1000000
#define PITCH_WHEEL 11100000
#define CONTROL_CHANGE 0b10110000
#define CC_MODWHEEL 1
#define CC_PORTAMENTO 5



void init_USART(void)
{
	UCSRB |=  (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
	UCSRC |= (1<<UCSZ0)|(1<<UCSZ1); 
	UBRRH = 0;
	UBRRL = 23;
}


void playnote(int note)
{
PORTB = note;
_delay_ms(100);
return 0;
}



// ############################# buffer stuff ##############################
// basic buffer read and write functions

#define SUCCESS 1
#define FAIL 0
 
#define BUFFER_SIZE 64 // size has to be 2^n (8, 16, 32, 64 ...)
#define BUFFER_MASK (BUFFER_SIZE-1)
 
struct Buffer {
  uint8_t data[BUFFER_SIZE];
  uint8_t read; 
  uint8_t write; 
} buffer = {{}, 0, 0};
 
uint8_t BufferIn(uint8_t byte)
{
  uint8_t next = ((buffer.write + 1) & BUFFER_MASK);
  if (buffer.read == next)
    return FAIL;
  buffer.data[buffer.write] = byte;
  // buffer.data[buffer.write & BUFFER_MASK] = byte; // more secure
  buffer.write = next;
  return SUCCESS;
}
 
uint8_t BufferOut(uint8_t *pByte)
{
  if (buffer.read == buffer.write)
    return FAIL;
  *pByte = buffer.data[buffer.read];
  buffer.read = (buffer.read+1) & BUFFER_MASK;
  return SUCCESS;
}


#define STACKSIZE 8

struct Notestack {
	uint8_t data[STACKSIZE];
	uint8_t fill;
	} notestack = {{},0};
int8_t match_pos = 0;


uint8_t to_stack(uint8_t input) {
	for(int8_t i=notestack.fill; i>=1; i--) {	
		notestack.data[i]=notestack.data[i-1]; // move each element in array to next place
	}
	notestack.data[0] = input; // insert new item at the top
	notestack.fill ++; // increment fill counter
	return notestack.fill;
}

uint8_t from_stack(uint8_t item_to_delete) {

	for(int8_t i=notestack.fill-1; i>=0; i--) {	// search backwards for item to remove (by value)
		if(notestack.data[i] == item_to_delete) {
		match_pos = i;
		break;
		}
	}	

	for(int8_t i=match_pos; i<=notestack.fill-2; i++) {	// shift back by one pos everything after item_to_delete
		notestack.data[i] = notestack.data[i+1];
	}	

	notestack.fill --; //decrement fill counter
	return notestack.fill;
}

uint8_t last_active_key() {
	return notestack.data[0]; // return new top element of key list
	}





int main (void) {     


uint8_t dummy, sense ,counter= 0,control_type,note,number_of_keys_pressed=0, dac_value=0, dac_value_h=0, dac_value_l=0, porta_mux=0;

uint8_t temp,temp_;    

init_USART(); 

PORTB = 0;
DDRB = 0xFF;


PORTD &= ~(1<<6);
DDRD |= 0b001111100;


TCCR0A = (1<<WGM11)|(1<<WGM10)|(1<<COM1A1);
TCCR0B = (0<<CS12) | (1<<CS11)| (1<<CS10);
TCCR1A = (1<<WGM11)|(1<<WGM10)|(1<<COM1A1)|(1<<COM1B1);
TCCR1B = (0<<CS12) | (1<<CS11)| (0<<CS10);



playnote(0);
playnote(4);
playnote(7);
playnote(12);
PORTB &= ~(0b00000011);
PORTB |= (0b00000001);
OCR1A = 511;
OCR1B = 1023;


while(1)
{
	while(!(UCSRA&(1<<RXC))){};

	//store received data to temp
	temp = UDR;
	if (temp >=128) // is command signal
	{
		counter = 0;
		if (temp == 0x90)
		{control_type = 1;} // 1 for Note-On command
		else if (temp == 0b10000000)
		{control_type = 2;} // 2 for Note-Off command
		else if (temp == 0b11100000)
		{control_type = 3;} // 3 for Pitch Wheel Change
		else if (temp == 0b10110000)
		{control_type = 4;} // 4 for Control Change
		else
		{control_type = 0;}

	}

	if(control_type && counter ==1)
	{		
	note = temp;
	}
	if(control_type && control_type <=2 && counter ==2)
	{	

		if(temp==0 || control_type == 2) {
			from_stack(note);

			if(notestack.fill == 0) {
			PORTB &= ~(0b00000011);
			}
		}
		else {	
			to_stack(note);
			if(notestack.fill >1) {
			PORTB = (PORTB & ~(0b00000011)) | porta_mux;
			}
		}

		if(notestack.fill >0){
			PORTD |= (1<<6);
		}
		else {
			PORTD &= ~(1<<6);
		}
		
		dac_value = last_active_key() -21;
		PORTD &= ~(0b00111100);
		PORTD |= (dac_value & 0x0F)<<2;
		PORTB &= ~(0b11100000);
		PORTB |= (dac_value & 0xF0)<<1;

		
		
		
	}
	if(control_type == 3 && counter ==2)
	{
		OCR1A = temp<<3;
	}

	if(control_type ==0 && counter ==2)
	{	
		if (temp <=2 ) {
			OCR1B = 1023;
			porta_mux=0;
		}
		else if(temp < 43+2) {
			porta_mux=1;
			OCR1B = 1024/(temp+1-2)-1;
		} 
		else if(temp <= 43+2+43) {
			porta_mux=2;
			OCR1B = 1024/(temp+23+1-2-43)-1;
		}
		else {
			porta_mux=3;
			OCR1B = 1024/(temp+33+1-2-2*43)-1;
		}



		
	}
	

	counter ++;
	if (counter ==3)
	{counter = 1;}





}




   return 0;              
}






