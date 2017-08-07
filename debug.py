#!/usr/bin/env python

#
# Copyright 2017, DornerWorks
#
# This software may be distributed and modified according to the terms of
# the GNU General Public License version 2. Note that NO WARRANTY is provided.
# See "LICENSE_GPLv2.txt" for details.
#
# @TAG(DORNERWORKS_GPL)
#
# This data was produced by DornerWorks, Ltd. of Grand Rapids, MI, USA under
# a DARPA SBIR, Contract Number D16PC00107.
#
# Approved for Public Release, Distribution Unlimited.
#

import sys
import os
camkes_path = os.path.realpath(__file__ + '/../../camkes/')
sys.path.insert(0, camkes_path)
import camkes.parser as parser
import camkes.ast as ast
import copy
import re
import shutil
import getopt

from debug_config import  ENET_RECV_PORT, ENET_IP_ADDR, ethernet_platform_info, get_composition, serial_platform_info

PROJECT_FOLDER = os.environ.get('SOURCE_DIR') + "/"
DEFINITIONS_DIR = os.path.realpath(__file__ + '/../include/definitions.camkes')
CONFIG_CAMKES = os.path.realpath(__file__ + '/../include/configurations.camkes')
TEMPLATES_SRC_DIR = os.path.realpath(__file__ + '/../templates') + "/"
DOT_CONFIG = os.environ.get('PWD') + "/.config"

# Parse the AST and find the declared debug components
def get_debug_components(target_ast):
    debug_types = dict()
    target_assembly = None

    # First find debug declaration in configuration
    for assembly in target_ast:
        if isinstance(assembly, ast.Objects.Assembly):
            for config in assembly.children():
                if isinstance(config, ast.Objects.Configuration):
                    # Find the configuration
                    for setting in config.settings:
                        # Go through each one and look for a debug attribute
                        if setting.attribute == 'debug' and setting.value == '"True"':
                            debug_types[setting.instance] = True

    for assembly in target_ast:
        if isinstance(assembly, ast.Objects.Assembly) or isinstance(assembly, ast.Objects.Component):
            for composition in assembly.children():
                if isinstance(composition, ast.Objects.Composition):
                    for instance in composition.children():
                        if isinstance(instance, ast.Objects.Instance):
                            if debug_types.get(instance.name):
                                del debug_types[instance.name]
                                debug_types[str(instance.children()[0])] = instance.name
                                target_assembly = assembly
    return debug_types, target_assembly

def add_serial_debug_declarations(target_ast, debug_components, selected_arm_plat):
    camkes_composition = get_composition(True)
    instance = debug_components.values()[0]

    # We always need to insert the GDB and Delegate connections
    camkes_composition.insert(-1, "connection seL4Debug debug_delegate(from debug.%s_GDB_delegate, to %s.GDB_delegate);" \
                              % (instance, instance))
    camkes_composition.insert(-1, "connection seL4GDB debug_fault(from %s.fault, to debug.%s_fault);" \
                              % (instance, instance))
    # Add UART Specific connections
    camkes_composition.insert(-1, "connection seL4HardwareMMIO uart_mem(from %s.uart_mem, to debug_hw_serial.uart_mem);" \
                              % instance)
    camkes_composition.insert(-1, "connection seL4HardwareInterrupt uart_irq(from debug_hw_serial.uart_irq, to %s.uart_irq);" \
                              % instance)
    camkes_composition.insert(-1, "connection seL4UART uart_conn(from %s.uart, to debug_hw_serial.uart_inf);" \
                              % instance)

    camkes_composition = "\n".join(camkes_composition)
    debug_ast = parser.parse_to_ast(camkes_composition)

    # update the target assembly to include components,connections,and configuration found in "include"
    for assembly in target_ast:
        if isinstance(assembly, ast.Objects.Assembly):
            for obj in assembly.children():
                if isinstance(obj, ast.Objects.Composition):
                    obj.instances += debug_ast[0].instances
                    obj.connections += debug_ast[0].connections
                elif isinstance(obj, ast.Objects.Configuration):
                    mem, irq = serial_platform_info(selected_arm_plat)
                    obj.settings.append(ast.Objects.Setting("debug_hw_serial", "uart_mem_attributes", "\"%s\"" % mem))
                    obj.settings.append(ast.Objects.Setting("debug_hw_serial", "uart_irq_attributes", "%d" % irq))
                    obj.settings.append(ast.Objects.Setting("%s" % instance, "uart_num", "%d" % 1))

    return target_ast

#Add in debug connections, instances, & configuration found in 'include' folder
def add_ethernet_debug_declarations(target_ast, debug_types, selected_arm_plat):
    camkes_composition =  get_composition(False)

    # Add in Ethernet Connections, GDB Connections
    for i, component in enumerate(debug_types):
        instance = debug_types.get(component)
        # We always need to insert the GDB and Delegate connections
        camkes_composition.insert(-1, "connection seL4Debug debug%d_delegate(from debug.%s_GDB_delegate, to %s.GDB_delegate);" \
                                % (i, instance, instance))
        camkes_composition.insert(-1, "connection seL4GDB debug%d_fault(from %s.fault, to debug.%s_fault);" \
                                % (i, instance, instance))

        camkes_composition.insert(-1, "connection seL4TCP recv%d(from %s.%s_tcp, to router.router);" % (i, instance, instance))
        camkes_composition.insert(-1, "connection seL4GlobalAsynchCallback ready%d(from router.recv_ready, to %s.%s_ready);" \
                                % (i, instance, instance))
        camkes_composition.insert(-1, "connection seL4MultiSharedData send_buf%d(from %s.%s_tcp_send_buf, to router.router_send_buf);" \
                                % (i, instance, instance))
        camkes_composition.insert(-1, "connection seL4MultiSharedData recv_buf%d(from %s.%s_tcp_recv_buf, to router.router_recv_buf);" \
                                % (i, instance, instance))

    camkes_composition = "\n".join(camkes_composition)
    debug_ast = parser.parse_to_ast(camkes_composition)

    platform_config_text, platform_enet_irq = ethernet_platform_info(selected_arm_plat)

    with open(CONFIG_CAMKES) as config_file:
        config_text = config_file.readlines()

    config_text = "\n".join(config_text)
    config_ast = parser.parse_to_ast(config_text)

    #update the target assembly to include components,connections,and configuration found in "include"
    for assembly in target_ast:
        for obj in assembly.children():
            if isinstance(obj, ast.Objects.Composition):
                obj.instances += debug_ast[0].instances
                obj.connections += debug_ast[0].connections
            elif isinstance(obj, ast.Objects.Configuration):
                obj.settings.append(platform_config_text)
                obj.settings.append(ast.Objects.Setting("HWEthdriver", "irq_attributes", "%d" % platform_enet_irq))

                for assembly in config_ast:
                    for config in assembly.children():
                        obj.settings.append(config)
                for i, component in enumerate(debug_types):
                    instance = debug_types.get(component)
                    enet = str(ast.Objects.Setting("%s" % instance, "%s_tcp_recv_buf_attributes" % instance, "\"%s\"" % i))
                    enet += str(ast.Objects.Setting("%s" % instance, "%s_tcp_send_buf_attributes" % instance, "\"%s\"" % i))
                    enet += str(ast.Objects.Setting("%s" % instance, "%s_tcp_attributes" % instance, "\"%s\"" % i))
                    enet += str(ast.Objects.Setting("%s" % instance, "%s_tcp_port" % instance, "%d" % (ENET_RECV_PORT+i)))
                    enet += str(ast.Objects.Setting("%s" % instance, "%s_tcp_global_endpoint" \
                                                    % instance, "\"%s\"" % str(instance + "_ep")))
                    enet += str(ast.Objects.Setting("%s" % instance, "%s_ready_global_endpoint" \
                                                    % instance, "\"%s\"" % str(instance + "_ep")))
                    obj.settings.append(enet)


    return target_ast

def generate_server_component(debug_components, serial_debug_selected):
    server = ""

    if serial_debug_selected:
        server += "component DebugSerial {\n"
        server += "  hardware;\n  dataport Buf uart_mem;\n  emits IRQ uart_irq;\n  provides UART uart_inf;\n}\n\n"

    server += "component DebugServer {\n"
    for instance in debug_components.values():
        server += "  uses CAmkES_Debug %s_GDB_delegate;\n" % instance
        server += "  provides CAmkES_Debug %s_fault;\n" % instance

    server += "}\n\n"
    return server


def get_debug_definitions(serial_debug_selected):

    with open(DEFINITIONS_DIR) as definitions_file:
        definitions_text = definitions_file.read()

    if serial_debug_selected:
         definitions_text += "import <UART.idl4>;\n"
    else:
        definitions_text += "import <Ethdriver/Ethdriver.camkes>;\n"
        definitions_text += "import <ethernet_procedures.camkes>;\n"
        definitions_text += "import <DebugRouter/DebugRouter.camkes>;\n\n"
        definitions_text += "import <tcp.idl4>;\n"
        definitions_text += "import <TIMER.idl4>;\n\n"

    return definitions_text

def update_component_config(project_dir, debug_types, serial_debug_selected):
    for keys in debug_types.keys():
        source = project_dir + 'components/' + keys + '/' + keys

        #if the backed up file DOESN'T EXIST
        if not os.path.isfile(source + ".camkes.bk"):
            with open(source + ".camkes", 'r+') as component_decl:
                component_text = component_decl.readlines()

            # Backup component
            with open(source + ".camkes.bk", 'w+') as component_bk:
                for line in component_text:
                    component_bk.write(line)

            # We search for the debugged component's declaration to add the gdb method connections
            adl_component = re.compile(r"component " + keys)

            component_found = False
            for index, line in enumerate(component_text):
                if adl_component.search(line):
                    if '{' in line:
                        x = index + 1
                    else:
                        x = index + 2
                    component_found = True
                    break

            if component_found:
                new_interface =  "    uses CAmkES_Debug fault;\n    provides CAmkES_Debug GDB_delegate;\n"

                if serial_debug_selected:
                    new_interface += "    dataport Buf uart_mem;\n    consumes IRQ uart_irq;\n    uses UART uart;\n"
                    new_imports = "import <UART.idl4>;\n"
                else:
                    new_interface += "    uses TCP %s_tcp;\n" % debug_types.get(keys)
                    new_interface += "    dataport Buf %s_tcp_recv_buf;\n" % debug_types.get(keys)
                    new_interface += "    dataport Buf %s_tcp_send_buf;\n" % debug_types.get(keys)
                    new_interface += "    consumes Notification %s_ready;\n" % debug_types.get(keys)
                    new_imports = "import <tcp.idl4>;\n"

                component_text.insert(x, new_interface)
                component_text.insert(0, new_imports)

            # Re-write component
            with open(source + ".camkes", 'w+') as component_rw:
                for line in component_text:
                    component_rw.write(line)

def update_makefile(project_dir, serial_debug_selected):
    # If the backup file doesn't exist, we need to make one!
    if not os.path.isfile(project_dir + "Makefile.bk"):
        # Read makefile
        with open(project_dir + "Makefile", 'r+') as orig_makefile:
            makefile_text = orig_makefile.readlines()

        # Backup Makefile
        with open(project_dir + "Makefile.bk", 'w+') as bk_makefile:
            for line in makefile_text:
                bk_makefile.write(line)

        # Search for the ADL and Templates liat and modify them
        templates_found = False
        control_found = False

        adl_regex = re.compile(r"ADL")
        template_regex = re.compile(r'TEMPLATES :=')

        if not serial_debug_selected:
            ethdriver_found = False
            ethdriver_regex = re.compile("Ethdriver")

        # Go through Makefile text and append the neccessary changes
        for index, line in enumerate(makefile_text):
            if template_regex.search(line):
                templates_found = True
                makefile_text[index] = line.rstrip() + "gdb_templates\n"
                if not serial_debug_selected:
                    makefile_text[index] = line.rstrip() + "eth_templates global-templates\n"
            if adl_regex.search(line):
                makefile_text[index] = line.rstrip() + ".dbg\n"

            if not serial_debug_selected:
                if ethdriver_regex.search(line):
                    ethdriver_found = True

        makefile_text.append("\n\n")

        # Add Templates if the project doesn't define their own
        new_lines = list()
        if not templates_found:
            if serial_debug_selected:
                new_lines.append("TEMPLATES := gdb_templates\n")
            else:
                new_lines.append("TEMPLATES := eth_templates gdb_templates global-templates\n")

        if not serial_debug_selected:
            if not ethdriver_found:
                new_lines.append("include Ethdriver/Ethdriver.mk\n")
            new_lines.append("include DebugRouter/DebugRouter.mk\n")

        # Write out new makefile
        makefile_text = new_lines + makefile_text
        with open(project_dir + "Makefile", 'w') as new_makefile:
            for line in makefile_text:
                new_makefile.write(line)

# Copy the templates to the project folder
def copy_templates(project_dir, serial_debug_selected):
    if not os.path.exists(project_dir + "gdb_templates"):
        os.symlink(TEMPLATES_SRC_DIR + 'gdb_templates/', project_dir + 'gdb_templates')
    if not serial_debug_selected:
        if not os.path.exists(project_dir + "eth_templates"):
            os.symlink(TEMPLATES_SRC_DIR + 'eth_templates/', project_dir + 'eth_templates')
        if not os.path.exists(project_dir + "global-templates"):
            os.symlink('../../projects/global-components/templates', project_dir + 'global-templates')

def write_gdbinit(projects_name, debug_components, selected_arm_plat, serial_debug_selected):
    top_folder = os.environ.get('PWD') + "/"
    for i, component in enumerate(debug_components):
        instance = debug_components.get(component)
        with open(top_folder + '%s.gdbinit' % instance, 'w+') as gdbinit_file:
            gdbinit_file.write("symbol-file build/arm/%s/%s/%s.instance.bin\n" %
                               (selected_arm_plat, projects_name, instance))
            if serial_debug_selected:
                gdbinit_file.write("set serial baud 115200\n")
            else:
                gdbinit_file.write("target remote %s:%d" %  (ENET_IP_ADDR, ENET_RECV_PORT+i))


def find_gdb_method():

    if os.path.isfile(DOT_CONFIG):
        # Default GDB communication is Serial, since UART is available on most/all platforms,
        # and Ethernet is only available if built in or on a breakout board.
        serial_debug_selected = True
        eth_regex = re.compile("CONFIG_GDB_ETHERNET=y")

        with open(DOT_CONFIG) as dot_config_file:
            dot_config_text = dot_config_file.readlines()

        # Since Serial is the default, we only search to see if Ethernet is selected
        for line in dot_config_text:
            if eth_regex.search(line):
                serial_debug_selected = False
                break
    else:
        print "Can't find the .config file"
        sys.exit(-1)

    return serial_debug_selected

# Cleanup any new files generated
def clean_debug(project_dir, camkes_file, debug_types):
    import string
    for keys in debug_types.keys():
        source = project_dir + 'components/' + keys + '/'
        if os.path.isfile(source + keys + ".camkes.bk"):
            path, name = os.path.split(source + keys + ".camkes")
            path += '/'
            os.remove(path + name)
            os.rename(path + name + ".bk", path + name)

    if os.path.isfile(project_dir + "Makefile.bk"):
        os.remove(project_dir + "Makefile")
        os.rename(project_dir + "Makefile.bk", project_dir + "Makefile")

    if os.path.isdir(project_dir + "global-templates"):
        os.unlink(project_dir + 'global-templates');
    if os.path.isfile(project_dir + camkes_file + ".dbg"):
        os.remove(project_dir + camkes_file + ".dbg")
    if os.path.isdir(project_dir + "gdb_templates"):
        os.unlink(project_dir + 'gdb_templates');
    if os.path.isdir(project_dir + "eth_templates"):
        os.unlink(project_dir + 'eth_templates');

    top_folder = os.environ.get('PWD') + "/"
    for instance in debug_types.values():
        if os.path.isfile(top_folder + "%s.gdbinit" % instance):
            os.remove(top_folder + "%s.gdbinit" % instance)

def main(argv):
    # Parse input
    try:
        opts, args = getopt.getopt(argv, "cmv:")
    except getopt.GetoptError as err:
        print str(err)
        sys.exit(1)
    if len(args) == 0:
        print "Not enough arguments"
        sys.exit(1)
    elif len(args) > 1:
        print "Too many args"
        sys.exit(1)
    else:
        project_camkes_config = args[0]
        if not os.path.isfile(PROJECT_FOLDER + project_camkes_config):
            print "File not found: %s" % PROJECT_FOLDER + project_camkes_config
            sys.exit(1)

    # Open camkes file for parsing
    with open(PROJECT_FOLDER + project_camkes_config) as camkes_file:
        lines = camkes_file.readlines()

    # Make our configuration usable by the parser
    camkes_text = "\n".join(lines)
    target_ast = parser.parse_to_ast(camkes_text, False, )

    # Use the declared debug components to find the types we must generate - end up with component names
    debug_types, target_assembly = get_debug_components(target_ast)

    if len(debug_types) == 0:
        print "No debug components found. Exiting"
        sys.exit(0)

    for opt, arg in opts:
        if opt == "-c":
            clean_debug(PROJECT_FOLDER, project_camkes_config, debug_types)
            sys.exit(0)

    serial_debug_selected = find_gdb_method()
    selected_arm_plat = os.environ.get('PLAT')

    if serial_debug_selected:
        print "Debugging %s using the serial port" % selected_arm_plat
        if len(debug_types) > 1:
            print "Cannot Debug Multiple Components w/ a Single Serial Port"
            sys.exit(0)
        target_assembly = add_serial_debug_declarations(target_ast, debug_types, selected_arm_plat)
    else:
        print "Debugging %s using the Ethernet port" % selected_arm_plat
        target_assembly = add_ethernet_debug_declarations(target_ast, debug_types, selected_arm_plat)

    # Get the static definitions needed every time and update with method specific lines
    debug_definitions = get_debug_definitions(serial_debug_selected)
    debug_definitions += generate_server_component(debug_types, serial_debug_selected)

    # update debug component's configuration
    update_component_config(PROJECT_FOLDER, debug_types, serial_debug_selected)

    # update the makefile to adjust for new ast, templates
    update_makefile(PROJECT_FOLDER, serial_debug_selected)

    # Copy the templates into the project directory
    copy_templates(PROJECT_FOLDER, serial_debug_selected)

    # Add our debug definitions
    final_camkes = debug_definitions + parser.pretty(parser.show(target_ast))

    # Write new camkes file
    with open(PROJECT_FOLDER + project_camkes_config + ".dbg", 'w') as f:
        for line in final_camkes:
            f.write(line)

    project_name = os.path.basename(os.path.dirname(PROJECT_FOLDER + project_camkes_config));

    # Write a gdbinit file
    write_gdbinit(project_name, debug_types, selected_arm_plat, serial_debug_selected)

if __name__ == "__main__":
    main(sys.argv[1:])
