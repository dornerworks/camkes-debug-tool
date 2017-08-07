/*
 * Copyright 2017, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DW_GPL)
 */
#include <platsupport/mach/gpt.h>
#include <platsupport/plat/timer.h>

void /*? me.from_interface.name ?*/__init(void)
{
    /* GPT: General Purpose Timer */
    gpt_config_t config;

    config.vaddr = (void *)timer_reg;
    config.prescaler = 0;

    /*? me.from_interface.name ?*/_drv = gpt_get_timer(&config);
    assert(/*? me.from_interface.name ?*/_drv);

    /*? me.from_interface.name ?*/_timer_init(0);
}
