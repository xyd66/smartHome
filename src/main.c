#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <wiringPi.h>

#include "voice_interface.h"
#include "msg_queue.h"
#include "control.h"
#include "global.h"
#include "socket_interface.h"
#include "smoke_interface.h"
#include "receive_interface.h"

#define SM_debug 1


int main(int argc,char **argv)
{
    
    pthread_t  thread_id;

    // 定义指向控制结构的指针
    struct control *control_phead = NULL;
    struct control *pointer = NULL;

    ctrl_info_t *ctrl_info = NULL;

    ctrl_info = (ctrl_info_t*)malloc(sizeof(ctrl_info_t));
    ctrl_info->ctrl_phead =NULL;// 初始化控制链表头部
    ctrl_info->mqd = -1;

    int node_num = 0;

    if(-1 ==wiringPiSetup())
    {
        return -1;
    }
    ctrl_info->mqd = msg_queue_create(); 

   // 检查消息队列是否创建成功
    if(ctrl_info->mqd == -1) 
    {
        printf("%s|%s|%d,ctrl_info->mqd=%d\n",__FILE__,__func__,__LINE__,ctrl_info->mqd);
        return -1;

    }  
   // 将语音处理模块添加到控制列表中
     ctrl_info->ctrl_phead = add_voice_to_ctrl_list(ctrl_info->ctrl_phead);
    
     ctrl_info->ctrl_phead = add_tcpsocket_to_ctrl_list(ctrl_info->ctrl_phead);

     ctrl_info->ctrl_phead = add_smoke_to_ctrl_list(ctrl_info->ctrl_phead);

    ctrl_info->ctrl_phead = add_receive_to_ctrl_list(ctrl_info->ctrl_phead);
     // 遍历控制链表，初始化每个节点
    pointer = ctrl_info->ctrl_phead;
    while(pointer != NULL)
    {
        if(NULL != pointer->init)
        {
#if SM_debug
            printf("%s|%s|%d:pointer->control_name=%s\n",__FILE__,__func__,__LINE__,pointer->control_name);
#endif
            pointer->init();
        }
       
        pointer = pointer->next;
        node_num++;
    }

    pthread_t *tid = malloc(sizeof(int) *node_num);

    pointer = ctrl_info->ctrl_phead;

    for(int i = 0;i<node_num;i++)
    {
        if(NULL !=pointer->get)
        {
            printf("%s|%s|%d:pointer->control_name=%s\n",__FILE__,__func__,__LINE__,pointer->control_name);
            pthread_create(&tid[i],NULL,(void *)pointer->get,(void *)ctrl_info);
        }
        pointer = pointer->next;
    }

    for(int i =0; i< node_num;i++)
    {
        pthread_join(tid[i],NULL);
    }

    for(int i =0; i< node_num;i++)
    {
        printf("%s|%s|%d\n",__FILE__,__func__,__LINE__);
        if(NULL != pointer->final)
            pointer->final();
        
        
        pointer = pointer->next;


    }
   // 清理消息队列资源
    msg_queue_final(ctrl_info->mqd);
    if(NULL!=ctrl_info)
    {
        free(ctrl_info);

    }
    
    if(NULL!=tid)
    {
        free(tid);
        
    }
    return 0;
}
