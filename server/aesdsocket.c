#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "queue.h"
#include <pthread.h>
#include "../aesd-char-driver/aesd_ioctl.h"


#define BUFF_SIZE 4096

int signalReceived = 0;
int socketfd;
int filefd = -1;
pthread_mutex_t fileMutex;
pthread_t* tsThread;

#ifdef USE_AESD_CHAR_DEVICE
  #define DEV_OUT "/dev/aesdchar"
  #undef TIMESTAMP_ENABLED
#else
  #define DEV_OUT "/var/tmp/aesdsocketdata"
  #undef TIMESTAMP_ENABLED
#endif

struct socketThread {
    pthread_t pthread;
    struct sockaddr connectingaddr;
    int streamfd;
    bool completeFlag;
    SLIST_ENTRY(socketThread) next;
};

SLIST_HEAD(slisthead, socketThread);
struct slisthead listHead;

static void signal_handler(int signal_number) {
    int tempErrno = errno;
    
    signalReceived = 1;

    // log signal interruption
    if (signal_number == SIGTERM) {
        syslog(LOG_USER | LOG_ERR, "Caught signal, exiting");
    }
    if (signal_number == SIGINT) {
        syslog(LOG_USER | LOG_ERR, "Caught signal, exiting");
    }
   
    errno = tempErrno;
    
}

void createDaemon(void) {
    pid_t pid;
    int rv;
    
    pid = fork();
    if (pid < 0) {
        syslog(LOG_USER | LOG_ERR, "Failed to fork, error = %d", errno);
        exit(EXIT_FAILURE);
    }
    // parent process
    else if (pid > 0) exit(EXIT_SUCCESS);
    
    // if we made it here, this is the child process
    if (setsid() < 0) exit(EXIT_FAILURE);
    
    // set file permissions
    umask(0);
    
    // change to parent directory
    if ((rv = chdir("/")) == -1) syslog(LOG_USER | LOG_ERR, "Failed to chdir");
    
    // redirect standard input and output
    open("/dev/null", O_RDWR);
    if ((rv = dup(0)) == -1) syslog(LOG_USER | LOG_ERR, "Failed to dup");
    if ((rv = dup(0)) == -1) syslog(LOG_USER | LOG_ERR, "Failed to dup");
}

void receiveAndSend(struct socketThread* threadStruct) {
    int returnVal;
    char recvbuf[BUFF_SIZE];
    int recvbufsize = 0;
    char sendbuf[BUFF_SIZE];
    size_t strsize;
    int recsize;
    struct aesd_seekto seekto;
    int x,y;
    int ioctlretval;

    while (!signalReceived) {
    
        // receive data, appending to file
        if ((recsize = recv(threadStruct->streamfd, 
                            &recvbuf[recvbufsize], 
                            BUFF_SIZE - recvbufsize, 0)) == -1) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)) continue;
            else {
                syslog(LOG_USER | LOG_DEBUG, "Error from recv, error = %d", errno);
                return;
            }
        }
        else if (recsize == 0) {
            // the connection is closed
            syslog(LOG_USER | LOG_DEBUG, "Connection has been closed");
            return;
           
        }
        else if (recsize > 0) {
        
            // adjust the size of our accumlated received strings
            recvbufsize += recsize;

            // if this is the end of the packet, write to the file and send the file
            if (recvbuf[recvbufsize-1] == '\n') {

                // see if this is a special command
                if (sscanf(recvbuf, "AESDCHAR_IOCSEEKTO:%d:%d", x, y) == 2) {
                
                    syslog(LOG_USER | LOG_DEBUG, "special command handling");
                    seekto.write_cmd = x;
                    seekto.write_cmd_offset = y;
                    
                    // make sure the file is already open
                    if (filefd > 0) {
                        ioctlretval = ioctl(fileno(filefd), AESDCHAR_IOCSEEKTO, &seekto);
                    }
                    
                    syslog(LOG_USER | LOG_DEBUG, "ioctl return: %d", ioctlretval);
                }
                else {
                
                  #ifndef USE_AESD_CHAR_DEVICE
                    // lock the mutex before accessing the file
                    if (pthread_mutex_lock(&fileMutex) != 0) {
                        syslog(LOG_USER | LOG_ERR, 
                               "Failed to lock mutex, error = %d", errno);
                        exit(EXIT_FAILURE);
                    }
                  #endif
            
                    // If it's not open, open the file, creating if it doesn't exist
                    if (filefd <= 0) {

                      #ifndef USE_AESD_CHAR_DEVICE
                        // remove it if it exists
                        remove("/var/tmp/aesdsocketdata");
                      #endif
            
                        if ((filefd = open(DEV_OUT, O_RDWR|O_CREAT|O_APPEND|O_CLOEXEC, 
                                                    S_IRWXU|S_IRWXG|S_IRWXO)) == -1) {
                            syslog(LOG_USER | LOG_ERR, "Failed to open file, error = %d", errno);
                            return;
                        }
                    }

                    // write what was received to the file (append)
                    if ((returnVal = write(filefd, recvbuf, recvbufsize)) < recvbufsize) {
                        syslog(LOG_USER | LOG_DEBUG, "Failed to write, error = %d", errno);
                    }
                
                  #ifndef USE_AESD_CHAR_DEVICE
                    // unlock the mutex after access is complete
                    if (pthread_mutex_unlock(&fileMutex) != 0) {
                        syslog(LOG_USER | LOG_ERR, "Failed to unlock mutex, error = %d", errno);
                        exit(EXIT_FAILURE);;
                    }
                  #endif
                
                }
                
              #ifndef USE_AESD_CHAR_DEVICE
                // lock the mutex before accessing the file
                if (pthread_mutex_lock(&fileMutex) != 0) {
                    syslog(LOG_USER | LOG_ERR, 
                           "Failed to lock mutex, error = %d", errno);
                    exit(EXIT_FAILURE);
                }
              #endif
            
                // send entire file content over the socket
                if((returnVal = lseek(filefd, 0, SEEK_SET)) != 0) {
                    syslog(LOG_USER | LOG_ERR, 
                    "Failed to seek, position = %d", returnVal);
                }
              
                while ((strsize = read(filefd, sendbuf, BUFF_SIZE)) > 0) {
                    send(threadStruct->streamfd, sendbuf, strsize, MSG_DONTWAIT);
                }

              #ifndef USE_AESD_CHAR_DEVICE
                // unlock the mutex after access is complete
                if (pthread_mutex_unlock(&fileMutex) != 0) {
                    syslog(LOG_USER | LOG_ERR, 
                           "Failed to unlock mutex, error = %d", errno);
                    exit(EXIT_FAILURE);;
                }
              #endif
                
            }
        }
    }
}

void manageConnection(struct socketThread* threadStruct) {

    // log connection
    // Later: Use getnameinfo() to get address in string form
    syslog(LOG_USER | LOG_DEBUG, "Accepted connection from %d.%d.%d.%d", 
          (int)(threadStruct->connectingaddr).sa_data[2], 
          (int)(threadStruct->connectingaddr).sa_data[3], 
          (int)(threadStruct->connectingaddr).sa_data[4], 
          (int)(threadStruct->connectingaddr).sa_data[5]); 
              
    receiveAndSend(threadStruct);
        
    // close connection and log disconnection
    threadStruct->completeFlag = true;
    close(threadStruct->streamfd);
    // Later: Use getnameinfo() to get address in string form
    syslog(LOG_USER | LOG_DEBUG, "Closed connection from %d.%d.%d.%d", 
      (int)(threadStruct->connectingaddr).sa_data[2], 
      (int)(threadStruct->connectingaddr).sa_data[3], 
      (int)(threadStruct->connectingaddr).sa_data[4], 
      (int)(threadStruct->connectingaddr).sa_data[5]); 
}

void writeTimestamp(void) {
    time_t currentTimeEpoch;
    struct tm* currentTimeLocal;
    char timeStr[100];
    int timeStrSize;
    char formatStr[] = "%a, %d %b %Y %T %z"; // RFC 2822-compliant format
    char fullTimeStr[100];
    int returnVal;

    timeStrSize = 0;
    strcpy(timeStr, "");
    strcpy(fullTimeStr, "timestamp:");
   
    // get current time
    currentTimeEpoch = time(NULL);
    currentTimeLocal = localtime(&currentTimeEpoch);
    
    // format time and append newline
    timeStrSize = strftime(timeStr, 100, formatStr, currentTimeLocal);
    strcat(fullTimeStr, timeStr);
    timeStrSize += 10; //"timestamp:"
    fullTimeStr[timeStrSize++] = '\n';

  #ifndef USE_AESD_CHAR_DEVICE
    // lock the mutex before accessing the file
    if (pthread_mutex_lock(&fileMutex) != 0) {
        syslog(LOG_USER | LOG_ERR, "Failed to lock mutex, error = %d", errno);
        exit(EXIT_FAILURE);
    }
  #endif
            
    // append the timestamp to the file if the file is open
    if (filefd > 0) {
        if ((returnVal = write(filefd, fullTimeStr, timeStrSize)) < timeStrSize) {
            syslog(LOG_USER | LOG_ERR, "Failed to write, error = %d", errno);
        }
    }
        
  #ifndef USE_AESD_CHAR_DEVICE          
    // unlock the mutex after access is complete
    if (pthread_mutex_unlock(&fileMutex) != 0) {
        syslog(LOG_USER | LOG_ERR, "Failed to unlock mutex, error = %d", errno);
        exit(EXIT_FAILURE);
    }
  #endif
}

int main(int argc, char* argv[]) {

    int returnVal;
    struct addrinfo sockaddrhints;
    struct addrinfo* mysockaddr;
    struct sockaddr connectingaddr;
    socklen_t sockaddrlength = sizeof(struct sockaddr);
    int sockoption = 1; // true
    int i;
    bool daemonMode = false;
    int streamfd;
    
  #ifdef TIMESTAMP_ENABLED
    time_t nextTime;
  #endif
    
  #ifndef USE_AESD_CHAR_DEVICE
    // initialize mutex lock for file
    if (pthread_mutex_init(&fileMutex, NULL) != 0) {
        syslog(LOG_USER | LOG_ERR, "Failed to init mutex, error = %d", errno);
        return errno;
    }
  #endif

    struct sigaction new_action;

    memset(&new_action, 0, sizeof(struct sigaction));
    new_action.sa_handler = signal_handler;
    
    // register the signal handler
    if (sigaction(SIGTERM, &new_action, NULL) != 0) {
        syslog(LOG_USER | LOG_ERR, "Failed to register SIGTERM, error = %d", errno);
    }
    if (sigaction(SIGINT, &new_action, NULL) != 0) {
        syslog(LOG_USER | LOG_ERR, "Failed to register SIGINT, error = %d", errno);
    }
    
    // decide if we are running as a daemon
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            daemonMode = true;
            syslog(LOG_USER, "Running as daemon");
        }
        else {
            syslog(LOG_USER, "Not running as daemon. argv[1] = %s", argv[1]);
        }
    }

    // create and open socket
    if ((socketfd = socket(PF_INET, 
                           SOCK_STREAM |SOCK_NONBLOCK | SOCK_CLOEXEC, 
                           0)) == -1) {
        syslog(LOG_USER | LOG_ERR, "Failed to create socket, error = %d", errno);
        return errno;
    }
    
    // set socket option to reuse address
    if ((returnVal = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, 
                                &sockoption, sizeof(sockoption))) != 0) {
        syslog(LOG_USER | LOG_ERR, "Failed to set SO_REUSEADDR, error = %d", returnVal);
    }
    
    // memset(&sockaddrhints, 0, sizeof(sockaddrhints));
    sockaddrhints.ai_family = AF_UNSPEC;
    sockaddrhints.ai_socktype = SOCK_STREAM;
    sockaddrhints.ai_flags = AI_PASSIVE;
    sockaddrhints.ai_protocol = IPPROTO_TCP;
    
    if ((returnVal = getaddrinfo(NULL, "9000", &sockaddrhints, &mysockaddr)) != 0) {
        syslog(LOG_USER | LOG_ERR, "Failed to get addrinfo, error = %d", returnVal);
        close(socketfd);
        return returnVal;
    }

    if ((returnVal = bind(socketfd, 
                          mysockaddr->ai_addr, 
                          sizeof(struct sockaddr))) == -1) {
        syslog(LOG_USER | LOG_ERR, "Failed to bind socket, error = %d", errno);
        close(socketfd);
        return returnVal;
    }
    else {
        // if we are in daemon mode create daemon at this point
        if (daemonMode) createDaemon();   
    }
    
    // free memory
    freeaddrinfo(mysockaddr);
    
    // initialize linked list for threads
    struct socketThread* newThread;
    struct slisthead listHead;
    SLIST_INIT(&listHead);
    
    // listen and accept connections until signal is received
    if ((returnVal = listen(socketfd, 5)) != 0) {
        syslog(LOG_USER | LOG_ERR, "Failed to listen, error = %d", errno);
        close(socketfd);
        
  #ifndef USE_AESD_CHAR_DEVICE        
        // lock the mutex before closing the fd, then unlock
        if (pthread_mutex_lock(&fileMutex) == 0) {
  #endif        
            close(filefd);
  #ifndef USE_AESD_CHAR_DEVICE        
            if (pthread_mutex_unlock(&fileMutex) != 0) {
                syslog(LOG_USER | LOG_ERR, "Failed to unlock mutex, error = %d", errno);
            }
        }
        else {
            syslog(LOG_USER | LOG_ERR, "Failed to lock mutex, error = %d", errno);
        }
        return returnVal;
  #endif
        
    }
    
  #ifdef TIMESTAMP_ENABLED
    // initialize timer for timestamp
    nextTime = time(NULL) + 10;
  #endif
     
    // keep accepting and handling connections until a signal is received
    while (!signalReceived) {
    
    
      #ifdef TIMESTAMP_ENABLED
        // if it's time, set the new timer and write the current timestamp
        if (time(NULL) >= nextTime) {
            nextTime = time(NULL) + 10;
            writeTimestamp();
        }
      #endif
            
        streamfd = accept(socketfd, 
                           &connectingaddr, 
                           &sockaddrlength);
        if (streamfd == -1) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) continue;
            else { 
                syslog(LOG_USER | LOG_ERR, "Failed to accept, error = %d", errno);
                break;
            }
        }
        else {
            // create thread and add it to the linked list
            newThread = malloc(sizeof(struct socketThread));
            newThread->connectingaddr = connectingaddr;
            newThread->streamfd = streamfd;
            newThread->completeFlag = false;
        
            pthread_create(&(newThread->pthread), 
                           NULL, 
                           (void*)manageConnection, 
                           newThread);

            SLIST_INSERT_HEAD(&listHead, newThread, next);
        
            struct socketThread *item, *tItem;
            
            SLIST_FOREACH_SAFE(item, &listHead, next, tItem){
                if (item->completeFlag) {
                    pthread_join(item->pthread, NULL);
                    SLIST_REMOVE(&listHead, item, socketThread, next);
                    free(item);
                    item = NULL;
                }
            }
        }
    }
    
    // clean up before exiting

    // close open fds and free list memory
    struct socketThread *item, *tItem;
    
    SLIST_FOREACH_SAFE(item, &listHead, next, tItem){
        close(item->streamfd);
        pthread_join(item->pthread, NULL);
        SLIST_REMOVE(&listHead, item, socketThread, next);
        free(item);
    }
    
    close(socketfd);
    
    // close device
    close(filefd);
    
  #ifndef USE_AESD_CHAR_DEVICE
    remove("/var/tmp/aesdsocketdata");
    pthread_mutex_destroy(&fileMutex);
  #endif

}
