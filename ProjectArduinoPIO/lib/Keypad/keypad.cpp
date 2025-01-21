#include "keypad.hpp"

const char keypad[4][3] = {
    {'1', '2', '3'},    // Connected to PC2
    {'4', '5', '6'},    // Connected to PC3
    {'7', '8', '9'},    // Connected to PC4
    {'*', '0', '#'}     // Connected to PC5
};

// Initialize keypad
// Columns: PB5, PB4, PB3 configured as inputs with pull-up resistors
void keypad_init() {
    // Set row pins as outputs
    DDRC |= (1 << PC5) | (1 << PC4) | (1 << PC3) | (1 << PC2);
    
    // Set column pins as inputs
    DDRB &= ~((1 << PB5) | (1 << PB4) | (1 << PB3));
    
    // Enable internal pull-up resistors for column pins
    PORTB |= (1 << PB5) | (1 << PB4) | (1 << PB3);
}

// Scan keypad
char scan_keypad() {
    uint8_t row;
    for(row = 0; row < 4; row++) {
        // Set all rows high
        PORTC |= (1 << PC5) | (1 << PC4) | (1 << PC3) | (1 << PC2);
        // Set current row low (active)
        // PC2 is top row (row 0), PC5 is bottom row (row 3)
        PORTC &= ~(1 << (PC2 + row));
        _delay_us(10);
        
        // Check each column for the current row
        if(!(PINB & (1 << PB5))) return keypad[row][0];  // Left column
        if(!(PINB & (1 << PB4))) return keypad[row][1];  // Middle column
        if(!(PINB & (1 << PB3))) return keypad[row][2];  // Right column
    }
    return 0;
}