#
# Copyright 2017, DornerWorks
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DORNERWORKS_BSD)
#
def serial_platform_info(platform):
    if platform == "zynq7000":
        return ("0xe0000000:0x1000", 59)
    elif platform == "imx6" or platform == "sabre":
        return ("0x02020000:0x1000", 58)

def get_composition():

    composition = "composition {\n"
    composition += "  component DebugServer debug;\n"
    composition += "  component DebugSerial debug_hw_serial;\n"
    composition += "  }"
    return composition.split("\n")
