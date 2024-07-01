#include "gdevice.h"
#include  "fan_gdevice.h"

struct gdevice fan_gdev = {

    .dev_name = "fan",//风扇
    .key = 0x43,
    .gpio_pin = 8,
    .gpio_mode = OUTPUT,
    .gpio_status = LOW,
    .check_face_status = 0,
    .voice_set_status = 0,
};

struct gdevice *add_fan_to_gdevice_list(struct gdevice *pdevhead)
{
    //头插法
    return add_device_to_gdevice_list(pdevhead,&fan_gdev);
}