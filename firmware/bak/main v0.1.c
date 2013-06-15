

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
		number_of_keys_pressed --;
		}
		else {	
			PORTB = note -21;
			number_of_keys_pressed ++;
		}

		if(number_of_keys_pressed >0){
			PORTD |= (1<<6);
		}
		else {
			PORTD &= ~(1<<6);
		}
		
	}
	

	counter ++;
	if (counter ==3)
	{counter = 1;}





}




   return 0;              
}






