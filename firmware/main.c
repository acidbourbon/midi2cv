

#define F_CPU 12e6
 
#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>

#define NOTE_ON 0b10010000
#define NOTE_OFF 0b10000000
#define PITCH_WHEEL 0b11100000
#define CONTROL_CHANGE 0b10110000
#define CC_MOD_WHEEL 1
#define CC_PORTAMENTO 5



void init_USART(void)
{
	UCSRB |= (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
	UCSRC |= (1<<UCSZ0)|(1<<UCSZ1); 
	UBRRH = 0;
	UBRRL = 23;
}


void playnote(int note)
{
PORTB = note;
_delay_ms(100);
}








#define STACKSIZE 8

struct Notestack {
	uint8_t data[STACKSIZE];
	uint8_t fill;
	} notestack = {{},0};
int8_t match_pos = 0;
uint8_t porta_mux= 0;


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

void make_gate() {
	if(notestack.fill >0) {
		PORTD |= (1<<6);
	}
	else {
		PORTD &= ~(1<<6);
	}

}
void make_cv() {
	static uint8_t dac_value = 0;
	dac_value = (last_active_key()-36)<<1;
	PORTD &= ~(0b00111100);
	PORTD |= (dac_value & 0x0F)<<2;
	PORTB &= ~(0b11100000);
	PORTB |= (dac_value & 0xF0)<<1;
}

void make_portamento(uint8_t setting) {
	if (setting <=2 ) {
		OCR1B = 1023;
		porta_mux=0;
	}
	else if(setting < 43+2) {
		porta_mux=1;
		OCR1B = 1024/(setting+1-2)-1;
	} 
	else if(setting <= 43+2+43) {
		porta_mux=2;
		OCR1B = 1024/(setting+23+1-2-43)-1;
	}
	else {
		porta_mux=3;
		OCR1B = 1024/(setting+33+1-2-2*43)-1;
	}
}




int main (void) {     


uint8_t counter=0, status_byte=0, data_byte_1=0, data_byte_2=0;

uint8_t temp;    

init_USART(); 

PORTB = 0;
DDRB = 0xFF;


PORTD &= ~(1<<6);
PORTD |= 1<<1; //activate pullup for gate mode pin
DDRD |= 0b001111100;


TCCR0A = (1<<WGM11)|(1<<WGM10)|(1<<COM1A1);
TCCR0B = (0<<CS12) | (1<<CS11)| (1<<CS10);
TCCR1A = (1<<WGM11)|(1<<WGM10)|(1<<COM1A1)|(1<<COM1B1);
TCCR1B = (0<<CS12) | (1<<CS11)| (0<<CS10);



// default settings

OCR1A = 511; // pitch wheel in the middle
make_portamento(35);


while(1)
{
	while(!(UCSRA&(1<<RXC))){};

	//store received data to temp
	temp = UDR;

	if (temp >=128) { // is status byte
		counter = 0; // start counting
		status_byte = temp;
	}

	if(counter ==1){ // store first data byte
		data_byte_1 = temp;
	}
	if(counter ==2){ // store second data byte and ...
		data_byte_2 = temp;

		switch(status_byte){ // evaluate !
			case NOTE_ON:
				if(data_byte_2) { // if key velocity greater zero
					to_stack(data_byte_1); // add note to stack
					if(notestack.fill >1) // activate glide when >1 keys pressed
						PORTB = (PORTB & ~(0b00000011)) | porta_mux; // select right resistor for RC filter
					make_cv();
					make_gate();
					break;
				}
				// else: if key velocity is zero, act the same as key off event
			case NOTE_OFF:
				from_stack(data_byte_1); // remove note from stack
				if(notestack.fill == 0) {// when stack empty turn off glide
					PORTB &= ~(0b00000011);
					}
				make_cv();
				make_gate();
				break;
			case PITCH_WHEEL:
				OCR1A = data_byte_2<<3; // output analogue voltage via 10 bit PWM
				break;
			case CONTROL_CHANGE:
				switch(data_byte_1) {
					case CC_MOD_WHEEL:
						OCR0A = data_byte_2 <<1; // output analogue voltage via 8 bit PWM
						break;
		
					case CC_PORTAMENTO:
						make_portamento(data_byte_2);
					}
				break;

				
		}
	}

	

	counter ++;
	if (counter ==3)
	{counter = 1;}





}




   return 0;              
}






