/*
 * ADC 5 button Keypad Reader
 *
 * keypad.h
 *
 * Author : fuzun
 * License : MIT License
 */ 

#ifndef KEYPAD_H_
#define KEYPAD_H_

// Definitions for the keypad:
#define KEYPAD_ANALOG_PIN 0 // PORTC, PIN0, ADC0

// For this specific keypad we have, states are: (ADC Outputs)
// No-press: 1023
// Left: 500
// Right: 0
// Select: 737
// Up: 140
// Down: 324
#define KEYPAD_STATE_RIGHT 0 // Right
#define KEYPAD_STATE_NONE 1023 // None
#define KEYPAD_STATE_UP 140 // Up
#define KEYPAD_STATE_DOWN 324 // Down
#define KEYPAD_STATE_LEFT 500 // Left
#define KEYPAD_STATE_SELECT 737 // Select

#define KEYPAD_TOLERANCE 50 // Should be fair

// Keypad states:
typedef enum Key_e
{
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_SELECT,
	KEY_NONE
}Key;

void init_keypad(int *prescaler, int *vref, int *pin);
Key readKey(void);

#endif /* KEYPAD_H_ */
