#
# Copyright 2017, DornerWorks
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DORNERWORKS_BSD)
#

# Should match configurations.camkes
ENET_IP_ADDR = "172.27.14.29"
ENET_RECV_PORT = 40001

def ethernet_platform_info(platform):
    if platform == "zynq7000":
        return ("router.ethdriver_mac = [0x00, 0x0A, 0x35, 0x02, 0xFF, 0x1E]; \n\
ethdriver.mmios =  \"0xE000B000:0x1000:12, \n\
                    0xF8000000:0x1000:12\"; \n\
ethdriver.simple_untyped20_pool = 4; \n\
ethdriver.simple_untyped12_pool = 2; \n\
HWEthdriver.mmio_attributes = \"0xE000B000:0x1000\";\n\
router.timer_id = TTC0_TIMER3;\n\
hw.timer_reg_attributes = \"0xF8001000:0x1000\"; \n\
hw.timer_irq_attributes = 44;", 54)

    elif platform == "imx6" or platform == "sabre":
        return ("router.ethdriver_mac = [0x00, 0x19, 0xB8, 0x02, 0xC9, 0xEA]; \n\
ethdriver.mmios =\n\
        \"0x02188000:0x4000:12,\n\
         0x021BC000:0x4000:12,\n\
         0x020E0000:0x4000:12,\n\
         0x020A4000:0x4000:12,\n\
         0x020B0000:0x4000:12,\n\
         0x020C4000:0x1000:12,\n\
         0x020C8000:0x1000:12\";\n\
ethdriver.simple_untyped20_pool = 2; \n\
HWEthdriver.mmio_attributes = \"0x02188000:0x1000\";\n\
router.timer_epit = true;\n\
router.timer_id = 1;\n\
hw.timer_reg_attributes = \"0x020D0000:0x1000\"; \n\
        hw.timer_irq_attributes = 88;", 150)

def serial_platform_info(platform):
    if platform == "zynq7000":
        return ("0xe0000000:0x1000", 59)
    elif platform == "imx6" or platform == "sabre":
        return ("0x02020000:0x1000", 58)

def get_composition(serial_debug_selected):

    composition = "composition {\n"
    composition += "  component DebugServer debug;\n"

    if serial_debug_selected:
        composition += " component DebugSerial debug_hw_serial;\n"

    else:
        composition += "  component DebugRouter router;\n"
        composition += "  component router_hw hw;\n"
        composition += "  component Ethdriver ethdriver;\n"
        composition += "  component HWEthDriver HWEthdriver;\n"
        composition += "  connection seL4HardwareMMIO timer_reg(from router.timer_reg, to hw.timer_reg);\n"
        composition += "  connection seL4HardwareInterrupt timer_irq(from hw.timer_irq, to router.timer_irq);\n"
        composition += "  connection seL4TIMER timer_inf(from router.timer, to hw.timer_inf);\n"
        composition += "  connection seL4HardwareInterrupt hwethirq(from HWEthdriver.irq, to ethdriver.irq);\n"
        composition += "  connection seL4Ethdriver eth_driver(from router.ethdriver, to ethdriver.client);\n"
        composition += "  connection seL4HardwareMMIO ethdrivermmio(from ethdriver.EthDriver, to HWEthdriver.mmio);\n"

    composition += "  }"
    return composition.split("\n")
