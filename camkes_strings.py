
debug_camkes_common = \
"""
connector seL4Debug {
  from  Procedure user_inf template \"seL4Debug-from.template.c\";
  to Procedure user_inf template \"seL4Debug-to.template.c\";
}

connector seL4GDB {
  from  Procedure user_inf template \"seL4GDB-from.template.c\";
  to Procedure user_inf template \"seL4GDB-to.template.c\";
}

procedure %s_debug {
  void debug(in int num);
} 

component Serial {
  hardware;\n  emits IRQ4 irq;
  provides IOPort serial;
}

component EthDriver {
  hardware;
  emits IRQ16 irq;
  dataport EthDriverMMIO_t mmio;
}

component debug_server {
  include "EthType.h";
  uses IOPort serial_port;
  consumes IRQ4 serial_irq;
  //dataport EthDriverMMIO_t mmio;
  //consumes IRQ16 eth_irq;
"""

debug_camkes_component_connection = \
"""
  //{0} debug
  uses {1}_debug {0}_internal;
  provides {1}_debug {0}_debug;
"""

debug_server_decls = \
"""
    component Serial hw_serial;
    component EthDriver hw_eth;
    connection seL4HardwareIOPort debug_port(from debug.serial_port, to hw_serial.serial);
    connection seL4HardwareInterrupt interrupt1(from hw_serial.irq, to debug.serial_irq);
    //connection seL4HardwareMMIO ethdrivermmio1(from debug.mmio, to hw_eth.mmio);
    //connection seL4HardwareInterrupt interrupt2(from hw_eth.irq, to debug.eth_irq);
"""

debug_server_config = \
"""
  configuration {
    hw_serial.serial_attributes = "0x3f8:0x3ff";
    hw_serial.irq_attributes = 4;
    //hw_eth.mmio_attributes = "0xf1b80000:0x80000";
    //hw_eth.irq_attributes = 16;
  }
"""