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
#include <camkes.h>


/*? macros.show_includes(me.from_instance.type.includes) ?*/
/*? macros.show_includes(me.from_interface.type.includes, '../static/components/' + me.from_instance.type.name + '/') ?*/
/*- set ep = alloc(me.to_instance.name + "_ep_GDB_delegate", seL4_EndpointObject, read=True, write=True) -*/
/*- set BUFFER_BASE = c_symbol('BUFFER_BASE') -*/
#define /*? BUFFER_BASE ?*/ ((void *)&seL4_GetIPCBuffer()->msg[0])

/*- set methods_len = len(me.from_interface.type.methods) -*/
/*- set instance = me.from_instance.name -*/
/*- set interface = me.from_interface.name -*/
/*- set threads = list(range(1, 2 + len(me.from_instance.type.provides) + len(me.from_instance.type.uses) + len(me.from_instance.type.emits) + len(me.from_instance.type.consumes))) -*/
/*- include 'GDB_delegate.h' -*/

int /*? me.from_interface.name ?*/__run(void) {
    return 0;
}

unsigned char * /*? me.to_instance.name ?*/_read_memory(seL4_Word addr, seL4_Word length, unsigned char *data) {
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, DELEGATE_READ_NUM_ARGS);
    // Setup arguments for call
    seL4_SetMR(DELEGATE_COMMAND_REG, GDB_READ_MEM);
    seL4_SetMR(DELEGATE_ARG(0), addr);
    seL4_SetMR(DELEGATE_ARG(1), length);
    // Send
    seL4_Send(/*? ep ?*/, info);
    info = seL4_Recv(/*? ep ?*/, NULL);
    // Loop variables
    unsigned char byte = 0;
    seL4_Word message = 0;
    // Unpack data from message
    for (int i = 0; i < length; i++) {
        if (i % BYTES_IN_WORD == 0) {
            message = seL4_GetMR(i/BYTES_IN_WORD);
        }
        byte = (message & FIRST_BYTE_MASK) >> FIRST_BYTE_BITSHIFT;
        message <<= BYTE_SHIFT;   
        data[i] = byte; 
    }
    return data;
}


seL4_Word /*? me.to_instance.name ?*/_write_memory(seL4_Word addr, seL4_Word length, 
                                                   unsigned char * data) {
    // Get length of data in MRs
    seL4_Word data_MR_length = CEIL_MR(length);
    // Generate message
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, DELEGATE_WRITE_NUM_ARGS + data_MR_length);
    // Loop variables
    unsigned char byte = 0;
    seL4_Word message = 0;
    // Pack data for messaging
    for (int i = 0; i < length; i++) {
        byte = *((unsigned char *)data + i);
        message |= ((seL4_Word) byte) << ((i % BYTES_IN_WORD) * BYTE_SIZE);
        // Check if last byte in message, and if so set MR and clear temp variable
        if ((i+1) % BYTES_IN_WORD == 0 || i == length-1) {
            seL4_SetMR((i / BYTES_IN_WORD) + DELEGATE_WRITE_NUM_ARGS, message);
            message = 0;
        }     
    }
    // Setup arguments for call
    seL4_SetMR(DELEGATE_COMMAND_REG, GDB_WRITE_MEM);
    //printf("Command register %d\n", seL4_GetMR(0));
    seL4_SetMR(DELEGATE_ARG(0), addr);
    seL4_SetMR(DELEGATE_ARG(1), length);
    // Send
    seL4_Send(/*? ep ?*/, info);
    info = seL4_Recv(/*? ep ?*/, NULL);
    return seL4_GetMR(0);
}

void /*? me.to_instance.name ?*/_read_registers(seL4_Word tcb_cap, seL4_Word registers[]) {
    // Generate message
    seL4_Word length;
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, DELEGATE_REGS_READ_NUM_ARGS);
    // Setup arguments for call
    seL4_SetMR(DELEGATE_COMMAND_REG, GDB_READ_REGS);
    seL4_SetMR(DELEGATE_ARG(0), tcb_cap);
    // Send
    seL4_Send(/*? ep ?*/, info);
    info = seL4_Recv(/*? ep ?*/, NULL);
    // Get data from message
    length = seL4_MessageInfo_get_length(info);
    for (int i = 0; i < length; i++) {
        registers[i] = seL4_GetMR(i);    
    }
}

void /*? me.to_instance.name ?*/_read_register(seL4_Word tcb_cap, seL4_Word *reg, seL4_Word reg_num) {
    // Generate message
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, DELEGATE_REG_READ_NUM_ARGS);
    // Setup arguments for call
    seL4_SetMR(DELEGATE_COMMAND_REG, GDB_READ_REG);
    seL4_SetMR(DELEGATE_ARG(0), tcb_cap);
    seL4_SetMR(DELEGATE_ARG(1), reg_num);
    // Send
    seL4_Send(/*? ep ?*/, info);
    info = seL4_Recv(/*? ep ?*/, NULL);
    *reg = seL4_GetMR(0);   
}

void /*? me.to_instance.name ?*/_write_registers(seL4_Word tcb_cap, seL4_Word registers[], int len) {
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, DELEGATE_REG_WRITE_NUM_ARGS);
    // Setup arguments for call
    seL4_SetMR(DELEGATE_COMMAND_REG, GDB_WRITE_REG);
    seL4_SetMR(DELEGATE_ARG(0), tcb_cap);
    for (int i = 0; i < len; i++) {
        seL4_SetMR(DELEGATE_ARG(i+1), registers[i]);    
    }
    for (int i = len; i < (sizeof(seL4_UserContext) / sizeof(seL4_Word)); i++) {
        seL4_SetMR(DELEGATE_ARG(i+1), 0);
    }
    // Send
    seL4_Send(/*? ep ?*/, info);
    info = seL4_Recv(/*? ep ?*/, NULL);
}