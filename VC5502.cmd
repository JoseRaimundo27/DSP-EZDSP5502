/******************************************************************************/
/* LNKX.CMD - COMMAND FILE FOR LINKING C PROGRAMS IN LARGE/HUGE MEMORY MODEL  */
/*                                                                            */
/* Usage:                                                                     */
/*  cl55 <src files> -z -o<out file> -m<map file> lnkx.cmd -l<RTS library>   */
/*                                                                            */
/* Description: This file is a sample command file that can be used for       */
/*              linking programs built with the C Compiler.  Use it as a      */
/*              guideline; you  may want to change the allocation scheme      */
/*              according to the size of your program and the memory layout   */
/*              of your target system.                                        */
/*                                                                            */
/*   Notes: (1) You must specify the directory in which <RTS library> is      */
/*              located.  Either add a "-i<directory>" line to this file      */
/*              file, or use the system environment variable C55X_C_DIR to    */
/*              specify a search path for the libraries.                      */
/*                                                                            */
/******************************************************************************/

-stack    0x2000      /* Primary stack size   */
-sysstack 0x1000      /* Secondary stack size */
-heap     0x2000      /* Heap area size       */

-c                    /* Use C linking conventions: auto-init vars at runtime */
-u _Reset             /* Force load of reset interrupt handler                */

/* SPECIFY THE SYSTEM MEMORY MAP */

MEMORY
{
  MMR    (RWIX): origin = 0x000000, length = 0x0000C0
  DARAM0 (RWIX): origin = 0x0000C0, length = 0x001F3F  /* 8 KB */
  DARAM  (RWIX): origin = 0x002000, length = 0x00DC00  /* resto da RAM interna */
  VECS   (RWIX): origin = 0x00FE00, length = 0x000200
  CE0          : origin = 0x010000, length = 0x3f0000
  CE1          : origin = 0x400000, length = 0x400000
  CE2          : origin = 0x800000, length = 0x400000
  CE3          : origin = 0xC00000, length = 0x3F8000
}

/* SPECIFY THE SECTIONS ALLOCATION INTO MEMORY */

SECTIONS
{
    .text      > DARAM align(32)
    .stack     > DARAM align(32)
    .sysstack  > DARAM align(32)
    .csldata   > DARAM align(32)
    .data      > DARAM align(32)
    .bss       > DARAM align(32)
    .const     > DARAM align(32)
    .sysmem    > DARAM
    .switch    > DARAM
    .cinit     > DARAM
    .pinit     > DARAM align(32)
    .cio       > DARAM align(32)
    .args      > DARAM align(32)
    vectors:   > VECS
    dmaMem     > DARAM0 align(256)
}
