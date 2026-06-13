/******************************************************************************
* test.c
*
* Test module for the morse-driver package.
*
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <sys/mman.h>
#include "morse-driver.h"


// these are 0s and 1s representing one time unit the signal sent/received
// Ideally, this will be a circular buffer. The application thread writes to
// outSignal and reads from inSignal. Separate send and receive threads read 
// from outSignal and write to readSignal.

struct signalBuffer {
    char buff[MAX_BUFFER_SIZE];
    int head;
    int tail;
};

// pre-load with a hard-coded message
struct signalBuffer outSignal;
struct signalBuffer inSignal;
volatile uint32_t* gpiomem;

int initHW(void) {
    void* memmap;
    int memfd;
    
    syslog(LOG_USER|LOG_DEBUG, "test: running test...");  

    if ((memfd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
        syslog(LOG_USER|LOG_DEBUG, "test: open failed, errno = %d", errno);
        return -1;
    }
    
    syslog(LOG_USER|LOG_DEBUG, "test: open success, fd = %d", memfd);  

    // map virtual address space for pins
    if ((memmap = mmap(NULL, 
                  BLOCK_SIZE, 
                  PROT_READ | PROT_WRITE, 
                  MAP_SHARED, 
                  memfd, 
                  GPIO_BASE)) == MAP_FAILED) {
        syslog(LOG_USER|LOG_DEBUG, "test: memmap failed, errno = %d", errno);
        close(memfd); 
        return -1;
    }
                  
    syslog(LOG_USER|LOG_DEBUG, "test: mmap success");  

    close(memfd);
    
    // this is our base address to which we add our offsets
    gpiomem = (uint32_t*)memmap;
    
    // set the pin as an output by setting the mode bits to 001 for write
    syslog(LOG_USER|LOG_DEBUG, "test: setting pin function");  

    *(gpiomem + GPIO_FSEL2_OFFSET) &= ~(7 << 9);
    *(gpiomem + GPIO_FSEL2_OFFSET) |= (1 << 9);

    return 0;
}

void deinitHW(void) {
    munmap((void*)gpiomem, BLOCK_SIZE);
}    

/******************************************************************************
* morse_test()
*
* No parameters: Output a square wave with a two-second wavelength (1 second
*                  high, one second low) on the output pin indefinitely.
*                Read the input pin at half second intervals and output it 
*                 (0 for low, 1 for high) to stdout.
* 
* -wN, where N is a number 1 - 5000:
*                Output a square wave with a 2N millisecond wavelength (N ms
*                  high, N ms low) on the output pin.
*                Read the input pin at N/2 ms intervals and output the first 
*                 MAX_OUTPUTS readings (0 for low, 1 for high) to stdout.
*
* -nN, where N is a number 1 - 5000:
*                Output N square waves on the output pin.
*                Read the input pin at the specified interval 2N times.
*
******************************************************************************/
void morse_test(char* inStr) {

    int i;
    int strLen = strlen(inStr);
    int signalLen;
    int success;

    struct timespec timeUnit;
    struct timespec timeLeft;
    
    
    outSignal.head = 0;
    outSignal.tail = 0;

    // For each character in the string, look up the encoded signal to send
    // and add it to the buffer in outSignal.
    for (i = 0; i < strLen; i++) {
        if (morseSignalLookUp[(uint8_t)inStr[i]] != NULL) {
            signalLen = strlen(morseSignalLookUp[(uint8_t)inStr[i]]);
            strncpy(&(outSignal.buff[outSignal.head]), morseSignalLookUp[(uint8_t)inStr[i]], signalLen);
            outSignal.head += signalLen;
        }
    }
    
    // output the signal
    // *** This thread will be dedicated to sending this signal until it is complete. ***
    // For future dev: Do this in a separate thread.
    for (; outSignal.tail != outSignal.head; outSignal.tail++) {
    
        if (outSignal.buff[outSignal.tail] == '0') {
            *(gpiomem + GPIO_CLEAR_OFFSET) = OUTPUT_PIN_MASK;
            syslog(LOG_USER|LOG_DEBUG, "test: 0");  
        }
        else {
            *(gpiomem + GPIO_SET_OFFSET) = OUTPUT_PIN_MASK;
            syslog(LOG_USER|LOG_DEBUG, "test: 1");  
        }
    
        // sleep for 1 time unit
        timeUnit.tv_sec = MORSE_TIME_UNIT / 1000;
        timeUnit.tv_nsec = (MORSE_TIME_UNIT % 1000) * 1000000; // milliseconds to nanoseconds

        do {
            syslog(LOG_USER|LOG_DEBUG, "test: sleep time: %ld seconds, %ld nanoseconds", 
                                       timeUnit.tv_sec, timeUnit.tv_nsec); 
            if ((success = nanosleep(&timeUnit, &timeLeft)) != 0) {
                timeUnit.tv_sec = timeLeft.tv_sec;
                timeUnit.tv_nsec = timeLeft.tv_nsec;
                syslog(LOG_USER|LOG_DEBUG, "test: errno = %d", errno);  
            }
        } while ((success != 0) && (errno == EINTR));
    }

}

/******************************************************************************
* int sendBufferContents(int* buffer)
*
* Parameters: int* buffer
*
******************************************************************************/

int main(int argc, char* argv[]) {

    int i;
    char sendStr[100] = "";  // arbitrary limit to avoid long execution time
    
    // copy the message to send
    if (argc > 1) {
        for (i = 1; i < argc; i++) {
            strcat(sendStr, argv[i]);
            strcat(sendStr, " ");
        }
    }
    // if no arguments were passed in, send the default message
    // (First message sent by telegraph)
    else {
        strcpy(sendStr, "what hath god wrought");
    }
    
    // initialize the output hardware
    if (initHW() != 0) { return -1;}
    
    morse_test(sendStr);

    deinitHW();
}
