/*
 * ADC 5 button Keypad Reader
 *
 * keypad.c
 *
 * Author : fuzun
 * License : MIT License
 */ 

#include "keypad.h"
#include "adc/adc.h"

#include <stddef.h>

// state declarations:
int state_right;
int state_none;
int state_up;
int state_down;
int state_left;
int state_select;

// adc declarations:
int _prescaler, _vref, _pin;

void init_keypad(int *prescaler, int *vref, int *pin)
{
	state_right = KEYPAD_STATE_RIGHT + KEYPAD_TOLERANCE;
	state_none = KEYPAD_STATE_NONE + KEYPAD_TOLERANCE;
	state_up = KEYPAD_STATE_UP + KEYPAD_TOLERANCE;
	state_down = KEYPAD_STATE_DOWN + KEYPAD_TOLERANCE;
	state_left = KEYPAD_STATE_LEFT + KEYPAD_TOLERANCE;
	state_select = KEYPAD_STATE_SELECT + KEYPAD_TOLERANCE;
	
	if(pin == NULL)
		_pin = KEYPAD_ANALOG_PIN;
	else
		_pin = *pin;
	
	if(prescaler == NULL)
		_prescaler = ADC_PRESCALER_128;
	else
		_prescaler = *prescaler;
	
	if(vref == NULL)
		_vref = ADC_VREF_AVCC;
	else
		_vref = *vref;
}

Key readKey(void)
{
	static int adcVal;
	adcVal = adc_read(_prescaler, _vref, _pin);
	
	// Order is important!
	if(adcVal < state_right)		
		return KEY_RIGHT;
	else if(adcVal < state_up)
		return KEY_UP;
	else if(adcVal < state_down)
		return KEY_DOWN;
	else if(adcVal < state_left)
		return KEY_LEFT;
	else if(adcVal < state_select)
		return KEY_SELECT;
	else // if(adcVal < state_none)
		return KEY_NONE;
}
