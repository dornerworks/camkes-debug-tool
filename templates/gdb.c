static int handle_gdb(void) {
    // Print command
    printf("Buffer: %.*s\n",buf.length, buf.data);
    int command_length = buf.checksum_index-1;
    char* command_ptr = &buf.data[1];
    char command[GETCHAR_BUFSIZ + 1] = {0};
    strncpy(command, command_ptr, command_length);
    printf("Length: %d\n", command_length);
    printf("Command string: %s\n", command);
    // Print checksum
    char* checksum = &buf.data[buf.checksum_index + 1];
    int checksum_length = buf.length - buf.checksum_index - 1;
    printf("Checksum: %.*s\n", checksum_length, checksum);
    handle_command(command);
    return 0;
}

static char* GDB_read_memory(char* command) {
    char* addr_string = strtok(command, "m,");
    char* length_string = strtok(NULL, ",");
    printf("length_string: %s\n", length_string);
    seL4_Word addr = (seL4_Word) strtol(addr_string, NULL, 16);
    seL4_Word length = (seL4_Word) strtol(length_string, NULL, 10);
    uint8_t data_string[2*length + 1];
    /*? me.from_instance.name ?*/_read_memory(addr, length, data_string);
    printf("Read length %d\n", length);
    for (int i = 0; i < 2*length; i += 2) {
        printf("%.*s ", 2, &data_string[i]);
        if((i+2) % 8 == 0) {
            printf("\n");
        }
    }
    if((length + 1) % 4 != 0) {
        printf("\n");
    }
    // TODO Send the GDB response packet over ethernet
    return 0;
}

static char* GDB_write_memory(char* command) {
    char* addr_string = strtok(command, "M,");
    char* length_string = strtok(NULL, ",:");
    char* data_string = strtok(NULL, ":");
    printf("Data string: %s\n", data_string);
    seL4_Word addr = (seL4_Word) strtol(addr_string, NULL, 16);
    seL4_Word length = (seL4_Word) strtol(length_string, NULL, 10);
    printf("Read length %d\n", length);
    size_t data_length = strlen(data_string);
    if (data_length < length) {
        length = data_length;
    }
    char data[length];
    for (int i = 0; i < length; i++) {
        sscanf(data_string, "%2hhx", &data[i]);
        data_string += 2;
    }
    int error = /*? me.from_instance.name ?*/_write_memory(addr, length, data);
    if (!error) {
        printf("Write successful\n");
    }
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