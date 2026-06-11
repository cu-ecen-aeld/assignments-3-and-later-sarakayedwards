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

char outSignal[MAX_BUFFER_SIZE];
char inSignal[MAX_BUFFER_SIZE];

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
    
    syslog(LOG_USER|LOG_DEBUG, "running test...");  

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
    
    // set the pin as an output by setting the mode bits to 111 for write
    syslog(LOG_USER|LOG_DEBUG, "test: setting pin function");  

    *(gpiomem + GPIO_FSEL2_OFFSET) &= ~(7 << 9);
    *(gpiomem + GPIO_FSEL2_OFFSET) |= (7 << 9);

    // read the gpio pins
    syslog(LOG_USER|LOG_DEBUG, "test: reading pins");  

    printf("GPIO pins read: %x", *(gpiomem + GPIO_READ_OFFSET));
        
    // output a square wave with a wavelength of 2 seconds
    for (i=0;i<20;i++) {
    
        syslog(LOG_USER|LOG_DEBUG, "test: setting output pin");  

        *(gpiomem + GPIO_SET_OFFSET) = OUTPUT_PIN_MASK;
    
        // sleep for 1 second
        sleep(1);    

        printf("GPIO pins read: %x", *(gpiomem + GPIO_READ_OFFSET));

        syslog(LOG_USER|LOG_DEBUG, "test: clearing output pin");  

        *(gpiomem + GPIO_CLEAR_OFFSET) = OUTPUT_PIN_MASK;
    
        // sleep for 1 second
        sleep(1);    

        printf("GPIO pins read: %x", *(gpiomem + GPIO_READ_OFFSET));

        munmap((void*)gpiomem, BLOCK_SIZE);
    }

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
