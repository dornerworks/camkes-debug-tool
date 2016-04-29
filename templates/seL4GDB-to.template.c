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
#include <stdarg.h>

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

static int handle_command(char* command);
static seL4_Word reg_pc;
static seL4_Word badge;
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
        seL4_Recv(/*? ep ?*/, &badge);
        stream_read = true;
        reg_pc = seL4_GetMR(0);
        gdb_printf("Fault received on /*? me.to_interface.name ?*/\n");
        if (DEBUG_PRINT) gdb_printf("Fault address %08x\n", seL4_GetMR(1));
        if (DEBUG_PRINT) gdb_printf("Badge %08x\n", badge);
        seL4_CNode_SaveCaller(/*? cnode ?*/, /*? reply_cap_slot ?*/, 32);
        if (seL4_GetMR(2)) {
            if (DEBUG_PRINT) printf("Instruction fault\n");
            handle_breakpoint();
        }
        // Start accepting GDB input
        serial_irq_reg_callback(serial_irq_rcv, 0);
        //eth_irq_reg_callback(eth_irq_rcv, 0);
        // TODO Start accepting ethernet input
    }
    UNREACHABLE();
}



static int handle_command(char* command) {
    switch (command[0]) {
        case '!':
            // Enable extended mode
            if (DEBUG_PRINT) printf("Not implemented: enable extended mode\n");
            break;
        case '?':
            // Halt reason
            GDB_halt_reason(command);
            break;
        case 'A':
            // Argv
            if (DEBUG_PRINT) printf("Not implemented: argv\n");
            break;
        case 'b':
            if (command[1] == 'c') {
                // Backward continue
                if (DEBUG_PRINT) printf("Not implemented: backward continue\n");
                break;
            } else if (command[1] == 's') {
                // Backward step
                if (DEBUG_PRINT) printf("Not implemented: backward step\n");
                break;
            } else {
               // Set baud rate
                if (DEBUG_PRINT) printf("Not implemented: Set baud rate\n"); 
                break;
            } 
        case 'c':
            // Continue
            if (DEBUG_PRINT) printf("Continuing\n");
            break;
        case 'C':
            // Continue with signal
            if (DEBUG_PRINT) printf("Not implemented: continue with signal\n");
            break;
        case 'd':
            if (DEBUG_PRINT) printf("Not implemented: toggle debug\n");
            break;
        case 'D':
            if (DEBUG_PRINT) printf("Not implemented: detach\n");
            break;
        case 'F':
            if (DEBUG_PRINT) printf("Not implemented: file IO\n");
            break;
        case 'g':
            if (DEBUG_PRINT) printf("Reading general registers\n");
            GDB_read_general_registers(command);
            break;
        case 'G':
            if (DEBUG_PRINT) printf("Write general registers\n");
            GDB_write_general_registers(command);
            break;
        case 'H':
            if (DEBUG_PRINT) printf("Set thread ignored\n");
            GDB_set_thread(command);
            break;
        case 'i':
            if (DEBUG_PRINT) printf("Not implemented: cycle step\n");
            break;
        case 'I':
            if (DEBUG_PRINT) printf("Not implemented: signal + cycle step\n");
            break;
        case 'k':
            if (DEBUG_PRINT) printf("Not implemented: kill\n");
            break;
        case 'm':
            if (DEBUG_PRINT) printf("Reading memory\n");
            GDB_read_memory(command);
            break;
        case 'M':
            if (DEBUG_PRINT) printf("Writing memory\n");
            GDB_write_memory(command);
            break;
        case 'p':
            if (DEBUG_PRINT) printf("Read register\n");
            GDB_read_register(command);
            break;
        case 'P':
            if (DEBUG_PRINT) printf("Not implemented: write register\n");
            break;
        case 'q':
            if (DEBUG_PRINT) printf("Query\n");
            GDB_query(command);
            break;
        case 'Q':
            if (DEBUG_PRINT) printf("Not implemented: set\n");
            break;
        case 'r':
            if (DEBUG_PRINT) printf("Not implemented: reset\n");
            break;
        case 'R':
            if (DEBUG_PRINT) printf("Not implemented: restart\n");
            break;
        case 's':
            if (DEBUG_PRINT) printf("Not implemented: step\n");
            break;
        case 'S':
            if (DEBUG_PRINT) printf("Not implemented: step + signal\n");
            break;
        case 't':
            if (DEBUG_PRINT) printf("Not implemented: search\n");
            break;
        case 'T':
            if (DEBUG_PRINT) printf("Not implemented: check thread\n");
            break;
        case 'v':
            if (DEBUG_PRINT) printf("Not implemented: v commands\n");
            break;
        case 'X':
            if (DEBUG_PRINT) printf("Not implemented: write binary\n");
            break;
        case 'z':
            if (DEBUG_PRINT) printf("Not implemented: remove point\n");
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
            if (DEBUG_PRINT) printf("Unknown command\n");
    }
    
    return 0;
}

