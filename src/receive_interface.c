#include <pthread.h> // 包含多线程库
#include <stdlib.h>  // 包含内存分配函数
#include <wiringPi.h> // 包含GPIO操作库


#include "control.h"
#include "myoled.h"
#include "receive_interface.h"
#include "global.h"
#include "msg_queue.h"
#include "face.h"


#include "gdevice.h"
#include "bled_gdevice.h"
#include "fan_gdevice.h"
#include "lrled_gdevice.h"
#include "beep_gdevice.h"
#include "lock_gdevice.h"

// 定义接收到的消息结构体
typedef struct {
    int msg_len;      // 消息长度
    unsigned char *buffer; // 消息缓冲区
    ctrl_info_t *ctrl_info; // 控制信息
} recv_msg_t;

// 全局变量声明
static int oled_fd = -1; // OLED显示屏文件描述符
static struct gdevice *pdevhead = NULL; // 设备链表头指针

// 初始化接收模块
static int receive_init(void) {
    // 将不同类型的设备添加到设备链表中
    pdevhead = add_lrled_to_gdevice_list(pdevhead); // 客厅灯
    pdevhead = add_bled_to_gdevice_list(pdevhead);  // 卧室灯
    pdevhead = add_fan_to_gdevice_list(pdevhead);   // 风扇
    pdevhead = add_beep_to_gdevice_list(pdevhead); //蜂鸣器
    pdevhead = add_lock_to_gdevice_list(pdevhead); //锁

    oled_fd = myoled_init(); // 初始化OLED显示屏
    face_init(); // 初始化摄像头

    return 0;
}

// 清理接收模块
static void receive_final(void) {
    face_final(); // 清摄像头
    if (oled_fd != -1) {
        close(oled_fd); // 关闭OLED显示屏文件描述符
        oled_fd = -1;
    }
}

// 设备处理线程函数
static void *handle_device(void *arg) {
    // 解析传入的消息，找到对应的设备并设置其状态
    recv_msg_t *recv_msg = NULL;
    struct gdevice *cur_gdev = NULL;
    char success_or_failed[20] = "success";
    int ret = -1;
    pthread_t tid = -1;
    int smoke_status = 0;
    double face_result = 0.0;

    pthread_detach(pthread_self()); // 线程分离
    if (NULL != arg) {
        recv_msg = (recv_msg_t *)arg;
        // 打印调试信息
        printf("recv_msg->msg_len = %d\n", recv_msg->msg_len);
        // 打印消息内容
        printf("%s|%s|%d:handle 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", \
               __FILE__, __func__, __LINE__,    \
               recv_msg->buffer[0], recv_msg->buffer[1], \
               recv_msg->buffer[2], recv_msg->buffer[3], \
               recv_msg->buffer[4], recv_msg->buffer[5]);
    }

    // 根据消息内容找到对应的设备，并更新其状态
    if (NULL != recv_msg && NULL != recv_msg->buffer) 
    {
        printf("%s|%s|%d:handle\n",__FILE__, __func__, __LINE__);
        cur_gdev = find_device_by_key(pdevhead, recv_msg->buffer[2]);
    }
     printf("%s|%s|%d:handle\n",__FILE__, __func__, __LINE__);
    if (NULL != cur_gdev) 
    {

        cur_gdev->gpio_status = recv_msg->buffer[3] == 0 ? LOW : HIGH;

        printf("%s|%s|%d:cur_gdev->check_face_status=%d\n",
                __FILE__, __func__, __LINE__,cur_gdev->check_face_status);
        if(1 == cur_gdev->check_face_status)
        {
            face_result = face_category();
            printf("%s|%s|%d:face_result=%f\n",__FILE__, __func__, __LINE__,face_result);
            if(face_result>0.6)
            {
                ret = set_gpio_gdevice_status(cur_gdev);
                recv_msg->buffer[2] = 0x47;
            }
            else
            {
                recv_msg->buffer[2] = 0x46;
                ret = -1;
            }
        }
        else if(0 == cur_gdev->check_face_status)
        {
             ret = set_gpio_gdevice_status(cur_gdev);
        }

        if(1==cur_gdev->voice_set_status)
        {
        
            if(NULL != recv_msg && NULL != recv_msg->ctrl_info 
                && NULL != recv_msg->ctrl_info->ctrl_phead)
            {   
                struct control *pcontrol = recv_msg->ctrl_info->ctrl_phead;
                while(NULL != pcontrol)
                {
                    printf("%s|%s|%d\n",
                                __FILE__, __func__, __LINE__);
                    if(strstr(pcontrol->control_name,"voice"))
                    {

                        if(0x45 == recv_msg->buffer[2] && 0 == recv_msg->buffer[3])
                        {
      
                            smoke_status = 1;
                       
                        }
                        pthread_create(&tid,NULL,pcontrol->set,(void *)recv_msg->buffer);
                        break;
                    }
                    pcontrol=pcontrol->next;
                }

            }
       
        }

        if(-1 == ret)
        {
            memset(success_or_failed,'\0',sizeof(success_or_failed));
            strncpy(success_or_failed,"failed", 6);
        }


        //oled屏显示
        char oled_msg[512];
        memset(oled_msg,0,sizeof(oled_msg));
        char *change_status = cur_gdev->gpio_status == LOW ? "Open" : "Close";
        sprintf(oled_msg,"%s %s %s!\n",change_status,cur_gdev->dev_name,success_or_failed);
        if(smoke_status == 1)
        {
            memset(oled_msg,0,sizeof(oled_msg));
            strcpy(oled_msg,"Present fire risk!\n");
        }
        printf("oled_msg=%s\n",oled_msg);
        oled_show(oled_msg);

    //关锁
        if(1==cur_gdev->voice_set_status && ret == 0 && face_result>0.6)
        {
            sleep(5);
            cur_gdev->gpio_status =  HIGH ;
            set_gpio_gdevice_status(cur_gdev);
        }
    }

    
    pthread_exit(0); // 线程退出
}

// 接收消息线程函数
static void* receive_get(void *arg) {
    // 从消息队列中接收消息，并处理
    recv_msg_t *recv_msg = NULL;
    pthread_t tid = -1;
    ssize_t read_len = -1;
    char *buffer = NULL;
    struct mq_attr attr;
    mqd_t mqd = -1;

    // 初始化recv_msg结构体
    if (NULL != arg) {
        recv_msg = (recv_msg_t *)malloc(sizeof(recv_msg_t));
        recv_msg->ctrl_info = (ctrl_info_t *)arg; // 获取控制信息
        recv_msg->msg_len = -1;
        recv_msg->buffer = NULL;
    } else {
        pthread_exit(0);
    }

    // 获取消息队列属性
    if (mq_getattr(recv_msg->ctrl_info->mqd, &attr) == -1) {
        pthread_exit(0);
    }

    // 分配消息缓冲区
    recv_msg->buffer = (unsigned char *)malloc(attr.mq_msgsize);
    buffer = (unsigned char *)malloc(attr.mq_msgsize);

    // 清零消息缓冲区
    memset(recv_msg->buffer, 0, attr.mq_msgsize);
    memset(buffer, 0, attr.mq_msgsize);

     pthread_detach(pthread_self()); // 线程分离
    while (1) {
        // 从消息队列中接收消息
        read_len = mq_receive(recv_msg->ctrl_info->mqd, buffer, attr.mq_msgsize, NULL);

        // 打印调试信息
        printf("%s|%s|%d:0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
               __FILE__, __func__, __LINE__,
               buffer[0], buffer[1], buffer[2], buffer[3],
               buffer[4], buffer[5]);
        printf("%s|%s|%d:read_len =%ld\n", __FILE__, __func__, __LINE__, read_len);

        // 检查接收状态
        if (-1 == read_len) {
            if (errno == EAGAIN) {
                printf("queue is empty\n"); // 队列为空
            } else {
                break; // 出现错误，退出循环
            }
        } else if (buffer[0] == 0xAA && buffer[1] == 0x55 &&
                   buffer[4] == 0x55 && buffer[5] == 0xAA) {
            // 检查消息完整性
            recv_msg->msg_len = read_len;
            memcpy(recv_msg->buffer, buffer, read_len); // 复制消息到recv_msg
            // 创建新线程处理设备
            pthread_create(&tid, NULL, handle_device, (void *)recv_msg);
        }
    }

    pthread_exit(0); // 线程退出
}

// 定义接收控制结构体
struct control receive_control = {
    .control_name = "receive",
    .init = receive_init,
    .final = receive_final,
    .get = receive_get,
    .set = NULL,
    .next = NULL
};

// 将接收控制结构体添加到控制链表中
struct control *add_receive_to_ctrl_list(struct control *phead) {
    return add_interface_to_ctrl_list(phead, &receive_control);
}