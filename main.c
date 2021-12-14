/*  Name: Nikhil Saroya
 *  Project 4: Digital Temperature sensor
 *  ECE 312 Section D51
 * 
 *  Acknowledgements: Abhi Sharma helped me with the furnace functionality; 
 * 
 */

#define F_CPU 1000000UL 
#include "defines.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdbool.h>

#include "lcd.h"
#include "hd44780.h"

FILE lcd_str = FDEV_SETUP_STREAM(lcd_putchar, NULL, _FDEV_SETUP_WRITE); 

// global variables
float temp_std = 22.0; //temperature set by the user
float I; // internal temperature
float E; // external temperature
int furnace;
int mode;

int main(void) {
    
    DDRB |= (1 << DDB0); // LED out
    DDRC &= ~(1 << DDC3); //button 1
    DDRC &= ~(1 << DDC4); // button 2
    DDRC &= ~(1 << DDC5); // button 3
    PORTC |= (1 << PORTC3);// turnoff pull ups
    PORTC |= (1 << PORTC4);
    PORTC |= (1 << PORTC5); //ADC initialization
    
    ADCSRA |= (1 << ADPS1) | (1 << ADPS0);
    ADCSRA |= (1 << ADEN);
    DIDR0 |= (1 << ADC0D);
    
    lcd_init();
    
    while (1) {
        I = sensor_internal(); // interior sensor calculations; out at I
        E = sensor_external(); // external sensor calculations; out at E
        furnace1(); // furnace furnace
        print(); // print function
        input(); // input function
    }
}

float sensor_external() //external temperature 
{

    float volt_ref = 5.0; //reference voltage used to calculate the external temperature
    float temp_var;
    ADMUX = 0;
    ADMUX |= (1 << REFS0); //set REFSO to 1
    ADCSRA |= (1 << ADSC); //set ADSC to 1
    while (!(ADCSRA & (1 << ADIF))) // this while loop 
    {}
    ADCSRA |= (1 << ADIF); //set ADRF to one 
    temp_var = (ADC * volt_ref * 100) / 1024; //calculation for the external temperature 
    return (temp_var); //returning external temperature to main
}

float sensor_internal() //internal temperature 
{
    int m = 1; // slope
    float c = 50; //calculated intercept from y= m x+c
    float temp_var = 0;

    ADMUX = 0;
    ADMUX = ((1 << REFS1)| (1 << REFS0)| (1 << MUX3));
    ADCSRA |= (1 << ADSC);

    while (!(ADCSRA & (1 << ADIF))) {}
    ADCSRA |= (1 << ADIF);

    temp_var = (m * ADC) - c; //calibrated value of the internal temperature 
    
    return (temp_var); //returns internal temperature to main
}

void furnace1() //furnace working
{
    if ((mode % 2 == 0)&&(temp_std > (E + 1))) //if mode check is even, then exterior sensor
    {
        furnace = 1;
        PORTB |= (1 << PORTB0); //lights LED
    }
    if ((mode % 2 != 0)&&(temp_std > (I + 1))) //if mode check is even, then interior sensor
    {
        furnace = 1;
        PORTB |= (1 << PORTB0);
    }
    if ((mode % 2 == 0) && (temp_std < (E - 1))) {
        furnace = 0;
        PORTB &= ~(1 << PORTB0); //turns LED off
    }
    if ((mode % 2 != 0) && (temp_std < (I - 1))) {
        furnace = 0;
        PORTB &= ~(1 << PORTB0);
    }
}

void input() {
    if (!(PINC & (1 << PINC3))) //if switch 1 is pressed
    {
        while ((!(PINC & (1 << PINC3)))); //user input
        if (temp_std < 32) //temp within 32 degrees
        {
            temp_std++; //increments set temperature by one
        }
    }
    if (!(PINC & (1 << PINC4))) {
        while ((!(PINC & (1 << PINC4))));
        if (temp_std > 10) {
            temp_std--;
        }
    }
    if (!(PINC & (1 << PINC5))) {
        while ((!(PINC & (1 << PINC5))));
        mode++;
        //even = external
        //odd = internal
    }
}

void print() // print the desired format
{
    if (mode % 2 == 0) {
        fprintf(&lcd_str, "%3.1f\xDF", I);
        fprintf(&lcd_str, "C");
        fprintf(&lcd_str, "-->%3.1f\xDF", E);
        fprintf(&lcd_str, "C\x1b\xC0");
        fprintf(&lcd_str, "   %3.1f\xDF", temp_std);

    } else if (mode % 2 != 0) {
        fprintf(&lcd_str, "%3.1f\xDF", I);
        fprintf(&lcd_str, "C");
        fprintf(&lcd_str, "<--%3.1f\xDF", E);
        fprintf(&lcd_str, "C\x1b\xC0");
        fprintf(&lcd_str, "   %3.1f\xDF", temp_std);
    }
    if (furnace == 1) {
        fprintf(&lcd_str, "C   ON");
        fprintf(&lcd_str, "\x1b\x80");
    } else if (furnace == 0) {
        fprintf(&lcd_str, "C  OFF");
        fprintf(&lcd_str, "\x1b\x80");
    }
}