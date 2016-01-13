import re
import os
import shutil
import paths
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

	for component_folder_name in os.listdir(paths.components_folder % project_name):
		for file_name in os.listdir(paths.component_path % (project_name, component_folder_name)):
			if regex1.match(file_name):
				debug_file_path = paths.component_files % (project_name, component_folder_name, file_name)
				os.remove(debug_file_path)

# Removes any debug camkes files
def clean_debug_camkes(project_name):
	if os.path.isfile(paths.new_camkes):
		os.remove(paths.new_camkes)
	if os.path.exists(paths.debug_folder % project_name):
		shutil.rmtree(paths.debug_folder % project_name)

# Resore the makefile
def clean_makefile(project_name):
	makefile_path = paths.makefile % project_name
	backup_path = paths.makefile_backup % project_name
	if os.path.isfile(backup_path):
		os.remove(makefile_path)
		os.rename(backup_path, makefile_path)