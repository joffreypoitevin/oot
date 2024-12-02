#include "global.h"

// Function to initialize a message queue for communication between threads or systems.
void osCreateMesgQueue(OSMesgQueue* mq, OSMesg* msg, s32 count) {
    // Initialize the "mtqueue" field of the message queue structure.
    // This queue will hold threads waiting for the message queue to have space (message senders).
    mq->mtqueue = (OSThread*)&__osThreadTail; // Set it to the default empty thread list (__osThreadTail).

    // Initialize the "fullqueue" field of the message queue structure.
    // This queue will hold threads waiting for messages (message receivers).
    mq->fullqueue = (OSThread*)&__osThreadTail; // Set it to the default empty thread list (__osThreadTail).

    // Set the initial count of valid messages in the queue to zero.
    mq->validCount = 0; // No messages are in the queue when it is first created.

    // Initialize the "first" index to zero.
    // This index tracks the position of the first message in the buffer.
    mq->first = 0; // The queue starts empty, so the first position is at the beginning of the buffer.

    // Set the total capacity of the queue.
    // This is the maximum number of messages the queue can hold.
    mq->msgCount = count;

    // Assign the provided message buffer to the queue's "msg" field.
    // This buffer is where the messages will be stored.
    mq->msg = msg; // Links the queue to the array where messages will be stored.
}
