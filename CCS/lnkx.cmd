
-stack    01000      /* Primary stack size   */
-sysstack 0x1000      /* Secondary stack size */
-heap     0x1000      /* Heap area size       */

-c                    /* Use C linking conventions: auto-init vars at runtime */
-u _Reset             /* Force load of reset interrupt handler                */

/* SPECIFY THE SYSTEM MEMORY MAP */

MEMORY
{
 PAGE 0:  /* ---- Unified Program/Data Address Space ---- */

  MMR    (RWIX): origin = 0x000000, length = 0x0000C0  /* MMRs */
  BTRSVD (RWIX): origin = 0x0000C0, length = 0x000240  /* Reserved for Boot Loader */
  DARAM  (RWIX): origin = 0x000300, length = 0x00FB00  /* 64KB - MMRs - VECS*/
  VECS   (RWIX): origin = 0x00FE00, length = 0x000200  /* 256 bytes Vector Table */
  CE0          : origin = 0x010000, length = 0x3f0000  /* 4M minus 64K Bytes */
  CE1          : origin = 0x400000, length = 0x400000
  CE2          : origin = 0x800000, length = 0x400000
  CE3          : origin = 0xC00000, length = 0x3F8000
  PDROM   (RIX): origin = 0xFF8000, length = 0x008000  /*  32KB */

 PAGE 2:  /* -------- 64K-word I/O Address Space -------- */

  IOPORT (RWI) : origin = 0x000000, length = 0x020000
}



SECTIONS
{
   .text     >  DARAM align(32) fill = 20h { * (.text)  }

   .stack    >  DARAM align(32) fill = 00h
   .sysstack >  DARAM align(32) fill = 00h
   .csldata  >  DARAM align(64) fill = 00h
   .data     >  DARAM align(32) fill = 00h
   .bss      >  DARAM align(32) fill = 00h
   .const    >  DARAM align(32) fill = 00h
   .sysmem   >  DARAM
   .switch   >  DARAM
   .cinit    >  DARAM
   .pinit    >  DARAM align(32) fill = 00h
   .cio      >  DARAM align(32) fill = 00h
   .args     >  DARAM align(32) fill = 00h
	.vectors:  >  VECS

	dmaMem    >  DARAM align(32) fill = 00h


   .ioport   >  IOPORT PAGE 2         /* Global & static ioport vars */
}
