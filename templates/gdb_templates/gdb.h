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
 */

#ifndef __GDB_HEADER_H
#define __GDB_HEADER_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

seL4_Word handle_breakpoint(void);
void put_reg_value(int reg, void* p_value, char** ptr, int size);
void putpacket (char *buffer);
char* getpacket (void);
int hexToInt(char** ptr, u32* intValue);
u8* hex2mem (char *string, u8* mem, int count);
char* mem2hex (u8* mem, char* string, int count);
u8 hex (char ch);
int set_bp(u32 pc, u32 opcode);
u32 find_bp(u32 pc);
int del_bp(u32 pc);

static inline void icache_sync(unsigned long addr, size_t len);
static inline void mb(void);
static inline void prefetch_flush(void);

#endif  /* __GDB_HEADER_H */
