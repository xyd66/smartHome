#include <pthread.h> // 多线程库
#include <wiringPi.h> // GPIO控制库
#include <stdio.h>   // 标准输入输出库

// 自定义头文件，可能包含一些控制和消息队列的定义
#include "control.h"
#include "smoke_interface.h"
#include "global.h"
#include "msg_queue.h"

// 定义烟雾传感器的GPIO引脚和模式
#define SMOKE_PIN 9
#define SMOKE_MODE INPUT 

// 初始化烟雾检测模块
static int smoke_init(void) {
    printf("%s|%s|%d \n", __FILE__, __func__, __LINE__); // 打印文件名、函数名和行号，用于调试
    pinMode(SMOKE_PIN, SMOKE_MODE); // 设置烟雾传感器引脚模式

    return 0; // 初始化成功
}

// 清理烟雾检测模块，当前为空实现
static void smoke_final(void) {
    // 这里可以添加清理资源的代码
}

// 烟雾检测线程函数
static void* smoke_get(void *arg) {
    int status = HIGH; // 初始状态设为高电平
    int switch_status = 0; // 用于记录状态变化的变量
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0x55, 0xAA}; // 消息缓冲区，包含消息头和尾

    size_t byte_send = -1; // 发送字节数
    mqd_t mqd = -1; // 消息队列描述符
    ctrl_info_t *ctrl_info = NULL; // 控制信息结构体指针

    // 从参数中获取控制信息
    if (NULL != arg)
        ctrl_info = (ctrl_info_t *)arg;

    // 获取消息队列描述符
    if (NULL != ctrl_info) {
        mqd = ctrl_info->mqd;
    }

    // 如果消息队列描述符无效，则退出线程
    if ((mqd_t)-1 == mqd) {
        pthread_exit(0);
    }

    pthread_detach(pthread_self()); // 线程分离
    printf("%s|%s|%d  smoke thread start\n", __FILE__, __func__, __LINE__); // 打印调试信息

    while (1) { // 无限循环检测烟雾状态
        status = digitalRead(SMOKE_PIN); // 读取烟雾传感器状态

        // 如果检测到烟雾（低电平），则发送消息
        if (LOW == status) {
            buffer[2] = 0x45; // 设置消息类型
            buffer[3] = 0x00; // 设置消息状态
            switch_status = 1; // 标记状态已变化
            printf("%s|%s|%d:0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", __FILE__, __func__, __LINE__, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]); // 打印消息内容
            byte_send = mq_send(mqd, buffer, 6, 0); // 发送消息
            if (-1 == byte_send) { // 如果发送失败，则继续下一次循环
                continue;
            }
        }
        // 如果烟雾状态恢复（高电平），并且之前已经发送过消息，则再次发送消息
        else if (HIGH == status && 1 == switch_status) {
            buffer[2] = 0x45; // 设置消息类型
            buffer[3] = 0x01; // 设置消息状态
            switch_status = 0; // 重置状态变化标记
            printf("%s|%s|%d:0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", __FILE__, __func__, __LINE__, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]); // 打印消息内容
            byte_send = mq_send(mqd, buffer, 6, 0); // 发送消息
            if (-1 == byte_send) { // 如果发送失败，则继续下一次循环
                continue;
            }
        }
        sleep(5); // 等待5秒再次检测
    }

    pthread_exit(0); // 线程退出
}

// 定义烟雾控制结构体
struct control smoke_control = {
    .control_name = "smoke", // 控制名称
    .init = smoke_init, // 初始化函数
    .final = smoke_final, // 清理函数
    .get = smoke_get, // 获取函数，用于线程执行
    .set = NULL, // 设置函数，当前未使用
    .next = NULL // 链表下一个节点，当前未使用
};

// 将烟雾控制结构体添加到控制链表中
struct control *add_smoke_to_ctrl_list(struct control *phead) {
    return add_interface_to_ctrl_list(phead, &smoke_control);
}