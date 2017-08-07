/*
 * Copyright 2017, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DW_GPL)
 */

#include <platsupport/plat/timer.h>

void /*? me.from_interface.name ?*/__init(void)
{
    /* TTC: Triple Timer Counter */
    timer_config_t config;

    config.vaddr = (void *)timer_reg;
    config.clk_src = NULL;

    /*? me.from_interface.name ?*/_drv = ps_get_timer(/*? timer_id ?*/, &config);
    assert(/*? me.from_interface.name ?*/_drv);

    /*? me.from_interface.name ?*/_timer_init(/*? timer_id ?*/);
}
