/*#
 *# Copyright 2014, NICTA
 *#
 *# This software may be distributed and modified according to the terms of
 *# the BSD 2-Clause license. Note that NO WARRANTY is provided.
 *# See "LICENSE_BSD2.txt" for details.
 *#
 *# @TAG(NICTA_BSD)
 #*/

/*? macros.show_includes(me.from_instance.type.includes) ?*/
/*? macros.show_includes(me.from_interface.type.includes, '../static/components/' + me.from_instance.type.name + '/') ?*/

/*- set thread_caps = [] -*/
/*- set fault_obj = alloc_obj(me.from_instance.name + "_ep_fault", seL4_EndpointObject, read=True, write=True, grant=True) -*/
 
/*- for cap in cap_space.cnode: -*/
    /*- if isinstance(cap_space.cnode[cap].referent, capdl.Object.TCB): -*/
        /*- set cap_name = cap_space.cnode[cap].referent.name-*/
        /*- do thread_caps.append((cap, cap_name)) -*/
    /*- endif -*/
/*- endfor -*/

/*- for cap, cap_name in thread_caps: -*/
    /*- set fault_cap = alloc_cap(cap_name + "_fault_ep", fault_obj, read=True, write=True, grant=True) -*/
    /*- do cap_space.cnode[fault_cap].set_badge(cap) -*/
    /*- do cap_space.cnode[cap].set_fault_ep_slot(fault_cap) -*/
/*- endfor -*/