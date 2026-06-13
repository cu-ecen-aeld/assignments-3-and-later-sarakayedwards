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
void morse_test(int argc, char* argv[]) {
    

}

/******************************************************************************
* int sendBufferContents(int* buffer)
*
* Parameters: int* buffer
*
******************************************************************************/

int main(void) {

    void* memmap;
    volatile uint32_t* gpiomem;
    int memfd;
    int i;
    int success;
    
    struct timespec timeUnit;
    struct timespec timeLeft;

    // Pre-load output signal with a message
    // "what hath god wrought" (First message sent by telegraph)
    // .-- .... .- - / .... .- - .... / --. --- -.. / .-- .-. --- ..- --. .... -
    // . = high for one unit of time
    // - = high for three units of time
    // Space between high signals is low for one unit of time
    // Space between characters is low for 3 units of time
    // Space between words is low for 7 units of time

    char morseBuffer[1024] = ".-- .... .- - / .... .- - .... / --. --- -.. / .-- .-. --- ..- --. .... -";
    outSignal.head = 0;
    outSignal.tail = 0;
    
    // Fill signal buffer with 0s and 1s to output
    for (i = 0; morseBuffer[i] != '\0'; i ++) {
        switch (morseBuffer[i]) {
            case '.':
                outSignal.buff[outSignal.head++] = 1;
                outSignal.buff[outSignal.head++] = 0;
                break;
            case '-':
                outSignal.buff[outSignal.head++] = 1;
                outSignal.buff[outSignal.head++] = 1;
                outSignal.buff[outSignal.head++] = 1;
                outSignal.buff[outSignal.head++] = 0;
                break;
            case ' ':   // only two time units need to be added to make the 3 total
                outSignal.buff[outSignal.head++] = 0;
                outSignal.buff[outSignal.head++] = 0; 
                break;
            case '/':   // only two are needed since ' ' will come before and after
                outSignal.buff[outSignal.head++] = 0;
                outSignal.buff[outSignal.head++] = 0;
                break;
            default :  // don't add anything for any other characters
        }
    }
    
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

    // read the gpio pins
    syslog(LOG_USER|LOG_DEBUG, "test: output signal");  

    // output a square wave with a wavelength of 2 seconds
    for (i = outSignal.tail; i != outSignal.head; i++, outSignal.tail++) {
    
        if (outSignal.buff[i] == 0) {
            *(gpiomem + GPIO_CLEAR_OFFSET) = OUTPUT_PIN_MASK;
            syslog(LOG_USER|LOG_DEBUG, "test: 0");  
        }
        else {
            *(gpiomem + GPIO_SET_OFFSET) = OUTPUT_PIN_MASK;
            syslog(LOG_USER|LOG_DEBUG, "test: 1");  
        }
    
        // sleep for 1 time unit
        timeUnit.tv_sec = MORSE_TIME_UNIT / 1000;
        timeUnit.tv_nsec = (MORSE_TIME_UNIT % 1000) * 1000; // milliseconds to nanoseconds

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

    munmap((void*)gpiomem, BLOCK_SIZE);

/*
    // start by sending a specified sequence
    printf("Output signal should see high-low-high-low, 500ms each");

    // set for 500ms
    delayTime.tv_sec = 0;
    delayTime.tv_nsec = 500000;
    
    MORSE_WRITE(1);
    
    do {
        if (sleep(delayTime, &delayTime) == -1) {
          errType = errno;
        }
    } while (errType == EINTR);      

    // set for 500ms
    delayTime.tv_sec = 0;
    delayTime.tv_nsec = 500000;
    
    MORSE_WRITE(0);
    
    do {
        if (sleep(delayTime, &delayTime) == -1) {
          errType = errno;
        }
    } while (errType == EINTR);      

    // set for 500ms
    delayTime.tv_sec = 0;
    delayTime.tv_nsec = 500000;
    
    MORSE_WRITE(1);
    
    do {
        if (sleep(delayTime, &delayTime) == -1) {
          errType = errno;
        }
    } while (errType == EINTR);      

    // set for 500ms
    delayTime.tv_sec = 0;
    delayTime.tv_nsec = 500000;
    
    MORSE_WRITE(0);
    
    do {
        if (sleep(delayTime, &delayTime) == -1) {
          errType = errno;
        }
    } while (errType == EINTR);      
    
    // Pause for 10 seconds for test setup
    printf("Please connect the input to the output.");
    // set for 10 sec
    delayTime.tv_sec = 10;
    delayTime.tv_nsec = 0;
    
   do {
        if (sleep(delayTime, &delayTime) == -1) {
          errType = errno;
        }
    } while (errType == EINTR); 
    
    //
*/
}
