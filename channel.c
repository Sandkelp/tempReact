#include "channel.h"

// Creates a new channel with the provided size and returns it to the caller
channel_t* channel_create(size_t size)
{
    //make the buffer
    buffer_t* buffer;
    buffer = buffer_create(size);

    pthread_mutex_t my_mutex ;
    int init_status = 0 ; 
    init_status += pthread_mutex_init(&my_mutex, NULL);
    channel_t* channel_curr;

    pthread_cond_t empty_buff_signal; 
    init_status += pthread_cond_init(&empty_buff_signal, NULL);

    pthread_cond_t full_signal;
    init_status += pthread_cond_init(&full_signal, NULL);
    
    channel_curr = malloc(sizeof(channel_t));
    
    if (init_status == 0){
    channel_curr->buffer = buffer; 
    channel_curr->buffer_size = buffer->size;
    channel_curr->mutex = my_mutex; 
    channel_curr->empty_buff_signal = empty_buff_signal;
    channel_curr->full_buff_signal = full_signal;
    channel_curr->closed_flag = 0;} 
    //make 
    

    return channel_curr;
    
}

// Writes data to the given channel
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_send(channel_t *channel, void* data)
{   
    pthread_mutex_lock(&channel->mutex);

    if (channel->closed_flag == 1){
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    
    
    
    while (channel->buffer->capacity == channel->buffer->size ) {
        pthread_cond_wait(&channel->full_buff_signal, &channel->mutex);
    }

    
    int adding_status = buffer_add(channel->buffer, data);
    
    if (adding_status == BUFFER_SUCCESS){
        pthread_cond_signal(&channel->empty_buff_signal);
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;
    }
    pthread_mutex_unlock(&channel->mutex);
    return GENERIC_ERROR;
}

// Reads data from the given channel and stores it in the function's input parameter, data (Note that it is a double pointer)
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_receive(channel_t* channel, void** data)
{

    
    pthread_mutex_lock(&channel->mutex);
    

    if (channel->closed_flag == 1){
        
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    
    

    while (0 == channel->buffer->size ) {
        pthread_cond_wait(&channel->empty_buff_signal, &channel->mutex);
        
    }

    
    int adding_status = buffer_remove(channel->buffer, data);
    
    if (adding_status == BUFFER_SUCCESS){

        pthread_cond_signal(&channel->full_buff_signal);
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;

    }
    pthread_mutex_unlock(&channel->mutex);
    return GENERIC_ERROR;
}

// Writes data to the given channel
// This is a non-blocking call i.e., the function simply returns if the channel is full
// Returns SUCCESS for successfully writing data to the channel,
// CHANNEL_FULL if the channel is full and the data was not added to the buffer,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_send(channel_t* channel, void* data)
{
    /* IMPLEMENT THIS */
    
    pthread_mutex_lock(&channel->mutex);
   

    if (channel->closed_flag == 1){
        
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    
    
    if (channel->buffer->capacity == channel->buffer->size ) {
        pthread_mutex_unlock(&channel->mutex);
        return CHANNEL_FULL;
    }

    
    int adding_status = buffer_add(channel->buffer, data);
    
    if (adding_status == BUFFER_SUCCESS){
        pthread_cond_signal(&channel->empty_buff_signal);
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;
    }
    pthread_mutex_unlock(&channel->mutex);
    return GENERIC_ERROR;

}

// Reads data from the given channel and stores it in the function's input parameter data (Note that it is a double pointer)
// This is a non-blocking call i.e., the function simply returns if the channel is empty
// Returns SUCCESS for successful retrieval of data,
// CHANNEL_EMPTY if the channel is empty and nothing was stored in data,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_receive(channel_t* channel, void** data)
{
    /* IMPLEMENT THIS */
    
    pthread_mutex_lock(&channel->mutex);

   
    if (channel->closed_flag == 1){
        
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    
    

    if (0 == channel->buffer->size ) {
        pthread_mutex_unlock(&channel->mutex);

        return CHANNEL_EMPTY; 
    }

    
    int adding_status = buffer_remove(channel->buffer, data);
    
    if (adding_status == BUFFER_SUCCESS){

        pthread_cond_signal(&channel->full_buff_signal);
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;

    }
    pthread_mutex_unlock(&channel->mutex);
    return GENERIC_ERROR;
}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// GENERIC_ERROR in any other error case
enum channel_status channel_close(channel_t* channel)
{
    pthread_mutex_lock(&channel->mutex);

    //is channel alr closed
    if (channel->closed_flag == 1) {

        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    channel->closed_flag =  1;

    

    //pthread_cond_broadcast(&channel->mutex);
    pthread_cond_broadcast(&channel->full_buff_signal);
    pthread_cond_broadcast(&channel->empty_buff_signal);

    pthread_mutex_unlock(&channel->mutex);

    return SUCCESS;
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// GENERIC_ERROR in any other error case
enum channel_status channel_destroy(channel_t* channel)
{

    /* IMPLEMENT THIS */
    //channel_close(channel);

    if (channel->closed_flag == 0){
        return DESTROY_ERROR; 
    }
    buffer_free(channel->buffer);
    pthread_mutex_destroy(&channel->mutex);
    pthread_cond_destroy(&channel->empty_buff_signal);
    pthread_cond_destroy(&channel->full_buff_signal);
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
    /* IMPLEMENT THIS */
    // tff is this shit
    
    return SUCCESS;
}
