/*
 * Copyright (C) 2003 Texas Instruments Incorporated
 * All Rights Reserved
 */
/*
 *---------main_dma4.c---------
 * This example places the MCBSP in digital loopback mode and
 * syncs MCBSP receive with DMA channel 4 and MCBSP transmit 
 * with DMA channel 5.                                       
 * The example uses predefined CSL macros and symbolic       
 * constants to create the initialization values needed for  
 * the MCBSP and DMA control registers to effect the transfer
 */
#include <stdio.h>

#include <csl_mcbsp.h>
#include <csl_dma.h>
#include <csl_irq.h>

//---------Global constants---------
#define N       128

//---------Global data definition---------

/* Define transmit and receive buffers */
#pragma DATA_SECTION(xmt,"dmaMem")
Uint16 xmt[N];
#pragma DATA_SECTION(rcv,"dmaMem")
Uint16 rcv[N];


  MCBSP_Config ConfigLoopBack16= {
  MCBSP_SPCR1_RMK(
    MCBSP_SPCR1_DLB_ON,                    /* DLB    = 1 */
    MCBSP_SPCR1_RJUST_RZF,                 /* RJUST  = 0 */
    MCBSP_SPCR1_CLKSTP_DISABLE,            /* CLKSTP = 0 */
    MCBSP_SPCR1_DXENA_NA,                  /* DXENA  = 0 */
    MCBSP_SPCR1_ABIS_DISABLE,              /* ABIS   = 0 */
    MCBSP_SPCR1_RINTM_RRDY,                /* RINTM  = 0 */
    0,                                     /* RSYNCER = 0 */
    MCBSP_SPCR1_RRST_DISABLE               /* RRST   = 0 */
   ),
    MCBSP_SPCR2_RMK(
    MCBSP_SPCR2_FREE_NO,                   /* FREE   = 0 */
    MCBSP_SPCR2_SOFT_NO,                   /* SOFT   = 0 */
    MCBSP_SPCR2_FRST_RESET,                /* FRST   = 0 */
    MCBSP_SPCR2_GRST_RESET,                /* GRST   = 0 */
    MCBSP_SPCR2_XINTM_XRDY,                /* XINTM  = 0 */
    0,                                     /* XSYNCER = N/A */
    MCBSP_SPCR2_XRST_DISABLE               /* XRST   = 0 */
   ),
  MCBSP_RCR1_RMK( 
  MCBSP_RCR1_RFRLEN1_OF(0),                /* RFRLEN1 = 0 */
  MCBSP_RCR1_RWDLEN1_16BIT                 /* RWDLEN1 = 5 */
  ),
 MCBSP_RCR2_RMK(    
    MCBSP_RCR2_RPHASE_SINGLE,              /* RPHASE  = 0 */
    MCBSP_RCR2_RFRLEN2_OF(0),              /* RFRLEN2 = 0 */
    MCBSP_RCR2_RWDLEN2_8BIT,               /* RWDLEN2 = 0 */
    MCBSP_RCR2_RCOMPAND_MSB,               /* RCOMPAND = 0 */
    MCBSP_RCR2_RFIG_YES,                   /* RFIG    = 0 */
    MCBSP_RCR2_RDATDLY_0BIT                /* RDATDLY = 0 */
    ),  
   MCBSP_XCR1_RMK(    
    MCBSP_XCR1_XFRLEN1_OF(0),              /* XFRLEN1 = 0 */ 
    MCBSP_XCR1_XWDLEN1_16BIT               /* XWDLEN1 = 5 */
    
 ),   
 MCBSP_XCR2_RMK(   
    MCBSP_XCR2_XPHASE_SINGLE,              /* XPHASE  = 0 */
    MCBSP_XCR2_XFRLEN2_OF(0),              /* XFRLEN2 = 0 */
    MCBSP_XCR2_XWDLEN2_8BIT,               /* XWDLEN2 = 0 */
    MCBSP_XCR2_XCOMPAND_MSB,               /* XCOMPAND = 0 */
    MCBSP_XCR2_XFIG_YES,                   /* XFIG    = 0 */
    MCBSP_XCR2_XDATDLY_0BIT                /* XDATDLY = 0 */
  ),            
 MCBSP_SRGR1_RMK( 
   MCBSP_SRGR1_FWID_OF(1),                /* FWID    = 1 */
   MCBSP_SRGR1_CLKGDV_OF(1)               /* CLKGDV  = 1 */
 ),   
 MCBSP_SRGR2_RMK(  
    MCBSP_SRGR2_GSYNC_FREE,                /* FREE    = 0 */
    MCBSP_SRGR2_CLKSP_RISING,              /* CLKSP   = 0 */
    MCBSP_SRGR2_CLKSM_INTERNAL,            /* CLKSM   = 1 */
    MCBSP_SRGR2_FSGM_DXR2XSR,              /* FSGM    = 0 */
    MCBSP_SRGR2_FPER_OF(15)                /* FPER    = 0 */
 ),  
 MCBSP_MCR1_DEFAULT,
 MCBSP_MCR2_DEFAULT, 
 MCBSP_PCR_RMK(
   MCBSP_PCR_XIOEN_SP,                     /* XIOEN    = 0   */
   MCBSP_PCR_RIOEN_SP,                     /* RIOEN    = 0   */
   MCBSP_PCR_FSXM_INTERNAL,                /* FSXM     = 1   */
   MCBSP_PCR_FSRM_EXTERNAL,                /* FSRM     = 0   */
   MCBSP_PCR_CLKXM_OUTPUT,                 /* CLKXM    = 1   */
   MCBSP_PCR_CLKRM_INPUT,                  /* CLKRM    = 0   */
   MCBSP_PCR_SCLKME_NO,                    /* SCLKME   = 0   */
   0,                                      /* DXSTAT = N/A   */
   MCBSP_PCR_FSXP_ACTIVEHIGH,              /* FSXP     = 0   */
   MCBSP_PCR_FSRP_ACTIVEHIGH,              /* FSRP     = 0   */
   MCBSP_PCR_CLKXP_RISING,                 /* CLKXP    = 0   */
   MCBSP_PCR_CLKRP_FALLING                 /* CLKRP    = 0   */
 ),
 MCBSP_RCERA_DEFAULT, 
 MCBSP_RCERB_DEFAULT, 
 MCBSP_RCERC_DEFAULT, 
 MCBSP_RCERD_DEFAULT, 
 MCBSP_RCERE_DEFAULT, 
 MCBSP_RCERF_DEFAULT, 
 MCBSP_RCERG_DEFAULT, 
 MCBSP_RCERH_DEFAULT, 
 MCBSP_XCERA_DEFAULT,
 MCBSP_XCERB_DEFAULT,
 MCBSP_XCERC_DEFAULT,
 MCBSP_XCERD_DEFAULT,  
 MCBSP_XCERE_DEFAULT,
 MCBSP_XCERF_DEFAULT,  
 MCBSP_XCERG_DEFAULT,
 MCBSP_XCERH_DEFAULT
};     

DMA_Config  dmaRcvConfig = { 
  DMA_DMACSDP_RMK(
    DMA_DMACSDP_DSTBEN_NOBURST,
    DMA_DMACSDP_DSTPACK_OFF,
    DMA_DMACSDP_DST_DARAMPORT1,
    DMA_DMACSDP_SRCBEN_NOBURST,
    DMA_DMACSDP_SRCPACK_OFF,
    DMA_DMACSDP_SRC_PERIPH,
    DMA_DMACSDP_DATATYPE_16BIT
  ),                                       /* DMACSDP  */
  DMA_DMACCR_RMK(
    DMA_DMACCR_DSTAMODE_POSTINC,
    DMA_DMACCR_SRCAMODE_CONST,
    DMA_DMACCR_ENDPROG_ON,
    DMA_DMACCR_WP_DEFAULT,
    DMA_DMACCR_REPEAT_OFF,
    DMA_DMACCR_AUTOINIT_OFF,
    DMA_DMACCR_EN_STOP,
    DMA_DMACCR_PRIO_LOW,
    DMA_DMACCR_FS_DISABLE,
    DMA_DMACCR_SYNC_REVT1
  ),                                       /* DMACCR   */
  DMA_DMACICR_RMK(
    DMA_DMACICR_AERRIE_ON,
    DMA_DMACICR_BLOCKIE_OFF,
    DMA_DMACICR_LASTIE_OFF,
    DMA_DMACICR_FRAMEIE_ON,
    DMA_DMACICR_FIRSTHALFIE_OFF,
    DMA_DMACICR_DROPIE_OFF,
    DMA_DMACICR_TIMEOUTIE_OFF
  ),                                       /* DMACICR  */
    (DMA_AdrPtr)(MCBSP_ADDR(DRR11)),        /* DMACSSAL */
    0,                                     /* DMACSSAU */
    (DMA_AdrPtr)&rcv,                      /* DMACDSAL */
    0,                                     /* DMACDSAU */
    N,                                     /* DMACEN   */
    1,                                     /* DMACFN   */
    0,                                     /* DMACFI   */
    0                                      /* DMACEI   */
};

DMA_Config  dmaXmtConfig = { 
  DMA_DMACSDP_RMK(
    DMA_DMACSDP_DSTBEN_NOBURST,
    DMA_DMACSDP_DSTPACK_OFF,
    DMA_DMACSDP_DST_PERIPH,
    DMA_DMACSDP_SRCBEN_NOBURST,
    DMA_DMACSDP_SRCPACK_OFF,
    DMA_DMACSDP_SRC_DARAMPORT0,
    DMA_DMACSDP_DATATYPE_16BIT
  ),                                       /* DMACSDP  */
  DMA_DMACCR_RMK(
    DMA_DMACCR_DSTAMODE_CONST,
    DMA_DMACCR_SRCAMODE_POSTINC,
    DMA_DMACCR_ENDPROG_ON,
    DMA_DMACCR_WP_DEFAULT,
    DMA_DMACCR_REPEAT_OFF,
    DMA_DMACCR_AUTOINIT_OFF,
    DMA_DMACCR_EN_STOP,
    DMA_DMACCR_PRIO_LOW,
    DMA_DMACCR_FS_DISABLE,
    DMA_DMACCR_SYNC_XEVT1
  ),                                       /* DMACCR   */
  DMA_DMACICR_RMK(
    DMA_DMACICR_AERRIE_ON,    
    DMA_DMACICR_BLOCKIE_OFF,
    DMA_DMACICR_LASTIE_OFF,
    DMA_DMACICR_FRAMEIE_ON,
    DMA_DMACICR_FIRSTHALFIE_OFF,
    DMA_DMACICR_DROPIE_OFF,
    DMA_DMACICR_TIMEOUTIE_OFF
  ),                                       /* DMACICR  */
    (DMA_AdrPtr)&xmt[1],                      /* DMACSSAL */
    0,                                     /* DMACSSAU */
    (DMA_AdrPtr)(MCBSP_ADDR(DXR11)),       /* DMACDSAL */
    0,                                     /* DMACDSAU */
    N,                                     /* DMACEN   */
    1,                                     /* DMACFN   */
    0,                                     /* DMACFI   */
    0                                      /* DMACEI   */
};

/* Define a DMA_Handle object to be used with DMA_open function */
DMA_Handle hDmaRcv, hDmaXmt;

/* Define a MCBSP_Handle object to be used with MCBSP_open function */
MCBSP_Handle hMcbsp;

volatile Uint16 transferComplete = FALSE;
Uint16 err = 0;
Uint16 old_intm;
Uint16 xmtEventId, rcvEventId;

//---------Function prototypes---------

/* Reference the start of the interrupt vector table */
/* This symbol is defined in file vectors.s55        */
extern void VECSTART(void);

/* Protoype for interrupt functions */
interrupt void dmaXmtIsr(void);
interrupt void dmaRcvIsr(void);
void taskFxn(void);

//---------main routine---------
void main(void)
{
    Uint16 i;

    /* Initialize CSL library - This is REQUIRED !!! */
    CSL_init();

    /* Set IVPH/IVPD to start of interrupt vector table */
    IRQ_setVecs((Uint32)(&VECSTART));

    for (i = 0; i <= N - 1; i++) {  
        xmt[i] =  i + 1;
        rcv[i] = 0;
    }

    /* Call function to effect transfer */
    taskFxn();
}

void taskFxn(void)
{
    Uint16 srcAddrHi, srcAddrLo;
    Uint16 dstAddrHi, dstAddrLo;
    Uint16 i;
 
    /* By default, the TMS320C55xx compiler assigns all data symbols word */
    /* addresses. The DMA however, expects all addresses to be byte       */
    /* addresses. Therefore, we must shift the address by 2 in order to   */
    /* change the word address to a byte address for the DMA transfer.    */ 
    srcAddrHi = (Uint16)(((Uint32)(MCBSP_ADDR(DRR11))) >> 15) & 0xFFFFu;
    srcAddrLo = (Uint16)(((Uint32)(MCBSP_ADDR(DRR11))) << 1) & 0xFFFFu;
    dstAddrHi = (Uint16)(((Uint32)(&rcv)) >> 15) & 0xFFFFu;
    dstAddrLo = (Uint16)(((Uint32)(&rcv)) << 1) & 0xFFFFu;

    dmaRcvConfig.dmacssal = (DMA_AdrPtr)srcAddrLo;
    dmaRcvConfig.dmacssau = srcAddrHi;
    dmaRcvConfig.dmacdsal = (DMA_AdrPtr)dstAddrLo;
    dmaRcvConfig.dmacdsau = dstAddrHi;

	srcAddrHi = (Uint16)(((Uint32)(&xmt[0])) >> 15) & 0xFFFFu;
    srcAddrLo = (Uint16)(((Uint32)(&xmt[0])) << 1) & 0xFFFFu;
    dstAddrHi = (Uint16)(((Uint32)(MCBSP_ADDR(DXR11))) >> 15) & 0xFFFFu;
    dstAddrLo = (Uint16)(((Uint32)(MCBSP_ADDR(DXR11))) << 1) & 0xFFFFu;

    dmaXmtConfig.dmacssal = (DMA_AdrPtr)srcAddrLo;
    dmaXmtConfig.dmacssau = srcAddrHi;
    dmaXmtConfig.dmacdsal = (DMA_AdrPtr)dstAddrLo;
    dmaXmtConfig.dmacdsau = dstAddrHi;


    /* Open MCBSP Port 1 and set registers to their power on defaults */
    hMcbsp = MCBSP_open(MCBSP_PORT1, MCBSP_OPEN_RESET);
    

    /* Open DMA channels 4 & 5 and set regs to power on defaults */
    hDmaRcv = DMA_open(DMA_CHA4,DMA_OPEN_RESET);
    hDmaXmt = DMA_open(DMA_CHA5,DMA_OPEN_RESET);  

    /* Get interrupt event associated with DMA receive and transmit */
    xmtEventId = DMA_getEventId(hDmaXmt);
    rcvEventId = DMA_getEventId(hDmaRcv);
    
    /* Temporarily disable interrupts and clear any pending */
    /* interrupts for MCBSP transmit */
    old_intm = IRQ_globalDisable();
    
    /* Clear any pending interrupts for DMA channels */
    IRQ_clear(xmtEventId);
    IRQ_clear(rcvEventId);

    /* Enable DMA interrupt in IER register */
    IRQ_enable(xmtEventId);
    IRQ_enable(rcvEventId);

    /* Place DMA interrupt service addresses at associate vector */
    IRQ_plug(xmtEventId,&dmaXmtIsr);
    IRQ_plug(rcvEventId,&dmaRcvIsr);

    /* Write values from configuration structure to MCBSP control regs */
    MCBSP_config(hMcbsp, &ConfigLoopBack16); 
    
    /* Write values from configuration structure to DMA control regs */
    DMA_config(hDmaRcv,&dmaRcvConfig);
    DMA_config(hDmaXmt,&dmaXmtConfig);
  
   /* Enable all maskable interrupts */
    IRQ_globalEnable();

    /* Start Sample Rate Generator and Enable Frame Sync */
    MCBSP_start(hMcbsp,
                MCBSP_SRGR_START | MCBSP_SRGR_FRAMESYNC,
                0x300u);

    /* Enable DMA */
    DMA_start(hDmaRcv);
    DMA_start(hDmaXmt);

    /* Take MCBSP transmit and receive out of reset */
    MCBSP_start(hMcbsp,
                MCBSP_XMIT_START | MCBSP_RCV_START,
                0u);
  
   /* Wait for DMA transfer to be complete */
    while (!transferComplete){
        ;   
    }
   
    /*------------------------------------------*\
     * Compare values 
    \*------------------------------------------*/   
    for(i = 0; i <= N - 1; i++){
        if (rcv[i] != xmt[i]){
            ++err;
            break;
       }
    }

    printf ("%s\n",err?"TEST FAILED" : "TEST PASSED");

    /* Restore status of global interrupt enable flag */
    IRQ_globalRestore(old_intm);
        
    /* We're done with MCBSP and DMA , so close them */
    MCBSP_close(hMcbsp);
    DMA_close(hDmaRcv);
    DMA_close(hDmaXmt);                     
}

interrupt void dmaXmtIsr(void) {
   DMA_stop(hDmaXmt);
   IRQ_disable(xmtEventId);
}

interrupt void dmaRcvIsr(void) {
   DMA_stop(hDmaRcv);
   IRQ_disable(rcvEventId);
   transferComplete = TRUE;
}
