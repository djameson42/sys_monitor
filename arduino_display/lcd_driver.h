#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <avr/io.h>


#define BOOT_DELAY	100000	/* delay in us after power on */
#define	INIT_DELAY	5000 	/* delay in us after function set */
#define EN_PULSE_DELAY	1	/* time to pulse enable pin high in us */


/* configure data pins to suit your setup */
#define D0_PORT		PORTD
#define D0_PIN		6
#define D1_PORT		PORTD
#define D1_PIN		7
#define D2_PORT		PORTB
#define D2_PIN		0
#define D3_PORT		PORTB	
#define D3_PIN		1
#define D4_PORT		PORTB
#define D4_PIN		2
#define D5_PORT		PORTB
#define D5_PIN		3
#define D6_PORT		PORTB
#define D6_PIN		4
#define D7_PORT		PORTB
#define D7_PIN		5

/* configure control pins to suit your setup */
#define EN_PORT		PORTD
#define EN_PIN		5
#define RW_PORT		PORTD
#define RW_PIN		4
#define RS_PORT		PORTD
#define RS_PIN		3

#define BF_MASK		0x80	/* masks with lcd_read(0) to determine BF */

/* instructions as per datasheet */
#define CLEAR_DISP	0x01	/* clear lcd display */
#define RET_HOME	0x02	/* set address counter to 0 */

/* entry mode set commands */
#define ENTRY_DEFAULT	0x06	/* default entry mode */
#define ENTRY_INC	0x06	/* increments DDRAM on write/read */
#define ENTRY_DEC	0x04	/* decrements DDRAM on write/read */
#define ENTRY_INC_SHIFT	0x07	/* increments DDRAM on write/read, shifts */
#define ENTRY_DEC_SHIFT	0x05	/* decrements DDRAM on write/read, shifts */

/* display on/off commands */
#define DISP_ON		0x0C	/* display on, cursor off */
#define DISP_OFF	0x08	/* display off, cursor off */

/* function set commands */
#define F_SET_INITIAL	0x30	/* 8bit operation, see initializing */
#define F_SET_DEFAULT	0x38	/* 8bit operation, 2-line disp, 5x8 font */

/**
 * initializes lcd display 
 */
void lcd_init(void);

/*
 * send instruction to the IR of the controller
 */
void lcd_command(uint8_t cmd); 

/*
 * put a character on the lcd display
 */
void lcd_putc(char c);

/*
 * put a null terminated string on the lcd display
 */
void lcd_puts(char *s);

/* 
 * clears the lcd display 
 */
void lcd_clrscr(void);

/* 
 * set's the address counter to the memory location in ddram corresponding 
 * to the x, y co-ordinates of the screen given 
 *
 * returns 0 on success, -1 on failure
 */
uint8_t lcd_goto(uint8_t x, uint8_t y); 

#endif
