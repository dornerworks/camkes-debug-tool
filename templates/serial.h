// Configuration
#define BAUD_RATE 115200

// Register layout. Done by offset from base port
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

static int fifo_depth = 1;
static int fifo_used = 0;
static int stream_read = true;
// Serial buffer manipulation
static void initialise_buffer(void);
static int push_buffer(int c);
static void clear_buffer(void);

// Register manipulation functions
static inline void write_latch(uint16_t val);
static inline void write_latch_high(uint8_t val);
static inline void write_latch_low(uint8_t val);
static inline void write_ier(uint8_t val);
static inline uint8_t read_ier(void);
static inline void write_lcr(uint8_t val);
static inline uint8_t read_lcr(void);
static inline void write_fcr(uint8_t val);
static inline void write_mcr(uint8_t val);
static inline uint8_t read_lsr(void);
static inline uint8_t read_rbr(void);
static inline void write_thr(uint8_t val);
static inline uint8_t read_iir(void);
static inline uint8_t read_msr(void);

// Serial config
static void serial_init(void);
static void set_dlab(int v);
static void disable_interrupt(void);
static void disable_fifo(void);
static void set_baud_rate(uint32_t baud);
static void reset_state(void);
static void enable_fifo(void);
static void enable_interrupt(void);
static void reset_lcr(void);
static void reset_mcr(void);
static void wait_for_fifo(void);
static void clear_iir(void);

// Serial usage
static void serial_putchar(int c);
static void handle_char(void);
static void serial_irq_rcv(void* cookie);