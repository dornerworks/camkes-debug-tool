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

#include <autoconf.h>

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <utils/util.h>
#include <camkes.h>

/*? macros.show_includes(me.to_instance.type.includes) ?*/
/*? macros.show_includes(me.to_interface.type.includes, '../static/components/' + me.to_instance.type.name + '/') ?*/

/*- set ep = alloc(me.to_instance.name + "_ep_GDB_delegate", seL4_EndpointObject, read=True, write=True) -*/
/*- set cnode = alloc_cap('cnode', my_cnode, write=True) -*/
/*- set my_pd_cap = alloc_cap('my_pd_cap', my_pd, write=true) -*/

/*- include 'GDB_delegate.h' -*/

/*- include 'gdb.c' -*/

/*- include 'serial_gdb.c' -*/

int /*? me.to_interface.name ?*/__run(void) {

    seL4_MessageInfo_t info;
    seL4_Word ret_addr, instruction;

    while (1) {

        seL4_Recv(/*? ep ?*/, NULL);
        instruction = seL4_GetMR(DELEGATE_COMMAND_REG);

        if(instruction == GDB_HANDLE) {
            ret_addr = handle_breakpoint();
            info = seL4_MessageInfo_new(0, 0, 0, 1);
            seL4_SetMR(0, ret_addr);
            seL4_Send(/*? ep ?*/, info);
        }
        else {
            printf("Unrecognised command %d %d %d\n", instruction, seL4_GetMR(1), seL4_GetMR(2));
            info = seL4_MessageInfo_new(0, 0, 0, 1);
            seL4_Send(/*? ep ?*/, info);
        }
    }
    UNREACHABLE();
}
