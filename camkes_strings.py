import config

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

procedure CAmkES_Debug {
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
  dataport EthDriverMMIO_t mmio;
  consumes IRQ16 eth_irq;
"""

debug_camkes_component_connection = \
"""
  //{0} debug
  uses CAmkES_Debug {0}_GDB_delegate;
  provides CAmkES_Debug {0}_fault;
"""

debug_server_component_connection = \
"""
  uses CAmkES_Debug fault;
  provides CAmkES_Debug GDB_delegate;
"""

debug_server_io_connections = \
"""
    component Serial hw_serial;
    component EthDriver hw_eth;
    connection seL4HardwareIOPort debug_port(from debug.serial_port, to hw_serial.serial);
    connection seL4HardwareInterrupt interrupt1(from hw_serial.irq, to debug.serial_irq);
    connection seL4HardwareMMIO ethdrivermmio1(from debug.mmio, to hw_eth.mmio);
    connection seL4HardwareInterrupt interrupt2(from hw_eth.irq, to debug.eth_irq);
"""

debug_server_config = \
"""
    hw_serial.serial_attributes = "0x3f8:0x3ff";
    hw_serial.irq_attributes = 4;
    hw_eth.mmio_attributes = "%s:%s";
    hw_eth.irq_attributes = 16;
""" % (config.eth_mmio_addr, config.eth_mmio_size)

debug_server_new_config = \
"""
  configuration {
    %s
  }
""" % debug_server_config

debug_server_decl = \
"\n    component debug_server debug;\n"

debug_import = \
"import \"debug/debug.camkes\";\n"

GDB_conn = \
"    connection seL4GDB debug%s(from %s.fault, to debug.%s_fault);\n"

Debug_conn = \
"    connection seL4Debug debug%s_delegate(from debug.%s_GDB_delegate, to %s.GDB_delegate);\n"