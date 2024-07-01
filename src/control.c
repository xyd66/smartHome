#include <stdio.h>

#include "control.h"

struct control *add_interface_to_ctrl_list(struct control *phead ,struct control *control_interface)
{
    

    if(NULL == phead)
    {
        phead = control_interface;
    }
    else
    {
        control_interface->next = phead;
        phead = control_interface;
    }

    return phead;


};