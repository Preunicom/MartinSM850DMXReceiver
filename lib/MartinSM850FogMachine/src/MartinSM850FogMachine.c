#include "MartinSM850FogMachine.h"
#include "avr/io.h"

/*
 * Initializes the DIN related Pins.
 */
void init_DIN() {
    // Set Fog_Relais to output (Pin D8)
    DDRB |= (1<<DDB0);
}

/*
 * Sets the Fog_Relais and Fogging_LED to ON (Pin D8)
 */
void fog() {
    PORTB |= (1<<PORTB0);
}

/*
 * Sets the Fog_Relais and Fogging_LED to OFF (Pin D8)
 */
void stop_fog() {
    PORTB &= ~(1<<PORTB0);
}