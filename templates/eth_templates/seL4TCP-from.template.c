/*
 * Copyright 2014, NICTA
 * Copyright 2017, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4/sel4.h>
#include <camkes/dataport.h>
#include <utils/util.h>
#include <string.h>
#include <lwip/ip_addr.h>
#include <lwip/tcp_impl.h>

#define TCP_SEND   0
#define TCP_POLL   1

/*? macros.show_includes(me.from_instance.type.includes) ?*/

/*- set ack_ep = alloc('ack_ep', seL4_EndpointObject, write=True, read=True) -*/
/*- set ep = alloc('ep', seL4_EndpointObject, write=True, grant=True) -*/

/*- set badge = configuration[me.from_instance.name].get('%s_attributes' % me.from_interface.name) -*/
/*- if badge is not none -*/
    /*- set badge = badge.strip('"') -*/
    /*- do cap_space.cnode[ep].set_badge(int(badge, 0)) -*/
/*- endif -*/

/* assume a dataport symbols exists */
extern void */*? me.from_interface.name?*/_send_buf;

int /*? me.from_interface.name ?*/_poll(unsigned int *len)
{
    int status;
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);

    seL4_SetMR(0, TCP_POLL);
    tag = seL4_Call(/*? ep ?*/, tag);

    status = seL4_GetMR(0);
    *len = (status != -1 ? seL4_GetMR(1) : 0);

    return status;
}

int /*? me.from_interface.name ?*/_send(void *p, unsigned int len)
{
    void * buffer = /*? me.from_interface.name?*/_send_buf;
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);

    memcpy(buffer, p, len);

    seL4_SetMR(0, TCP_SEND);
    seL4_SetMR(1, len);
    seL4_Call(/*? ep ?*/, tag);

    return len;
}

/*- set is_reader = True -*/
/*- set instance = me.from_instance.name -*/
/*- set interface = me.from_interface.name -*/
/*- include 'global-endpoint.template.c' -*/
/*- set aep = pop('notification') -*/

seL4_CPtr /*? me.from_interface.name ?*/_aep(void) {
    return /*? aep ?*/;
}
