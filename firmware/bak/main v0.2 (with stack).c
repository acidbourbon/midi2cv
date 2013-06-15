

#define F_CPU 12e6
 
#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>



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


uint8_t dummy, sense ,counter= 0,control_type,note,number_of_keys_pressed=0;

uint8_t temp,temp_;    

init_USART(); 

PORTB = 0;
DDRB = 0xFF;

PORTD &= ~(1<<6);
DDRD |= (1<<6);




playnote(0);
playnote(4);
playnote(7);
playnote(12);


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
		else
		{control_type = 0;}

	}

	if(control_type && counter ==1)
	{		
	note = temp;
	}
	if(control_type && counter ==2)
	{	

		if(temp==0) {
			from_stack(note);
		}
		else {	
			to_stack(note);
		}

		if(notestack.fill >0){
			PORTD |= (1<<6);
		}
		else {
			PORTD &= ~(1<<6);
		}
		
		PORTB = last_active_key() -21;
		
	}
	

	counter ++;
	if (counter ==3)
	{counter = 1;}





}




   return 0;              
}






