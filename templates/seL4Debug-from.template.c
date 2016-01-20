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
/*- include 'GDB_delegate.h' -*/

int /*? me.from_interface.name ?*/__run(void) {
    return 0;
}

char* /*? me.to_instance.name ?*/_read_memory(seL4_Word addr, seL4_Word length, uint8_t *data_string) {
// Setup arguments for call
  seL4_SetMR(0, GDB_READ_MEM);
  seL4_SetMR(1, addr);
  seL4_SetMR(2, length);
  // Generate message
  seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 3);
  // Send
  seL4_Send(/*? ep ?*/, info);
  info = seL4_Recv(/*? ep ?*/, NULL);
  // Unpack data
  uint8_t data[length];
  // Loop variables
  uint8_t byte = 0;
  seL4_Word message = 0;
  // Unpack data from message
  for (int i = 0; i < length; i++) {
    if (i%4 == 0) {
      message = seL4_GetMR(i/4);
    }
    byte = (message & 0xFF000000) >> 24;
    message <<= 8;   
    data[i] = byte; 
  }
  for (int i = 0; i < length; i++) {
    sprintf(&data_string[2*i], "%02x", data[i]);
  }
  return data_string;
}


seL4_Word /*? me.to_instance.name ?*/_write_memory(seL4_Word addr, seL4_Word length, char* data) {
  // Setup arguments for call
  seL4_SetMR(0, GDB_WRITE_MEM);
  seL4_SetMR(1, addr);
  seL4_SetMR(2, length);
  // Get length of data in MRs
  seL4_Word data_MR_length = (length + 3)/4;
  // Generate message
  seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, data_MR_length + 3);
  // Loop variables
  uint8_t byte = 0;
  seL4_Word message = 0;
  // Pack data for messaging
  for (int i = 0; i < length; i++) {
    byte = *((char*)data + i);
    message |= ((seL4_Word) byte) << ((i%4)*8);
    if ((i+1)%4 == 0 || i == length-1) {
        seL4_SetMR((i/4) + 3, message);
        message = 0;
    }     
  }
  // Send
  seL4_Send(/*? ep ?*/, info);
  info = seL4_Recv(/*? ep ?*/, NULL);
  return seL4_GetMR(0);
}