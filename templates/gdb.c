#include <endian.h>

static int handle_gdb(void) {
    // Acknowledge packet
    gdb_printf(GDB_RESPONSE_START GDB_ACK GDB_RESPONSE_END "\n");
    // Get command and checksum
    int command_length = buf.checksum_index-1;
    char *command_ptr = &buf.data[COMMAND_START];
    char command[GETCHAR_BUFSIZ + 1] = {0};
    strncpy(command, command_ptr, command_length);
    char *checksum = &buf.data[buf.checksum_index + 1];
    // Calculate checksum of data
    if (DEBUG_PRINT) printf("command: %s\n", command);
    unsigned char computed_checksum = compute_checksum(command, command_length);
    unsigned char received_checksum = (unsigned char) strtol(checksum, NULL, HEX_STRING);
    if (computed_checksum != received_checksum) {
        if (DEBUG_PRINT) printf("Checksum error, computed %x, received %x received_checksum\n",
               computed_checksum, received_checksum);
    }
    // Parse the command
    handle_command(command);
    return 0;
}

static unsigned char compute_checksum(char *data, int length) {
    unsigned char checksum = 0;
    for (int i = 0; i < length; i++) {
        checksum += (unsigned char) data[i];
    }
    return checksum;
}

static void handle_breakpoint(void) {
    unsigned char inst_data[BREAKPOINT_INSTRUCTION_SIZE];
    /*? me.from_instance.name ?*/_read_memory(reg_pc, BREAKPOINT_INSTRUCTION_SIZE,
                                              (unsigned char *) &inst_data);
    if (inst_data[BREAKPOINT_INSTRUCTION_INDEX] == BREAKPOINT_INSTRUCTION) {
        curr_breakpoint = inst_data[BREAKPOINT_NUM_INDEX];
        if (DEBUG_PRINT) printf("Breakpoint %d\n", curr_breakpoint);
        restore_breakpoint_data(curr_breakpoint);
    }
}

static void breakpoint_init(void) {
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        breakpoints[i].saved_data[0] = i + 1;
    }
    breakpoints[MAX_BREAKPOINTS - 1].saved_data[0] = 0;
}

static unsigned char save_breakpoint_data(seL4_Word addr) {
    // Read data we will write over
    unsigned char data[2];
    /*? me.from_instance.name ?*/_read_memory(addr, BREAKPOINT_INSTRUCTION_SIZE , data);
    unsigned char next_breakpoint = free_breakpoint_head;
    free_breakpoint_head = breakpoints[next_breakpoint].saved_data[0];
    memcpy(breakpoints[next_breakpoint].saved_data, data, BREAKPOINT_INSTRUCTION_SIZE);
    return next_breakpoint;
}

static void restore_breakpoint_data(unsigned char breakpoint_num) {
    unsigned char* inst_data = breakpoints[breakpoint_num].saved_data;
    /*? me.from_instance.name ?*/_write_memory(reg_pc, BREAKPOINT_INSTRUCTION_SIZE, inst_data);
    for (int i = 0; i < BREAKPOINT_INSTRUCTION_SIZE; i++) {
        breakpoints[breakpoint_num].saved_data[i] =  0;
    }
    breakpoints[free_breakpoint_tail].saved_data[0] = breakpoint_num;
}


// GDB read memory format:
// m[addr],[length]
static void GDB_read_memory(char *command) {
    // Get address to read from command
    char *addr_string = strtok(command, "m,");
    // Get num of bytes to read from command
    char *length_string = strtok(NULL, ",");
    // Convert strings to values
    seL4_Word addr = (seL4_Word) strtol(addr_string, NULL, HEX_STRING);
    seL4_Word length = (seL4_Word) strtol(length_string, NULL, DEC_STRING);
    // Buffer for raw data
    unsigned char data[length];
    // Buffer for data formatted as hex string
    int buf_len = CHAR_HEX_SIZE * length + 1;
    char data_string[buf_len];
    if (DEBUG_PRINT) printf("length: %d\n", length);
    // Do a read call to the GDB delegate who will read from the process on our behalf
    /*? me.from_instance.name ?*/_read_memory(addr, length, data);
    // Format the data
    for (int i = 0; i < length; i++) {
      sprintf(&data_string[CHAR_HEX_SIZE * i], "%02x", data[i]);
    }
    unsigned char checksum = compute_checksum(data_string, buf_len);
    // Print hex stream of read data
    gdb_printf(GDB_RESPONSE_START);
    gdb_printf("$%.*s#%02X", buf_len, data_string, checksum);
    /*for (int i = 0; i < CHAR_HEX_SIZE * length; i += 2) {
        gdb_printf("%02x", data[i]);
    }*/
    gdb_printf(GDB_RESPONSE_END);
    gdb_printf("\n");
}

// GDB write memory format:
// M[addr],[length]:[data]
static void GDB_write_memory(char* command) {
    // Get address to write to from command
    char *addr_string = strtok(command, "M,");
    // Get num of bytes to write from command
    char *length_string = strtok(NULL, ",:");
    // Get data from command
    char *data_string = strtok(NULL, ":");
     // Convert strings to values
    seL4_Word addr = (seL4_Word) strtol(addr_string, NULL, HEX_STRING);
    seL4_Word length = (seL4_Word) strtol(length_string, NULL, DEC_STRING);
    // Buffer for data to be written
    unsigned char data[length];
    memset(data, 0, length);
    // Parse data to be written as raw hex
    for (int i = 0; i < length; i++) {
        sscanf(data_string, "%2hhx", &data[i]);
        data_string += CHAR_HEX_SIZE;
    }
    // Do a write call to the GDB delegate who will write to the process on our behalf
    seL4_Word error = /*? me.from_instance.name ?*/_write_memory(addr, length, data);
    if (!error) {
        gdb_printf(GDB_RESPONSE_START GDB_OK GDB_RESPONSE_END "\n");
    }
}

static void GDB_query(char *command) {
    char *query_type = strtok(command, "q:#");
    if (strcmp("Supported", query_type) == 0) {// Setup argument storage
        // TODO Parse arguments and respond what the stub supports
        gdb_printf(GDB_RESPONSE_START "$PacketSize=100#c1" GDB_RESPONSE_END "\n");
    } else if (strcmp("TStatus", query_type) == 0) {
        gdb_printf(GDB_RESPONSE_START "$#00" GDB_RESPONSE_END "\n");
    } else if (strcmp("TfV", query_type) == 0) {
        gdb_printf(GDB_RESPONSE_START "$#00" GDB_RESPONSE_END "\n");
    } else if (strcmp("C", query_type) == 0) {
        gdb_printf(GDB_RESPONSE_START "$QC1#c5" GDB_RESPONSE_END "\n");
    } else if (strcmp("Attached", query_type) == 0) {
        gdb_printf(GDB_RESPONSE_START "$#00" GDB_RESPONSE_END "\n");
    } else if (strcmp("fThreadInfo", query_type) == 0) {
        gdb_printf(GDB_RESPONSE_START "$m01#ce" GDB_RESPONSE_END "\n");
    } else if (strcmp("sThreadInfo", query_type) == 0) {
        gdb_printf(GDB_RESPONSE_START "$l#6c" GDB_RESPONSE_END "\n");
    } else if (strcmp("Symbol", query_type) == 0) {
        gdb_printf(GDB_RESPONSE_START "$OK#9a" GDB_RESPONSE_END "\n");
    } else {
        printf("Unrecognised query command\n");
    }
}

static void GDB_insert_sw_breakpoint(char* command) {
    char *addr_string = strtok(command + 2, ",#");
    seL4_Word addr = (seL4_Word) strtol(addr_string, NULL, 16);
    unsigned char breakpoint_index = save_breakpoint_data(addr);
    unsigned char breakpoint[2] = {0xCC, breakpoint_index};
    seL4_Word error = /*? me.from_instance.name ?*/_write_memory(addr, 2, (unsigned char *)&breakpoint);
    if (!error) {
        gdb_printf(GDB_RESPONSE_START GDB_OK GDB_RESPONSE_END "\n");
    }
}

static void GDB_set_thread(char *command) {
    gdb_printf(GDB_RESPONSE_START "$OK#9a" GDB_RESPONSE_END "\n");
}   

static void GDB_halt_reason(char *command) {
    gdb_printf(GDB_RESPONSE_START "$T05thread:01;#07" GDB_RESPONSE_END "\n");
}

static void GDB_read_general_registers(char* command) {
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    seL4_Word registers[num_regs];
    /*? me.from_instance.name ?*/_read_registers(badge, registers);
    int buf_len = num_regs * sizeof(int) * CHAR_HEX_SIZE + 1;
    char data[buf_len];
    for (int i = 0; i < num_regs; i++) {
        sprintf(data + sizeof(seL4_Word) * CHAR_HEX_SIZE * i, 
                "%08x", __builtin_bswap32(registers[i]));
    }
    unsigned char checksum = compute_checksum(data, buf_len);
    gdb_printf("$%.*s#%02x\n", buf_len, data, checksum);
}

static void GDB_read_register(char* command) {
    seL4_Word reg;
    // Get which register we want to read
    char *register_string = strtok(command + 1, "#");
    if (register_string == NULL) {
        char error[] = "E00";
        unsigned char checksum = compute_checksum(error, strlen(error));
        gdb_printf("$%s#%02x\n", error, checksum);
        return;
    }
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    seL4_Word reg_num = strtol(register_string, NULL, 16);
    if (reg_num >= num_regs) {
        char error[] = "E00";
        unsigned char checksum = compute_checksum(error, strlen(error));
        gdb_printf("$%s#%02x\n", error, checksum);
        return;
    }
    /*? me.from_instance.name ?*/_read_register(badge, &reg, reg_num);
    int buf_len = sizeof(seL4_Word) * CHAR_HEX_SIZE + 1;
    char data[buf_len];
    sprintf(data, "%02x", __builtin_bswap32(reg));
    unsigned char checksum = compute_checksum(data, buf_len);
    gdb_printf("$%2s#%02x\n", data, checksum);
}

static void GDB_write_general_registers(char *command) {
    char *data_string = strtok(command + 1, "#");
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    int num_regs_data = (strlen(data_string)) / (sizeof(int) * 2);
    if (DEBUG_PRINT) printf("num_regs: %d, data regs: %d, len: %d\n", num_regs, num_regs_data, strlen(data_string));
    if (num_regs_data > num_regs) {
        num_regs_data = num_regs;
    }
    seL4_Word data[num_regs_data];
    char buf[50];
    printf("Data string: %s\n", data_string);
    for (int i = 0; i < num_regs_data; i++) {
        memset(buf, 0, 50);
        strncpy(buf, data_string, sizeof(int) * 2);
        printf("Test\n");
        data_string += sizeof(int) * 2;
        printf("Test1 %p\n", buf);
        data[i] = (seL4_Word) strtol((char *) buf, NULL, 16);
        printf("Test2\n");
        if (DEBUG_PRINT) printf("Reg%d: %x\n", i, data[i]);
    }
    /*? me.from_instance.name ?*/_write_registers(badge, data, num_regs_data);
}