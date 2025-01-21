#include "buttons.hpp"

uint8_t read_buttons() {
    // Check status of 4 buttons connected to Port D pins 0-3
    // Returns the index (0-3) of the first pressed button, or 255 if no button is pressed
    for(uint8_t i = 0; i < 4; i++) {
        // Check if button i is pressed by checking if its bit in PIND is 0
        // PIND & (1 << i) creates a mask to isolate the button's pin
        // Buttons are active low, meaning a pressed button reads as 0
        if(!(PIND & (1 << i))) { 
            _delay_ms(50);  
            return i;      
        }
    }
    return 255;  
}

void buttons_init() {
    // Configure pins 0-3 on Port D for button inputs
    
    // Set pins as inputs by clearing bits 0-3 in DDRD (Data Direction Register)
    DDRD &= ~((1 << PD0) | (1 << PD1) | (1 << PD2) | (1 << PD3));
    
    // Enable internal pull-up resistors on these pins by setting bits in PORTD
    // This ensures pins read high when buttons are not pressed
    PORTD |= (1 << PD0) | (1 << PD1) | (1 << PD2) | (1 << PD3);
}