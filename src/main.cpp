#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#define F_CPU 16000000UL

#define BAUDRATE 250000
#define BAUD_CONST (((F_CPU/(BAUDRATE*16UL)))-1)

#define START_BIT 0b11111111

volatile uint8_t DMX_value = 0;
volatile uint16_t DMX_address = 1;

//ToDo: Move UART in header file?

ISR(USART_RX_vect) {
    static uint16_t counter = 0;
    uint8_t received_value = UDR0;
    if(DMX_value != START_BIT) {
        counter++;
    } else {
        counter = 0;
    }
    if(counter == DMX_address) {
        DMX_value = received_value;
    }
}

void init_UART() {
    //Set BAUD rate
    UBRR0H = (unsigned char)(BAUD_CONST>>8);
    UBRR0L = (unsigned char)BAUD_CONST;
    // Set frame format: 8 data, 2 stop bit
    UCSR0C = (1<<USBS0)|(3<<UCSZ00);
    // Activate RX Complete Interrupts
    UCSR0B = (1<<RXCIE0);
}

int main() {
    init_UART();
    sei();
    //ToDo: Set initial DMX dddress
    while(1) {
        //ToDo: Set DMX address --> BTN + Interrupt / UART ISR couter = 0 case
        if(DMX_value > 200) {
            // On path
            //ToDo: Martin Protocol (/LED for testing)
            ;
        } else {
            // Off path
            ;;
        }
        _delay_ms(20); //ToDo: Set proper delay time
    }
}