/*
 * Copyright (C) 2003 Texas Instruments Incorporated
 * All Rights Reserved
 */
/*
 *---------dma4.cmd---------
 *
 */

MEMORY
{
	MMR		:	o = 0x000000		l = 0x0000c0
	DARAM0	:	o = 0x0000C0		l = 0x001F3F
	DARAM1	:	o = 0x002000		l = 0x005FFF
  /*
  	DARAM1	:	o = 0x002000		l = 0x001FFF
	DARAM2	:	o =	0x004000		l = 0x001FFF	
    DARAM3	:	o = 0x006000		l = 0x001FFF
  */
	DARAM4	:	o = 0x008000		l = 0x001FFF
	DARAM5	:	o = 0x00A000		l = 0x001FFF
	DARAM6	:	o = 0x00C000		l = 0x001FFF
	DARAM7	:	o = 0x00E000		l = 0x001FFF
	
	CE0		:	o = 0x010000		l = 0x3EFFFF
	CE1		:	o = 0x400000		l = 0x3FFFFF
	CE2		:	o = 0x800000		l = 0x3FFFFF
	CE3		:	o = 0xC00000		l = 0x3F7FFF
}

SECTIONS
{
	.cinit		>	DARAM0
	.text		>	DARAM1
	.stack		>	DARAM0
	.sysstack	>	DARAM0
	.sysmem		>	DARAM4
	.data		>	DARAM4
	.cio		>	DARAM0
	.bss		>	DARAM4
	.const		>	DARAM0
	.csldata	>	DARAM0
	dmaMem		>	DARAM0
}
