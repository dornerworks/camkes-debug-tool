app_folder          = "apps/%s/"

debug_folder        = app_folder + "debug/"
components_folder   = app_folder + "components/"
makefile            = app_folder + "Makefile"
makefile_backup     = app_folder + "Makefile.bk"
new_camkes			= app_folder + "%s.camkes.dbg"
old_camkes			= app_folder + "%s.camkes"

debug_include       = debug_folder + "include/"
debug_camkes	    = debug_folder + "debug.camkes"
templates_to		= debug_folder + "templates"
templates_from      = "tools/debug/templates"

component_path      = components_folder + "%s/"

ethtype_to          = debug_include + "EthType.h"
ethtype_from        = "tools/debug/include/EthType.h"

original_component  = component_path + "%s.camkes"
debugged_component  = component_path + "%s.camkes.dbg"
component_files     = component_path + "%s"
component_files_rel = "components/%s/%s.camkes"
