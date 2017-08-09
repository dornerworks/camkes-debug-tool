/*
 * Copyright 2017, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DORNERWORKS_GPL)
 *
 * This data was produced by DornerWorks, Ltd. of Grand Rapids, MI, USA under
 * a DARPA SBIR, Contract Number D16PC00107.
 *
 * Approved for Public Release, Distribution Unlimited.
 *
 *
 * Adapted from sparc-stub.c, offered to public domain by HP.
 * https://sourceware.org/git/gitweb.cgi?p=binutils-gdb.git;a=blob;f=gdb/stubs/sparc-stub.c;h=c12d4360a4bb413f9e3052061ace07f2b15a0d30;hb=HEAD
 *
 */

#include <endian.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include <assert.h>

#include <camkes.h>

/*- include 'gdb.h' -*/

#define CPSR      NUMREGS
#define BUFMAX    2048
#define BAD_ARG   0xFF
#define TRAP      0xFEDEFFE7
#define NUM_BP    50

/* Note, if this is defined, ethernet debugging may not function as expected */
#ifdef CONFIG_GDB_PRINT_DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif

typedef enum
{
    R0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,SP,LR,PC,NUMREGS
} ARM_REG;

typedef struct
{
    u32 opcode;
    u32 pc;
} break_point_t;

static const char hexchars[16]="0123456789abcdef";
static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];

/* variables for single step */
static u32* stepOpPtr;
static u32  nextCode = 0;
static bool stepped = false;

break_point_t breakpoints[NUM_BP] = {{0}};

extern void breakinst(void);

void breakpoint(void)
{
    asm(".global breakinst");
    asm("breakinst:");
    asm(".word 0xfedeffe7\n");
}

int gdb_enabled(void) __attribute__((weak));
int gdb_enabled(void)
{
    return 1;
}

static inline void prefetch_flush(void)
{
    /* armv7-A specific. Will need to modifiy for a differnt version */
    asm volatile("mcr p15, 0, %0, c7, c5, 4" :: "r"(0) : "memory");
}

// memory barrier to ensure things are synced up
static inline void mb(void)
{
	asm("DSB\n");	// allow all previous instructions to complete
	asm("DMB\n");	// allow all previous memory transactions to compelte
}

// clear out caches to ensure that any changes to memory will impact future execution
static inline void icache_sync(unsigned long addr, size_t len)
{
    int error;

    prefetch_flush();

    /* We have to rely on seL4 mechanisms to flush the cache because this thread only
       runs with user permissions, and you need supervisor rights to run cache flush opcodes */
    error = seL4_ARM_PageDirectory_CleanInvalidate_Data(/*? my_pd_cap ?*/,  addr, addr+len);
    assert(error == 0);
    error = seL4_ARM_PageDirectory_Unify_Instruction(/*? my_pd_cap ?*/, addr, addr+len);
    assert(error == 0);

    mb();               // memory barrier
    asm("ISB\n");       // flush instruction pipeline
}

static seL4_UserContext parse_registers(seL4_UserContext regs, int reg, seL4_Word value)
{
    switch(reg)
    {
    case R0:
        regs.r0 = value;
        break;
    case R1:
        regs.r1 = value;
        break;
    case R2:
        regs.r2 = value;
        break;
    case R3:
        regs.r3 = value;
        break;
    case R4:
        regs.r4 = value;
        break;
    case R5:
        regs.r5 = value;
        break;
    case R6:
        regs.r6 = value;
        break;
    case R7:
        regs.r7 = value;
        break;
    case R8:
        regs.r8 = value;
        break;
    case R9:
        regs.r9 = value;
        break;
    case R10:
        regs.r10 = value;
        break;
    case R11:
        regs.r11 = value;
        break;
    case R12:
        regs.r12 = value;
        break;
    case SP:
        regs.sp = value;
        break;
    case LR:
    case PC:
    case CPSR:
    default:
        break;
    }
    return regs;
}

// remove the breakpoint, restore original opcode
int del_bp(u32 pc)
{
    int i;
    u32* pc_ptr;
    for(i = 0; i < NUM_BP; i++)
    {
        if(pc == breakpoints[i].pc)
        {
            pc_ptr = (u32*)pc;
            *pc_ptr = breakpoints[i].opcode;

            breakpoints[i].pc = 0;
            breakpoints[i].opcode = 0;
            return 0;
        }
    }
    return -1;
}

// look to see if a breakpoint has been set at this address and what the opcode is
// returns 0xFFFFFFFF if no breakpoint found for the address
u32 find_bp(u32 pc)
{
	int i;

	for(i = 0; i < NUM_BP; i++)
	{
		if(pc == breakpoints[i].pc)
		{
			u32 opcode = breakpoints[i].opcode;
			return opcode;
		}
	}
	return 0xFFFFFFFF;
}

// set a breakpoint at the address in PC
int set_bp(u32 pc, u32 opcode)
{
    int i;
    int slot = NUM_BP;
    for(i = 0; i < NUM_BP; i++)
    {
        if(breakpoints[i].opcode == 0 && breakpoints[i].pc == 0)
            slot = i;
        else if(breakpoints[i].pc == pc)
        {
            //breakpoint at this PC was found, should have the TRAP opcode already set
            return 0;
        }
    }
    if(slot < NUM_BP)
    {
        breakpoints[slot].pc = pc;
        breakpoints[slot].opcode = opcode;
        return 0;
    }
    return -1;
}

/* Convert ch from a hex digit to a numeric value, valid outputs are 0 through 15 */
u8 hex (char ch)
{
    if (ch >= 'a' && ch <= 'f')
        return ch-'a'+10;
    if (ch >= '0' && ch <= '9')
        return ch-'0';
    if (ch >= 'A' && ch <= 'F')
        return ch-'A'+10;

    return BAD_ARG;
}

//converts a series of bytes (as integers) from memory into a string)
char* mem2hex (u8* mem, char* string, int count)
{
    unsigned char ch;

    // other stub implementations install a handler for mem execptions here
    while (count-- > 0)
    {
        ch = *mem++;
        *string++ = hexchars[ch >> 4];
        *string++ = hexchars[ch & 0xf];
    }
    // other stub implementations uninstall the mem execption handler here

    *string = 0;


    return string;
}

//converts a string into a series of bytes (as integers) in memory
u8* hex2mem (char *string, u8* mem, int count)
{
    int i;
    unsigned int ch;

    // other stub implementations install a handler for mem execptions here
    for (i=0; i<count && 0 != *string; i++)
    {
        ch = hex(*string++) << 4;
        ch |= hex(*string++);
        *mem++ = (unsigned char)ch&0xFF;
    }
    // other stub implementations uninstall the mem execption handler here

    return mem;
}

// walk a string, converting each character into a number and building up a total value
int hexToInt(char** ptr, u32* intValue)
{
    int numChars = 0;
    u8 hexValue;

    *intValue = 0;

    while (**ptr)
    {
        hexValue = hex(**ptr);
        if (BAD_ARG == hexValue)
            break;

        *intValue = (*intValue << 4) | hexValue;
        numChars++;

        (*ptr)++;
    }

    return (numChars);
}

/* puts the register offset and value in that register in the buffer */
void put_reg_value(int reg, void* p_value, char** ptr, int size)
{
    **ptr = hexchars[reg >> 4];
    (*ptr)++;
    **ptr = hexchars[reg & 0xf];
    (*ptr)++;
    **ptr = ':';
    (*ptr)++;
    *ptr = mem2hex((u8*)p_value, *ptr, size);
}


seL4_Word handle_breakpoint(void)
{
    int sigval = 5;
    u32 addr, value, length;
    char *ptr;

    seL4_UserContext tcb_regs;
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);

    seL4_Word tcb_cap = seL4_GetMR(1);
    seL4_TCB_ReadRegisters(tcb_cap, false, 0, num_regs, &tcb_regs);

    /* The seL4_UserContext array is not in the same order as GDB expects, therefore,
       we create our own array that we will manipulate as needed */
    seL4_Word local_registers[num_regs];
    memset(local_registers, 0, num_regs);

    local_registers[R0] = tcb_regs.r0;
    local_registers[R1] = tcb_regs.r1;
    local_registers[R2] = tcb_regs.r2;
    local_registers[R3] = tcb_regs.r3;
    local_registers[R4] = tcb_regs.r4;
    local_registers[R5] = tcb_regs.r5;
    local_registers[R6] = tcb_regs.r6;
    local_registers[R7] = tcb_regs.r7;
    local_registers[R8] = tcb_regs.r8;
    local_registers[R9] = tcb_regs.r9;
    local_registers[R10] = tcb_regs.r10;
    local_registers[R11] = tcb_regs.r11;
    local_registers[R12] = tcb_regs.r12;
    local_registers[SP] = tcb_regs.sp;
    local_registers[LR] = tcb_regs.r14;
    local_registers[PC] = tcb_regs.pc;
    local_registers[CPSR] = tcb_regs.cpsr;

    if(!gdb_enabled())
    {
        return local_registers[PC]+4;
    }

    memset(remcomOutBuffer, 0, BUFMAX);
    ptr = remcomOutBuffer;

    if(stepped)
    {
        u32* opPtr;

        /* sanity check */
        opPtr = (u32*)local_registers[PC];

        /* pop a message so someone knows something goofy is going on */
        if(opPtr != stepOpPtr)
        {
            dbg_printf("!GDB-STUB!, PC != opPtr %x %lx,  ", local_registers[PC], (u32)stepOpPtr);
        }

        /* if just single stepped, need to restore the TRAP with the opcode it replaced */
        *stepOpPtr = nextCode;
        icache_sync((u32)stepOpPtr, 4);

        stepped = false;
        *ptr++ = 'S';
        *ptr++ = hexchars[sigval >> 4];
        *ptr++ = hexchars[sigval & 0xf];
    }
    else
    {
        u32* pc_ptr = (u32*)local_registers[PC];
        u32 opcode = find_bp(local_registers[PC]);
        if(0xFFFFFFFF != opcode)
        {
            *pc_ptr = opcode;
            icache_sync((u32)stepOpPtr, 4);
        }
        *ptr++ = 'T';
        *ptr++ = hexchars[sigval >> 4];
        *ptr++ = hexchars[sigval & 0xf];

        put_reg_value(PC, &local_registers[PC], &ptr, 4);
        *ptr++ = ';';
        put_reg_value(SP, &local_registers[SP], &ptr, 4);
        *ptr++ = ';';
    }
    *ptr++ = 0;
    putpacket(remcomOutBuffer);

    /* stay in this loop...forever...or until a single step or continue command returns out of it */
    while (gdb_enabled())
    {
        memset(remcomOutBuffer, 0, BUFMAX);
        ptr = getpacket();

        switch (*ptr++)
        {
        case '?':
            remcomOutBuffer[0] = 'S';
            remcomOutBuffer[1] = hexchars[sigval >> 4];
            remcomOutBuffer[2] = hexchars[sigval & 0xf];
            remcomOutBuffer[3] = 0;
            break;

        /* read a register value */
        case 'p':

            mb();
            if(hexToInt(&ptr, &addr))
            {
                if(25 == addr)
                {
                    mem2hex((u8*)&local_registers[CPSR], remcomOutBuffer, 4);
                }
                else if(addr < NUMREGS)
                {
                    mem2hex((u8*)&local_registers[addr], remcomOutBuffer, 4);
                }
                else
                {
                    strcpy(remcomOutBuffer, "xxxxxxxx");
                }
                break;
            }
            strcpy(remcomOutBuffer, "E01");
            break;

        /* write a register (not PC. can make things not work) */
        case 'P':

            mb();
            if(hexToInt(&ptr, &addr) && *ptr++ == '=')
            {
                if((addr < NUMREGS) && (addr >= 0) && addr != PC)    {
                    value = 0;

                    hexToInt(&ptr, &value);
                    local_registers[addr] = ((value>>24)&0xff) | ((value<<8)&0xff0000)
                                      | ((value>>8)&0xff00) | ((value<<24)&0xff000000);
                    tcb_regs = parse_registers(tcb_regs, addr, local_registers[addr]);
                    if(!seL4_TCB_WriteRegisters(tcb_cap, false, 0, num_regs, &tcb_regs))
                    {
                        strcpy(remcomOutBuffer, "OK");
                    }
                    else
                    {
                        strcpy(remcomOutBuffer, "E01");
                    }
                }
                else
                {
                    strcpy(remcomOutBuffer, "E01");
                }
            }
            break;

        case 'v':
            /* TODO: Implement vCont */
            if(0 == strncmp(remcomInBuffer, "vCont?", 6));
            break;

        case 'H':
            if(0 == strncmp(remcomInBuffer, "Hc-1", 4) || 0 == strncmp(remcomInBuffer, "Hg-1", 4)
               || 0 == strncmp(remcomInBuffer, "Hc0", 3) || 0 == strncmp(remcomInBuffer, "Hg0", 3))
            {
                strcpy(remcomOutBuffer,"OK");
            }
            else
            {
                strcpy(remcomOutBuffer, "E01");
            }
            break;

        case 'q':
            if('C' == *ptr);
            else if(0 == strncmp(remcomInBuffer, "qSupported", 10));
            else if(0 == strncmp(remcomInBuffer, "qfThreadInfo", 12))
            {
                strcpy(remcomOutBuffer, "1");
            }
            else if(0 == strncmp(remcomInBuffer, "qAttached", 9))
            {
                strcpy(remcomOutBuffer, "0");
            }
            else if(0 == strncmp(remcomInBuffer, "qTStatus", 8));
            else if(0 == strncmp(remcomInBuffer, "qOffsets", 8));
            else if(0 == strncmp(remcomInBuffer, "qSymbol::", 9))
            {
                strcpy(remcomOutBuffer, "OK");
            }
            else
            {
                strcpy(remcomOutBuffer, "E01");
            }
            break;

        /* return the value of the CPU registers */
        case 'g':
            mb();
            mem2hex ((u8*) local_registers, remcomOutBuffer, (NUMREGS)*4);
            break;

        /* set the value of the CPU registers - return OK */
        case 'G':
            mb();
            hex2mem(ptr, (u8*)local_registers, NUMREGS * 4);
            strcpy(remcomOutBuffer, "OK");
            break;

        /* remove sw breakpoint */
        case 'z':
            if ((*ptr++ == '0') && (*ptr++ == ','))
            {
                if ((hexToInt(&ptr, &addr)) && (*ptr++ == ',') && (*ptr++ == '4'))
                {
                    if(del_bp(addr))
                    {
                        strcpy(remcomOutBuffer, "E03");
                        break;
                    }
                    icache_sync(addr, 4);
                    strcpy(remcomOutBuffer, "OK");
                    break;
                }
            }
            strcpy(remcomOutBuffer, "E01");
            break;

        /*
           Z0,<addr>,4

           Z0 = SW-BP. Z1 = HW-BP, Z2 = Write Watchpoint,
           Z3 = Read Watchpoint, Z4 = Access Watchpoint
        */
        case 'Z':
            if ((*ptr++ =='0') && (*ptr++ ==','))
            {
                if ((hexToInt(&ptr, &addr)) && (*ptr++ == ',') && (*ptr++ == '4'))
                {
                    u32* pc_ptr = (u32*)addr;

                    /* ignore bp set at current PC, already one there or we wouldn't be here */
                    if(addr != local_registers[PC])
                    {
                        if(set_bp((u32)pc_ptr, *pc_ptr))
                        {
                            strcpy(remcomOutBuffer, "E03");
                            break;
                        }
                        *pc_ptr = TRAP;
                        icache_sync(addr, 4);
                    }
                    strcpy(remcomOutBuffer, "OK");
                    break;
                }
            }
            strcpy(remcomOutBuffer,"E01");
            break;

        /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
        case 'm':

            /* Try to read %x,%x.  */
            mb();
            if (hexToInt(&ptr, &addr) && *ptr++ == ',' && hexToInt(&ptr, &length))
            {
                if (mem2hex((u8*)addr, remcomOutBuffer, length))
                {
                    break;
                }
                strcpy (remcomOutBuffer, "E03");
            }
            else
            {
                strcpy(remcomOutBuffer, "E01");
            }
            break;

        /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
        case 'M':

            /* Try to write '%x,%x:'.  */
            if (hexToInt(&ptr, &addr) && *ptr++ == ',' && hexToInt(&ptr, &length) && *ptr++ == ':')
            {
                if (hex2mem(ptr, (u8*)addr, length))
                {
                    icache_sync(addr, length);
                    strcpy(remcomOutBuffer, "OK");
                    break;
                }
                strcpy(remcomOutBuffer, "E03");
            }
            else
            {
                strcpy(remcomOutBuffer, "E02");
            }
            break;

        /* Single Step */
        case 's':
        {
            u32* opPtr;

            /* when single stepping, save off the next opcode and replace it with the TRAP
                  -this doesn't always work, e.g., when the next opcode isn't going to be
                   executed next because of branch instructions
            */
            opPtr = (u32*)(local_registers[PC]+4);
            stepOpPtr= opPtr;
            nextCode = *opPtr;
            *opPtr = TRAP;

            /* if already sitting at a breakpoint, have to advance PC to next instruction */
            if(local_registers[PC] == (u32)breakinst)
            {
                local_registers[PC]+=4;
            }

            icache_sync((u32)opPtr, 4);
            stepped = true;

            /* break out of the loop and run free */
            return local_registers[PC];
        }

        /* cAA..AA    Continue at address AA..AA(optional) */
        case 'c':

            /* try to read optional parameter, pc unchanged if no param */
            if (hexToInt(&ptr, &addr))
            {
                local_registers[PC] = addr;
            }
            else if(local_registers[PC] == (u32)breakinst)
            {
                local_registers[PC]+=4;
            }
            /* break out of the while(1) loop and run free */
            return local_registers[PC];

        /* toggle debug flag */
        case 'd':
            break;

        /* Kill the program */
        case 'k':
            return 0;
            break;

        /* Reset */
        case 'r':
            break;

        default:
            dbg_printf("'%s' Not implemented!\n",remcomInBuffer);
        }/* end switch */

        /* reply to the request */
        putpacket(remcomOutBuffer);
    }

    return local_registers[PC]+4;
}
