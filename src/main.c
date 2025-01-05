#include <util/delay.h>
#include "../lib/DMX_Switch/src/DMX_Switch.h"
#include "../lib/MartinSM850FogMachine/src/MartinSM850FogMachine.h"

int main() {
    init_DMX();
    init_DIN();
    while(1) {
        if(get_DMX_value() > 250 && is_valid_DMX_address()) {
            // DMX value "On" path
            fog();
        } else {
            // DMX value "Off" or DMX address invalid path
            stop_fog();
        }
        // Perfect would be 1 ms because 250K Baud and 512 Channel per Universe.
        // This is very fast, and we don't need this high speed,
        // because we don't change the signal at this high rates with a fader.
        // Even if the signal change reaction would be delayed by 20 ms we won't recognize it,
        // and it reduces a lot of load on the microcontroller.
        _delay_ms(20);
    }
}