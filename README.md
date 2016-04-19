<style>
div.warn {    
    background-color: #fcf2f2;
    border-color: #dFb5b4;
    border-left: 5px solid #fcf2f2;
    padding: 0.5em;
    }
</style>

<style>
div.attn {    
    background-color: #ffffb3;
    border-color: #dFb5b4;
    border-left: 5px solid #ffffb3;
    padding: 0.5em;
    }
</style>

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

## Table of Contents
1. [Quick Start Guide](#quick)
2. [Usage](#usage)
3. [Developers](#developers)
4. [TODO](#todo)

##  Quick Start Guide <a name="quick"></a> ##
This is a guide to get the debugger and run it on an example application.

### Getting the debug project
```
mkdir camkes_debug
cd camkes_debug
repo init -u https://github.com/smaccm/camkes_debug_manifest.git
repo sync
```

### Building the debugger
```
./tools/debug/debug.py gdb_test/gdb_test.camkes
make ia32_gdb_test_defconfig
make
```

### Running the system
```
./qemu_debug.sh
```

We will now connect to the system through GDB remotely (on a different shell)
### Running gdb
```
gdb -ex 'target remote :1234'
```

### Using GDB
Current functionality includes reading memory and registers.
```
info registers
x $esp

```

</br>
## Usage

This debug tool will provide an interface for you to debug components as you wish within CAmkES. 

To select which components you wish to debug, specify the debug attribute in the camkes configuration.

### Example CAmkES file
```c
import <std_connector.camkes>;
import "components/Sender1/Sender1.camkes";
import "components/Receiver1/Receiver1.camkes";

assembly {
  composition {
    component Sender1 sender1;
    component Receiver1 receiver1;
    connection seL4RPC conn1(from sender1.out1, to receiver1.in1);
  }
}

configuration {
    sender1.debug = "True"
}
```
### Running the tool
The debug tool is used by running the script **debug.py**.

The format for running the tool is **./debug.py [-c] project_name/project_camkes_file**.

Options:

**-c** : Cleans the debug files that were generated.

</br>
### Debugging

Currently, the generated tool implements reading and writing memory, and setting software breakpoints.

<div class=warn>
**Warning:** The tool does not currently implement address checking, so reading or writing to an invalid address in the debug component will cause it to crash, requiring it to be restarted. Address checking will be implemented in a future version.
</div>
<div class=attn>
**Attention:** Software breakpoints are currently reset on hitting that breakpoint, if you want to keep the breakpoint you must set it again manually. 
</div>
<br/>

After you have built the image, you should be able to connect to the serial port via GDB. The GDB connection will only be opened once there is a fault or breakpoint, so if you want to inspect on startup you should set a code breakpoint within the component you are debugging.

The same serial port also provides other information about the state of the debugger, simply connect to the port (eg. via minicom) to view any messages.
<br> </br>


## Developers

This section is targeted at those intending to modify the CAmkES debug tool
implementation itself. The information below assumes you are familiar with 
the functionality of CAmkES.

TODO

### Files

**debug.py** - The main script for the debug tool. This is what you should run. Contains the code for parsing CAmkES and CapDL files.

**seL4GDB-to.template.c** - Template for the GDB server code on the fault ep.

**seL4GDB-from.template.c** - Template for the fault ep on the component side. This should just be generating a cap, since it is set manually later.

**seL4Debug-to.template.c** - Template for the GDB delegate on the component being debugged. This will do the actual reading and writing and then send relevant info back to the GDB server.

**seL4Debug-from.template.c** - Template for the GDB server code that communicates with the GDB delegate.

**gdb.c** - GDB remote stub implementation. 

**gdb.h** - GDB remote stub header.
<br></br>

### Inspecting output

You can communicate directly with the debugger using GDB remote protocol commands. These are the commands that GDB sends over the serial port to communicate with the debugger. Currently supported commands are listed below, for more information on commands read the [GDB serial protocol](https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html).


| Command   | Command Description       | Notes                                                                             |
|---------  |------------------------   |--------------------------------------------------------------------------------   |
| q         | Query                     | Working for queries required for GDB initialisation.                              |
| g         | Read general registers    | Complete                                                                          |
| ?         | Halt reason               | Handled for GDB initialisation, but otherwise ignored.                            |
| Hc        | Set thread                | Handled for GDB initialisation, but otherwise ignored.                            |
| m/M       | Memory read / write       | Currently in progress. Working but does not handle faults on accessed memory.     |
| Z0        | Set SW breakpoint         | Working, but dependent on write memory being completed to be fully functional.    |

## TODO
A list of features that need to be implemented, roughly in order of importance and necessity to getting the tool to work.

### Integrate with build system
Make the tool run automatically when provided a flag to debug. Also look at fixing make dependencies so a clean is not necessary when using debug mode.

### Hardware breakpoints
Add kernel support for hardware breakpoints, then implement the GDB function for them.

### Add symbol file functionality
Implement the qSymbol gdb remote command, which will allow us to load symbol files on GDB.

### Safe memory access
Currently memory access is not safe, and will cause the GDB delegate to crash if it is not valid. The GDB delegate currently connects to the standard fault handler, a separate handler must be written and the fault ep changed.

### Divert serial ports correctly
Currently, diverting serial connections on QEMU diverts both debug and program output. Should find a way to divert a single COM port as opposed to all serial.

### Software breakpoints
Currently software breakpoints are removed upon first encountering them. This is probably not the correct behaviour, but there is no current alternative since the instruction must be replaced before the thread is resumed.

### Stepping
Since breakpoints are available, stepping should not be too much work (hardware breakpoints would make this even easier). Main problem is that it is not trivial to determine the next instruction on architectures like i386, where the instruction length varies. 

### Multi debugging
Support multi-debugging either through multiple serial ports (or ethernet) or using a lock on the serial port and using GDB's thread switch functionality.

### Other commands to implement

| Command   | Command Description               |
|---------  |---------------------------------- |
| qSymbol   | Used for getting symbols for GDB  |
| P         | Write register n                  |
| i         | Step one clock cycle              |
| s/S       | Single step                       |
| c/C       | Continue                          |
| D         | Detach                            |
| k         | Kill the target                   |
| vCont?    | Report supported vCont actions    |
| X         | Load binary data                  |
| z/Z       | Clear/Set breakpoints/watchpoins  |