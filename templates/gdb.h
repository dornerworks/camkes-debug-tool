typedef struct gdb_buffer {
    uint32_t length;
    uint32_t checksum_count;
    uint32_t checksum_index;
    char data[GETCHAR_BUFSIZ];
} gdb_buffer_t;

gdb_buffer_t buf;

static int handle_gdb(void);
static char* GDB_read_memory(char* command);
static char* GDB_write_memory(char* command);