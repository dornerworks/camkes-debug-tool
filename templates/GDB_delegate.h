#define FIRST_BYTE_MASK     0xFF000000
#define LAST_BYTE_MASK		0xFF
#define FIRST_BYTE_BITSHIFT 24
#define BYTE_SHIFT       	8
#define BYTE_SIZE			8
#define BYTES_IN_WORD       (sizeof(seL4_Word) / sizeof(char))
#define HEX_STRING			16
#define DEC_STRING			10
#define CHAR_HEX_SIZE		2

#define GDB_READ_MEM 			0
#define GDB_WRITE_MEM 			1
#define GDB_READ_REG			2

#define DELEGATE_COMMAND_REG	0
#define DELEGATE_ARG(X)			(X + 1)

#define DELEGATE_WRITE_NUM_ARGS	        3
#define DELEGATE_READ_NUM_ARGS	        3
#define DELEGATE_REG_READ_NUM_ARGS      2

#define CEIL_MR(X)				(X + BYTES_IN_WORD - 1) / BYTES_IN_WORD

static void read_memory(void);
static void write_memory(void);
static void read_registers(void);