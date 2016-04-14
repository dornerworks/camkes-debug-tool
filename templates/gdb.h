#define NO_BREAKPOINT -1
#define USER_BREAKPOINT 0
#define BREAKPOINT_INSTRUCTION 0xCC
#define DEBUG_PRINT false
#define MAX_ARGS 20
#define MAX_BREAKPOINTS 256
#define BREAKPOINT_INSTRUCTION_SIZE     2   
#define BREAKPOINT_INSTRUCTION_INDEX    0
#define BREAKPOINT_NUM_INDEX            1    
#define COMMAND_START                   1
#define HEX_STRING						16
#define DEC_STRING						10
#define CHAR_HEX_SIZE					2
// Colour coding for response packets from GDB stub
#define GDB_RESPONSE_START 		  ""
#define GDB_RESPONSE_END		  ""
//#define GDB_RESPONSE_START      "\x1b[31m"
//#define GDB_RESPONSE_END        "\x1b[0m"

// Ok packet for GDB
#define GDB_ACK                 "+"
#define GDB_OK                  "$OK#9a"
#define GDB_EMPTY               "$#00"

typedef struct gdb_buffer {
    uint32_t length;
    uint32_t checksum_count;
    uint32_t checksum_index;
    char data[GETCHAR_BUFSIZ];
} gdb_buffer_t;

typedef struct breakpoint_data {
    unsigned char saved_data[BREAKPOINT_INSTRUCTION_SIZE];
} breakpoint_data_t;

gdb_buffer_t buf;



static int handle_gdb(void);
static void handle_breakpoint(void);

static unsigned char compute_checksum(char *data, int length);

static breakpoint_data_t breakpoints[MAX_BREAKPOINTS];
static unsigned char free_breakpoint_head = 1;
static unsigned char free_breakpoint_tail = 255;
static unsigned char curr_breakpoint = NO_BREAKPOINT;

static void breakpoint_init(void);
static unsigned char save_breakpoint_data(seL4_Word addr);
static void restore_breakpoint_data(unsigned char breakpoint_num);



static void GDB_read_memory(char *command);
static void GDB_write_memory(char *command);
static void GDB_query(char *command);
static void GDB_insert_sw_breakpoint(char *command);
static void GDB_set_thread(char *command);
static void GDB_halt_reason(char *command);
static void GDB_read_general_registers(char* command);

extern seL4_Word /*? me.from_instance.name ?*/_write_memory(seL4_Word addr, seL4_Word length, 
															unsigned char *data);
extern unsigned char* /*? me.from_instance.name ?*/_read_memory(seL4_Word addr, seL4_Word length, 
																unsigned char *data);
extern void /*? me.from_instance.name ?*/_read_registers(seL4_Word tcb_cap, seL4_Word registers[]);
