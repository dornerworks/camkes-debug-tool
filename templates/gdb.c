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

static void send_message(char *message, int len) {
    if (len == 0) {
        len = strlen(message);
    }
    unsigned char checksum = compute_checksum(message, len);
    gdb_printf(GDB_RESPONSE_START "$%s#%02X", message, checksum);
    gdb_printf(GDB_RESPONSE_END);
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

    send_message(data_string, buf_len);
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
        send_message("OK", 0);
    }
}

static void GDB_query(char *command) {
    char *query_type = strtok(command, "q:#");
    if (strcmp("Supported", query_type) == 0) {// Setup argument storage
        // TODO Parse arguments and respond what the stub supports
        send_message("PacketSize=100", 0);
    } else if (strcmp("TStatus", query_type) == 0) {
        send_message("", 0);
    } else if (strcmp("TfV", query_type) == 0) {
        send_message("", 0);
    } else if (strcmp("C", query_type) == 0) {
        send_message("QC1", 0);
    } else if (strcmp("Attached", query_type) == 0) {
        send_message("", 0);
    } else if (strcmp("fThreadInfo", query_type) == 0) {
        send_message("m01", 0);
    } else if (strcmp("sThreadInfo", query_type) == 0) {
        send_message("l", 0);
    } else if (strcmp("Symbol", query_type) == 0) {
        send_message("", 0);
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
        send_message("OK", 0);
    }
}

static void GDB_set_thread(char *command) {
    send_message("OK", 0);
}   

static void GDB_halt_reason(char *command) {
    send_message("T05thread:01;", 0);
}

static void GDB_read_general_registers(char* command) {
    seL4_Word registers[x86_MAX_REGISTERS] = {0};
    /*? me.from_instance.name ?*/_read_registers(badge, registers);
    int buf_len = x86_MAX_REGISTERS * sizeof(int) * CHAR_HEX_SIZE + 1;
    char data[buf_len];
    memset(data, 0, buf_len);
    for (int i = 0; i < x86_MAX_REGISTERS; i++) {
        sprintf(data + sizeof(seL4_Word) * CHAR_HEX_SIZE * i, 
                "%08x", __builtin_bswap32(registers[i]));
    }
    send_message(data, buf_len);
}

static void GDB_read_register(char* command) {
    seL4_Word reg;
    // Get which register we want to read
    char *register_string = strtok(command + 1, "#");
    if (register_string == NULL) {
        send_message("E00", 0);
        return;
    }
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    seL4_Word reg_num = strtol(register_string, NULL, 16);
    if (reg_num >= x86_INVALID_REGISTER) {
        send_message("E00", 0);
        return;
    }
    // Convert to the register order we have
    seL4_Word gdb_reg_num = x86_GDB_Register_Map[reg_num];
    /*? me.from_instance.name ?*/_read_register(badge, &reg, gdb_reg_num);
    int buf_len = sizeof(seL4_Word) * CHAR_HEX_SIZE + 1;
    char data[buf_len];
    sprintf(data, "%02x", __builtin_bswap32(reg));
    send_message(data, buf_len);
}

static void GDB_write_general_registers(char *command) {
    char *data_string = strtok(command + 1, "#");
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    int num_regs_data = (strlen(data_string)) / (sizeof(int) * 2);
    if (num_regs_data > num_regs) {
        num_regs_data = num_regs;
    }
    seL4_Word data[num_regs_data];
    char buf[50];
    for (int i = 0; i < num_regs_data; i++) {
        memset(buf, 0, 50);
        strncpy(buf, data_string, sizeof(int) * 2);
        data_string += sizeof(int) * 2;
        data[i] = (seL4_Word) strtol((char *) buf, NULL, 16);
    }
    /*? me.from_instance.name ?*/_write_registers(badge, data, num_regs_data);
}