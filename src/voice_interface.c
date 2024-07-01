
#if 0
struct control
{
    char control_name[128]; // 监听模块名称
    int (*init)(void);       // 初始化函数
    void (*final)(void);    // 结束释放函数
    void *(*get)(void *arg);// 监听函数，如语音监听
    void *(*set)(void *arg); // 设置函数，如语音播报
    struct control *next;   // 指向下一个control结构的指针，用于构建链表
};
#endif

#include <pthread.h>
#include <stdio.h>

#include "voice_interface.h"
#include "msg_queue.h"
#include "uart.h"
#include "global.h"

// 静态变量，用于存储串口文件描述符
static int serial_fd = -1;

// 语音模块的初始化函数
static int voice_init(void)
{
    serial_fd = myserialOpen(SERIAL_DEV, BAUD); // 打开串口设备，设置波特率
    printf("%s | %s | %d:serial_fd = %d\n", __FILE__, __func__, __LINE__, serial_fd);
    return serial_fd; // 返回串口文件描述符
}

// 语音模块的结束释放函数
static void voice_final(void)
{
    if (serial_fd != -1) // 检查文件描述符是否有效
    {
        close(serial_fd); // 关闭串口
        serial_fd = -1;   // 将文件描述符重置为无效值
    }
}

// 接收语音指令的线程函数
static void *voice_get(void *arg)
{
    // 定义缓冲区、长度变量、消息队列描述符和控制信息结构体
    unsigned char buffer[6] = {0};
    int len = 0;
    mqd_t mqd = -1;
    ctrl_info_t *ctrl_info = NULL;

    // 检查传入的参数是否为空，如果不为空则转换为ctrl_info_t类型
    if (NULL != arg)
    {
        ctrl_info = (ctrl_info_t *)arg;
    }

    // 检查串口文件描述符是否有效，如果无效则尝试重新初始化
    if (serial_fd == -1)
    {
        serial_fd = voice_init();
        if (serial_fd == -1) // 如果初始化失败，则退出线程
        {
            pthread_exit(0);
        }
    }

    // 如果控制信息不为空，则获取消息队列描述符
    if (NULL != ctrl_info)
    {
        mqd = ctrl_info->mqd;
    }

    // 如果消息队列描述符无效，则退出线程
    if (mqd == -1)
    {
        pthread_exit(0);
    }

    // 将当前线程分离，使其成为守护线程
    // pthread_detach(pthread_self());

    // 打印开始接收语音指令的日志
    printf("%s voice thread start\n", __func__);

    // 无限循环，接收串口数据
    while (1)
    {
        len = serialGetstring(serial_fd, buffer); // 从串口读取数据

        // printf("%s|%s|%d:len=%d\n", __FILE__, __func__, __LINE__, len);
        //  检查接收到的数据是否符合特定的格式
        if (len > 0)
        {
            printf("%s|%s|%d:len=%d\n", __FILE__, __func__, __LINE__, len);
            printf("%s|%s|%d:send 0x%x,0x%x,0x%x,0x%x\n", __FILE__, __func__, __LINE__, buffer[0], buffer[1], buffer[4], buffer[5]);

            if (buffer[0] == 0xAA && buffer[1] == 0x55 && buffer[4] == 0x55 && buffer[5] == 0xAA)
            {
                // 打印接收到的数据和长度
                printf("%s|%s|%d:send 0x%x,0x%x\n", __FILE__, __func__, __LINE__,  buffer[2], buffer[3]);

                // 发送消息到消息队列
                send_message(mqd, buffer, len);
                // 清空缓冲区
                memset(buffer, 0, sizeof(buffer));
            }
        }
    }
    // 线程正常退出
    pthread_exit(0);
}

// 语音播报的线程函数
static void *voice_set(void *arg)
{

    // 将当前线程分离
    pthread_detach(pthread_self());
    // 检查传入的参数是否为空，如果不为空则转换为unsigned char类型
    unsigned char *buffer = (unsigned char *)arg;
    printf("%s|%s|%d\n", __FILE__, __func__, __LINE__);
    // 检查串口文件描述符是否有效，如果无效则尝试重新初始化
    if (serial_fd == -1)
    {
        serial_fd = voice_init();
        if (serial_fd == -1) // 如果初始化失败，则退出线程
        {
            pthread_exit(0);
        }
    }

    // 如果传入的buffer不为空，则通过串口发送数据
    if (NULL != buffer)
    {
        serialSendstring(serial_fd, buffer, 6);
    }
    // 线程正常退出
    pthread_exit(0);
}

// 定义voice_control结构体，用于注册语音控制模块
struct control voice_control = {
    .control_name = "voice",
    .init = voice_init,
    .final = voice_final,
    .get = voice_get,
    .set = voice_set,
    .next = NULL};

// 将语音控制模块添加到控制链表的函数
struct control *add_voice_to_ctrl_list(struct control *phead)
{

    return add_interface_to_ctrl_list(phead, &voice_control);
};