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

from debug_config import get_composition, serial_platform_info

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
    camkes_composition = get_composition()
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

def generate_server_component(debug_components):
    server = ""

    server += "component DebugSerial {\n"
    server += "  hardware;\n  dataport Buf uart_mem;\n  emits IRQ uart_irq;\n  provides UART uart_inf;\n}\n\n"

    server += "component DebugServer {\n"
    for instance in debug_components.values():
        server += "  uses CAmkES_Debug %s_GDB_delegate;\n" % instance
        server += "  provides CAmkES_Debug %s_fault;\n" % instance

    server += "}\n\n"
    return server

def get_debug_definitions():

    with open(DEFINITIONS_DIR) as definitions_file:
        definitions_text = definitions_file.read()

    definitions_text += "import <UART.idl4>;\n"

    return definitions_text

def update_component_config(project_dir, debug_types):
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

                new_interface += "    dataport Buf uart_mem;\n    consumes IRQ uart_irq;\n    uses UART uart;\n"
                new_imports = "import <UART.idl4>;\n"

                component_text.insert(x, new_interface)
                component_text.insert(0, new_imports)

            # Re-write component
            with open(source + ".camkes", 'w+') as component_rw:
                for line in component_text:
                    component_rw.write(line)


def update_makefile(project_dir):
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

        adl_regex = re.compile(r"ADL")
        template_regex = re.compile(r'TEMPLATES :=')

        # Go through Makefile text and append the neccessary changes
        for index, line in enumerate(makefile_text):
            if template_regex.search(line):
                templates_found = True
                makefile_text[index] = line.rstrip() + "gdb_templates\n"
            if adl_regex.search(line):
                makefile_text[index] = line.rstrip() + ".dbg\n"
        makefile_text.append("\n\n")

        # Add Templates if the project doesn't define their own
        new_lines = list()
        if not templates_found:
            new_lines.append("TEMPLATES := gdb_templates\n")

        # Write out new makefile
        makefile_text = new_lines + makefile_text
        with open(project_dir + "Makefile", 'w') as new_makefile:
            for line in makefile_text:
                new_makefile.write(line)

# Copy the templates to the project folder
def copy_templates(project_dir):
    if not os.path.exists(project_dir + "gdb_templates"):
        os.symlink(TEMPLATES_SRC_DIR + 'gdb_templates/', project_dir + 'gdb_templates')

def write_gdbinit(projects_name, debug_components, selected_arm_plat):
    top_folder = os.environ.get('PWD') + "/"
    for component in debug_components.values():
        with open(top_folder + '%s.gdbinit' % component, 'w+') as gdbinit_file:
            gdbinit_file.write("symbol-file build/arm/%s/%s/%s.instance.bin\n" %
                               (selected_arm_plat, projects_name, component))
            gdbinit_file.write("set serial baud 115200\n")

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

    if os.path.isfile(project_dir + camkes_file + ".dbg"):
        os.remove(project_dir + camkes_file + ".dbg")
    if os.path.isdir(project_dir + "gdb_templates"):
        os.unlink(project_dir + 'gdb_templates');

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

    selected_arm_plat = os.environ.get('PLAT')

    print "Debugging %s using the serial port" % selected_arm_plat
    if len(debug_types) > 1:
        print "Cannot Debug Multiple Components w/ a Single Serial Port"
        sys.exit(0)
    target_assembly = add_serial_debug_declarations(target_ast, debug_types, selected_arm_plat)

    # Get the static definitions needed every time and update with method specific lines
    debug_definitions = get_debug_definitions()
    debug_definitions += generate_server_component(debug_types)

    # update debug component's configuration
    update_component_config(PROJECT_FOLDER, debug_types)

    # update the makefile to adjust for new ast, templates
    update_makefile(PROJECT_FOLDER)

    # Copy the templates into the project directory
    copy_templates(PROJECT_FOLDER)

    # Add our debug definitions
    final_camkes = debug_definitions + parser.pretty(parser.show(target_ast))

    # Write new camkes file
    with open(PROJECT_FOLDER + project_camkes_config + ".dbg", 'w') as f:
        for line in final_camkes:
            f.write(line)

    project_name = os.path.basename(os.path.dirname(PROJECT_FOLDER + project_camkes_config));

    # Write a gdbinit file
    write_gdbinit(project_name, debug_types, selected_arm_plat)

if __name__ == "__main__":
    main(sys.argv[1:])
