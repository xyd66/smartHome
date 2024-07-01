#include <pthread.h>

#include "socket.h"
#include "control.h"
#include "socket_interface.h"
#include "global.h"
#include "msg_queue.h"

static int s_fd = -1;

static int tcpsocket_init(void)//socket初始话
{
        
    s_fd = socket_init(IPADDR,IPPORT);//获取IP地址和端口号

    return -1;
}

static void tcpsocket_final(void)
{
    close(s_fd);
    s_fd = -1;

}

static void* tcpsocket_get(void *arg)
{
    int ret = -1;
    int c_fd = -1;
    
    struct sockaddr_in c_addr; // 客户端地址结构 
    unsigned char buffer[BUF_SIZE];
    mqd_t mqd = -1;
    ctrl_info_t *ctrl_info = NULL;


    int keepalive = 1;         // 开启TCP KeepAlive功能   
    int keepidle = 10;         // 设置探测时间间隔为10s
    int keepinterval = 5;      // 设置探测包发送间隔为5s
    int keepcount = 3;         // 设置探测包发送次数为3次

    pthread_detach(pthread_self());//和主线程分离

    printf("%s|%s|%d s_fd=%d thread start\n",__FILE__,__func__,__LINE__,s_fd);

    if(-1  == s_fd)
    {
        s_fd = tcpsocket_init();
        if(-1 == s_fd)
        {
            printf("tcpsocket_init failed\n");
            pthread_exit(0);
        }
    }
    if(NULL!=arg)
        ctrl_info = (ctrl_info_t *)arg;

    if(NULL != ctrl_info)
    {
        mqd = ctrl_info->mqd;

    }
    if((mqd_t)-1== mqd)
    {
        pthread_exit(0);
    }
    
 

    memset(&c_addr,0,sizeof(struct sockaddr_in)); 

    //4.accept  

    int clen = sizeof(struct sockaddr_in);
    printf("%s|%s|%d tcpsocket thread start\n",__FILE__,__func__,__LINE__);
    while(1){

        c_fd = accept(s_fd,(struct sockaddr *)&c_addr,&clen); 
      
        if(c_fd ==-1){  
            perror("accept");  
            continue; // 如果接受失败，继续等待下一个连接  
        } 
        
        
       
        ret =setsockopt( c_fd , SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));//设置TCP_KEEPALIVE选项
        if(ret == -1)//如果设置失败，打印错误信息并退出循环
        {
            perror("setsockopt");
            break;
        }

        ret =setsockopt( c_fd , IPPROTO_TCP, TCP_KEEPIDLE, (void *) &keepidle, sizeof(keepidle));//设置探测时间间隔选项
        if(ret == -1)//如果设置失败，打印错误信息并退出循环
        {
            perror("setsockopt");
            break;
        }


        ret =setsockopt( c_fd , IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepinterval,sizeof(keepinterval));//设置探测包发送间隔选项
        if(ret == -1)//如果设置失败，打印错误信息并退出循环
        {
            perror("setsockopt");
            break;
        }

        ret =setsockopt( c_fd , IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepcount, sizeof(keepcount));//设置探测包发送次数选项
        if(ret == -1)//如果设置失败，打印错误信息并退出循环
        {
            perror("setsockopt");
            break;
        }

        printf("%s|%s|%d: Accept a connection from %s:%d\n",__FILE__,__func__,__LINE__,inet_ntoa(c_addr.sin_addr),ntohs(c_addr.sin_port));
        
 

        while(1)
        {
            memset(buffer,0,BUF_SIZE);

            //等价于n_read = read(c_fd,readbuffer,sizeof(bufferfer)); 
            ret = recv(c_fd,buffer,BUF_SIZE,0);//读取客户端数据
            printf("%s|%s|%d:0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",__FILE__,__func__,__LINE__,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
            if(ret > 0)
            {
                if(buffer[0] == 0xAA && buffer[1] == 0x55  \
                && buffer[4] == 0x55 && buffer[5] == 0xAA  );
                {
                    printf("%s|%s|%d:0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",__FILE__,__func__,__LINE__,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
                    send_message( mqd, buffer ,ret);//注意不要用strlen去计算实际的长度
                }
                
            }
            else if(0 == ret || -1 == ret)//读取成功或者失败退出循环
            {
                break;
            }
        }
        close(c_fd);


    }
    pthread_exit(0);

}


struct control tcpsocket_control ={
    .control_name = "tcpsocket",
    .init = tcpsocket_init,
    .final = tcpsocket_final,
    .get = tcpsocket_get,
    .set = NULL,
    .next = NULL
};

struct control *add_tcpsocket_to_ctrl_list(struct control *phead )
{
    return add_interface_to_ctrl_list(phead ,&tcpsocket_control);
    
};