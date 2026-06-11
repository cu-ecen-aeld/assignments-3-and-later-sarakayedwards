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

#define GPIO_FSEL2_OFFSET 0x08   // we are only using GPIO pins 23 and 24
#define GPIO_SET_OFFSET   0x1C   // selects pins to set (high)
#define GPIO_CLEAR_OFFSET 0x28   // selects pins to clear (low)
#define GPIO_READ_OFFSET  0x34   // 0 for low, 1 for high

#define OUTPUT_PIN_MASK   (1 << 23)   // using pin 23 for output
#define INPUT_PIN_MASK    (1 << 24)   // using pin 24 for input

#endif /* MORSE_DRIVER_H_ */
