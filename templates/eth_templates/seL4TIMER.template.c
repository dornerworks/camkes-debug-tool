/*
 * Copyright 2017, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DW_GPL)
 */

#include <autoconf.h>

#include <stdio.h>

#include <platsupport/timer.h>

#include <camkes.h>

pstimer_t * /*? me.from_interface.name ?*/_drv = NULL;

/*- set timer_val = configuration[me.from_instance.name].get("%s_period_ns" % me.from_interface.name) -*/
/*- set timer_id = configuration[me.from_instance.name].get('%s_id' % me.from_interface.name) -*/
/*- set irq = configuration[me.to_instance.name].get('%s_irq_attributes' % me.from_interface.name) -*/

uint64_t /*? me.from_interface.name ?*/_get_period_ns(void)
{
   return /*? timer_val ?*/;
}

void /*? me.from_interface.name ?*/_handle_interrupt(void)
{
   timer_handle_irq(/*? me.from_interface.name ?*/_drv, /*? irq ?*/);
}

void /*? me.from_interface.name ?*/_timer_init(uint32_t timer_id)
{
    int ret;
    ret = timer_periodic(/*? me.from_interface.name ?*/_drv, /*? timer_val ?*/);
    assert(0 == ret);
    timer_start(/*? me.from_interface.name ?*/_drv);
}

/*- if os.environ.get('PLAT') == 'imx6' -*/
  /*- if configuration[me.from_instance.name].get('%s_epit' % me.from_interface.name) == 'true' -*/
    /*- include 'seL4EPIT.template.c' -*/
  /*- elif configuration[me.from_instance.name].get('%s_gpt' % me.from_interface.name) == 'true' -*/
    /*- include 'seL4GPT.template.c' -*/
  /*- else -*/
    /*? raise(Exception('Need to select a timer type')) ?*/
  /*- endif -*/
/*- elif os.environ.get('PLAT') == 'zynq7000' -*/
  /*- include 'seL4TTC.template.c' -*/
/*- else -*/
    /*? raise(Exception('Need to select a valid arm platform')) ?*/
/*- endif -*/
