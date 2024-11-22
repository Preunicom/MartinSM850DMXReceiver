# MartinSM850DMXReceiver
Adapter from DMX to Martin fog control for Martin SM850

## Hardware
### Components
#### Microcontroller
> Arduino Pro Mini (ATmega328P) with 5V and 16MHz.

#### RS485 Converter
> MAX485

#### Other components
> 10 channel DIP switch --> Input for DMX address

> LED green --> In-/Valid DMX address set

> LED blue --> Power

> 220 Ohm resistor --> Current limiting resistors for LEDs

> 120 Ohm resistor --> Termination resistor

> 100 nF capacitor --> Power stabilisation for MAX485 (coupling capacitor)

### Circuit design
#### DIP Switch
```
Arduino[D3] <-> DIP[1] <-> GND
Arduino[D4] <-> DIP[2] <-> GND
Arduino[D5] <-> DIP[3] <-> GND
Arduino[D6] <-> DIP[4] <-> GND
    
Arduino[A0] <-> DIP[5] <-> GND
Arduino[A1] <-> DIP[6] <-> GND
Arduino[A2] <-> DIP[7] <-> GND
Arduino[A1] <-> DIP[8] <-> GND
Arduino[A4] <-> DIP[9] <-> GND
Arduino[A5] <-> DIP[10] <-> GND
```
#### Status LEDs
```
Arduino[D7] <-> LED green <-> 220Ohm resistor <-> +5V
Arduino[VCC] <-> LED blue <-> 220Ohm resistor <-> GND
```

#### MAX485
```
MAX485[VCC] <-> ARDUINO[VCC]
MAX485[VCC] <-> 100nF capacitor <-> MAX485[GND]
MAX485[B] <-> DMX[2]
MAX485[A] <-> 120Ohm resistor <-> MAX[B]
MAX485[A] <-> DMX[3]
MAX485[GND] <-> Arduino[GND]
MAX485[GND] <-> DMX[1]

MAX485[RO] <-> Arduino[RXI]
MAX485[!RE] <-> Arduino[GND]
MAX485[DE] <-> Arduino[GND]
MAX485[DI]: <unconnected>
```