

static int handle_gdb(void) {
    // Acknowledge packet
    printf(GDB_RESPONSE_START GDB_ACK GDB_RESPONSE_END "\n");
    // Get command and checksum
    int command_length = buf.checksum_index-1;
    char *command_ptr = &buf.data[COMMAND_START];
    char command[GETCHAR_BUFSIZ + 1] = {0};
    strncpy(command, command_ptr, command_length);
    char *checksum = &buf.data[buf.checksum_index + 1];
    // Calculate checksum of data
    printf("command: %s\n", command);
    unsigned char computed_checksum = compute_checksum(command, command_length);
    unsigned char received_checksum = (unsigned char) strtol(checksum, NULL, HEX_STRING);
    if (computed_checksum != received_checksum) {
        printf("Checksum error, computed %x, received %x received_checksum\n",
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
        printf("Breakpoint %d\n", curr_breakpoint);
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
    unsigned char *inst_data = &(breakpoints[breakpoint_num].saved_data);
    /*? me.from_instance.name ?*/_write_memory(reg_pc, BREAKPOINT_INSTRUCTION_SIZE, inst_data);
    for (int i = 0; i < BREAKPOINT_INSTRUCTION_SIZE; i++) {
        breakpoints[breakpoint_num].saved_data[i] =  0;
    }
    breakpoints[free_breakpoint_tail].saved_data[0] = breakpoint_num;
}

// GDB read memory format:
// m[addr],[length]
static unsigned char *GDB_read_memory(char *command) {
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
    char data_string[CHAR_HEX_SIZE * length + 1];
    // Do a read call to the GDB delegate who will read from the process on our behalf
    /*? me.from_instance.name ?*/_read_memory(addr, length, data);
    // Format the data
    for (int i = 0; i < length; i++) {
      sprintf(&data_string[CHAR_HEX_SIZE * i], "%02x", data[i]);
    }
    // Print hex stream of read data
    printf(GDB_RESPONSE_START);
    for (int i = 0; i < CHAR_HEX_SIZE * length; i += 2) {
        printf("%.*s", CHAR_HEX_SIZE, &data_string[i]);
    }
    printf(GDB_RESPONSE_END);
    printf("\n");
    return 0;
}

// GDB write memory format:
// M[addr],[length]:[data]
static char *GDB_write_memory(char* command) {
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
        printf(GDB_RESPONSE_START GDB_OK GDB_RESPONSE_END "\n");
    }
    return NULL;
}

static char *GDB_query(char *command) {
    char *query_type = strtok(command, "q:#");
    if (strcmp("Supported", query_type) == 0) {// Setup argument storage
        // TODO Parse arguments and respond what the stub supports
        printf(GDB_RESPONSE_START GDB_EMPTY GDB_RESPONSE_END "\n");
    } else if (strcmp("TStatus", query_type) == 0) {
        printf(GDB_RESPONSE_START "$T0#84" GDB_RESPONSE_END "\n");
    } else {
        printf("Unrecognised query command\n");
    }
    return NULL;
}

static char *GDB_insert_sw_breakpoint(char* command) {
    char *addr_string = strtok(command + 2, ",#");
    seL4_Word addr = (seL4_Word) strtol(addr_string, NULL, 16);
    unsigned char breakpoint_index = save_breakpoint_data(addr);
    unsigned char breakpoint[2] = {0xCC, breakpoint_index};
    seL4_Word error = /*? me.from_instance.name ?*/_write_memory(addr, 2, (unsigned char *)&breakpoint);
    if (!error) {
        printf(GDB_RESPONSE_START GDB_OK GDB_RESPONSE_END "\n");
    }
    return NULL;
}