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

#include <sel4/sel4.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <camkes.h>

/*? macros.show_includes(me.from_instance.type.includes) ?*/
/*? macros.show_includes(me.from_interface.type.includes, '../static/components/' + me.from_instance.type.name + '/') ?*/

/*- set ep = alloc(me.to_instance.name + "_ep_GDB_delegate", seL4_EndpointObject, read=True, write=True) -*/
/*- set vspace = alloc_cap('my_pd_cap', my_pd, write=true) -*/

/*- include 'GDB_delegate.h' -*/

int /*? me.from_interface.name ?*/__run(void) {
    return 0;
}

uint32_t /*? me.to_instance.name ?*/_handle_breakpoint(seL4_Word tcb_cap) {

    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 2);   //new message. [0] = GDB_HANDLE [1] = tcb capability
    seL4_SetMR(0, GDB_HANDLE);                                    //set handle gdb request
    seL4_SetMR(1, tcb_cap);

    seL4_Send(/*? ep ?*/, info);                                  //send handle gdb request
    info = seL4_Recv(/*? ep ?*/, NULL);                           //get response
    return (uint32_t)seL4_GetMR(0);                               //return updated program counter
}
