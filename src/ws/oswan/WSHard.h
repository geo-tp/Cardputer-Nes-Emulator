/*
$Date: 2009-10-30 05:26:46 +0100 (ven., 30 oct. 2009) $
$Rev: 71 $
*/

#ifndef WSHARD_H_
#define WSHARD_H_

#define LCD_MAIN_W 224
#define LCD_MAIN_H 144
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 135

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

typedef void  (*WriteMemFn) (DWORD A, BYTE V);
extern WriteMemFn WriteMemFnTable[0x10];
BYTE  ReadMem(DWORD A);
void  WriteMem(DWORD A, BYTE V);
void  WriteIO(DWORD A, BYTE V);
BYTE  ReadIO(DWORD A);

#define cpu_readop(A)               (ReadMem(A))
#define cpu_readop_arg(A)           (ReadMem(A))
#define cpu_readmem20(A)            (ReadMem(A))
#define cpu_writemem20(A, D)        (WriteMem((A), (BYTE)(D)))
#define cpu_readport(port)          (ReadIO((port)))
#define cpu_writeport(port, val)    (WriteIO((port), (BYTE)(val)))
#define cpu_readport16(port)        (ReadIO((port)))
#define cpu_writeport16(port, val)  (WriteIO((port), (BYTE)(val)))

//---------------------------------------------------------------------------
// I/O
//---------------------------------------------------------------------------
#define DSPCTL  IO[0x00]        //
#define BORDER  IO[0x01]        //
#define RSTRL   IO[0x02]        // 
#define RSTRLC  IO[0x03]        // 
#define SPRTAB  IO[0x04]        // 
#define SPRBGN  IO[0x05]        // 
#define SPRCNT  IO[0x06]        // 
#define SCRMAP  IO[0x07]        // 
#define SCR2WL  IO[0x08]        // foreground X
#define SCR2WT  IO[0x09]        // foreground Y
#define SCR2WR  IO[0x0A]        // foreground X
#define SCR2WB  IO[0x0B]        // foreground Y
#define SPRWL   IO[0x0C]        // X
#define SPRWT   IO[0x0D]        // Y
#define SPRWR   IO[0x0E]        // X
#define SPRWB   IO[0x0F]        // Y
#define SCR1X   IO[0x10]        // 
#define SCR1Y   IO[0x11]        // 
#define SCR2X   IO[0x12]        // foreground X
#define SCR2Y   IO[0x13]        // foreground Y
#define LCDSLP  IO[0x14]        // LCD
#define LCDSEG  IO[0x15]        // segment

#define COL     (BYTE*)(IO+0x1C)    // 
#define PAL     (BYTE*)(IO+0x20)    // 

#define DMASRC  (*(DWORD*)(IO+0x40))    //IO[]��4�o�C�g���E�ɂ��邱�Ƃ��K�v
#define DMADST  (*(WORD*)(IO+0x44)) //
#define DMACNT  (*(WORD*)(IO+0x46)) //

#define SDMASA  (*(WORD*)(IO+0x4A)) //
#define SDMASB  IO[0x4C]        // 
#define SDMACNT (*(WORD*)(IO+0x4E)) //
#define SDMACTL IO[0x52]        // 

#define COLCTL  IO[0x60]        // 

#define SNDP    IO[0x80]        // 
#define SNDV    IO[0x88]        // 
#define SNDSWP  IO[0x8C]        // 
#define SWPSTP  IO[0x8D]        // 
#define NSCTL   IO[0x8E]        // 
#define WAVDTP  IO[0x8F]        // WAVE
#define SNDMOD  IO[0x90]        // 
#define SNDOUT  IO[0x91]        // 
#define NCSR    (*(WORD*)(IO+0x92))  //

#define HWARCH  IO[0xA0]        // 

#define TIMCTL  IO[0xA2]        // 
#define HPRE    (*(WORD*)(IO+0xA4))  //
#define VPRE    (*(WORD*)(IO+0xA6))  //
#define HCNT    (*(WORD*)(IO+0xA8))  //
#define VCNTL   (*(WORD*)(IO+0xAA))  //  L
#define VCNTH   (*(WORD*)(IO+0xAC))  //  H

#define IRQBSE  IO[0xB0]        // 
#define COMDT   IO[0xB1]        // 
#define IRQENA  IO[0xB2]        // 
#define COMCTL  IO[0xB3]        // 
#define KEYCTL  IO[0xB5]        // 
#define IRQACK  IO[0xB6]        // 

#define EEPDTL  IO[0xBA]        // 
#define EEPDTH  IO[0xBA]        // 
#define EEPADL  IO[0xBA]        // 
#define EEPADH  IO[0xBA]        // 
#define EEPCTL  IO[0xBA]        // 

#define BNK1SEL IO[0xC1]        // 
#define BNK2SEL IO[0xC2]        // 
#define BNK3SEL IO[0xC3]        // 

//---------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------
#define TXD_IFLAG 0x01
#define KEY_IFLAG 0x02
#define CRT_IFLAG 0x04
#define RXD_IFLAG 0x08
#define RST_IFLAG 0x10
#define VTM_IFLAG 0x20
#define VBB_IFLAG 0x40
#define HTM_IFLAG 0x80

//---------------------------------------------------------------------------
#endif
