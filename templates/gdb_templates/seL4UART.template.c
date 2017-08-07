/*
 * Copyright 2017, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DW_GPL)
 */

/* General structure taken from this website:
 *  - https://stratifylabs.co/embedded%20design%20tips/2013/10/02/Tips-A-FIFO-Buffer-Implementation/
 */

#include <camkes.h>
#include <camkes/io.h>

#include <stdio.h>
#include <string.h>

#include <platsupport/io.h>
#include <platsupport/chardev.h>
#include <platsupport/plat/serial.h>

#include <sel4/sel4.h>

/*- set uart_num = configuration[me.from_instance.name].get('%s_num' % me.from_interface.name) -*/

ps_chardevice_t uart;

#define UART_NUMBER /*? uart_num ?*/-1
#define UART_MIN    1
#define UART_MAX    5

/* MAX_CHAR needs to be a power-of-two - 1 */
#define MAX_CHAR    255

typedef struct {
    char * buf;
    int head;
    int tail;
    int size;
} uart_fifo_t;

volatile uart_fifo_t uart_fifo_buffer;
static char * fifo_buffer;

extern void uart_irq_handle(void * cookie);

int /*? me.from_interface.name ?*/_char_ready(void)
{
    /* if the head == tail, get the characters */
    if (uart_fifo_buffer.head == uart_fifo_buffer.tail)
    {
        return 0;
    }
    else
    {
        if (uart_fifo_buffer.head > uart_fifo_buffer.tail)
        {
            return uart_fifo_buffer.head - uart_fifo_buffer.tail;
        }
        /* tail > head */
        else
        {
            return (MAX_CHAR - (uart_fifo_buffer.tail - uart_fifo_buffer.head));
        }
    }
}

/* Framework for an interrupt handler that the user can configure to do whatever...
 *
 * Stores into a FIFO Buffer for the get_string function to read
 */
void /*? me.from_interface.name ?*/_handle_interrupt(void)
{
    char c;
    while(ps_cdev_read(&uart, &c, 1, NULL, NULL) < 1);

    if (((uart_fifo_buffer.head + 1) & MAX_CHAR) == uart_fifo_buffer.tail)
    {
        return;
    }
    else
    {
        uart_fifo_buffer.buf[uart_fifo_buffer.head++] = c;
        uart_fifo_buffer.head %= uart_fifo_buffer.size;
    }
}

char /*? me.from_interface.name ?*/_get_char(void)
{
    char ret = 0;
    if (uart_fifo_buffer.tail != uart_fifo_buffer.head)
    {
        /* Grab a byte and increment the tail */
        ret = uart_fifo_buffer.buf[uart_fifo_buffer.tail++];
        uart_fifo_buffer.tail %= uart_fifo_buffer.size;
    }

    return ret;
}

int /*? me.from_interface.name ?*/_send_char(char buf)
{
    while(ps_cdev_write(&uart, &buf, 1, NULL, NULL) < 1);
    return 1;
}

void /*? me.from_interface.name ?*/__init(void)
{
    /* Ensure that the configured UART is in-range */
    assert(/*? uart_num ?*/ >= UART_MIN && /*? uart_num ?*/ <= UART_MAX);

    ps_io_ops_t io_ops;
    camkes_io_ops(&io_ops);
    ps_chardevice_t *uart_ptr = ps_cdev_init(UART_NUMBER, &io_ops, &uart);

    /* Ensure serial port was configured correctly */
    assert(uart_ptr == &uart);

    /* Initialize our FIFO Buffer */
    fifo_buffer = (char *)malloc(MAX_CHAR);

    uart_fifo_buffer.head = 0;
    uart_fifo_buffer.tail = 0;
    uart_fifo_buffer.size = MAX_CHAR;
    uart_fifo_buffer.buf = fifo_buffer;

    uart_irq_reg_callback(uart_irq_handle, NULL);
}
