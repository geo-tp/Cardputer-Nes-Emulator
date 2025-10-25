/********************************************************************************/
/*                                                                              */
/* CZ80 macro file                                                              */
/* C Z80 emulator version 0.92                                                  */
/* Copyright 2004-2005 Stéphane Dallongeville                                   */
/*                                                                              */
/********************************************************************************/

// use zR8 for B/C/D/E/H/L registers only
// use zR16 for BC/DE/HL registers only

#ifdef CZ80_USE_MAME_CHANGE_PC
#include "memory.h"
#undef READ_WORD
#undef WRITE_WORD
#endif

//#define mame4all_cz80_rom OP_ROM
//#define mame4all_cz80_ram OP_RAM
extern unsigned char *mame4all_cz80_rom;
extern unsigned char *mame4all_cz80_ram;

int ngc_cpu_readmem16(int address);
void ngc_cpu_writemem16(int address,int data);
int ngc_cpu_readport(int Port);
void ngc_cpu_writeport(int Port,int Value);

#define zR8(A)          CPU->r8[(A) ^ 1]
#define zR16(A)         CPU->r16[A].W
#define pzR16(A)        &(CPU->r16[A])

#define pzFA            &(CPU->FA)
#define zFA             CPU->FA.W
#define zlFA            CPU->FA.B.L
#define zhFA            CPU->FA.B.H
#define zA              zlFA
#define zF              zhFA

#define pzBC            &(CPU->BC)
#define zBC             CPU->BC.W
#define zlBC            CPU->BC.B.L
#define zhBC            CPU->BC.B.H
#define zB              zhBC
#define zC              zlBC

#define pzDE            &(CPU->DE)
#define zDE             CPU->DE.W
#define zlDE            CPU->DE.B.L
#define zhDE            CPU->DE.B.H
#define zD              zhDE
#define zE              zlDE

#define pzHL            &(CPU->HL)
#define zHL             CPU->HL.W
#define zlHL            CPU->HL.B.L
#define zhHL            CPU->HL.B.H
#define zH              zhHL
#define zL              zlHL

#define zFA2            CPU->FA2.W
#define zlFA2           CPU->FA2.B.L
#define zhFA2           CPU->FA2.B.H
#define zA2             zlFA2
#define zF2             zhFA2

#define zBC2            CPU->BC2.W
#define zDE2            CPU->DE2.W
#define zHL2            CPU->HL2.W

#define pzIX            &(CPU->IX)
#define zIX             CPU->IX.W
#define zlIX            CPU->IX.B.L
#define zhIX            CPU->IX.B.H

#define pzIY            &(CPU->IY)
#define zIY             CPU->IY.W
#define zlIY            CPU->IY.B.L
#define zhIY            CPU->IY.B.H

#define pzSP            &(CPU->SP)
#define zSP             CPU->SP.W
#define zlSP            CPU->SP.B.L
#define zhSP            CPU->SP.B.H

#define zPC             PC
#define zRealPC         ((uintptr_t)(PC) - ((uintptr_t)mame4all_cz80_rom))

#define zI              CPU->I
#define zIM             CPU->IM

#define zwR             CPU->R.W
#define zR1             CPU->R.B.L
#define zR2             CPU->R.B.H
#define zR              zR1

#define zIFF            CPU->IFF.W
#define zIFF1           CPU->IFF.B.L
#define zIFF2           CPU->IFF.B.H


#if CZ80_USE_JUMPTABLE
#define _SSOP(A,B) A##B
#define OP(A) _SSOP(OP,A)
#define OPCB(A) _SSOP(OPCB,A)
#define OPED(A) _SSOP(OPED,A)
#define OPXY(A) _SSOP(OPXY,A)
#define OPXYCB(A) _SSOP(OPXYCB,A)
#else
#define OP(A) case A
#define OPCB(A) case A
#define OPED(A) case A
#define OPXY(A) case A
#define OPXYCB(A) case A
#endif

#define REAL_FETCH_BYTE              \
    (*zPC++)

#define REAL_FETCH_BYTE_S            \
    ((s8)(*zPC++))

#define FETCH_BYTE(A)		\
   A = (mame4all_cz80_ram[((uintptr_t)zPC)-((uintptr_t)mame4all_cz80_rom)]); \
   zPC++

#define FETCH_BYTE_S(A)		\
   A = ((s8)(mame4all_cz80_ram[((uintptr_t)zPC)-((uintptr_t)mame4all_cz80_rom)])); \
   zPC++

#if CZ80_LITTLE_ENDIAN
#define FETCH_WORD(A) \
    A= ((unsigned short)(mame4all_cz80_ram[((uintptr_t)zPC)-((uintptr_t)mame4all_cz80_rom)])) | (((unsigned short)(mame4all_cz80_ram[1+((uintptr_t)zPC)-((uintptr_t)mame4all_cz80_rom)])) << 8); 	\
    zPC += 2
#else
#define FETCH_WORD(A) \
    A= ((unsigned short)(mame4all_cz80_ram[1+((uintptr_t)zPC)-((uintptr_t)mame4all_cz80_rom)])) | (((unsigned short)(mame4all_cz80_ram[((uintptr_t)zPC)-((uintptr_t)mame4all_cz80_rom)])) << 8); 	\
    zPC += 2
#endif

#if CZ80_SIZE_OPT
    #define RET(A)              \
        Z80_ICount -= A;              \
        goto Cz80_Exec_Check;
#else
    #define RET(A)              \
        if ((Z80_ICount -= A) <= 0) goto Cz80_Check_Int;  \
        goto Cz80_Exec;
#endif

#ifdef CZ80_USE_MAME_CHANGE_PC
#define SET_PC(A) \
 change_pc16(A) \
 zPC = (u8*)&mame4all_cz80_rom[A]
#else
#define SET_PC(A) zPC = (u8*)&mame4all_cz80_rom[A]
#endif

#define PRE_IO                  \
    CPU->CycleIO = Z80_ICount;

#define POST_IO                 \
    Z80_ICount = CPU->CycleIO;

#define READ_BYTE(A, D)                 \
    D = ngc_cpu_readmem16(A);

#if CZ80_USE_WORD_HANDLER
#define READ_WORD(A, D)                 \
    D = CPU->Read_Word(A);
#elif CZ80_LITTLE_ENDIAN
#define READ_WORD(A, D)                 \
    D = ngc_cpu_readmem16(A) | (ngc_cpu_readmem16((A) + 1) << 8);
#else
#define READ_WORD(A, D)                 \
    D = (ngc_cpu_readmem16(A) << 8) | ngc_cpu_readmem16((A) + 1);
#endif

#define READSX_BYTE(A, D)               \
    D = (s8)ngc_cpu_readmem16(A);

#define WRITE_BYTE(A, D)                \
    ngc_cpu_writemem16(A, D);

#if CZ80_USE_WORD_HANDLER
#define WRITE_WORD(A, D)                \
    ngc_cpu_writemem16(A, D);
#elif CZ80_LITTLE_ENDIAN
#define WRITE_WORD(A, D)                \
    ngc_cpu_writemem16(A, D);              \
    ngc_cpu_writemem16((A) + 1, (D) >> 8);
#else
#define WRITE_WORD(A, D)                \
    ngc_cpu_writemem16(A, D);              \
    ngc_cpu_writemem16((A) + 1, (D) >> 8);
#endif

#define PUSH_16(A)          \
    {                       \
        u32 sp;             \
                            \
        zSP -= 2;           \
        sp = zSP;           \
        WRITE_WORD(sp, A);  \
    }

#define POP_16(A)           \
    {                       \
        u32 sp;             \
                            \
        sp = zSP;           \
        READ_WORD(sp, A)    \
        zSP = sp + 2;       \
    }

#define IN(A, D)            \
    D = ngc_cpu_readport(A);

#define OUT(A, D)           \
    ngc_cpu_writeport(A, D);
