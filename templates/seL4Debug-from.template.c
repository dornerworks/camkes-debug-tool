/*#
 *# Copyright 2014, NICTA
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
#include <camkes/marshal.h>
#include <camkes/dataport.h>
#include <camkes/error.h>
#include <camkes/timing.h>
#include <camkes/tls.h>
#include <sync/sem-bare.h>

/*? macros.show_includes(me.from_instance.type.includes) ?*/
/*? macros.show_includes(me.from_interface.type.includes, '../static/components/' + me.from_instance.type.name + '/') ?*/
/*- set ep = alloc(me.to_instance.name + "_ep_GDB_delegate", seL4_EndpointObject, read=True, write=True) -*/

/*- set BUFFER_BASE = c_symbol('BUFFER_BASE') -*/
#define /*? BUFFER_BASE ?*/ ((void*)&seL4_GetIPCBuffer()->msg[0])

/*- set methods_len = len(me.from_interface.type.methods) -*/
/*- set instance = me.from_instance.name -*/
/*- set interface = me.from_interface.name -*/
/*- set threads = list(range(1, 2 + len(me.from_instance.type.provides) + len(me.from_instance.type.uses) + len(me.from_instance.type.emits) + len(me.from_instance.type.consumes))) -*/

#define GDB_READ_MEM 0

int /*? me.from_interface.name ?*/__run(void) {
    return 0;
}

int /*? me.to_instance.name ?*/_read_memory(seL4_Word addr, seL4_Word length) {
  /*- set info = c_symbol('info') -*/
  seL4_SetMR(0, GDB_READ_MEM);
  seL4_SetMR(1, addr);
  seL4_SetMR(2, length);
  seL4_MessageInfo_t /*? info ?*/ = seL4_MessageInfo_new(0, 0, 0, 3);
  seL4_Send(/*? ep ?*/, /*? info ?*/);
  /*? info ?*/ = seL4_Recv(/*? ep ?*/, NULL);
  printf("Received value %p\n", (void *) seL4_GetMR(0));
  return seL4_GetMR(0);
}