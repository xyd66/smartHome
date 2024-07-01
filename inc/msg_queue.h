#ifndef __MSG_QUEUE__H
#define __MSG_QUEUE__H


#include <mqueue.h> 
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

mqd_t msg_queue_create(void );
void msg_queue_final(mqd_t mqd);
int send_message(mqd_t mqd,void *msg,int msg_len);

#endif