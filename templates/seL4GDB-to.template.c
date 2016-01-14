/*#
 *# Copyright 2014, NICTA
 *#
 *# This software may be distributed and modified according to the terms of
 *# the BSD 2-Clause license. Note that NO WARRANTY is provided.
 *# See "LICENSE_BSD2.txt" for details.
 *#
 *# @TAG(NICTA_BSD)
 #*/

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <camkes/marshal.h>
#include <camkes/error.h>
#include <camkes/tls.h>
#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <camkes/dataport.h>
#include <utils/util.h>

// configuration
#define BAUD_RATE 115200

// register layout. Done by offset from base port
#define THR_ADDR (0)
#define RBR_ADDR (0)
#define LATCH_LOW_ADDR (0)
#define LATCH_HIGH_ADDR (1)
#define IER_ADDR (1)
#define FCR_ADDR (2)
#define IIR_ADDR (2)
#define LCR_ADDR (3)
#define MCR_ADDR (4)
#define LSR_ADDR (5)
#define MSR_ADDR (6)

#define IER_RESERVED_MASK (BIT(6) | BIT(7))

#define FCR_ENABLE BIT(0)
#define FCR_CLEAR_RECEIVE BIT(1)
#define FCR_CLEAR_TRANSMIT BIT(2)
#define FCR_TRIGGER_16_1 (0)

#define LCR_DLAB BIT(7)

#define MCR_DTR BIT(0)
#define MCR_RTS BIT(1)
#define MCR_AO1 BIT(2)
#define MCR_AO2 BIT(3)

#define LSR_EMPTY_DHR BIT(6)
#define LSR_EMPTY_THR BIT(5)
#define LSR_DATA_READY BIT(0)

#define IIR_FIFO_ENABLED (BIT(6) | BIT(7))
#define IIR_REASON (BIT(1) | BIT(2) | BIT(3))
#define IIR_MSR (0)
#define IIR_THR BIT(1)
#define IIR_RDA BIT(2)
#define IIR_TIME (BIT(3) | BIT(2))
#define IIR_LSR (BIT(2) | BIT(1))
#define IIR_PENDING BIT(0)

#define GETCHAR_BUFSIZ 512

/*? macros.show_includes(me.to_instance.type.includes) ?*/
/*? macros.show_includes(me.to_interface.type.includes, '../static/components/' + me.to_instance.type.name + '/') ?*/


/*- set methods_len = len(me.to_interface.type.methods) -*/
/*- set instance = me.to_instance.name -*/
/*- set interface = me.to_interface.name -*/
/*- set size = 'seL4_MsgMaxLength * sizeof(seL4_Word)' -*/
/*- set allow_trailing_data = False -*/
/*- set ep = alloc(me.from_instance.name + "_ep_fault", seL4_EndpointObject, read=True, write=True, grant=True) -*/
/*- set cnode = alloc_cap('cnode', my_cnode, write=True) -*/
/*- set reply_cap_slot = alloc_cap('reply_cap_slot', None) -*/
/*- set info = c_symbol('info') -*/

typedef struct gdb_buffer {
    uint32_t length;
    uint32_t checksum_count;
    uint32_t checksum_index;
    char data[GETCHAR_BUFSIZ];
} gdb_buffer_t;

gdb_buffer_t buf;

static void send_reply(void);
static int handle_command(char* command);

static int fifo_depth = 1;
static int fifo_used = 0;

/* Register access functions */

static inline void write_ier(uint8_t val) {
    serial_port_out8_offset(IER_ADDR, val);
}
static inline uint8_t read_ier() {
    return serial_port_in8_offset(IER_ADDR);
}

static inline void write_lcr(uint8_t val) {
    serial_port_out8_offset(LCR_ADDR, val);
}
static inline uint8_t read_lcr() {
    return serial_port_in8_offset(LCR_ADDR);
}

static inline void write_fcr(uint8_t val) {
    serial_port_out8_offset(FCR_ADDR, val);
}
// you cannot read the fcr

static inline void write_mcr(uint8_t val) {
    serial_port_out8_offset(MCR_ADDR, val);
}

static inline uint8_t read_lsr() {
    return serial_port_in8_offset(LSR_ADDR);
}

static inline uint8_t read_rbr() {
    return serial_port_in8_offset(RBR_ADDR);
}

static inline void write_thr(uint8_t val) {
    serial_port_out8_offset(THR_ADDR, val);
}

static inline uint8_t read_iir() {
    return serial_port_in8_offset(IIR_ADDR);
}

static inline uint8_t read_msr() {
    return serial_port_in8_offset(MSR_ADDR);
}

static void reset_lcr(void) {
    // set 8-n-1
    write_lcr(3);
}

static void reset_mcr(void) {
    write_mcr(MCR_DTR | MCR_RTS | MCR_AO1 | MCR_AO2);
}

/* GDB functions */
static char* GDB_read_memory(char* command) {
    char* addr_string = strtok(command, "m,");
    char* length_string = strtok(NULL, "m,");
    seL4_Word addr = (seL4_Word) strtol(addr_string, NULL, 16);
    seL4_Word length = (seL4_Word) strtol(length_string, NULL, 10);
    /*? me.from_instance.name ?*/_read_memory(addr, length);
    // TODO Send the GDB response packet over ethernet
    return 0;
}

/*static char* GDB_insert_point(char* command) {
    char* type = strtok(command, "Z,");
    if (type == "0") {
        printf("Inserting memory breakpoint\n");
        GDB_insert_memory_breeakpoint(command)
    } else {
        printf("Point type not implemented\n");
    }
}*/

/*static char* GDB_insert_memory_breakpoint(char* command) {
    char* addr_string = strtok(NULL, "Z,");
    // Ignored
    //char* kind = strtok(NULL, "Z,");
    seL4_Word addr = (seL4_Word) strtol(addr_string, NULL, 16);
    /// *? me.from_instance.name ?* /_set_memory(addr, length, data);
}*/

/* Char buffer functions */
static void initialise_buffer(void) {
    buf.length = 0;
    buf.checksum_count = 0;
    buf.checksum_index = 0;
}

static int push_buffer(int c) {
    if (buf.length == GETCHAR_BUFSIZ) {
        return -1;
    } else {
        buf.data[buf.length] = c;
        buf.length++;
        return 0;
    }
}

static void clear_buffer(void) {
    int i;
    for (i = 0; i < buf.length; i++) {
        buf.data[i] = 0;
    }
    initialise_buffer();
}

static int handle_gdb(void) {
    // Print command
    int command_length = buf.checksum_index-1;
    char* command_ptr = &buf.data[1];
    char command[GETCHAR_BUFSIZ + 1];
    strncpy(command, command_ptr, command_length);
    printf("Command string: %s\n", command);
    // Print checksum
    char* checksum = &buf.data[buf.checksum_index + 1];
    int checksum_length = buf.length - buf.checksum_index - 1;
    printf("Checksum: %.*s\n", checksum_length, checksum);
    handle_command(command);
    return 0;
}

static void handle_char(void) {
    char c = read_rbr();
    if (c == '$') {
        clear_buffer();

    } else if (buf.checksum_index) {
        buf.checksum_count++;
    } else if (c == '#') {
        buf.checksum_index = buf.length;
    }
    push_buffer(c);
    if (buf.checksum_count == 2) {
        handle_gdb();
        clear_buffer();
    }
}

static void clear_iir(void) {
    uint8_t iir;
    while (! ((iir = read_iir()) & IIR_PENDING)) {
        switch(iir & IIR_REASON) {
        case IIR_RDA:
        case IIR_TIME:
            while (read_lsr() & LSR_DATA_READY) {
                handle_char();            
            }
        default:
            break;
        }
    }
}

static void serial_irq_rcv(void* cookie) {
    clear_iir();
    serial_irq_reg_callback(serial_irq_rcv, cookie);
}




int /*? me.to_interface.name ?*/__run(void) {
    /*# Check any typedefs we have been given are not arrays. #*/
    // Make connection to gdb
    while (1) { 
        seL4_Word badge;
        seL4_Word label;
        seL4_MessageInfo_t /*? info ?*/ = seL4_Recv(/*? ep ?*/, &badge);
        printf("Fault received on /*? me.to_interface.name ?*/\n");
        label = seL4_MessageInfo_get_label(/*? info ?*/);
        // Start accepting GDB input
        serial_irq_reg_callback(serial_irq_rcv, 0);
        // TODO Start accepting ethernet input
        //send_reply();
    }
    UNREACHABLE();
}

static void send_reply(void) {
    seL4_Error err = seL4_CNode_SaveCaller(/*? cnode ?*/, /*? reply_cap_slot ?*/, 32);
    assert(err==0);
    seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(/*? reply_cap_slot ?*/, reply);
}

static void wait_for_fifo() {
    while(! (read_lsr() & (LSR_EMPTY_DHR | LSR_EMPTY_THR)));
    fifo_used = 0;
}

static void serial_putchar(int c) {
    // check how much fifo we've used and if we need to drain it
    if (fifo_used == fifo_depth) {
        wait_for_fifo();
    }
    write_thr((uint8_t)c);
    fifo_used++;
}

static int handle_command(char* command) {
    switch (command[0]) {
        case '!':
            // Enable extended mode
            printf("Not implemented: enable extended mode\n");
            break;
        case '?':
            // Halt reason
            printf("Not implemented: halt reason\n");
            break;
        case 'A':
            // Argv
            printf("Not implemented: argv\n");
            break;
        case 'b':
            if (command[1] == 'c') {
                // Backward continue
                printf("Not implemented: backward continue\n");
                break;
            } else if (command[1] == 's') {
                // Backward step
                printf("Not implemented: backward step\n");
                break;
            } else {
               // Set baud rate
                printf("Not implemented: Set baud rate\n"); 
                break;
            } 
        case 'c':
            // Continue
            printf("Not implemented: continue\n");
            break;
        case 'C':
            // Continue with signal
            printf("Not implemented: continue with signal\n");
            break;
        case 'd':
            printf("Not implemented: toggle debug\n");
            break;
        case 'D':
            printf("Not implemented: detach\n");
            break;
        case 'F':
            printf("Not implemented: file IO\n");
            break;
        case 'g':
            printf("Not implemented: read general registers\n");
            break;
        case 'G':
            printf("Not implemented: write general registers\n");
            break;
        case 'H':
            printf("Not implemented: set thread\n");
            break;
        case 'i':
            printf("Not implemented: cycle step\n");
            break;
        case 'I':
            printf("Not implemented: signal + cycle step\n");
            break;
        case 'k':
            printf("Not implemented: kill\n");
            break;
        case 'm':
            printf("Reading memory\n");
            GDB_read_memory(command);
            break;
        case 'M':
            printf("Not implemented: write memory\n");
            break;
        case 'p':
            printf("Not implemented: read register\n");
            break;
        case 'P':
            printf("Not implemented: write register\n");
            break;
        case 'q':
            printf("Not implemented: query\n");
            break;
        case 'Q':
            printf("Not implemented: set\n");
            break;
        case 'r':
            printf("Not implemented: reset\n");
            break;
        case 'R':
            printf("Not implemented: restart\n");
            break;
        case 's':
            printf("Not implemented: step\n");
            break;
        case 'S':
            printf("Not implemented: step + signal\n");
            break;
        case 't':
            printf("Not implemented: search\n");
            break;
        case 'T':
            printf("Not implemented: check thread\n");
            break;
        case 'v':
            printf("Not implemented: v commands\n");
            break;
        case 'X':
            printf("Not implemented: write binary\n");
            break;
        case 'z':
            printf("Not implemented: remove point\n");
            break;
        case 'Z':
            printf("Inserting point\n");
            //GDB_insert_point(command);
            break;
        default:
            printf("Unknown command\n");
    }
    
    return 0;
}







// assume DLAB == 1
static inline void write_latch_high(uint8_t val) {
    serial_port_out8_offset(LATCH_HIGH_ADDR, val);
}
static inline void write_latch_low(uint8_t val) {
    serial_port_out8_offset(LATCH_LOW_ADDR, val);
}

static void set_dlab(int v) {
    if (v) {
        write_lcr(read_lcr() | LCR_DLAB);
    } else {
        write_lcr(read_lcr() & ~LCR_DLAB);
    }
}

static inline void write_latch(uint16_t val) {
    set_dlab(1);
    write_latch_high(val >> 8);
    write_latch_low(val & 0xff);
    set_dlab(0);
}

static void disable_interrupt() {
    write_ier(0);
}

static void disable_fifo() {
    // first attempt to use the clear fifo commands
    write_fcr(FCR_CLEAR_TRANSMIT | FCR_CLEAR_RECEIVE);
    // now disable with a 0
    write_fcr(0);
}

static void set_baud_rate(uint32_t baud) {
    assert(baud != 0);
    assert(115200 % baud == 0);
    uint16_t divisor = 115200 / baud;
    write_latch(divisor);
}

static void reset_state() {
    // clear internal global state here
    fifo_used = 0;
}

static void enable_fifo() {
    // check if there is a fifo and how deep it is
    uint8_t info = read_iir();
    if ((info & IIR_FIFO_ENABLED) == IIR_FIFO_ENABLED) {
        fifo_depth = 16;
        write_fcr(FCR_TRIGGER_16_1 | FCR_ENABLE);
    } else {
        fifo_depth = 1;
    }
}



static void enable_interrupt() {
    write_ier(1);
}








void /*? me.to_interface.name ?*/__init(void) {

    // Initialize the serial port
    set_dlab(0); // we always assume the dlab is 0 unless we explicitly change it
    disable_interrupt();
    disable_fifo();
    reset_lcr();
    reset_mcr();
    clear_iir();
    set_baud_rate(BAUD_RATE);
    reset_state();
    enable_fifo();
    enable_interrupt();
    clear_iir();
    initialise_buffer();
    set_putchar(serial_putchar);
}