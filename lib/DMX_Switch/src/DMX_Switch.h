#ifndef MARTINSM850DMXRECIEVER_DMX_SWITCH_H
#define MARTINSM850DMXRECIEVER_DMX_SWITCH_H

#include "avr/io.h"

void init_DMX();
uint8_t get_DMX_value();
uint8_t is_valid_DMX_address();

#endif //MARTINSM850DMXRECIEVER_DMX_SWITCH_H
