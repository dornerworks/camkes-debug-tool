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

def clean_debug(project_name):
	print "Cleaning"
	clean_debug_components(project_name)
	clean_debug_camkes(project_name)
	clean_makefile(project_name)
	clean_debug_interface(project_name)
	clean_debug_component(project_name)
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

# Removes any top level debug camkes files
def clean_debug_camkes(project_name):
	# Regex used to find debug camkes files
	regex1 = re.compile(r'.*\.camkes\.dbg')

	for file_name in os.listdir("apps/%s" % project_name):
		if regex1.match(file_name):
			debug_camkes_path = "apps/%s/%s" % (project_name, file_name)
			os.remove(debug_camkes_path)

def clean_makefile(project_name):
	makefile_path = "apps/%s/Makefile" % project_name
	backup_path = "apps/%s/Makefile.bk" % project_name
	if os.path.isfile(backup_path):
		os.remove(makefile_path)
		os.rename(backup_path, makefile_path)


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

	print used_components

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
							create_debug_component(project_name, original_camkes_file_path, component_file_text)
	print debug_component_types
	return debug_found

# Creates a new camkes file for a debug component. 
# The new file is created with a .dbg extension
def create_debug_component(project_name, original_camkes_file_path, 
	                       component_file_text):
	# Regex used to find debug keyword
	regex1 = re.compile(r'\s*debug;')
	with open(original_camkes_file_path + ".dbg", 'w+') as f:
		for line in component_file_text:
			if not regex1.match(line):
				f.write(line)

# Finds the names of all debug component instances 
# and adds them to the debug_component_instances dict

# Creates a new camkes file that points to the newly
# generated debug component camkes files
def parse_debug_camkes(project_name):
	# Regex used to find component types and names
	regex1 = re.compile(r'component (\w*)\s*(\w*);')
	# Regex used to find component imports
	regex2 = re.compile(r'import \"(.*)\";')

	global camkes_file_text
	with open("apps/%s/%s.camkes" % (project_name, project_name), "r+") as f:
		camkes_file_text = f.readlines()
		for line in camkes_file_text:
			component_search = regex1.search(line)
			if component_search:
				component_type = component_search.group(1)
				component_instance_name = component_search.group(2)
				if component_type in debug_component_types:
					debug_component_instances[component_instance_name] = 0

	for index, line in enumerate(camkes_file_text):
		import_match = regex2.match(line)
		if import_match:
			component_path = import_match.group(1)
			if component_path in debug_component_paths.keys():
				camkes_file_text[index] = "import \"%s.dbg\";\n" % component_path

def write_camkes(project_name):
	print len(camkes_file_text)
	with open("apps/%s/%s.camkes.dbg" % (project_name, project_name), "w+") as f:
		for line in camkes_file_text:
			f.write(line)

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
			f.seek(0)
			for line in makefile_text:
				f.write(line)

# Generates an interface file
def generate_debug_interface(project_name):
	with open("apps/%s/interfaces/%s_debug.idl4" % (project_name, project_name), 'w+') as f:
		f.write("procedure %s_debug {\n    void debug(in int num);\n}\n" % project_name)

# Cleans up the debug interface file
def clean_debug_interface(project_name):
	interface_path = "apps/%s/interfaces/%s_debug.idl4" % (project_name, project_name)
	if os.path.isfile(interface_path):
		os.remove(interface_path)

# Generates the debug component
def generate_debug_component(project_name):
	component_path = "apps/%s/components/%s_debug" % (project_name, project_name)
	if not os.path.isdir(component_path):
		os.mkdir(component_path)
	with open(component_path + ("/%s_debug.camkes" % project_name), 'w+') as f:
		f.write("component %s_debug {\n" % project_name)
		f.write("}")

# Cleans up the debug component
def clean_debug_component(project_name):
	component_path = "apps/%s/components/%s_debug" % (project_name, project_name)
	if os.path.isdir(component_path):
		shutil.rmtree(component_path)
	

def main(argv):
	os.chdir("../..")
	project_name = "gdb-test"	
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
	parse_debug_camkes(project_name)
	# Generate interface
	generate_debug_interface(project_name)
	# Add a debug component
	generate_debug_component(project_name)
	# Write new camkes
	write_camkes(project_name)
	# Modify makefile to build new camkes
	modify_makefile(project_name)
	os.system("make")

if __name__ == "__main__":
	main(sys.argv[1:])