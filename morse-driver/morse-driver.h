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

// . = high for one unit of time
// - = high for three units of time
// Space between high signals is low for one unit of time
// Space between characters is low for 3 units of time
// Space between words is low for 7 units of time

char* morseSignalLookUp[128] = {
    // ASCII values 0 - 31 are control characters. Ignore these.
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, 
    NULL,NULL,NULL,NULL,
    
    "0000",              // SPACE (32)
    
    // ASCII values 33 - 64 are unsupported characters. Ignore these.
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, 
    NULL,NULL,NULL,NULL,
    
    "10111000",          // A (65) .-
    "111010101000",      // B (66) -...
    "11101011101000",    // C (67) -.-.
    "1110101000",        // D (68) -..
    "1000",              // E (69) .
    "101011101000",      // F (70) ..-.
    "111011101000",      // G (71) --.
    "1010101000",        // H (72) ....
    "101000",            // I (73) ..
    "1011101110111000",  // J (74) .---
    "111010111000",      // K (75) -.-
    "101110101000",      // L (76) .-..
    "1110111000",        // M (77) --
    "11101000",          // N (78) -.
    "11101110111000",    // O (79) ---
    "10111011101000",    // P (80) .--.
    "1110111010111000",  // Q (81) --.-
    "1011101000",        // R (82) .-.
    "10101000",          // S (83) ...
    "111000",            // T (84) -
    "1010111000",        // U (85) ..-
    "101010111000",      // V (86) ...-
    "101110111000",      // W (87) .--
    "11101010111000",    // X (88) -..-
    "1110101110111000",  // Y (89) -.--
    "11101110101000",    // Z (90) --..
    
    // ASCII values 91 - 96 are unsupported characters. Ignore these.
    NULL,NULL,NULL,NULL,NULL,NULL,
    
    "10111000",          // a (97) .-
    "111010101000",      // b (98) -...
    "11101011101000",    // c (99) -.-.
    "1110101000",        // d (100) -..
    "1000",              // e (101) .
    "101011101000",      // f (102) ..-.
    "111011101000",      // g (103) --.
    "1010101000",        // h (104) ....
    "101000",            // i (105) ..
    "1011101110111000",  // j (106) .---
    "111010111000",      // k (107) -.-
    "101110101000",      // l (108) .-..
    "1110111000",        // m (109) --
    "11101000",          // n (110) -.
    "11101110111000",    // o (111) ---
    "10111011101000",    // p (112) .--.
    "1110111010111000",  // q (113) --.-
    "1011101000",        // r (114) .-.
    "10101000",          // s (115) ...
    "111000",            // t (116) -
    "1010111000",        // u (117) ..-
    "101010111000",      // v (118) ...-
    "101110111000",      // w (119) .--
    "11101010111000",    // x (120) -..-
    "1110101110111000",  // y (121) -.--
    "11101110101000",    // z (122) --..
    
    // ASCII values 123 - 127 are unsupported characters. Ignore these.
    NULL,NULL,NULL,NULL,NULL
    };

#endif /* MORSE_DRIVER_H_ */
