#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    int retVal = -1;
    
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    thread_func_args->thread_complete_success = false;
    
    //retVal = pthread_mutex_init(thread_func_args->mutex, NULL);
    //if (retVal != 0) {
    //    ERROR_LOG("pthread_mutex_init failed, error=%d", retVal);
    //    return (void *)thread_func_args;
    //}
    
    retVal = nanosleep(&thread_func_args->wait_to_obtain, NULL);
    if (retVal != 0) {
        ERROR_LOG("nanosleep before obtaining failed, error=%d", retVal);
        return (void *)thread_param;
    }
    DEBUG_LOG("Success of first nanosleep!");
    
    retVal = pthread_mutex_lock(thread_func_args->mutex);
    if (retVal != 0) {
        ERROR_LOG("pthread_mutex_lock failed, error=%d", retVal);
        return (void *)thread_param;
    }
    DEBUG_LOG("Success of mutex lock!");
    
    retVal = nanosleep(&thread_func_args->wait_to_release, NULL);
    if (retVal != 0) {
        ERROR_LOG("nanosleep before releasing failed, error=%d", retVal);
        return (void *)thread_param;
    }
    
    retVal = pthread_mutex_unlock(thread_func_args->mutex);
    if (retVal != 0) {
        ERROR_LOG("pthread_mutex_unlock failed, error=%d", retVal);
        return (void *)thread_param;
    }    
    
    thread_func_args->thread_complete_success = true;
    
    return (void *)thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    int retVal = -1;
    struct thread_data* my_thread_data;
    
    my_thread_data = (struct thread_data *)malloc(sizeof(struct thread_data));
    
    if (my_thread_data == NULL) {
        ERROR_LOG("malloc failed");
        //perror("malloc failed");
        return false;
    }
    my_thread_data->mutex = mutex;
    my_thread_data->wait_to_obtain.tv_sec = wait_to_obtain_ms / 1000;
    my_thread_data->wait_to_obtain.tv_nsec = (wait_to_obtain_ms % 1000) * 1000;
    my_thread_data->wait_to_release.tv_sec = wait_to_release_ms / 1000;
    my_thread_data->wait_to_release.tv_nsec = (wait_to_release_ms % 1000) * 1000;
    my_thread_data->thread_complete_success = false;
    
    retVal = pthread_create(thread, 
                            NULL,
                            threadfunc,
                            (void *)my_thread_data); 

    if (retVal != 0) {
        ERROR_LOG("pthread_create failed, error=%d", retVal);
        free(my_thread_data);
        return false;
    }
      

    return true;
}

