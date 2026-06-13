/******************************************************************************
* morse-driver.h
*
* Header file morse-driver package.
*
******************************************************************************/
#ifndef MORSE_DRIVER_H_
#define MORSE_DRIVER_H_

#define MAX_BUFFER_SIZE 1024
#define BLOCK_SIZE 4096

#define GPIO_BASE 0xFE200000

// offsets are in bytes, but we are counting by 32 bits
#define GPIO_FSEL2_OFFSET (0x08 / 4)   // we are only using GPIO pins 23 and 24
#define GPIO_SET_OFFSET   (0x1C / 4)   // selects pins to set (high)
#define GPIO_CLEAR_OFFSET (0x28 / 4)   // selects pins to clear (low)
#define GPIO_READ_OFFSET  (0x34 / 4)   // 0 for low, 1 for high

#define OUTPUT_PIN_MASK   (1 << 23)   // using pin 23 for output
#define INPUT_PIN_MASK    (1 << 24)   // using pin 24 for input

#define MORSE_TIME_UNIT  200  // milliseconds

char* morseSignalLookUp[128] = {
    // ASCII values 0 - 31 are control characters. Ignore these.
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, 
    NULL,NULL,NULL,NULL,
    "00",       // SPACE (32)
    // ASCII values 33 - 64 are unsupported characters. Ignore these.
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, 
    NULL,NULL,NULL,NULL,
    "101110",      // A (65) .-
    "1110101010",  // B (66) -...
    // unsupported characters
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, 
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, 
    NULL
    };

#endif /* MORSE_DRIVER_H_ */
