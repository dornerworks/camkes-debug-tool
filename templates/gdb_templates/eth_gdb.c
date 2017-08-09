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

/* Function declarations for sending/receiving Ethernet packets */
int enet_getPacket(char * ret_buf);
void enet_putPacket(char * send_buf, int len);

typedef struct ethernet_packet {
    unsigned int len;
    uint8_t ready;
    char buf[BUFMAX];
} eth_pkt_t;

typedef struct gdb_acknowledgment {
    uint8_t recv;
    char ack;
} gdb_ack_t;

/* Global Structure for the RX Packet, the GDB Host information, and GDB Ackknowledgment info */
volatile static eth_pkt_t rx_packet = {0};
volatile static gdb_ack_t gdb_ack = {0};

/* Flags for whether we've received a packet from GDB Host (to get IP Addr and Port #) */
volatile uint8_t ethernet_func = 0;

/* _ready_callback:
 *
 *     Callback function for when a TCP Packet is ready!
 *
 *     packet has arrived and the information is copied to the global structure. If the
 *     received packet is 1, then an ack response could be present, which gets stored globally
 *     for the getAck function to use.
 *
 *     After the packet has been used, the buffer is cleared and this function is re-registered
 *     as the callback.
 */
void /*? me.to_instance.name ?*/_ready_callback(void)
{
    if (!gdb_enabled())
    {
        return;
    }

    /* Variables for storing the previous message information */
    unsigned int len = 0;
    char * tst = (char *)/*? me.to_instance.name ?*/_tcp_recv_buf;

    /* Set a flag saying ethernet communications are up. Only used to delay 1st packet sending
       since we don't know the IP Address or Port that GDB will use */
    ethernet_func = 1;

    /*? me.to_instance.name ?*/_tcp_poll(&len);

    /* If there is an Ack */
    if (tst[0] == '+' || tst[0] == '-')
    {
        if(len > 1)
        {
            printf("Warning: Ack (+/-) received with packet length > 1\n");
        }
        gdb_ack.recv = 1;
        gdb_ack.ack = tst[0];
    }
    else
    {
        seL4_Yield();
        if (len > BUFMAX)
        {
            len = BUFMAX;
        }

        memcpy((char *)rx_packet.buf, tst, len);
        rx_packet.len = len;
        rx_packet.ready = 1;
    }
}

int enet_getPacket(char * ret_buf)
{
    if (!gdb_enabled())
    {
        return 0;
    }

    while(rx_packet.ready == 0);

    memcpy(ret_buf, (char *)rx_packet.buf, rx_packet.len);

    rx_packet.ready = 0;
    return rx_packet.len;
}

/*
 * Function call to get Ack ('+' or '-') from GDB Host.
 *
 * The callback function stores information into global array if its an Ack packet.
 * This function acts as a wrapper to get that info and clear the ack received flag.
 */
static char enet_getAck(void)
{
    if (!gdb_enabled())
    {
        return 0;
    }

    if(gdb_ack.recv != 0)
    {
        gdb_ack.recv = 0;
        return gdb_ack.ack;
    }
    else
    {
        return 0;
    }
}

void enet_putPacket(char * send_buf, int len)
{
    if (!gdb_enabled())
    {
        return;
    }
    /*? me.to_instance.name ?*/_tcp_send((uintptr_t)send_buf, len);
}


/*
 * Get a GDB Packet, discard the '$' and '#', calculate and compare the checksums, then
 * respond to the GDB Host with a '+' or '-', and return the address of the decoded buffer.
 */
char* getpacket (void)
{
    /* Buffer to store decoded GDB Packet into */
    char *buffer = remcomInBuffer;

    /* Temporary Buffer (and Pointer to Buffer) to get GDB Packet */
    char tmp_buffer[BUFMAX] = {0};
    char *read_buf = tmp_buffer;

    u8 checksum, xmitcsum;

    int count, gdb_buffer_size = 0;
    char ch;

    while (1)
    {
        /* wait around for a packet */
        while (tmp_buffer[0] != '$')
        {
            gdb_buffer_size = enet_getPacket(tmp_buffer);
            if (tmp_buffer[0] != '$')
            {
                printf("Warning. Received GDB Packet without leading '$'\n");
            }
        }

        /* Increment buffer position since we know 1st position is '$' */
        read_buf++;

    retry:
        checksum = 0;
        xmitcsum = -1;
        count = 0;

        /* Loop through buffer. Calculate Checksum. Store Packet */
        while (count < gdb_buffer_size)
        {
            ch = read_buf[count];

            if ('$' == ch)
            {
                goto retry;
            }
            if ('#' == ch)
            {
                break;
            }

            checksum += ch;
            buffer[count++] = ch;
        }
        buffer[count] = 0;

        /* End of GDB Packet */
        if ('#' == ch)
        {
            ch = read_buf[++count];
            xmitcsum = hex(ch) << 4;
            ch = read_buf[++count];
            xmitcsum += hex(ch);

            /* failed checksum */
            if (checksum != xmitcsum)
            {
                enet_putPacket((char *)"-", 1);
            }
            /* Successful Transfer */
            else
            {
                enet_putPacket((char *)"+", 1);

                /* if a sequence char is present, reply the sequence ID */
                if (':' == buffer[2])
                {
                    enet_putPacket(buffer, 2);

                    dbg_printf("<<$%s#\n",buffer+3);
                    return &buffer[3];
                }

                dbg_printf("<<$%s#\n",buffer);
                return &buffer[0];
            }
        }
    }
    return NULL;
}

/*
 * Input a buffer to store format into GDB standard ( $ <buffer> # <checksum> ) and send over
 * ethernet to the GDB host.
 */
void putpacket (char *buffer)
{
    u8 checksum;
    int count;
    char ch = 0;

    int packet_length = strlen(buffer);

    /* Buffer for sending packet over ethernet */
    char buf[packet_length + 5];

    /* We don't know what Port GDB sends packets over, so we need to wait for a signal */
    while(!ethernet_func);

    do
    {
        /*  $<packet info>#<checksum>. */
        checksum = 0;
        count = 0;

        /* Loop through passed in packet */
        do
        {
            ch = buffer[count++];
            checksum += ch;
        } while ((0 != ch) && count < packet_length);

        /* We need to add '$' '#' the Checksum & NULL terminator */
        count = snprintf(buf, sizeof(buf), "$%s#%c%c", buffer, hexchars[checksum >> 4], hexchars[checksum & 0xF]);

        enet_putPacket(buf, count);

        /* Wait for an Ack Response... (+/-) */
        do
        {
            ch = enet_getAck();
        } while(0 == ch);

    }   while(ch != '+');

    dbg_printf(">>%s\n",buf);
}
