/*
 * Copyright 2017, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DW_GPL)
 */

#include <platsupport/mach/epit.h>

void /*? me.from_interface.name ?*/__init(void)
{
    /* EPIT: Enhanced Periodic Interrupt Timer */
    epit_config_t config;

    config.vaddr = (void*)timer_reg;
    config.prescaler = 0;
    config.irq = /*? configuration[me.to_instance.name].get('%s_irq_attributes' % me.from_interface.name) ?*/;

    /*? me.from_interface.name ?*/_drv = epit_get_timer(&config);
    assert(/*? me.from_interface.name ?*/_drv);

    /*? me.from_interface.name ?*/_timer_init(/*? timer_id ?*/);
}
