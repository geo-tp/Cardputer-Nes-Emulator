/********************************************************************************/
/*                                                                              */
/* CZ80 exec include source file                                                */
/* C Z80 emulator version 0.92                                                  */
/* Copyright 2004-2005 Stéphane Dallongeville                                   */
/*                                                                              */
/********************************************************************************/

s32 Z80_ICount;

s32 CZ80_FASTCALL Cz80_Exec(cz80_struc *cpu, s32 cycles)
{
#if CZ80_USE_JUMPTABLE
    #include "cz80jmp.inc"
#endif

#if __x86__
    register cz80_struc *CPU asm ("ebx");
    register u8 *PC asm ("esi");
    register u32 Opcode asm ("edx");
#else
    cz80_struc *CPU;
    u8 *PC;
    u32 Opcode;
#endif

    CPU = cpu;
    PC = CPU->PC;

    if (CPU->Status & (CZ80_RUNNING | CZ80_DISABLE | CZ80_FAULTED))
    {
        return (CPU->Status | 0x80000000);
    }

    CPU->CycleToDo = Z80_ICount = cycles;
    CPU->CycleSup = 0;
    CPU->Status |= CZ80_RUNNING;
    
Cz80_Check_Int:
    // check for interrupt
    if (CPU->Status & (zIFF1 | CZ80_HAS_NMI))
    {
        u32 newPC;

        if (CPU->Status & CZ80_HAS_NMI)
        {
            // NMI
            CPU->Status &= ~(CZ80_HALTED | CZ80_HAS_NMI);
            zIFF1 = 0;
            newPC = 0x66;
        }
        else
        {
            // INT
            CPU->Status &= ~(CZ80_HALTED | CZ80_HAS_INT);
            zIFF= 0;

            // IM = 1
            if (zIM == 1) newPC = 0x38;
            else
            {
                u32 adr;

                Opcode = CPU->IntVect & 0xFF;
                // IM = 0
                if (zIM == 0)
			goto Cz80_Exec_direct;
                // IM = 2
                adr = Opcode | (zI << 8);
               	READ_WORD(adr, newPC)
#if CZ80_IRQ_CYCLES
                Z80_ICount -= 8;
#endif
            }
        }

        // set new PC
        {
            u32 src = zRealPC;

            PUSH_16(src)
            SET_PC(newPC);
#if CZ80_IRQ_CYCLES
            Z80_ICount -= 11;
#endif
        }
    }

    // if some cycles left
    if ((Z80_ICount += CPU->CycleSup) > 0)
    {
        CPU->CycleSup = 0;
        if (!(CPU->Status & CZ80_HALTED)) goto Cz80_Exec;

        // CPU halted
        Z80_ICount = 0;
    }
    
Cz80_Exec_Really_End:
    // no more cycles, end execution
    CPU->Status &= ~CZ80_RUNNING;
    CPU->PC = PC;

    // number of executed cycles
    Z80_ICount = CPU->CycleToDo - Z80_ICount;

    // update R register
    zR = (zR + (Z80_ICount >> 2)) & 0x7F;

    return Z80_ICount;

#if CZ80_SIZE_OPT
Cz80_Exec_Check:
    if (Z80_ICount <= 0) goto Cz80_Check_Int;
#endif

Cz80_Exec:
    {
        Opcode = REAL_FETCH_BYTE;
Cz80_Exec_direct:
        {
        union16 *data = pzHL;
        #include "cz80_op.inc"
     }
    }
}
