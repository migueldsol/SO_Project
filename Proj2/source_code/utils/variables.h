#ifndef __UTILS_VARIABLES_H__
#define __UTILS_VARIABLES_H__

/*************************
 *    INCLUDE LIBRARIES
 **************************/

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*************************
 *     INCLUDE FILES
 *************************/

#include "betterassert.h"
#include "logging.h"

/***************
/    MESSAGE
***************/

#define MAX_PIPE_NAME (256) // max size of client pipe name
#define MAX_BOX_NAME (32)   // max size for the box name
#define UINT8_T_SIZE (1)    // size of uint8_t
#define UINT16_T_SIZE (2)   // size of uint16_t
#define INT32_T_SIZE (4)    // size of uint32_t
#define UINT64_T_SIZE (8)   // size of uint64_t
#define MAX_MESSAGE (1024) // max size for the message to be sent from publisher
// max size for the message reply from publisher or subscriber
#define MAX_PUB_SUB_MESSAGE (UINT8_T_SIZE + MAX_MESSAGE)
// max size for the message to be sent from the server to register a
// box/publisher/subscriber
#define MAX_SERVER_REGISTER (UINT8_T_SIZE + MAX_PIPE_NAME + MAX_BOX_NAME)
// max size for the message to be sent from the server to list the boxes
#define MAX_SERVER_BOX_LIST_REPLY                                              \
    (2 * UINT8_T_SIZE + MAX_BOX_NAME + 3 * UINT64_T_SIZE)
// max size for the message to be sent to the server to list the boxes
#define MAX_SERVER_BOX_LIST (UINT8_T_SIZE + MAX_PIPE_NAME)
// max size for the message to be sent from the server to answer to a request
#define MAX_SERVER_REQUEST_REPLY (UINT8_T_SIZE + INT32_T_SIZE + MAX_MESSAGE)
// max size for the message to be sent to the server to do a request
#define MAX_PUB_SUB_REQUEST (UINT8_T_SIZE + MAX_PIPE_NAME + MAX_BOX_NAME)
/**********************
/    MESSAGE_CODE
**********************/

#define PUBLISHER_CODE (1)         // code to ask to add a publisher
#define SUBSCRIBER_CODE (2)        // code to ask to add a subscriber
#define CREATE_BOX_CODE (3)        // code to ask to create a box
#define CREATE_BOX_CODE_REPLY (4)  // code to answer to the creation of a box
#define REMOVE_BOX_CODE (5)        // code to ask to remove a box
#define REMOVE_BOX_CODE_REPLY (6)  // code to answer to the removal of a box
#define LIST_SEND_CODE (7)         // code to ask to list the boxes
#define LIST_RECEIVE_CODE (8)      // code to answer to the listing of the boxes
#define PUBLISHER_MESSAGE_CODE (9) // code to send a message from a publisher
#define SUBSCRIBER_MESSAGE_CODE (10) // code to send a message to a subscriber

/***********************
 *   MANAGER COMMANDS
 ***********************/

#define CREATE_COMMAND "create" // command to create a box
#define REMOVE_COMMAND "remove" // command to remove a box
#define LIST_COMMAND "list"     // command to list the boxes

#endif // __UTILS_VARIABLES_H__