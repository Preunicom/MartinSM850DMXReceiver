#include "DMX_Switch.h"
#include <avr/interrupt.h>
#include <avr/io.h>

// UART parameters
#define F_CPU 16000000UL
#define BAUD_RATE 250000UL
#define BAUD_CONST (((F_CPU/(BAUD_RATE*16UL)))-1)

// DMX protocol parameters
#define DMX_SIGNAL_STATE_WAITING_FOR_BREAK 0b00000001
#define DMX_SIGNAL_STATE_BREAK_DETECTED 0b00000010
#define DMX_SIGNAL_STATE_RECEIVING_DATA 0b00000100
volatile uint8_t _DMX_signal_state = DMX_SIGNAL_STATE_WAITING_FOR_BREAK;

// abstract DMX parameters
volatile uint8_t _DMX_value = 0;
volatile uint16_t _DMX_address = 1;

/*
 * Initialize UART module.
 *      - 8 data, 2 stop-bit frame
 *      - RX Complete interrupts enabled
 */
void _init_UART() {
    //Set BAUD rate
    UBRR0H = (unsigned char)(BAUD_CONST>>8);
    UBRR0L = (unsigned char)BAUD_CONST;
    // Set frame format: 8 data, 2 stop bit
    UCSR0C = (1<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00);
    // Activate RX Complete interrupts and UART receiver
    UCSR0B = (1<<RXCIE0)|(1<<RXEN0);
}

/*
 * Initializes the DMX address DIP switch.
 * User Pins:
 *      PCINT19-22 (PortD)(D3-D6): Bit 0...3 of DMX address --> Interrupt PCINT2
 *      PCINT8-13 (PortC)(A0-A5): Bit 4...9 of DMX address --> Interrupt PCINT1
 */
void _init_DIP_switch() {
    // Set address valid LED to output (Pin D7)
    DDRD |= (1 << DDD7);
    // Set pull-up for DIP switch input pins
    PORTC |= (1 << PORTC5) | (1 << PORTC4) | (1 << PORTC3) | (1 << PORTC2) | (1 << PORTC1) | (1 << PORTC0);
    PORTD |= (1 << PORTD6) | (1 << PORTD5) | (1 << PORTD4) | (1 << PORTD3);
    // Enable interrupt pin bank
    PCICR |= (1 << PCIE2) | (1 << PCIE1);
    // Enable pins for interrupt
    PCMSK1 |= (1 << PCINT13) | (1 << PCINT12) | (1 << PCINT11) | (1 << PCINT10) | (1 << PCINT9) | (1 << PCINT8);
    PCMSK2 |= (1 << PCINT22) | (1 << PCINT21) | (1 << PCINT20) | (1 << PCINT19);
}

/*
 * Erases received DMX data from UART until we are on time with the signal
 */
void _flush_DMX()
{
    volatile uint8_t voiddata;
    while (UCSR0A & (1 << RXC0)) {
        // Data available
        voiddata = UDR0; // get data
    }
}

/*
 * Returns the DMX address set at the DIP switch.
 */
void _set_DMX_address_from_DIP_switch() {
    // Enter critical section
    uint8_t old_SREG = SREG;
    cli();
    // Get bit 0...3 of DMX address in PinD at bit 3...6 (0 is set, because of pull-up)
    uint16_t dip_states = ~PIND;
    // Place these 4 bits at the end of DMX address
    _DMX_address = (dip_states >> 3) & 0x000F;
    // Get bit 4...9 of DMX address in PinC at bit 0...5 (0 is set, because of pull-up)
    dip_states = ~PINC;
    // Place these 6 bits in front of the last 3 bits of DMX address
    _DMX_address |= (dip_states << 4) & 0x03F0;
    // Set LED to new DMX address status
    if(_DMX_address && _DMX_address < 513) {
        // Address >0 and <513
        PORTD &= ~(1<<PORTD7); // Valid_Address_LED ON
    } else {
        // Address 0 or >512
        PORTD |= (1<<PORTD7); // Valid_Address_LED OFF
        _DMX_address = 0; // Reduce load on Arduino by ignoring DMX data input because of invalid DMX address
    }
    // Exit critical section
    SREG = old_SREG;
}

/*
 * Initialize DMX
 *      - UART module
 *      - DIP Switch (address)
 */
void init_DMX() {
    _init_DIP_switch();
    _init_UART();
    _set_DMX_address_from_DIP_switch();
    _flush_DMX();
    sei();
}

/*
 * Returns a copy of the current DMX value
 */
uint8_t get_DMX_value() {
    return _DMX_value;
}

/*
 * Returns 1 if DMX address is valid, 0 otherwise
 */
uint8_t is_valid_DMX_address() {
    return (_DMX_address != 0);
}

/*
 * Change at DIP switch
 */
ISR(PCINT0_vect) {
        _set_DMX_address_from_DIP_switch();
}

/*
 * Change at DIP switch
 */
ISR(PCINT1_vect) {
        _set_DMX_address_from_DIP_switch();
}

/*
 * Received data from UART is set as current DMX value if package id matches current DMX address.
 */
ISR(USART_RX_vect) {
        static uint16_t dmx_address_counter = 0;

        // Get data from UART module
        uint8_t uart_has_frame_error = UCSR0A & (1<<FE0);
        uint8_t received_value = UDR0;

        if(_DMX_address == 0) {
            // Invalid DMX address is set
            return;
        }

        if(uart_has_frame_error) {
            // Break found in DMX signal
            _DMX_signal_state = DMX_SIGNAL_STATE_BREAK_DETECTED;
        } else if(_DMX_signal_state & DMX_SIGNAL_STATE_BREAK_DETECTED) {
            // First DMX frame after break
            if(received_value == 0) {
                //Normal initial first DMX byte
                _DMX_signal_state = DMX_SIGNAL_STATE_RECEIVING_DATA;
                dmx_address_counter = 0;
            } else {
                // Wait for next break because of special DMX command not implemented
                _DMX_signal_state = DMX_SIGNAL_STATE_WAITING_FOR_BREAK;
            }
        } else if(_DMX_signal_state & DMX_SIGNAL_STATE_RECEIVING_DATA) {
            // DMX frame received
            dmx_address_counter++;
            if (dmx_address_counter == _DMX_address) {
                // Value for our DMX address
                _DMX_value = received_value;
                _DMX_signal_state = DMX_SIGNAL_STATE_WAITING_FOR_BREAK;
            }
        }
}