#include <avr/io.h>
#include <util/delay.h>
#include "lcd_driver.h"

#ifndef F_CPU
#define	F_CPU	16000000UL
#endif

#define BAUD		9600
#define BAUDRATE	((F_CPU)/(BAUD*16UL)-1)
#define	NEWLINE		0x0A
#define MAX_STR_LEN	33		/* 16*2 display positions + '\0' */

void uart_init(void)
{
	/* set baudrate */
	UBRR0H = (BAUDRATE >> 8);
	UBRR0L = BAUDRATE;

	UCSR0B |= _BV(TXEN0)|_BV(RXEN0);	/* enable transmit/receive */
	UCSR0C |= _BV(UCSZ01)|_BV(UCSZ00);	/* set 8bit data mode */
}

void uart_transmit(uint8_t data)
{
	while (!(UCSR0A & _BV(UDRE0)));
	UDR0 = data;
}

void uart_stransmit(char *s)
{
	while (*s)
		uart_transmit(*s++);
}

uint8_t uart_receive(void)
{
	while (!(UCSR0A & _BV(RXC0)));
	return UDR0;
}

void disp_received(char *s)
{
	lcd_goto(1,1);
	while (*s) {
		if (*s == '\n')
			lcd_goto(1,2);
		else
			lcd_putc(*s);
		++s;
	}
}

int main(void)
{
	uint8_t a;
	uint8_t i;
	char buffer[MAX_STR_LEN];

	lcd_init();
	uart_init();

	while (1) {
		i = 0;
		while ((a = uart_receive()))
			buffer[i++] = a;
		buffer[i] = '\0';

		lcd_clrscr();
		disp_received(buffer);	/* display received serial data */
		_delay_ms(100);
	}
}
