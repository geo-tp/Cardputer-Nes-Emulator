/********************************************************************************/
/*                                                                              */
/* CZ80 include file                                                            */
/* C Z80 emulator version 0.92                                                  */
/* Copyright 2004-2005 Stéphane Dallongeville                                   */
/*                                                                              */
/********************************************************************************/

#ifndef _CZ80_H_
#define _CZ80_H_

#if defined(__cplusplus)
extern "C" {
#endif

/******************************/
/* Compiler dependant defines */
/******************************/

#ifndef u8
#define u8              unsigned char
#endif

#ifndef s8
#define s8              signed char
#endif

#ifndef u16
#define u16             unsigned short
#endif

#ifndef s16
#define s16             signed short
#endif

#ifndef u32
#define u32             unsigned int
#endif

#ifndef s32
#define s32             signed int
#endif

//#define CZ80_FASTCALL   __fastcall
#define CZ80_FASTCALL


/*************************************/
/* Z80 core Structures & definitions */
/*************************************/

#define CZ80_FETCH_BITS         4   // [4-12]   default = 8

#define CZ80_FETCH_SFT          (16 - CZ80_FETCH_BITS)
#define CZ80_FETCH_BANK         (1 << CZ80_FETCH_BITS)

#define CZ80_LITTLE_ENDIAN      1
#define CZ80_USE_JUMPTABLE      1
#define CZ80_IRQ_CYCLES			1
#define CZ80_SIZE_OPT           1
#define CZ80_USE_WORD_HANDLER   0
#define CZ80_EXACT              0
#define CZ80_DEBUG              0

//use MAME's change_pc function or internal?
//#define CZ80_USE_MAME_CHANGE_PC 


#define CZ80_SF_SFT     7
#define CZ80_ZF_SFT     6
#define CZ80_YF_SFT     5
#define CZ80_HF_SFT     4
#define CZ80_XF_SFT     3
#define CZ80_PF_SFT     2
#define CZ80_VF_SFT     2
#define CZ80_NF_SFT     1
#define CZ80_CF_SFT     0

#define CZ80_SF         (1 << CZ80_SF_SFT)
#define CZ80_ZF         (1 << CZ80_ZF_SFT)
#define CZ80_YF         (1 << CZ80_YF_SFT)
#define CZ80_HF         (1 << CZ80_HF_SFT)
#define CZ80_XF         (1 << CZ80_XF_SFT)
#define CZ80_PF         (1 << CZ80_PF_SFT)
#define CZ80_VF         (1 << CZ80_VF_SFT)
#define CZ80_NF         (1 << CZ80_NF_SFT)
#define CZ80_CF         (1 << CZ80_CF_SFT)

#define CZ80_IFF_SFT    CZ80_PF_SFT
#define CZ80_IFF        CZ80_PF

#define CZ80_HAS_INT    CZ80_IFF
#define CZ80_HAS_NMI    0x08

#define CZ80_RUNNING    0x10
#define CZ80_HALTED     0x20
#define CZ80_FAULTED    0x80
#define CZ80_DISABLE    0x40


typedef u32  CZ80_FASTCALL CZ80_READ(u32 adr);
typedef void CZ80_FASTCALL CZ80_WRITE(u32 adr, u32 data);

typedef void CZ80_FASTCALL CZ80_RETI_CALLBACK();
typedef s32  CZ80_FASTCALL CZ80_INT_CALLBACK(s32 param);

typedef union
{
    u8 B;
    s8 SB;
} union8;

typedef union
{
	struct
	{
#if CZ80_LITTLE_ENDIAN
		u8 L;
		u8 H;
#else
		u8 H;
		u8 L;
#endif
	} B;
	struct
	{
#if CZ80_LITTLE_ENDIAN
		s8 L;
		s8 H;
#else
		s8 H;
		s8 L;
#endif
	} SB;
	u16 W;
	s16 SW;
} union16;

typedef struct
{
    union
    {
        u8      r8[8];
        union16 r16[4];
        struct
        {
            union16 BC;         // 32 bytes aligned
            union16 DE;
            union16 HL;
            union16 FA;
        };
    };
    
    union16 IX;
    union16 IY;
    union16 SP;
    u8      *PC;
    
    union16 BC2;
    union16 DE2;
    union16 HL2;
    union16 FA2;

	union16 R;
	union16 IFF;
	
	u8 I;
	u8 IM;
	u8 IntVect;
	u8 Status;

	u32 BasePC;
	u32 CycleIO;
	
	u32 CycleToDo;     // 32 bytes aligned
	u32 CycleSup;
} cz80_struc;



/*************************/
/* Publics Z80 variables */
/*************************/

//extern cz80_struc CZ80;


/*************************/
/* Publics Z80 functions */
/*************************/

void    Cz80_Init(cz80_struc *cpu);
u32     Cz80_Reset(cz80_struc *cpu);

u32     Cz80_Read_Byte(cz80_struc *cpu, u32 adr);
u32     Cz80_Read_Word(cz80_struc *cpu, u32 adr);
void    Cz80_Write_Byte(cz80_struc *cpu, u32 adr, u32 data);
void    Cz80_Write_Word(cz80_struc *cpu, u32 adr, u32 data);

void    CZ80_FASTCALL Cz80_Enable(cz80_struc *cpu);
void    CZ80_FASTCALL Cz80_Disable(cz80_struc *cpu);

s32     CZ80_FASTCALL Cz80_Exec(cz80_struc *cpu, s32 cycles);

void    CZ80_FASTCALL Cz80_Set_IRQ(cz80_struc *cpu, s32 vector);
void    CZ80_FASTCALL Cz80_Set_NMI(cz80_struc *cpu);
void    CZ80_FASTCALL Cz80_Clear_IRQ(cz80_struc *cpu);
void    CZ80_FASTCALL Cz80_Clear_NMI(cz80_struc *cpu);

s32     CZ80_FASTCALL Cz80_Get_CycleToDo(cz80_struc *cpu);
s32     CZ80_FASTCALL Cz80_Get_CycleRemaining(cz80_struc *cpu);
s32     CZ80_FASTCALL Cz80_Get_CycleDone(cz80_struc *cpu);
void    CZ80_FASTCALL Cz80_End_Execute(cz80_struc *cpu);
void    CZ80_FASTCALL Cz80_Waste_Cycle(cz80_struc *cpu, s32 cycle);

u32     CZ80_FASTCALL Cz80_Get_BC(cz80_struc *cpu);
u32     CZ80_FASTCALL Cz80_Get_DE(cz80_struc *cpu);
u32     CZ80_FASTCALL Cz80_Get_HL(cz80_struc *cpu);
u32     CZ80_FASTCALL Cz80_Get_AF(cz80_struc *cpu);

u32     CZ80_FASTCALL Cz80_Get_BC2(cz80_struc *cpu);
u32     CZ80_FASTCALL Cz80_Get_DE2(cz80_struc *cpu);
u32     CZ80_FASTCALL Cz80_Get_HL2(cz80_struc *cpu);
u32     CZ80_FASTCALL Cz80_Get_AF2(cz80_struc *cpu);

u32     CZ80_FASTCALL Cz80_Get_IX(cz80_struc *cpu);
u32     CZ80_FASTCALL Cz80_Get_IY(cz80_struc *cpu);
u32     CZ80_FASTCALL Cz80_Get_SP(cz80_struc *cpu);
u32     CZ80_FASTCALL Cz80_Get_PC(cz80_struc *cpu);

u32     CZ80_FASTCALL Cz80_Get_R(cz80_struc *cpu);
u32     CZ80_FASTCALL Cz80_Get_IFF(cz80_struc *cpu);
u32     CZ80_FASTCALL Cz80_Get_IM(cz80_struc *cpu);
u32     CZ80_FASTCALL Cz80_Get_I(cz80_struc *cpu);

void    CZ80_FASTCALL Cz80_Set_BC(cz80_struc *cpu, u32 value);
void    CZ80_FASTCALL Cz80_Set_DE(cz80_struc *cpu, u32 value);
void    CZ80_FASTCALL Cz80_Set_HL(cz80_struc *cpu, u32 value);
void    CZ80_FASTCALL Cz80_Set_AF(cz80_struc *cpu, u32 value);

void    CZ80_FASTCALL Cz80_Set_BC2(cz80_struc *cpu, u32 value);
void    CZ80_FASTCALL Cz80_Set_DE2(cz80_struc *cpu, u32 value);
void    CZ80_FASTCALL Cz80_Set_HL2(cz80_struc *cpu, u32 value);
void    CZ80_FASTCALL Cz80_Set_AF2(cz80_struc *cpu, u32 value);

void    CZ80_FASTCALL Cz80_Set_IX(cz80_struc *cpu, u32 value);
void    CZ80_FASTCALL Cz80_Set_IY(cz80_struc *cpu, u32 value);
void    CZ80_FASTCALL Cz80_Set_SP(cz80_struc *cpu, u32 value);
void    CZ80_FASTCALL Cz80_Set_PC(cz80_struc *cpu, u32 value);

void    CZ80_FASTCALL Cz80_Set_R(cz80_struc *cpu, u32 value);
void    CZ80_FASTCALL Cz80_Set_IFF(cz80_struc *cpu, u32 value);
void    CZ80_FASTCALL Cz80_Set_IM(cz80_struc *cpu, u32 value);
void    CZ80_FASTCALL Cz80_Set_I(cz80_struc *cpu, u32 value);

#if defined(__cplusplus)
}
#endif

#endif  // _CZ80_H_

