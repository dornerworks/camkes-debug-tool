#!/usr/bin/env python
import sys
import os
camkes_path =  os.path.realpath(__file__ + '/../../camkes/')
sys.path.insert(0, camkes_path)
import camkes.parser as parser
import camkes.ast as ast
import code
import copy
import re
import shutil
import getopt

apps_folder = os.path.realpath(__file__ + '/../../../apps/') + "/"
definitions_dir = os.path.realpath(__file__ + '/../include/definitions.camkes')
templates_src_dir = os.path.realpath(__file__ + '/../templates') + "/"
debug_camkes = os.path.realpath(__file__ + '/../include/debug.camkes')

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
                                type_ref._symbol = "debug_" + type_ref._symbol
                                debug_types[old_type_name] = type_ref._symbol
    return debug_types

# Generate the new types
def create_debug_component_types(target_ast, debug_types):
    debug_components = []
    for component in target_ast:
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
                string =  " ".join(string)
                debug_ast = parser.parse_to_ast(string);
                # Add the new component to the list
                debug_components.append(debug_ast[0])
    target_ast = debug_components + target_ast
    return target_ast

def add_debug_declarations(target_ast, debug_components):
    # Cheat a little bit by parsing the new debug declarations separately then adding them in
    # Saves us having to manually call object constructors etc.
    with open(debug_camkes) as f:
        s = f.readlines();
    i = 0;
    for component in debug_components:
        s.insert(-1, "connection seL4Debug debug%d_delegate(from debug.%s_GDB_delegate, to %s.GDB_delegate);" % (i, component, component))
        s.insert(-1, "connection seL4GDB debug%d(from %s.fault, to debug.%s_fault);" % (i, component, component))
        i += 1
    s = "\n".join(s)
    debug_ast = parser.parse_to_ast(s);
    debug_instances = debug_ast[0].instances
    debug_connections = debug_ast[0].connections
    for assembly in target_ast:
        if isinstance(assembly, ast.Objects.Assembly):
            for obj in assembly.children():
                if isinstance(obj, ast.Objects.Composition):
                    obj.instances += debug_instances
                    obj.connections += debug_connections
                elif isinstance(obj, ast.Objects.Configuration):
                    serial_irq = ast.Objects.Setting("hw_serial", "irq_attributes", 4);
                    obj.settings.append(serial_irq)
                    serial_attr = ast.Objects.Setting("hw_serial", "serial_attributes", "\"0x3f8:0x3ff\"");
                    obj.settings.append(serial_attr)
    return target_ast

def generate_server_component(debug_components):
    server = ""
    server += "component debug_server {\n"
    server += "  uses IOPort serial_port;\n"
    server += "  consumes IRQ4 serial_irq;\n"
    for component in debug_components:
        server += "  uses CAmkES_Debug %s_GDB_delegate;\n" % component
        server += "  provides CAmkES_Debug %s_fault;\n" % component
    server += "}\n"
    return server

def get_debug_definitions():
    with open(definitions_dir) as f:
        s = f.read()
    return s

def update_makefile(project_camkes, debug_types):
    project_dir = os.path.dirname(os.path.realpath(apps_folder + project_camkes)) + "/"
    if not os.path.isfile(project_dir + "Makefile.bk"):
        # Read makefile
        with open(project_dir + "Makefile", 'r+') as f:
            makefile_text = f.readlines()
        # Backup Makefile
        with open(project_dir + "Makefile.bk", 'w+') as f2:
            for line in makefile_text:
                f2.write(line)
        new_lines = list()
        cfile_regex = re.compile(r"(.*)_CFILES")
        adl_regex = re.compile(r"ADL")
        for index, line in enumerate(makefile_text):
            component_search = cfile_regex.search(line)
            if component_search:
                if component_search.group(1) in debug_types:
                    new_lines.append("debug_" + line)
                #print debug_components
            if adl_regex.search(line):
                makefile_text[index] = line.rstrip() + ".dbg\n"
        makefile_text.append("\n\n")
        # Add template location
        new_lines.append("TEMPLATES := debug\n")
        makefile_text = new_lines + makefile_text
        with open(project_dir + "Makefile", 'w') as f:
            for line in makefile_text:
                f.write(line)

# Cleanup any new files generated
def clean_debug(project_camkes):
    project_dir = os.path.dirname(os.path.realpath(apps_folder + project_camkes)) + "/"
    if os.path.isfile(project_dir + "Makefile.bk"):
        os.remove(project_dir + "Makefile")
        os.rename(project_dir + "Makefile.bk", 
                  project_dir + "Makefile")
    if os.path.isfile(apps_folder + project_camkes + ".dbg"):
        os.remove(apps_folder + project_camkes + ".dbg")
    if os.path.isdir(project_dir + "debug"):
        shutil.rmtree(project_dir + "debug")

# Copy the templates to the project folder
def copy_templates(project_camkes):
    project_dir = os.path.dirname(os.path.realpath(apps_folder + project_camkes)) + "/"
    if not os.path.exists(project_dir + "debug"):
        shutil.copytree(templates_src_dir, project_dir + "debug")

def main(argv):
    # Parse input
    try:
        opts, args = getopt.getopt(argv, "cm")
    except getopt.GetoptError as err:
        print str(err)
        sys.exit(1)
    if len(args) == 0:
        print "Must provide project camkes file"
        sys.exit(1)
    elif len(args) > 1:
        print "Too many args"
        sys.exit(1)
    else:
        project_camkes = args[0]
        if not os.path.isfile(apps_folder + project_camkes):
            print "File not found: %s" % apps_folder + project_camkes
            sys.exit(1)
    for o, a in opts:
        if o == "-c":
            clean_debug(project_camkes)
            sys.exit(0)
    # Open camkes file for parsing
    with open(apps_folder + project_camkes) as f:
        lines = f.readlines();
    # Remove any global imports and add them back in later
    s = ""
    imports = ""
    import_regex = re.compile(r"import \<")
    for line in lines:
        if (import_regex.search(line)):
            imports += line
        else:
            s += line
    # Parse using camkes parser
    target_ast = parser.parse_to_ast(s);
    include_path = ["include/builtin"]
    # Resolve other imports
    project_dir = os.path.dirname(os.path.realpath(apps_folder + project_camkes)) + "/"
    target_ast, _ = parser.resolve_imports(target_ast, project_dir, include_path)
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
    # Output the camkes file
    new_camkes = imports  + debug_definitions + parser.pretty(parser.show(target_ast))
    with open(apps_folder + project_camkes + ".dbg", 'w') as f:
        for line in new_camkes:
            f.write(line)

if __name__ == "__main__":
    main(sys.argv[1:])