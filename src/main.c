/*
 * Countdown Timer
 *
 * main.c
 *
 * Author : fuzun
 * License : MIT License
 */ 

// Hardware:
// * RobotDyn UNO R3 CH340 / ATMega328PA 2016 version Arduino UNO Board
//  -> Schematic: https://robotdyn.com/pub/media/0G-00004029==UNO-R3-CH340G/DOCS/Schematic==0G-00004029==UNO-R3-CH340G.pdf
//  -> Pinout: https://robotdyn.com/pub/media/0G-00004029==UNO-R3-CH340G/DOCS/PINOUT==0G-00004029==UNO-R3-CH340G.pdf
// * RobotDyn LCD 16x2 keypad shield non-Cyrillic version
//  -> Drawing: https://robotdyn.com/pub/media/0G-00004297==LCD(16x2)-Shield/DOCS/DIM==0G-00004297==LCD(16x2)-Shield.pdf
//  -> Schematic: https://robotdyn.com/pub/media/0G-00004297==LCD(16x2)-Shield/DOCS/Schematic==0G-00004297==LCD(16x2)-Shield.pdf

#ifdef F_CPU
#undef F_CPU
#endif
// Since the project is targeted for Arduino UNO board, we have to specify the clock.
#define F_CPU 16000000L // Arduino UNO operates at 16 MHZ.


#include <avr/io.h> // AVR definitions
#include <stdlib.h> // libc operations
#include <string.h> // libc operations
#include <stdio.h> // libc operations
#include <util/delay.h> // for non-crucial delay
#include <stdbool.h> // for BOOL
#include <stddef.h> // for NULL

// This project has to use different kind of (non-standard) LCD library because the existing shield does not have RW pin connected to the LCD.
// As we don't have control on RW (because it is directly connected to ground), the library we have to use need to mirror any 
// writing process to memory/registers in order to keep track with LCD. Because R/W connected to ground, it is impossible to read the LCD
// or make it operate at READ mode. This results in one free pin for the shield but with memory usage disadvantage for the program.
// It is also slightly modified to make it work for the specific configuration the shield has:
// LCD Type: non-inverted HD44780
// 2 "rows" (lines) / 16 "columns" (characters)
// 4-bit operation mode
// Mappings: (https://www.arduino.cc/en/Reference/PortManipulation)
// DATA0_PIN -> PORTD Pin 4 (mapped to Arduino Digital Pin 4)
// DATA1_PIN -> PORTD Pin 5 (mapped to Arduino Digital Pin 5)
// DATA2_PIN -> PORTD Pin 6 (mapped to Arduino Digital Pin 6)
// DATA3_PIN -> PORTD Pin 7 (mapped to Arduino Digital Pin 7)
// RS_PIN -> PORTB Pin 0 (mapped to Arduino Digital Pin 8)
// RW_PIN -> * (There is no R/W pin) therefore LCD_READ_REQUIRED is set to 0.
// E_PIN (Enable Pin) -> PORTB Pin 1 (mapped to Arduino Digital Pin 9)
#include "lcd/lcd.h"

// The shield has 5 buttons and they are all connected to A0 with resistors. (Ref: shield schematic)
// Therefore it is necessary to use ADC to determine which key is pressed. 
#include "adc/adc.h"

// Simple key reader.
#include "keypad.h"

// Timer structure:
typedef struct Timer_s
{
	int hour;
	int minute;
	int second;
	int millisecond;
}Timer;
Timer timer; // Global timer

void initUI(void); // UI initializer function
void tick(void); // Tick of the timer function
void printTime(bool showMilliseconds);
inline void delay_tenMillisecs(void); // inline, asm 10 ms delay function

int cursorPosition = 0; // indexing starts with 1 && '-1' is lock state
int tickCount = 0; // tick count used for LCD refreshing

bool isTicking = false; // ticking indicator
bool isOver = false; // finish indicator

int main(void)
{
	memset(&timer, 0, sizeof(timer)); // To set hour, minute etc. to 0
	initUI();
	
	Key key, previousKey = KEY_NONE;
	
	// Main Loop:
    while (1)
    {
		key = readKey();
		
		// this if is needed not to register keys over and over again and to only allow
		// KEY_SELECT when the timer is active. KEY_NONE is blocked as well to make
		// the loop execute faster.
		if(key != KEY_NONE && key != previousKey && !((isTicking || isOver) && key != KEY_SELECT))
		{
			switch(key)
			{
				case KEY_LEFT:
					if(cursorPosition == -1 || cursorPosition <= 1) break; // Can not go any left
					cursorPosition--;
					break;
					
				case KEY_RIGHT:
					if(cursorPosition == -1 || cursorPosition >= 8) break; // Can not go any right
					cursorPosition++;
					break;
					
				case KEY_UP:
					switch(cursorPosition)
					{
						case 1:
							if(timer.hour < 90) timer.hour += 10;
							break;
						case 2:
							if(timer.hour < 99) timer.hour++;
							break;
						case 4:
							if(timer.minute < 90) timer.minute += 10;
							break;
						case 5:
							if(timer.minute < 99) timer.minute++;
							break;
						case 7:
							if(timer.second < 90) timer.second += 10;
							break;
						case 8:
							if(timer.second < 99) timer.second++;
							break;
					}
					break;
			
				case KEY_DOWN:
					switch(cursorPosition)
					{
						case 1:
							if(timer.hour >= 10) timer.hour -= 10;
							break;
						case 2:
							if(timer.hour > 0) timer.hour--;
							break;
						case 4:
							if(timer.minute >= 10) timer.minute -= 10;
							break;
						case 5:
							if(timer.minute > 0) timer.minute--;
							break;
						case 7:
							if(timer.second >= 10) timer.second -= 10;
							break;
						case 8:
							if(timer.second > 0) timer.second--;
							break;
					}
					break;
				
				case KEY_SELECT:
					if(isTicking) // if ticking, pause
					{
						isTicking = false;
						cursorPosition = 1;
						lcd_clrscr(); // clear the screen
						lcd_gotoxy(0, 0);
						lcd_puts("PAUSED");
					}
					else if(!isTicking && !isOver) // start the timer
					{
						if(timer.hour != 0 || timer.minute != 0 || timer.second != 0)
						{
							isTicking = true; // timer is active
							cursorPosition = -1; // to lock the cursor
							lcd_clrscr(); // clear the screen
							lcd_gotoxy(0, 0);
							lcd_puts("Timer is active!");
						}
					}
					else if(isOver) // reset the timer if it's over
					{
						memset(&timer, 0, sizeof(timer)); // To set hour, minute etc. to 0
						initUI();
						isOver = false;
					}
					break;
					
				case KEY_NONE: // just to make the compiler happy
				default:
					break;
			}
			
			// To avoid cursor to be placed at ":"
			if(cursorPosition == 3)
			{
				if(key == KEY_LEFT)
				cursorPosition = 2;
				else // if(key == KEY_RIGHT)
				cursorPosition = 4;
			}
			else if(cursorPosition == 6)
			{
				if(key == KEY_LEFT)
				cursorPosition = 5;
				else // if(key == KEY_RIGHT)
				cursorPosition = 7;
			}
		
			if(key == KEY_UP || key == KEY_DOWN || key == KEY_SELECT)
				printTime(false);
		
			if(cursorPosition != -1) // because -1 is lock state
				lcd_gotoxy(cursorPosition - 1, 1);
		}
		previousKey = key;
		
		if(isTicking)
		{
			tick();
			tickCount++;
			if(tickCount == 10) // Since LCD is not fast enough, refresh it every 10. tick which makes it refresh at a rate of ~ 100 ms.
			{
				printTime(true);
				tickCount = 0;
			}
		}
		else
		{
			// Since the timer is not active,
			// it is better to use the standard delay function
			// because the performance penalty won't affect us
			 _delay_ms(50); // To slow down as the timer is not active
		}
    }
	
	return EXIT_SUCCESS; // Won't be executed because of the endless loop but it's added to make the compiler happy.
}

void initUI(void)
{
	lcd_init(LCD_DISP_ON_CURSOR); // initialize the LCD.
	lcd_gotoxy(0, 0); // Set cursor to (0, 0)
	lcd_puts("Countdown Timer"); // Print the string
	// lcd_gotoxy(0, 1); // Goto second line / set cursor to (0, 1)
	printTime(false); // Print "00:00:00"
	lcd_gotoxy(7, 1);
	cursorPosition = 8;
	init_keypad(NULL, NULL, 0 /* NULL */); // Pin: 0
}

void printTime(bool showMilliseconds)
{
	static char buffer[32]; // buffer for lcd 1-line string
	
	if(showMilliseconds)
	{
		// padding through printf formatting
		// 00:00:00  000
		snprintf(buffer, sizeof(buffer), "%.2d:%.2d:%.2d  %.3d", \
		timer.hour, \
		timer.minute, \
		timer.second, \
		timer.millisecond);
	}
	else
	{
		// 00:00:00
		snprintf(buffer, sizeof(buffer), "%.2d:%.2d:%.2d", \
		timer.hour, \
		timer.minute, \
		timer.second);
	}
	
	lcd_gotoxy(0, 1);
	lcd_puts(buffer);
}

void tick(void)
{
	if(timer.hour <= 0 && \
	 timer.minute <= 0 && \
	 timer.second <= 0 && \
	 timer.millisecond <= 0)
	{
		isTicking = false;
		isOver = true;
		lcd_clrscr();
		lcd_gotoxy(0, 0);
		lcd_puts("Time is over!!!");
		printTime(true);
		cursorPosition = -1;
		return;
	}
	
	timer.millisecond -= 10;
	if(timer.millisecond < 0)
	{
		timer.millisecond += 1000;
		timer.second--;
		if(timer.second < 0)
		{
			timer.second = 59;
			timer.minute--;
			if(timer.minute < 0)
			{
				timer.minute = 59;
				timer.hour--;
			}
		}
	}
	
	delay_tenMillisecs();
}


// Fast Delay Function
// Instead of using from an external library,
// This one will be used to delay 10 ms @ 16 MHZ
// purpose of inline: suggest compiler to inline this function to make it even faster
// and reduce the call overhead
inline void delay_tenMillisecs(void)
{
	asm volatile (
	"    ldi  r18, 208"	"\n"
	"    ldi  r19, 202"	"\n"
	"1:  dec  r19"	"\n"
	"    brne 1b"	"\n"
	"    dec  r18"	"\n"
	"    brne 1b"	"\n"
	"    nop"	"\n"
	);
}
