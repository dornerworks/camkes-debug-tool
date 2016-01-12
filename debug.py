#!/usr/bin/env python

import sys
import os
import getopt
import re
import shutil

fault_slots = dict()
used_components = dict()
debug_component_types = dict()
debug_component_paths = dict()
debug_component_instances = dict()
camkes_file_text = []
capdl_text = []
### Cleanup functions ###

# Initiates cleanup
def clean_debug(project_name):
	print "Cleaning"
	clean_debug_components(project_name)
	clean_debug_camkes(project_name)
	clean_makefile(project_name)
	os.system("make clean")

# Removes any component camkes debug files
def clean_debug_components(project_name):
	# Regex used to find debug files
	regex1 = re.compile(r'.*\.camkes\.dbg')

	for component_folder_name in os.listdir("apps/%s/components" % project_name):
		for file_name in os.listdir("apps/%s/components/%s" % (project_name, component_folder_name)):
			if regex1.match(file_name):
				debug_file_path = "apps/%s/components/%s/%s" % (project_name, component_folder_name, file_name)
				os.remove(debug_file_path)

# Removes any debug camkes files
def clean_debug_camkes(project_name):
	# Regex used to find debug camkes files
	regex1 = re.compile(r'.*\.camkes\.dbg')

	for file_name in os.listdir("apps/%s" % project_name):
		if regex1.match(file_name):
			debug_camkes_path = "apps/%s/%s" % (project_name, file_name)
			os.remove(debug_camkes_path)
	if os.path.exists("apps/%s/debug" % project_name):
		shutil.rmtree("apps/%s/debug" % project_name)
	#if os.path.islink("apps/%s/debug/include/EthType.h" % project_name):
	#	os.remove("apps/%s/debug/include/EthType.h" % project_name)		

def clean_makefile(project_name):
	makefile_path = "apps/%s/Makefile" % project_name
	backup_path = "apps/%s/Makefile.bk" % project_name
	if os.path.isfile(backup_path):
		os.remove(makefile_path)
		os.rename(backup_path, makefile_path)

	
### Code gen functions ###

def add_templates(project_name):
	os.symlink(os.path.abspath("tools/debug/templates"), "apps/%s/debug/templates" % project_name)

# Finds the types of components used in this project and adds them to the used_components dict
def find_used_components(project_name):
	# Regex used to find used components
	regex1 = re.compile(r'component (\w*)')

	with open("apps/%s/%s.camkes" % (project_name, project_name), "r+") as f:
		for line in f:
			component_search = regex1.search(line)
			if component_search:
				component = component_search.group(1)
				used_components[component] = 0

# Searches used components for debug keywords, 
# and adds the path to the debug_component_paths dict,
# and the name of the type to debug_component_types
# Creates a new camkes file for each one found
# Returns true if any debug components found, false otherwise
def parse_debug_components(project_name):
	# Regex used to find camkes files
	regex1 = re.compile(r'.*\.camkes')
	# Regex used to find name of component
	regex2 = re.compile(r'component (\w*)')
	# Regex used to find debug keyword
	regex3 = re.compile(r'\s*debug;')

	debug_found = False

	for component_folder_name in os.listdir("apps/%s/components" % project_name):
		for file_name in os.listdir("apps/%s/components/%s" % (project_name, component_folder_name)):
			if regex1.match(file_name):
				original_camkes_file_path = "apps/%s/components/%s/%s" % (project_name, component_folder_name, file_name)
				relative_path = "components/%s/%s" % (component_folder_name, file_name)
				with open(original_camkes_file_path) as f:
					component_file_text = f.readlines()
				component_type = ""
				for line_index in range(0, len(component_file_text)):
					component_match = regex2.match(component_file_text[line_index])
					debug_match = regex3.match(component_file_text[line_index])
					if component_match:
						component_type = component_match.group(1)
						if component_type not in used_components.keys():
							break
					if debug_match and component_type in used_components.keys():
						debug_found = True
						debug_component_types[component_type] = 0
						debug_component_paths[relative_path] = 0
						create_debug_camkes(project_name, original_camkes_file_path, component_file_text)
	return debug_found

# Creates a new camkes file for a debug component. 
# The new file is created with a .dbg extension
def create_debug_camkes(project_name, original_camkes_file_path, 
	                       component_file_text):
	# Regex used to find debug keyword
	regex1 = re.compile(r'\s*debug;')
	with open(original_camkes_file_path + ".dbg", 'w+') as f:
		for line in component_file_text:
			if not regex1.match(line):
				f.write(line)
			else:
				f.write("  uses %s_debug debug;\n" % project_name)
				f.write("  provides %s_debug internal;\n" % project_name)
# Finds the names of all debug component instances 
# and adds them to the debug_component_instances dict

# Creates a new camkes file that points to the newly
# generated debug component camkes files
def parse_camkes(project_name):
	# Regex used to find component types and names
	regex1 = re.compile(r'debug_component (\w*)\s*(\w*);')
	# Regex used to find component imports
	regex2 = re.compile(r'import \"(.*)\";')

	global camkes_file_text
	with open("apps/%s/%s.camkes" % (project_name, project_name), "r+") as f:
		camkes_file_text = f.readlines()
	for index, line in enumerate(camkes_file_text):
		component_search = regex1.search(line)
		if component_search:
			component_type = component_search.group(1)
			component_instance_name = component_search.group(2)
			if component_type in debug_component_types:
				debug_component_instances[component_instance_name] = 0
			camkes_file_text[index] = line.replace("debug_component", "component")
					
	for index, line in enumerate(camkes_file_text):
		import_match = regex2.match(line)
		if import_match:
			component_path = import_match.group(1)
			if component_path in debug_component_paths.keys():
				camkes_file_text[index] = "import \"%s.dbg\";\n" % component_path

# Modifies the makefile to build the debug camkes file instead
def modify_makefile(project_name):
	# Regex to find camkes file reference
	regex1 = re.compile(r'ADL := (.*)\.camkes')
	# If a backup exists it means it has already been modified, no need to do anything
	if not os.path.isfile("apps/%s/Makefile.bk" % project_name):
		with open("apps/%s/Makefile" % project_name, 'r+') as f:
			makefile_text = f.readlines()
			# Create a backup
			with open("apps/%s/Makefile.bk" % project_name, 'w+') as f2:
				for line in makefile_text:
					f2.write(line)
			# Modify the makefile and write to it
			for line_index in range(0, len(makefile_text)):
				camkes_match = regex1.match(makefile_text[line_index])
				if camkes_match:
					makefile_text[line_index] = "ADL := %s.camkes.dbg\n" % camkes_match.group(1)
			makefile_text.insert(line_index, "debug_server_HFILES = debug/include/EthType.h\n")
			makefile_text.insert(line_index, "TEMPLATES := debug/templates\n")
			f.seek(0)
			for line in makefile_text:
				f.write(line)

# Modify the camkes text to include the new component, and add dummy connections
# Also add the serial port and connections
# TODO: Change this to make sure we insert in the composition section
def modify_camkes(project_name):
	# Regex to find camkes composition start
	regex1 = re.compile(r'\s*}')
	regex2 = re.compile(r'assembly {')
	global camkes_file_text
	# Insert the debug component import
	camkes_file_text.insert(0 , "import \"debug/debug.camkes\";\n");
	for index, line in enumerate(camkes_file_text):
		if regex1.match(line):
			camkes_file_text.insert(index, "    component debug_server debug;\n" )
			index += 1
			conn_num = 1
			for component_instance in debug_component_instances.keys():
				camkes_file_text.insert(index, "    connection seL4GDB debug%s(from %s.debug, to debug.%s_debug);\n" % 
			    (conn_num, component_instance, component_instance))
				camkes_file_text.insert(index, "    connection seL4Debug debug%s_internal(from debug.%s_internal, to %s.internal);\n" % 
			    (conn_num, component_instance, component_instance))
				index += 1
			index += 1
			new_line =  "    component Serial hw_serial;\n"
			new_line += "    component EthDriver hw_eth;\n"
			new_line += "    connection seL4HardwareIOPort debug_port(from debug.serial_port, to hw_serial.serial);\n"
			new_line += "    connection seL4HardwareInterrupt interrupt1(from hw_serial.irq, to debug.serial_irq);\n"
			new_line += "    //connection seL4HardwareMMIO ethdrivermmio1(from debug.mmio, to hw_eth.mmio);\n"
			new_line += "    //connection seL4HardwareInterrupt interrupt2(from hw_eth.irq, to debug.eth_irq);\n"
			camkes_file_text.insert(index , new_line);
			new_line =  "  configuration {\n    hw_serial.serial_attributes = \"0x3f8:0x3ff\";\n    hw_serial.irq_attributes = 4;\n"
			new_line += "    //hw_eth.mmio_attributes = \"0xf1b80000:0x80000\";\n    //hw_eth.irq_attributes = 16;\n  }\n"
			camkes_file_text.insert(index + 2, new_line)
			break


def add_debug_files(project_name):
	os.mkdir("apps/%s/debug" % project_name)
	if not os.path.exists("apps/%s/debug/include" % project_name):
		os.mkdir("apps/%s/debug/include" % project_name)
	os.symlink(os.path.abspath("tools/debug/EthType.h"), "apps/%s/debug/include/EthType.h" % project_name)
	with open("apps/%s/debug/debug.camkes" % (project_name), "w+") as f:
		new_line =  "connector seL4Debug {\n  from  Procedure user_inf template \"seL4Debug-from.template.c\";\n"
		new_line += "  to Procedure user_inf template \"seL4Debug-to.template.c\";\n}\n\n"
		new_line += "connector seL4GDB {\n  from  Procedure user_inf template \"seL4GDB-from.template.c\";\n"
		new_line += "  to Procedure user_inf template \"seL4GDB-to.template.c\";\n}\n\n"
		new_line += "procedure %s_debug {\n  void debug(in int num);\n}\n\n" % project_name
		new_line += "component Serial {\n  hardware;\n  emits IRQ4 irq;\n  provides IOPort serial;\n}\n\n"
		new_line += "component EthDriver {\n  hardware;\n  emits IRQ16 irq;\n  dataport EthDriverMMIO_t mmio;\n}\n\n"
		new_line += "component debug_server {\n  include \"EthType.h\";\n  uses IOPort serial_port; \n"
		new_line += "  consumes IRQ4 serial_irq;\n  //dataport EthDriverMMIO_t mmio;\n  //consumes IRQ16 eth_irq;\n"
		for component_instance in debug_component_instances.keys():
			new_line += "  //%s debug\n" % component_instance
			new_line += "  uses %s_debug %s_internal;\n" % (project_name, component_instance)
			new_line += "  provides %s_debug %s_debug;\n" % (project_name, component_instance)
		new_line += "}\n\n"	
		f.write(new_line)

# Write a new project camkes file
def write_camkes(project_name):
	with open("apps/%s/%s.camkes.dbg" % (project_name, project_name), "w+") as f:
		for line in camkes_file_text:
			f.write(line)

# Find fault ep slots
def find_fault_eps(project_name):
	global debug_component_instances
	global capdl_text

	with open("build/x86/pc99/%s/%s.cdl" % (project_name, project_name)) as f:
		capdl_text = f.readlines();

	for component_instance in debug_component_instances.keys():
		regex1 = re.compile(r'cnode_%s {' % component_instance)
		regex2 = re.compile(r'(0x\w*): conn\d*_%s_debug \(RWX\)' % component_instance)
		for index, line in enumerate(capdl_text):
			cnode_match = regex1.match(line)
			if cnode_match:
				while capdl_text[index] != "}\n":
					fault_slot_match = regex2.match(capdl_text[index])
					if fault_slot_match:
						debug_component_instances[component_instance] = int(fault_slot_match.group(1), 16)
						break
					index += 1
				break

# Register fault eps
def register_fault_eps():
	global debug_component_instances
	global capdl_text

	for component_instance, fault_ep in debug_component_instances.iteritems():
		regex1 = re.compile(r'(%s_tcb_.*) = tcb \((.*)\)' % component_instance)
		for index, line in enumerate(capdl_text):
			tcb_decl_match = regex1.match(line)
			if tcb_decl_match:
				tcb_name = tcb_decl_match.group(1)
				tcb_params = tcb_decl_match.group(2) 
				capdl_text[index] = "%s = tcb (%s, fault_ep: %s)\n" % (tcb_name, tcb_params, hex(fault_ep))

# Write out the capdl file
def write_capdl(project_name):
	with open("build/x86/pc99/%s/%s.cdl" % (project_name, project_name), 'w+') as f:
		for line in capdl_text:
			f.write(line)

def main(argv):
	os.chdir("../..")
	project_name = "gdb_test"	
	try:
		opts, args = getopt.getopt(argv, "c")
	except getopt.GetoptError as err:
		print str(err)
		sys.exit(1)
	# Check for clean flag
	for o, a in opts:
		if o == "-c":
			clean_debug(project_name)
			sys.exit(0)
	# Find components used in this project
	find_used_components(project_name)
	# Parse debug components
	debug_found = parse_debug_components(project_name)
	# Check if debug components found
	if not debug_found:
		print "No used debug components were found in %s" % project_name
		sys.exit(0)
	# Parse project camkes
	parse_camkes(project_name)

	# Modify camkes to include debug code
	modify_camkes(project_name)
	add_debug_files(project_name)
	# Write new camkes
	write_camkes(project_name)
	# Modify makefile to build new camkes
	modify_makefile(project_name)
	# Add custom templates
	add_templates(project_name)
	os.system("make -j8 libmuslc")
	os.system("make")
	os.system("make")
	# Find the slots that fault eps are placed
	find_fault_eps(project_name)
	register_fault_eps()
	write_capdl(project_name)
	os.system("make")

if __name__ == "__main__":
	main(sys.argv[1:])