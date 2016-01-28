
// Serial buffer manipulation
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

// Register manipulation functions
static inline void write_latch(uint16_t val) {
    set_dlab(1);
    write_latch_high(val >> 8);
    write_latch_low(val & 0xff);
    set_dlab(0);
}

static inline void write_latch_high(uint8_t val) {
    serial_port_out8_offset(LATCH_HIGH_ADDR, val);
}

static inline void write_latch_low(uint8_t val) {
    serial_port_out8_offset(LATCH_LOW_ADDR, val);
}

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

// Serial config

static void serial_init(void) {
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

static void set_dlab(int v) {
    if (v) {
        write_lcr(read_lcr() | LCR_DLAB);
    } else {
        write_lcr(read_lcr() & ~LCR_DLAB);
    }
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

static void reset_state(void) {
    // clear internal global state here
    fifo_used = 0;
}

static void enable_fifo(void) {
    // check if there is a fifo and how deep it is
    uint8_t info = read_iir();
    if ((info & IIR_FIFO_ENABLED) == IIR_FIFO_ENABLED) {
        fifo_depth = 16;
        write_fcr(FCR_TRIGGER_16_1 | FCR_ENABLE);
    } else {
        fifo_depth = 1;
    }
}

static void enable_interrupt(void) {
    write_ier(1);
}

static void reset_lcr(void) {
    // set 8-n-1
    write_lcr(3);
}

static void reset_mcr(void) {
    write_mcr(MCR_DTR | MCR_RTS | MCR_AO1 | MCR_AO2);
}

static void wait_for_fifo() {
    while(! (read_lsr() & (LSR_EMPTY_DHR | LSR_EMPTY_THR)));
    fifo_used = 0;
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

// Serial usage
static void serial_putchar(int c) {
    // check how much fifo we've used and if we need to drain it
    if (fifo_used == fifo_depth) {
        wait_for_fifo();
    }
    write_thr((uint8_t)c);
    fifo_used++;
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



static void serial_irq_rcv(void *cookie) {
    clear_iir();
    if (stream_read) {
    	serial_irq_reg_callback(serial_irq_rcv, cookie);
    }  
}