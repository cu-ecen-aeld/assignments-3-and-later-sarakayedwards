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
    
    "00",              // SPACE (32)
    
    // ASCII values 33 - 64 are unsupported characters. Ignore these.
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, 
    NULL,NULL,NULL,NULL,
    
    "101110",          // A (65) .-
    "1110101010",      // B (66) -...
    "111010111010",    // C (67) -.-.
    "11101010",        // D (68) -..
    "10",              // E (69) .
    "1010111010",      // F (70) ..-.
    "1110111010",      // G (71) --.
    "10101010",        // H (72) ....
    "1010",            // I (73) ..
    "10111011101110",  // J (74) .---
    "1110101110",      // K (75) -.-
    "1011101010",      // L (76) .-..
    "11101110",        // M (77) --
    "111010",          // N (78) -.
    "111011101110",    // O (79) ---
    "101110111010",    // P (80) .--.
    "11101110101110",  // Q (81) --.-
    "10111010",        // R (82) .-.
    "101010",          // S (83) ...
    "1110",            // T (84) -
    "10101110",        // U (85) ..-
    "1010101110",      // V (86) ...-
    "1011101110",      // W (87) .--
    "111010101110",    // X (88) -..-
    "11101011101110",  // Y (89) -.--
    "111011101010",    // Z (90) --..
    
    // ASCII values 91 - 96 are unsupported characters. Ignore these.
    NULL,NULL,NULL,NULL,NULL,NULL,
    
    "101110",          // a (97) .-
    "1110101010",      // b (98) -...
    "111010111010",    // c (99) -.-.
    "11101010",        // d (100) -..
    "10",              // e (101) .
    "1010111010",      // f (102) ..-.
    "1110111010",      // g (103) --.
    "10101010",        // h (104) ....
    "1010",            // i (105) ..
    "10111011101110",  // j (106) .---
    "1110101110",      // k (107) -.-
    "1011101010",      // l (108) .-..
    "11101110",        // m (109) --
    "111010",          // n (110) -.
    "111011101110",    // o (111) ---
    "101110111010",    // p (112) .--.
    "11101110101110",  // q (113) --.-
    "10111010",        // r (114) .-.
    "101010",          // s (115) ...
    "1110",            // t (116) -
    "10101110",        // u (117) ..-
    "1010101110",      // v (118) ...-
    "1011101110",      // w (119) .--
    "111010101110",    // x (120) -..-
    "11101011101110",  // y (121) -.--
    "111011101010",    // z (122) --..
    
    // ASCII values 123 - 127 are unsupported characters. Ignore these.
    NULL,NULL,NULL,NULL,NULL
    };

#endif /* MORSE_DRIVER_H_ */
