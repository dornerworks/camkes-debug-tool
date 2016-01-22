static void eth_init(void) {
	uint16_t *data = mmio;
	for (int i = 0; i < 512; i++) {
		printf("%X: %2X\n", data + 4*i, data[2*i]);
	}
}

static void eth_irq_rcv(void* cookie) {
	//printf("MMIO: %s\n", mmio);
    //eth_irq_reg_callback(serial_irq_rcv, cookie);
}