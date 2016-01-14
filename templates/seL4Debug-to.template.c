/*#
 *# Copyright 2014, NICTA
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
#include <camkes/marshal.h>
#include <camkes/error.h>
#include <camkes/tls.h>
#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <camkes/dataport.h>
#include <utils/util.h>

/*? macros.show_includes(me.to_instance.type.includes) ?*/
/*? macros.show_includes(me.to_interface.type.includes, '../static/components/' + me.to_instance.type.name + '/') ?*/


/*- set methods_len = len(me.to_interface.type.methods) -*/
/*- set instance = me.to_instance.name -*/
/*- set interface = me.to_interface.name -*/
/*- set size = 'seL4_MsgMaxLength * sizeof(seL4_Word)' -*/
/*- set allow_trailing_data = False -*/
/*- set ep = alloc(me.to_instance.name + "_ep_GDB_delegate", seL4_EndpointObject, read=True, write=True) -*/
/*- set cnode = alloc_cap('cnode', my_cnode, write=True) -*/
/*- set reply_cap_slot = alloc_cap('reply_cap_slot', None) -*/
/*- set info = c_symbol('info') -*/

int /*? me.to_interface.name ?*/__run(void) {   
    while (1) { 
        // Receive RPC and reply
        seL4_MessageInfo_t /*? info ?*/ = seL4_Recv(/*? ep ?*/, NULL);
        seL4_Word addr = seL4_GetMR(0);
        seL4_Word length = seL4_GetMR(1);
        seL4_Word message = 0;
        uint8_t byte = 0;
        printf("addr: %p\n", (void *)addr);
        printf("length: %d\n", length);
        for (int i = 0; i < length; i++) {
        	byte = *((char*)addr + i);
        	message |= ((seL4_Word) byte) << ((i%4)*8);
        	printf("byte: 0x%02x\n", byte & 0xFF);
        	if ((i+1)%4 == 0 || i == length-1) {
        		seL4_SetMR(i/4, message);
        		printf("value: %p\n", (void *) message);
        		message = 0;
        	}
        	
        }
        
       	
        seL4_Send(/*? ep ?*/, /*? info ?*/);
    }
    UNREACHABLE();
}
