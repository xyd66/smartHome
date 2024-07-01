
#include "gdevice.h"

// 定义了一个函数add_device_to_gdevice_list，用于将一个新设备添加到gdevice链表的头部
struct gdevice *add_device_to_gdevice_list(struct gdevice *pgdevhead ,struct gdevice *gdev)
{
    // 如果链表头为空，则将新设备设置为链表头
    if(NULL == pgdevhead)
    {
        pgdevhead = gdev;
    }
    // 如果链表头不为空，则将新设备的next指针指向当前的链表头，然后将新设备设置为新的链表头
    else
    {
        gdev->next = pgdevhead; // 将新设备的next指向当前头节点
        pgdevhead = gdev;      // 将新设备设置为新的头节点
    }
    // 返回更新后的链表头
    return pgdevhead;
}

// 定义了一个函数find_device_by_key，用于在gdevice链表中查找具有特定key的设备
struct gdevice *find_device_by_key(struct gdevice *pgdevhead, unsigned char key)
{
    struct gdevice *p = NULL;
    // 如果链表头为空，直接返回NULL
    if(NULL == pgdevhead)
    {
        return NULL; 
    }

    p = pgdevhead; // 初始化指针p为链表头
    while(NULL != p) // 遍历链表直到p为NULL
    {
        // 如果当前设备的结构体成员key与参数key相等，则返回当前设备
        if(p->key == key)
        {
            return p;
        }
        p = p->next; // 移动到下一个设备
    }
    // 如果没有找到匹配的设备，返回NULL
    return NULL;
}

// 定义了一个函数set_gpio_gdevice_status，用于设置gdevice结构体中gpio相关的状态
int set_gpio_gdevice_status(struct gdevice *pgdev)
{
    // 如果设备指针为空，返回错误代码-1
    if(NULL == pgdev)
    {
        return -1;
    }

    // 如果gpio_pin成员不是-1，表示有有效的GPIO引脚号
    if(-1 != pgdev->gpio_pin)
    {
        // 如果gpio_mode成员不是-1，表示有有效的GPIO模式
        if(-1 != pgdev->gpio_mode)
        {
            pinMode(pgdev->gpio_pin, pgdev->gpio_mode); // 配置引脚的输入输出模式
        }

        // 如果gpio_status成员不是-1，表示有有效的GPIO状态
        if(-1 != pgdev->gpio_status)
        {
            digitalWrite(pgdev->gpio_pin, pgdev->gpio_status); // 设置引脚的高低状态
        }
    }
    

}