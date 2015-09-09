#include <avr/io.h>
#include <util/delay.h>
#include "lcd_driver.h"

#define set_rs_high()	RS_PORT |= _BV(RS_PIN)	/* selects DR */
#define set_rs_low()	RS_PORT &= ~_BV(RS_PIN)	/* selects IR */

#define set_rw_high()	RW_PORT |= _BV(RW_PIN) 	/* selects read */
#define set_rw_low()	RW_PORT &= ~_BV(RW_PIN)	/* selects write */

#define set_en_high()	EN_PORT |= _BV(EN_PIN)	/* enables data read/write */
#define set_en_low()	EN_PORT &= ~_BV(EN_PIN) /* disables data read/write */

#define DDR(port) 	(*(&port - 1))
#define PIN(port)  	(*(&port - 2))

static void data_pins_out(void); 
static void data_pins_in(void);
static void set_data_pins(uint8_t data);
static uint8_t read_pins(void);
static void e_delay(void); 
static void toggle_e(void);
static void waitbusy(void);
static uint8_t lcd_read(uint8_t rs);
static void lcd_write(uint8_t data, uint8_t rs); 

/* set data pins to output mode */
static void data_pins_out(void) 
{
	DDR(D0_PORT) |= _BV(D0_PIN);
	DDR(D1_PORT) |= _BV(D1_PIN);
	DDR(D2_PORT) |= _BV(D2_PIN);
	DDR(D3_PORT) |= _BV(D3_PIN);
	DDR(D4_PORT) |= _BV(D4_PIN);
	DDR(D5_PORT) |= _BV(D5_PIN);
	DDR(D6_PORT) |= _BV(D6_PIN);
	DDR(D7_PORT) |= _BV(D7_PIN);
}

/* set data pins to input mode */
static void data_pins_in(void)
{
	DDR(D0_PORT) &= ~_BV(D0_PIN);
	DDR(D1_PORT) &= ~_BV(D1_PIN);
	DDR(D2_PORT) &= ~_BV(D2_PIN);
	DDR(D3_PORT) &= ~_BV(D3_PIN);
	DDR(D4_PORT) &= ~_BV(D4_PIN);
	DDR(D5_PORT) &= ~_BV(D5_PIN);
	DDR(D6_PORT) &= ~_BV(D6_PIN);
	DDR(D7_PORT) &= ~_BV(D7_PIN);
}

static void set_data_pins(uint8_t data)
{
	if (data & 0x01) D0_PORT |= _BV(D0_PIN); else D0_PORT &= ~_BV(D0_PIN);
	if (data & 0x02) D1_PORT |= _BV(D1_PIN); else D1_PORT &= ~_BV(D1_PIN);
	if (data & 0x04) D2_PORT |= _BV(D2_PIN); else D2_PORT &= ~_BV(D2_PIN);
	if (data & 0x08) D3_PORT |= _BV(D3_PIN); else D3_PORT &= ~_BV(D3_PIN);
	if (data & 0x10) D4_PORT |= _BV(D4_PIN); else D4_PORT &= ~_BV(D4_PIN);
	if (data & 0x20) D5_PORT |= _BV(D5_PIN); else D5_PORT &= ~_BV(D5_PIN);
	if (data & 0x40) D6_PORT |= _BV(D6_PIN); else D6_PORT &= ~_BV(D6_PIN);
	if (data & 0x80) D7_PORT |= _BV(D7_PIN); else D7_PORT &= ~_BV(D7_PIN);
}

/* reads current levels at D0-D7 pins */
static uint8_t read_pins(void) 
{
	uint8_t data = 0; 
	if (PIN(D0_PORT) & _BV(D0_PIN))	data |= 0x01;
	if (PIN(D1_PORT) & _BV(D1_PIN))	data |= 0x02;
	if (PIN(D2_PORT) & _BV(D2_PIN))	data |= 0x04;
	if (PIN(D3_PORT) & _BV(D3_PIN))	data |= 0x08;
	if (PIN(D4_PORT) & _BV(D4_PIN))	data |= 0x10;
	if (PIN(D5_PORT) & _BV(D5_PIN))	data |= 0x20;
	if (PIN(D6_PORT) & _BV(D6_PIN))	data |= 0x40;
	if (PIN(D7_PORT) & _BV(D7_PIN))	data |= 0x80;

	return data; 
}

static void e_delay(void)
{
	_delay_us(EN_PULSE_DELAY);
}

/* pulses enable pin high */
static void toggle_e(void)
{
	set_en_high();
	e_delay();
	set_en_low(); 
}

/* initializes lcd for use, this or lcd_init_wo_ir MUST be called first
 *
 * Note: this function does NOT assume internel reset circuit power 
 * conditions were met
 */
void lcd_init(void) 
{
	int i; 

	/* set control pins as output */
	DDR(EN_PORT) |= _BV(EN_PIN);
	DDR(RW_PORT) |= _BV(RW_PIN);
	DDR(RS_PORT) |= _BV(RS_PIN); 

	/* set data pins as output */
	data_pins_out();

	/* wait until lcd is finished booting */
	_delay_us(BOOT_DELAY);	

	/* set control pins for function set */
	for (i = 0; i < 3; i++) {
		set_rs_low();
		set_rw_low();
		set_data_pins(F_SET_INITIAL);
		toggle_e(); 
		_delay_us(INIT_DELAY); 
	}

	/* final function set, set's 8bit operation, 2-line disp, 5x8 font */
	set_data_pins(F_SET_DEFAULT);
	toggle_e(); 
	_delay_us(INIT_DELAY); 

	lcd_command(DISP_OFF);		/* display off */
	lcd_command(CLEAR_DISP);	/* display clear*/
	lcd_command(ENTRY_DEFAULT); 	/* set entry mode to INC, no shift */
	lcd_command(DISP_ON);		/* display on */
}

/* waits until busy flag is not set, i.e. the next instruction can be sent */
static void waitbusy(void)
{
	while (lcd_read(0) & BF_MASK)
		;
}

/* displays a single character on the lcd display */
void lcd_putc(char c)
{
	waitbusy();
	lcd_write(c, 1);
}

/* displays a null terminated string on the lcd display */
void lcd_puts(char *s)
{
	while (*s)
		lcd_putc(*s++);
}

/* clears the lcd screen by writing `spaces` to all locations in DDRAM */
void lcd_clrscr(void)
{
	lcd_command(CLEAR_DISP);
}

/*
 *      1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16  x
 *     +-----------------------------------------------+
 * 1   |00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F|
 * 2   |40 41 42 43 44 45 46 47 48 49 4A 4B 4C 4D 4E 4F|
 *     +-----------------------------------------------+
 * y
 *
 * set's the address counter to the memory location in DDRAM that corresponds
 * to the x, y co-ordinates given. x and y must be in the range [1,1] 
 * to [16,16] 
 *
 * returns 0 on success, -1 on failure
 */
uint8_t lcd_goto(uint8_t x, uint8_t y)
{
	uint8_t w_data;

	if (y > 0 && y < 3 && x > 0 && x < 17) {
		waitbusy();
		(y == 1) ? (w_data = x - 1) : (w_data = x-1+0x40);
		lcd_write(w_data | 0x80, 0);
		return 0; 
	}
	return -1;
}

/* writes instruction byte to controller, see datasheet for available 
 * instructions
 */
void lcd_command(uint8_t cmd)
{
	waitbusy(); 
	lcd_write(cmd, 0); 
}

/* reads byte from controller
 * Input:	rs	1: read data from DDRAM/CGRAM
 * 			0: read busy flag and address counter 
 */
static uint8_t lcd_read(uint8_t rs)
{
	uint8_t data;

	if (rs)
		set_rs_high();	/* RS=0 -> read data from DD/CGRAM */
	else
		set_rs_low();	/* RS=1 -> read busy flag and AC */
	set_rw_high(); 		/* RW=1 for all read operations */
	data_pins_in(); 

	set_en_high();
	e_delay();		/* enable 1us high (could be set much less) */
	
	data = read_pins(); 
	set_en_low();

	return data; 
}

/* writes byte to controller
 * Input	data	byte to write to LCD
 * 		rs	1: write data
 * 			0: write instruction
 */
static void lcd_write(uint8_t data, uint8_t rs) 
{
	if (rs)
		set_rs_high(); 
	else
		set_rs_low(); 
	set_rw_low(); 		/* RW=0 for all write operations */
	data_pins_out(); 	/* set's data pins for output */

	set_data_pins(data); 
	
	toggle_e(); 
}
