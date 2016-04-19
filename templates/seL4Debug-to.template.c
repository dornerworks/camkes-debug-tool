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
#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <utils/util.h>
#include <camkes.h>

/*? macros.show_includes(me.to_instance.type.includes) ?*/
/*? macros.show_includes(me.to_interface.type.includes, '../static/components/' + me.to_instance.type.name + '/') ?*/

/*- set methods_len = len(me.to_interface.type.methods) -*/
/*- set instance = me.to_instance.name -*/
/*- set interface = me.to_interface.name -*/
/*- set size = 'seL4_MsgMaxLength * sizeof(seL4_Word)' -*/
/*- set allow_trailing_data = False -*/
/*- set ep = alloc(me.to_instance.name + "_ep_GDB_delegate", seL4_EndpointObject, read=True, write=True) -*/
/*- set cnode = alloc_cap('cnode', my_cnode, write=True) -*/
/*- set info = c_symbol('info') -*/
 /*- set reply_cap_slot = alloc_cap('reply_cap_slot', None) -*/
/*- include 'GDB_delegate.h' -*/

static void read_memory(void);
static void write_memory(void);
static void read_register(void);
static void read_registers(void);
static void write_registers(void);

int /*? me.to_interface.name ?*/__run(void) {   
    while (1) { 
        // Receive RPC and reply
        seL4_Recv(/*? ep ?*/, NULL);
        seL4_Word instruction = seL4_GetMR(DELEGATE_COMMAND_REG);
        seL4_MessageInfo_t info;
        switch (instruction) {
            case GDB_READ_MEM:
                read_memory();
                break;
            case GDB_WRITE_MEM:
                write_memory();
                break;
            case GDB_READ_REGS:
                read_registers();
                break;
            case GDB_WRITE_REG:
                write_registers();
                break;
            case GDB_READ_REG:
                read_register();
                break;
            default:
                printf("Unrecognised command %d %d %d\n", instruction, seL4_GetMR(1), seL4_GetMR(2));
                info = seL4_MessageInfo_new(0, 0, 0, 1);
                seL4_Send(/*? ep ?*/, info);
                continue;
        }
    }
    UNREACHABLE();
}

static void read_memory(void) {
    seL4_MessageInfo_t info;
    seL4_Word addr = seL4_GetMR(DELEGATE_ARG(0));
    seL4_Word length = seL4_GetMR(DELEGATE_ARG(1));
    //seL4_CNode_SaveCaller(/*? cnode ?*/, /*? reply_cap_slot ?*/, 32);
    seL4_Word message = 0;
    unsigned char byte = 0;
    // Pack data for messaging   
    for (int i = 0; i < length; i++) {
        byte = *((unsigned char*)(addr + i));
        message |= ((seL4_Word) byte) << ((FIRST_BYTE_BITSHIFT - (i % BYTES_IN_WORD) * BYTE_SHIFT));
        if ((i+1) % BYTES_IN_WORD == 0 || i == length-1) {
            seL4_SetMR(i / BYTES_IN_WORD, message);
            message = 0;
        }
    }
    info = seL4_MessageInfo_new(0, 0, 0, CEIL_MR(length));
    seL4_Send(/*? ep ?*/, info);
}

static void write_memory(void) {
    //printf("Writing memory\n");
    seL4_MessageInfo_t info;
    char *addr = (char *) seL4_GetMR(DELEGATE_ARG(0));
    seL4_Word length = seL4_GetMR(DELEGATE_ARG(1));
    seL4_Word message = 0;
    unsigned char byte = 0;
    char data[length];
    // Unpack data from message
    for (int i = 0; i < length; i++) {
        if (i % BYTES_IN_WORD == 0) {
          message = seL4_GetMR(DELEGATE_WRITE_NUM_ARGS + (i / BYTES_IN_WORD));
        }
        byte = message & LAST_BYTE_MASK;
        message >>= BYTE_SHIFT;   
        data[i] = byte; 
    }
    memcpy(addr, data, length);
    info = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(/*? ep ?*/, info);
}

static void read_registers(void) {
    seL4_UserContext regs = {0};
    int err;
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, num_regs);
    seL4_Word tcb_cap = seL4_GetMR(DELEGATE_ARG(0));
    err = seL4_TCB_ReadRegisters(tcb_cap, false, 0, num_regs, &regs);
    seL4_Word *reg_word = (seL4_Word *) (& regs);
    for (int i = 0; i < num_regs; i++) {
        seL4_SetMR(i, reg_word[i]);
    }
    seL4_Send(/*? ep ?*/, info);
}

static void read_register(void) {
    seL4_UserContext regs = {0};
    int err;
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_Word tcb_cap = seL4_GetMR(DELEGATE_ARG(0));
    seL4_Word reg_num = seL4_GetMR(DELEGATE_ARG(1));
    err = seL4_TCB_ReadRegisters(tcb_cap, false, 0, num_regs, &regs);
    seL4_Word *reg_word = (seL4_Word *) (& regs);
    seL4_SetMR(0, reg_word[reg_num]);
    seL4_Send(/*? ep ?*/, info);
}

static void write_registers(void) {
    seL4_MessageInfo_t info;
    seL4_UserContext regs;
    seL4_Word *reg_word = (seL4_Word *) (&regs);
    int err;
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    seL4_Word tcb_cap = seL4_GetMR(DELEGATE_ARG(0));
    printf("Writing registers on cap: %d\n", tcb_cap);
    for (int i = 0; i < num_regs; i++) {
        reg_word[i] = seL4_GetMR(DELEGATE_ARG(i+1));
        //regs.eip = seL4_GetMR(DELEGATE_ARG(1));
        printf("%d 0x%x\n", i, reg_word[i]);
    }
    err = seL4_TCB_WriteRegisters(tcb_cap, false, 0, num_regs, &regs);
    printf("Write registers err: %d\n", err);
    seL4_Send(/*? ep ?*/, info);
}