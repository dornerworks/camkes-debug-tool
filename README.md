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

<!--
     Copyright 2017, DornerWorks

     This software may be distributed and modified according to the terms of
     the BSD 2-Clause license. Note that NO WARRANTY is provided.
     See "LICENSE_BSD2.txt" for details.

     @TAG(DORNERWORKS_BSD)
  -->

# CAmkES Debug Manual

This document describes the structure and use of the CAmkES debug tool, which allows you to debug
systems built on the CAmkES platform. The documentation is divided into sections for users and
developers. The [Usage](#usage) section is for people wanting to debug a component that they or
someone else has built on CAmkES, as well as the current limitations of the tool. The
[Developers](#developers) will describe the internal implementation of the tool, for anyone who
wishes to modify or extend the functionality of the tool itself.

This document assumes some familiary with [CAmkES](https://github.com/seL4/
camkes-tool/blob/master/docs/index.md) and the [seL4 microkernel](http://sel4.systems/). If you are
not familiar with them then you should read their documentation first.

All of the following steps were tested using Linux Ubuntu 14.04. This tutorial also assumes the
user is capable of downloading a seL4 application onto the target platform, and that you are
familiar with graphical debug tools.

## Table of Contents
1. [Getting Started](#get_started)
2. [Usage](#usage)
3. [Developers](#developers)
4. [TODO](#TODO)

##  Getting Started <a name="get_started"></a>
This section guides the user on how to get the CAmkES debug tool and compile it with an example
application.

### Getting the Example Application
Performing the following commands will get the all of the required code to build the GDB Sample
Application. The tool will work with compatible ARM Platforms; however, some modification of the
sample application may be required.

```
mkdir camkes_debug
cd camkes_debug
repo init -u https://github.com/dornerworks/dw-sel4-manifest -m gdb.xml
repo sync
```

### Building the Application
The debug tool is integrated into the CAmkES build process; therefore, there is no need to invoke
the tool separately.
```
make zynq7000_serial_gdb_defconfig
make
```
The debug tool will output an <instance>.gdbinit file for every component that is getting debugged.
For reference, per the CAmkES manual, an instance is defined as "An instantiation of a component
type." Any instance of a component should be debuggable.

The debug tool can be used on the i.MX6 and zynq7000 platforms and can communicate with GDB using
the serial protocol. Therefore, either of the following defconfigs can be used.
* imx6\_serial\_gdb\_defconfig
* zynq\_serial\_gdb\_defconfig

### Running the system

Load the compiled application onto your target platform. Once the __pre\_init__ and __post\_init__ functions
have completed, the system will not start due to an initial breakpoint being placed before the __run__
function.

<div class=attn>
<b>Attention:</b> GDB Utilizes CAmkES interfaces to communicate with the remote host; therefore,
debugging functions before __run()__ is not possible.
</div>

At this point, each component selected in the configuration for debugging is ready to be debugged by
GDB.

### Running GDB on the Terminal
On your terminal, enter the following commands to start the GDB Host, load in the symbol-file from
the selected component, and initialize a connection to your target platform.

```
dmesg | grep ttyUSB
sudo chmod 666 /dev/ttyUSBx
```

Where `USBx` is the serial connection used to debug.

```
arm-none-eabi-gdb
(gdb) source <instance>.gdbinit
(gdb) target remote /dev/ttyUSBx
```
If everything goes correctly, you should see the following on your terminal after a few seconds.

```
breakpoint ()
    at /home/chrisguikema/seL4/test/build/arm/zynq7000/gdb_test/src/s1/generated/GDB_delegate_seL4Debug.c:100
100	    asm(".word 0xfedeffe7\n");
```
At this point, GDB is ready to debug your program!

### Using GDB on the Terminal
Current functionality includes reading and writing memory & registers, setting SW Breakpoints, SW
Watchpoints, Stepping, Continuing, and Killing the program.

</br>

## Usage <a name="usage"></a>

This debug tool will provide an interface for you to debug components as you wish within CAmkES.

To select which components you wish to debug, specify the debug attribute in the camkes
configuration. This debug tool cannot handle multiple components being debugged at once using
a single serial port.

### Example CAmkES file
```
import <std_connector.camkes>;
import "components/Sender1/Sender1.camkes";
import "components/Receiver1/Receiver1.camkes";

assembly {
  composition {
    component Sender1 s1;
    component Receiver1 r1;
    connection seL4RPC conn1(from s1.out1, to r1.in1);
  }
}

configuration {
    s1.debug = "True";
    r1.debug = "True";
}
```
### Running the tool
The debug tool is used by running the script **debug.py**. The build process automatically performs
this operation.

The format for running the tool is `./debug.py [-c] project_name/project_camkes_file`. You can
also run the tool with the `make camkes-debug` command.

Options:

 -`-c` : Cleans the debug files that were generated.

</br>

## Developers <a name="developers"></a>

This section is targeted at those intending to modify the CAmkES debug tool implementation
itself. The information below assumes you are familiar with the functionality of CAmkES.

### Architecture

When a CAmkES project is built with the debug tool, a Debug Server is generated that allows
manipulation of the target debug component by GDB. It contains the GDB code and two endpoints to
each debug component.

1. Fault Endpoint that is connected from the debug component to the server.
2. Connection to a delegate used to manipulate data on GDB's behalf.

Debugging with the Serial port, a Dataport is added to the DebugServer and component to be debugged.

### Files

#### Python Scripts

These files are found in the top level of the debug folder.

**debug.py** - The main script for the debug tool. Contains the code for parsing CAmkES files,
  modifying the system architecture and components, and creating backups of the required components
  for restoration.

**debug_config.py** - File for including configuration variables like HW Addresses used in **debug.py**

---

#### GDB Templates

These files are found under `templates/gdb_templates`.

**seL4GDB-to.template.c** - Template for the GDB server code on the fault ep.

**seL4GDB-from.template.c** - Template for the fault ep on the component side. This should just be generating a cap, since it is set manually later.

**seL4Debug-to.template.c** - Template for the GDB delegate on the component being debugged. This will do the actual reading and writing and then send relevant info back to the GDB server.

**seL4Debug-from.template.c** - Template for the GDB server code that communicates with the GDB delegate.

**seL4UART.template.c** - Template for the platform independent UART interface.

**gdb.c** - common GDB remote stub implementation.

**serial_gdb.c** - contains put and get packet functions for serial debugging

**gdb.h** - GDB remote stub header.

---

#### Include

**definitions.camkes** - Contains the custom connector and import statements that will endup in the
  updated application configuration.

---

#### Interfaces

This folder contains the interface `.idl4` files used to communicate between using UART

---

### Inspecting the Output

You can communicate directly with the debugger using GDB remote protocol commands. These are the
commands that GDB sends over the port to communicate with the debugger. Currently supported commands
are listed below, for more information on commands read the
[GDB Protocol](https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html).

Below is a table of all implemented commands.


| **Command** |     **Command Description**    |                        **Notes**                       |
|-------------|--------------------------------|--------------------------------------------------------|
| q           | Query                          | Working for queries required for GDB initialisation.   |
| g/G         | Read/Write general registers   | Complete                                               |
| ?           | Halt reason                    | Complete for current functionality                     |
| Hc          | Set thread                     | Handled for GDB initialisation, but otherwise ignored. |
| m/M         | Memory read / write            | Complete                                               |
| p/P         | Read/write a Register          | Complete                                               |
| Z0          | Set SW breakpoint              | Complete                                               |
| Z2/Z3/Z4    | Write/Read/Access a Watchpoint | Complete                                               |
| s           | Step                           | Complete                                               |
| c           | Continue                       | Complete                                               |
| k           | Kill                           | Complete                                               |

## TODO <a name="TODO"></a>
A list of features that need to be implemented, roughly in order of importance and necessity to
getting the tool to work.

### Hardware breakpoints
Add kernel support for hardware breakpoints, then implement the GDB function for them.

### Safe memory access
Currently memory access is not safe, and will cause the GDB delegate to crash if it is not
valid. The GDB delegate currently connects to the standard fault handler, a separate handler must be
written and the fault ep changed.
