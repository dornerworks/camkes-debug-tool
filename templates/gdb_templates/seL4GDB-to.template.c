/*#
 *# Copyright 2014, NICTA
 *# Copyright 2017, DornerWorks
 *#
 *# This software may be distributed and modified according to the terms of
 *# the BSD 2-Clause license. Note that NO WARRANTY is provided.
 *# See "LICENSE_BSD2.txt" for details.
 *#
 *# @TAG(NICTA_BSD)
 #*/

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <utils/util.h>
#include <camkes.h>
#include <stdarg.h>

/*? macros.show_includes(me.to_instance.type.includes) ?*/
/*? macros.show_includes(me.to_interface.type.includes, '../static/components/' + me.to_instance.type.name + '/') ?*/

/*- set ep = alloc(me.from_instance.name + "_ep_fault", seL4_EndpointObject, read=True, write=True, grant=True) -*/
/*- set cnode = alloc_cap('cnode', my_cnode, write=True) -*/
/*- set reply_cap_slot = alloc_cap('reply_cap_slot', None) -*/
/*- set vspace = alloc_cap('my_pd_cap', my_pd, write=true) -*/

extern uint32_t /*? me.from_instance.name ?*/_handle_breakpoint(seL4_Word tcb_cap);

void /*? me.to_interface.name ?*/__init(void) {
}

int /*? me.to_interface.name ?*/__run(void)
{
    seL4_Word reg_pc;
    seL4_Word badge;

    while (1)
    {
        seL4_Recv(/*? ep ?*/, &badge);

        seL4_CNode_SaveCaller(/*? cnode ?*/, /*? reply_cap_slot ?*/, 32);
        reg_pc = /*? me.from_instance.name ?*/_handle_breakpoint(badge);

        if(0 != reg_pc)
        {
            seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
            seL4_SetMR(0, reg_pc);
            seL4_NBSend(/*? reply_cap_slot ?*/, info);
        }
        else
        {
            seL4_TCB_Suspend(badge);
        }
    }
    UNREACHABLE();
}
