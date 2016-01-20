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
#include <stdio.h>
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
/*- include 'GDB_delegate.h' -*/

static void read_memory(void);
static void write_memory(void);

int /*? me.to_interface.name ?*/__run(void) {   
    while (1) { 
        // Receive RPC and reply
        seL4_Recv(/*? ep ?*/, NULL);
        seL4_Word instruction = seL4_GetMR(0);
        switch (instruction) {
            case GDB_READ_MEM:
                read_memory();
                break;
            case GDB_WRITE_MEM:
                write_memory();
            default:
                continue;
        }
    }
    UNREACHABLE();
}

static void read_memory(void) {
    seL4_MessageInfo_t info;
    seL4_Word addr = seL4_GetMR(1);
    seL4_Word length = seL4_GetMR(2);
    seL4_Word message = 0;
    uint8_t byte = 0;
    printf("Reading length %d\n", length);
    // Pack data for messaging
    for (int i = 0; i < length; i++) {
        byte = *((uint8_t*)addr + i);
        message |= ((seL4_Word) byte) << ((24 - (i%4)*8));
        if ((i+1)%4 == 0 || i == length-1) {
            seL4_SetMR(i/4, message);
            message = 0;
        }    
    }
    info = seL4_MessageInfo_new(0, 0, 0, (length + 3)/4); // ceiling
    seL4_Send(/*? ep ?*/, info);
}

static void write_memory(void) {
    seL4_MessageInfo_t info;
    char* addr = seL4_GetMR(1);
    seL4_Word length = seL4_GetMR(2);
    seL4_Word message = 0;
    uint8_t byte = 0;
    uint8_t data[length];
    uint8_t data_string[2*length];
    printf("Writing length\n", length);
    // Unpack data from message
    for (int i = 0; i < length; i++) {
        if (i%4 == 0) {
          message = seL4_GetMR(3 + i/4);
        }
        byte = message & 0xFF;
        message >>= 8;   
        data[i] = byte; 
    }
    for (int i = 0; i < length; i++) {
        sprintf(&data_string[2*i], "%02X", data[i]);
    }
    memcpy(addr, data, length);
    info = seL4_MessageInfo_new(0, 0, 0, 1); // ceiling
    seL4_SetMR(0, 0);
    seL4_Send(/*? ep ?*/, info);
}

