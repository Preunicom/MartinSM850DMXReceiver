#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include "../lib/DMX_Switch.h"
#include "../lib/MartinSM850FogMachine.h"

#define F_CPU 16000000UL

#define BAUDRATE 250000UL
#define BAUD_CONST (((F_CPU/(BAUDRATE*16UL)))-1)

volatile uint8_t DMX_value = 0;
volatile uint16_t DMX_address = 1;

#define DMX_SIGNAL_STATE_WAITING_FOR_BREAK 0b00000001
#define DMX_SIGNAL_STATE_BREAK_DETECTED 0b00000010
#define DMX_SIGNAL_STATE_RECEIVING_DATA 0b00000100
volatile uint8_t DMX_signal_state = DMX_SIGNAL_STATE_WAITING_FOR_BREAK;

/*
 * Initialize UART module.
 *      - 8 data, 2 stop-bit frame
 *      - RX Complete interrupts enabled
 */
void init_UART() {
    //Set BAUD rate
    UBRR0H = (unsigned char)(BAUD_CONST>>8);
    UBRR0L = (unsigned char)BAUD_CONST;
    // Set frame format: 8 data, 2 stop bit
    UCSR0C = (1<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00);
    // Activate RX Complete interrupts and UART receiver
    UCSR0B = (1<<RXCIE0)|(1<<RXEN0);
}

/*
 * Erases received DMX data from UART until we are on time with the signal
 */
void flush_DMX()
{
    volatile uint8_t voiddata;
    while (UCSR0A & (1 << RXC0)) {
        // Data available
        voiddata = UDR0; // get data
    }
}

/*
 * Initializes the DMX address DIP switch.
 * User Pins:
 *      PCINT19-22 (PortD)(D3-D6): Bit 0...3 of DMX address --> Interrupt PCINT2
 *      PCINT8-13 (PortC)(A0-A5): Bit 4...9 of DMX address --> Interrupt PCINT1
 */
void init_DIP_switch() {
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
 * Initializes the DIN related Pins.
 */
void init_DIN() {
    // Set Fog_Relais to output (Pin D8)
    DDRB |= (1<<DDB0);
}

/*
 * 1 is LED off because of pull-up
 */
#define SET_LED_INVALID() PORTD |= (1<<PORTD7);

/*
 * 0 is LED on because of pull-up
 */
#define SET_LED_VALID() PORTD &= ~(1<<PORTD7);

/*
 * Returns the DMX address set at the DIP switch.
 */
void set_DMX_address_from_DIP_switch() {
    // Enter critical section
    uint8_t old_SREG = SREG;
    cli();
    // Get bit 0...3 of DMX address in PinD at bit 3...6 (0 is set, because of pull-up)
    uint16_t dip_states = ~PIND;
    // Place these 4 bits at the end of DMX address
    DMX_address = (dip_states >> 3) & 0x000F;
    // Get bit 4...9 of DMX address in PinC at bit 0...5 (0 is set, because of pull-up)
    dip_states = ~PINC;
    // Place these 6 bits in front of the last 3 bits of DMX address
    DMX_address |= (dip_states << 4) & 0x03F0;
    // Set LED to new DMX address status
    if(DMX_address && DMX_address < 513) {
        // Address >0 and <513
        SET_LED_VALID();
    } else {
        // Address 0 or >512
        SET_LED_INVALID();
        DMX_address = 0; // Reduce load on Arduino by ignoring DMX data input because of invalid DMX address
    }
    // Exit critical section
    SREG = old_SREG;
}

/*
 * Change at DIP switch
 */
ISR(PCINT0_vect) {
    set_DMX_address_from_DIP_switch();
}

/*
 * Change at DIP switch
 */
ISR(PCINT1_vect) {
    set_DMX_address_from_DIP_switch();
}

/*
 * Received data from UART is set as current DMX value if package id matches current DMX address.
 */
ISR(USART_RX_vect) {
    static uint16_t dmx_address_counter = 0;

    // Get data from UART module
    uint8_t uart_has_frame_error = UCSR0A & (1<<FE0);
    uint8_t received_value = UDR0;

    if(DMX_address == 0) {
        // Invalid DMX address is set
        return;
    }

    if(uart_has_frame_error) {
        // Break found in DMX signal
        DMX_signal_state = DMX_SIGNAL_STATE_BREAK_DETECTED;
    } else if(DMX_signal_state & DMX_SIGNAL_STATE_BREAK_DETECTED) {
        // First DMX frame after break
        if(received_value == 0) {
            //Normal initial first DMX byte
            DMX_signal_state = DMX_SIGNAL_STATE_RECEIVING_DATA;
            dmx_address_counter = 0;
        } else {
            // Wait for next break because of special DMX command not implemented
            DMX_signal_state = DMX_SIGNAL_STATE_WAITING_FOR_BREAK;
        }
    } else if(DMX_signal_state & DMX_SIGNAL_STATE_RECEIVING_DATA) {
        // DMX frame received
        dmx_address_counter++;
        if (dmx_address_counter == DMX_address) {
            // Value for our DMX address
            DMX_value = received_value;
            DMX_signal_state = DMX_SIGNAL_STATE_WAITING_FOR_BREAK;
        }
    }
}

int main() {
    init_DIP_switch();
    init_UART();
    init_DIN();
    set_DMX_address_from_DIP_switch();
    flush_DMX();
    sei();
    while(1) {
        if(DMX_value > 250) {
            // DMX value "On" path
            //Fog_Relais ON (Pin D8)
            PORTB |= (1<<PORTB0);
        } else {
            // DMX value "Off" path
            // Fog_Relais OFF (Pin D8)
            PORTB &= ~(1<<PORTB0);
        }
        // Perfect would be 1 ms because 250K Baud and 512 Channel per Universe.
        // This is very fast, and we don't need this high speed,
        // because we don't change the signal at this high rates with a fader.
        // Even if the signal change reaction would be delayed by 20 ms we won't recognize it,
        // and it reduces a lot of load on the microcontroller.
        _delay_ms(20);
    }
}