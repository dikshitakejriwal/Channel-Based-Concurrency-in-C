#include "channel.h"

// Creates a new channel with the provided size and returns it to the caller
// A 0 size indicates an unbuffered channel, whereas a positive size indicates a buffered channel
channel_t* channel_create(size_t size)
{
    /* IMPLEMENT THIS */
    //send in term of messages  -> 1 send = 1 message

    //unbuffered channel
    if (size == 0){
        return NULL;
    }

    //1. Allocate memory for channel structure
    channel_t* channel = (channel_t*)malloc(sizeof(channel_t));
    //create a buffer of the specificed size
    channel->buffer = buffer_create(size);
    //Intialize the mutex
    pthread_mutex_init(&channel->mutex, NULL);
    //Intialize the condition variable for read operation
    pthread_cond_init(&channel->cond_read, NULL);
    //Intialize the condition variable for write operation
    pthread_cond_init(&channel->cond_write, NULL);
    channel->is_closed = false;//channel is set to open intially

    //intialize condition variable and mutex for selection
    pthread_cond_init(&channel->cond_select, NULL);
    pthread_mutex_init(&channel->mutex_select, NULL);
    //create select list
    channel->select_list = list_create();

    return channel;
}

// Writes data to the given channel
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_send(channel_t *channel, void* data)
{
    /* IMPLEMENT THIS */
    //lock the mutex
    pthread_mutex_lock(&channel->mutex);

    //check if channel is closed - unlock mutex, return closed_error
    if (channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    //else add in buffer and check if it full or not - if it throws an error then the buffer is full
    while (buffer_add(channel->buffer, data) == BUFFER_ERROR){
        //wait until the buffer is not full anymore - signaled by cond_read
        if (channel->is_closed) {
            pthread_mutex_unlock(&channel->mutex);
            return CLOSED_ERROR;
        }
        pthread_cond_wait(&channel->cond_write, &channel->mutex);
    }

    pthread_cond_signal(&channel->cond_read); //signal that the buffer is not empty anymore
    pthread_mutex_unlock(&channel->mutex); //unlock the mutex

    return SUCCESS;
}

// Reads data from the given channel and stores it in the function's input parameter, data (Note that it is a double pointer)
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_receive(channel_t* channel, void** data)
{
    /* IMPLEMENT THIS */

    //lock the mutex
    pthread_mutex_lock(&channel->mutex);

    if (channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    //remove data from the buffer - check if the buffer is empty - if it is wait until data is added
    while (buffer_remove(channel->buffer, data) == BUFFER_ERROR) {
        //wait until the buffer is not empty anymore - signaled by cond_write
        if (channel->is_closed) {
            pthread_mutex_unlock(&channel->mutex);
            return CLOSED_ERROR;
        }
        pthread_cond_wait(&channel->cond_read, &channel->mutex);
    }

    pthread_cond_signal(&channel->cond_write); //signal that the buffer is not full anymore
    pthread_mutex_unlock(&channel->mutex); //unlock the mutex

    return SUCCESS;
}

// Writes data to the given channel
// This is a non-blocking call i.e., the function simply returns if the channel is full
// Returns SUCCESS for successfully writing data to the channel,
// CHANNEL_FULL if the channel is full and the data was not added to the buffer,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_send(channel_t* channel, void* data)
{
    /* IMPLEMENT THIS */
    // if channel doesn't exist return GEN_ERROR

    //lock the mutex
    pthread_mutex_lock(&channel->mutex);

    //check if channel is closed - unlock mutex, return closed_error
    if (channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    if (buffer_add(channel->buffer, data) == BUFFER_ERROR) {
        pthread_mutex_unlock(&channel->mutex);
        return CHANNEL_FULL;
    }

    pthread_cond_signal(&channel->cond_read); //signal that the buffer is not empty anymore
    pthread_mutex_unlock(&channel->mutex); //unlock the mutex

    return SUCCESS;
}

// Reads data from the given channel and stores it in the function's input parameter data (Note that it is a double pointer)
// This is a non-blocking call i.e., the function simply returns if the channel is empty
// Returns SUCCESS for successful retrieval of data,
// CHANNEL_EMPTY if the channel is empty and nothing was stored in data,
// CLOSED_ERROR if the channel is closed, and
// GEN_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_receive(channel_t* channel, void** data)
{
    /* IMPLEMENT THIS */

    // receive wont block if there are no messages in the buffer and buffer is empty instead just say the buffer is empty

    //lock the mutex
    pthread_mutex_lock(&channel->mutex);

    if (channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }


    if (buffer_remove(channel->buffer, data) == BUFFER_ERROR){
        pthread_mutex_unlock(&channel->mutex);
        return CHANNEL_EMPTY;
    }

    pthread_cond_signal(&channel->cond_write); //signal that the buffer is not full anymore
    pthread_mutex_unlock(&channel->mutex); //unlock the mutex

    return SUCCESS;
}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// GEN_ERROR in any other error case
enum channel_status channel_close(channel_t* channel)
{
    /* IMPLEMENT THIS */
    //clears out everything that is happening in the channel  - any threads that are in middle of send or receive in that channel those will be stopped
    
    //lock the mutex
    pthread_mutex_lock(&channel->mutex);

    //check if channel is already closed
    if (channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    //mark channel as closed
    channel->is_closed = true;

    //wake up all the threads waiting on cond_read and cond_write
    pthread_cond_broadcast(&channel->cond_read);
    pthread_cond_broadcast(&channel->cond_write);

    pthread_mutex_unlock(&channel->mutex); //unlock the mutex

    return SUCCESS;
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// GEN_ERROR in any other error case
enum channel_status channel_destroy(channel_t* channel)
{
    /* IMPLEMENT THIS */
    pthread_mutex_lock(&channel->mutex);

    //DESTROY_ERROR if channel_destroy is called on an open channel
    if (!channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        return DESTROY_ERROR;
    }

    //free buffer allocated to the channel
    buffer_free(channel->buffer);

    //unlock mutex
    pthread_mutex_unlock(&channel->mutex); 

    //destroy mutex and cond variables
    pthread_mutex_destroy(&channel->mutex);
    pthread_cond_destroy(&channel->cond_read);
    pthread_cond_destroy(&channel->cond_write);
    free(channel);

    return SUCCESS;
}

// Takes an array of channels (channel_list) of type select_t and the array length (channel_count) as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index)
{
    if (!channel_list || channel_count == 0 || !selected_index) {
        return GEN_ERROR;
    }

    //goes over each channel in the channel_list
    for (size_t i = 0; i < channel_count; i++) {
        channel_t* curr_channel = channel_list[i].channel;

        //lock mutex
        pthread_mutex_lock(&curr_channel->mutex);

        //f channel is closed
        if (curr_channel->is_closed) {
            *selected_index = i;
            pthread_mutex_unlock(&curr_channel->mutex);
            return CLOSED_ERROR;
        }

        //checking the direction
        if (channel_list[i].dir == SEND) {
            if (buffer_add(curr_channel->buffer, channel_list[i].data) != BUFFER_ERROR) {
                *selected_index = i;
                pthread_cond_signal(&curr_channel->cond_read);
                pthread_mutex_unlock(&curr_channel->mutex);
                return SUCCESS;
            }
        } else { // RECV
            if (buffer_remove(curr_channel->buffer, &(channel_list[i].data)) != BUFFER_ERROR) {
                *selected_index = i;
                pthread_cond_signal(&curr_channel->cond_write);
                pthread_mutex_unlock(&curr_channel->mutex);
                return SUCCESS;
            } 
        }

        pthread_mutex_unlock(&curr_channel->mutex);
    }
    

    // no operation was performed, wait for a signal indicating that a channel is ready.
    size_t current_channel = 0;
    while (1) {
        channel_t* curr_channel = channel_list[current_channel].channel;
        pthread_mutex_lock(&curr_channel->mutex);
        
        if (curr_channel->is_closed) {
            *selected_index = current_channel;
            pthread_mutex_unlock(&curr_channel->mutex);
            return CLOSED_ERROR;
        }

        if (channel_list[current_channel].dir == SEND) {
            if (buffer_add(curr_channel->buffer, channel_list[current_channel].data) != BUFFER_ERROR) {
                *selected_index = current_channel;
                pthread_cond_signal(&curr_channel->cond_read);
                pthread_mutex_unlock(&curr_channel->mutex);
                return SUCCESS;
            }
        } else { // RECV
            if (buffer_remove(curr_channel->buffer, &(channel_list[current_channel].data)) != BUFFER_ERROR) {
                *selected_index = current_channel;
                pthread_cond_signal(&curr_channel->cond_write);
                pthread_mutex_unlock(&curr_channel->mutex);
                return SUCCESS;
            }
        }

        pthread_mutex_unlock(&curr_channel->mutex);

        // Move to the next channel
        current_channel = (current_channel + 1) % channel_count;
    }
}







