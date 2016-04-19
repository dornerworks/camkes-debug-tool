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
#define GDB_READ_REGS			2
#define GDB_WRITE_REG			3
#define GDB_READ_REG            4

#define DELEGATE_COMMAND_REG	0
#define DELEGATE_ARG(X)			(X + 1)

#define DELEGATE_WRITE_NUM_ARGS	        3
#define DELEGATE_READ_NUM_ARGS	        3
#define DELEGATE_REGS_READ_NUM_ARGS     2
#define DELEGATE_REG_WRITE_NUM_ARGS     2 + (sizeof(seL4_UserContext) / sizeof(seL4_Word))
#define DELEGATE_REG_READ_NUM_ARGS      3

#define CEIL_MR(X)				(X + BYTES_IN_WORD - 1) / BYTES_IN_WORD