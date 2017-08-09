/*
 * Copyright 2017, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DORNERWORKS_GPL)
 *
 * This data was produced by DornerWorks, Ltd. of Grand Rapids, MI, USA under
 * a DARPA SBIR, Contract Number D16PC00107.
 *
 * Approved for Public Release, Distribution Unlimited.
 *
 */

#define putDebugChar(x) uart_send_char(x)
#define getDebugChar    uart_get_char

void uart_irq_handle(void * cookie)
{
    if (!gdb_enabled())
    {
        return;
    }

    uart_handle_interrupt();
    uart_irq_reg_callback(uart_irq_handle, NULL);
}

char* getpacket (void)
{
    char *buffer = remcomInBuffer;
    u8 checksum;
    u8 xmitcsum;
    int count;
    char ch;

    while (1)
    {
        ch = 0;
        /* wait around for the start character, ignore all other characters */
        do
        {
            ch = getDebugChar();
        } while (ch != '$');

    retry:
        checksum = 0;
        xmitcsum = -1;
        count = 0;

        /* read until a # or end of buffer is found */
        while (count < BUFMAX - 1)
        {
            ch = getDebugChar ();
            if (ch == '$')
                goto retry;
            if (ch == '#')
                break;

            seL4_Yield();

            checksum = checksum + ch;
            buffer[count] = ch;
            count = count + 1;
        }
        buffer[count] = 0;

        if (ch == '#')
        {
            ch = getDebugChar ();
            seL4_Yield();
            xmitcsum = hex (ch) << 4;
            ch = getDebugChar ();
            xmitcsum += hex (ch);

            if (checksum != xmitcsum)
            {
                putDebugChar ('-');    /* failed checksum */
            }
            else
            {
                putDebugChar ('+');    /* successful transfer */

                /* if a sequence char is present, reply the sequence ID */
                if (buffer[2] == ':')
                {
                    putDebugChar (buffer[0]);
                    putDebugChar (buffer[1]);
                    dbg_printf("<<$%s#\n",buffer+3);
                    return &buffer[3];
                }
                dbg_printf("<<$%s#\n",buffer);
                return &buffer[0];
            }
        }
    }

    return NULL; // can't get here
}

/* calculates checksum of message in buffer and sends it to host */
void putpacket (char *buffer)
{
    u8 checksum;
    int count;
    char ch;

    /*  $<packet info>#<checksum>. */
    do
    {
        putDebugChar('$');
        checksum = 0;
        count = 0;

        while (0 != (ch = buffer[count]))
        {
            putDebugChar(ch);
            checksum += ch;
            count += 1;
        }

        putDebugChar('#');
        putDebugChar(hexchars[checksum >> 4]);
        putDebugChar(hexchars[checksum & 0xf]);
    }
    while (getDebugChar() != '+');

    dbg_printf(">>$%s#\n",buffer);
}
