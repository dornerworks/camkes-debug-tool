# CAmkES Debug Manual

<!--
     Copyright 2014, NICTA

     This software may be distributed and modified according to the terms of
     the BSD 2-Clause license. Note that NO WARRANTY is provided.
     See "LICENSE_BSD2.txt" for details.

     @TAG(NICTA_BSD)
  -->

This document describes the structure and use of the CAmkES debug tool, which allows you to debug systems built on the CAmkES platform. The documentation is divided into sections for users and developers. The [Usage](#usage) section is for people wanting to debug a component that they or someone else has built on CAmkES, as well as the current limitations of the tool. The [Developers](#developers) will describe the internal implementation of the tool, for anyone who wishes to modify or extend the functionality of the tool itself.
This document assumes some familiary with [CAmkES](https://github.com/seL4/ camkes-tool/blob/master/docs/index.md) and the [seL4 microkernel](http://sel4.systems/). If you are not familiar with them then you should read their documentation first.

## Terminology

## Usage

This debug tool will provide an interface for you to debug components as you wish within CAmkES. 

To select which components you wish to debug, declare the components as debug components in the project CAmkES file. An example is shown below, which assumes the default prefix of "_debug". This can be configured in the config.py file.

### Original CAmkES file

### Debug CAmkES file

The debug tool is used by running the script debug.py. For the tool to work correctly you must ensure that the makefile is configured to build your project (such that when yo run make the project you are using is built).

The format for running the tool is ./debug.py [-c | -m] project_name.

Options:

-c : Cleans the debug files that were generated. Also runs make clean.

-m : Forces code sections to be writable. Use this if you want to debug instruction code, but your project has set read-only code section (which it should by default). Not that this may cause unexpected behaviour if normal operation of your code relies on read-only code.

Both these options require a project name.

## Developers

This section is targeted at those intending to modify the CAmkES debug tool
implementation itself. The information below assumes you are familiar with 
the functionality of CAmkES.

The debug tool works by auto-generating the necessary CAmkES components to provide a remote GDB interface over serial. The steps it does are roughly as follows:

1. Finds the component you have specified to debug ("debug_component" by default).

2. Generate a debug server component that contains the fault_ep to the component you have specified. This means that any fault in that component will trap into the debug server.

3. Generates a new interface within the component you have specified, to allow memory access for the GDB server. Connected to the server by an RPC endpoint.

4. Generate the hardware definitions for the serial connection.

5. Write the changes to a new project .camkes file (project_name + ".dbg" by default)

6. Modify the makefile to point to the new .camkes file

7. Add necessary files to the project (includes, templates for connections, etc.)

8. Build the project

9. Find the fault endpoints RPC caps we generated in the .capdl file and set them as the fault_ep for the process being debugged. This is necessary because CAmkES does not yet have this functionality at CAmkES definition level.

10. Build the project again.

### Files

debug.py - The main script for the debug tool. This is what you should run. Contains the code for parsing CAmkES and CapDL files.

config.py - Contains anything that needs to be user configurable

paths.py - Contains paths that the tool expects files to be. If the project paths change or you are using different paths, specify them here.

clean.py - Code for cleaning the generated debug files.

camkes_strings.py - Any generated camkes code required for rewriting the CAmkES file.