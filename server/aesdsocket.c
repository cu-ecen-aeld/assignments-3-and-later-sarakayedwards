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

#define BUFF_SIZE 4096

int signalReceived = 0;
int socketfd;
int streamfd;
int filefd;

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
    
    // close open sockets
    close(filefd);
    close(streamfd);
    close(socketfd);
    
    // delete file
    remove("/var/tmp/aesdsocketdata");
    
    errno = tempErrno;
}

void createDaemon(void) {
    pid_t pid;
    
    pid = fork();
    if (pid < 0) {
        syslog(LOG_USER | LOG_ERR, "Failed to fork, error = %d", errno);
        exit(EXIT_FAILURE);
    }
    // parent process
    else if (pid > 0) exit(EXIT_SUCCESS);
    
    // if we made it here, this is the child process
    if (setsid() < 0) exit(EXIT_FAILURE);
    
    pid = fork();
     if (pid < 0) {
        syslog(LOG_USER | LOG_ERR, "Failed to fork, error = %d", errno);
        exit(EXIT_FAILURE);
    }
    // parent process
    else if (pid > 0) exit(EXIT_SUCCESS);
    
    // set file permissions
    umask(0);
    
    // change to parent directory
    chdir("/");
    
    // close file descriptors
    //for (int fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--) close(fd);
    
    // redirect standard input and output
    open("/dev/null", O_RDWR);
    dup(0);
    dup(0);
}

int main(int argc, char* argv[]) {

    int returnVal;
    struct addrinfo sockaddrhints;
    struct addrinfo* mysockaddr;
    struct sockaddr connectingaddr;
    bool connected;
    bool receiving;
    char recvbuf[BUFF_SIZE];
    char sendbuf[BUFF_SIZE];
    size_t strsize;
    socklen_t sockaddrlength = sizeof(struct sockaddr);
    int recsize;
    int sockoption = 1; // true
    int i;
    bool daemonMode = false;
    
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
    //*** make sure to close this fd
    if ((socketfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
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
    
    // listen and accept connections until signal is received
    if ((returnVal = listen(socketfd, 5)) != 0) {
        syslog(LOG_USER | LOG_ERR, "Failed to listen, error = %d", errno);
        close(socketfd);
        return returnVal;
    }
     
    // keep accepting and handling connections until a signal is received
    while (!signalReceived) {
    
        //*** make sure to close this fd
        if ((streamfd = accept(socketfd, &connectingaddr, &sockaddrlength)) == -1) {
            syslog(LOG_USER | LOG_ERR, "Failed to accept, error = %d", errno);
            close(socketfd);
            return returnVal;
        }
    
        // log connection
        // Later: Use getnameinfo() to get address in string form
        syslog(LOG_USER | LOG_DEBUG, "Accepted connection from %d.%d.%d.%d", 
              (int)connectingaddr.sa_data[2], (int)connectingaddr.sa_data[3], 
              (int)connectingaddr.sa_data[4], (int)connectingaddr.sa_data[5]); 
              
        connected = true;
        receiving = true;
    
        // Open the file, creating if it doesn't exist
        if ((filefd = open("/var/tmp/aesdsocketdata", O_RDWR|O_CREAT|O_APPEND|O_CLOEXEC, 
                                                  S_IRWXU|S_IRWXG|S_IRWXO)) == -1) {
            syslog(LOG_USER | LOG_ERR, "Failed to open file, error = %d", errno);
            close(socketfd);
            close(streamfd);
            return returnVal;
        }
    
        while (receiving) {
    
            // receive data, appending to file
            if ((recsize = recv(streamfd, recvbuf, BUFF_SIZE, 0)) == -1) {
                syslog(LOG_USER | LOG_DEBUG, "Failed to receive, error = %d", errno);
                receiving = false;
            }
            else if (recsize == 0) {
                receiving = false;
            }
            else {
                if ((returnVal = write(filefd, recvbuf, recsize)) < recsize) {
                    syslog(LOG_USER | LOG_DEBUG, "Failed to write, error = %d", errno);
                }
                
                // if this is the end of the packet, send the file
                if (recvbuf[recsize-1] == '\n') {
                
                    // send entire file content
                    if((returnVal = lseek(filefd, 0, SEEK_SET)) != 0) {
                        syslog(LOG_USER | LOG_DEBUG, 
                        "Failed to seek, position = %d", returnVal);
                    }
                    while ((strsize = read(filefd, sendbuf, BUFF_SIZE)) > 0) {
                        syslog(LOG_DEBUG, "Read %d bytes", (int)strsize);
                        send(streamfd, sendbuf, strsize, MSG_DONTWAIT);
                    }
                }
                
                // continue to receive another packet
                receiving = true;
            }
        }
        
       // close connection and log disconnection
        if (connected == false) {
            close(streamfd);
            // Later: Use getnameinfo() to get address in string form
            syslog(LOG_USER | LOG_DEBUG, "Closed connection from %d.%d.%d.%d", 
              (int)connectingaddr.sa_data[2], (int)connectingaddr.sa_data[3], 
              (int)connectingaddr.sa_data[4], (int)connectingaddr.sa_data[5]); 
        }
    
        close(filefd);
    }

    close(socketfd);
        
    remove("/var/tmp/aesdsocketdata");


}
