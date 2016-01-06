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
/*- set ep = alloc(me.to_interface.name, seL4_EndpointObject, read=True, write=True, grant=True) -*/
/*- set cnode = alloc_cap('cnode', my_cnode, write=True) -*/
/*- set reply_cap_slot = alloc_cap('reply_cap_slot', None) -*/
/*- set info = c_symbol('info') -*/

void print_fault(seL4_MessageInfo_t info, seL4_Word label);
void print_message_registers(unsigned int size);
void print_VMFault (void);
void print_MessageInfo(seL4_MessageInfo_t info);
void send_reply(void);
void print_registers(void);

int /*? me.to_interface.name ?*/__run(void) {
    //debug_fault_handler(/*? ep ?*/, NULL);
    /*# Check any typedefs we have been given are not arrays. #*/
    while (1) { 
        seL4_Word badge;
        seL4_Word label;
        int i;
        seL4_MessageInfo_t /*? info ?*/ = seL4_Recv(/*? ep ?*/, &badge);
        printf("Fault received on /*? me.to_interface.name ?*/\n");
        label = seL4_MessageInfo_get_label(/*? info ?*/);
        print_fault(/*? info ?*/, label);
        switch(label) {
            //case seL4_VMFault:
                //print_VMFault();
                //break;
            //default:
                //printf("Unknown fault type: %d\n", label);
        }
        print_registers();
        //send_reply();
    }
    UNREACHABLE();
}

void print_fault(seL4_MessageInfo_t info, seL4_Word label) { 
	unsigned int size = seL4_MessageInfo_get_length(info) * sizeof(seL4_Word);
	printf("/*? me.to_interface.name ?*/ received a fault\n");
    print_MessageInfo(info);
    print_message_registers(size);
    
}

void print_MessageInfo(seL4_MessageInfo_t info) {
	unsigned int size = seL4_MessageInfo_get_length(info) * sizeof(seL4_Word);
	seL4_Word label = seL4_MessageInfo_get_label(info);
	seL4_Word extraCaps = seL4_MessageInfo_get_extraCaps(info);
	seL4_Word capsUnwrapped = seL4_MessageInfo_get_capsUnwrapped(info);
    assert(size <= seL4_MsgMaxLength * sizeof(seL4_Word));
    printf("Label: %d\n", label);
    printf("Message length: %d\n", size);
    printf("Extra caps: %p\n", (void *) extraCaps);
    printf("Caps Unwrapped: %p\n", (void *) capsUnwrapped);
    
}

void print_message_registers(unsigned int size) {
    printf("\n");
    int i;
    for (i = 0; i < size/4; i++) {
        printf("MR%d: %p\n", i, (void *) seL4_GetMR(i));
    }
    printf("\n");
}

void print_registers(void) {
    sel4debug_dump_registers(1);
    printf("\n");
}

void print_VMFault(void) {
	 printf("Fault type: seL4_VMFault\n");
    printf("PC: %p\n", (void *) seL4_GetMR(0));
    switch (seL4_GetMR(2)) {
    	case 0:
    		printf("Data fault\n");
    		break;
    	case 1:
    		printf("Instruction fault\n");
    		break;
    	default:
    		printf("Error in MR2\n");
    }
    printf("Fault Vaddr: %p\n", (void *) seL4_GetMR(1));
    printf("FSR: %p\n",(void *) seL4_GetMR(3));
}

void send_reply(void) {
    seL4_Error err = seL4_CNode_SaveCaller(/*? cnode ?*/, /*? reply_cap_slot ?*/, 32);
    assert(err==0);
    seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(/*? reply_cap_slot ?*/, reply);
}