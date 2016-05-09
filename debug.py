#!/usr/bin/env python
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

from debug_config import SERIAL_IRQ_NUM, SERIAL_PORTS, \
                         PLAT, ARCH

APPS_FOLDER = os.path.realpath(__file__ + '/../../../apps/') + "/"
DEFINITIONS_DIR = os.path.realpath(__file__ + '/../include/definitions.camkes')
TEMPLATES_SRC_DIR = os.path.realpath(__file__ + '/../templates') + "/"
DEBUG_CAMKES = os.path.realpath(__file__ + '/../include/debug.camkes')

# Find debug components declared in the camkes file
def get_debug_components(target_ast):
    debug = dict()
    for assembly in target_ast:
        if isinstance(assembly, ast.Objects.Assembly):
            for config in assembly.children():
                if isinstance(config, ast.Objects.Configuration):
                    # Find the configuration
                    for setting in config.settings:
                        # Go through each one and look for a debug attribute
                        if setting.attribute == 'debug':
                            debug[setting.instance] = None
                            config.settings.remove(setting)
    return debug

# Use the declared debug components to find the types we must generate
def get_debug_component_types(target_ast, debug_components):
    debug_types = dict()
    for assembly in target_ast:
        if isinstance(assembly, ast.Objects.Assembly):
            for composition in assembly.children():
                if isinstance(composition, ast.Objects.Composition):
                    for instance in composition.children():
                        if isinstance(instance, ast.Objects.Instance):
                            # Go through each declared component, 
                            # and check if it is a debug component
                            # Copy the type and prefix with debug_
                            if instance.name in debug_components:
                                type_ref = instance.children()[0]
                                old_type_name = copy.copy(type_ref._symbol)
                                type_ref._symbol = type_ref._symbol
                                debug_types[old_type_name] = type_ref._symbol
    return debug_types

# Generate the new types
def create_debug_component_types(target_ast, debug_types):
    for index, component in enumerate(target_ast):
        if isinstance(component, ast.Objects.Component):
            # Find the debug types and copy the definition
            # Add the necessary interfaces
            if component.name in debug_types:
                debug_component = copy.copy(component)
                debug_component.name = debug_types[component.name]
                #Have to make a new component with the debug connections
                # These are the debug interfaces
                new_interface = "uses CAmkES_Debug fault;\n"
                new_interface += "provides CAmkES_Debug GDB_delegate;\n"
                # Get the component as a string and re-parse it with the new interfaces
                string = debug_component.__repr__().split()
                string[-1:-1] = new_interface.split()
                string = " ".join(string)
                debug_ast = parser.parse_to_ast(string)
                # Replace the debug component
                target_ast[index] = debug_ast[0]
    return target_ast

def add_debug_declarations(target_ast, debug_components):
    # Cheat a little bit by parsing the new debug declarations separately then adding them in
    # Saves us having to manually call object constructors etc.
    with open(DEBUG_CAMKES) as debug_file:
        camkes_text = debug_file.readlines()
    i = 0
    for component in debug_components:
        camkes_text.insert(-1, "connection seL4Debug debug%d_delegate(from debug.%s_GDB_delegate, \
                      to %s.GDB_delegate);" % (i, component, component))
        camkes_text.insert(-1, "connection seL4GDB debug%d(from %s.fault, to debug.%s_fault);"
                     % (i, component, component))
        i += 1
    camkes_text = "\n".join(camkes_text)
    debug_ast = parser.parse_to_ast(camkes_text)
    debug_instances = debug_ast[0].instances
    debug_connections = debug_ast[0].connections
    for assembly in target_ast:
        if isinstance(assembly, ast.Objects.Assembly):
            for obj in assembly.children():
                if isinstance(obj, ast.Objects.Composition):
                    obj.instances += debug_instances
                    obj.connections += debug_connections
                elif isinstance(obj, ast.Objects.Configuration):
                    serial_irq = ast.Objects.Setting("debug_hw_serial", "irq_attributes", \
                                                     SERIAL_IRQ_NUM)
                    obj.settings.append(serial_irq)
                    serial_attr = ast.Objects.Setting("debug_hw_serial", "serial_attributes",\
                                                      "\"%s\"" % SERIAL_PORTS)
                    obj.settings.append(serial_attr)
    return target_ast

def generate_server_component(debug_components):
    server = ""
    server += "component debug_server {\n"
    server += "  uses IOPort serial_port;\n"
    server += "  consumes IRQ%s serial_irq;\n" % SERIAL_IRQ_NUM
    for component in debug_components:
        server += "  uses CAmkES_Debug %s_GDB_delegate;\n" % component
        server += "  provides CAmkES_Debug %s_fault;\n" % component
    server += "}\n"
    return server

def get_debug_definitions():
    with open(DEFINITIONS_DIR) as definitions_file:
        definitions_text = definitions_file.read() % SERIAL_IRQ_NUM
    return definitions_text

def update_makefile(project_camkes, debug_types):
    project_dir = os.path.dirname(os.path.realpath(APPS_FOLDER + project_camkes)) + "/"
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
        for index, line in enumerate(makefile_text):
            if template_regex.search(line):
                # Add the debug templates
                makefile_text[index] = line.rstrip() + " debug\n"
                templates_found = True
            if adl_regex.search(line):
                # Change the CDL target
                makefile_text[index] = line.rstrip() + ".dbg\n"
        makefile_text.append("\n\n")
        # Add templates if they didn't define their own
        new_lines = list()
        if not templates_found:
            new_lines.append("TEMPLATES := debug\n")
        # Write out new makefile
        makefile_text = new_lines + makefile_text
        with open(project_dir + "Makefile", 'w') as new_makefile:
            for line in makefile_text:
                new_makefile.write(line)

# Cleanup any new files generated
def clean_debug(project_camkes):
    project_dir = os.path.dirname(os.path.realpath(APPS_FOLDER + project_camkes)) + "/"
    if os.path.isfile(project_dir + "Makefile.bk"):
        os.remove(project_dir + "Makefile")
        os.rename(project_dir + "Makefile.bk", 
                  project_dir + "Makefile")
    if os.path.isfile(APPS_FOLDER + project_camkes + ".dbg"):
        os.remove(APPS_FOLDER + project_camkes + ".dbg")
    if os.path.isdir(project_dir + "debug"):
        shutil.rmtree(project_dir + "debug")
    top_folder = os.path.realpath(__file__ + '/../../..') + "/"
    if os.path.isfile(top_folder + ".gdbinit"):
        os.remove(top_folder + ".gdbinit")

# Copy the templates to the project folder
def copy_templates(project_camkes):
    project_dir = os.path.dirname(os.path.realpath(APPS_FOLDER + project_camkes)) + "/"
    if not os.path.exists(project_dir + "debug"):
        shutil.copytree(TEMPLATES_SRC_DIR, project_dir + "debug")

# Write a .gdbinit file
# Currently only loads the symbol table for the first debug component
def write_gdbinit(projects_name, debug_components):
    top_folder = os.path.realpath(__file__ + '/../../..') + "/"
    with open(top_folder + '.gdbinit', 'w+') as gdbinit_file:
        component_name = debug_components.keys()[0]
        gdbinit_file.write("target remote :1234\n")
        gdbinit_file.write("symbol-file build/%s/%s/%s/%s.instance.bin\n" % 
                (ARCH, PLAT, projects_name, component_name))
        


def main(argv):
    # Parse input
    vm_mode = False
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
        project_camkes = args[0]
        if not os.path.isfile(APPS_FOLDER + project_camkes):
            print "File not found: %s" % APPS_FOLDER + project_camkes
            sys.exit(1)
    for opt, arg in opts:
        if opt == "-c":
            clean_debug(project_camkes)
            sys.exit(0)
        if opt == "-v":
            vm_mode = True
            vm = arg
    # Open camkes file for parsing
    with open(APPS_FOLDER + project_camkes) as camkes_file:
        lines = camkes_file.readlines()
    # Save any imports and add them back in
    imports = ""
    import_regex = re.compile(r'import .*')
    for line in lines:
        if import_regex.match(line):
            imports += line

    camkes_text = "\n".join(lines)
    # Parse using camkes parser
    camkes_builtin_path = os.path.realpath(__file__ + '/../../camkes/include/builtin')
    include_path = [camkes_builtin_path]
    if vm_mode:
        print "vm mode"
        cpp = True
        config_path = os.path.realpath(__file__ + '/../../../apps/%s/configurations' % vm)
        vm_components_path = os.path.realpath(__file__ + '/../../../apps/%s/../../components/VM' % vm)
        plat_includes = os.path.realpath(__file__ + '/../../../kernel/include/plat/%s' % PLAT)
        cpp_options  = ['-DCAMKES_VM_CONFIG=%s' % vm, "-I"+config_path, "-I"+vm_components_path, "-I"+plat_includes]
        include_path.append(os.path.realpath(__file__ + "/../../../projects/vm/components"))
        include_path.append(os.path.realpath(__file__ + "/../../../projects/vm/interfaces"))
    else:
        cpp = False
        cpp_options = []
    target_ast = parser.parse_to_ast(camkes_text, cpp, cpp_options)  
    # Resolve other imports
    project_dir = os.path.dirname(os.path.realpath(APPS_FOLDER + project_camkes)) + "/"
    target_ast, _ = parser.resolve_imports(target_ast, project_dir, include_path, cpp, cpp_options)
    target_ast = parser.resolve_references(target_ast)
    # Find debug components declared in the camkes file
    debug_components = get_debug_components(target_ast)
    # Use the declared debug components to find the types we must generate
    debug_types = get_debug_component_types(target_ast, debug_components)
    # Generate the new types
    target_ast = create_debug_component_types(target_ast, debug_types)
    # Add declarations for the new types
    target_ast = add_debug_declarations(target_ast, debug_components)
    # Get the static definitions needed every time
    debug_definitions = get_debug_definitions()
    # Generate server based on debug components
    debug_definitions += generate_server_component(debug_components)
    # Update makefile with the new debug camkes
    update_makefile(project_camkes, debug_types)
    # Copy the templates into the project directory
    copy_templates(project_camkes)
    # Add our debug definitions
    new_camkes = parser.pretty(parser.show(target_ast))
    # Reparse and rearrange the included code
    procedures = []
    main_ast = []
    new_ast = parser.parse_to_ast(new_camkes, cpp, cpp_options)
    for component in new_ast:
        if isinstance(component, ast.Objects.Procedure):
            procedures.append(component)
        else:
            main_ast.append(component)   
    final_camkes = imports + debug_definitions + parser.pretty(parser.show(procedures + main_ast))
    # Write new camkes file
    with open(APPS_FOLDER + project_camkes + ".dbg", 'w') as f:
        for line in final_camkes:
            f.write(line)
    # Write a gdbinit file
    name_regex = re.compile(r"(.*)/")
    search = name_regex.search(project_camkes)
    if debug_components:
        write_gdbinit(search.group(1), debug_components)
    else:
        print "No debug components found"

if __name__ == "__main__":
    main(sys.argv[1:])