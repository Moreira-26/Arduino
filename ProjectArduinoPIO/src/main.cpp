#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "HD44780.hpp"
#include "keypad.hpp"
#include "buttons.hpp"
#include "uart_buffer.hpp"

#define MAX_ACCOUNTS 10
#define ID_LENGTH 4
#define PIN_LENGTH 4
#define BUTTON_LEFT 3
#define BUTTON_RIGHT 2
#define BUTTON_ENTER 1
#define BUTTON_BACK 0

// Structure to store account information
struct Account {
    char id[ID_LENGTH + 1]; //ID composed by 4 characters + /0
    char pin[PIN_LENGTH + 1]; //PIN composed by 4 characters + /0
    uint32_t balance; //Amount of money in account
};

// Global variables
struct Account accounts[MAX_ACCOUNTS]; //Array with accounts information
uint8_t current_account_count = 0;

/**
 * Gets input from keypad
 * @param buffer: Buffer to store the input
 * @param length: Maximum length of input expected
 * Displays input on LCD second line and handles backspace
 */
void get_input(char *buffer, uint8_t length) {
    uint8_t pos = 0;
    LCD_GoTo(0, 1);
    
    // Clear the second line first
    LCD_GoTo(0, 1);
    for(uint8_t i = 0; i < 16; i++) {
        LCD_WriteData(' ');
    }
    LCD_GoTo(0, 1);
    
    // Clear the buffer first
    for(uint8_t i = 0; i < length + 1; i++) {
        buffer[i] = '\0';
    }
    
    // Wait for key release
    while(scan_keypad() != 0) {
        _delay_ms(10);
    }
    _delay_ms(200);  
    
    while(pos < length) {
        char key = scan_keypad();
        uint8_t buttonPressed = read_buttons();
        if(key != 0) {
            if(key >= '0' && key <= '9') {
                buffer[pos] = key;
                LCD_GoTo(pos, 1);
                LCD_WriteData(key);  
                pos++;
                
                // Wait for key release before accepting next input
                while(scan_keypad() != 0) {
                    _delay_ms(10);
                }
                _delay_ms(100);
            }
        } else if (buttonPressed == BUTTON_BACK) {
            if (pos > 0) {
                pos--;
                buffer[pos] = '\0';
                LCD_GoTo(pos, 1); 
                LCD_WriteData(' '); 
                LCD_GoTo(pos, 1); 
            }
        }
    }
    buffer[length] = '\0';
}

// Write static text to LCD 
void write_static_text(uint8_t text_id) {
    switch(text_id) {
        case 0:
            LCD_WriteText("1.Create Account");
            break;
        case 1:
            LCD_WriteText("2.Check Balance");
            break;
        case 2:
            LCD_WriteText("3.Deposit Money");
            break;
        case 3:
            LCD_WriteText("4.Withdraw Money");
            break;
        case 4:
            LCD_WriteText("Enter ID:");
            break;
        case 5:
            LCD_WriteText("Enter PIN:");
            break;
        case 6:
            LCD_WriteText("Amount: $");
            break;
    }
}

/**
 * Check if id exits in accounts array
 */
bool id_exists(const char* id) {
    for(uint8_t i = 0; i < current_account_count; i++) {
        if(strcmp(accounts[i].id, id) == 0) return true;
    }
    return false;
}


/**
 * Creates a new bank account
 * - Checks if maximum account limit is reached
 * - Gets ID and PIN from user
 * - Checks if ID already exists
 * - Initializes balance to 0
 * - Stores account in accounts array
 */
void create_account() {
    if(current_account_count >= MAX_ACCOUNTS) {
        LCD_Clear();
        LCD_GoTo(0,0);
        LCD_WriteText("Max accounts");
        _delay_ms(2000);
        return;
    }
    

    struct Account new_account;
    
    LCD_Clear();
    LCD_GoTo(0,0);
    write_static_text(4);  // "Enter new ID:"
    get_input(new_account.id, ID_LENGTH);

    if(id_exists(new_account.id)) {
        LCD_Clear();
        LCD_GoTo(0,0);
        LCD_WriteText("ID already exists!");
        _delay_ms(2000);
        return;
    }
    
    _delay_ms(500);
    LCD_Clear();
    LCD_GoTo(0,0);
    write_static_text(5);  // "Enter PIN:"
    get_input(new_account.pin, PIN_LENGTH);
    
    new_account.balance = 0;
    
    accounts[current_account_count++] = new_account;
    
    LCD_Clear();
    LCD_GoTo(0,0);
    LCD_WriteText("Done!");
    _delay_ms(2000);
}

/**
 * Validates user credentials
 * @param entered_id: ID entered by user
 * @param entered_pin: PIN entered by user
 * @return: Index of matching account or 255 if invalid
 */
uint8_t login(char *entered_id, char *entered_pin) {
    for(uint8_t i = 0; i < current_account_count; i++) {
        if(strcmp(accounts[i].id, entered_id) == 0 && strcmp(accounts[i].pin, entered_pin) == 0) {
            return i;
        }
    }
    return 255;
}

/**
 * Gets user credentials and validates them
 * @return: Account index if login successful, 255 if failed
 */
uint8_t get_user_credentials() {
    char id[ID_LENGTH + 1];
    char pin[PIN_LENGTH + 1];
    
    LCD_Clear();
    LCD_GoTo(0,0);
    write_static_text(4);  // "Enter ID:"
    get_input(id, ID_LENGTH);
    
    LCD_Clear();
    LCD_GoTo(0,0);
    write_static_text(5);  // "Enter PIN:"
    get_input(pin, PIN_LENGTH);
    
    return login(id, pin);
}

/**
 * Gets monetary amount from user input
 * @return: Amount entered as uint32_t, or -1 if invalid
 * - Handles numeric input up to 6 digits
 * - Validates input range
 * - Supports backspace and enter for confirmation
 */
uint32_t get_amount() {
    char amount_str[10];
    uint8_t pos = 0;
    
    // Clear the buffer first
    for(uint8_t i = 0; i < 7; i++) {
        amount_str[i] = '\0';
    }
    
    // Clear the second line of LCD
    LCD_GoTo(0, 1);
    for(uint8_t i = 0; i < 16; i++) {
        LCD_WriteData(' ');
    }
    LCD_GoTo(0, 1);
    
    // Wait for key release
    while(scan_keypad() != 0) {
        _delay_ms(10);
    }
    _delay_ms(200);
    
    char key;
    uint8_t buttonPressed;
    
    while(pos < 9) {
        key = scan_keypad();
        buttonPressed = read_buttons();
        if(key != 0) {
            if(key >= '0' && key <= '9') {
                amount_str[pos] = key;
                LCD_WriteData(key);
                pos++;
                
                while(scan_keypad() != 0) {
                    _delay_ms(10);
                }
                _delay_ms(100);
            }
        } else if (buttonPressed == BUTTON_ENTER && pos > 0) {
            break;
        } else if (buttonPressed == BUTTON_BACK) {
            if (pos > 0) {
                pos--;
                amount_str[pos] = '\0';
                LCD_GoTo(pos, 1);
                LCD_WriteData(' '); 
                LCD_GoTo(pos, 1); 
            }
        }
    }
    
    amount_str[pos] = '\0';
    char *endptr;
    long amount = strtol(amount_str, &endptr, 10);
    
    // Check for invalid input or out-of-range values
    if (*endptr != '\0' || amount > INT32_MAX || amount < 0) {
        return -1; 
    }

    return (uint32_t)amount;
}

/**
 * Handles money deposit operation
 * - Authenticates user
 * - Gets deposit amount
 * - Updates account balance
 * - Shows success/failure message
 */
void deposit_money() {
    uint8_t acc_index = get_user_credentials();
    if(acc_index != 255) {
        LCD_Clear();
        LCD_GoTo(0,0);
        LCD_WriteText("Amount: $");
        uint32_t amount = get_amount();
        
        accounts[acc_index].balance += amount;
        
        LCD_Clear();
        LCD_GoTo(0,0);
        LCD_WriteText("Deposit success!");
        _delay_ms(2000);
    } else {
        LCD_Clear();
        LCD_GoTo(0,0);
        LCD_WriteText("Invalid login!");
        _delay_ms(2000);
    }
}



/**
 * Handles money withdrawal operation
 * - Authenticates user
 * - Gets withdrawal amount
 * - Checks for sufficient funds
 * - Updates balance if funds available
 */
void withdraw_money() {
    
    uint8_t acc_index = get_user_credentials();
    if(acc_index != 255) {
        LCD_Clear();
        LCD_GoTo(0,0);
        LCD_WriteText("Amount: $");
        uint32_t amount = get_amount();
        
        if(amount <= accounts[acc_index].balance) {
            accounts[acc_index].balance -= amount;
            LCD_Clear();
            LCD_GoTo(0,0);
            LCD_WriteText("Withdraw success!");
        } else {
            LCD_Clear();
            LCD_GoTo(0,0);
            LCD_WriteText("Insufficient");
            LCD_GoTo(0,1);
            LCD_WriteText("funds!");
        }
        _delay_ms(2000);
    } else {
        LCD_Clear();
        LCD_GoTo(0,0);
        LCD_WriteText("Invalid login!");
        _delay_ms(2000);
    }
}


/**
 * Displays account balance
 * - Authenticates user
 * - Shows current balance
 * - Displays error if login invalid
 */
void check_balance() {
    
    uint8_t acc_index = get_user_credentials();
    if(acc_index != 255) {
        char balance_str[10];
        LCD_Clear();
        LCD_GoTo(0,0);
        LCD_WriteText("Balance: $");
        sprintf(balance_str, "%ld", accounts[acc_index].balance); 
        LCD_GoTo(0,1);
        LCD_WriteText(balance_str);
        _delay_ms(3000);
    } else {
        LCD_Clear();
        LCD_GoTo(0,0);
        LCD_WriteText("Invalid!");
        _delay_ms(2000);
    }
}


int main(void) {
    buttons_init();
    LCD_Initalize();
    keypad_init();

    uint8_t menu_position = 0;
    bool change_menu_static = true;


    while(1) {
        if(change_menu_static) {
            LCD_Clear();
            LCD_GoTo(0,0);
            write_static_text(menu_position);
            change_menu_static = false;
        }
        
        uint8_t buttonPressed = read_buttons();
        
        if(buttonPressed != 255) {
            switch(buttonPressed) {
                case BUTTON_RIGHT:
                    menu_position = (menu_position + 1) % 4;
                    change_menu_static = true;
                    _delay_ms(200);  
                    break;
                    
                case BUTTON_LEFT:
                    if(menu_position == 0) {
                        menu_position = 3;
                    } else {
                        menu_position--;
                    }
                    change_menu_static = true;
                    _delay_ms(200); 
                    break;
                    
                case BUTTON_ENTER:
                    switch(menu_position) {
                        case 0:
                            create_account();
                            break;
                        case 1:
                            check_balance();
                            break;
                        case 2:
                            deposit_money();
                            break;
                        case 3:
                            withdraw_money();
                            break;
                    }
                    change_menu_static = true;  
                    break;
            }
        }
    }
    
    return 0;
}
