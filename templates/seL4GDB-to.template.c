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
/*- set ep = alloc(me.from_instance.name + "_ep_fault", seL4_EndpointObject, read=True, write=True, grant=True) -*/
/*- set cnode = alloc_cap('cnode', my_cnode, write=True) -*/
/*- set reply_cap_slot = alloc_cap('reply_cap_slot', None) -*/
/*- set info = c_symbol('info') -*/

/*- include 'serial.h' -*/
/*- include 'gdb.h' -*/

static void send_reply(void);
static int handle_command(char* command);
static seL4_Word reg_pc;

/*- include 'serial.c' -*/
/*- include 'gdb.c' -*/

void /*? me.to_interface.name ?*/__init(void) {
    serial_init();
    breakpoint_init();
}

int /*? me.to_interface.name ?*/__run(void) {
    /*# Check any typedefs we have been given are not arrays. #*/
    // Make connection to gdb
    while (1) { 
        seL4_Word badge;
        seL4_Recv(/*? ep ?*/, &badge);
        stream_read = true;
        reg_pc = seL4_GetMR(0);
        printf("Fault received on /*? me.to_interface.name ?*/\n");
        printf("PC %08x\n", reg_pc);
        printf("Fault address %08x\n", seL4_GetMR(1));
        seL4_CNode_SaveCaller(/*? cnode ?*/, /*? reply_cap_slot ?*/, 32);
        if (seL4_GetMR(2)) {
            printf("Instruction fault\n");
            handle_breakpoint();
        }
        // Start accepting GDB input
        serial_irq_reg_callback(serial_irq_rcv, 0);
        //eth_irq_reg_callback(eth_irq_rcv, 0);
        // TODO Start accepting ethernet input
        //send_reply();
    }
    UNREACHABLE();
}

static void send_reply(void) {
    stream_read = false;
    //assert(err==0);
    seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, reg_pc);
    seL4_Send(/*? reply_cap_slot ?*/, reply);
}



static int handle_command(char* command) {
    switch (command[0]) {
        case '!':
            // Enable extended mode
            printf("Not implemented: enable extended mode\n");
            break;
        case '?':
            // Halt reason
            printf("Not implemented: halt reason\n");
            break;
        case 'A':
            // Argv
            printf("Not implemented: argv\n");
            break;
        case 'b':
            if (command[1] == 'c') {
                // Backward continue
                printf("Not implemented: backward continue\n");
                break;
            } else if (command[1] == 's') {
                // Backward step
                printf("Not implemented: backward step\n");
                break;
            } else {
               // Set baud rate
                printf("Not implemented: Set baud rate\n"); 
                break;
            } 
        case 'c':
            // Continue
            printf("Continuing\n");
            send_reply();
            break;
        case 'C':
            // Continue with signal
            printf("Not implemented: continue with signal\n");
            break;
        case 'd':
            printf("Not implemented: toggle debug\n");
            break;
        case 'D':
            printf("Not implemented: detach\n");
            break;
        case 'F':
            printf("Not implemented: file IO\n");
            break;
        case 'g':
            printf("Not implemented: read general registers\n");
            break;
        case 'G':
            printf("Not implemented: write general registers\n");
            break;
        case 'H':
            printf("Set thread ignored\n");
            GDB_set_thread(command);
            break;
        case 'i':
            printf("Not implemented: cycle step\n");
            break;
        case 'I':
            printf("Not implemented: signal + cycle step\n");
            break;
        case 'k':
            printf("Not implemented: kill\n");
            break;
        case 'm':
            printf("Reading memory\n");
            GDB_read_memory(command);
            break;
        case 'M':
            printf("Writing memory\n");
            GDB_write_memory(command);
            break;
        case 'p':
            printf("Not implemented: read register\n");
            break;
        case 'P':
            printf("Not implemented: write register\n");
            break;
        case 'q':
            printf("Query\n");
            GDB_query(command);
            break;
        case 'Q':
            printf("Not implemented: set\n");
            break;
        case 'r':
            printf("Not implemented: reset\n");
            break;
        case 'R':
            printf("Not implemented: restart\n");
            break;
        case 's':
            printf("Not implemented: step\n");
            break;
        case 'S':
            printf("Not implemented: step + signal\n");
            break;
        case 't':
            printf("Not implemented: search\n");
            break;
        case 'T':
            printf("Not implemented: check thread\n");
            break;
        case 'v':
            printf("Not implemented: v commands\n");
            break;
        case 'X':
            printf("Not implemented: write binary\n");
            break;
        case 'z':
            printf("Not implemented: remove point\n");
            break;
        case 'Z':
            if (command[1] == '0') {
                printf("Inserting software breakpoint\n");
                GDB_insert_sw_breakpoint(command);
            } else {
                printf("Command Not supported\n");
            }     
            break;
        default:
            printf("Unknown command\n");
    }
    
    return 0;
}

